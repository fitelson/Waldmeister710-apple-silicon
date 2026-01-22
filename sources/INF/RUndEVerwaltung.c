/*=================================================================================================================
  =                                                                                                               =
  =  RUndEVerwaltung.c                                                                                            =
  =                                                                                                               =
  =  Verwaltung der Regel- und der Gleichungsmenge                                                                =
  =                                                                                                               =
  =                                                                                                               =
  =  22.03.94  Thomas.                                                                                            =
  =  25.03.94  Ziel-Verwaltung eingebaut. Arnim.                                                                  =
  =  27.03.94  Ausgaben eingebaut.                                                                                =
  =  11.06.96  Statistik eingebaut, Ziel-Verwaltung ausgelagert. Arnim.                                           =
  =================================================================================================================*/


#include "ACGrundreduzibilitaet.h"
#include "Ausgaben.h"
#include "compiler-extra.h"
#include "ClasFunctions.h"
#include "ClasHeuristics.h"
#include "RUndEVerwaltung.h"
#include "SymbolOperationen.h"
#include "Praezedenz.h"

#ifndef CCE_Source
#include "DSBaumAusgaben.h"
#include "DSBaumOperationen.h"
#include "DSBaumTest.h"
#include "Hauptkomponenten.h"
#include "general.h"
#include "Grundzusammenfuehrung.h"
#include "KDHeap.h"
#include "KPVerwaltung.h"
#include "Parameter.h"
#include "Statistiken.h"
#include "WaldmeisterII.h"
#include "Zielverwaltung.h"
#else
#include "parse-util.h"
#define CF_Generation(x,y) 0
#define CH_GtWeight(a) 0
#define Z_IstGemantelteUngleichung(x,y) FALSE
#define HK_getAbsTime_M() 0
#define GZ_ZSFB_BEHALTEN 1
#define KDH_UndefinedObserver 0
CounterT RE_AnzRegelnGesamt;
CounterT RE_AnzGleichungenGesamt;

#define RE_AnzRegelnGesamt()              RE_AnzRegelnGesamt 
#define RE_AnzGleichungenGesamt()         RE_AnzGleichungenGesamt

#define RE_IncAnzRegelnGesamt() (RE_AnzRegelnGesamt++) 
#define RE_IncAnzGleichungenGesamt() (RE_AnzGleichungenGesamt++)  

#define RE_IncRegelnAktuell() /* nix tun */
#define RE_DecRegelnAktuell() /* nix tun */

#define IncMonogleichungen() (void) 0
#define IncAktMonogleichungen() (void) 0
#define DecAktMonogleichungen() (void) 0
#define IncGleichungenAktuell() /* nix tun */
#define DecGleichungenAktuell() /* nix tun */
#define BA_BaumMalenDefaultOhneG(a) /* nix tun */
#define BA_BaumMalenDefaultMitG(a) /* nix tun */
#define BA_DumpBaum(a,b) /* nix tun */
#define KPV_KillParent(a) /* nix tun */
#define PA_doSK() (1)
#define PA_doACG() (1)
#endif


/*=================================================================================================================*/
/*= 0. Globale Variablen                                                                                          =*/
/*=================================================================================================================*/


DSBaumT RE_Regelbaum;
DSBaumT RE_Gleichungsbaum;

/*=================================================================================================================*/
/*= I. Bedingter Testbetrieb und Testfunktion                                                                     =*/
/*=================================================================================================================*/

#define TestMitSprungverweisen TRUE            /* Alternativen: TRUE und FALSE; sollte moeglichst auf TRUE stehen! */
#define Testbeginn (unsigned long int) 1                                        /* legt fest, ab wievieltem Aufruf */
                                                              /* wirklich getestet bzw. getestet und gedruckt wird */
#if RE_TEST
static BOOLEAN RegelmengenTest(void);
static BOOLEAN GleichungsmengenTest(BOOLEAN GegenrichtungenSuchen);
#endif

void RE_RUndETest(void)
{
#if RE_TEST
  printf("\nChecking set of rules ... ");
  if (RegelmengenTest())
    printf("\n                      ... ok\n");
  else
    our_error("set of rules inconsistent");
  printf("\nChecking set of equations ... ");
  if (GleichungsmengenTest(TRUE))
    printf("\n                          ... ok\n");
  else
    our_error("set of equations inconsistent");
#endif
}


#if RE_TEST==2
static long unsigned int TestaufrufNr = 0;
#define RTest(/*char[]*/ Ankuendigungstext)                                                                         \
  TestaufrufNr++;                                                                                                   \
  if (TestaufrufNr>=Testbeginn) {                                                                                   \
    printf("\nRETest %ld: %s\n",TestaufrufNr,Ankuendigungstext);                                         \
    if (!RegelmengenTest())                                                                                         \
      our_fatal_error("Regelmenge inkonsistent.");                                                                  \
  }
#define ETest(/*BOOLEAN*/ GegenrichtungenSuchen, /*char[]*/ Ankuendigungstext)                                      \
  TestaufrufNr++;                                                                                                   \
  if (TestaufrufNr>=Testbeginn) {                                                                                   \
    printf("\nRETest %ld: %s\n",TestaufrufNr,Ankuendigungstext);                                         \
    if (!GleichungsmengenTest(GegenrichtungenSuchen))                                                               \
      our_fatal_error("Gleichungsmenge inkonsistent.");                                                             \
  }
#else
#define RTest(/*char[]*/ Ankuendigungstext)
#define ETest(/*BOOLEAN*/ GegenrichtungenSuchen, /*char[]*/ Ankuendigungstext)
#endif



/*=================================================================================================================*/
/*= II. Regelmenge                                                                                                =*/
/*=================================================================================================================*/
static void re_addActive (RegelOderGleichungsT re);               /*wird hier gebraucht aber erst spaeter definiert*/

/* 1. Erzeugen von Regeln aus Termpaaren ------------------------------------------------------------------------- */

static unsigned int TiefeLinks;

