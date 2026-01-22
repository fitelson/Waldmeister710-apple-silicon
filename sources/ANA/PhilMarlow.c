/*=================================================================================================================
  =                                                                                                               =
  =  PhilMarlow.c                                                                                                 =
  =                                                                                                               =
  =  Erkennung algebraischer Strukturen                                                                           =
  =                                                                                                               =
  =  28.04.98 Extraktion aus Ordnungsgenerator. Thomas.                                                           =
  =                                                                                                               =
  =================================================================================================================*/



#include "Ausgaben.h"
#include "compiler-extra.h"
#include <ctype.h>
#include "DSBaumOperationen.h"
#include "Praezedenzgenerator.h"
#include "Parameter.h"
#include "PhilMarlow.h"
#include "SpezNormierung.h"
#include <stdarg.h>
#include "VariablenMengen.h"
#include "XFiles.h"
#include "YFiles.h"



/*=================================================================================================================*/
/*= I. Aufsammeln von Gleichungen und Zielen sowie simple Analysen                                                =*/
/*=================================================================================================================*/

BOOLEAN PM_Existenzziele;                                  /* Besetzung erfolgt schon in SN_SpezifikationAuswerten */

ListenT PM_Gleichungen,
        PM_Ziele;

static void GleichungenUndZielePuffern(void)
{
  int i;
  TermT Flunder1, Flunder2;
  PM_Gleichungen = NULL;
  for (i=0; i<SN_AnzahlGleichungen; i++) {
    term  Term1 = SN_LHS[i],
          Term2 = SN_RHS[i];
    ListenT Eintrag;
    TO_PlattklopfenLRMitMalloc(Term1,Term2,&Flunder1,&Flunder2);
    BO_TermpaarNormierenAlsKP(Flunder1, Flunder2);
    Eintrag = consM(Flunder1, Flunder2);
    ConsM(Eintrag,&PM_Gleichungen);
  }
  reverse(&PM_Gleichungen);
  PM_Ziele = NULL;
  for (i=0; i<SN_AnzahlZiele; i++) {
    term termLeft = SN_Ziele[i].l,
         termRight = SN_Ziele[i].r;
    ListenT Eintrag;
    TO_PlattklopfenLRMitMalloc(termLeft, termRight, &Flunder1, &Flunder2);
    Eintrag = consM(Flunder1, Flunder2);
    ConsM(Eintrag,&PM_Ziele);
  }
  reverse(&PM_Ziele);
}


static void GleichungLoeschenM(void *Gleichung)
{
  TermT s = car((ListenT)Gleichung),
        t = cdr((ListenT)Gleichung);
  TO_TermLoeschenM(s);
  TO_TermLoeschenM(t);
  free_cellM(Gleichung);
}


typedef enum {GarNix, NurA, NurC, AundC} ACStatusT;

static void ACSymboleSuchen(void)
{
   ListenT Index;
   ACStatusT *ACStatus = array_alloc(SO_AnzahlFunktionen,ACStatusT);
   SymbolT Symbol; 
   SO_forFktSymbole(Symbol) 
     ACStatus[Symbol] = GarNix;
   for (Index=PM_Gleichungen; Index; (void)Cdr(&Index)) {
     ListenT Eintrag = car(Index);
     TermT Flunder1 = car(Eintrag),
           Flunder2 = cdr(Eintrag);
     BOOLEAN IstASymbol = TO_IstAssoziativitaet(Flunder1,Flunder2),
             IstCSymbol = !IstASymbol && TO_IstKommutativitaet(Flunder1,Flunder2);
     if (IstASymbol || IstCSymbol) {
       SymbolT Symbol = TO_TopSymbol(Flunder1);
       ACStatusT StatusAlt = ACStatus[Symbol],
                 StatusNeu = StatusAlt | (unsigned)IstASymbol<<(NurA-1) | (unsigned)IstCSymbol<<(NurC-1);
       if (StatusAlt!=StatusNeu && (ACStatus[Symbol] = StatusNeu)==AundC) {
         SO_SymbolInfo[Symbol].IstAC = TRUE;
       }
     }
   }
   our_dealloc(ACStatus);
}


static void DistributivgesetzeSuchen(void)
{
   ListenT Index;
   for (Index=PM_Gleichungen; Index; (void)Cdr(&Index)) {
     ListenT Eintrag = car(Index);
     TermT Flunder1 = car(Eintrag),
           Flunder2 = cdr(Eintrag);
     SymbolT mal = SO_ErstesVarSymb,
             plus = SO_ErstesVarSymb;
     BOOLEAN Aktion = FALSE;
     if (TO_IstDistribution(Flunder1,Flunder2)) {
       mal = TO_TopSymbol(Flunder1);
       plus = TO_TopSymbol(Flunder2);
       Aktion = TRUE;
     }
     else if (TO_IstAntiDistribution(Flunder1,Flunder2)) {
       mal = TO_TopSymbol(Flunder2);
       plus = TO_TopSymbol(Flunder1);
       Aktion = TRUE;
     }
     if (Aktion) {
       SO_SymbolInfo[mal].DistribuiertUeber = plus;
     }
   }
}



/*=================================================================================================================*/
/*= II. Komplettierung von Tafel1                                                                                 =*/
/*=================================================================================================================*/

static TermT VorigerTerm,
             JetzigerTerm;

static SymbolT NaechsteVariable,
               NaechsterOperatorIndex;

static char *VorigerTermAnfang,
            *Lesezeiger,
            *Leseanfang;

static BOOLEAN IstSyntaxzeichen(char Zeichen)
{
  return Zeichen=='(' || Zeichen==',' || Zeichen==')' || Zeichen=='=' || Zeichen==' ';
}

static BOOLEAN IstVariablenzeichen(char Zeichen)
{
  return islower((int)Zeichen);
}

static BOOLEAN IstOperatorzeichen(char Zeichen)
{
  return !IstSyntaxzeichen(Zeichen) && !IstVariablenzeichen(Zeichen) && isgraph((int)Zeichen);
}

static char GeholtesZeichen(void)
{
  test_error(Lesezeiger<Leseanfang, "Gleichungsstring wird beim Lesen unterschritten");
  pr(("%c", *Lesezeiger));
  return *(Lesezeiger--);
}

static void Lesen(char SollZeichen MAYBE_UNUSEDPARAM)
{
#if XF_TEST
  char IstZeichen = GeholtesZeichen();
  test_error(IstZeichen!=SollZeichen, "unerwartetes Zeichen in Gleichungsstring");
#else
  (void) GeholtesZeichen();
#endif
}


#define OhneEntsprechung SHRT_MIN

static SymbolT EntsprechungAbIn(char C, char *String, UTermT Term)
{
  while (Term) {
    char S = *String;
    if (!IstSyntaxzeichen(S)) {
      if (S==C)
        return TO_TopSymbol(Term);
      Term = TO_Schwanz(Term);
    }
    String++;
  }
  return OhneEntsprechung;
}

