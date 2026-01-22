/*=================================================================================================================
  =                                                                                                               =
  =  Praezedenzgenerator.c                                                                                        =
  =                                                                                                               =
  =                                                                                                               =
  =  26.07.97 Angelegt. Thomas.                                                                                   =
  =  19.06.98 Durch Ausduennen aus altem Ordnungsgenerator erzeugt. Thomas.                                       =
  =                                                                                                               =
  =================================================================================================================*/


#include "Ausgaben.h"
#include "compiler-extra.h"
#include "LispLists.h"
#include "PhilMarlow.h"
#include "Praezedenz.h"
#include "Praezedenzgenerator.h"
#include <string.h>
#include "SpeicherVerwaltung.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "XFiles.h"


/*=================================================================================================================*/
/*= II. Management von Prazedenzzeichenketten und Angeordnetheitspraedikat                                        =*/
/*=================================================================================================================*/

typedef struct {
  char *Zeichenkette;                               /* Praezedenz als Zeichenkette fuer modifizierte Kommandozeile */
  BOOLEAN *Angeordnet;                                           /* Ist Symbol schon in der Praezedenz angeordnet? */
} PraezedenzT;


static PraezedenzT NeuePraezedenz(void)
{
  SymbolT f;
  unsigned int Stringlaenge = (MaximaleBezeichnerLaenge+1                                /* Platz fuer Folge f.g.h */
    +MaximaleBezeichnerLaenge+2+LaengeNatZahl(UINT_MAX))*SO_AnzahlFunktionen;                      /* f=9:g=7:h=17 */
  PraezedenzT Praezedenz;                             /* ggf. auch Existenzquantifizierungshilfssymbole beinhaltet */
  Praezedenz.Zeichenkette = string_alloc(Stringlaenge);
  Praezedenz.Zeichenkette[0] = '\0';
  Praezedenz.Angeordnet = array_alloc(SO_AnzahlFunktionen,BOOLEAN);
  SO_forFktSymbole(f)
    Praezedenz.Angeordnet[f] = FALSE;
  return Praezedenz;
}


static void SymbolAnordnen_(PraezedenzT *Praezedenz, SymbolT f)
{
  if (!Praezedenz->Angeordnet[f]) {
    if (Praezedenz->Zeichenkette[0])
      strcat(Praezedenz->Zeichenkette,".");
    strcat(Praezedenz->Zeichenkette,IO_FktSymbName(f));
    Praezedenz->Angeordnet[f] = TRUE;
  }
}

#define /*void*/ SymbolAnordnen(/*SymbolT*/ f)                                                                      \
  SymbolAnordnen_(&Praezedenz,f)


static char *FertigePraezedenz(PraezedenzT *Praezedenz)
{
  our_dealloc(Praezedenz->Angeordnet);
  return Praezedenz->Zeichenkette;
}



/*=================================================================================================================*/
/*= III. Semantische Praezedenzen                                                                                 =*/
/*=================================================================================================================*/

typedef char Signatur_MusterT[maxAnzOperatoren+3+1];
typedef unsigned int Signatur_StelligkeitenT[maxAnzOperatoren+1];

static void SignaturTesten(Signatur_SymbolT Ist MAYBE_UNUSEDPARAM, Signatur_ZeichenT Ist_c MAYBE_UNUSEDPARAM, 
                           Signatur_ZeichenT Soll MAYBE_UNUSEDPARAM, 
                           Signatur_StelligkeitenT Stelligkeiten MAYBE_UNUSEDPARAM)
{
#if XF_TEST
  unsigned int LetzteSollStelligkeit,
               LaengeIst = strlen(Ist_c),
               LaengeSoll = strlen(Soll);
  BOOLEAN Mittenmang = FALSE;
  PM_SignaturTesten(Soll,4);
  test_error(LaengeIst<LaengeSoll,"zu kurze Signatur in semantischer Praezedenzerzeugung");
  test_error(LaengeIst>LaengeSoll,"zu lange Signatur in semantischer Praezedenzerzeugung");
  do {
    unsigned int IstStelligkeit = SO_Stelligkeit(*Ist),
                 SollStelligkeit = *Stelligkeiten;
    test_error(Mittenmang && LetzteSollStelligkeit<SollStelligkeit,
      "steigende Eintraege im Stelligkeitsvektor in semantischer Praezedenzerzeugung");
    test_error(IstStelligkeit!=SollStelligkeit,"unerwartete Stelligkeit in Signatur in semantischer Praezedenzerzeugung");
    LetzteSollStelligkeit = SollStelligkeit;
    Mittenmang = TRUE;
  } while (++Ist, ++Stelligkeiten, *++Soll);
#else
  ;
#endif
}


