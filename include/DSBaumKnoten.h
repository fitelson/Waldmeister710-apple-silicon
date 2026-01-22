#ifndef DSBAUMKNOTEN_H
#define DSBAUMKNOTEN_H

/*=================================================================================================================
  =                                                                                                               =
  =  DSBaumKnoten.h                                                                                               =
  =                                                                                                               =
  =  Anlegen, freigeben, auslesen und beschreiben von Regelbaumknoten                                             =
  =                                                                                                               =
  =                                                                                                               =
  =  17.03.94 Thomas. Nach einigen Geburtswehen.                                                                  =
  =  29.06.94 Arnim.  Zugriffsmakros auf Objekte in Baeumen (Regeln und Gleichungen) eingebaut, um Abstraktion    =
  =                   zu erhalten.                                                                                =
  =  31.08.94 Thomas. Grundlegende Revision fuer Sprunglisten.                                                    =
  =                                                                                                               =
  =================================================================================================================*/


#include "general.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "Termpaare.h"


/*=================================================================================================================*/
/*= I. Typdeklarationen                                                                                           =*/
/*=================================================================================================================*/

typedef struct KnotenTS KnotenT;
typedef KnotenT *KantenT;
typedef struct {
  KantenT Wurzel,
          ErstesBlatt;
  unsigned int maxTiefeLinks;
  KantenT *TiefeBlaetter;
  unsigned int TiefeBlaetterDimension;
} DSBaumT, *DSBaumP;

typedef struct SprungeintragsTS *SprungeintragsT;
typedef struct PraefixeintragsTS *PraefixeintragsT;

typedef BOOLEAN (*UDeltaT)(void);

typedef struct {
  struct {
    KantenT UVorg;
    UTermT MVorgZelle;

/* *******************
 * BL BL BL BL 
 * *******************/

    KantenT BackKn;
    SymbolT BackSub;

/* *******************
 * BL BL BL BL 
 * *******************/

  } MUDaten1;
  union {
    struct {
      UTermT aktTIndexM;
      SymbolT MSymbol;

      UTermT aktTIndexU;
      unsigned short int UAnzBindungen;
      UDeltaT UDelta;
      union {
        SymbolT USymbol;
        SprungeintragsT UZiel;
      } UVariante;
    } MUDaten2;
    struct {
      unsigned short int LinkeBreite;
      unsigned short int RechteBreite;
    } ADaten;
  } MUADaten;
} RuecksetzInfoT;

#define InnereUndBlattObjekte                                                                                       \
  KantenT Vorg;                                                                                                     \
  SprungeintragsT Sprungeingaenge;                                                                                  \
  SymbolT VorgSymb;                                                                                                 \
  RuecksetzInfoT RuecksetzInfo;                                                                                     \
  BOOLEAN IstBlatt                                                                                                  \

typedef struct {
  KantenT *AnfNachf;
  KantenT *Nachf;
  SprungeintragsT Sprungausgaenge;
  SymbolT minSymb, maxSymb;
} InnererKnotenT;


typedef struct {
  UTermT Rest;
  UTermT linkeSeite;
  GNTermpaarT Regeln;
  PraefixeintragsT Praefixliste;
  unsigned int SuffixPosition;
  KantenT VorgBlatt, NachfBlatt;
} BlattKnotenT;


struct KnotenTS {
  InnereUndBlattObjekte;
  union {
    InnererKnotenT Innen;
    BlattKnotenT Blatt;
  } Variante;
};


struct SprungeintragsTS {
  KantenT Zielknoten;
  KantenT Startknoten;
  UTermT Teilterm;
  unsigned int PositionNachTeilterm;
  SprungeintragsT NaechsterStarteintrag;
  SprungeintragsT VorigerZieleintrag;
  SprungeintragsT NaechsterZieleintrag;
  PraefixeintragsT Praefixeintrag;
};


struct PraefixeintragsTS {
  SprungeintragsT Sprungeintrag;
  PraefixeintragsT NaechsterPraefixeintrag;
};
 


