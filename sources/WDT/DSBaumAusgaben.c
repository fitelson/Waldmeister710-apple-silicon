/*=================================================================================================================
  =                                                                                                               =
  =  DSBaumAusgaben.c                                                                                             =
  =                                                                                                               =
  =  -einfache Ausgabe von Baeumen                                                                                =
  =  -graphisch unterstuetzte Ausgabe von Baeumen                                                                 =
  =  -Dump aller Bauminformationen                                                                                =
  =                                                                                                               =
  =                                                                                                               =
  =  10.08.94  Thomas.                                                                                            =
  =================================================================================================================*/

#include "Ausgaben.h"
#include "compiler-extra.h"
#include "general.h"
#include "DSBaumAusgaben.h"
#include "DSBaumKnoten.h"
#include <string.h>
#include "SymbolOperationen.h"
#include "TermOperationen.h"

#if BA_AUSGABEN

/*=================================================================================================================*/
/*= I. Funktionen fuer schlichte Ausgabe                                                                          =*/
/*=================================================================================================================*/

static SymbolT Symbolfolge[maxDrucklaengeInnererPfad+1];

static void DruckeBlatt(KantenT Blatt, unsigned int naechsterEintrag)
{
  UTermT TermIndex;
  GNTermpaarT RegelIndex;
  unsigned int i;

  for(i=0; i<naechsterEintrag; i++) {
    IO_DruckeSymbol(Symbolfolge[i]);
    printf(" ");
  }
  printf("#");
  TermIndex = BK_Rest(Blatt);
  while (TermIndex) {
    IO_DruckeSymbol(TO_TopSymbol(TermIndex));
    printf(" ");
    TermIndex = TO_Schwanz(TermIndex);
  }
  printf("#");
  IO_DruckeTerm(BK_linkeSeite(Blatt)); printf("#");
  RegelIndex = BK_Regeln(Blatt);
  do {
    IO_DruckeTerm(RegelIndex->rechteSeite);
    printf("_");
    IO_DruckeGanzzahl(TP_ElternNr(RegelIndex));
    printf("! ");
    RegelIndex = RegelIndex->Nachf;
  } while (RegelIndex);
  IO_LF();
}


static void DruckeBaumRek(KantenT Baum, unsigned int naechsterEintrag)
{
  SymbolT SymbolIndex;

  if (BK_IstBlatt(Baum))
    DruckeBlatt(Baum,naechsterEintrag);
  else {
    SO_forSymboleAbwaerts(SymbolIndex,BK_maxSymb(Baum),BK_minSymb(Baum))
      if (BK_Nachf(Baum,SymbolIndex)) {
        Symbolfolge[naechsterEintrag] = SymbolIndex;
        DruckeBaumRek(BK_Nachf(Baum,SymbolIndex),min(naechsterEintrag+1,maxDrucklaengeInnererPfad));
      }
  }
}


void BA_DruckeBaum(DSBaumT Baum)
{
  if (Baum.Wurzel)
    DruckeBaumRek(Baum.Wurzel,0);
  else
    printf("NULL\n");
}



/*=================================================================================================================*/
/*= II. Funktionen fuer Adressassoziierung                                                                        =*/
/*=================================================================================================================*/

typedef struct AdresseintragsTS *AdresseintragsT;

struct AdresseintragsTS {
  void *Adresse;
  AdresseintragsT NaechsterAdresseintrag;
};

AdresseintragsT AAListe;                                            /* Abkuerzung fuer: Adressenassoziierungsliste */
unsigned int LaengeAAListe;

void BA_AdressassoziierungBeginnen(void)
{
  AAListe = NULL;
  LaengeAAListe = 0;
}

unsigned int BA_Assoziation(void *Adresse)                    /* realisiert injektive Abbildung void* -> {0 ... n} */
{                                                        /* mit n minimal. Dabei wird stets NULL auf 0 abgebildet. */
  AdresseintragsT AAIndex;
  unsigned int AAPosition;

  if (Adresse) {                                                     /* Wenn die Adresse von NULL verschieden ist: */
    AAIndex = AAListe;                                                    /* Adressenassoziierungsliste durchgehen */
    AAPosition = 0;

    while (AAIndex && AAIndex->Adresse!=Adresse) {
      AAIndex = AAIndex->NaechsterAdresseintrag;
      AAPosition++;
    }

    if (AAIndex)      /* Wenn ein Eintrag gefunden worden ist, dessen Position in der linearen Liste, gezaehlt vom */
      return LaengeAAListe-AAPosition;                                                /* Listenende, zurueckgeben. */
    else {         /* Ansonsten wurde nichts gefunden. Neuen Eintrag vor Listenkopf setzen und AALaenge+1 zurueck. */
      (AAIndex = our_alloc(sizeof(struct AdresseintragsTS)))->Adresse = Adresse;
      AAIndex->NaechsterAdresseintrag = AAListe;
      AAListe = AAIndex;
      return ++LaengeAAListe;
    }
  }
  else
    return 0;
}

void BA_AdressassoziierungBeenden(void)
{
  AdresseintragsT Nachf;

  while (AAListe) {
    Nachf = AAListe->NaechsterAdresseintrag;
    our_dealloc(AAListe);
    AAListe = Nachf;
  }
  LaengeAAListe = 0;
}

void BA_DruckeAssAdresse(void *Adresse)
{
  printf("$");
  IO_DruckeGanzzahl(BA_Assoziation(Adresse));
}

