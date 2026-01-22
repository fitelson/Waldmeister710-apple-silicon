/*=================================================================================================================
  =                                                                                                               =
  =  Konsulat.c                                                                                                   =
  =                                                                                                               =
  =                                                                                                               =
  =  25.10.98  Thomas.                                                                                            =
  =                                                                                                               =
  =================================================================================================================*/


#include "Ausgaben.h"
#include "ClasHeuristics.h"
#include "DSBaumOperationen.h"
#include "Konsulat.h"
#include "Hauptkomponenten.h"
#include "KPVerwaltung.h"
#include "LispLists.h"
#include "MNF.h"
#include "NewClassification.h"
#include "Parameter.h"
#include "parser.h"
#include "PhilMarlow.h"
#include "Ordnungen.h"
#include "RUndEVerwaltung.h"
#include "Termpaare.h"
#include "WaldmeisterII.h"
#include "XFilesFuerPMh.h"
#include "Zielverwaltung.h"


/*=================================================================================================================*/
/*= I. Kern-Strategie                                                                                             =*/
/*=================================================================================================================*/

/* ------- 0. Apparat -------------------------------------------------------------------------------------------- */

BOOLEAN KO_Mehrstufig;
BOOLEAN KO_KonsultationErforderlich;
unsigned int KO_Stufe;
TermT KO_Kern,
      KO_Fixpunktkombinator;
static struct ListenTS InitialesZiel;
static SymbolT Skolemfunktion;

static PA_PQArtT ko_mnf_pq_art;

#define /*SymbolT*/ _     TO_ApplyOperator
#define /*SymbolT*/ combi SO_const1
#define /*SymbolT*/ sfp   SO_const2
#define /*SymbolT*/ x1    SO_ErstesVarSymb
#define /*SymbolT*/ eq    SO_EQ
#define /*SymbolT*/ true  SO_TRUE
#define /*SymbolT*/ false SO_FALSE


static void PassivesFaktumAufnehmen(TermT l, TermT r)
{
  KPV_Axiom(l,r);
}

static void AktivesFaktumWegwerfen(RegelOderGleichungsT Faktum)
{
  RE_FaktumToeten(Faktum);
}


/* ------- 1. Stufe 1 -------------------------------------------------------------------------------------------- */

static void SchwacheFixpunkteigenschaftAufnehmen(void)
{
  TermT l = TO_(x1),
    r = TO_(_,combi,x1);
  ListenT Ziel = car(PM_Ziele);
  InitialesZiel = *Ziel;
  Ziel->car = l;
  Ziel->cdr = r;
}


/* ------- 2. Stufe 2 -------------------------------------------------------------------------------------------- */

static void NegativeAktiveFaktenWegwerfen(void)
{
  RegelOderGleichungsT Faktum;
  for (Faktum = RE_ErstesFaktum(); Faktum; Faktum = RE_NaechstesFaktum())
    if (RE_IstUngleichung(Faktum))
      AktivesFaktumWegwerfen(Faktum);
}


static void FixedPositivAufnehmen(void)
{
  const SymbolT f = Skolemfunktion;
  TermT l = TO_(eq, f,x1, _,x1,f,x1),
        r = TO_(true);
  PassivesFaktumAufnehmen(l,r);
}


static void KernAbstrahieren(TermT *Term1, TermT *Term2)
{ 
  SymbolT x = SO_VorErstemVarSymb,
          y;
  UTermT t;
  TO_doSymbole(KO_Kern,y, {
    if (SO_VariableIstNeuer(y,x))
      x = y;
  });
  x = SO_NaechstesVarSymb(x);                     /* kann 2*BO_NeuesteBaumvariable um 1 ueberschreiten -> anmelden */
  BO_VariablenbaenkeAnpassen(x);
  TO_doStellen(KO_Kern,t, {
    if (SO_SymbGleich(TO_TopSymbol(t),combi))
      TO_SymbolBesetzen(t,x);
  });
  *Term1 = TO_(x);
  *Term2 = KO_Kern;
}


static void FixedNegativAufnehmen(TermT s, TermT t)
{
  Z_UngleichungEinmanteln(&s,&t);
  PassivesFaktumAufnehmen(s,t);
}


/* ------- 3. Stufe 3 -------------------------------------------------------------------------------------------- */

static void AktiveFaktenWegwerfen(void)
{
  RegelOderGleichungsT Faktum;
  for (Faktum = RE_ErstesFaktum(); Faktum; Faktum = RE_NaechstesFaktum())
    AktivesFaktumWegwerfen(Faktum);
}

static void KombinatorenAufnehmen(void)
{
  ListenT Index;
  for (Index=PM_Gleichungen; Index; Cdr(&Index)) {
    ListenT Eintrag = car(Index);
    TermT Flunder1 = TO_Termkopie(car(Eintrag)),
          Flunder2 = TO_Termkopie(cdr(Eintrag));
    PassivesFaktumAufnehmen(Flunder1,Flunder2);
  }
}

static void FixpunktkombinatorAufnehmen(void)
{
  TermT l = TO_(sfp),
        r = KO_Fixpunktkombinator;
  PassivesFaktumAufnehmen(l,r);
}