/*=================================================================================================================*/
/*= II. Spezielle Zugriffsfunktionen fuer innere Knoten                                                           =*/
/*=================================================================================================================*/

#define /*KantenT*/ BK_AnfNachf(/*KantenT*/ Kante)                                                                  \
  ((Kante)->Variante.Innen.AnfNachf)                             /* Zeiger auf Anfangsadresse des Nachfolgerfeldes */

#define /*void*/ BK_AnfNachfSei(/*KantenT*/ Kante, /*KantenT*/ AnfNachfolger)                                       \
  (Kante)->Variante.Innen.AnfNachf = AnfNachfolger

#define /*KantenT*/ BK_Nachf(/*KantenT*/ Kante, /*SymbolT*/ Symbol)                                                 \
  ((Kante)->Variante.Innen.Nachf[Symbol])                  /* Eintrag im Nachfolgerfeld zu einem bestimmten Symbol */ 

#define /*void*/ BK_NachfSei(/*KantenT*/ Kante, /*SymbolT*/ Symbol, /*KantenT*/ Nachfolger)                         \
  (Kante)->Variante.Innen.Nachf[Symbol] = Nachfolger

#define /*KantenT*/ BK_FeldNachf(/*KantenT*/ Kante)                                                                 \
  ((Kante)->Variante.Innen.Nachf)                                       /* Zeiger auf Feldelement an Position Null */

#define /*void*/ BK_FeldNachfSei(/*KantenT*/ Kante, /*KantenT**/ Feld)                                              \
  (Kante)->Variante.Innen.Nachf = Feld                                  /* Zeiger auf Feldelement an Position Null */

#define /*SymbolT*/ BK_minSymb(/*KantenT*/ Kante)                                                                   \
  ((Kante)->Variante.Innen.minSymb)                            /* kleinstes mit einem Nachfolger versehenes Symbol */

#define /*SymbolT*/ BK_minSymbSei(/*KantenT*/ Kante, /*SymbolT*/ FktSymb)                                           \
  (Kante)->Variante.Innen.minSymb = FktSymb

#define /*SymbolT*/ BK_maxSymb(/*KantenT*/ Kante)                                                                   \
  ((Kante)->Variante.Innen.maxSymb)                            /* groesstes mit einem Nachfolger versehenes Symbol */

#define /*SymbolT*/ BK_maxSymbSei(/*KantenT*/ Kante, /*SymbolT*/ FktSymb)                                           \
  (Kante)->Variante.Innen.maxSymb = FktSymb

#define /*SymbolT*/ BK_MinMaxSymbSei1(/*KantenT*/ Kante, /*SymbolT*/ Symb)                                          \
  (Kante)->Variante.Innen.maxSymb = ((Kante)->Variante.Innen.minSymb) = Symb /* Besetzen bei gen. einem Nachfolger */ 

#define /*SymbolT*/ BK_MinMaxSymbSei2(/*KantenT*/ Kante, /*SymbolT*/ Symb1, /*SymbolT*/ Symb2)                      \
  if ((Symb1)<(Symb2)) {                                                                                            \
    BK_minSymbSei(Kante,Symb1);                                                                                     \
    BK_maxSymbSei(Kante,Symb2);                                                                                     \
  }                                                                                                                 \
  else {                                                                                                            \
    BK_minSymbSei(Kante,Symb2);                                                                                     \
    BK_maxSymbSei(Kante,Symb1);                                                                                     \
  }                                                                         /* Besetzen bei genau zwei Nachfolgern */

#define /*void*/ BK_MinMaxSymbAktualisieren(/*KantenT*/ Kante, /*SymbolT*/ Symbol)                                  \
  if ((Symbol)<BK_minSymb(Kante))                                                                                   \
    BK_minSymbSei(Kante,Symbol);                                                                                    \
  else {                                                                                                            \
    if ((Symbol)>BK_maxSymb(Kante))                                                                                 \
      BK_maxSymbSei(Kante,Symbol);                                                                                  \
  }                                                                    /* Besetzen bei beliebig vielen Nachfolgern */