void BA_BaumknotenAssoziieren(DSBaumT Baum)                                   /* Top-Down-Durchlauf der Baumknoten */
{
  typedef struct SchlangenTS *SchlangenT;
  struct SchlangenTS {
    KantenT Knoten;
    SchlangenT Nachf;
  };
  SchlangenT Anfang = NULL, Ende = NULL;
  KantenT Knoten;

#define /*void*/ PushUndAss(/*KantenT*/ PushKnoten)                                                                 \
  {                                                                                                                 \
     SchlangenT Neu = (SchlangenT) our_alloc(sizeof(struct SchlangenTS));                                           \
     Neu->Nachf = NULL;                                                                                             \
     BA_Assoziation(Neu->Knoten = PushKnoten);                                                                      \
     if (Anfang)                                                                                                    \
       Ende = Ende->Nachf = Neu;                                                                                    \
     else                                                                                                           \
       Anfang = Ende = Neu;                                                                                         \
   }

#define /*void*/ GetKnoten(/*void*/)                                                                                \
  {                                                                                                                 \
    SchlangenT Alt;                                                                                                 \
                                                                                                                    \
    Knoten = Anfang->Knoten;                                                                                        \
    Anfang = (Alt = Anfang)->Nachf;                                                                                 \
    our_dealloc(Alt);                                                                                               \
  }                                                                                                                 \

#define /*BOOLEAN*/ SchlangeNichtLeer(/*void*/)                                                                     \
  (Anfang)


  if (Baum.Wurzel) {
    PushUndAss(Baum.Wurzel);

    do {
      SymbolT SymbolIndex;

      GetKnoten();

      if (!BK_IstBlatt(Knoten))
        SO_forSymboleAbwaerts(SymbolIndex,BK_maxSymb(Knoten),BK_minSymb(Knoten))
          if (BK_Nachf(Knoten,SymbolIndex))
            PushUndAss(BK_Nachf(Knoten,SymbolIndex));

    } while (SchlangeNichtLeer());
  }

#undef Push
#undef GetKnoten
#undef SchlangeNichtLeer
}


/*=================================================================================================================*/
/*= III. Funktionen fuer Dump der Baumstruktur                                                                    =*/
/*=================================================================================================================*/

static void DumpSprungeintrag(SprungeintragsT Sprungeintrag)
{
  printf("{SEintrag ");
  BA_DruckeAssAdresse(Sprungeintrag);
  printf(":  Teilterm=");
  BA_DruckeAssAdresse(Sprungeintrag->Teilterm);
  printf(";");
  IO_DruckeTerm(Sprungeintrag->Teilterm);
  printf("  PositionNachTeilterm=");
  IO_DruckeGanzzahl(Sprungeintrag->PositionNachTeilterm);
  printf("  Zielknoten=");
  BA_DruckeAssAdresse(Sprungeintrag->Zielknoten);
  printf("  Startknoten=");
  BA_DruckeAssAdresse(Sprungeintrag->Startknoten);
  IO_LF();
  printf("                 NaechsterZieleintrag=");
  BA_DruckeAssAdresse(Sprungeintrag->NaechsterZieleintrag);
  printf("  VorigerZieleintrag=");
  BA_DruckeAssAdresse(Sprungeintrag->VorigerZieleintrag);
  printf("  NaechsterStarteintrag=");
  BA_DruckeAssAdresse(Sprungeintrag->NaechsterStarteintrag);
  printf("  Praefixeintrag=");
  BA_DruckeAssAdresse(Sprungeintrag->Praefixeintrag);
  printf("}");
}


static void DumpPraefixliste(PraefixeintragsT Praefixliste)
{
  printf("[Praefixliste:");
  while (Praefixliste) {
    printf(" ");
    BA_DruckeAssAdresse(Praefixliste);
    printf(":");
    BA_DruckeAssAdresse(Praefixliste->Sprungeintrag);
    Praefixliste = Praefixliste->NaechsterPraefixeintrag;
  }
  printf("]");
}


static void DumpRegeln(GNTermpaarT Regelliste)
{
  printf("[rechte Seiten:");
  while (Regelliste) {
    printf(" ");
    IO_DruckeTerm(Regelliste->rechteSeite);
    Regelliste = Regelliste->Nachf;
  }
  printf("]");
}


static void DumpSprungausgaenge(SprungeintragsT Sprungliste)
{
  SprungeintragsT Vorg = NULL;

  printf("Sprungausgaenge:");
  while (Sprungliste) {
    IO_LF(); printf("  ");
    if (Vorg!=Sprungliste->VorigerZieleintrag)
      printf("Doppelverkettung falsch!");
    DumpSprungeintrag(Sprungliste);
    Sprungliste = (Vorg = Sprungliste)->NaechsterZieleintrag;
  }
}


static void DumpSprungeingaenge(SprungeintragsT Sprungliste)
{
  printf("Sprungeingaenge:");
  while (Sprungliste) {
    IO_LF(); printf("  ");
    DumpSprungeintrag(Sprungliste);
    Sprungliste = Sprungliste->NaechsterStarteintrag;
  }
}


static void DumpKnoten(KantenT Knoten);


static void DumpInnerenKnoten(KantenT Knoten)
{
  SymbolT Symbol;
  
  printf("*Innerer Knoten ");
  BA_DruckeAssAdresse(Knoten); IO_LF();
  printf("Vorg="); BA_DruckeAssAdresse(BK_Vorg(Knoten));
  printf("  VorgSymbol="); IO_DruckeSymbol(BK_VorgSymb(Knoten));
  printf("  MaxSymb="); IO_DruckeSymbol(BK_maxSymb(Knoten));
  printf("  MinSymb="); IO_DruckeSymbol(BK_minSymb(Knoten)); IO_LF();
  DumpSprungeingaenge(BK_Sprungeingaenge(Knoten)); IO_LF();
  DumpSprungausgaenge(BK_Sprungausgaenge(Knoten)); IO_LF();

  /* jetzt Nachfolger */
  SO_forSymboleAbwaerts(Symbol,SO_GroesstesFktSymb,SO_NummerVar(BK_AnzVariablen(Knoten)))
    if (BK_Nachf(Knoten,Symbol)) {
      IO_LF();
      printf("Nachfolger zum inneren Knoten ");
      BA_DruckeAssAdresse(Knoten); IO_LF();
      DumpKnoten(BK_Nachf(Knoten,Symbol));
    }
}


static void DumpBlattknoten(KantenT Knoten)
{
  UTermT TermIndex;
  unsigned int Suffixlaenge = 0;
  
  printf("+Blattknoten ");
  BA_DruckeAssAdresse(Knoten); IO_LF();
  printf("Vorg="); BA_DruckeAssAdresse(BK_Vorg(Knoten));
  printf("  VorgSymbol="); IO_DruckeSymbol(BK_VorgSymb(Knoten));
  printf("  linkeSeite="); IO_DruckeTerm(BK_linkeSeite(Knoten));
  printf("  Suffix=");
  TermIndex = BK_Rest(Knoten);
  while (TermIndex) {
    Suffixlaenge++;
    IO_DruckeSymbol(TO_TopSymbol(TermIndex));
    TermIndex = TO_Schwanz(TermIndex);
  }
  printf("  SuffixPosition="); IO_DruckeGanzzahl(BK_SuffixPosition(Knoten)); IO_LF();
  if (Suffixlaenge+BK_SuffixPosition(Knoten)!=TO_Termlaenge(BK_linkeSeite(Knoten))+1) {
    unsigned int Weitermachen = 0;
    printf("Fehler in der Konstellation von linker Seite, Suffix und Suffixposition!\n");
    printf("Weitermachen? (!=0) ");
    scanf("%d",&Weitermachen);
    if (!Weitermachen)
     exit(1);
  }
  DumpRegeln(BK_Regeln(Knoten)); IO_LF();
  DumpPraefixliste(BK_Praefixliste(Knoten)); IO_LF();
  DumpSprungeingaenge(BK_Sprungeingaenge(Knoten)); IO_LF();
}