static SymbolT Entsprechung(char Zeichen)
{
  if (VorigerTerm) {
    SymbolT Resultat = EntsprechungAbIn(Zeichen,VorigerTermAnfang,VorigerTerm);
    if (Resultat!=OhneEntsprechung)
      return Resultat;
  }
  return EntsprechungAbIn(Zeichen,Lesezeiger+2,JetzigerTerm);
}

static SymbolT Variablenentsprechung(char Zeichen)
{
  SymbolT Variable = Entsprechung(Zeichen);
  if (Variable==OhneEntsprechung) {
    Variable = NaechsteVariable;
    test_error(NaechsteVariable==OhneEntsprechung, "zu viele Variablen in Gleichungsstring");
    SO_NaechstesVarSymbD(NaechsteVariable);
  }
  return Variable;
}

static unsigned int Operatorindex(SymbolT Operator) MAYBE_UNUSEDFUNC;
static unsigned int Operatorindex(SymbolT Operator)
{ return (unsigned int) ss_unpair_x(Operator); }

static unsigned int Operatorstelligkeit(SymbolT Operator)
{ return (unsigned int) ss_unpair_y(Operator); }

static SymbolT Operatorentsprechung(char Zeichen, unsigned int SollStelligkeit)
{
  SymbolT Operator = Entsprechung(Zeichen);
  if (Operator==OhneEntsprechung)
    Operator = ss_pair(NaechsterOperatorIndex++,(short)SollStelligkeit);
  else
    test_error(SollStelligkeit!=Operatorstelligkeit(Operator), "Stelligkeitshuddel in Gleichungsstring");
  return Operator;
}


static void VariableUebersetzen(char Zeichen)
{
  SymbolT Variable = Variablenentsprechung(Zeichen);
  TermzellenZeigerT Neu;
  TO_TermzelleHolen(Neu);
  TO_SymbolBesetzen(Neu,Variable);
  TO_NachfBesetzen(Neu,JetzigerTerm);
  TO_EndeBesetzen(Neu,Neu);
  JetzigerTerm = Neu;
}

static void KonstanteUebersetzen(char Zeichen)
{
  SymbolT Operator = Operatorentsprechung(Zeichen,0);
  TermzellenZeigerT Neu;
  TO_TermzelleHolen(Neu);
  TO_SymbolBesetzen(Neu,Operator);
  TO_NachfBesetzen(Neu,JetzigerTerm);
  TO_EndeBesetzen(Neu,Neu);
  JetzigerTerm = Neu;
}


static void TermUebersetzen(void);
static void KomplexumUebersetzen(char Zeichen)
{
  unsigned int Stelligkeit = 1;
  UTermT Ende;
  SymbolT Operator;
  TermzellenZeigerT Neu;
  TermUebersetzen();
  Ende = TO_TermEnde(JetzigerTerm);
  while ((Zeichen = GeholtesZeichen())==',') {
    TermUebersetzen();
    Stelligkeit++;
  }
  test_error(Zeichen!='(', "unerwartetes Zeichen in Gleichungsstring");
  Zeichen = GeholtesZeichen();
  test_error(!IstOperatorzeichen(Zeichen), "Operator erwartet in Gleichungsstring");
  Operator = Operatorentsprechung(Zeichen,Stelligkeit);
  TO_TermzelleHolen(Neu);
  TO_SymbolBesetzen(Neu,Operator);
  TO_NachfBesetzen(Neu,JetzigerTerm);
  TO_EndeBesetzen(Neu,Ende);
  JetzigerTerm = Neu;
}

static void TermUebersetzen(void)
{
  char Zeichen = GeholtesZeichen();
  if (IstVariablenzeichen(Zeichen))
    VariableUebersetzen(Zeichen);
  else if (IstOperatorzeichen(Zeichen))
    KonstanteUebersetzen(Zeichen);
  else if (Zeichen==')')
    KomplexumUebersetzen(Zeichen);
  else
    test_error(TRUE, "illegaler Gleichungsstring");
}


static void TermDrucken(UTermT Term MAYBE_UNUSEDPARAM)
{
#if XF_TEST
  SymbolT Symbol = TO_TopSymbol(Term);
  if (SO_SymbIstVar(Symbol))
    pr(("x%d",SO_VarNummer(Symbol)));
  else
    pr(("F%d/%d",Operatorindex(Symbol),Operatorstelligkeit(Symbol)));
  if (TO_NichtNullstellig(Term)) {
    UTermT NULLTerm = TO_NULLTerm(Term);
    pr(("("));
    Term = TO_ErsterTeilterm(Term);
    TermDrucken(Term);
    while (TO_PhysischUngleich(Term = TO_NaechsterTeilterm(Term),NULLTerm)) {
      pr((","));
      TermDrucken(Term);
    }
    pr((")"));
  }
#else
  ;
#endif
}

#define /*void*/ GleichungDrucken(/*UTermT*/ links, /*UTermT*/ rechts)                                              \
  TermDrucken(links); pr(("=")); TermDrucken(rechts)


static void SignaturErzeugen(GesetzT *Gesetz)
{
  unsigned int i;
  char Zeichen;
  Lesezeiger--;                                                        /* wg. Verwendung von Funktion Entsprechung */
  for_OperatorenC(i,Gesetz,Zeichen) {
    SymbolT Operator = Entsprechung(Zeichen);
    Gesetz->Signatur_Symbol[i] = Operator;
    test_error(i && Operatorstelligkeit(Gesetz->Signatur_Symbol[i-1])<Operatorstelligkeit(Operator),
      "Operatoren in Signatur in Tafel1 nicht nach fallender Stelligkeit angeordnet");
    test_error(i && Operatorstelligkeit(Gesetz->Signatur_Symbol[i-1])==Operatorstelligkeit(Operator)
      && strchr(Leseanfang,Gesetz->Signatur_Zeichen[i-1])>strchr(Leseanfang,Zeichen),
      "gleichstellige Operatoren in Signatur in Tafel1 nicht nach Auftreten angeordnet");
  }
}


static void GesetzUebersetzen(GesetzT *Gesetz)
{
  unsigned int j;
  SymbolT Operator;
  GNTermpaarT Termpaar;
  pr(("Parsen von Gesetz %d: %s\n  Lesen:             ",Gesetz-Tafel1,Gesetz->Gleichung_String));
  TP_GNTermpaarHolen(Termpaar);
  VorigerTerm = JetzigerTerm = NULL;
  NaechsteVariable = SO_ErstesVarSymb;
  NaechsterOperatorIndex = 1;
  Leseanfang = Gesetz->Gleichung_String;
  Lesezeiger = Leseanfang+strlen(Leseanfang)-1;
  TermUebersetzen();
  Termpaar->rechteSeite = JetzigerTerm;
  VorigerTerm = JetzigerTerm;
  JetzigerTerm = NULL;
  VorigerTermAnfang = Lesezeiger+1;
  Lesen(' '), Lesen('='); Lesen(' ');
  TermUebersetzen();
  Termpaar->linkeSeite = JetzigerTerm;
  test_error(Lesezeiger!=Leseanfang-1, "Gleichungsstring nur partiell interpretierbar");
  SignaturErzeugen(Gesetz);
  Gesetz->Gleichung_Termpaar = Termpaar;
  pr(("\n  Resultat:          ")); GleichungDrucken(Termpaar->linkeSeite,Termpaar->rechteSeite);
  pr(("\n  Operatoren:        "));
  for_OperatorenS(j,Gesetz,Operator)
    pr(("F%d/%d  ",Operatorindex(Operator),Operatorstelligkeit(Operator)));
  pr(("\n"));
}


