/* ----------------------------------------------------------------------------------------------------------
   Statistiken.h
  
   Diese Modul ist zustaendig fuer die Ausgabe von Statistiken, genauer:

   Fuer den Aufruf der Statistik-Funktionen der einzelnen Module,
   fuer Ergebnis-Ausgabe, und fuer Ergebnis- und Zwischen-Menues.

   20.06.96 Ausgelagert aus Ausgaben

*/
#ifndef STATISTIKEN_H
#define STATISTIKEN_H


#include "general.h"

/* -------------------------------------------------------*/
/* -------------- Statistik ------------------------------*/
/* -------------------------------------------------------*/
/* Bei eingeschalteter Interaktion: Interaktives Ergebnis-Menue.
   Sonst Ausgabe diverser Daten gemaess Aufruf.

   In den Modi convergence und termination, wo der Terminationstest auf der Regelmenge ausgefuehrt wird,
   gibt der Wert von Resultat an, ob alle Ausgangsgleichungen richtbar waren.
*/

void ST_SuccessStatistik(const char *Bemerkung, BOOLEAN Resultat);
void ST_ErrorStatistik(const char *Bemerkung, BOOLEAN Resultat);
void ST_NoMemory (const char *Bemerkung, BOOLEAN Resultat);
void ST_TimeoutStatistik(const char *Bemerkung, BOOLEAN Resultat);
void ST_SignalUsrStatistik(const char *Bemerkung);
void ST_GoalProvedStatistik(const char *Bemerkung);
#if COMP_POWER
void ST_PowerStatistik(const char *Bemerkung);
#endif
void SV_Statistik(BOOLEAN Fehlerfall, BOOLEAN NurDiskrepanzen);

/* -------------------------------------------------------*/
/* -------------- Interne Statistik ----------------------*/
/* -------------------------------------------------------*/

void ST_InterneStatistik(void);
/* Ausgabe von internen Daten (zB. Pfad-Ausdehnungen etc.) */

/* -------------------------------------------------------*/
/* -------------- Prozess-Statistik ----------------------*/
/* -------------------------------------------------------*/

void ST_ProzessStatistik(void);
/* Ausgabe von Prozess-Daten (aus getrusage)  */

/* -------------------------------------------------------*/
/* -------------- Konfiguration --------------------------*/
/* -------------------------------------------------------*/

void ST_DruckeKonfiguration(void);

/* -------------------------------------------------------*/
/* -------------------------------------------------------*/
/* ----GESAMMELTE ZAEHLER ETC. ---------------------------*/
/* -------------------------------------------------------*/
/* -------------------------------------------------------*/