static void ZielStarkeFixpunkteigenschaftAufnehmen(void)
{
  TermT l = TO_(_,sfp,combi),
        r = TO_(_,combi,_,sfp,combi);
  ko_mnf_pq_art = PA_PQ_Art();
  PA_Set_PQ_Art(PA_PQfifo);
  MNFZ_InitAposteriori (); /* ??? */
  Z_EinZielEinfuegen(l,r,3,Z_MNF_Ziele);
}


/* ------- 4. Stufe 4 -------------------------------------------------------------------------------------------- */

static void StarkeFixpunkteigenschaftAufnehmen(void)
{
  TermT l = TO_(_,sfp,x1),
        r = TO_(_,x1,_,sfp,x1);
  PassivesFaktumAufnehmen(l,r);
}


static void InitialesZielAufnehmen(void)
{
  TermT l = TO_Termkopie(car(&InitialesZiel)),
        r = TO_Termkopie(cdr(&InitialesZiel));
  ListenT Ziel = car(PM_Ziele);
  PA_Set_PQ_Art(ko_mnf_pq_art); /* geht hier auch einfach ein PA_PQstack ? ! ! */
  /* oder braucht man das garnicht, nimmt stattdessen rnf ? */
  Z_EinZielEinfuegen(l,r,4,Z_Paramodulation);
#if 0
  TO_TermeLoeschen((TermT)car(Ziel),(TermT)cdr(Ziel));
#endif
  *Ziel = InitialesZiel;
}


static void EqXXIsTrueAufnehmen(void)
{
  TermT l = TO_(eq,x1,x1),
        r = TO_(true);
  PassivesFaktumAufnehmen(l,r);
}


/* ------- 5. Aggregationen --------------------------------------------------------------------------------- */

void KO_Konsultieren(void)
{
  if (KO_KonsultationErforderlich) {
    TermT s, t;
    KO_KonsultationErforderlich = FALSE;
    switch (++KO_Stufe) {

    case 1:
      ORD_OrdnungUmstellen(ordering_col4);
      if (KO_Mehrstufig)
        SchwacheFixpunkteigenschaftAufnehmen();
      break;

    case 2:
      KPV_PassiveFaktenWegwerfen();
      NegativeAktiveFaktenWegwerfen();
      FixedPositivAufnehmen();
      KernAbstrahieren(&s,&t);
      FixedNegativAufnehmen(s,t);
      break;

    case 3:
      KPV_PassiveFaktenWegwerfen();
      AktiveFaktenWegwerfen();
      ORD_OrdnungUmstellen(ordering_col3);
      KombinatorenAufnehmen();
      FixpunktkombinatorAufnehmen();
      ZielStarkeFixpunkteigenschaftAufnehmen();
      break;

    case 4:
      KPV_PassiveFaktenWegwerfen();
      AktiveFaktenWegwerfen();
      ORD_OrdnungUmstellen(ordering_col4);
      StarkeFixpunkteigenschaftAufnehmen();
      InitialesZielAufnehmen();
      EqXXIsTrueAufnehmen();
      SetAnzErfZusfuehrungenZiele(-1);                               /* Hack: Statistik in Zielverwaltung bereinigen */
      PA_SetGolem(FALSE);
      break;

    default:
      break;
    }
  }
}



/*=================================================================================================================*/
/*= II. Initialisierung                                                                                           =*/
/*=================================================================================================================*/


static BOOLEAN IstSFP(TermT l, TermT r, SymbolT f)
{
  return TO_TermEntspricht(l,4, _,x1,f,x1)
    && TO_TermEntspricht(r,7, _,f,x1,_,x1,f,x1);
}

static BOOLEAN ZielIstSFP(TermT l, TermT r)
{
  SymbolT f;
  SO_forConclusionSymbole(f)
    if (SO_Stelligkeit(f)==1)
      if (IstSFP(l,r,f) || IstSFP(r,l,f)) {
        Skolemfunktion = f;
        return TRUE;
      }
  return FALSE;
}


extern ClassifyProcT CGClassifyProc;

static void KernrelevanzTesten(void)
{
  eq_list Ziele = get_conclusions();
  if (!Ziele || eq_rest(Ziele))
    our_error("Option -kern requires exactly one hypothesis.");
  else {
    term L = eq_first_lhs(Ziele),
         R = eq_first_rhs(Ziele);
    TermT l,r;
    TO_PlattklopfenLRMitMalloc(L,R,&l,&r);
    KO_Mehrstufig = ZielIstSFP(l,r);
    TO_TermLoeschenM(l);
    TO_TermLoeschenM(r);
    KPV_CGHeuristikUmstellen(KO_Mehrstufig ? CH_Unifikationsmass : CH_AddWeight);
  }
}


void KO_InitAposteriori(void)
{
  KO_Kern = KO_Fixpunktkombinator = NULL;
  KO_Stufe = 0;
  Skolemfunktion = SO_ErstesVarSymb;
  if ((KO_KonsultationErforderlich = PA_KernAn())) {
    TO_KombinatorapparatAktivieren();
    KernrelevanzTesten();
    HK_SetUnfair();
    KO_Konsultieren();
  }
}