static RegelT re_ErzeugteRegel(SelectRecT selRec)
{
  RegelT ZuletztErzeugteRegel;
  TermT links,rechts;

  if (selRec.result == Kleiner){
    links  = TO_FrischzellenTermkopie(selRec.rhs);
    rechts = TO_FrischzellenTermkopie(selRec.lhs);
  }
  else {
    links  = TO_FrischzellenTermkopie(selRec.lhs);
    rechts = TO_FrischzellenTermkopie(selRec.rhs);
  }
  TO_TermeLoeschen(selRec.lhs,selRec.rhs);
  RE_IncAnzRegelnGesamt();

  TP_GNTermpaarHolen(ZuletztErzeugteRegel);
  BO_TermpaarNormieren(links,rechts,&TiefeLinks);
  ZuletztErzeugteRegel->rechteSeite = rechts;
  ZuletztErzeugteRegel->r2 = NULL;                     /* Annahme: Regeln enthalten keine rechts freien Variablen. */
  ZuletztErzeugteRegel->linkeSeite = links;
  ZuletztErzeugteRegel->Nummer = RE_AnzRegelnGesamt(); /* Argl */
  TP_setGeburtsTag_M(ZuletztErzeugteRegel, HK_getAbsTime_M());
  TP_initializeTodesTag_M(ZuletztErzeugteRegel);
  ZuletztErzeugteRegel->tiefeLinks = TiefeLinks;
  ZuletztErzeugteRegel->laengeLinks = TO_Termlaenge(links);
  ZuletztErzeugteRegel->laengeRechts = TO_Termlaenge(rechts);
  ZuletztErzeugteRegel->rewrites = 0;

  re_addActive(ZuletztErzeugteRegel);
  if (IO_level4()) {
    IO_DruckeFlex("Regel in activa aufgenommen %t -> %t\n", ZuletztErzeugteRegel->linkeSeite, ZuletztErzeugteRegel->rechteSeite);
  }
  ZuletztErzeugteRegel->IstMonogleichung = TRUE;
  ZuletztErzeugteRegel->lebtNoch = TRUE;
  ZuletztErzeugteRegel->Gegenrichtung = NULL;
  ZuletztErzeugteRegel->RichtungAusgezeichnet = TRUE;
  ZuletztErzeugteRegel->DarfNichtReduzieren = FALSE;

  ZuletztErzeugteRegel->generation = CF_Generation(selRec.actualParent,selRec.otherParent);
  ZuletztErzeugteRegel->IstUngleichung = Z_IstGemantelteUngleichung(links,rechts);

  ZuletztErzeugteRegel->wid = selRec.wid;
  ZuletztErzeugteRegel->used = FALSE;
  ZuletztErzeugteRegel->proofPrinted = FALSE;

  ZuletztErzeugteRegel->gzfbStatus = GZ_undef;
  ZuletztErzeugteRegel->GescheitertL = NULL;
  ZuletztErzeugteRegel->GescheitertR = NULL;
  if (selRec.istGZFB){
    IO_DruckeFlex("Regel ist als GZFB bekannt! ####################### \n");
    ZuletztErzeugteRegel->ACGrundreduzibel = FALSE;
    ZuletztErzeugteRegel->gzfbStatus = GZ_zusammenfuehrbar;
    ZuletztErzeugteRegel->Grundzusammenfuehrbar = TRUE;
    ZuletztErzeugteRegel->DarfNichtUeberlappen = TRUE;
  }
  else if (AC_FaktumGrundreduzibel(ZuletztErzeugteRegel)){
    ZuletztErzeugteRegel->ACGrundreduzibel = TRUE;
    ZuletztErzeugteRegel->DarfNichtUeberlappen = TRUE;
    ZuletztErzeugteRegel->Grundzusammenfuehrbar = TRUE;
  }
  else {
    ZuletztErzeugteRegel->ACGrundreduzibel = FALSE;
    ZuletztErzeugteRegel->Grundzusammenfuehrbar = 
#ifndef CCE_Source
      GZ_ZSFB_BEHALTEN && GZ_doGZFB() && GZ_GrundzusammenfuehrbarVorwaerts(ZuletztErzeugteRegel);
#else
    FALSE;
#endif
    ZuletztErzeugteRegel->DarfNichtUeberlappen = ZuletztErzeugteRegel->Grundzusammenfuehrbar;
  }
  ZuletztErzeugteRegel->StrengKonsistent = AC_FaktumStrengKonsistent(ZuletztErzeugteRegel);
  if (PA_doSK() && ZuletztErzeugteRegel->StrengKonsistent){
    ZuletztErzeugteRegel->DarfNichtUeberlappen = TRUE;
  }
  ZuletztErzeugteRegel->aEntryObserver = KDH_UndefinedObserver;
#ifdef CCE_Protocol
  ZuletztErzeugteRegel->CCE_lhs = TO_Termkopie(ZuletztErzeugteRegel->linkeSeite);
  ZuletztErzeugteRegel->CCE_rhs = TO_Termkopie(ZuletztErzeugteRegel->rechteSeite);
#endif  
  return ZuletztErzeugteRegel;
}


/* 3. Einfuegeoperation ------------------------------------------------------------------------------------------ */

static void RegelEinfuegen(RegelT Regel)
{
  RE_IncRegelnAktuell();
  RTest("Regelbaum vor dem Einfuegen");
  BO_ObjektEinfuegen(&RE_Regelbaum,Regel,TiefeLinks);
  RTest("Regelbaum nach dem Einfuegen");
}


/* 4. Loeschoperation -------------------------------------------------------------------------------------------- */

 void RE_RegelEntfernen(RegelT Regel)
{
  RE_DecRegelnAktuell();
  RTest("Regelbaum vor dem Entfernen");
  BO_ObjektEntfernen(&RE_Regelbaum,Regel);
  RTest("Regelbaum nach dem Entfernen");
  KPV_KillParent(Regel);
}

static void RE_RegelIndexLoeschen(void)
{
  IO_AusgabenAus();
  while (RE_Regelbaum.Wurzel) {
    RegelT Regel = BK_Regeln(RE_Regelbaum.ErstesBlatt);
    RE_RegelEntfernen(Regel);
  }
  IO_AusgabenEin();
}

/* 5. Ausgabeoperation ------------------------------------------------------------------------------------------- */