typedef struct {
  CounterT AnzEingefRegeln;                                              /* Einfuege-Operationen. */
  CounterT AnzEntfernterRegeln;                                            /* Loesch-Operationen. */
  CounterT AnzSchrumpfPfadExp;               /* Expansionen von geschrumpften Pfaden zu Blaettern */
  CounterT AnzPfadschrumpfungen;  /* Zusammenziehungen linearer Baeume zu Blaettern beim Loeschen */

  CounterT AnzGleichungenInterRed;            /* Gleichungen, die der Interreduktion zum Opfer fielen. */
  CounterT AnzRegRechtsRed;                   /* Reduktionen an rechten Seiten von Regeln.*/
  CounterT AnzRegWgLinksRedWeg;               /* Regeln, die wegen reduzierter linker Seite rausgeworfen wurden.*/
  CounterT AnzErfSubsumptionenAlt;            /* Alte Gleichungen, die von neuen subsummiert wurden. */
  CounterT AnzKPIRBehandelt;                  /* Bei der KPM-IR insgesamt betrachtete KPs */
  CounterT AnzKPIRReduziert;                  /* Bei der KPM-IR reduzierte KPs */
  CounterT AnzKPIRZusammengefuehrt;           /* Bei der KPM-IR von der Anwendungsprozedur zusammengefuehrte KPs */
  CounterT AnzKPIRSubsummiert;                /* Bei der KPM-IR von der Anwendungsprozedur subsummierte KPs */

  CounterT KPV_AnzCPs;
  CounterT KPV_AnzCritGoalsGesamt; 
  CounterT KPV_AnzMaxCritGoals; 
  CounterT KPV_AnzCritGoalsActual;
  CounterT KPV_AnzIRGleich; 
  CounterT KPV_AnzIRReduziert; 
  CounterT KPV_AnzIRWMord;

  CounterT KPV_AnzAktivierterFakten;          /* beinhaltet auch die entfernten Fakten */
  CounterT KPV_AnzAktivierterRE;              /* aktivierte Elemente aus KP-Menge */
  CounterT KPV_AnzAktivierterG;               /* dito aus CG-Menge */
  CounterT KPV_AnzAktivierterREFifo;          /* aus KP-Menge mit Fifo */
  CounterT KPV_AnzAktivierterREHeu;           /* aus KP-Menge mit Heuristik */
  CounterT KPV_AnzAktivierterGFifo;           /* dito aus CG-Menge */
  CounterT KPV_AnzAktivierterGHeu;            /* dito aus CG-Menge */

  CounterT KPV_AnzKPGesamt;                   /* Insgesamt berechnete kritischer Paare */

  CounterT KPV_AnzKPAusSpezifikation;         /* Kritische Paare aus der Spezifikation */
  CounterT KPV_AnzKPIROpfer;                  /* KPs aus geloeschten Regeln und/oder Gleichungen */

  CounterT KPV_AnzKPdirektZusammengefuehrt;   /* Sofort nach der Bildung zusammenfuehrbare kritische Paare */
  CounterT KPV_AnzKPdirektSubsummiert;        /* Sofort nach der Bildung subsummierte kritische Paare */
  CounterT KPV_AnzKPdirektPermSubsummiert;    /* Sofort nach der Bildung PERM-subsummierte kritische Paare */
  CounterT KPV_AnzKPInserted;                 /* In die Menge eingefuegte KPs */

  CounterT KPV_AnzKPBehandeltInVervollst;     /* Zum Vervollstaendigungsalgorithmus hochgereichte KPs */
  CounterT KPV_AnzKPZurueckgestellt;          /* Kritische Paare, die wegen Unrichtbarkeit zurueckgestellt wurden. */
  CounterT KPV_AnzKPspaeterZusammengefuehrt;  /* Kritische Paare, die bei ihrer Behandlung zusammengefuehrt werden konnten. */
  CounterT KPV_AnzKPspaeterSubsummiert;       /* Kritische Paare, die bei ihrer Behandlung subsummiert werden konnten. */
  CounterT KPV_AnzKPspaeterPermSubsummiert;   /* Kritische Paare, die bei ihrer Behandlung PERM-subsummiert werden konnten. */
  CounterT KPV_AnzKPZuRE;                     /* Zu Regeln/Gleichungen gemachte KPs */
  CounterT KPV_AnzKPWeggeworfen;              /* Weggeworfen, weil Gewicht zu gross */

  CounterT AnzKPSpaeterWaisenmord;            /* Bei erreichen der Top-Position der PQ als Waisen gekillte KPs */
  CounterT AnzKPFrueherWaisenmord;            /* Vorzeitig als Waisen gekillte KPs */

  CounterT AnzKPIRBetrachtet;                 /* Bei der KPM-IR insgesamt betrachtete KPs */
  CounterT AnzKPIRWaisenmord;                 /* Bei der KPM-IR als Waisen geloeschte KPs */
  CounterT AnzKPIRGeloescht;                  /* Bei der KPM-IR von der Anwendungsprozedur geloeschte KPs */
  CounterT AnzKPIRVeraendert;                 /* Bei der KPM-IR reduzierte und deshalb ggf. verschobene KPs */
  CounterT AnzKPIRVorgezogen;                 /* Bei der KPM-IR red. und gewichtsverkleinerte KPs */
  CounterT AnzKPIRZurueckgeschoben;           /* Bei der KPM-IR red. und gewichtsvergroesserte KPs */

  CounterT AnzKPBlaeh;                        /* Maximal gleichzeitig gespeicherte KP's */
  CounterT AnzKPEingefuegt;                   /* Anzahl der insgesamt eingefuegten KPs */
  CounterT AnzKPGeloescht;                    /* Anzahl der insgesamt (incl. Waisenmord) entfernten KPs */

  CounterT HA_AnzBucketGleich;
  CounterT HA_AnzKey1Gleich;
  CounterT HA_AnzKey2Gleich;
  CounterT HA_MaxHashSize;
  CounterT HA_MaxElemsInBucket;

  CounterT MNF_AnzDirektZus;
  CounterT MNF_AnzReduktionen;
  CounterT MNF_AnzMatchVersucheE;
  CounterT MNF_AnzMatchVersucheR;
  CounterT MNF_AnzCalls;
  CounterT MNF_AnzTermGleich;
  CounterT MNF_AnzTermGleichCol;
  CounterT MNF_AnzTermGleichOpp;
  CounterT MNF_MaxNachfolger;
  CounterT MNF_MaxEQUp;
  CounterT MNF_AbbruchMaxSize;
  CounterT MNF_SumAnzNachfolger;
  CounterT MNF_AnzCallsHAAddData;
  CounterT MNF_AnzAntiR;
  CounterT MNF_AnzEQUp;

  CounterT MNFZ_MaxCountMNFSet;

  CounterT AnzRedMisserfolgeOrdnung;                /* An >-Test gescheiterte Reduktionsversuche mit E */

  CounterT AnzRedVersMitRegeln;   /* Reduktionsversuche mit der Regelmenge */
  CounterT AnzRedErfRegeln;       /* Erfolgreiche Reduktionsversuche mit der Regelmenge */
  CounterT AnzRedVersMitGleichgn; /* Reduktionsversuche mit der Gleichungsmenge */
  CounterT AnzRedErfGleichgn;     /* Erfolgreiche Reduktionsversuche mit der Gleichungsmenge */

  CounterT VorwaertsMatchR;
  CounterT VorwaertsMatchE;
  CounterT VorwaertsMatchS;

  CounterT AnzKPOpferKapur;       /* KPs, die dem Kapur-Kriterium bei ihrer Bildung zum Opfer fielen */
  CounterT AnzKPGG;                                                    /* KPs zwischen 2 Gleichungen */
  CounterT AnzKPRR;                                                         /* KPs zwischen 2 Regeln */
  CounterT AnzKPRG;                                           /* KPs zwischen Regeln und Gleichungen */
  CounterT AnzUnifVersRegelnToplevel;       /* Unifikationsversuche mit der Regelmenge auf Toplevel. */
  CounterT AnzUnifVersRegelnTeilterme;    /* Unifikationsversuche mit den Teiltermen der Regelmenge. */
  CounterT AnzUnifVersRegelnEigeneTterme; /* Unifikationsversuche mit den eigenen Teiltermen (Regel) */
  CounterT AnzUnifErfRegelnToplevel;    /* Erfolgreiche Unifikationsversuche mit der R auf Toplevel. */
  CounterT AnzUnifErfRegelnTeilterme; /* Erfolgreiche Unifikationsversuche mit den Teiltermen der R. */
  CounterT AnzUnifErfRegelnEigeneTterme;      /* Erfolgreiche Unifikationsversuche den eigenen Teiltermen (Regel). */
  CounterT AnzUnifVersGleichgnToplevel;/* Unifikationsversuche mit der Gleichungsmenge auf Toplevel. */
  CounterT AnzUnifVersGleichgnTeilterme;         /* Versuche mit den Teiltermen der Gleichungsmenge. */
  CounterT AnzUnifVersGleichgnEigeneTterme;        /* Versuche mit den eigenen Teiltermen (Gleichg.) */
  CounterT AnzUnifErfGleichgnToplevel;              /* Erfolge mit der Gleichungsmenge auf Toplevel. */
  CounterT AnzUnifErfGleichgnTeilterme;           /* Erfolge mit den Teiltermen der Gleichungsmenge. */
  CounterT AnzUnifErfGleichgnEigeneTterme;         /* Erfolge mit den eigenen Teiltermen (Gleichg.). */
  CounterT AnzUnifMisserfolgeOrdnungRG;       /* Wegen >-Test nicht gebildete KPs zwischen Regel und Gleichung */
  CounterT AnzUnifMisserfolgeOrdnungGG;       /* Wegen >-Test nicht gebildete KPs zwischen zwei Gleichungen */
  CounterT AnzUnifUeberlappungenGebildet;                    /* Fuer >-Test gebildete Ueberlappungen */
  CounterT AnzUnifUeberflKritTerme;                /* Ueberfluessigerweise gebildete kritische Terme */
  CounterT AnzNonEinsSternKPs;              /* Anzahl der Paare, die dem 1*-Kriterium nicht genuegen */
  CounterT AnzNonNusfuKPs;               /* Anzahl der Ueberlappungen unterhalb von Skolemfunktionen */

  CounterT AnzUnifVers;
  CounterT AnzUnifErf;

  CounterT AnzErfZusfuehrungenZiele;          /* Hack: Nicht statisch, denn Konsulat setzt diese zurueck. */

  CounterT RE_AnzRegelnGesamt;
  CounterT RE_AnzGleichungenGesamt;
  CounterT AnzRegelnAktuell;    /* Lokaler Zaehler. Enthaelt immer die aktuelle Regelzahl im System. */

  CounterT AnzGleichungenAktuell;                      /* lokale Zaehler, die die Ausdehnung */
  CounterT AnzAktMonogleichungen;                      /* der Gleichungsmenge beschreiben */
  CounterT AnzMonogleichungen;                         /* Insgesamt aufgetretene Monogleichungen */
} StatisticsT;