static void DumpKnoten(KantenT Knoten)
{
  if (BK_IstBlatt(Knoten))
    DumpBlattknoten(Knoten);
  else
    DumpInnerenKnoten(Knoten);
}


void BA_DumpBaum(DSBaumT Baum, BOOLEAN MitGraphik)
{
  if (MitGraphik)
    BA_BaumMalenDefaultMitG(Baum);
  else
    BA_BaumMalenDefaultOhneG(Baum);
  if (Baum.Wurzel) {
    IO_LF(); IO_LF();
    BA_AdressassoziierungBeginnen();
    BA_BaumknotenAssoziieren(Baum);
    DumpKnoten(Baum.Wurzel);
    BA_AdressassoziierungBeenden();
  }
}



/*=================================================================================================================*/
/*= IV. Grundlagen fuer graphisch unterstuetzte Ausgabe des Baumes                                                =*/
/*=================================================================================================================*/

/* Verwendung von graphischen Symbolen des Zeichensatzes auf Non-Standard-ASCII-Positionen. Diese werden nicht 
   immer angezeigt. Daher Angabe von Ersatzzeichen nach folgendem Schema:
					   grafisch      Ctrl Q-    Ersatz
			     
			     Waagerecht:                  R          - 
									       
			     Senkrecht:                   Y          |         
									       
			     TNormal:                     X          +
			     
			     TAufKopf:                    W          -
			     
			     TLinks:                      V          |
			     
			     TRechts:                     U          |
									       
			     Kreuz:                       O          +         
									       
			     LinksUnten:                  L          +   
									       
			     RechtsUnten:                 M          +
			     
			     LinksOben:                   K          +  
									       
			     RechtsOben:                  N          +                                           */

typedef enum {Waagerecht, Senkrecht, TNormal, TAufKopf, TLinks, TRechts,
  Kreuz, LinksUnten, RechtsUnten, LinksOben, RechtsOben} GraphikzeichenIndexT;

typedef enum{Normal, Ersatz} GraphikmodusT;
GraphikmodusT Graphikmodus;

static const char Graphikzeichen[RechtsOben+1][Ersatz+1] = {
  {'\x12', '-'},  /* Waagerecht  */
  {'\x19', '|'},  /* Senkrecht   */
  {'\x18', '+'},  /* TNormal     */
  {'\x17', '+'},  /* TAufKopf    */
  {'\x16', '|'},  /* TLinks      */
  {'\x15', '|'},  /* TRechts     */
  {'\x0f', '+'},  /* Kreuz       */
  {'\x0c', '+'},  /* LinksUnten  */
  {'\x0d', '+'},  /* RechtsUnten */
  {'\x0b', '+'},  /* LinksOben   */
  {'\x0e', '+'}   /* RechtsOben  */
};

unsigned int AbstandGeschwisterknoten;                                     /* hier zwei wichtige Eingangsgroessen, */
BOOLEAN MitAdressen;                                  /* mit denen die Durchreichung von Parametern vermieden wird */



/*=================================================================================================================*/
/*= V. Linke und rechte Breiten im Baum                                                                           =*/
/*=================================================================================================================*/

/* Zugriff auf Information ueber linke und rechte Breite im Knoten */

static void LinkeRechteBreiteSei(KantenT Knoten, unsigned int BreiteLinks, unsigned int BreiteRechts)
{
  BK_LinkeBreiteSei(Knoten,BreiteLinks);
  BK_RechteBreiteSei(Knoten,BreiteRechts);
}

static unsigned int LinkeBreite(KantenT Knoten)
{
  return BK_LinkeBreite(Knoten);
}

static unsigned int RechteBreite(KantenT Knoten)
{
  return BK_RechteBreite(Knoten);
}

static unsigned int Gesamtbreite(KantenT Knoten)
{
  return LinkeBreite(Knoten)+RechteBreite(Knoten)+1;
}


/* Nachfolgend allgemeine Form der Aufteilung einer Textbreite auf linken und rechten Anteil. Geht aus von Breite
   minus 1, da Zeichen in der Mitte ("Ast") weder nach links noch nach rechts gezaehlt wird. Der groessere Teil
   soll auf die Seite gesetzt werden, auf welcher die Beschriftung liegt. */

static unsigned LinkerAnteil(unsigned int Breite, BOOLEAN BeschriftungLinks)
{
  return BeschriftungLinks ? (Breite-1)-(Breite-1)/2 : (Breite-1)/2;           /* ceiling bzw. floor((Breite-1)/2) */
}

static unsigned RechterAnteil(unsigned int Breite, BOOLEAN BeschriftungLinks)
{
  return BeschriftungLinks ? (Breite-1)/2 : (Breite-1)-(Breite-1)/2;           /* floor bzw. ceiling((Breite-1)/2) */
}

static unsigned int Adresslaenge(KantenT Knoten)
{
  return LaengeNatZahl(BA_Assoziation(Knoten));
}



/*=================================================================================================================*/
/*= VI. Berechnung der Knotenbreiten                                                                              =*/
/*=================================================================================================================*/

static void LRBreiteBerechnen(KantenT Knoten, BOOLEAN BeschriftungLinks);

