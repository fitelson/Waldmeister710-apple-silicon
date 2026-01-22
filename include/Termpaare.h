/*=================================================================================================================
  =                                                                                                               =
  =  Termpaare.h                                                                                                  =
  =                                                                                                               =
  =  Realisierung von Termtupeln                                                                                  =
  =                                                                                                               =
  =                                                                                                               =
  =  22.03.95 Thomas.                                                                                             =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef TERMPAARE_H
#define TERMPAARE_H


#include "general.h"
#include "SpeicherVerwaltung.h"
#include "TermOperationen.h"
#include "Id.h"

/*=================================================================================================================*/
/*= I. Typen                                                                                                      =*/
/*=================================================================================================================*/

typedef struct GNTermpaarTS {
  TermT               linkeSeite;
  TermT               rechteSeite;
  TermT               r2;
  long int            Nummer;
  BOOLEAN             lebtNoch;
  AbstractTime        geburtsTag; /*Zeitstempel, wird benoetigt fuer das genaue Nachbilden einer Reduktionsrelation,
          wie sie zu einem Zeitpunkt j < momentaner Zeitpunkt bestand. geburtsTag beinhaltet dabei den Zeitpunkt, zu
          dem die Regel in die Relation aufgenommen wurde.*/
  AbstractTime        todesTag; /*Zeitstempel, wird benoetigt fuer das genaue Nachbilden einer Reduktionsrelation,
          wie sie zu einem Zeitpunkt j < momentaner Zeitpunkt bestand. todesTag beinhaltet dabei den Zeitpunkt, zu dem
          die Regel aus der Reduktionsrelation entfernt wurde. todesTag sollte bei der Aufnahme einer Regel initial auf
          maximalAbstractTime() (=ULONG_MAX) gesetzt werden*/
  unsigned int        generation; /* Axiome: generation = 1; ansonsten: gen(Kind) = 1 + max(gen(Eltern)) */
  unsigned int        tiefeLinks;
  unsigned int        laengeLinks;
  unsigned int        laengeRechts;

  CounterT            rewrites;
  
  struct KnotenTS     *Blatt;
  BOOLEAN             RichtungAusgezeichnet;
  BOOLEAN             IstMonogleichung;
  BOOLEAN             IstUngleichung;
  struct GNTermpaarTS *Gegenrichtung;
  struct GNTermpaarTS *Vorg;
  struct GNTermpaarTS *Nachf;
  BOOLEAN             DarfNichtReduzieren;
  BOOLEAN             DarfNichtUeberlappen;
  BOOLEAN             Grundzusammenfuehrbar;
  BOOLEAN             ACGrundreduzibel;
  BOOLEAN             StrengKonsistent;
  GZ_statusT          gzfbStatus;
  TermT               GescheitertL;
  TermT               GescheitertR;
  unsigned long       aEntryObserver; 
  WId                 wid;
  BOOLEAN             used;
  BOOLEAN             proofPrinted;
#ifdef CCE_Protocol
  TermT               CCE_lhs;
  TermT               CCE_rhs;
#endif
} *GNTermpaarT;



/*=================================================================================================================*/
/*= II. Speicherverwaltung                                                                                        =*/
/*=================================================================================================================*/

extern SV_SpeicherManagerT TP_FreispeicherManager;

#define /*void*/ TP_GNTermpaarHolen(/*L-Ausdruck*/ Index)                                                           \
  SV_Alloc(Index,TP_FreispeicherManager,GNTermpaarT)

#define /*void*/ TP_GNTermpaarLoeschen(/*GNTermpaarT*/ Termpaar)                                                    \
  SV_Dealloc(TP_FreispeicherManager,(Termpaar))

#define /*void*/ TP_LinksRechtsFrischLoeschen(/*GNTermpaarT*/ Termpaar)                                                   \
  TO_FrischzellenTermeLoeschen(TP_LinkeSeite(Termpaar),TP_RechteSeite(Termpaar))

#define /*void*/ TP_LinksRechtsLoeschen(/*GNTermpaarT*/ Termpaar)                                                   \
  TO_TermeLoeschen(TP_LinkeSeite(Termpaar),TP_RechteSeite(Termpaar))



/*=================================================================================================================*/
/*= III. Lesender Zugriff                                                                                         =*/
/*=================================================================================================================*/

#define /*BOOLEAN*/ TP_RichtungAusgezeichnet(/*GNTermpaarT*/ Termpaar)                                              \
  ((Termpaar)->RichtungAusgezeichnet)

#define /*BOOLEAN*/ TP_RichtungUnausgezeichnet(/*GNTermpaarT*/ Termpaar)                                            \
  (!(Termpaar)->RichtungAusgezeichnet)

#define /*BOOLEAN*/ TP_IstMonogleichung(/*GNTermpaarT*/ Termpaar)                                                   \
  ((Termpaar)->IstMonogleichung)

#define /*BOOLEAN*/ TP_IstKeineMonogleichung(/*GNTermpaarT*/ Termpaar)                                              \
  (!(Termpaar)->IstMonogleichung)

#define /*GNTermpaarT*/ TP_Gegenrichtung(/*GNTermpaarT*/ Termpaar)                                                  \
  ((Termpaar)->Gegenrichtung)

#define /*TermT*/ TP_LinkeSeite(/*GNTermpaarT*/ Termpaar)                                                           \
  ((Termpaar)->linkeSeite)