extern StatisticsT Stat;
/* --------------------------------------------------------------------------------------------------------*/
/* ------------ STATISTICS --------------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------------------------------------*/

#define DM_STATISTIK normal_power_comp(1,0,0)

#if DM_STATISTIK
#  define if_sst(Action)  Action
#  define if_sstF(Action) (Action,0)
#else
#  define if_sst(Action)
#  define if_sstF(Action) (void) 0
#endif

#define BO_STATISTIK normal_power_comp(1,0,0)

#if BO_STATISTIK
#define IncAnzEingefRegeln()        (Stat.AnzEingefRegeln++)         
#define IncAnzEntfernterRegeln()    (Stat.AnzEntfernterRegeln++)             
#define IncAnzSchrumpfPfadExp()     (Stat.AnzSchrumpfPfadExp++)
#define IncAnzPfadschrumpfungen()   (Stat.AnzPfadschrumpfungen++)
#else   
#define IncAnzEingefRegeln()           /* nix tun */
#define IncAnzEntfernterRegeln()       /* nix tun */
#define IncAnzSchrumpfPfadExp()        /* nix tun */
#define IncAnzPfadschrumpfungen()      /* nix tun */
#endif

#define IncAnzErfSubsumptionenAlt()   (Stat.AnzGleichungenInterRed++)
#define IncAnzGleichungenInterRed()   (Stat.AnzErfSubsumptionenAlt++)  
#define IncAnzRegWgLinksRedWeg()      (Stat.AnzRegWgLinksRedWeg++)  
#define IncAnzRegRechtsRed()          (Stat.AnzRegRechtsRed++) 