static void LRBreiteBerechnenBlatt(KantenT Knoten, BOOLEAN BeschriftungLinks)
{
/* Funktions- und Variablensymbole sowie darueber ein * bzw. eine assoziierte Adresse werden links- oder rechts-
   zentriert untereinandergeschrieben. */
  unsigned int Breite;
  UTermT TermIndex;
  GNTermpaarT Regel;

  Breite = 1; /* Wenn Suffix leer, dannn mindestens Platz fuer * noetig */
  for (TermIndex = BK_Rest(Knoten); TermIndex; TermIndex = TO_Schwanz(TermIndex))    /* Funktionssymbole im Suffix */
    Maximieren(&Breite,IO_SymbAusgabelaenge(TO_TopSymbol(TermIndex)));
  if (MitAdressen)                                                               /* ggf. assoziierte Knotenadresse */
    Maximieren(&Breite,Adresslaenge(Knoten));
  for (Regel = BK_Regeln(Knoten); Regel; Regel = Regel->Nachf)
    Maximieren(&Breite,1+LaengeNatZahl(TP_ElternNr(Regel)));                 /* Aufzaehlung der anhaengigen Regeln */

  /* Aufteilen auf links und rechts und in Knoten schreiben */
  LinkeRechteBreiteSei(Knoten,LinkerAnteil(Breite,BeschriftungLinks),RechterAnteil(Breite,BeschriftungLinks));
}


static void LRBreiteBerechnenInnen1(KantenT Knoten, BOOLEAN BeschriftungLinks)
{                                                                /* fuer inneren Knoten mit genau einem Nachfolger */
  SymbolT NachfSymbol = BK_minSymb(Knoten);
  KantenT Nachf = BK_Nachf(Knoten,NachfSymbol);
  unsigned int links, rechts;


  /* linke und rechte Breite des Nachfolgers ermitteln */
  LRBreiteBerechnen(Nachf,BeschriftungLinks);
  links = LinkeBreite(Nachf);
  rechts = RechteBreite(Nachf);

  /* anpassen mit Laenge der Beschriftung der Kante zum Nachfolger */
  Maximieren(BeschriftungLinks ? &links : &rechts,IO_SymbAusgabelaenge(NachfSymbol));

  /* ggf. anpassen mit Adressbeschriftung */
  if (MitAdressen) {
    Maximieren(&links,LinkerAnteil(Adresslaenge(Knoten),BeschriftungLinks));
    Maximieren(&rechts,RechterAnteil(Adresslaenge(Knoten),BeschriftungLinks));
  }

  /* in Knoten schreiben */
  LinkeRechteBreiteSei(Knoten,links,rechts);
}


static void LRBreiteBerechnenInnenN(KantenT Knoten, BOOLEAN BeschriftungLinks)
{                                                             /* fuer inneren Knoten mit mehr als einem Nachfolger */
  SymbolT SymbolIndex;
  unsigned int i, Breite = 0, links, rechts;
  KantenT Nachf;

  /* zunaechst Beschriftungsrichtungen der Nachfolgerknoten festlegen. Dabei haelftig auf der linken bzw. auf der
     rechten Seite  beschriften. Fuer ungerade Anzahl der Nachfolger den ueberzaehligen auf der Seite der von oben
     eingegangenen Beschriftungsrichtung beschriften. */
  unsigned int AnzNachfLinks = LinkerAnteil(BK_AnzNachfolger(Knoten)+1,BeschriftungLinks);
  unsigned int AnzNachfRechts = RechterAnteil(BK_AnzNachfolger(Knoten)+1,BeschriftungLinks);
     /* jeweils +1, da Linker/RechterAnteil Zeichenfolgen aufteilen auf links, ein Zeichen in die Mitte und rechts */

  /* Summe der Breiten der Nachfolger mit Beschriftung links */
  for (i = AnzNachfLinks,SymbolIndex = BK_maxSymb(Knoten); i; SymbolIndex = SO_NaechstesVarSymb(SymbolIndex), i--) {
    while (!(Nachf = BK_Nachf(Knoten,SymbolIndex)))
      SymbolIndex = SO_NaechstesVarSymb(SymbolIndex);
    LRBreiteBerechnen(Nachf,TRUE);
    MaximumAddieren(&Breite,LinkeBreite(Nachf),IO_SymbAusgabelaenge(SymbolIndex));       /* Beschriftung der Kante */
    Breite += RechteBreite(Nachf)+1;                                                /* +1 wegen ausgesparter Mitte */
  }

  /* Summe der Breiten der Nachfolger mit Beschriftung rechts */
  for (i = AnzNachfRechts; i; SymbolIndex = SO_NaechstesVarSymb(SymbolIndex), i--) {
    while (!(Nachf = BK_Nachf(Knoten,SymbolIndex)))
      SymbolIndex = SO_NaechstesVarSymb(SymbolIndex);
    LRBreiteBerechnen(Nachf,FALSE);
    MaximumAddieren(&Breite,RechteBreite(Nachf),IO_SymbAusgabelaenge(SymbolIndex));          /* Kantenbeschriftung */
    Breite += LinkeBreite(Nachf)+1;                                                 /* +1 wegen ausgesparter Mitte */
  }

  /* Breite erhoehen um vorgegebenen Abstand von Geschwisterknoten und aufteilen auf links und rechts */
  Breite += (AnzNachfLinks+AnzNachfRechts-1)*AbstandGeschwisterknoten;
  links = LinkerAnteil(Breite,BeschriftungLinks);
  rechts = RechterAnteil(Breite,BeschriftungLinks);

  /* ggf. anpassen mit Adressbeschriftung */
  if (MitAdressen) {
    Maximieren(&links,LinkerAnteil(Adresslaenge(Knoten),BeschriftungLinks));
    Maximieren(&rechts,RechterAnteil(Adresslaenge(Knoten),BeschriftungLinks));
  }

  /* in Knoten schreiben */
  LinkeRechteBreiteSei(Knoten,links,rechts);
}


static void LRBreiteBerechnen(KantenT Knoten, BOOLEAN BeschriftungLinks)
{
  if (BK_IstBlatt(Knoten))
    LRBreiteBerechnenBlatt(Knoten,BeschriftungLinks);
  else if (BK_genauEinNachf(Knoten))
    LRBreiteBerechnenInnen1(Knoten,BeschriftungLinks);
  else
    LRBreiteBerechnenInnenN(Knoten,BeschriftungLinks);
}



/*=================================================================================================================*/
/*= VII. Funktionen fuer Realisierung des Koordinatensystems                                                      =*/
/*=================================================================================================================*/

/* Orientierung des Koordinatensystems:     +--------> x 
					    |
					    |
					    |
					    |
					    |
					 y  V                                                                      */


unsigned int maxOrdinate;                                    /* maximaler Koordinatenwert in der aktuellen Graphik */
unsigned int maxAbszisseTafel, maxOrdinateTafel;    /* maximale Koordinatenwerte, fuer die Speicher angelegt wurde */