#define /*TermT*/ TP_RechteSeite(/*GNTermpaarT*/ Termpaar)                                                          \
  ((Termpaar)->rechteSeite)

#define /*unsigned*/ TP_ElternNr(/*GNTermpaarT*/ Termpaar)                                                          \
  ((Termpaar)->Nummer)

#define /*GNTermpaarT*/ TP_Vorg(/*GNTermpaarT*/ Termpaar)                                                           \
  ((Termpaar)->Vorg)

#define /*GNTermpaarT*/ TP_Nachf(/*GNTermpaarT*/ Termpaar)                                                          \
  ((Termpaar)->Nachf)

#define /*AbstractTime*/ TP_getGeburtsTag_M(/*GNTermpaarT*/ Termpaar)                                              \
  ((Termpaar)->geburtsTag)

#define /*AbstractTime*/ TP_getTodesTag_M(/*GNTermpaarT*/ Termpaar)                                                \
  ((Termpaar)->todesTag)

#define /* unsgigned int*/ TP_GetGeneration(/*GNTermpaarT*/ Termpaar)                                               \
  ((Termpaar)->generation)

#define /*BOOLEAN*/ TP_IstACGrundreduzibel(/*GNTermpaarT*/ Termpaar) \
  ((Termpaar)->ACGrundreduzibel)

/*=================================================================================================================*/
/*= IV. Schreibender Zugriff                                                                                      =*/
/*=================================================================================================================*/

#define /*void*/ TP_GegenrichtungBesetzen(/*GNTermPaarT*/ Termpaar, /*GNTermPaarT*/ gegenrichtung)                  \
  (Termpaar)->Gegenrichtung = gegenrichtung

#define /*GNTermpaarT*/ TP_VorgSei(/*GNTermpaarT*/ Termpaar, /*GNTermpaarT*/ Vorgaenger)                            \
  ((Termpaar)->Vorg = Vorgaenger)

#define /*GNTermpaarT*/ TP_NachfSei(/*GNTermpaarT*/ Termpaar, /*GNTermpaarT*/ Nachfolger)                           \
  ((Termpaar)->Nachf = Nachfolger)

#define /*void*/ TP_setGeburtsTag_M(/*GNTermpaarT*/ Termpaar, /*AbstractTime*/ currentTime)                         \
  ((Termpaar)->geburtsTag = currentTime)

#define /*void*/ TP_setTodesTag_M(/*GNTermpaarT*/ Termpaar, /*AbstractTime*/ currentTime)                           \
  ((Termpaar)->todesTag = currentTime)

#define /*void*/ TP_initializeTodesTag_M(/*GNTermpaarT*/ Termpaar)  /*hier wird der Todestag auf den maximalen Wert*/\
        /*gesetzt, um anzuzeigen, dass das Termpaar bis zur Unendlichkeit lebt (also noch nicht tot ist)*/          \
  ((Termpaar)->todesTag = maximalAbstractTime())

/*=================================================================================================================*/
/*= V. Abgeleitete Praedikate und Funktionen                                                                      =*/
/*=================================================================================================================*/

#define /*BOOLEAN*/ TP_TermpaarIstRegel(/*GNTermpaarT*/ Termpaar)                                                   \
  !((Termpaar)->Gegenrichtung)

#define /*BOOLEAN*/ TP_TermpaareGleich(/*GNTermpaarT*/ Termpaar1, /*GNTermpaarT*/ Termpaar2)                        \
  (TO_TermeGleich(TP_LinkeSeite(Termpaar1),TP_LinkeSeite(Termpaar2))                                                \
  && TO_TermeGleich(TP_RechteSeite(Termpaar1),TP_RechteSeite(Termpaar2)))

#define /*BOOLEAN*/ TP_FreieVariablen(/*GNTermpaarT*/ Termpaar)                                                     \
  ((Termpaar)->r2)

#define /*BOOLEAN*/ TP_RechteSeiteUnfrei(/*GNTermpaarT*/ Termpaar)                                                  \
  (TP_FreieVariablen(Termpaar) ? (Termpaar)->r2 : TP_RechteSeite(Termpaar))

/*dieses Makro liefert true, falls ein Termpaar zum angegeben Zeitpunkt (schon und noch) als Regel in der 
  Reduktionsrelation existierte; false sonst*/
#ifndef CCE_Source
#define /*BOOLEAN*/ TP_lebtZumZeitpunkt_M(/*GNTermpaarT*/ Termpaar, /*AbstractTime*/ zeitpunkt)                     \
  ((TP_getGeburtsTag_M(Termpaar) <= zeitpunkt) && (TP_getTodesTag_M(Termpaar) > zeitpunkt))

#define /*BOOLEAN*/ TP_lebtAktuell_M(/*GNTermpaarT*/ Termpaar)                                                      \
  (TP_getTodesTag_M(Termpaar) == maximalAbstractTime())
#else
#define /*BOOLEAN*/ TP_lebtZumZeitpunkt_M(/*GNTermpaarT*/ Termpaar, /*AbstractTime*/ zeitpunkt) TRUE

#define /*BOOLEAN*/ TP_lebtAktuell_M(/*GNTermpaarT*/ Termpaar) TRUE
#endif

/*=================================================================================================================*/
/*= VI. Initialisierung                                                                                           =*/
/*=================================================================================================================*/

void TP_InitApriori(void);

#endif /* Termpaare_H */