#define KPIR_STATISTIK normal_power_comp(0,0,0)

#if KPIR_STATISTIK

#define IncAnzKPIRBehandelt()        (Stat.AnzKPIRBehandelt++)
#define IncAnzKPIRReduziert()        (Stat.AnzKPIRReduziert++)
#define IncAnzKPIRZusammengefuehrt() (Stat.AnzKPIRZusammengefuehrt++)
#define IncAnzKPIRSubsummiert()      (Stat.AnzKPIRSubsummiert++)

#else

#define IncAnzKPIRBehandelt()        /* nix tun */
#define IncAnzKPIRReduziert()        /* nix tun */
#define IncAnzKPIRZusammengefuehrt() /* nix tun */
#define IncAnzKPIRSubsummiert()      /* nix tun */

#endif

#define KPV_IncAnzCPs() (Stat.KPV_AnzCPs++)

#define KPV_IncAnzKPGesamt() (Stat.KPV_AnzKPGesamt++)
#define KPV_IncAnzBehandelterKPsVervollst() (Stat.KPV_AnzKPBehandeltInVervollst++)
#define KPV_IncAnzKPAusSpezifikation() (Stat.KPV_AnzKPAusSpezifikation++)

#define GetKPV_AnzKPAusSpezifikation() (Stat.KPV_AnzKPAusSpezifikation)
#define GetKPV_AnzKPGesamt() (Stat.KPV_AnzKPGesamt)
#define SUE_STATISTIK normal_power_comp(1,1,1)
/* Alternativen: 0 fuer aus, 1 fuer an */

#define SetKPV_AnzCritGoalsActual(x) (Stat.KPV_AnzCritGoalsActual = x)
#define KPV_IncCritGoalsGesamt() (Stat.KPV_AnzCritGoalsGesamt++)
#define KPV_IncMaxCritGoals()    (Stat.KPV_AnzMaxCritGoals++)   
#define KPV_IncCritGoalsActual() (Stat.KPV_AnzCritGoalsActual++)
#define KPV_DecCritGoalsActual() (Stat.KPV_AnzCritGoalsActual--)
#define KPV_IncIRGleich()        (Stat.KPV_AnzIRGleich++)           
#define KPV_IncIRReduziert()     (Stat.KPV_AnzIRReduziert++)    
#define KPV_IncIRWMord()         (Stat.KPV_AnzIRWMord++)        