static void PraezedenzmusterTesten(Signatur_ZeichenT Benennung MAYBE_UNUSEDPARAM, Signatur_MusterT Muster MAYBE_UNUSEDPARAM)
{
#if XF_TEST
  char *c,
       Metazeichen[] = "ecp";
  for (c = Benennung; *c; c++)
    test_error(!strchr(Muster,*c), "Zeichen aus Benennung fehlt in Praezedenzmuster in semantischer Praezedenzerzeugung");
  for (c = Metazeichen; *c; c++)
    test_error(!strchr(Muster,*c), "Metazeichen fehlt in Praezedenzmuster in semantischer Praezedenzerzeugung");
  for (c = Muster; *c; c++)
    test_error(!strchr(Benennung,*c) && !strchr(Metazeichen,*c), 
      "unbekanntes Zeichen in Praezedenzmuster in semantischer Praezedenzerzeugung");
  test_error(strlen(Muster)!=strlen(Benennung)+strlen(Metazeichen), 
    "mehrfach auftretendes Zeichen in Praezedenzmuster in semantischer Praezedenzerzeugung");
#else
  ;
#endif
}


static char Symbol2Char(SymbolT Symbol, Signatur_SymbolT sig, Signatur_ZeichenT Benennung)
{
  SymbolT *s;
  char *b;
  for (s=sig, b=Benennung; *b && *s!=Symbol; s++, b++);
  return *b ? *b : 'x';                                                  /* taucht nicht in Musterpraezedenzen auf */
}  

static SymbolT Char2Symbol(char Char, Signatur_SymbolT sig, Signatur_ZeichenT Benennung)
{
  char *b;
  SymbolT *s;
  for (b=Benennung, s=sig; *b!=Char; b++, s++);
  return *s;
}

static char *SemantischePraezedenz(Signatur_SymbolT sig, Signatur_ZeichenT sig_c, ordering Familie MAYBE_UNUSEDPARAM, 
                                   PraezedenzartenT Art MAYBE_UNUSEDPARAM, Signatur_ZeichenT Benennung,  
                                   Signatur_StelligkeitenT Stelligkeiten, Signatur_MusterT Muster)
{
  PraezedenzT Praezedenz = NeuePraezedenz();
  char *m = Muster;
  SymbolT f;

  SignaturTesten(sig,sig_c,Benennung,Stelligkeiten);
  PraezedenzmusterTesten(Benennung,Muster);

  do 
    switch (*m) {
    case 'e':
      SO_forEquationSymbole(f)
        if (!strchr(m+1,Symbol2Char(f,sig,Benennung)))
          SymbolAnordnen(f);
      break;
    case 'c':
      SO_forConclusionSymbole(f)
        if (!strchr(m+1,Symbol2Char(f,sig,Benennung)))
          SymbolAnordnen(f);
      break;
    case 'p':
      SO_forParaSymbole(f)
        SymbolAnordnen(f);
      break;
    default:
      SymbolAnordnen(Char2Symbol(*m,sig,Benennung));
    } 
  while (*++m);
  return FertigePraezedenz(&Praezedenz);
}


static void InversesNullgewichten(char *Ergebnis,Signatur_SymbolT sig, Signatur_ZeichenT sig_c)
{
  SymbolT Inverses = Char2Symbol('/',sig,sig_c);
  char *Bezeichner = IO_FktSymbName(Inverses);
  strcat(Ergebnis,":");
  strcat(Ergebnis,Bezeichner);
  strcat(Ergebnis,"=0");
}



/*=================================================================================================================*/
/*= IV. Syntaktische Praezedenzen                                                                                 =*/
/*=================================================================================================================*/