typedef struct InverseintragsTS *InverseintragsT;
struct InverseintragsTS {
  unsigned int Spalte;
  unsigned int Laenge;
  InverseintragsT Nachf;
};
typedef struct {          /* Zeile = Verbund aus Feld von Zeichen und Eintraegen ueber invers zu setzende Bereiche */
  char *Zeichen;
  InverseintragsT Inverse;
} ZeilenT;
typedef ZeilenT *TafelT;                                                    /* Tafel = Feld von Zeigern auf Zeilen */
TafelT Tafel;                                                                       /* nimmt Koordinatensystem auf */
char * Leerzeile;                                  /* vorraetig halten als Kopiervorlage fuer spaetere neue Zeilen */


static void TafelAnlegen(unsigned int Spaltenzahl)
/* Ergebnis: Es wird Platz geschaffen genau fuer alle Eintraege (*Tafel)[x][y] mit 0<=y<BA_AnzErsterZeilen und 
   0<=x<Spaltenzahl sowie mit x=Spaltenzahl. Die erstgenannten Eintrage werden mit ' ' initialisiert, die letztge-
   nannten mit '\0'. Dadurch lassen sich die einzelnen Zeilen als Zeichenketten ausgeben.                          */
{
  unsigned int i;

  Tafel = (TafelT) our_alloc(BA_AnzErsterZeilen*sizeof(ZeilenT));
  maxAbszisseTafel = Spaltenzahl-1;

  Leerzeile = (char *) our_alloc((Spaltenzahl+1)*sizeof(char));                        /* Leerzeile initialisieren */
  for (i=0; i<Spaltenzahl; i++)
    Leerzeile[i] = ' ';
  Leerzeile[Spaltenzahl] = '\0';

  for (i=0; i<BA_AnzErsterZeilen; i++) {                         /* die Tafelzeilen als deren Kopie initialisieren */
    Tafel[i].Zeichen = strcpy((char *) our_alloc((Spaltenzahl+1)*sizeof(char)),Leerzeile);
    Tafel[i].Inverse = NULL;
  }

  maxOrdinate = 0;                            /* maximalen Koordinatenwert in der aktuellen Graphik initialisieren */
  maxOrdinateTafel = BA_AnzErsterZeilen-1;       /* dito maximalen Ordinatenwert, fuer den Speicher angelegt wurde */
}

static void TafelFreigeben(void)
{
  unsigned int i;

  for (i=0; i<maxOrdinateTafel; i++) {
    InverseintragsT InversIndex = Tafel[i].Inverse, InversIndexAlt;
    our_dealloc(Tafel[i].Zeichen);
    while (InversIndex) {
      InversIndex = (InversIndexAlt = InversIndex)->Nachf;
      our_dealloc(InversIndexAlt);
    }
  }
  our_dealloc(Tafel);
}

static void TafelAusgeben(void)
{
  unsigned int i;

  for (i=0; i<=maxOrdinate; i++) {           /* bis maxOrdinate, damit keine unnoetigen Leerzeilen gedruckt werden */
    if ((Tafel[i].Inverse) && (Graphikmodus==Normal)) {
      InverseintragsT InversIndex = Tafel[i].Inverse;
      char *ZeichenIndex = Tafel[i].Zeichen;
      unsigned int SpalteAlt = 0;
      #define /*void*/ DruckeBisVor(/*unsigned int*/ Laenge)                                                        \
      {                                                                                                             \
        char Hilf;                                                                                                  \
        Hilf = ZeichenIndex[Laenge];                                                                                \
        ZeichenIndex[Laenge] = '\000';                                                                              \
        printf(ZeichenIndex);                                                                                    \
        ZeichenIndex[Laenge] = Hilf;                                                                                \
        ZeichenIndex += Laenge;                                                                                     \
      }

      do {
        DruckeBisVor(InversIndex->Spalte-SpalteAlt);
        printf(BA_SequenzInversAn);
        DruckeBisVor(InversIndex->Laenge);
        printf(BA_SequenzInversAus);
        SpalteAlt = InversIndex->Spalte+InversIndex->Laenge;
        InversIndex = InversIndex->Nachf;
      } while (InversIndex);

      printf(ZeichenIndex);
      #undef DruckeBisVor
    }
    else
      printf(Tafel[i].Zeichen);
    IO_LF();
  }
}

static void GgfTafelAusdehnen(unsigned int Ordinate)
{
  if (Ordinate>maxOrdinateTafel) {                                    /* gegebenenfalls Tafel nach unten ausdehnen */
    unsigned int i;
    maxOrdinate = Ordinate;
    Ordinate = BA_AnzErsterZeilen*((Ordinate+1)/BA_AnzErsterZeilen+1)-1;   /* ab hier als neuer Wert fuer maxOrdi- */
    Tafel = (TafelT) our_realloc(Tafel,(Ordinate+1)*sizeof(ZeilenT),"Erweiterung der Ausgabetafel");  /* nateTafel */
    for (i=maxOrdinateTafel+1; i<=Ordinate; i++) {
      Tafel[i].Zeichen = strcpy((char *) our_alloc((maxAbszisseTafel+1)*sizeof(char)),Leerzeile);
      Tafel[i].Inverse = NULL;
    }
    maxOrdinateTafel = Ordinate;
  }
  else
    Maximieren(&maxOrdinate,Ordinate);
}

static void ZeichenSchreiben(char Zeichen, unsigned int Abszisse, unsigned int Ordinate)
{
  GgfTafelAusdehnen(Ordinate);                                        /* gegebenenfalls Tafel nach unten ausdehnen */
  Tafel[Ordinate].Zeichen[Abszisse] = Zeichen;                                          /* Zeichen in Tafel setzen */
}

static void ZeichenketteSchreiben(char *Zeichenkette, unsigned int Abszisse, unsigned int Ordinate)
{                                /* Zeichenkette horizontal ab Position (Abszisse|Ordinate) in die Tafel schreiben */
  unsigned int i=0;

  GgfTafelAusdehnen(Ordinate);                                        /* gegebenenfalls Tafel nach unten ausdehnen */
  while (Zeichenkette[i]) {                                                        /* Zeichenkette in Tafel setzen */
     Tafel[Ordinate].Zeichen[Abszisse+i] = Zeichenkette[i];
     i++;
  }
  
}