#define KPV_MaximizeCritGoals()  MaximierenLLong(&Stat.KPV_AnzMaxCritGoals, Stat.KPV_AnzCritGoalsActual)

#define KPV_IncAktivierterFakten() (Stat.KPV_AnzAktivierterFakten++)
#define KPV_IncAktivierterG()      (Stat.KPV_AnzAktivierterG++)
#define KPV_IncAktivierterGFifo()      (Stat.KPV_AnzAktivierterGFifo++)
#define KPV_IncAktivierterGHeu()      (Stat.KPV_AnzAktivierterGHeu++)
#define KPV_IncAktivierterRE()    (Stat.KPV_AnzAktivierterRE++)
#define KPV_IncAktivierterREFifo()    (Stat.KPV_AnzAktivierterREFifo++)
#define KPV_IncAktivierterREHeu()    (Stat.KPV_AnzAktivierterREHeu++)

#define GetKPV_AnzAktivierterG() (Stat.KPV_AnzAktivierterG)
#define GetKPV_AnzAktivierterRE()(Stat.KPV_AnzAktivierterRE)
#define GetKPV_AnzAktivierterGFifo() (Stat.KPV_AnzAktivierterGFifo)
#define GetKPV_AnzAktivierterREFifo()(Stat.KPV_AnzAktivierterREFifo)
#define GetKPV_AnzAktivierterFakten()  (Stat.KPV_AnzAktivierterFakten)

#if SUE_STATISTIK
#define KPV_IncAnzKPIROpfer() (Stat.KPV_AnzKPIROpfer++)

#define KPV_IncAnzKPdirektZusammengefuehrt() (Stat.KPV_AnzKPdirektZusammengefuehrt++) 
#define KPV_IncAnzKPdirektSubsummiert() (Stat.KPV_AnzKPdirektSubsummiert++) 
#define KPV_IncAnzKPdirektPermSubsummiert() (Stat.KPV_AnzKPdirektPermSubsummiert++) 
#define KPV_IncAnzKPInserted() (Stat.KPV_AnzKPInserted++)

#define KPV_IncAnzKPZurueckgestellt() (Stat.KPV_AnzKPZurueckgestellt++)
#define KPV_IncAnzKPspaeterZusammengefuehrt() (Stat.KPV_AnzKPspaeterZusammengefuehrt++)
#define KPV_IncAnzKPspaeterSubsummiert() (Stat.KPV_AnzKPspaeterSubsummiert++)
#define KPV_IncAnzKPspaeterPermSubsummiert() (Stat.KPV_AnzKPspaeterPermSubsummiert++)
#define KPV_IncAnzKPZuRE() (Stat.KPV_AnzKPZuRE++)
#define KPV_IncAnzKPWeggeworfen() (Stat.KPV_AnzKPWeggeworfen++)

#else

#define KPV_IncAnzKPIROpfer()                                /* Nix Tun */

#define KPV_IncAnzKPdirektZusammengefuehrt()                 /* Nix Tun */
#define KPV_IncAnzKPdirektSubsummiert()                      /* Nix Tun */
#define KPV_IncAnzKPdirektPermSubsummiert()                  /* Nix Tun */
#define KPV_IncAnzKPInserted()                               /* Nix Tun */

#define KPV_IncAnzKPZurueckgestellt()                        /* Nix Tun */
#define KPV_IncAnzKPspaeterZusammengefuehrt()                /* Nix Tun */
#define KPV_IncAnzKPspaeterSubsummiert()                     /* Nix Tun */
#define KPV_IncAnzKPspaeterPermSubsummiert()                 /* Nix Tun */
#define KPV_IncAnzKPZuRE()                                   /* Nix Tun */
#define KPV_IncAnzKPWeggeworfen()                            /* dito    */

#endif

/* --------------------------------------------------------------------------------------------------------*/
#define KPH_STATISTIK normal_power_comp(1,0,0)
  
#if KPH_STATISTIK
#define IncAnzKPSpaeterWaisenmord() (Stat.AnzKPSpaeterWaisenmord++)
#define IncAnzKPFrueherWaisenmord() (Stat.AnzKPFrueherWaisenmord++)

#define IncAnzKPIRBetrachtet() (Stat.AnzKPIRBetrachtet++)
#define IncAnzKPIRWaisenmord() (Stat.AnzKPIRWaisenmord++)
#define IncAnzKPIRGeloescht() (Stat.AnzKPIRGeloescht++)
#define IncAnzKPIRVeraendert() (Stat.AnzKPIRVeraendert++)
#define IncAnzKPIRVorgezogen() (Stat.AnzKPIRVorgezogen++)
#define IncAnzKPIRZurueckgeschoben() (Stat.AnzKPIRZurueckgeschoben++)