#if XF_TEST
#define Lokus(Fehlermeldung)                                                                                        \
  (Tafelnummer==1 ? Fehlermeldung " in Tafel1" :                                                                    \
   Tafelnummer==3 ? Fehlermeldung " in Tafel3" :                                                                    \
   Tafelnummer==4 ? Fehlermeldung " in semantischer Praezedenzerzeugung" :                                          \
   (our_error("vermurkster Gebrauch des Makros Lokus"),""))
#define test_error_(/*BOOLEAN*/ Fehler, /*char **/ Meldung)                                                         \
  test_error(Fehler,Lokus(Meldung))
void PM_SignaturTesten(char *Signatur, unsigned int Tafelnummer)
{
  unsigned int j;
  for(j=0; j<=maxAnzOperatoren; j++) {
    char Zeichen = Signatur[j];
    if (Zeichen=='\0')
      break;
    test_error_(!IstOperatorzeichen(Zeichen), "ungueltiges Operatorzeichen in Signatureintrag");
    test_error_(Signatur+j!=strrchr(Signatur,Zeichen),
      "Operator mehrfach deklariert in Signatur");
  }
  test_error_(j==0, "leerer Signatureintrag");
  test_error_(j>maxAnzOperatoren, "zu langer Signatureintrag");
}
#else
void PM_SignaturTesten(char *Signatur MAYBE_UNUSEDPARAM, unsigned int Tafelnummer MAYBE_UNUSEDPARAM) 
{ ; }
#endif


static void Tafel1Vortesten(void)
{
#if XF_TEST
  unsigned int i,j;
  char Zeichen;
  GesetzT *Gesetz;
  test_error(!Unbekannt, "keine Gesetznamen ausser bzw. vor 'Unbekannt' angegeben");
  for_Gesetze(i,Gesetz)
    test_error(Gesetz->Name!=i, "falsch markierter Gesetzeintrag in Tafel1");
  test_error(Tafel1[Unbekannt].Name!=Unbekannt, "Tafel1 nicht mit Unbekannt-markiertem Eintrag abgeschlossen");
  for_Gesetze(i,Gesetz) {
    PM_SignaturTesten(Gesetz->Signatur_Zeichen,1);
    for(j=0; j<=maxAnzOperatoren; j++) {
      char Zeichen = Gesetz->Signatur_Zeichen[j];
      test_error(!strchr(Gesetz->Gleichung_String,Zeichen), "in Tafel1 in Signatur deklarierter Operator nicht verwendet");
    }
    for(j=0; (Zeichen = Gesetz->Gleichung_String[j]); j++)
      test_error(IstOperatorzeichen(Zeichen) && !strchr(Gesetz->Signatur_Zeichen,Zeichen),
        "in Tafel1 nicht in Signatur deklarierter Operator verwendet");
    test_error(j==0, "leeres Gesetz in Tafel1");
  }
  test_error(Tafel1[Unbekannt].Signatur_Zeichen[0]!='\0',
    "illegale Signatur im Unbekannt-markiertem Eintrag in Tafel1");
  test_error(Tafel1[Unbekannt].Gleichung_String[0]!='\0',
    "illegales Gesetz im Unbekannt-markiertem Eintrag in Tafel1");
#else
  ;
#endif
}


static void Tafel1Nachtesten(void)
{
#if XF_TEST
  unsigned int i,j;
  GesetzT *i_Gesetz, *j_Gesetz;
  for_Gesetze(i,i_Gesetz) {
    GNTermpaarT i_Termpaar = i_Gesetz->Gleichung_Termpaar;
    TermT i_links = TP_LinkeSeite(i_Termpaar),
          i_rechts = TP_RechteSeite(i_Termpaar);
    BO_TermpaarNormierenAlsKP(i_links,i_rechts);
    for_Gesetze(j,j_Gesetz)
      if (i<j) {
        GNTermpaarT j_Termpaar = j_Gesetz->Gleichung_Termpaar;
        TermT j_links = TP_LinkeSeite(j_Termpaar),
              j_rechts = TP_RechteSeite(j_Termpaar);
        BO_TermpaarNormierenAlsKP(j_links,j_rechts);
        IO_DruckeFlex("Doublettentest Tafel1 %d-%d: ",i+1,j+1);
        GleichungDrucken(i_links,i_rechts);
        IO_DruckeFlex(" vs. ");
        GleichungDrucken(j_links,j_rechts);
        IO_DruckeFlex("\n");
        test_error(TO_TermeGleich(i_links,j_links) && TO_TermeGleich(i_rechts,j_rechts), "Doublette in Gleichungen in Tafel1");
        j_links = TP_RechteSeite(j_Termpaar);
        j_rechts = TP_LinkeSeite(j_Termpaar);
        BO_TermpaarNormierenAlsKP(j_links,j_rechts);
        test_error(TO_TermeGleich(i_links,j_links) && TO_TermeGleich(i_rechts,j_rechts), "Doublette in Gleichungen in Tafel1");
      }
  }
#else
  ;
#endif
}


static void Tafel1Initialisieren(void)
{
  unsigned int i;
  GesetzT *Gesetz;
  Tafel1Vortesten();
  for_Gesetze(i,Gesetz)
    GesetzUebersetzen(Gesetz);
  Tafel1Nachtesten();
}



/*=================================================================================================================*/
/*= III. Komplettierung von Tafel3                                                                                =*/
/*=================================================================================================================*/

static const char Oednis[maxAnzOperatoren+1];   /* leeres Signaturfeld fuer oberpingeligen Vergleich auf Tafelende */