#define /*BOOLEAN*/ BK_genauEinNachf(/*KantenT*/ Kante)                                                             \
  (BK_minSymb(Kante)==BK_maxSymb(Kante))                                 /* Hat der Knoten genau einen Nachfolger? */

#define /*int*/ BK_AnzVariablen(/*KantenT*/ Kante)                                                                  \
  ((Kante)->Variante.Innen.Nachf-(Kante)->Variante.Innen.AnfNachf)    /* Anzahl moeglicher Nachfolger zu Variablen */

#define /*SymbolT*/ BK_letzteVar(/*KantenT*/ Kante)                                                                 \
  ((Kante)->Variante.Innen.AnfNachf-(Kante)->Variante.Innen.Nachf)  /* letzte Variable, zu der Nachfolger moeglich */

#define /*BOOLEAN*/ BK_letzteVarBelegt(/*KantenT*/ Kante)                                                           \
  SO_SymbGleich(BK_letzteVar(Kante),BK_minSymb(Kante))   /* Gibt es einen Nachfolger zur letztmoeglichen Variable? */

#define /*SprungeintragsT*/ BK_Sprungausgaenge(/*KantenT*/ Kante)                                                   \
  ((Kante)->Variante.Innen.Sprungausgaenge)

#define /*SprungeintragsT*/ BK_SprungausgaengeSei(/*KantenT*/ Kante, /*SprungeintragsT*/ Sprungliste)               \
  ((Kante)->Variante.Innen.Sprungausgaenge = Sprungliste)

#define BK_forSprungziele(/*SprungeintragsT*/ ZielIndex, /*SprungeintragsT*/ erstesZiel)                            \
  for (ZielIndex = erstesZiel; ZielIndex; ZielIndex = ZielIndex->NaechsterZieleintrag)

unsigned int BK_AnzNachfolger(KantenT Knoten);                  /* Anzahl der existierenden Nachfolger des Knotens */

/* *******************
 * BL BL BL BL 
 * *******************/

#define /*KnotenT*/ BK_backtrKnoten(/*KantenT*/ Kante)\
  ((Kante)->RuecksetzInfo.MUDaten1.BackKn)
#define /*KnotenT*/ BK_backtrKnotenSei(/*KantenT*/ Kante, /*KantenT*/ BackKante)\
  ((Kante)->RuecksetzInfo.MUDaten1.BackKn = BackKante)
#define /*SymbolT*/ BK_backtrSub(/*KantenT*/ Kante)\
  ((Kante)->RuecksetzInfo.MUDaten1.BackSub)
#define /*SymbolT*/ BK_backtrSubSei(/*KantenT*/ Kante, /*SymbolT*/ Sym)\
  ((Kante)->RuecksetzInfo.MUDaten1.BackSub = Sym)

/* *******************
 * BL BL BL BL 
 * *******************/

/*=================================================================================================================*/
/*= III. Spezielle Zugriffsfunktionen fuer Blattknoten                                                            =*/
/*=================================================================================================================*/

#define /*UTermT*/ BK_Rest(/*KantenT*/ Kante)                                                                       \
  ((Kante)->Variante.Blatt.Rest)  /* Zeiger auf den nicht den inneren Knoten zu entnehmenden Rest der linken Seite */

#define /*void*/ BK_RestSei(/*KantenT*/ Kante, /*UTermT*/ Termrest)                                                 \
  (Kante)->Variante.Blatt.Rest = Termrest

#define /*UTermT*/ BK_linkeSeite(/*KantenT*/ Kante)                                                                 \
  ((Kante)->Variante.Blatt.linkeSeite)                                         /* Zeiger auf linke Seite der Regel */

#define /*void*/ BK_linkeSeiteSei(/*KantenT*/ Kante, /*UTermT*/ Term)                                               \
  (Kante)->Variante.Blatt.linkeSeite = Term

#define /*GNTermpaarT*/ BK_Regeln(/*KantenT*/ Kante)                                                                \
  ((Kante)->Variante.Blatt.Regeln)                                     /* Zeiger auf erstes Element der Regelliste */