#define IncAnzKPEingefuegt()(                                                        \
  Stat.AnzKPEingefuegt++,                                                            \
  Stat.AnzKPBlaeh = ((Stat.AnzKPEingefuegt - Stat.AnzKPGeloescht) > Stat.AnzKPBlaeh) \
                  ? (Stat.AnzKPEingefuegt - Stat.AnzKPGeloescht)                     \
                  : Stat.AnzKPBlaeh                                                  \
)
#define IncAnzKPEntfernt()   (Stat.AnzKPGeloescht++)

#else

#define IncAnzKPSpaeterWaisenmord()                     /* Nix Tun */
#define IncAnzKPFrueherWaisenmord()                     /* Nix Tun */

#define IncAnzKPIRBetrachtet()                          /* Nix Tun */
#define IncAnzKPIRWaisenmord()                          /* Nix Tun */
#define IncAnzKPIRGeloescht()                           /* Nix Tun */
#define IncAnzKPIRVeraendert()                          /* Nix Tun */
#define IncAnzKPIRVorgezogen()                          /* Nix Tun */
#define IncAnzKPIRZurueckgeschoben()                    /* Nix Tun */

#define IncAnzKPEingefuegt()                            /* Nix Tun */
#define IncAnzKPEntfernt()                              /* Nix Tun */

#endif /* KPH_STATISTIK */


#define HA_STATISTIK normal_power_comp(0,0,0)

#if HA_STATISTIK
# define INC_AnzKey1Gleich    {Stat.HA_AnzKey1Gleich++;}
# define INC_AnzKey2Gleich    {Stat.HA_AnzKey2Gleich++;}
# define INC_AnzBucketGleich  {Stat.HA_AnzBucketGleich++;}
#else
# define INC_AnzKey1Gleich
# define INC_AnzKey2Gleich
# define INC_AnzBucketGleich
#endif /* HA_STATISTIK */

#define MNF_STATISTIK normal_power_comp(1,0,0)

#if MNF_STATISTIK
# define MNF_IncAnzReduktionen      (Stat.MNF_AnzReduktionen++)
# define MNF_IncAnzCalls            (Stat.MNF_AnzCalls++)
# define MNF_IncAnzTermGleich       (Stat.MNF_AnzTermGleich++)
# define MNF_IncAnzTermGleichCol    (Stat.MNF_AnzTermGleichCol++)
# define MNF_IncAnzTermGleichOpp    (Stat.MNF_AnzTermGleichOpp++)
# define MNF_IncAbbruchMaxSize      (Stat.MNF_AbbruchMaxSize++)
# define MNF_IncAnzDirektZusammenfuehrbar (Stat.MNF_AnzDirektZus++)
# define MNF_IncAnzCallsHAAddData   (Stat.MNF_AnzCallsHAAddData++)
# define MNF_IncAnzEQUp             (Stat.MNF_AnzEQUp++)
# define MNF_IncAnzAntiR            (Stat.MNF_AnzAntiR++)
# define MNF_IncAnzMatchVersucheE   (Stat.MNF_AnzMatchVersucheE++)
# define MNF_IncAnzMatchVersucheR   (Stat.MNF_AnzMatchVersucheR++)
# define MNF_MAximizeMaxEQUp(x)     (Stat.MNF_MaxEQUp = max (Stat.MNF_MaxEQUp,(x)))
# define MNF_IncSumAnzNachfolger(x) (Stat.MNF_SumAnzNachfolger += (x))
# define MNF_MaximizeMaxNachfolger(x)(MaximierenLLong (&Stat.MNF_MaxNachfolger, (x)))
#else
# define MNF_IncAnzReduktionen
# define MNF_IncAnzCalls
# define MNF_IncAnzTermGleich
# define MNF_IncAnzTermGleichCol
# define MNF_IncAnzTermGleichOpp
# define MNF_IncAbbruchMaxSize
# define MNF_IncAnzDirektZusammenfuehrbar
# define MNF_IncAnzCallsHAAddData
# define MNF_IncAnzEQUp
# define MNF_IncAnzAntiR
# define MNF_IncAnzMatchVersucheE
# define MNF_IncAnzMatchVersucheR
# define MNF_MAximizeMaxEQUp(x)     
# define MNF_IncSumAnzNachfolger(x) 
# define MNF_MaximizeMaxNachfolger(x)
#endif /* MNF_STATISTIK */