static void Tafel3Vortesten(void)
{
#if XF_TEST
  unsigned int i,j,k;
  ZuordnungsT *Zuordnung, *j_Zuordnung;
  PraemissenT *Praemisse, *k_Praemisse;
  char *f, *g;
  SymbolT *F, *G;
  GesetzT *Gesetz;
  test_error(!Orkus, "keine Strukturnamen ausser bzw. vor 'Orkus' angegeben");
  test_error(!AnzZuordnungen, "keine Zuordnungen angegeben in Tafel3");
  for_Zuordnungen(i,Zuordnung) {
    test_error(Zuordnung->Name>=Orkus, "ungueltiger Strukturname in Tafel3");
    for(j=0; j<=maxAnzPraemissen; j++)
      if (Zuordnung->Praemissen[j].Name==0 && !memcmp(Zuordnung->Praemissen[j].Signatur_Zeichen,Oednis,maxAnzOperatoren+1))
        break;
    test_error(j>maxAnzPraemissen, "zu viele Praemisseneintraege in Zuordnung in Tafel3");
    test_error(!j, "keine Praemisseneintraege in Zuordnung in Tafel3");
    Zuordnung->Praemissen[j].Name = Unbekannt;  /* Diese Zuweisung erfolgt nochmals in Tafel3Initialisieren, damit */
    PM_SignaturTesten(Zuordnung->Signatur_Zeichen,3);         /* der Iterator for_Praemissen auch im Normalbetrieb */
    for_Praemissen(Zuordnung,j,Praemisse) {                           /* verwendet werden kann. Konsistent halten! */
      GesetznamenT Gesetzname = Praemisse->Name;
      test_error(Gesetzname>=Unbekannt, "ungueltiger Gesetzname in Praemisse in Tafel3");
      PM_SignaturTesten(Praemisse->Signatur_Zeichen,3);
      test_error(strlen(Praemisse->Signatur_Zeichen)!=strlen(Tafel1[Gesetzname].Signatur_Zeichen),
        "von Gesetzdefinition abweichende Signaturlaenge in Praemisse in Tafel3");
    }
  }
  for_Zuordnungen(i,Zuordnung) {
    for (f=Zuordnung->Signatur_Zeichen; *f; f++) {
      int fStelligkeit = -1;
      for_Praemissen(Zuordnung,j,Praemisse) {
        for (g=Praemisse->Signatur_Zeichen; *g; g++)
          if (*f==*g) {
            GesetznamenT gGesetz = Praemisse->Name;
            unsigned int gIndex = g-Praemisse->Signatur_Zeichen;
            SymbolT gSymbol = Tafel1[gGesetz].Signatur_Symbol[gIndex];
            int gStelligkeit = Operatorstelligkeit(gSymbol);
            if (fStelligkeit==-1) {
              fStelligkeit = gStelligkeit;
              Zuordnung->Signatur_Symbol[f-Zuordnung->Signatur_Zeichen] = ss_pair(j,fStelligkeit);
            }
            else
              test_error(fStelligkeit!=gStelligkeit, "Operator mit variabler Stelligkeit in Praemissenfeld in Tafel3");
            break;
          }
        for_Praemissen(Zuordnung,k,k_Praemisse)
          if (j<k)
            test_error(Praemisse->Name==k_Praemisse->Name && !strcmp(Praemisse->Signatur_Zeichen,k_Praemisse->Signatur_Zeichen),
              "identische Praemissen in Zuordnung in Tafel3");
      }
      test_error(fStelligkeit==-1, "Operator in Signatur von Zuordnung in Tafel3 ohne Auftreten in Praemissen");
    }
    for_Praemissen(Zuordnung,j,Praemisse)
      for (g=Praemisse->Signatur_Zeichen; *g; g++) {
        for (f=Zuordnung->Signatur_Zeichen; *f; f++)
          if (*f==*g)
            break;
        test_error(!*f, "Operator in Praemisse in Tafel3 ohne Auftreten in Signatur der Zuordnung");
      }
    for (g=Zuordnung->Signatur_Zeichen+1, F=Zuordnung->Signatur_Symbol; G=F+1, *g; F++, g++) 
      test_error(Operatorstelligkeit(*F)<Operatorstelligkeit(*G),
        "in Zuordnungssignatur in Tafel3 Operatoren nicht nach fallender Stelligkeit angeordnet");
    for_Zuordnungen(j,j_Zuordnung)
      if (i>j && Zuordnung->Name==j_Zuordnung->Name) {
        test_error(strcmp(Zuordnung->Signatur_Zeichen,j_Zuordnung->Signatur_Zeichen),
          "Zuordnungen verschiedener Signaturen auf dieselbe Struktur in Tafel3");
        for (f=Zuordnung->Signatur_Zeichen, F=Zuordnung->Signatur_Symbol, G=j_Zuordnung->Signatur_Symbol; *f; f++, F++, G++)
          test_error(Operatorstelligkeit(*F)!=Operatorstelligkeit(*G),"Operator in Struktur in Tafel3 mit nichtkonstanter Stelligkeit");
      }
  }
  for_Strukturen(i) {
    for_Zuordnungen(j,Zuordnung)
      if (i==Zuordnung->Name)
        break;
    test_error(j==AnzZuordnungen, "Struktur ohne Zuordnung in Tafel3");
  }
  for_Gesetze(i,Gesetz) {
    for_Zuordnungen(j,Zuordnung) {
      for_Praemissen(Zuordnung,k,Praemisse)
        if (Praemisse->Name==Gesetz->Name)
          break;
      if (Praemisse->Name==Gesetz->Name)
        break;
    }
    IO_DruckeFlex("Suchen Verwendung von Gesetz %d in Tafel3\n",i+1);
    test_error(Praemisse->Name!=Gesetz->Name, "in Tafel1 Gesetz ohne Verwendung in Tafel3");
  }
#else
  ;
#endif
}


static void Tafel3Initialisieren(void)
{
  unsigned int i,j;
  ZuordnungsT *Zuordnung;
  Tafel3Vortesten();
  for_Zuordnungen(i,Zuordnung) {
    for_Operatoren(j,Zuordnung)
      Zuordnung->Signatur_Symbol[j] = OhneEntsprechung;
    for(j=0; j<maxAnzPraemissen; j++)
      if (Zuordnung->Praemissen[j].Name==0 && !memcmp(Zuordnung->Praemissen[j].Signatur_Zeichen,Oednis,maxAnzOperatoren+1))
        break;
    Zuordnung->Praemissen[j].Name = Unbekannt;             /* Diese Zuweisung erfolgt im Testbetrieb auch schon in */
  }                              /* Tafel3Vortesten, damit dort der Iterator for_Praemissen verwendet werden kann. */
}                                                                                            /* Konsistent halten! */



/*=================================================================================================================*/
/*= IV. Aufbau der Ordnung zur Aufloesung von Strukturkonflikten                                                  =*/
/*=================================================================================================================*/

#if 0
static void AufNaechstesPhi(ZuordnungsT *A, ZuordnungsT *B MAYBE_UNUSEDPARAM)
{                                                                         /* kleine Fingeruebung auf Zeichenketten */
  unsigned int i = strlen(PhiFeld)-1;
  char *c_neu;
  BOOLEAN Ueberlauf;
  do {
    c_neu = strchr(A->Signatur_Zeichen,PhiFeld[i]);
    do
      Ueberlauf = !*++c_neu;
    while (!Ueberlauf && our_strnchr(PhiFeld,*c_neu,i));
  } while (Ueberlauf && i--);
  if (Ueberlauf)
    PhiFeld[0] = '\0';
  else {
    PhiFeld[i] = *c_neu;
    c_neu = A->Signatur_Zeichen;
    while (++i<strlen(PhiFeld)) {
      while (our_strnchr(PhiFeld,*c_neu,i))
        c_neu++;
      PhiFeld[i] = *c_neu;
    }
  }
}
#endif