#define /*void*/ BK_RegelnSei(/*KantenT*/ Kante, /*GNTermpaarT*/ Regelliste)                                        \
  ((Kante)->Variante.Blatt.Regeln = Regelliste)

#define /*unsigned int*/ BK_SuffixPosition(/*KantenT*/ Kante)                                                       \
  ((Kante)->Variante.Blatt.SuffixPosition)                                                                               

#define /*unsigned int*/ BK_SuffixPositionSei(/*KantenT*/ Kante, /*unsigned int*/ Nummer)                           \
  ((Kante)->Variante.Blatt.SuffixPosition = Nummer)                                                                               

#define /*PraefixeintragsT*/ BK_Praefixliste(/*KantenT*/ Kante)                                                     \
  ((Kante)->Variante.Blatt.Praefixliste)                                                                               

#define /*PraefixeintragsT*/ BK_PraefixlisteSei(/*KantenT*/ Kante, /*PraefixeintragsT*/ NeuePraefixliste)           \
  ((Kante)->Variante.Blatt.Praefixliste = NeuePraefixliste)                                                                               

#define /*KantenT*/ BK_VorgBlatt(/*KantenT*/ Kante)                                                                 \
  ((Kante)->Variante.Blatt.VorgBlatt)          /* Zeiger auf vorhergehendes Blatt in der Verzeigerung der Blaetter */

#define /*void*/ BK_VorgBlattSei(/*KantenT*/ Kante, /*KantenT*/ VBlatt)                                             \
  ((Kante)->Variante.Blatt.VorgBlatt = VBlatt)

#define /*KantenT*/ BK_NachfBlatt(/*KantenT*/ Kante)                                                                \
  ((Kante)->Variante.Blatt.NachfBlatt)          /* Zeiger auf nachfolgendes Blatt in der Verzeigerung der Blaetter */

#define /*void*/ BK_NachfBlattSei(/*KantenT*/ Kante, /*KantenT*/ NBlatt)                                            \
  ((Kante)->Variante.Blatt.NachfBlatt = NBlatt)

#define BK_forBlaetter(/*KantenT*/ BlattIndex, /*DSBaumT*/ Baum)                                                    \
  for (BlattIndex = Baum.ErstesBlatt; BlattIndex; BlattIndex = BK_NachfBlatt(BlattIndex))


/*=================================================================================================================*/
/*= IV. Gemeinsame Zugriffsfunktionen                                                                             =*/
/*=================================================================================================================*/

#define /*KantenT*/ BK_Vorg(/*KantenT*/ Kante)                                                                      \
  ((Kante)->Vorg)                                                              /* Zeiger auf vorhergehenden Knoten */

#define /*void*/ BK_VorgSei(/*KantenT*/ Kante, /*KantenT*/ Vorgaenger)                                              \
  (Kante)->Vorg = Vorgaenger

#define /*SymbolT*/ BK_VorgSymb(/*KantenT*/ Kante)                                                                  \
  ((Kante)->VorgSymb)

#define /*void*/ BK_VorgSymbSei(/*KantenT*/ Kante, /*SymbolT*/ Symbol)                                              \
  (Kante)->VorgSymb = Symbol


/* Ruecksetzinformationen fuer das Matchen */

#define /*SymbolT*/ BK_aktTIndexM(/*KantenT*/ Kante)                                                                \
  ((Kante)->RuecksetzInfo.MUADaten.MUDaten2.aktTIndexM)           /* gerade betrachteter Teilterm des Anfrageterms */

#define /*void*/ BK_aktTIndexMSei(/*KantenT*/ Kante, /*UTermT*/ TIndex)                                             \
  ((Kante)->RuecksetzInfo.MUADaten.MUDaten2.aktTIndexM) = TIndex

#define BK_NULL                                                                                                     \
  SO_GroesstesFktSymb+2                /* besonderes Symbol, zu dem es im ganzen Baum keinen Nachfolger geben kann */