#define MO_STATISTIK normal_power_comp(1,0,0)     /* Misserfolge wg. Ordnung zaehlen. Alternativen: 0 fuer aus, 1 fuer an */
#if MO_STATISTIK
#define IncAnzRedMisserfolgeOrdnung() (Stat.AnzRedMisserfolgeOrdnung++)
#else
#define IncAnzRedMisserfolgeOrdnung()  /* nix tun */
#endif

#define NFB_STATISTIK normal_power_comp(1,0,0)
/* Alternativen: 0 fuer aus, 1 fuer an */

#if NFB_STATISTIK

#define IncAnzRedErfRegeln()       (Stat.AnzRedErfRegeln++)           
#define IncAnzRedErfGleichgn()     (Stat.AnzRedErfGleichgn++)
#define IncAnzRedErf(p)            ((p) ? Stat.AnzRedErfRegeln++ : Stat.AnzRedErfGleichgn++)
#define IncAnzRedVersMitRegeln()   (Stat.AnzRedVersMitRegeln++)                
#define IncAnzRedVersMitGleichgn() (Stat.AnzRedVersMitGleichgn++)            
#define IncAnzRedVers(p)           ((p) ? Stat.AnzRedVersMitRegeln++ : Stat.AnzRedVersMitGleichgn++)

#else

#define IncAnzRedErfRegeln()                ((void) 0) /* nix tun */
#define IncAnzRedErfGleichgn()              ((void) 0) /* nix tun */ 
#define IncAnzRedErf(p)                     ((void) 0) /* nix tun */ 
#define IncAnzRedVersMitRegeln()            ((void) 0) /* nix tun */ 
#define IncAnzRedVersMitGleichgn()          ((void) 0) /* nix tun */ 
#define IncAnzRedVers(p)                    ((void) 0) /* nix tun */ 

#endif

#define IncVorwaertsMatchR() (Stat.VorwaertsMatchR++)
#define IncVorwaertsMatchE() (Stat.VorwaertsMatchE++)
#define IncVorwaertsMatchS() (Stat.VorwaertsMatchS++)

#define U1_STATISTIK normal_power_comp(0,0,0)
/* Alternativen: 0 fuer aus, 1 fuer an */

#if U1_STATISTIK

#define IncAnzKapurOpfer() (Stat.AnzKPOpferKapur++)
#define IncAnzKPRR() (Stat.AnzKPRR++)
#define IncAnzKPGG() (Stat.AnzKPGG++)
#define IncAnzKPRG() (Stat.AnzKPRG++)
#define IncAnzUnifVersRegelnToplevel() (Stat.AnzUnifVersRegelnToplevel++)  
#define IncAnzUnifVersRegelnTeilterme() (Stat.AnzUnifVersRegelnTeilterme++)  
#define IncAnzUnifVersRegelnEigeneTterme() (Stat.AnzUnifVersRegelnEigeneTterme++)  
#define IncAnzUnifErfRegelnToplevel() (Stat.AnzUnifErfRegelnToplevel++)
#define IncAnzUnifErfRegelnTeilterme() (Stat.AnzUnifErfRegelnTeilterme++)
#define IncAnzUnifErfRegelnEigeneTterme() (Stat.AnzUnifErfRegelnEigeneTterme++)
#define IncAnzUnifVersGleichgnToplevel() (Stat.AnzUnifVersGleichgnToplevel++)                
#define IncAnzUnifVersGleichgnTeilterme() (Stat.AnzUnifVersGleichgnTeilterme++)
#define IncAnzUnifVersGleichgnEigeneTterme() (Stat.AnzUnifVersGleichgnEigeneTterme++)  
#define IncAnzUnifErfGleichgnToplevel() (Stat.AnzUnifErfGleichgnToplevel++)
#define IncAnzUnifErfGleichgnTeilterme() (Stat.AnzUnifErfGleichgnTeilterme++)
#define IncAnzUnifErfGleichgnEigeneTterme() (Stat.AnzUnifErfGleichgnEigeneTterme++)
#define IncAnzUnifMisserfolgeOrdnungRG() (Stat.AnzUnifMisserfolgeOrdnungRG++)
#define IncAnzUnifMisserfolgeOrdnungGG() (Stat.AnzUnifMisserfolgeOrdnungGG++)
#define IncAnzUnifUeberlappungenGebildet() (Stat.AnzUnifUeberlappungenGebildet++)
#define IncAnzUnifUeberflKritTerme() (Stat.AnzUnifUeberflKritTerme++)
#define IncAnzNonEinsSternKPs() (Stat.AnzNonEinsSternKPs++)
#define IncAnzNonNusfuKPs() (Stat.AnzNonNusfuKPs++)