static void RE_AusgabeRegelmenge(int Grad)
{
  RegelT RegelIndex;
  if (Grad) {
     if (Grad == 7)
        IO_DruckeUeberschrift("Completed system: Set of rules");
     else 
        IO_DruckeUeberschrift("Set of rules");
  }
  
  switch (Grad) {
  case 0:                                                           /* wird fuer Ausgabe als Spezifikation benutzt */
    BK_forRegeln(RegelIndex,RE_Regelbaum, {
      IO_DruckeFlex("\t%t = %t\n",TP_LinkeSeite(RegelIndex),TP_RechteSeite(RegelIndex));
    });
    break;
  case 1:
    BK_forRegeln(RegelIndex,RE_Regelbaum, {
      printf("No. %4ld %s   ",TP_ElternNr(RegelIndex),
              RegelIndex->Grundzusammenfuehrbar ? "G" : " ");
      IO_DruckeFlex("%t -> %t\n",TP_LinkeSeite(RegelIndex),TP_RechteSeite(RegelIndex));
    });
    break;
  case 7:
    BK_forRegeln(RegelIndex,RE_Regelbaum, {
      if(TP_lebtZumZeitpunkt_M(RegelIndex,HK_getAbsTime_M())){
        printf("No. %4ld %s   ",TP_ElternNr(RegelIndex),
               RegelIndex->Grundzusammenfuehrbar ? "G" : " ");
        IO_DruckeFlex("%t -> %t\n",TP_LinkeSeite(RegelIndex),TP_RechteSeite(RegelIndex));
      }
    });
    break;
  case 2:
    BA_BaumMalenDefaultOhneG(RE_Regelbaum);
    break;
  case 3:
    BA_BaumMalenDefaultMitG(RE_Regelbaum);
    break;
  case 4:
  case 5:
    BA_DumpBaum(RE_Regelbaum, Grad==5);
  default:
    break;
  }
   if (Grad)
     printf("\n\n");
}


/* 7. Testfunktion ----------------------------------------------------------------------------------------------- */

#if RE_TEST
static BOOLEAN RegelPraedikat(RegelT Regel)
{
  if (TP_Nachf(Regel)) {
    printf("\nFehler: Regel mit eingetragener Nachfolgerregel zur selben linken Seite.\n");
    return FALSE;
  }
  if (TP_Gegenrichtung(Regel)) {
    printf("\nFehler: Regel mit eingetragener Gegenrichtung.\n");
    return FALSE;
  }
  if (TP_ElternNr(Regel)<=0) {
    printf("\nFehler: Regel mit nichtpositiver Nummer.\n");
    return FALSE;
  }
  if (!TO_TermKorrekt(TP_LinkeSeite(Regel))) {
    printf("\nFehler: Regel hat fehlerhafte linke Seite.\n");
    return FALSE;
  }
  if (!TO_TermKorrekt(TP_RechteSeite(Regel))) {
    printf("\nFehler: Regel hat fehlerhafte rechte Seite.\n");
    return FALSE;
  }
  return TRUE;
}


static BOOLEAN RegelmengenTest(void)
{
  if (BT_IndexTesten(RE_Regelbaum,RegelPraedikat,TestMitSprungverweisen))
    return TRUE;
  RE_AusgabeRegelmenge(1);
  return FALSE;
}
#endif


/* 8. Umbenennungen ---------------------------------------------------------------------------------------------- */

static void re_FaktumUmbenennen(RegelT Regel, signed int Delta)
{
  TO_VariablenUmbenennen(TP_LinkeSeite(Regel),Delta);
  TO_VariablenUmbenennen(TP_RechteSeite(Regel),Delta);
}

void RE_FaktumExternalisieren(RegelOderGleichungsT Faktum){
  re_FaktumUmbenennen(Faktum,SO_VarNummer(BO_NeuesteBaumvariable()));
}

void RE_FaktumReinternalisieren(RegelOderGleichungsT Faktum){
  re_FaktumUmbenennen(Faktum,-SO_VarNummer(BO_NeuesteBaumvariable()));
}

  
/*=================================================================================================================*/
/*= III. Gleichungsmenge                                                                                          =*/
/*=================================================================================================================*/

/* 1. Erzeugen von Gleichungen aus Termpaaren -------------------------------------------------------------------- */

static unsigned int TiefeRechts;

static void RechtsUnfreiErzeugen(GleichungsT e)
/* Falls die rechte Seite freie Variablen enthaelt, eine Instanz erzeugen, bei der die freien Variablen 
   an die kleinste Konstante gebunden sind. Jedoch nur, wenn die verwendete Reduktiosordnung grundtotal ist. */
{
  TermT r2 = NULL;
  if (PR_PraezedenzTotal) {
    TermT l = e->linkeSeite,
          r = e->rechteSeite;
    SymbolT x = TO_neuesteVariable(l),
            y = TO_neuesteVariable(r);
    if (SO_VariableIstNeuer(y,x)) {
      TermT t;
      t = r2 = TO_Termkopie(r);
      do {
        SymbolT z = TO_TopSymbol(t);
        if (SO_SymbIstVar(z) && SO_VariableIstNeuer(z,x))
          TO_SymbolBesetzen(t,PR_minimaleKonstante);
      } while ((t = TO_Schwanz(t)));
    }
  }
  e->r2 = r2;
#if 0
  IO_DruckeFlex("+++  %t => %t",e->linkeSeite,e->rechteSeite);
  if (r2)
    IO_DruckeFlex(" ### %t",r2);
  IO_LF();
#endif
}