static void ZeichenketteInversSchreiben(char *Zeichenkette, unsigned int Abszisse, unsigned int Ordinate)
{                     /* Zeichenkette invertiert horizontal ab Position (Abszisse|Ordinate) in die Tafel schreiben */
                           /* Vorausgesetzt wird, dass die Invers-Kontrollsequenzen nicht geschachtelt auftauchen. */
  InverseintragsT Neu = (InverseintragsT) our_alloc(sizeof(struct InverseintragsTS));
  InverseintragsT Vorg = NULL, Index = Tafel[Ordinate].Inverse;

  ZeichenketteSchreiben(Zeichenkette,Abszisse,Ordinate);
  while (Index && Index->Spalte<Abszisse)
    Index = (Vorg = Index)->Nachf;
  Neu->Spalte = Abszisse;
  Neu->Laenge = strlen(Zeichenkette);
  if (Vorg)
    Vorg->Nachf = Neu;
  else
    Tafel[Ordinate].Inverse = Neu;
  Neu->Nachf = Index;
}



/*=================================================================================================================*/
/*= VIII. Funktionen zum Einpassen von graphischen Primitiven (Symbolnamen etc.)                                  =*/
/*=================================================================================================================*/

static void SymbolBuendigMalen(SymbolT Symbol, BOOLEAN BeschriftungLinks,  /* Symbol in Zeile y horizontal schrei- */
  unsigned int Abszisse, unsigned int Ordinate)     /* ben fuer BeschriftungLinks links von Spalte x, sonst rechts */
{
  if (SO_SymbIstVar(Symbol)) {
    char Symbolname[BA_maxAusgabelaengeVariable];
    sprintf(Symbolname,"x%d",SO_VarNummer(Symbol));
    ZeichenketteSchreiben(Symbolname,BeschriftungLinks ? Abszisse-strlen(Symbolname) : Abszisse+1,Ordinate);
  }
  else
    ZeichenketteSchreiben(IO_FktSymbName(Symbol),
      BeschriftungLinks ? Abszisse-IO_SymbAusgabelaenge(Symbol) : Abszisse+1,Ordinate);
}

static void SymbolZentriertMalen(SymbolT Symbol, BOOLEAN BeschriftungLinks,/* Symbol horizontal schreiben in Zeile */
  unsigned int Abszisse, unsigned int Ordinate)     /* y zentriert um Spalte x. Bei gerader Anzahl von Zeichen das */
{                                          /* ueberzaehlige Zeichen auf die Seite der Beschriftungsrichtung setzen */
  if (SO_SymbIstVar(Symbol)) {
    char Symbolname[BA_maxAusgabelaengeVariable];
    sprintf(Symbolname,"x%d",SO_VarNummer(Symbol));
    ZeichenketteSchreiben(Symbolname,Abszisse-LinkerAnteil(strlen(Symbolname),BeschriftungLinks),Ordinate);
  }
  else
    ZeichenketteSchreiben(IO_FktSymbName(Symbol),
      Abszisse-LinkerAnteil(IO_SymbAusgabelaenge(Symbol),BeschriftungLinks),Ordinate);
}

static void AdresseZentriertMalen(void *Adresse, BOOLEAN BeschriftungLinks,/* Adresse horizontal schreiben in Zei- */
  unsigned int Abszisse, unsigned int Ordinate)  /* le y zentriert um Spalte x. Bei gerader Anzahl von Zeichen das */
{                                          /* ueberzaehlige Zeichen auf die Seite der Beschriftungsrichtung setzen */
  if (MitAdressen) {
    char Adressname[BA_maxAusgabelaengeAdressenassoziation];

    sprintf(Adressname,"%d",BA_Assoziation(Adresse));
    if (Graphikmodus==Normal)
      ZeichenketteInversSchreiben(Adressname,Abszisse-LinkerAnteil(strlen(Adressname),BeschriftungLinks),Ordinate);
    else
      ZeichenketteSchreiben(Adressname,Abszisse-LinkerAnteil(strlen(Adressname),BeschriftungLinks),Ordinate);
  }
  else
    ZeichenSchreiben('*',Abszisse,Ordinate);
}

static void RegelnrZentriertMalen(GNTermpaarT Regel, BOOLEAN BeschriftungLinks, /* Regelnr horizontal schreiben in */
  unsigned int Abszisse, unsigned int Ordinate)   /* Zeile y zentriert um Spalte x. Bei gerader Anzahl von Zeichen */
{                                      /* das ueberzaehlige Zeichen auf die Seite der Beschriftungsrichtung setzen */
  char Regelnr[BA_maxAusgabelaengeRegelnr];

  sprintf(Regelnr,"%s%ld",TP_Gegenrichtung(Regel) ? "G" : "R",TP_ElternNr(Regel));
  ZeichenketteSchreiben(Regelnr,Abszisse-LinkerAnteil(strlen(Regelnr),BeschriftungLinks),Ordinate);
}

static void KanteMalen(SymbolT Kantensymbol, GraphikzeichenIndexT IntroZeichen,
  BOOLEAN BeschriftungLinks, unsigned int Abszisse, unsigned int Ordinate)
{
  ZeichenSchreiben(Graphikzeichen[IntroZeichen][Graphikmodus],Abszisse,Ordinate++);
  ZeichenSchreiben(Graphikzeichen[Senkrecht][Graphikmodus],Abszisse,Ordinate);
  SymbolBuendigMalen(Kantensymbol,BeschriftungLinks,Abszisse,Ordinate++);
  ZeichenSchreiben(Graphikzeichen[Senkrecht][Graphikmodus],Abszisse,Ordinate);
}



/*=================================================================================================================*/
/*= IX. Funktionen zum Malen verschiedener Knotentypen                                                            =*/
/*=================================================================================================================*/

static void BlattMalen(KantenT Knoten, BOOLEAN BeschriftungLinks, unsigned int Abszisse, unsigned int Ordinate)
{
  UTermT TermIndex;
  GNTermpaarT RegelIndex;

  AdresseZentriertMalen(Knoten,BeschriftungLinks,Abszisse,Ordinate++);

  for(TermIndex = BK_Rest(Knoten); TermIndex; TermIndex = TO_Schwanz(TermIndex))
    SymbolZentriertMalen(TO_TopSymbol(TermIndex),BeschriftungLinks,Abszisse,Ordinate++);

  ZeichenSchreiben('#',Abszisse,Ordinate++);

  for (RegelIndex = BK_Regeln(Knoten); RegelIndex; RegelIndex = RegelIndex->Nachf)
    RegelnrZentriertMalen(RegelIndex,BeschriftungLinks,Abszisse,Ordinate++);
}