static char Phi(char BOperator, ZuordnungsT *ZuordnungA, unsigned int i[], unsigned int k, ZuordnungsT *ZuordnungB)
{
  unsigned int j;
  for(j=0; j<k; j++) {
    char *A = ZuordnungA->Praemissen[i[j]].Signatur_Zeichen,
         *B = ZuordnungB->Praemissen[j].Signatur_Zeichen,
         *Position = strchr(B,BOperator);
    if (Position)
      return A[Position-B];
  }
  return '\0';
}


static BOOLEAN PraemissenPhiGleich(ZuordnungsT *ZuordnungA, unsigned int i[], unsigned int k, ZuordnungsT *ZuordnungB)
{
  PraemissenT *A = &ZuordnungA->Praemissen[i[k]],
              *B = &ZuordnungB->Praemissen[k];
  if (A->Name==B->Name) {
    unsigned int j;
    for_Operatoren(j,A) {
      char a = A->Signatur_Zeichen[j],
           b = B->Signatur_Zeichen[j],
           Phi_b = Phi(b,ZuordnungA,i,k,ZuordnungB);
      if (!Phi_b) {                                                           /* testen, ob Injektivitaet erfuellt */
        char sonstiges_b;
        unsigned int l;
        for_OperatorenC(l,ZuordnungB,sonstiges_b)
          if (b!=sonstiges_b && a==Phi(sonstiges_b,ZuordnungA,i,k,ZuordnungB))
            return FALSE;
        /* Der Wertebereich der Fortsetzung von Phi auf dem Praefix der Signatur von B muss nicht betrachtet
           werden: Wuerde dort ein sonstiges_b auf a abgebildet werden, so muesste a mehrfach in der Signatur
           von A auftreten, was aber ausgeschlossen ist. */
      }
      else
        if (Phi_b!=a)
          return FALSE;
    }
    return TRUE;
  }
  else
    return FALSE;
}


static void PhiDrucken(ZuordnungsT *ZuordnungA MAYBE_UNUSEDPARAM, unsigned int i[] MAYBE_UNUSEDPARAM,
  unsigned int k MAYBE_UNUSEDPARAM, ZuordnungsT *ZuordnungB MAYBE_UNUSEDPARAM)
{
#if XF_TEST
  unsigned int j;
  char b;
  for(j=0; j<k; j++) {
    PraemissenT *A = &ZuordnungA->Praemissen[i[j]],
                *B = &ZuordnungB->Praemissen[j];
    char *AName = Tafel1[A->Name].Klartext,
         *BName = Tafel1[B->Name].Klartext;
    pr(("B=%s(%s) auf A=%s(%s)\n",BName,B->Signatur_Zeichen,AName,A->Signatur_Zeichen));
  }
  pr(("mit"));
  for_OperatorenC(j,ZuordnungB,b)
    pr((" %c=>%c",b,Phi(b,ZuordnungA,i,k,ZuordnungB)));
  pr(("\n"));
#else
  ;
#endif
}


static BOOLEAN ZuordnungQuasiSpezieller(ZuordnungsT *A, ZuordnungsT *B)
{
  if (strlen(A->Signatur_Zeichen)<strlen(B->Signatur_Zeichen))
    return FALSE;
  else {
    unsigned int m = 0, n = 0,
                 k;
    PraemissenT *Praemisse;
    for_Praemissen(A,k,Praemisse) m++;
    for_Praemissen(B,k,Praemisse) n++;
    if (m<n)
      return FALSE;
    else {
      unsigned int i[maxAnzPraemissen];
      i[k = 0] = UINT_MAX;                                                              /* korrekt! Vgl. KR S. 188 */
      do
        while (++i[k]<m)
          if (PraemissenPhiGleich(A,i,k,B)) {
            if (++k==n) {
              pr(("\nSpezialisierungsgeordnete Zuordnungen:\n"));
              PhiDrucken(A,i,n,B);
              return TRUE;
            }
            else
              i[k] = UINT_MAX;
	  }
      while (k-->0);
      return FALSE;
    }
  }
}


static BOOLEAN StrukturQuasiSpezieller(StrukturnamenT A, StrukturnamenT B)
{
  ZuordnungsT *ZuordnungA,
              *ZuordnungB;
  unsigned int i,j;
  for_Zuordnungen(i,ZuordnungA)
    if (A==ZuordnungA->Name)
      for_Zuordnungen(j,ZuordnungB)
        if (B==ZuordnungB->Name)
          if (ZuordnungQuasiSpezieller(ZuordnungA,ZuordnungB))
            return TRUE;
  return FALSE;
}


static VergleichsT Ordnung[AnzStrukturen+1][AnzStrukturen+1];

static void OrdnungDrucken(void)
{
#if XF_TEST
  unsigned int i,
               N = 0;
  StrukturnamenT A;
  StrukturT *StrukturA;
  for_StrukturenMitOrkusS(A,StrukturA)
    Maximieren(&N,strlen(StrukturA->Klartext));
  pr(("\n"));
  for (i=0; i<N; i++) {
    pr(("%*s",N,""));
    for_StrukturenMitOrkusS(A,StrukturA) {
      char *s = StrukturA->Klartext;
      unsigned int n = strlen(s);
      pr(("  %c",i>=N-n ? s[i+n-N] : ' '));
    }
    pr(("\n"));
  }
  pr(("\n"));
  for_StrukturenMitOrkusS(A,StrukturA) {
    StrukturnamenT B;
    pr(("%*s",N,StrukturA->Klartext));
    for_StrukturenMitOrkus(B) {
      VergleichsT v = Ordnung[A][B];
      char c =
        v==Kleiner ? '<' :
        v==Groesser ? '>' :
        v==Gleich ? '=' :
        v==Unvergleichbar ? '#' :
        '?';
      pr(("  %c",c));
    }
    pr(("\n"));
  }
#else
  ;
#endif
}

static void OrdnungAufbauen(void)
{
  StrukturnamenT A, B;
  for_Strukturen(A) {
    for_Strukturen(B)
      if (A<B) {
        BOOLEAN A_gr_aequiv_B = StrukturQuasiSpezieller(A,B),
                B_gr_aequiv_A = StrukturQuasiSpezieller(B,A);
        VergleichsT Verhaeltnis = (VergleichsT)
          (A_gr_aequiv_B ? B_gr_aequiv_A ? Gleich : Groesser : B_gr_aequiv_A ? Kleiner : Unvergleichbar),
                    AntiVerhaeltnis = (VergleichsT)
          (Verhaeltnis==Groesser ? Kleiner : Verhaeltnis==Kleiner ? Groesser : Verhaeltnis);
        Ordnung[A][B] = Verhaeltnis;
        Ordnung[B][A] = AntiVerhaeltnis;
      }
      else if (A==B)
        Ordnung[A][B] = Gleich;
    Ordnung[Orkus][A] = Kleiner;
    Ordnung[A][Orkus] = Groesser;
  }
  Ordnung[Orkus][Orkus] = Gleich;
  OrdnungDrucken();
}


static BOOLEAN StrukurSpezieller(StrukturnamenT A, StrukturnamenT B)
{
  VergleichsT v = Ordnung[A][B];
  return v==Groesser || (v==Unvergleichbar && A<B);
}