static GleichungsT re_ErzeugteGleichung(SelectRecT selRec)
{
  GleichungsT ZuletztErzeugteGleichung;
  GNTermpaarT ZuletztErzeugteAntigleichung;
  TermT links, rechts;

  links = selRec.lhs;
  rechts = selRec.rhs;
  RE_IncAnzGleichungenGesamt();

  TP_GNTermpaarHolen(ZuletztErzeugteAntigleichung);
  TP_GNTermpaarHolen(ZuletztErzeugteGleichung);
  BO_TermpaarNormieren(links,rechts,&TiefeLinks);
  ZuletztErzeugteGleichung->rechteSeite = TO_FrischzellenTermkopie(rechts);
  ZuletztErzeugteGleichung->linkeSeite = TO_FrischzellenTermkopie(links);
  ZuletztErzeugteGleichung->lebtNoch = TRUE;
  TP_setGeburtsTag_M(ZuletztErzeugteGleichung, HK_getAbsTime_M());
  TP_initializeTodesTag_M(ZuletztErzeugteGleichung);
  ZuletztErzeugteGleichung->tiefeLinks = TiefeLinks;
  ZuletztErzeugteGleichung->laengeLinks = TO_Termlaenge(links);
  ZuletztErzeugteGleichung->laengeRechts = TO_Termlaenge(rechts);

  ZuletztErzeugteGleichung->rewrites = 0;

#if 1
  re_addActive(ZuletztErzeugteGleichung);
  if (IO_level4()) {
    IO_DruckeFlex("Gleichung in activa aufgenommen %t = %t\n", ZuletztErzeugteGleichung->linkeSeite, ZuletztErzeugteGleichung->rechteSeite);
  }
#endif
  ZuletztErzeugteGleichung->Nummer = -RE_AnzGleichungenGesamt();
  ZuletztErzeugteGleichung->RichtungAusgezeichnet = TRUE;
  /* Dies ist die einzige Stelle im System, an der die Abbildung der Gleichungs-Nummern auf negative Zahlen stattfindet!*/
  ZuletztErzeugteGleichung->DarfNichtReduzieren = FALSE;
  RechtsUnfreiErzeugen(ZuletztErzeugteGleichung);
  ZuletztErzeugteGleichung->IstUngleichung = FALSE;

  BO_TermpaarNormieren(rechts,links,&TiefeRechts);
  ZuletztErzeugteAntigleichung->linkeSeite = TO_FrischzellenTermkopie(rechts);
  ZuletztErzeugteAntigleichung->rechteSeite = TO_FrischzellenTermkopie(links);
  TO_TermeLoeschen(rechts,links);
  ZuletztErzeugteAntigleichung->tiefeLinks = TiefeRechts;
  ZuletztErzeugteAntigleichung->laengeLinks  = ZuletztErzeugteGleichung->laengeRechts;
  ZuletztErzeugteAntigleichung->laengeRechts = ZuletztErzeugteGleichung->laengeLinks;

  ZuletztErzeugteAntigleichung->rewrites = 0;

  ZuletztErzeugteGleichung->wid = selRec.wid;
  ZuletztErzeugteGleichung->used = FALSE;
  ZuletztErzeugteGleichung->proofPrinted = FALSE;
  ZuletztErzeugteAntigleichung->wid = selRec.wid;
  ZuletztErzeugteAntigleichung->used = FALSE;
  ZuletztErzeugteAntigleichung->proofPrinted = FALSE;

  if ((ZuletztErzeugteGleichung->IstMonogleichung = 
    ZuletztErzeugteAntigleichung->IstMonogleichung =
    TP_TermpaareGleich(ZuletztErzeugteGleichung,ZuletztErzeugteAntigleichung)))
    IncMonogleichungen();
  ZuletztErzeugteAntigleichung->RichtungAusgezeichnet = FALSE;
  ZuletztErzeugteAntigleichung->Nummer = -RE_AnzGleichungenGesamt();
  ZuletztErzeugteAntigleichung->lebtNoch = TRUE;
  TP_setGeburtsTag_M(ZuletztErzeugteAntigleichung, HK_getAbsTime_M());
  TP_initializeTodesTag_M(ZuletztErzeugteAntigleichung);
  ZuletztErzeugteAntigleichung->DarfNichtReduzieren = FALSE;
  RechtsUnfreiErzeugen(ZuletztErzeugteAntigleichung);  
  ZuletztErzeugteAntigleichung->IstUngleichung = FALSE;
  ZuletztErzeugteGleichung->gzfbStatus = GZ_undef;
  ZuletztErzeugteGleichung->GescheitertL = NULL;
  ZuletztErzeugteGleichung->GescheitertR = NULL;
  ZuletztErzeugteAntigleichung->gzfbStatus = GZ_undef;
  ZuletztErzeugteAntigleichung->GescheitertL = NULL;
  ZuletztErzeugteAntigleichung->GescheitertR = NULL;

  (ZuletztErzeugteGleichung->Gegenrichtung = ZuletztErzeugteAntigleichung)->Gegenrichtung = ZuletztErzeugteGleichung;
  if (selRec.istGZFB){
    IO_DruckeFlex("Gleichung ist als GZFB bekannt! ####################### \n");
    ZuletztErzeugteGleichung->ACGrundreduzibel = FALSE;
    ZuletztErzeugteGleichung->gzfbStatus = GZ_zusammenfuehrbar;
    ZuletztErzeugteGleichung->GescheitertL = NULL;
    ZuletztErzeugteGleichung->GescheitertR = NULL;
    ZuletztErzeugteGleichung->Grundzusammenfuehrbar = TRUE;
    ZuletztErzeugteGleichung->DarfNichtUeberlappen = TRUE;

    ZuletztErzeugteGleichung->Gegenrichtung->Grundzusammenfuehrbar = TRUE;
    ZuletztErzeugteGleichung->Gegenrichtung->gzfbStatus = GZ_zusammenfuehrbar;
    ZuletztErzeugteGleichung->Gegenrichtung->GescheitertL = NULL;
    ZuletztErzeugteGleichung->Gegenrichtung->GescheitertR = NULL;
  }
  else if (AC_FaktumGrundreduzibel(ZuletztErzeugteGleichung)){
    ZuletztErzeugteGleichung->ACGrundreduzibel = TRUE;
    ZuletztErzeugteGleichung->Gegenrichtung->ACGrundreduzibel = TRUE;
    ZuletztErzeugteGleichung->Gegenrichtung->Grundzusammenfuehrbar = ZuletztErzeugteGleichung->Grundzusammenfuehrbar = TRUE;
  }
  else {
    ZuletztErzeugteGleichung->ACGrundreduzibel = FALSE;
    ZuletztErzeugteGleichung->Gegenrichtung->ACGrundreduzibel = FALSE;
    ZuletztErzeugteGleichung->gzfbStatus = GZ_undef;
    ZuletztErzeugteGleichung->GescheitertL = NULL;
    ZuletztErzeugteGleichung->GescheitertR = NULL;
    ZuletztErzeugteGleichung->Grundzusammenfuehrbar = 
#ifndef CCE_Source
      GZ_ZSFB_BEHALTEN && GZ_doGZFB() && GZ_GrundzusammenfuehrbarVorwaerts(ZuletztErzeugteGleichung); 
#else
    FALSE;
#endif

    ZuletztErzeugteGleichung->Gegenrichtung->Grundzusammenfuehrbar = ZuletztErzeugteGleichung->Grundzusammenfuehrbar;
  }
  ZuletztErzeugteGleichung->Gegenrichtung->DarfNichtUeberlappen = 
    ZuletztErzeugteGleichung->DarfNichtUeberlappen = 
      ZuletztErzeugteGleichung->ACGrundreduzibel || ZuletztErzeugteGleichung->Grundzusammenfuehrbar;

  ZuletztErzeugteGleichung->Gegenrichtung->StrengKonsistent =
    ZuletztErzeugteGleichung->StrengKonsistent = AC_FaktumStrengKonsistent(ZuletztErzeugteGleichung);

  if (PA_doSK() && ZuletztErzeugteGleichung->StrengKonsistent){
    ZuletztErzeugteGleichung->Gegenrichtung->DarfNichtUeberlappen = 
      ZuletztErzeugteGleichung->DarfNichtUeberlappen = TRUE;
  }


  ZuletztErzeugteGleichung->generation = 
    ZuletztErzeugteAntigleichung->generation = CF_Generation(selRec.actualParent,selRec.otherParent);

  ZuletztErzeugteGleichung->aEntryObserver = KDH_UndefinedObserver;
  ZuletztErzeugteAntigleichung->aEntryObserver = KDH_UndefinedObserver;
#ifdef CCE_Protocol
  ZuletztErzeugteGleichung->CCE_lhs = TO_Termkopie(ZuletztErzeugteGleichung->linkeSeite);
  ZuletztErzeugteGleichung->CCE_rhs = TO_Termkopie(ZuletztErzeugteGleichung->rechteSeite);
#endif  
  return ZuletztErzeugteGleichung;
}