static void KnotenMalen(KantenT Knoten, BOOLEAN BeschriftungLinks, unsigned int Abszisse, unsigned int Ordinate);

static void Innen1Malen(KantenT Knoten, BOOLEAN BeschriftungLinks, unsigned int Abszisse, unsigned int Ordinate)
{
  AdresseZentriertMalen(Knoten,BeschriftungLinks,Abszisse,Ordinate++);
  KanteMalen(BK_minSymb(Knoten),Senkrecht,BeschriftungLinks,Abszisse,Ordinate);
  KnotenMalen(BK_Nachf(Knoten,BK_minSymb(Knoten)),BeschriftungLinks,Abszisse,Ordinate+3);
}


static void InnenNMalen(KantenT Knoten, BOOLEAN BeschriftungLinks, unsigned int Abszisse, unsigned int Ordinate)
{
  SymbolT SymbolIndex;
  unsigned int i, j, Breite = 0, AbszisseMittig;
  KantenT Nachf;
  unsigned int AnzNachfLinks = LinkerAnteil(BK_AnzNachfolger(Knoten)+1,BeschriftungLinks);
  unsigned int AnzNachfRechts = RechterAnteil(BK_AnzNachfolger(Knoten)+1,BeschriftungLinks);
  BOOLEAN NachfInMitte = FALSE;

  /* Breite links und Breite rechts neu berechnen, da Werte im Knoten mit Adresse maximiert */

  /* Summe der Breiten der Nachfolger mit Beschriftung links */
  for (i = AnzNachfLinks,SymbolIndex = BK_maxSymb(Knoten); i; SymbolIndex = SO_NaechstesVarSymb(SymbolIndex), i--) {
    while (!(Nachf = BK_Nachf(Knoten,SymbolIndex)))
      SymbolIndex = SO_NaechstesVarSymb(SymbolIndex);
    MaximumAddieren(&Breite,LinkeBreite(Nachf),IO_SymbAusgabelaenge(SymbolIndex));       /* Beschriftung der Kante */
    Breite += RechteBreite(Nachf)+1;                                                /* +1 wegen ausgesparter Mitte */
  }

  /* Summe der Breiten der Nachfolger mit Beschriftung rechts */
  for (i = AnzNachfRechts; i; SymbolIndex = SO_NaechstesVarSymb(SymbolIndex), i--) {
    while (!(Nachf = BK_Nachf(Knoten,SymbolIndex)))
      SymbolIndex = SO_NaechstesVarSymb(SymbolIndex);
    MaximumAddieren(&Breite,RechteBreite(Nachf),IO_SymbAusgabelaenge(SymbolIndex));          /* Kantenbeschriftung */
    Breite += LinkeBreite(Nachf)+1;                                                 /* +1 wegen ausgesparter Mitte */
  }

  /* Breite erhoehen um vorgegebenen Abstand von Geschwisterknoten */
  Breite += (AnzNachfLinks+AnzNachfRechts-1)*AbstandGeschwisterknoten;

  /* Knotenadresse */
  AdresseZentriertMalen(Knoten,BeschriftungLinks,Abszisse,Ordinate++);

  /* Bisherige Abszisse merken. Nachfolger links aussen. */
  AbszisseMittig = Abszisse;
  Abszisse -= LinkerAnteil(Breite,BeschriftungLinks);     /* auf von links gesehen erste beschriebene Spalte gehen */
  MaximumAddieren(&Abszisse,                           /* nach rechts gehen bis zur Mitte des folgenden Teilbaumes */
    LinkeBreite(Nachf = BK_Nachf(Knoten,BK_maxSymb(Knoten))),IO_SymbAusgabelaenge(BK_maxSymb(Knoten)));
  KanteMalen(BK_maxSymb(Knoten),RechtsUnten,TRUE,Abszisse,Ordinate);
  KnotenMalen(Nachf,TRUE,Abszisse,Ordinate+3);
  j = RechteBreite(Nachf)+AbstandGeschwisterknoten;

  /* Restliche Nachfolger mit Beschriftung links */
  for (i = AnzNachfLinks-1,SymbolIndex = SO_NaechstesVarSymb(BK_maxSymb(Knoten)); i;
    SymbolIndex = SO_NaechstesVarSymb(SymbolIndex), i--) {
    while (!(BK_Nachf(Knoten,SymbolIndex)))                                         /* naechsten Nachfolger finden */
      SymbolIndex = SO_NaechstesVarSymb(SymbolIndex);
    MaximumAddieren(&j,LinkeBreite(Nachf = BK_Nachf(Knoten,SymbolIndex)),IO_SymbAusgabelaenge(SymbolIndex));    
    if ((Abszisse += j+1)==AbszisseMittig)
      NachfInMitte = TRUE;                                                          /* naechste Abszisse berechnen */
    while (j)                                                               /* horizontalen Strich dorthin fuehren */
      ZeichenSchreiben(Graphikzeichen[Waagerecht][Graphikmodus],Abszisse-j--,Ordinate);
    KanteMalen(SymbolIndex,TNormal,TRUE,Abszisse,Ordinate);
    KnotenMalen(Nachf,TRUE,Abszisse,Ordinate+3);                                     /* nachfolgenden Knoten malen */
    j = RechteBreite(Nachf)+AbstandGeschwisterknoten;    /* den von diesem Knoten herruehrenden Teil des Abstandes */
  }                                                                /* zur naechsten Abszisse im Vorgriff berechnen */


  /* Nachfolger mit Beschriftung rechts ausser dem auessersten */
  for (i = AnzNachfRechts-1; i; SymbolIndex = SO_NaechstesVarSymb(SymbolIndex), i--) {
    while (!(BK_Nachf(Knoten,SymbolIndex)))                                         /* naechsten Nachfolger finden */
      SymbolIndex = SO_NaechstesVarSymb(SymbolIndex);
    if ((Abszisse += (j += LinkeBreite(Nachf = BK_Nachf(Knoten,SymbolIndex)))+1)==AbszisseMittig)
      NachfInMitte = TRUE;                                                          /* naechste Abszisse berechnen */
    while (j)                                                               /* horizontalen Strich dorthin fuehren */
      ZeichenSchreiben(Graphikzeichen[Waagerecht][Graphikmodus],Abszisse-j--,Ordinate);
    KanteMalen(SymbolIndex,TNormal,FALSE,Abszisse,Ordinate);
    KnotenMalen(Nachf,FALSE,Abszisse,Ordinate+3);                                    /* nachfolgenden Knoten malen */
    j = AbstandGeschwisterknoten; /* den von diesem Knoten herruehrenden Teil des Abstandes zur naechsten Abszisse */
    MaximumAddieren(&j,RechteBreite(Nachf),IO_SymbAusgabelaenge(SymbolIndex));           /* im Vorgriff berechnen */
  }

  /* Nachfolger rechts aussen */
  Abszisse +=                                                                       /* naechste Abszisse berechnen */
    (j += LinkeBreite(Nachf = BK_Nachf(Knoten,BK_minSymb(Knoten))))+1;
  while (j)                                                                 /* horizontalen Strich dorthin fuehren */
    ZeichenSchreiben(Graphikzeichen[Waagerecht][Graphikmodus],Abszisse-j--,Ordinate);
  KanteMalen(BK_minSymb(Knoten),LinksUnten,FALSE,Abszisse,Ordinate);
  KnotenMalen(Nachf,FALSE,Abszisse,Ordinate+3);                                      /* nachfolgenden Knoten malen */
  
  /* Symbol zwischen Knotenadresse und horizontalem Strich */
  ZeichenSchreiben(Graphikzeichen[NachfInMitte ? Kreuz : TAufKopf][Graphikmodus],AbszisseMittig,Ordinate);
}