static void StrukturSpezialisieren(StrukturnamenT *Struktur, ZuordnungsT **PassendeZuordnung, ZuordnungsT *Kandidatin)
{
  if (StrukurSpezieller(Kandidatin->Name,*Struktur)) {
    *Struktur = Kandidatin->Name;
    *PassendeZuordnung = Kandidatin;
  }
}



/*=================================================================================================================*/
/*= V. Erzeugung von Komprimaten                                                                                  =*/
/*=================================================================================================================*/

static ListenT Komprimate;

static SV_SpeicherManagerT PM_FreispeicherManager;

typedef struct {
  GesetznamenT Name;
  SymbolT Signatur_Symbol[maxAnzOperatoren+1];
  BOOLEAN Benutzt;
  VMengenT Datum;
} KomprimatT;

static SymbolT BindungEinfach(SymbolT Symbol, TermT Start, TermT start, UTermT Schluss)
{
  while (TO_PhysischUngleich(Start,Schluss))
    if (SO_SymbGleich(Symbol,TO_TopSymbol(Start)))
      return TO_TopSymbol(start);
    else
      Start = TO_Schwanz(Start), start = TO_Schwanz(start);
  return OhneEntsprechung;
}
static SymbolT Bindung(SymbolT Symbol, TermT Jetzt, TermT jetzt, UTermT Index, TermT Vorher, TermT vorher)
{ SymbolT Resultat = BindungEinfach(Symbol,Jetzt,jetzt,Index);
  return Resultat==OhneEntsprechung ? BindungEinfach(Symbol,Vorher,vorher,NULL) : Resultat;
}

static BOOLEAN SchonBenutzt(SymbolT symbol, TermT jetzt, UTermT index, TermT vorher)
{
  return Bindung(symbol,jetzt,jetzt,index,vorher,vorher)!=OhneEntsprechung;
}

static BOOLEAN SeitePasst(TermT Jetzt, TermT jetzt, TermT Vorher, TermT vorher)
{
  UTermT Index = Jetzt,
         index = jetzt;
  do {
    SymbolT S = TO_TopSymbol(Index),
            s = TO_TopSymbol(index),
            E = Bindung(S,Jetzt,jetzt,Index,Vorher,vorher);
    if (SO_SymbUngleich(E,s)) {
      if (E!=OhneEntsprechung || SchonBenutzt(s,jetzt,index,vorher))
        return FALSE;
      if (SO_SymbIstVar(S)) {
        if (SO_SymbIstFkt(s))
          return FALSE;
      }
      else
        if (SO_SymbIstVar(s) || Operatorstelligkeit(S)!=SO_Stelligkeit(s))
          return FALSE;
    }
  } while ((Index = TO_Schwanz(Index), index = TO_Schwanz(index)));
  return TRUE;
}


static BOOLEAN GesetzPasst(GesetzT *Gesetz, TermT links, TermT rechts)
{
  GNTermpaarT Termpaar = Gesetz->Gleichung_Termpaar;
  TermT Links = Termpaar->linkeSeite,
        Rechts = Termpaar->rechteSeite;
  return SeitePasst(Links,links,NULL,NULL) && SeitePasst(Rechts,rechts,Links,links);
}


static SymbolT Substitut(SymbolT Symbol, TermT links, TermT rechts, GesetzT *Gesetz)
{
  GNTermpaarT Termpaar = Gesetz->Gleichung_Termpaar;
  TermT Links = Termpaar->linkeSeite,
        Rechts = Termpaar->rechteSeite;
  return Bindung(Symbol,Links,links,NULL,Rechts,rechts);
}


static void KomprimatDrucken(KomprimatT *Komprimat MAYBE_UNUSEDPARAM)
{
#if XF_TEST
  unsigned int j;
  GesetzT *Gesetz = &Tafel1[Komprimat->Name];
  pr(("%s(",Gesetz->Klartext));
  for_Operatoren(j,Gesetz) {
    SymbolT Operator = Komprimat->Signatur_Symbol[j];
    pr(("%s%s",j ? "," : "",IO_FktSymbName(Operator)));
  }
  pr((")"));
#else
  ;
#endif
}

static KomprimatT *Termpaarkomprimat(TermT links, TermT rechts)
{
  unsigned int i;
  GesetzT *Gesetz;
  for_Gesetze(i,Gesetz)
    if (GesetzPasst(Gesetz,links,rechts) || (TO_TermeTauschen(&links,&rechts), GesetzPasst(Gesetz,links,rechts))) {
      unsigned int j;
      SymbolT Operator;
      KomprimatT *Komprimat;
      SV_Alloc(Komprimat,PM_FreispeicherManager,KomprimatT*);
      Komprimat->Name = Gesetz->Name;
      pr(("        "));
      for_OperatorenS(j,Gesetz,Operator)
        Komprimat->Signatur_Symbol[j] = Substitut(Operator,links,rechts,Gesetz);
      Komprimat->Benutzt = FALSE;
      KomprimatDrucken(Komprimat);
      pr(("\n"));
      return Komprimat;
    }
  pr(("        Unbekannt\n"));
  return NULL;
}


static void KomprimatErzeugen(void *Gleichung)
{
  TermT s = car((ListenT)Gleichung),
        t = cdr((ListenT)Gleichung);
  KomprimatT *Verdichtung = Termpaarkomprimat(s,t);
  if (Verdichtung)
    Cons(Verdichtung,&Komprimate);
}


static void KomprimatLoeschen(void *Komprimat)
{
  SV_Dealloc(PM_FreispeicherManager,Komprimat);
}


#define for_Komprimate(/*ListenT*/ Index, /*KomprimatT**/ Komprimat)                                                \
  for(Index=Komprimate; Index ? (Komprimat = car(Index), TRUE) : FALSE; (void)Cdr(&Index))

static void KomprimateDrucken(void)
{
#if XF_TEST
  ListenT Index;
  KomprimatT *Komprimat;
  pr(("\n\nKomprimate:\n"));
  for_Komprimate(Index,Komprimat) {
    pr(("        "));
    KomprimatDrucken(Komprimat);
    pr(("\n"));
  }
#else
  ;
#endif
}


static void KomprimateErzeugen(void)
{
  Komprimate = NULL;
  mapc(KomprimatErzeugen,PM_Gleichungen);
  KomprimateDrucken();
}



/*=================================================================================================================*/
/*= VI. Detektieren von Strukturen                                                                                =*/
/*=================================================================================================================*/

static void EssenzHerausdestillieren(ZuordnungsT *Zuordnung, VMengenT *Datum)
{
  unsigned int i;
  SymbolT Symbol;
  VM_NeueMengeSeiLeer(*Datum);
  for_OperatorenS(i,Zuordnung,Symbol)
    if (Symbol!=OhneEntsprechung)
      VM_ElementEinfuegen(Datum,-(short int)i);
}

static void EssenzEinfloessen(ZuordnungsT *Zuordnung, VMengenT *Datum)
{
  unsigned int i;
  for_Operatoren(i,Zuordnung)
    if (VM_NichtEnthaltenIn(Datum,-(short int)i))
      Zuordnung->Signatur_Symbol[i] = OhneEntsprechung;
  VM_MengeLoeschen(Datum);
}