/* 3. Einfuegeoperation ------------------------------------------------------------------------------------------ */

static void GleichungEinfuegen(GleichungsT Gleichung)
{
  /* Hinrichtung */
  IncGleichungenAktuell();
  ETest(TRUE,"Gleichungsbaum vor dem Einfuegen der letzten Hinrichtung");
  BO_ObjektEinfuegen(&RE_Gleichungsbaum,Gleichung,TiefeLinks);
  ETest(FALSE,"Gleichungsbaum nach dem Einfuegen der letzten Hinrichtung");

  /* Herrichtung */
  if (TP_IstKeineMonogleichung(Gleichung)) {
    ETest(FALSE,"Gleichungsbaum vor dem Einfuegen der letzten Herrichtung");
    BO_ObjektEinfuegen(&RE_Gleichungsbaum,TP_Gegenrichtung(Gleichung),TiefeRechts);
    ETest(TRUE,"Gleichungsbaum nach dem Einfuegen der letzten Herrichtung");
  }
  else
    IncAktMonogleichungen();
}

/* 4. Loeschoperationen ------------------------------------------------------------------------------------------ */

 void RE_GleichungEntfernen(GleichungsT Gleichung)            /* Nur, wenn nicht Gegenrichtung einer Monogleichung. */
{
  if (TP_RichtungAusgezeichnet(Gleichung) || TP_IstKeineMonogleichung(Gleichung)) {
    ETest(FALSE,"Gleichungsbaum vor dem Entfernen");
    BO_ObjektEntfernen(&RE_Gleichungsbaum,Gleichung);
    ETest(FALSE,"Gleichungsbaum nach dem Entfernen");
  }
  if (TP_RichtungAusgezeichnet(Gleichung)){
    KPV_KillParent(Gleichung);
  }
}

static void RE_GleichungsIndexLoeschen(void)
{
  IO_AusgabenAus();
  while (RE_Gleichungsbaum.Wurzel) {
    GleichungsT Gleichung = BK_Regeln(RE_Gleichungsbaum.ErstesBlatt);
    RE_GleichungEntfernen(TP_Gegenrichtung(Gleichung));
    RE_GleichungEntfernen(Gleichung);
  }
  IO_AusgabenEin();
}


/* 5. Ausgabeoperationen ----------------------------------------------------------------------------------------- */

static void RE_AusgabeGleichungsmenge(int Grad)
{
  RegelT RegelIndex;
  if (Grad) {
     if (Grad == 7)
        IO_DruckeUeberschrift("Completed system: Set of equations");
     else 
        IO_DruckeUeberschrift("Set of equations");
  }
  switch (Grad) {
  case 0: {                                                                      /* fuer Ausgabe als Spezifikation */
    BK_forRegeln(RegelIndex,RE_Gleichungsbaum, {
      if (TP_RichtungAusgezeichnet(RegelIndex)) {
        IO_DruckeFlex("\t%t = %t\n",TP_LinkeSeite(RegelIndex),TP_RechteSeite(RegelIndex));
      }
    });
    break;
  }
  case 1:
    BK_forRegeln(RegelIndex,RE_Gleichungsbaum, {
      if (TP_RichtungAusgezeichnet(RegelIndex)) {
        printf("Nr. %4ld %s   ",TP_ElternNr(RegelIndex),
              RegelIndex->Grundzusammenfuehrbar ? "G" : " ");
        IO_DruckeFlex("%t = %t\n",TP_LinkeSeite(RegelIndex),TP_RechteSeite(RegelIndex));
      }
    });
    break;
  case 7:
    BK_forRegeln(RegelIndex,RE_Gleichungsbaum, {
      if (TP_lebtZumZeitpunkt_M(RegelIndex,HK_getAbsTime_M())){
        if (TP_RichtungAusgezeichnet(RegelIndex)) {
          printf("Nr. %4ld %s   ",TP_ElternNr(RegelIndex),
                 RegelIndex->Grundzusammenfuehrbar ? "G" : " ");
          IO_DruckeFlex("%t = %t\n",TP_LinkeSeite(RegelIndex),TP_RechteSeite(RegelIndex));
        }
      }
    });
    break;
  case 2: 
    BA_BaumMalenDefaultOhneG(RE_Gleichungsbaum);
    break;
  case 3:
    BA_BaumMalenDefaultMitG(RE_Gleichungsbaum);
    break;
  case 4:
  case 5:
    BA_DumpBaum(RE_Gleichungsbaum, Grad==5);
  default:
    break;
  }
  if (Grad)
    printf("\n\n");
}


/* 6. Schleifenkonstrukte ---------------------------------------------------------------------------------------- */

static RegelT RE_ErsteRegel(void){
  return BK_ErstesTermpaar(RE_Regelbaum);
}

static RegelT RE_NaechsteRegel(void){
  return BK_NaechstesTermpaar();
}

static GleichungsT RE_ErsteGleichung(void)
{
  GleichungsT ErsteGleichung = BK_ErstesTermpaar(RE_Gleichungsbaum);
  while (ErsteGleichung && TP_RichtungUnausgezeichnet(ErsteGleichung))
    ErsteGleichung = BK_NaechstesTermpaar();
  return ErsteGleichung;
}

static GleichungsT RE_NaechsteGleichung(void)
{
  GleichungsT NaechsteGleichung = BK_NaechstesTermpaar();
  while (NaechsteGleichung && TP_RichtungUnausgezeichnet(NaechsteGleichung))
    NaechsteGleichung = BK_NaechstesTermpaar();
  return NaechsteGleichung;
}

static BOOLEAN SchonInGleichungen;

RegelOderGleichungsT RE_ErstesFaktum(void)
{
  RegelT ErsteRegel = RE_ErsteRegel();
  return ((SchonInGleichungen = !ErsteRegel)) ? RE_ErsteGleichung() : ErsteRegel;
}
RegelOderGleichungsT RE_NaechstesFaktum(void)
{
  if (SchonInGleichungen)
    return RE_NaechsteGleichung();
  else {
    RegelT NaechsteRegel = RE_NaechsteRegel();
    return (NaechsteRegel ? NaechsteRegel : (SchonInGleichungen = TRUE, RE_ErsteGleichung()));
  }
}