static BOOLEAN *DefinierteSymbole(ordering Familie)
{                             /* Beachte: Nur fuer Ordnung LPO koennen Symbole als definiert interpretiert werden. */
  SymbolT f;
  BOOLEAN *Feld = array_alloc(SO_AnzahlFunktionen,BOOLEAN);
  SO_forFktSymbole(f)
    Feld[f] = FALSE;
  if (Familie==ordering_lpo) {
    ListenT Gleichung = PM_Gleichungen;
    while (Gleichung) {
      TermT l = car((ListenT)car(Gleichung)),
            r = cdr((ListenT)car(Gleichung));
      SymbolT f = TO_TopSymbol(l),
              g = TO_TopSymbol(r);
      if (TO_DefinierendeGleichung(l,r))                  /* Fuer Gleichungen wie f(x,y)=g(y,x) wird nichtdetermi- */
        Feld[f] = TRUE;                                    /* nistisch nur ein Symbol als definiert interpretiert. */
      else if (TO_DefinierendeGleichung(r,l))         /* Ausserdem werden keine Hierarchien oder Zykel detektiert. */
        Feld[g] = TRUE;
      Cdr(&Gleichung);
    }
  }
  return Feld;
}


static BOOLEAN KBODefinierendeGleichung(TermT l, TermT r)
{
  if (TO_DefinierendeGleichung(l,r)) {
    while ((l = TO_Schwanz(l))) {
      SymbolT x = TO_TopSymbol(l);
      if (SO_SymbIstVar(x))
        if (TO_Symbollaenge(x,l)<TO_Symbollaenge(x,r))
          return FALSE;
    }
    return TRUE;
  }
  else
    return FALSE;
}

static void GewichtlichMajorisieren(TermT l, TermT r, PraezedenzT *Praezedenz)
{
  SymbolT f = TO_TopSymbol(l);
  unsigned int L = TO_Termlaenge(l),
               R = TO_Termlaenge(r),
               F = L>R ? 1 : R-L+2;
  char *s = Praezedenz->Zeichenkette;
  if (F>1)
    sprintf(s+strlen(s),":%s=%d",IO_FktSymbName(f),F);
}

static void UmGewichteVerlaengern(PraezedenzT *Praezedenz, ordering Familie)
{
  if (Familie==ordering_kbo) {
    ListenT Gleichung = PM_Gleichungen;
    while (Gleichung) {
      TermT l = car((ListenT)car(Gleichung)),
            r = cdr((ListenT)car(Gleichung));
      if (KBODefinierendeGleichung(l,r))                  /* Fuer Gleichungen wie f(x,y)=g(y,x) wird nichtdetermi- */
        GewichtlichMajorisieren(l,r,Praezedenz);           /* nistisch nur ein Symbol als definiert interpretiert. */
      else if (KBODefinierendeGleichung(r,l))         /* Ausserdem werden keine Hierarchien oder Zykel detektiert. */
        GewichtlichMajorisieren(r,l,Praezedenz);           /* nistisch nur ein Symbol als definiert interpretiert. */
      Cdr(&Gleichung);
    }
  }
}