static BOOLEAN Passabel_(ZuordnungsT *Zuordnung, PraemissenT *Praemisse, KomprimatT *Komprimat)
{
  unsigned int i,j,k;
  char ZeichenZ,
       ZeichenP;
  for_OperatorenC(i,Praemisse,ZeichenP) {
    SymbolT SymbolIst = Komprimat->Signatur_Symbol[i],
            SymbolSoll = OhneEntsprechung;                                            /* gegen Uebersetzer-Warnung */
    for_OperatorenCS(j,Zuordnung,ZeichenZ,SymbolSoll)
      if (ZeichenZ==ZeichenP)
        break;
    if (SymbolSoll==OhneEntsprechung) {
      for_OperatorenS(k,Zuordnung,SymbolSoll)
        if (SymbolSoll==SymbolIst)
          return FALSE;
      Zuordnung->Signatur_Symbol[j] = SymbolIst;
    }
    else if (SymbolSoll!=SymbolIst)
      return FALSE;
  }
  return TRUE;
}


static BOOLEAN Passabel(ZuordnungsT *Zuordnung, PraemissenT *Praemisse, KomprimatT *Komprimat)
{
  if (Komprimat->Benutzt || Komprimat->Name!=Praemisse->Name)
    return FALSE;
  else {
    BOOLEAN Resultat;
    EssenzHerausdestillieren(Zuordnung,&Komprimat->Datum);
    if ((Resultat = Passabel_(Zuordnung,Praemisse,Komprimat))) {
      unsigned int i;
      SymbolT Zeichen;
      Komprimat->Benutzt = TRUE;
      pr(("        %s(", Tafel1[Praemisse->Name].Klartext));
      for_OperatorenC(i,Praemisse,Zeichen)
        pr(("%s%c/%s", i ? ", " : "", Zeichen,
          IO_FktSymbName(Komprimat->Signatur_Symbol[i])));
      pr((")\n"));
    }
    else
      EssenzEinfloessen(Zuordnung,&Komprimat->Datum);
    return Resultat;
  }
}


static BOOLEAN PraemissenDurchgenudelt(ZuordnungsT *Zuordnung, PraemissenT *Praemisse)
{
  if (Praemisse->Name==Unbekannt) {
    unsigned int i;
    char Zeichen;
    SymbolT Symbol;
    pr(("     ok mit Signatur "));
    for_OperatorenCS(i,Zuordnung,Zeichen,Symbol)
      pr(("%s%c/%s", i ? ", " : "", Zeichen, IO_FktSymbName(Symbol)));
    pr(("\n"));
    return TRUE;
  }
  else {
    ListenT k;
    KomprimatT *Komprimat;
    for_Komprimate(k,Komprimat)
      if (Passabel(Zuordnung,Praemisse,Komprimat)) {
        BOOLEAN TieferErfolg = PraemissenDurchgenudelt(Zuordnung,Praemisse+1);
        Komprimat->Benutzt = FALSE;
        if (TieferErfolg)
          return TRUE;
        EssenzEinfloessen(Zuordnung,&Komprimat->Datum);
      }
    return FALSE;
  } 
/* N.B.
   Sichtlich kann ein und dieselbe Zuordnung auf mehrere Weisen auf eine Spezifikation matchen, man betrachte 
   etwa 0 und 1 in den Zuordnungen fuer die Struktur BoolescheAlgebra. Die jeweils in der Spezifikation ge-
   matchten Symbole sind dann aber im erkannten Spezifikationsteil isomorph und folglich nicht unterscheidbar,
   so dass in dieser Funktion nach dem ersten Treffer abgebrochen werden kann. 
*/
}


static BOOLEAN ZuordnungPasst(ZuordnungsT *Zuordnung)
{
  return PraemissenDurchgenudelt(Zuordnung,Zuordnung->Praemissen);
}


static ZuordnungsT *DetektierteZuordnung(void)
{
  StrukturnamenT SpeziellsteStruktur = Orkus;
  ZuordnungsT *PassendeZuordnung;
  unsigned int i;
  ZuordnungsT *Zuordnung;
  for_Zuordnungen(i,Zuordnung) {
    pr(("\nZuordnung %s:\n",Tafel2[Zuordnung->Name].Klartext));
    if (ZuordnungPasst(Zuordnung))
      StrukturSpezialisieren(&SpeziellsteStruktur,&PassendeZuordnung,Zuordnung);
  }
  return SpeziellsteStruktur==Orkus ? NULL : PassendeZuordnung;
}



/*=================================================================================================================*/
/*= VII. Auswertung von Tafel2                                                                                    =*/
/*=================================================================================================================*/

StrategieT **GewaehlteStrategien;    /* Nicht static, damit Patch durch Konsulat moeglich. Unsauber, aber einfach. */
static StrukturnamenT DetektierteStruktur;
static ZuordnungsT *PassendeZuordnung;

static void Tafel2Vortesten(void)
{
#if XF_TEST
  unsigned int i;
  StrukturT *Struktur;
  for_StrukturenS(i,Struktur)
    test_error(Struktur->Name!=i, "falsch markierter Struktureintrag in Tafel2");
  test_error(Tafel2[Orkus].Name!=Orkus, "Tafel2 nicht mit Orkus-markiertem Eintrag abgeschlossen");
#else
  ;
#endif
}


static StrategieT **Strategien;
static unsigned int AnzahlStrategien;
#define /*StrategieT**/ AktuelleStrategie (Strategien[AnzahlStrategien-1])
#define /*char**/ AktuelleOptionen (AktuelleStrategie->Optionen)
static unsigned int maxOptionenlaenge;

static void InitAktuelleStrategie(void)
{
  AktuelleStrategie = struct_alloc(StrategieT);
  AktuelleOptionen = string_alloc(maxOptionenlaenge = PM_ErsteOptionenlaenge);
  AktuelleOptionen[0] = '\0';
}

static void TiniAktuelleStrategie(unsigned int Schrittzahl)
{
  AktuelleStrategie->Schrittzahl = Schrittzahl;
  string_realloc(AktuelleOptionen,strlen(AktuelleOptionen));
}


static void StartAufzeichnung(void)
{
  Strategien = array_alloc(AnzahlStrategien = 1,StrategieT*);
  InitAktuelleStrategie();
}



void NeueStrategie(unsigned int Schrittzahl)
{
  TiniAktuelleStrategie(Schrittzahl);
  array_realloc(Strategien,++AnzahlStrategien,StrategieT*);
  InitAktuelleStrategie();
}


static StrategieT **StoppAufzeichnung(void)
{
  char *s = strchr((*Strategien)->Optionen,'@');
  TiniAktuelleStrategie(0);
  if (s)
    *s = '\0';
  return Strategien;
}


#define /*void*/ cputc(/*char*/ c)                                                                                  \
  (*(Cursor++) = c)

#define /*unsigned int*/ cprintf(/*char**/ Format, Arg)                                                             \
  Cursor += our_sprintf((Cursor,Format,Arg))