/* 7. Testfunktion ----------------------------------------------------------------------------------------------- */

#if RE_TEST
static BOOLEAN ImGleichungsbaumVorhanden(GleichungsT Gleichung)
{
  KantenT Knoten = RE_Gleichungsbaum.Wurzel;
  UTermT TermIndex = TP_LinkeSeite(Gleichung),
         SuffixIndex;
  GleichungsT GleichungsIndex;
  if (!Knoten)
    return FALSE;
  do {
    Knoten = BK_Nachf(Knoten,TO_TopSymbol(TermIndex));
    if (!Knoten)
      return FALSE;
    TermIndex = TO_Schwanz(TermIndex);
  } while (!BK_IstBlatt(Knoten));
  SuffixIndex = BK_Rest(Knoten);
  while (SuffixIndex) {
    if (!TO_TopSymboleGleich(SuffixIndex,TermIndex))
      return FALSE;
    SuffixIndex = TO_Schwanz(SuffixIndex);
    TermIndex = TO_Schwanz(TermIndex);
  }
  GleichungsIndex = BK_Regeln(Knoten);
  while (GleichungsIndex) {
    if (GleichungsIndex==Gleichung)
      return TRUE;
    GleichungsIndex = TP_Nachf(GleichungsIndex);
  }
  return FALSE;
}


static BOOLEAN GleichungsPraedikatSchwach(GleichungsT Gleichung)
{
  GleichungsT Antigleichung = TP_Gegenrichtung(Gleichung);
  TermT Links = TP_LinkeSeite(Gleichung),
        Rechts = TP_RechteSeite(Gleichung),
        AntiLinks = TP_LinkeSeite(Antigleichung),
        AntiRechts = TP_RechteSeite(Antigleichung);
  unsigned int TiefeTerm1;

  if (!Antigleichung) {
    printf("\nFehler: Gleichung mit Antigleichung NULL.\n");
    return FALSE;
  }
  if (TP_RichtungAusgezeichnet(Gleichung)==TP_RichtungAusgezeichnet(Antigleichung)) {
    printf("\nFehler: Gleichung und Antigleichung beide ausgezeichnet oder beide unausgezeichnet.\n");
    return FALSE;
  }
  if (TP_ElternNr(Gleichung)>=0) {
    printf("\nFehler: Gleichung mit nichtnegativer Nummer.\n");
    return FALSE;
  }
  if (TP_ElternNr(Gleichung)!=TP_ElternNr(Antigleichung)) {
    printf("\nFehler: Gleichung und Antigleichung haben verschiedene Nummern.\n");
    return FALSE;
  }
  if (TP_Gegenrichtung(Antigleichung)!=Gleichung) {
    printf("\nFehler: Gleichung und Antigleichung sind nicht korrekt miteinander verzeigert.\n");
    return FALSE;
  }
  if (!TO_TermKorrekt(Links)) {
    printf("\nFehler: Gleichung hat fehlerhafte linke Seite.\n");
    return FALSE;
  }
  if (!TO_TermKorrekt(Rechts)) {
    printf("\nFehler: Gleichung hat fehlerhafte rechte Seite.\n");
    return FALSE;
  }
  BO_TermpaarNormieren(AntiRechts,AntiLinks,&TiefeTerm1);
  if (!TO_TermeGleich(Links,AntiRechts) || !TO_TermeGleich(Rechts,AntiLinks)) {
    printf("\nFehler: Gleichung und Antigleichung sind nicht Varianten voneinander.\n");
    return FALSE;
  }
  BO_TermpaarNormieren(AntiLinks,AntiRechts,&TiefeTerm1);
  if (TP_IstMonogleichung(Gleichung)!=TP_IstMonogleichung(Antigleichung)) {
    printf("\nFehler: Gleichung und Antigleichung haben unterschiedliche"
      " Monogleichungskennzeichnung.\n");
    return FALSE;
  }
  if (TP_IstMonogleichung(Gleichung)) {
    GleichungsT BaumExterne = TP_RichtungUnausgezeichnet(Gleichung) ? Gleichung : Antigleichung;
    if (!TO_TermKorrekt(TP_LinkeSeite(BaumExterne))) {
      printf("\nFehler: Unausgezeichnete Richtung von Monogleichung hat fehlerhafte linke Seite.\n");
      return FALSE;
    }
    if (!TO_TermKorrekt(TP_RechteSeite(BaumExterne))) {
      printf("\nFehler: Unausgezeichnete Richtung von Monogleichung hat fehlerhafte rechte Seite.\n");
      return FALSE;
    }
    if (ImGleichungsbaumVorhanden(BaumExterne)) {
      printf("\nFehler: Unausgezeichnete Richtung von Monogleichung steht im Gleichungsbaum.\n");
      return FALSE;
    }
  }
  return TRUE;
}


static BOOLEAN GleichungsPraedikatNormal(GleichungsT Gleichung)
{
  if (!GleichungsPraedikatSchwach(Gleichung))
    return FALSE;
  if (TP_IstKeineMonogleichung(Gleichung) && !ImGleichungsbaumVorhanden(TP_Gegenrichtung(Gleichung))) {
    printf("\nFehler: Gegenrichtung von Stereogleichung steht nicht im Gleichungsbaum.\n");
    return FALSE;
  }
  return TRUE;
}


static BOOLEAN GleichungsmengenTest(BOOLEAN GegenrichtungenSuchen)
{
  if (BT_IndexTesten(RE_Gleichungsbaum,GegenrichtungenSuchen ? GleichungsPraedikatNormal : 
    GleichungsPraedikatSchwach,TestMitSprungverweisen))
    return TRUE;
  RE_AusgabeGleichungsmenge(4);
  return FALSE;
}
#endif



/*=================================================================================================================*/
/*= IV. Ergebnisausgabe R-, E- und KP-Menge                                                                       =*/
/*=================================================================================================================*/



/*=================================================================================================================*/
/*= V. Regeln und Gleichungen als Eltern von KPs                                                                  =*/
/*=================================================================================================================*/

#ifndef CCE_Source
void RE_UngleichungEntmanteln(UTermT* s, UTermT* t, RegelOderGleichungsT Faktum)
{ Z_UngleichungEntmanteln(s,t, TP_LinkeSeite(Faktum),TP_RechteSeite(Faktum)); }