#define /*SymbolT*/ BK_MSymbol(/*KantenT*/ Kante)                                                                   \
  ((Kante)->RuecksetzInfo.MUADaten.MUDaten2.MSymbol)

#define /*void*/ BK_MSymbolSei(/*KantenT*/ Kante, /*SymbolT*/ Symbol)                                               \
  ((Kante)->RuecksetzInfo.MUADaten.MUDaten2.MSymbol = Symbol)

#define /*UTermT*/ BK_MVorgZelle(/*KantenT*/ Kante)                                                                 \
  ((Kante)->RuecksetzInfo.MUDaten1.MVorgZelle)

#define /*void*/ BK_MVorgZelleSei(/*KantenT*/ Kante, /*UTermT*/ VorgZelle)                                          \
  ((Kante)->RuecksetzInfo.MUDaten1.MVorgZelle = VorgZelle)


/* Ruecksetzinformationen fuer das Unifizieren */

#define /*SymbolT*/ BK_aktTIndexU(/*KantenT*/ Kante)                                                                \
  ((Kante)->RuecksetzInfo.MUADaten.MUDaten2.aktTIndexU)           /* gerade betrachteter Teilterm des Anfrageterms */

#define /*void*/ BK_aktTIndexUSei(/*KantenT*/ Kante, /*UTermT*/ TIndex)                                             \
  ((Kante)->RuecksetzInfo.MUADaten.MUDaten2.aktTIndexU) = TIndex

#define /*unsigned short int*/ BK_UAnzBindungen(/*KantenT*/ Kante)                   /* Anzahl der beim Absteigen */\
  ((Kante)->RuecksetzInfo.MUADaten.MUDaten2.UAnzBindungen)      /* aus dem Knoten vorgenommenen Variablenbindungen */

#define /*void*/ BK_UAnzBindungenSei(/*KantenT*/ Kante, /*unsigned short int */ Anzahl)                             \
  (Kante)->RuecksetzInfo.MUADaten.MUDaten2.UAnzBindungen = Anzahl

#define /*KantenT*/ BK_UVorg(/*KantenT*/ Kante)                                                                     \
  ((Kante)->RuecksetzInfo.MUDaten1.UVorg)                      /* Zeiger auf vorhergehenden Knoten bei Unifikation */

#define /*void*/ BK_UVorgSei(/*KantenT*/ Kante, /*KantenT*/ Vorgaenger)                                             \
  (Kante)->RuecksetzInfo.MUDaten1.UVorg = Vorgaenger

#define /*SymbolT*/ BK_USymbol(/*KantenT*/ Kante)                                                                   \
  ((Kante)->RuecksetzInfo.MUADaten.MUDaten2.UVariante.USymbol)

#define /*void*/ BK_USymbolSei(/*KantenT*/ Kante, /*SymbolT*/ Symbol)                                               \
  ((Kante)->RuecksetzInfo.MUADaten.MUDaten2.UVariante.USymbol = Symbol)

#define /*SprungeintragsT*/ BK_UZiel(/*KantenT*/ Kante)                                                             \
  ((Kante)->RuecksetzInfo.MUADaten.MUDaten2.UVariante.UZiel)

#define /*void*/ BK_UZielSei(/*KantenT*/ Kante, /*SprungeintragsT*/ Ziel)                                           \
  ((Kante)->RuecksetzInfo.MUADaten.MUDaten2.UVariante.UZiel = Ziel)

#define /*void*/ BK_UDatumSei(/*KantenT*/ Kante, /*{"Ziel","Symbol"}*/ SorteUDatum,                                 \
                                                                                /*SprungeintragsT U SymbolT*/ Datum)\
  BK_U ## SorteUDatum ## Sei(Kante,Datum)

#define /*UDeltaT*/ BK_UDelta(/*KantenT*/ Kante)                                                                    \
  ((*((Kante)->RuecksetzInfo.MUADaten.MUDaten2.UDelta))())