static void KnotenMalen(KantenT Knoten, BOOLEAN BeschriftungLinks, unsigned int Abszisse, unsigned int Ordinate)
{
  if (BK_IstBlatt(Knoten))
    BlattMalen(Knoten,BeschriftungLinks,Abszisse,Ordinate);
  else if (BK_genauEinNachf(Knoten))
    Innen1Malen(Knoten,BeschriftungLinks,Abszisse,Ordinate);
  else
    InnenNMalen(Knoten,BeschriftungLinks,Abszisse,Ordinate);
}



/*=================================================================================================================*/
/*= X. Graphisch unterstuetzte Baumausgabe                                                                        =*/
/*=================================================================================================================*/

void BA_BaumMalen(DSBaumT Baum, BOOLEAN PMitAdressen, unsigned int PAbstandGeschwisterknoten,
 unsigned int Spaltenzahl, BOOLEAN MitGraphikzeichen)
{
  if (!Baum.Wurzel) {
    printf("NULL\n");
    return;
  }
  
  BA_AdressassoziierungBeginnen();
  BA_BaumknotenAssoziieren(Baum);
  Graphikmodus = MitGraphikzeichen ? Normal : Ersatz;
  MitAdressen = PMitAdressen;     /* globale Variablen (als Ersatz fuer durchzureichende Parameter) initialisieren */
  AbstandGeschwisterknoten = PAbstandGeschwisterknoten;

  LRBreiteBerechnen(Baum.Wurzel,FALSE);

  TafelAnlegen(Spaltenzahl);
  KnotenMalen(Baum.Wurzel,FALSE,LinkeBreite(Baum.Wurzel),0);
  TafelAusgeben();
  TafelFreigeben();
  BA_AdressassoziierungBeenden();
}


#define MacheBaumMalenDefault(/*GraphikmodusT*/ GModus)                                                             \
{                                                                                                                   \
  if (!Baum.Wurzel) {                                                                                               \
    printf("NULL\n");                                                                                    \
    return;                                                                                                         \
  }                                                                                                                 \
  BA_AdressassoziierungBeginnen();                                                                                  \
  BA_BaumknotenAssoziieren(Baum);                                                                                   \
  Graphikmodus = GModus;                                                                                            \
  MitAdressen = TRUE;                                                                                               \
  AbstandGeschwisterknoten = BA_KleinsterSchickerAbstand;                                                           \
  LRBreiteBerechnen(Baum.Wurzel,FALSE);                                                                             \
                                                                                                                    \
  while (Gesamtbreite(Baum.Wurzel)>BA_StandardSpaltenzahl && AbstandGeschwisterknoten) {                            \
    AbstandGeschwisterknoten--;                                                                                     \
    LRBreiteBerechnen(Baum.Wurzel,FALSE);                                                                           \
  }                                                                                                                 \
                                                                                                                    \
  TafelAnlegen(Gesamtbreite(Baum.Wurzel));                                                                          \
  KnotenMalen(Baum.Wurzel,FALSE,LinkeBreite(Baum.Wurzel),0);                                                        \
  TafelAusgeben();                                                                                                  \
  TafelFreigeben();                                                                                                 \
  BA_AdressassoziierungBeenden();                                                                                   \
}


void BA_BaumMalenDefaultMitG(DSBaumT Baum)
  MacheBaumMalenDefault(Normal)

void BA_BaumMalenDefaultOhneG(DSBaumT Baum)
  MacheBaumMalenDefault(Ersatz)



/*=================================================================================================================*/
/*= XI. Leere Funktionen                                                                                          =*/
/*=================================================================================================================*/

#else

void BA_DruckeBaum(DSBaumT Baum MAYBE_UNUSEDPARAM) {;}

void BA_BaumMalen(DSBaumT Baum MAYBE_UNUSEDPARAM, BOOLEAN PMitAdressen MAYBE_UNUSEDPARAM, 
  unsigned int PAbstandGeschwisterknoten MAYBE_UNUSEDPARAM, unsigned int Spaltenzahl MAYBE_UNUSEDPARAM, 
  BOOLEAN MitGraphikzeichen MAYBE_UNUSEDPARAM) {;}

void BA_BaumMalenDefaultMitG(DSBaumT Baum MAYBE_UNUSEDPARAM) {;}

void BA_BaumMalenDefaultOhneG(DSBaumT Baum MAYBE_UNUSEDPARAM) {;}

void BA_DumpBaum(DSBaumT Baum MAYBE_UNUSEDPARAM, BOOLEAN MitGraphik MAYBE_UNUSEDPARAM) {;}

void BA_AdressassoziierungBeginnen(void) {;}

unsigned int BA_Assoziation(void *Adresse MAYBE_UNUSEDPARAM) {return 0;}

void BA_AdressassoziierungBeenden(void) {;}

void BA_DruckeAssAdresse(void *Adresse MAYBE_UNUSEDPARAM) {;}

void BA_BaumknotenAssoziieren(DSBaumT Baum MAYBE_UNUSEDPARAM) {;}

#endif