BOOLEAN RE_UngleichungEntmantelt(UTermT* s, UTermT* t, RegelOderGleichungsT Faktum)
{ return Z_UngleichungEntmantelt(s,t, TP_LinkeSeite(Faktum),TP_RechteSeite(Faktum)); }
#endif


/*=================================================================================================================*/
/*= VI. Initialisierungen                                                                                         =*/
/*=================================================================================================================*/

void RE_InitApriori(void)
{
  BO_DSBaumInitialisieren(&RE_Regelbaum);                      /* muss vor jedem neuen Strategiedurchlauf erfolgen */
  BO_DSBaumInitialisieren(&RE_Gleichungsbaum);
}  

/*=================================================================================================================*/
/*= VII. Activa                                                                                                   =*/
/*=================================================================================================================*/

/* Im folgenden werden die Activa definiert. Sie sind die eigentliche Menge A (wobei momentan auch noch die schon  */
                                     /* interreduzierten und somit toten Regeln und Gleichungen noch drin bleiben) */
typedef struct {
  RegelOderGleichungsT *xs;
  unsigned long size;
  unsigned long activeNumber;
  unsigned long offset;
} ActivaT;


ActivaT Activa = {NULL, 0, 0, 0};    /* erstes Instantiieren der Menge A. Die Menge ist initial leer, es wurde noch */
                                                          /* kein Platz allokiert und es sind 0 Elemente enthalten. */

static long ACTIVA_ERWEITERUNGSSCHRITT = 1000;   /* gibt an, um wieviele Elemente der Platz in den Activa erweitert */
					                                             /* wird, wenn diese voll sind. */

unsigned long RE_ActiveFirstIndex(void)
{
  return Activa.offset;
}

unsigned long RE_ActiveLastIndex(void)
{
  return Activa.activeNumber - 1;
}

static void re_addActive (RegelOderGleichungsT re)                     /* Aufnehmen einer Regel oder Gleichung in A.*/
						         /* Ist die Menge A leer, so allokiere Platz, ist der ganze */
            					      /* allokierte Platz belegt, so allokiere weiteren Platz. Dann */ 
						                                            /* nimm das Element auf */
{
  if (Activa.activeNumber == Activa.size){
    Activa.size += ACTIVA_ERWEITERUNGSSCHRITT;
    Activa.xs = our_realloc(Activa.xs, Activa.size * sizeof(RegelOderGleichungsT), "extending Activa");
  }
  Activa.xs[Activa.activeNumber] = re;
  Activa.activeNumber++;
}      

/* Liefert zu einem long Wert index den 'Regel oder Gleichungs'-Pointer, der an dieser Stelle in A steht.*/
/* Gibt es keinen Pointer an dieser Stelle, so wird NULL zurueckgegeben. */
/* (Es sollte gelten: RE_getActive(i)->geburtsTag == i. D.h. die zum Zeitpunkt i geborene Gleichung steht */
/* an i. Stelle in A) */
RegelOderGleichungsT RE_getActive (unsigned long index) {
  if ((index <= Activa.activeNumber) && (index >= Activa.offset)) {
    return Activa.xs[index];
  } 
  else {
    /* XXX IO_DruckeFlex("!!!!Error!!! Activa.offset = %d, Activa.activeNumber = %d, you tried to get: %d", Activa.offset, Activa.activeNumber, index);*/
    our_error("Fehler HS007 (NoSuchElementException in Activa.RE_getActive())");
    return NULL;
  }
}   

RegelOderGleichungsT RE_getActiveWithBirthday(AbstractTime birthday) {
  if ((Activa.xs[(unsigned long) birthday - 1])->geburtsTag == birthday) {
    return RE_getActive((unsigned long) birthday - 1);
  } else {
    IO_DruckeFlex("birthday = %d, geburtsTag = %d\n", birthday, (Activa.xs[(unsigned long) birthday - 1])->geburtsTag);
    our_error("Fehler HS008 Birthday != Activa.xs[Birthday - 1]->geburtsTag");
    return NULL;
  }
}

void RE_setActivaOffset(unsigned long offsetValue) {
  if (IO_level4()) {
    IO_DruckeFlex("Activa.offset now %d, was %d\n", offsetValue, Activa.offset);
  }
  Activa.offset = offsetValue;
}

void RE_deleteActivum(RegelOderGleichungsT i_Element)
{
    if (IO_level4()) {
      IO_DruckeFlex("Loeschen der Regel oder Gleichung #%d (*%d)\n", i_Element->Nummer, 
		  i_Element->geburtsTag);
    }
    if (!TP_TermpaarIstRegel(i_Element)) {      
      TP_LinksRechtsFrischLoeschen(TP_Gegenrichtung(i_Element));
      if ((TP_Gegenrichtung(i_Element))->r2) {
	TO_TermLoeschen(TP_Gegenrichtung(i_Element)->r2);
      }
      if ((TP_Gegenrichtung(i_Element))->GescheitertL) {
	TO_TermLoeschen((TP_Gegenrichtung(i_Element))->GescheitertL);
      }
      if ((TP_Gegenrichtung(i_Element))->GescheitertR) {
	TO_TermLoeschen((TP_Gegenrichtung(i_Element))->GescheitertR);
      }
      TP_GNTermpaarLoeschen(TP_Gegenrichtung(i_Element));
    }
    if ((i_Element)->r2) {
      TO_TermLoeschen((i_Element)->r2);
    }
    if ((i_Element)->GescheitertL) {
      TO_TermLoeschen((i_Element)->GescheitertL);
    }
    if ((i_Element)->GescheitertR) {
      TO_TermLoeschen((i_Element)->GescheitertR);
    }
    TP_LinksRechtsFrischLoeschen(i_Element);
    TP_GNTermpaarLoeschen(i_Element);
}

/* Prozedur zum finalen Abraeumen aller Elemente in A */
/* XXX Statistiken werden nicht gefuehrt z.b. DecGleichungenAktuell() */
static void RE_deleteAllActives() {
  RegelOderGleichungsT i_Element;
  unsigned long i;
  for (i = Activa.offset; i < Activa.activeNumber; ++i) {    /*Hier i < activeNumber, da activeNumber ist Anzahl der*/
                                                                                /* Elemente, nicht Index des Letzten*/
    i_Element = Activa.xs[i];
    RE_deleteActivum(i_Element);
  }
}         

void RE_deleteActivesAndIndex(void) {
  RE_RegelIndexLoeschen();
  RE_GleichungsIndexLoeschen();
  RE_deleteAllActives();
}

/*=================================================================================================================*/
/*= XXX. Sonstiges                                                                                                =*/
/*=================================================================================================================*/