#define /*void*/ BK_UDeltaSei(/*KantenT*/ Kante, /*UDeltaT*/ Delta)                                                 \
  ((Kante)->RuecksetzInfo.MUADaten.MUDaten2.UDelta = Delta)


/* Linke und rechte Breite fuer DSBaumAusgaben */

#define /*unsigned short int*/ BK_LinkeBreite(/*KantenT*/ Kante)                                                    \
  ((Kante)->RuecksetzInfo.MUADaten.ADaten.LinkeBreite)

#define /*void*/ BK_LinkeBreiteSei(/*KantenT*/ Kante, /*unsigned short in*/ Breite)                                 \
  ((Kante)->RuecksetzInfo.MUADaten.ADaten.LinkeBreite = Breite)

#define /*unsigned short int*/ BK_RechteBreite(/*KantenT*/ Kante)                                                   \
  ((Kante)->RuecksetzInfo.MUADaten.ADaten.RechteBreite)

#define /*void*/ BK_RechteBreiteSei(/*KantenT*/ Kante, /*unsigned short in*/ Breite)                                \
  ((Kante)->RuecksetzInfo.MUADaten.ADaten.RechteBreite = Breite)


/* Sonstiges */

void BK_NachfolgenLassen(KantenT Knoten1, SymbolT Symbol, KantenT Knoten2);
   /* Knoten2 als Nachfolger von Knoten1 zum angegebenen Symbol eintragen; Knoten1 als Vorgaenger von Knoten1 ein- */
                                      /* tragen; in Knoten2 Symbol eintragen, mit dem in Knoten2 abgestiegen wurde */

#define /*BOOLEAN*/ BK_IstBlatt(/*KantenT*/ Kante)                                                                  \
  ((Kante)->IstBlatt)                                /* Ist der aktuelle Knoten ein Blatt oder ein innerer Knoten? */

#define /*void*/ BK_IstBlattSei(/*KantenT*/ Kante, /*BOOLEAN*/ Blatt)                                               \
  (Kante)->IstBlatt = Blatt

#define /*SprungeintragsT*/ BK_Sprungeingaenge(/*KantenT*/ Kante)                                                   \
  ((Kante)->Sprungeingaenge)

#define /*SprungeintragsT*/ BK_SprungeingaengeSei(/*KantenT*/ Kante, /*SprungeintragsT*/ Sprungliste)               \
  ((Kante)->Sprungeingaenge = Sprungliste)



/*=================================================================================================================*/
/*= V. Speicherverwaltung fuer Regelbaum                                                                          =*/
/*=================================================================================================================*/

void BK_InitApriori(void);                 /* Initialisiert die Speicherverwaltungen fuer Blaetter, innere Knoten, */
                                                                              /* Sprungeintraege, Praefixeintraege */

void BK_InitAposteriori(void);                 /* Initialisiert die Speicherverwaltung fuer die Nachfolger-Arrays. */



/*=================================================================================================================*/
/*= VI. Anlegen und Freigeben der verschiedenen Objekte                                                            */
/*=================================================================================================================*/

SprungeintragsT BK_SprungeintragAnlegen(void);       /* Legt einen Sprungeintrag an und gibt Zeiger darauf zurueck.*/

void BK_SprungeintragFreigeben(SprungeintragsT Sprungeintrag);            /* Gibt einen Sprungeintrag wieder frei. */

PraefixeintragsT BK_PraefixeintragAnlegen(void);    /* Legt einen Praefixeintrag an und gibt Zeiger darauf zurueck.*/

void BK_PraefixeintragFreigeben(PraefixeintragsT Praefixeintrag);        /* Gibt einen Praefixeintrag wieder frei. */

KantenT BK_BlattAnlegen(void);  /* Legt ein Blatt fuer den Regelbaum an u. gibt Zeiger darauf als KantenT zurueck. */

void BK_BlattFreigeben(KnotenT *Blatt);                 /* Gibt ein mit BlattAnlegen angelegtes Blatt wieder frei. */