#else

#define IncAnzKapurOpfer()                                                                            /* Nix Tun */
#define IncAnzKPRR()                                                                                  /* Nix Tun */
#define IncAnzKPGG()                                                                                  /* Nix Tun */
#define IncAnzKPRG()                                                                                  /* Nix Tun */
#define IncAnzUnifVersRegelnToplevel()                                                                /* nix tun */
#define IncAnzUnifVersRegelnTeilterme()                                                               /* nix tun */
#define IncAnzUnifVersRegelnEigeneTterme()                                                            /* nix tun */
#define IncAnzUnifErfRegelnToplevel()                                                                 /* nix tun */
#define IncAnzUnifErfRegelnTeilterme()                                                                /* nix tun */
#define IncAnzUnifErfRegelnEigeneTterme()                                                             /* nix tun */
#define IncAnzUnifVersGleichgnToplevel()                                                              /* nix tun */
#define IncAnzUnifVersGleichgnTeilterme()                                                             /* nix tun */
#define IncAnzUnifVersGleichgnEigeneTterme()                                                          /* nix tun */
#define IncAnzUnifErfGleichgnToplevel()                                                               /* nix tun */
#define IncAnzUnifErfGleichgnTeilterme()                                                              /* nix tun */
#define IncAnzUnifErfGleichgnEigeneTterme()                                                           /* nix tun */
#define IncAnzUnifMisserfolgeOrdnungRG() 0                                                            /* nix tun */
#define IncAnzUnifMisserfolgeOrdnungGG() 0                                                            /* nix tun */
#define IncAnzUnifUeberlappungenGebildet()                                                            /* nix tun */
#define IncAnzUnifUeberflKritTerme()                                                                  /* nix tun */
#define IncAnzNonEinsSternKPs()                                                                       /* nix tun */
#define IncAnzNonNusfuKPs() (void)0                                                                   /* nix tun */

#endif

#define U2_STATISTIK normal_power_comp(0,0,0)                               /* Alternativen: 0 fuer aus, 1 fuer an */

#if U2_STATISTIK

#define IncAnzUnifVers() (Stat.AnzUnifVers++)  
#define IncAnzUnifErf() (Stat.AnzUnifErf++)

#else

#define IncAnzUnifVers()                                                                              /* Nix Tun */
#define IncAnzUnifErf()                                                                               /* Nix Tun */

#endif

#define IncAnzErfZusammenfuehrungenZiele   (Stat.AnzErfZusfuehrungenZiele++)
#define GetAnzErfZusfuehrungenZiele()      (Stat.AnzErfZusfuehrungenZiele)
#define SetAnzErfZusfuehrungenZiele(x)     (Stat.AnzErfZusfuehrungenZiele=x)                               /* Hack: Statistik in Zielverwaltung bereinigen */

#define GetMNFZ_MaxCountMNFSet() (Stat.MNFZ_MaxCountMNFSet)
#define SetMNFZ_MaxCountMNFSet(x) (Stat.MNFZ_MaxCountMNFSet = x)

#define RE_AnzRegelnGesamt() (Stat.RE_AnzRegelnGesamt) 
#define RE_AnzGleichungenGesamt() (Stat.RE_AnzGleichungenGesamt)  
#define RE_IncAnzRegelnGesamt() (Stat.RE_AnzRegelnGesamt++) 
#define RE_IncAnzGleichungenGesamt() (Stat.RE_AnzGleichungenGesamt++)  
#define RE_IncRegelnAktuell() (Stat.AnzRegelnAktuell++)
#define RE_DecRegelnAktuell() (Stat.AnzRegelnAktuell--)

#define IncGleichungenAktuell() (Stat.AnzGleichungenAktuell++)
#define IncAktMonogleichungen() (Stat.AnzAktMonogleichungen++)
#define IncMonogleichungen()    (Stat.AnzMonogleichungen++)   
#define DecGleichungenAktuell() (Stat.AnzGleichungenAktuell--)
#define DecAktMonogleichungen() (Stat.AnzAktMonogleichungen--)
#define DecMonogleichungen()    (Stat.AnzMonogleichungen--)   



#endif