void RE_FaktumEinfuegen(RegelOderGleichungsT Faktum)
{
  if (RE_IstRegel(Faktum)){
    RegelEinfuegen(Faktum);
  }
  else {
    GleichungEinfuegen(Faktum);
  }
}

void RE_FaktumToeten(RegelOderGleichungsT Faktum)
/* ist das uebergebene Faktum eine Gleichung, so ist sie ausgezeichnet */
{
  TP_setTodesTag_M(Faktum, HK_getAbsTime_M());
  KPV_KillParent(Faktum);

  if (RE_IstGleichung(Faktum) && !TP_RichtungAusgezeichnet(Faktum)){
    our_error("RE_FaktumToeten: Invariante verletzt");
  }
  if (TP_Gegenrichtung(Faktum) != NULL) {
    TP_setTodesTag_M(TP_Gegenrichtung(Faktum), HK_getAbsTime_M());
    /* KPV_KillParent(TP_Gegenrichtung(Faktum)) ??? */
  }
}

RegelOderGleichungsT RE_ErzeugeFaktumAusFertigenTeilen(int flags, int generation, TermT lhs, TermT rhs)
{
  RegelOderGleichungsT ZuletztErzeugt;

  TP_GNTermpaarHolen(ZuletztErzeugt);
  ZuletztErzeugt->linkeSeite  = lhs;
  ZuletztErzeugt->rechteSeite = rhs;
  BO_TermpaarNormieren(lhs,rhs,&TiefeLinks);
  ZuletztErzeugt->tiefeLinks = TiefeLinks;
  if (flags & 2){
    RechtsUnfreiErzeugen(ZuletztErzeugt);
  }
  else {
    ZuletztErzeugt->r2 = NULL;
  }
  if (flags & 1){
    RE_IncAnzRegelnGesamt();
    ZuletztErzeugt->Nummer = RE_AnzRegelnGesamt(); /* Argl */
  }
  else {
    RE_IncAnzGleichungenGesamt();
    ZuletztErzeugt->Nummer = -RE_AnzGleichungenGesamt(); /* Argl 2*/
  }
  ZuletztErzeugt->lebtNoch = TRUE;
  TP_setGeburtsTag_M(ZuletztErzeugt, HK_getAbsTime_M());
  TP_initializeTodesTag_M(ZuletztErzeugt);
  re_addActive(ZuletztErzeugt);
  ZuletztErzeugt->generation = generation;
  ZuletztErzeugt->RichtungAusgezeichnet = TRUE;
  ZuletztErzeugt->IstMonogleichung = (flags & 4);
  ZuletztErzeugt->DarfNichtReduzieren   = FALSE;
  ZuletztErzeugt->DarfNichtUeberlappen  = (flags & 8);
  ZuletztErzeugt->Grundzusammenfuehrbar = (flags & 16);
  ZuletztErzeugt->ACGrundreduzibel      = (flags & 32);
  ZuletztErzeugt->StrengKonsistent      = (flags & 64);
  ZuletztErzeugt->gzfbStatus   = GZ_undef;
  ZuletztErzeugt->GescheitertL = NULL;
  ZuletztErzeugt->GescheitertR = NULL;
  ZuletztErzeugt->aEntryObserver = KDH_UndefinedObserver;

  if (flags & 1){
    ZuletztErzeugt->Gegenrichtung = NULL;
  }
  else {
    GleichungsT ZuletztErzeugteAntigleichung;

    TP_GNTermpaarHolen(ZuletztErzeugteAntigleichung);
    ZuletztErzeugteAntigleichung->linkeSeite = TO_Termkopie(rhs);
    ZuletztErzeugteAntigleichung->rechteSeite = TO_Termkopie(lhs);
    BO_TermpaarNormieren(ZuletztErzeugteAntigleichung->linkeSeite,ZuletztErzeugteAntigleichung->rechteSeite,&TiefeRechts);
    ZuletztErzeugteAntigleichung->tiefeLinks = TiefeRechts;
    if (flags & 128){
      RechtsUnfreiErzeugen(ZuletztErzeugteAntigleichung);
    }
    else {
      ZuletztErzeugteAntigleichung->r2 = NULL;
    }
    ZuletztErzeugteAntigleichung->Nummer = ZuletztErzeugt->Nummer;
    ZuletztErzeugteAntigleichung->lebtNoch = TRUE;
    TP_setGeburtsTag_M(ZuletztErzeugteAntigleichung, HK_getAbsTime_M());
    TP_initializeTodesTag_M(ZuletztErzeugteAntigleichung);
    ZuletztErzeugteAntigleichung->generation = generation;
    ZuletztErzeugteAntigleichung->RichtungAusgezeichnet = FALSE;
    ZuletztErzeugteAntigleichung->IstMonogleichung      = (flags & 4);
    ZuletztErzeugteAntigleichung->DarfNichtReduzieren   = FALSE;
    ZuletztErzeugteAntigleichung->DarfNichtUeberlappen  = (flags & 8);
    ZuletztErzeugteAntigleichung->Grundzusammenfuehrbar = (flags & 16);
    ZuletztErzeugteAntigleichung->ACGrundreduzibel      = (flags & 32);
    ZuletztErzeugteAntigleichung->StrengKonsistent      = (flags & 64);
    ZuletztErzeugteAntigleichung->gzfbStatus   = GZ_undef;
    ZuletztErzeugteAntigleichung->GescheitertL = NULL;
    ZuletztErzeugteAntigleichung->GescheitertR = NULL;
    ZuletztErzeugteAntigleichung->aEntryObserver = KDH_UndefinedObserver;

    ZuletztErzeugteAntigleichung->Gegenrichtung = ZuletztErzeugt;
    ZuletztErzeugt->Gegenrichtung = ZuletztErzeugteAntigleichung;
  }

  return ZuletztErzeugt;
}

RegelOderGleichungsT RE_ErzeugtesFaktum(SelectRecT selRec)
{
  if (selRec.result == Unvergleichbar){
    return re_ErzeugteGleichung(selRec);
  }
  else if ((selRec.result == Kleiner)||(selRec.result == Groesser)){
    return re_ErzeugteRegel(selRec);
  }
  else {
    our_error("violation of internal invariant (activating trivial fact)");
    return NULL; /* shut up compiler */
  }
}

void RE_AusgabeAktiva(void)
{
  RE_AusgabeRegelmenge(7);
  if (!PA_FailingCompletion() && !RE_GleichungsmengeLeer())
    RE_AusgabeGleichungsmenge (7);
}