KantenT BK_KnotenAnlegen(unsigned int VariablenZahl);  /* Legt einen Knoten mit Platz fuer VariablenZahl Variablen an. Die
      entsprechenden Anpassungen fuer Variablenzahlen, die noch nicht vorkommen, werden dabei automatisch durchge-
                                                                 fuehrt. Annahme/Voraussetzung: VariablenZahl > 0. */
void BK_KnotenFreigeben(KnotenT *Knoten);            /* Gibt einen mit KnotenAnlegen angelegten Knoten wieder frei.*/



/*=================================================================================================================*/
/*= VII. Zugriffe auf in Baeumen gespeicherte Objekte (Regeln und Gleichungen)                                     */
/*=================================================================================================================*/

#define BK_forRegeln(/*GNTermpaarT*/ RegelIndex, /*DSBaumT*/ Baum, /*Block*/ BK_forRegelnAction)                    \
{                                                                                                                   \
  KantenT _BlattIndex;                                                                                              \
  BK_forBlaetter(_BlattIndex,Baum) {                                                                                \
    RegelIndex = BK_Regeln(_BlattIndex);                                                                            \
    do {                                                                                                            \
      BK_forRegelnAction;                                                                                           \
    } while ((RegelIndex = RegelIndex->Nachf));                                                                     \
  }                                                                                                                 \
}


#define BK_forRegelnRobust(/*GNTermpaarT*/ RegelIndex, /*DSBaumT*/ Baum, /*Block*/ BK_forRegelnAction)              \
{  /* Diese Durchlauf-Prozedur verkraftet auch die Entfernung des aktuellen Objektes (in RegelIndex) aus dem Baum.*/\
  KantenT BlattIndex = Baum.ErstesBlatt;                                                                            \
  while (BlattIndex) {                                                                                              \
    KantenT _naechstesBlatt = BK_NachfBlatt(BlattIndex);                                                            \
    RegelIndex = BK_Regeln(BlattIndex);                                                                             \
    do {                                                                                                            \
      GNTermpaarT _naechsteRegel = TP_Nachf(RegelIndex);                                                            \
      BK_forRegelnAction;                                                                                           \
      RegelIndex = _naechsteRegel;                                                                                  \
    } while (RegelIndex);                                                                                           \
    BlattIndex = _naechstesBlatt;                                                                                   \
  }                                                                                                                 \
}

/* Wendet sukzessive test auf alle Referenz-Regeln des Baumes an. Liefert test TRUE, so werden alle Eintraege des 
   Blattes entfernt und anschliessend beh darauf angewendet.*/
#define BK_ReferenzDurchlauf(/*GNTermpaarT*/ RegelIndex, /*DSBaumT*/ Baum, /*Boolexpr*/ Test, /*Block*/ Action) \
{                                                                                                               \
  KantenT BlattIndex = Baum.ErstesBlatt;                                                                        \
  while(BlattIndex){                                                                                            \
    KantenT _naechstesBlatt = BK_NachfBlatt(BlattIndex);                                                        \
    RegelIndex = BK_Regeln(BlattIndex);                                                                         \
    if(Test){                                                                                                   \
      do {                                                                                                      \
        GNTermpaarT _naechsteRegel = TP_Nachf(RegelIndex);                                                      \
        Action;                                                                                                 \
        RegelIndex = _naechsteRegel;                                                                            \
      } while (RegelIndex);                                                                                     \
    }                                                                                                           \
    BlattIndex = _naechstesBlatt;                                                                               \
  }                                                                                                             \
}

GNTermpaarT BK_ErstesTermpaar(DSBaumT Baum);                                            /* Zaehlen sukzessive alle */
GNTermpaarT BK_NaechstesTermpaar(void);                                        /* im Index abgelegten Elemente auf */

GNTermpaarT BK_ErstesTermpaarM(DSBaumT Baum, KantenT *iBlatt, GNTermpaarT *iTermpaar);  /* dito mit expliziten In- */
GNTermpaarT BK_NaechstesTermpaarM(KantenT *iBlatt, GNTermpaarT *iTermpaar);             /* dizes fuer Schachtelung */



#endif