void oprintf(char *Format MAYBE_UNUSEDPARAM, ...)
{                     /* man stdarg => "Multiple traversals, each bracketed by va_start and va_end, are possible." */
  BOOLEAN LeerzeichenDavorsetzen = AktuelleOptionen[0]!='\0';
  unsigned int JetztLaenge = strlen(AktuelleOptionen),
               Laenge = JetztLaenge+(LeerzeichenDavorsetzen ? 1u : 0u);
  char *Cursor;
  unsigned int d;
  char *s = Format;
  va_list ap;

  va_start(ap, Format);                                                              /* 1. Ausgabelaenge bestimmen */
  while (*s)
    if (*s=='%') {
      switch (*++s) {
      case '%':
        Laenge++;
        break;
      case 's':
        Laenge += strlen(va_arg(ap,char*));
        break;
      case 'd':
        Laenge += LaengeNatZahl(va_arg(ap,unsigned int));
        break;
      default:
        test_error(TRUE,"unbekanntes Formatzeichen in oprintf");
        break;
      }
      s++;
    }
    else
      Laenge++, s++;
  va_end(ap);

  if (Laenge>maxOptionenlaenge)                                              /* 2. AktuelleOptionen ggf. ausdehnen */
    string_realloc(AktuelleOptionen,maxOptionenlaenge = minKXgrY(PM_ErsteOptionenlaenge,Laenge));
  Cursor = AktuelleOptionen+JetztLaenge;
  if (LeerzeichenDavorsetzen)
    *(Cursor++) = ' ';

  va_start(ap, Format);                                                           /* 3. AktuelleOptionen bedrucken */
  while (*Format)
    if (*Format=='%') {
      switch (*++Format) {
      case '%':
        cputc('%');
        break;
      case 's':
        s = va_arg(ap,char*);
        cprintf("%s",s);
        break;
      case 'd':
        d = va_arg(ap,signed int);
        cprintf("%d",d);
        break;
      }
      Format++;
    }
    else
      cputc(*(Format++));
  cputc('\0');
  va_end(ap);
}


#define AufGehts() (void)((void)0
#define ZuGehts()  0)

#define __ Mantelung(
#define _ ,__
#define Mantelung(Strukturname, Strategienausdruck)                                                                 \
    ZuGehts();                                                                                                      \
    break;                                                                                                          \
  case Strukturname:                                                                                                \
    GewaehlteStrategien = (StartAufzeichnung(), Strategienausdruck, StoppAufzeichnung());                           \
    AufGehts()

static void Tafel2Auswerten(StrukturnamenT Struktur,
                            Signatur_SymbolT *sig MAYBE_UNUSEDPARAM, Signatur_ZeichenT *sig_c MAYBE_UNUSEDPARAM)
{
  switch (Struktur) {
  default: 
    AufGehts(),
    Tafel2(),
    Mantelung(Orkus,Orkusstrategien),
    ZuGehts();
  }
}

#undef Mantelung



/*=================================================================================================================*/
/*= VIII. Strategienmanagement                                                                                    =*/
/*=================================================================================================================*/

#define for_Strategien(/*unsigned int*/ i, /*StrategieT**/ Strategie)                                               \
  for(Strategie = GewaehlteStrategien[i = 0]; Strategie; Strategie = Strategie->Schrittzahl ? Strategien[++i] : NULL)

static void StrategienFokussieren(ZuordnungsT *Zuordnung)
{
  Signatur_SymbolT *sig = &(Zuordnung->Signatur_Symbol);
  Signatur_ZeichenT *sig_c = &(Zuordnung->Signatur_Zeichen);
  PassendeZuordnung = Zuordnung;
  DetektierteStruktur = (StrukturnamenT) (Zuordnung ? Zuordnung->Name : Orkus);  /* Hier benimmt sich tcc seltsam. */
  Tafel2Auswerten(DetektierteStruktur,sig,sig_c);
}


StrategieT *PM_Strategie(unsigned int AnlaufNr)
{
  return GewaehlteStrategien[AnlaufNr-1];
}


void PM_StrategienDrucken(void)
{
  if (PA_Automodus()) {
#if !COMP_COMP && !COMP_POWER
    StrategieT *Strategie;
    unsigned int i;
    char Zeichen;
    SymbolT Symbol;
#endif

    printf("\nDetected structure: %s",Tafel2[DetektierteStruktur].Klartext);
#if !COMP_COMP && !COMP_POWER
    if (DetektierteStruktur!=Orkus) {
      printf("(");
      for_OperatorenCS(i,PassendeZuordnung,Zeichen,Symbol)
        printf("%s%c/%s", i ? "," : "", Zeichen, IO_FktSymbName(Symbol));
      printf(")");
    }
    printf("\nSelected strategies:\n");
    for_Strategien(i,Strategie) {
      printf("%d. %s",i+1,Strategie->Optionen);
      if (Strategie->Schrittzahl)
        printf(" for up to %d steps",Strategie->Schrittzahl);
      printf("\n");
    }
    printf("\n");
#endif
  }
}


static void StrategienLoeschen(void)
{
  unsigned int i;
  StrategieT *Strategie;
  return; /* Bug in Memory-Freigabe! fuehrt unter Linux zu Absturz. XXX */
  for_Strategien(i,Strategie) {
    our_dealloc(Strategie->Optionen);
    our_dealloc(Strategie);
  }
  our_dealloc(Strategien);
}



/*=================================================================================================================*/
/*= IX. Spezifikationsanalyse                                                                                     =*/
/*=================================================================================================================*/

static void ApparatInitialisieren(void)
{
  SV_ManagerInitialisieren(&PM_FreispeicherManager, KomprimatT, PM_AnzKomprimateJeBlock,
    "Komprimatdarstellung fuer Analysekomponente");
  Tafel1Initialisieren();
  Tafel2Vortesten();
  Tafel3Initialisieren();
  OrdnungAufbauen();
}


static void ApparatDeinitialisieren(void)
{
  unsigned int i;
  GesetzT *Gesetz;
  mapc(KomprimatLoeschen,Komprimate);
  free_list(Komprimate);
  for_Gesetze(i,Gesetz) {
    TP_LinksRechtsLoeschen(Gesetz->Gleichung_Termpaar);
    TP_GNTermpaarLoeschen(Gesetz->Gleichung_Termpaar);
  }
}


void PM_SpezifikationAnalysieren(void)
{
  GleichungenUndZielePuffern();
  ACSymboleSuchen();
  DistributivgesetzeSuchen();
  if (PA_Automodus()) {
    ZuordnungsT *Zuordnung = (ApparatInitialisieren(),
                              KomprimateErzeugen(),
                              DetektierteZuordnung());
    StrategienFokussieren(Zuordnung);
    ApparatDeinitialisieren();
  }
}


void PM_ArchivLeeren(void)
{
  mapc(GleichungLoeschenM,PM_Gleichungen);
  mapc(GleichungLoeschenM,PM_Ziele);
  free_listM(PM_Gleichungen);
  free_listM(PM_Ziele);
  if (PA_Automodus())
    StrategienLoeschen();
}