static char *FuchsPraezedenz(Signatur_SymbolT sig MAYBE_UNUSEDPARAM, Signatur_ZeichenT sig_c MAYBE_UNUSEDPARAM, 
                             ordering Familie, PraezedenzartenT Art, Signatur_ZeichenT Benennung MAYBE_UNUSEDPARAM,
                             Signatur_StelligkeitenT Stelligkeiten MAYBE_UNUSEDPARAM, 
                             Signatur_MusterT Muster MAYBE_UNUSEDPARAM)
{
  /* Erzeugte Praezedenz: 
     Unterscheide (E) mit Existenzzielen (A) ohne Existenzziele
     (E) definierte EQUATION-Symbole > sonstige EQUATION- und      CONCLUSION-Symbole > Existenzquantifizierungs-Symbole
     (A) definierte EQUATION-Symbole > sonstige EQUATION-Symbole > CONCLUSION-Symbole

     Definierte Symbole werden nur fuer die LPO verwendet, und zwar aus den EQUATION-Symbolen herausgenommen.

     (1)  innerhalb der definierten Symbole entsprechend Symbolzuordnung
          (Dies ist natuerlich fragwuerdig, betrachte etwa a=b, b=c, c=a, jedoch praktisch ausreichend.)
     (2A) innerhalb der EQUATION-Symbole 1-stellig > 2-stellig > ... > n-stellig > Konstanten
            bei gleicher Stelligkeit entsprechend Symbolzuordnung
     (2E) ebenso innerhalb von EQUATION- und CONCLUSION-Symbolen
     (3A) innerhalb der CONCLUSION-Symbole 0-stellig > 1-stellig > 2-stellig > ... > n-stellig
            bei gleicher Stelligkeit entsprechend Symbolzuordnung
     (3E) innerhalb der Existenzquantifizierungs-Symbole eq > true > false
  */
  PraezedenzT Praezedenz = NeuePraezedenz();
  unsigned int i,
               N = SO_MaximaleStelligkeit;
  BOOLEAN *DefiniertesSymbol = DefinierteSymbole(Familie);
  SymbolT f;

  SO_forEquationSymbole(f)                                                               /* (1) definierte Symbole */
    if (DefiniertesSymbol[f])
      SymbolAnordnen(f);

  if (!PM_Existenzziele()) {                                                             /* (A) Allquantifizierung */
    for(i=1; i<=N+1; i++)                                                                 /* (2A) EQUATION-Symbole */
      SO_forEquationSymbole(f)
        if (i%(N+1)==SO_Stelligkeit(f))
          SymbolAnordnen(f);                                          /* SymbolAnordnen prueft auch auf Doubletten */
    for(i=0; i<=N; i++)                                                                 /* (3A) CONCLUSION-Symbole */
      SO_forConclusionSymbole(f)
        if (i==SO_Stelligkeit(f))
          SymbolAnordnen(f);
  }

  else {                                                                            /* (E) Existenzquantifizierung */
    for(i=1; i<=N+1; i++)                                        /* (2E) sonstige EQUATION- und CONCLUSION-Symbole */
      SO_forSpezSymbole(f)
        if (i%(N+1)==SO_Stelligkeit(f))
          SymbolAnordnen(f);
    SymbolAnordnen(SO_EQ);                                                /* (3E) Existenzquantifizierungs-Symbole */
    SymbolAnordnen(SO_TRUE);
    SymbolAnordnen(SO_FALSE);
  }

  if (Art==Std)
    UmGewichteVerlaengern(&Praezedenz,Familie);

  our_dealloc(DefiniertesSymbol);
  return FertigePraezedenz(&Praezedenz);
}



/*=================================================================================================================*/
/*= V. Verteilerfunktion                                                                                          =*/
/*=================================================================================================================*/

typedef enum { semantisch, syntaktisch } GattungsT;

typedef char *(*GeneratorT)(Signatur_SymbolT,Signatur_ZeichenT,ordering,PraezedenzartenT,
                            Signatur_ZeichenT,Signatur_StelligkeitenT,Signatur_MusterT);

#define ___ Mantelung(
#define __ ,___
#define Mantelung(Kurzname,Gattung,Benennung,Stelligkeiten,Muster)                                                  \
  { Gattung,Benennung,Stelligkeiten,Muster } 
#define _(X,Y) X,Y

char *PG_Praezedenz(PraezedenzartenT Art, Signatur_SymbolT sig, Signatur_ZeichenT sig_c, ordering Familie)
{
  struct {
    GattungsT Gattung;
    Signatur_ZeichenT Benennung; 
    Signatur_StelligkeitenT Stelligkeiten;
    Signatur_MusterT Muster;
  } Praezedenzparameter[] = { Tafel4() },
    Par = Praezedenzparameter[Art];

  GeneratorT Generatoren[] = { 
               SemantischePraezedenz,
               FuchsPraezedenz
             },
             Generator = Generatoren[Par.Gattung];

  char *Ergebnis = Generator(sig,sig_c,Familie,Art,Par.Benennung,Par.Stelligkeiten,Par.Muster);

  if (Art==Lat) /*Provisorium*/
    InversesNullgewichten(Ergebnis,sig,sig_c);

  return Ergebnis;
}

#undef Mantelung
