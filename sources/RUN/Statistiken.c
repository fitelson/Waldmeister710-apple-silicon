/* -------------------------------------------------------*/
/* ------------- Includes --------------------------------*/
/* -------------------------------------------------------*/

#include "antiwarnings.h"
#if defined (SUNOS5)
#include <fcntl.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/procfs.h>
#endif
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/utsname.h>

#include "compiler-extra.h"
#include "Ausgaben.h"
#include "DSBaumAusgaben.h"
#include "DSBaumKnoten.h"
#include "DSBaumOperationen.h"                                                   /* wegen Ausdehnung Variablenbank */
#include "FehlerBehandlung.h"
#include "general.h"
#include "Interreduktion.h"
#include "KPVerwaltung.h"
#include "MatchOperationen.h"                                                    /* wegen Ausdehnung Variablenbank */
#include "MNF.h"
#include "NewClassification.h"
#include "NFBildung.h"                                                      /* wegen Ausdehnung des Reduktionspfades */
#include "Parameter.h"
#include "parser.h"
#include "RUndEVerwaltung.h"
#include "Signale.h"
#include "Statistiken.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "TermIteration.h"
#include "TermVerfolgung.h"
#include "Unifikation1.h"                                                 /* wegen Ausdehnung Variablenbank, Stack */
#include "Unifikation2.h"
#include "ZeitMessung.h"
#include "Zielverwaltung.h"
#include "version.h"
#include "WaldmeisterII.h"

extern char **environ;



StatisticsT Stat;



/* ######################################################################################################## */
/* ######################################################################################################## */
/* ######################################################################################################## */
/* ######################################################################################################## */
/* ######################################################################################################## */

/* -------------------------------------------------------*/
/* ------------- Statistik-Ausgaben ----------------------*/
/* -------------------------------------------------------*/

void IO_StatistikModusAufLatexSetzen(void);

void IO_StatistikModusAufNormalSetzen(void);

void IO_DruckeStatistikZeile(const char *str, CounterT Datum);

void IO_DruckeStatistikZeileEingerueckt(const char *str, unsigned long Datum, int einruecktiefe);

void IO_DruckeStatistikLeerZeile(void);

void IO_DruckeStatistikZeileAnteil(const char *str, unsigned long int Zaehler, unsigned long int Nenner);
/* Gibt den String und dahinter das Datum aus, falls der Nenner nicht null ist.*/

/* ######################################################################################################## */
/* ######################################################################################################## */

static BOOLEAN LatexAusgabeAktiviert = FALSE;


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* ++++++++++++++ Statistik ++++++++++++++++++++++++++++++*/
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static void DruckeBeispielTeX(void)
{
  char *Spezifikationsname = IO_Spezifikationsname();
  if (Spezifikationsname != NULL){
   if (LatexAusgabeAktiviert){
      char *Dateiname_ = strrchr(Spezifikationsname,'/'),
           *Dateiname = Dateiname_ ? Dateiname_+1 : Spezifikationsname;
      printf("%-10s ", Dateiname);
   }
  }
}  

static int Stelligkeit(unsigned long int zahl)
{
   int stellenzahl = 0;
   while (zahl > 0){
      zahl = zahl / 10;
      stellenzahl++;
   }
   return stellenzahl;
}

static int Platzverbrauch(unsigned long int zahl)
{
   int st = Stelligkeit(zahl);

   return st + ((st-1)/3);
}

static int Einruecktiefe(int bisherigeTiefe, unsigned long int maxEintrag)
/* Berechnet die Einruecktiefe, die fuer die folgenden Eintraege noetig ist.
   Dabei ist maxEintrag der groesste der folgenden Eintraege.
*/
{
   return (bisherigeTiefe + Platzverbrauch(maxEintrag) +2);
}

static void DZFR(unsigned long int zahl, int *stellen, int stellenzahl)
{
  if (zahl == 0){
    *stellen = 0;
    return;
  }
  else if ((zahl / 10) == 0){
    printf("%lu", zahl);
    *stellen = 1;
  }
  else{
    DZFR(zahl/10, stellen, stellenzahl);
    /*
    if ( (((stellenzahl - *stellen)/3)*3 == stellenzahl-*stellen) && ((stellenzahl-*stellen) > 0))
    */
    if (((stellenzahl - *stellen) % 3 == 0) && ((stellenzahl-*stellen) > 0))
      printf(".");
    /*
      printf("%lu", zahl - (zahl/10)*10);
      */
    printf("%lu", zahl % 10);
    (*stellen)++;
  }
}
      

static void DruckeZahlFormatiert(unsigned long int zahl, int platz)
/* Gibt die Zahl zahl aus, setzt nach jeder dritten Ziffer (von hinten) einen Punkt,
   und fuellt die Ausgabe auf insgesamt platz Zeichen von links mit Leerzeichen auf.*/
{
  int stellenzahl = Stelligkeit(zahl), Leerzeichen, i;
  Leerzeichen = platz - Platzverbrauch(zahl);
  for (i = 0; i < Leerzeichen; i++)
    printf(" ");
  i = 0;
  DZFR(zahl,&i, stellenzahl);
}


const int RechterRandDatum = 14;


/* -------------------------------------------------------*/
/* ------------- Statistik-Ausgaben ----------------------*/
/* -------------------------------------------------------*/

void IO_StatistikModusAufLatexSetzen(void)
{
   LatexAusgabeAktiviert = TRUE;
}

void IO_StatistikModusAufNormalSetzen(void)
{
   LatexAusgabeAktiviert = FALSE;
}

void IO_DruckeKurzUeberschrift(const char *text, char zeichen)
{
   if (!LatexAusgabeAktiviert){
      short i, rahmen;
      
      IO_LF();
      rahmen = 5; /*((IO_Ueberschriftenbreite - strlen(text) - 2) / 2);*/
      for (i=0; i<rahmen; i++){
         putc (zeichen, stdout);
      }
      putc (' ', stdout);
      printf(text);
      putc (' ', stdout);
      for (i=rahmen+strlen(text)+2; i<IO_Ueberschriftenbreite; i++){
         putc (zeichen, stdout);
      }
      IO_LF();
      IO_LF();
   }
}  


   
void IO_DruckeStatistikZeile (const char *str, CounterT Datum)
/* Gibt den String und dahinter das Datum aus, falls Datum nicht null ist.*/
{
   if (LatexAusgabeAktiviert){
     printf("%10llu ", Datum);
     /*
      if (Datum == 0)
         printf(" 0 & ");
      else{
         int stellenzahl = Stelligkeit(Datum), i;
         DZFR(Datum, &i, stellenzahl);
         printf(" & ");
      }
     */
   }
   else if (Datum){
      DruckeZahlFormatiert(Datum, RechterRandDatum);
      printf(" %s\n", str);
  }
}

void IO_DruckeStatistikZeileEingerueckt(const char *str, unsigned long Datum, int Einruecktiefe)
/* Druckt die Statistik-Zeile um Einruecktiefe 'Stellen' eingerueckt aus.
   Einruecktiefe gibt die Position des rechten Zeichens des Datums an.
*/
{
   if (LatexAusgabeAktiviert){
      if (Datum == 0)
         printf(" 0 & ");
      else{
         int stellenzahl = Stelligkeit(Datum), i;
         DZFR(Datum, &i, stellenzahl);
         printf(" & ");
      }
   }
   else if (Datum){
      DruckeZahlFormatiert(Datum, RechterRandDatum + Einruecktiefe);
      printf(" %s\n", str);
   }
      
}

void IO_DruckeStatistikLeerZeile(void)
{
   if (!LatexAusgabeAktiviert)  printf("\n");
}

void IO_DruckeStatistikZeileAnteil(const char *str,unsigned long int Zaehler, unsigned long int Nenner)
/* Gibt den String und dahinter das Datum aus, falls der Nenner nicht null ist.*/
{
  if (LatexAusgabeAktiviert){
    if ((Zaehler == 0) || (Nenner == 0))
      printf(" 0 & ");
    else{
      double Quotient = 100.0*(double)Zaehler/(double)Nenner;
      printf("%3.1f & \n", Quotient);
    }
  }
  else if (Nenner) {
    double Quotient = 100.0*(double)Zaehler/(double)Nenner;
    unsigned int Vorkomma = (Quotient = ((double)(unsigned int)(10.0*Quotient+0.5))/10.0),
                 Nachkomma = (unsigned int)(10.0*(Quotient-(double)Vorkomma)+0.5),
                 i = RechterRandDatum-LaengeNatZahl(Vorkomma)-LaengeNatZahl(Nachkomma)-1;
    while (i--)
      printf(" ");
    printf("%d,%d %s\n",Vorkomma,Nachkomma,str);
  }
}



/* ######################################################################################################## */
/* ######################################################################################################## */
/* ######################################################################################################## */
/* ######################################################################################################## */
/* ######################################################################################################## */
/* ######################################################################################################## */

extern unsigned long int BK_AnzEinzelnAllokierterNachfolgerArrays;



extern BOOLEAN MNF_Initialised;

extern unsigned long int AnzRegelnAktuell;
extern unsigned long int AnzGleichungenAktuell;
extern unsigned long int AnzMonogleichungen;
extern unsigned long int AnzAktMonogleichungen;


extern SV_SpeicherManagerT *ManagerListe;

static void DruckeSpeicherManagerStatistik(int, const char*, unsigned long, unsigned long, unsigned long,
  unsigned long, unsigned long, unsigned long) MAYBE_UNUSEDFUNC;
static void DruckeSpeicherManagerStatistik(int Grad, const char *Name, unsigned long Blockgroesse, 
  unsigned long ElementGroesse, unsigned long Anforderungen, unsigned long Freigaben,
  unsigned long InsgesamtAngelegteBloecke, unsigned long MaximalGleichzeitigBelegteElemente )
/* Grad: 0 - Ausgabe nur, wenn Manager benutzt wurde und Diskrepanzen
         1 - Ausgabe, wenn Manager benutzt wurde, unabhaengig von Diskrepanzen
*/
{
  BOOLEAN Ausgeben = FALSE;
  
  switch (Grad){
  case 0:
    Ausgeben = ((Anforderungen+Freigaben>0) && (Anforderungen != Freigaben));
    break;
  case 1:
    Ausgeben = (Anforderungen+Freigaben>0);
    break;
  default:
    our_error("Falscher Ausgabe-Grad in IO_DruckeSpeicherManagerStatistik\n");
    break;
  }
  if (Ausgeben){
    IO_DruckeKurzUeberschrift(Name, '+');
    IO_DruckeStatistikZeile("Block size", Blockgroesse);
    IO_DruckeStatistikZeile("Element size", ElementGroesse);
    IO_DruckeStatistikZeile("Total number of allocations",Anforderungen );
    IO_DruckeStatistikZeile("Total number of deallocations",Freigaben );
    IO_DruckeStatistikZeile("Total number of allocated blocks",InsgesamtAngelegteBloecke );
    IO_DruckeStatistikZeile("Max. number of elements used at the same time",MaximalGleichzeitigBelegteElemente );
  }
}


static void DruckeNestedSpeicherManagerStatistik(const char*, unsigned long, unsigned long, unsigned long,
  BOOLEAN, unsigned long, unsigned long) MAYBE_UNUSEDFUNC;
static void DruckeNestedSpeicherManagerStatistik(const char *Name, unsigned long Blockgroesse,
  unsigned long ElementGroesse, unsigned long Anforderungen, BOOLEAN Fehlerfall, 
  unsigned long AnzNochBelegterElemente, unsigned long InsgesamtAngelegteBloecke )
{
  char VerlaengerterName[SV_maxLaengeNameNestedManager];
  strcpy(VerlaengerterName,Name);
  strcat(VerlaengerterName," (Nested-Manager)");
  IO_DruckeKurzUeberschrift(VerlaengerterName, '+');
  IO_DruckeStatistikZeile("Block size", Blockgroesse);
  IO_DruckeStatistikZeile("Element size", ElementGroesse);
  IO_DruckeStatistikZeile("Total number of allocations",Anforderungen );
  if (!Fehlerfall)
    IO_DruckeStatistikZeile("Remaining number of allocated elements",AnzNochBelegterElemente );
  IO_DruckeStatistikZeile("Total number of allocated blocks",InsgesamtAngelegteBloecke );
  IO_DruckeStatistikZeile("Max. number of elements usable at the same time",Blockgroesse*InsgesamtAngelegteBloecke);
}


void SV_Statistik(BOOLEAN Fehlerfall MAYBE_UNUSEDPARAM, BOOLEAN NurDiskrepanzen MAYBE_UNUSEDPARAM)   
{                                                                /* Gibt die Statistik der Speicherverwaltung aus. */
  
#if DM_STATISTIK
    BOOLEAN UeberschriftGedruckt = FALSE;
    unsigned long GesamtAnforderungen = 0,
                  GesamtFreigaben = 0,
                  GesamtAngelegteBloecke = 0,
                  GesamtBelegterSpeicher = 0;
                  SV_SpeicherManagerT *runny = ManagerListe;
      
    if (!NurDiskrepanzen) {
      UeberschriftGedruckt = TRUE;
      IO_DruckeUeberschrift("Memory statistics");
    }
      
    while (runny) {
      SV_SpeicherManagerT mbm = *runny;
      if (mbm.Nested) {
        if (Fehlerfall) {
          if (!UeberschriftGedruckt) {
            IO_DruckeKurzUeberschrift("Discrepancies in memory allocation", '*');
            UeberschriftGedruckt = TRUE;
          }
          DruckeNestedSpeicherManagerStatistik(mbm.Name, mbm.block_size, mbm.element_size, 
                                                   mbm.Anforderungen, TRUE, 0, mbm.InsgesamtAngelegteBloecke);
        } else {
          unsigned long int AnzNochBelegterElemente = SV_AnzNochBelegterElemente(mbm);
          GesamtFreigaben += mbm.Anforderungen - AnzNochBelegterElemente;
          /* Hier kann nur festgestellt werden, ob noch nicht freigegebene Elemente in den Bloecken stehen */
          if (AnzNochBelegterElemente || FALSE)
            DruckeNestedSpeicherManagerStatistik(mbm.Name, mbm.block_size, mbm.element_size, mbm.Anforderungen,
                                                 FALSE, AnzNochBelegterElemente, mbm.InsgesamtAngelegteBloecke);
        }
      }
      else {
        int AusgabeGrad = NurDiskrepanzen?0:1;
        GesamtFreigaben += mbm.Freigaben;
        if (!UeberschriftGedruckt && mbm.Anforderungen != mbm.Freigaben) {
          IO_DruckeKurzUeberschrift("Discrepancies in memory allocation", '*');
          UeberschriftGedruckt = TRUE;
        }
        DruckeSpeicherManagerStatistik(AusgabeGrad,mbm.Name, mbm.block_size, mbm.element_size, mbm.Anforderungen,
                                       mbm.Freigaben, mbm.InsgesamtAngelegteBloecke, mbm.MaximalGleichzeitigBelegteElemente);
      }
      GesamtAnforderungen += mbm.Anforderungen;
      GesamtAngelegteBloecke += mbm.InsgesamtAngelegteBloecke;
      GesamtBelegterSpeicher += mbm.InsgesamtAngelegteBloecke * mbm.element_size * mbm.block_size;
      runny = runny->Nachf;
    }
    if (UeberschriftGedruckt) {
      IO_DruckeZeile('+');
      IO_DruckeStatistikZeile("Total number of allocations",GesamtAnforderungen );
      IO_DruckeStatistikZeile("Total number of deallocations",GesamtFreigaben );
      IO_DruckeStatistikZeile("Total number of allocated blocks",GesamtAngelegteBloecke );
      IO_DruckeStatistikZeile("Thereby allocated memory in bytes", GesamtBelegterSpeicher);
    }
#elif !COMP_POWER
    printf("\nMemory statistics not compiled into object module.\n");
    printf("If desired, define constant DM_STATISTIK as 1\n");
    printf("in header file DomaenenMemorium.h.\n\n");
#endif
}



enum Zeitsorten {User,System,Wall}; /* XXX */
extern struct timeval Start[3];
extern struct timeval Dauer[3];
static void DruckeZeitSolo(struct timeval time) /* Neufassung. BL. */
{ 
  long seconds, milliseconds;

  milliseconds = (time.tv_usec + 500) / 1000;
  seconds      = time.tv_sec + milliseconds / 1000;
  milliseconds = milliseconds % 1000;

  printf("%ld.%03ld", seconds, milliseconds);
}

static void DruckeLangzeit(struct timeval time)
{ 
  long seconds, milliseconds, hours, days;

  milliseconds = (time.tv_usec + 500) / 1000;
  seconds      = time.tv_sec + milliseconds / 1000;
  milliseconds = milliseconds % 1000;
  hours        = seconds / 3600;
  days         = hours / 24;
  if (days > 1){
    printf(" - more than %ldd or %ldh", days, hours);
  }
  else if (hours > 1){
    printf(" - more than %ldh", hours);
  }
}


static void ZM_Statistik_standard(void)
{
  IO_DruckeUeberschrift("TIMES (user- and system-time in seconds)");
  DruckeZeitSolo(Dauer[User]);
  printf("s ut ");
  DruckeZeitSolo(Dauer[System]);
  printf("s st (");
  DruckeZeitSolo(Dauer[Wall]);
  printf(" wct)");
  printf("   total time (without initialisation)\n");
}
static void ZM_Statistik_latex(void)
{
  printf(" ");
  DruckeZeitSolo(Dauer[User]);
  printf("s ut ");
  DruckeZeitSolo(Dauer[System]);
  printf("s st (");
  DruckeZeitSolo(Dauer[Wall]);
  printf(" wct");
  DruckeLangzeit(Dauer[Wall]);
  printf(")");
}


/* -------------------------------------------------------*/
/* -------------- Statistiken der einzelnen Module -------*/
/* -------------------------------------------------------*/


static void ST_Statistik_standard(void)
{
  int tiefe1, tiefe2, i;
  char str[100];
  IO_DruckeKurzUeberschrift("Unselected Equations",'+');
  
  IO_DruckeStatistikZeile("Total number of ues",Stat.KPV_AnzKPGesamt);
  tiefe1 = Einruecktiefe(0, Maximum(7, Stat.KPV_AnzKPAusSpezifikation, Stat.KPV_AnzKPIROpfer,
                                       Stat.KPV_AnzKPdirektZusammengefuehrt, Stat.KPV_AnzKPdirektSubsummiert,
                                       Stat.KPV_AnzKPInserted, Stat.KPV_AnzKPWeggeworfen,
                                       Stat.KPV_AnzKPdirektPermSubsummiert));
  IO_DruckeStatistikZeileEingerueckt("of them from Specification",Stat.KPV_AnzKPAusSpezifikation, tiefe1);
  IO_DruckeStatistikZeileEingerueckt("of them from Killed RE", Stat.KPV_AnzKPIROpfer, tiefe1);
  IO_DruckeStatistikZeileEingerueckt("of them from Generation", 
                                     Stat.KPV_AnzKPGesamt-Stat.KPV_AnzKPAusSpezifikation-Stat.KPV_AnzKPIROpfer, tiefe1);
  IO_DruckeStatistikLeerZeile();
  
  IO_DruckeStatistikZeileEingerueckt("of them joined immediately",Stat.KPV_AnzKPdirektZusammengefuehrt, tiefe1);
  IO_DruckeStatistikZeileEingerueckt("of them subsumed immediately", Stat.KPV_AnzKPdirektSubsummiert, tiefe1);
  IO_DruckeStatistikZeileEingerueckt("of them perm-subsumed immediately", Stat.KPV_AnzKPdirektPermSubsummiert, tiefe1);
  IO_DruckeStatistikZeileEingerueckt("of them thrown away", Stat.KPV_AnzKPWeggeworfen, tiefe1);
  IO_DruckeStatistikZeileEingerueckt("of them inserted into PQ", Stat.KPV_AnzKPInserted, tiefe1);
  IO_DruckeStatistikLeerZeile();
  IO_DruckeStatistikZeileEingerueckt("ues selected",Stat.KPV_AnzKPBehandeltInVervollst, tiefe1);
  
  tiefe2 = Einruecktiefe(tiefe1, Maximum(5, Stat.KPV_AnzKPspaeterZusammengefuehrt, Stat.KPV_AnzKPZuRE,
                                            Stat.KPV_AnzKPZurueckgestellt, Stat.KPV_AnzKPspaeterSubsummiert, 
                                            Stat.KPV_AnzKPspaeterPermSubsummiert));
  
  IO_DruckeStatistikZeileEingerueckt("of them not orientable, postponed", Stat.KPV_AnzKPZurueckgestellt, tiefe1);
  IO_DruckeStatistikZeileEingerueckt("of them joined", Stat.KPV_AnzKPspaeterZusammengefuehrt, tiefe2);
  IO_DruckeStatistikZeileEingerueckt("of them subsumed (forward subsumption)", Stat.KPV_AnzKPspaeterSubsummiert, tiefe2);
  IO_DruckeStatistikZeileEingerueckt("of them perm-subsumed (forward subsumption)", Stat.KPV_AnzKPspaeterPermSubsummiert, tiefe2);
  IO_DruckeStatistikZeileEingerueckt("of them made RE", Stat.KPV_AnzKPZuRE, tiefe2);
#if KPH_STATISTIK
  {
    int tiefe1, tiefe2, tiefe3;
    int AnzGueltigeKPsInMenge = KPH_LebendeKPsZaehlen();

    IO_DruckeKurzUeberschrift("Priority-Queue: Regular heap", '+');

    IO_DruckeStatistikZeile("ues inserted into PQ", Stat.AnzKPEingefuegt);
    IO_DruckeStatistikZeile("ues removed from PQ", Stat.AnzKPGeloescht);
    IO_DruckeStatistikZeileEingerueckt("of them because of early orphan murder", Stat.AnzKPFrueherWaisenmord, 
                                       Einruecktiefe(0, Maximum(2,Stat. AnzKPFrueherWaisenmord, Stat.AnzKPSpaeterWaisenmord)));
    IO_DruckeStatistikZeileEingerueckt("of them because of late orphan murder", Stat.AnzKPSpaeterWaisenmord, 
                                       Einruecktiefe(0, Maximum(2, Stat.AnzKPFrueherWaisenmord, Stat.AnzKPSpaeterWaisenmord)));
    IO_DruckeStatistikLeerZeile();
    IO_DruckeStatistikZeile("ues currently in PQ", Stat.AnzKPEingefuegt - Stat.AnzKPGeloescht);
    IO_DruckeStatistikZeileEingerueckt("of them still valid", AnzGueltigeKPsInMenge, Einruecktiefe(0, AnzGueltigeKPsInMenge));
    IO_DruckeStatistikLeerZeile();
    IO_DruckeStatistikZeile("Max. No. of ues in PQ",Stat. AnzKPBlaeh);
    
    IO_DruckeStatistikLeerZeile();
    IO_DruckeStatistikZeile("ues touched during IRP", Stat.AnzKPIRBetrachtet);
    tiefe1 = Einruecktiefe(0, Maximum(2, Stat.AnzKPIRWaisenmord, Stat.AnzKPIRBetrachtet-Stat.AnzKPIRWaisenmord));
    tiefe2 = Einruecktiefe(tiefe1, Maximum(2, Stat.AnzKPIRGeloescht, Stat.AnzKPIRVeraendert));
    tiefe3 = Einruecktiefe(tiefe2, Maximum(2, Stat.AnzKPIRVorgezogen, Stat.AnzKPIRZurueckgeschoben));
    
    IO_DruckeStatistikZeileEingerueckt("of them removed as orphans", Stat.AnzKPIRWaisenmord, tiefe1);
    IO_DruckeStatistikZeileEingerueckt("of them reprocessed", Stat.AnzKPIRBetrachtet-Stat.AnzKPIRWaisenmord, tiefe1);
    IO_DruckeStatistikZeileEingerueckt("of them removed", Stat.AnzKPIRGeloescht, tiefe2);
    IO_DruckeStatistikZeileEingerueckt("of them changed", Stat.AnzKPIRVeraendert, tiefe2);
    IO_DruckeStatistikZeileEingerueckt("of them prefered", Stat.AnzKPIRVorgezogen, tiefe3);
    IO_DruckeStatistikZeileEingerueckt("of them defered", Stat.AnzKPIRZurueckgeschoben, tiefe3);
  }
#endif

  IO_DruckeKurzUeberschrift("Classification", '+');
  if (ClasCriteria) {
    for (i = 0; ClasCriteria[i].criterion != Crit_LastMarker; i++){
      strcpy (str, get_criterion (ClasCriteria[i].criterion));
      strcat (str, " during CLASSIFY");
      IO_DruckeStatistikZeile(str, ClasCriteria[i].fullfilled);
    }
  }
  if (ReClasCriteria) {
    for (i = 0; ReClasCriteria[i].criterion != Crit_LastMarker; i++){
      strcpy (str, get_criterion (ReClasCriteria[i].criterion));
      strcat (str, " during RECLASSIFY");
      IO_DruckeStatistikZeile(str, ReClasCriteria[i].fullfilled);
    }
  }

  IO_DruckeKurzUeberschrift("Critical Goals", '+');
  IO_DruckeStatistikZeile("total number of critical goals", Stat.KPV_AnzCritGoalsGesamt);
  IO_DruckeStatistikZeile("current number of critical goals", Stat.KPV_AnzCritGoalsActual);
  IO_DruckeStatistikZeile("max number of critical goals", Stat.KPV_AnzMaxCritGoals);
  IO_DruckeStatistikZeile("joined during interreduction", Stat.KPV_AnzIRGleich);
  IO_DruckeStatistikZeile("reduced during interreduction", Stat.KPV_AnzIRReduziert);
  IO_DruckeStatistikZeile("killed orphans during interreduction", Stat.KPV_AnzIRWMord);
  
  IO_DruckeKurzUeberschrift("Interreduktion", '+');
  IO_DruckeStatistikZeile("Regeln fielen der Interreduktion zum Opfer", Stat.AnzRegWgLinksRedWeg);
  IO_DruckeStatistikZeile("Regeln rechts reduziert", Stat.AnzRegRechtsRed);
  IO_DruckeStatistikZeile("Gleichungen fielen der Interreduktion zum Opfer", Stat.AnzGleichungenInterRed);
  IO_DruckeStatistikZeile("Gleichungen fielen der Backward-Subsumption zum Opfer", Stat.AnzErfSubsumptionenAlt);
#if KPIR_STATISTIK
  if (StatistikAn){
    int tiefe1 = Platzverbrauch(Maximum(2,Stat. AnzKPIRReduziert, Stat.AnzKPIRSubsummiert));
     
     IO_DruckeKurzUeberschrift("Intermediate Reprocessing (IRP)", '+');
     IO_DruckeStatistikZeile("CPs touched (total)", Stat.AnzKPIRBehandelt);
     IO_DruckeStatistikZeileEingerueckt("of them subsumed", Stat.AnzKPIRSubsummiert, tiefe1);
     IO_DruckeStatistikZeileEingerueckt("of them simplified", Stat.AnzKPIRReduziert, tiefe1);
     IO_DruckeStatistikZeileEingerueckt("of them joined", Stat.AnzKPIRZusammengefuehrt, tiefe1+Platzverbrauch(Stat.AnzKPIRZusammengefuehrt));
     IO_DruckeStatistikZeile("CPs touched only for reweighting", Stat.AnzKPnurW);
   }
#endif
   {
   }
   {
     int tiefe1;
     /******** Regelmenge ****************************** */
     if (RE_AnzRegelnGesamt() > 0){
       unsigned int KnotenImBaum, GesparteKnoten;
       BO_KnotenZaehlen(&KnotenImBaum, &GesparteKnoten, RE_Regelbaum);
       IO_DruckeKurzUeberschrift("Set of Rules",'+');
       tiefe1 = Einruecktiefe(0, Stat.AnzRegelnAktuell);
       IO_DruckeStatistikZeile("Total Number of Rules",Stat.RE_AnzRegelnGesamt);
       IO_DruckeStatistikZeileEingerueckt("of them still in R", Stat.AnzRegelnAktuell, tiefe1);
       IO_DruckeStatistikLeerZeile();
       IO_DruckeStatistikZeile("Nodes in R-tree", KnotenImBaum);
       IO_DruckeStatistikZeile("Nodes saved", GesparteKnoten);
       IO_DruckeStatistikZeileAnteil("Percentage of saved nodes (compared with SDT)", 
                                     GesparteKnoten, GesparteKnoten+KnotenImBaum);
     }
     /******** Gleichungsmenge ************************* */
     if (RE_AnzGleichungenGesamt() > 0){
       unsigned int KnotenImBaum, GesparteKnoten;
       BO_KnotenZaehlen(&KnotenImBaum, &GesparteKnoten, RE_Gleichungsbaum);
       IO_DruckeKurzUeberschrift("Set of Equations",'+');
       IO_DruckeStatistikZeile("Total Number of Equations",Stat.RE_AnzGleichungenGesamt);
       tiefe1 = Einruecktiefe(0, Maximum(2, Stat.AnzGleichungenAktuell, Stat.AnzMonogleichungen));
       IO_DruckeStatistikZeileEingerueckt("of them still in E", Stat.AnzGleichungenAktuell, tiefe1);
       IO_DruckeStatistikZeileEingerueckt("of them Mono-Equations", Stat.AnzMonogleichungen, tiefe1);
       IO_DruckeStatistikZeileEingerueckt("of them still in E", Stat.AnzAktMonogleichungen, tiefe1+Platzverbrauch(Stat.AnzAktMonogleichungen)+2);
       IO_DruckeStatistikLeerZeile();
       IO_DruckeStatistikZeile("Nodes in E-tree", KnotenImBaum);
       IO_DruckeStatistikZeile("Nodes saved", GesparteKnoten);
       IO_DruckeStatistikZeileAnteil("Percentage of saved nodes (compared with SDT)", 
                                     GesparteKnoten, GesparteKnoten+KnotenImBaum);
     }
   }
#if U1_STATISTIK
   {
     int tiefe1;
     if(WerteDa(2,Stat.AnzUnifVersRegelnToplevel, Stat.AnzUnifVersGleichgnToplevel)){
       IO_DruckeKurzUeberschrift("Unifikation", '+');
       tiefe1 = Einruecktiefe(0, Maximum(6, Stat.AnzUnifErfRegelnToplevel, Stat.AnzUnifErfRegelnTeilterme,
                                            Stat.AnzUnifErfRegelnEigeneTterme, Stat.AnzUnifErfGleichgnToplevel,
                                            Stat.AnzUnifErfGleichgnTeilterme, Stat.AnzUnifErfGleichgnEigeneTterme));
       IO_DruckeStatistikZeile("Kapur-Opfer", Stat.AnzKPOpferKapur);
       IO_DruckeStatistikLeerZeile();
       IO_DruckeStatistikZeile("Unterdrueckte Nicht-Eins-Stern-Paare", Stat.AnzNonEinsSternKPs);
       IO_DruckeStatistikLeerZeile();
       IO_DruckeStatistikZeile("Unterdrueckte Nusfu-Paare", Stat.AnzNonNusfuKPs);
       IO_DruckeStatistikLeerZeile();
       IO_DruckeStatistikZeile("Unifikationsversuche mit der Regelmenge Toplevel",Stat.AnzUnifVersRegelnToplevel);
       IO_DruckeStatistikZeileEingerueckt("dabei erfolgreich",Stat.AnzUnifErfRegelnToplevel, tiefe1);
       IO_DruckeStatistikLeerZeile();
       IO_DruckeStatistikZeile("Unifikationsversuche mit Teiltermen der Regelmenge",Stat.AnzUnifVersRegelnTeilterme);
       IO_DruckeStatistikZeileEingerueckt("dabei erfolgreich",Stat.AnzUnifErfRegelnTeilterme, tiefe1);
       IO_DruckeStatistikLeerZeile();
       IO_DruckeStatistikZeile("Unifikationsversuche mit eigenen Teiltermen (RM)",Stat.AnzUnifVersRegelnEigeneTterme);
       IO_DruckeStatistikZeileEingerueckt("dabei erfolgreich",Stat.AnzUnifErfRegelnEigeneTterme, tiefe1);
       if (RE_AnzGleichungenGesamt > 0){
         IO_DruckeStatistikLeerZeile();
         IO_DruckeStatistikZeile("Unifikationsversuche mit der Gleichungsmenge Toplevel",Stat.AnzUnifVersGleichgnToplevel);
         IO_DruckeStatistikZeileEingerueckt("dabei erfolgreich"Stat.,AnzUnifErfGleichgnToplevel, tiefe1);
         IO_DruckeStatistikLeerZeile();
         IO_DruckeStatistikZeile("Unifikationsversuche mit Teiltermen der Gleichungsmenge",Stat.AnzUnifVersGleichgnTeilterme);
         IO_DruckeStatistikZeileEingerueckt("dabei erfolgreich",Stat.AnzUnifErfGleichgnTeilterme, tiefe1);
         IO_DruckeStatistikLeerZeile();
         IO_DruckeStatistikZeile("Unifikationsversuche mit eigenen Teiltermen (GM)",Stat.AnzUnifVersGleichgnEigeneTterme);
         IO_DruckeStatistikZeileEingerueckt("dabei erfolgreich",Stat.AnzUnifErfGleichgnEigeneTterme, tiefe1);
         IO_DruckeStatistikLeerZeile();
         IO_DruckeStatistikZeile("Ueberlappungen erzeugt", Stat.AnzUnifUeberlappungenGebildet);
         IO_DruckeStatistikZeile("Wegen >-Test fehlgeschlagene KP-Bildungen Regel-Gleichung", Stat.AnzUnifMisserfolgeOrdnungRG);
         IO_DruckeStatistikZeile("Wegen >-Test fehlgeschlagene KP-Bildungen Gleichung-Gleichung",Stat. AnzUnifMisserfolgeOrdnungGG);
         IO_DruckeStatistikZeile("Dabei ueberfluessigerweise gebildete kritische Terme", Stat.AnzUnifUeberflKritTerme);
       }
       IO_DruckeStatistikLeerZeile();
       IO_DruckeStatistikZeile("Insgesamt erzeugte KPs", 
                               Stat.AnzUnifErfRegelnToplevel+Stat.AnzUnifErfRegelnTeilterme+
                               Stat.AnzUnifErfRegelnEigeneTterme+Stat.AnzUnifErfGleichgnToplevel+
                               Stat.AnzUnifErfGleichgnTeilterme+Stat.AnzUnifErfGleichgnEigeneTterme);
       tiefe1 = Einruecktiefe(0, Maximum(3, Stat.AnzKPRR, Stat.AnzKPGG,Stat.AnzKPRG));
       IO_DruckeStatistikZeileEingerueckt("davon enstanden zwischen 2 Regeln", Stat.AnzKPRR, tiefe1);
       IO_DruckeStatistikZeileEingerueckt("davon enstanden zwischen 2 Gleichungen", Stat.AnzKPGG, tiefe1);
       IO_DruckeStatistikZeileEingerueckt("davon enstanden zwischen Regel und Gleichung", Stat.AnzKPRG, tiefe1);
     }
   }
#endif
   if(WerteDa(2,Stat.AnzUnifVers,Stat.AnzUnifErf)) {
     int Tiefe1 = Einruecktiefe(0, Maximum(1, Stat.AnzUnifErf));
     IO_DruckeKurzUeberschrift("Pattern-Unifikation", '+');
     IO_DruckeStatistikZeile("Unifikationsversuche",Stat.AnzUnifVers);
     IO_DruckeStatistikZeileEingerueckt("dabei erfolgreich",Stat.AnzUnifErf, Tiefe1);
   }

#if NFB_STATISTIK
   {
     if (WerteDa(2, Stat.AnzRedVersMitRegeln, Stat.AnzRedVersMitGleichgn)){
       IO_DruckeKurzUeberschrift("rewriting",'+');
       IO_DruckeStatistikZeile("match queries R",Stat.AnzRedVersMitRegeln);
       IO_DruckeStatistikLeerZeile();
       IO_DruckeStatistikZeile("Reductions with R",Stat.AnzRedErfRegeln);
       IO_DruckeStatistikLeerZeile();
       IO_DruckeStatistikZeile("match queries E",Stat.AnzRedVersMitGleichgn);
       IO_DruckeStatistikLeerZeile();
       IO_DruckeStatistikZeile("Reductions with E",Stat.AnzRedErfGleichgn);
       IO_DruckeStatistikLeerZeile();
       IO_DruckeStatistikZeile("total match queries", Stat.AnzRedVersMitRegeln + Stat.AnzRedVersMitGleichgn);
       IO_DruckeStatistikZeile("total reductions", Stat.AnzRedErfRegeln + Stat.AnzRedErfGleichgn);
     }
   }
#else
   IO_DruckeStatistikZeile("rewriting-statistics OFF", 0);
#endif

   if(WerteDa(1,Stat.AnzRedMisserfolgeOrdnung)) {
     IO_DruckeKurzUeberschrift("MatchOperationen", '+');
     IO_DruckeStatistikZeile("An >-Test gescheiterte Reduktionsversuche mit GM", Stat.AnzRedMisserfolgeOrdnung);
   }
   if (Stat.AnzEingefRegeln>0){
     IO_DruckeKurzUeberschrift("Operationen auf Regellisten (insgesamt, dh. R & E)", '+');
     IO_DruckeKurzUeberschrift("Operationen auf digitalen Suchbaeumen (insgesamt, d.h. R & E)",'+');
     IO_DruckeStatistikZeile("Einfuege-Operationen",Stat.AnzEingefRegeln);
     IO_DruckeStatistikZeile("Loesch-Operationen",Stat.AnzEntfernterRegeln);
     IO_DruckeStatistikZeile("Aufspaltungen von Termsuffici aus Blaettern",Stat.AnzSchrumpfPfadExp);
     IO_DruckeStatistikZeile("Zusammenziehungen von linearen Unterbaeumen in Blaetter",Stat.AnzPfadschrumpfungen);
   }
   IO_DruckeKurzUeberschrift("Treatment of Goals", '+');
   IO_DruckeStatistikZeile("Goals proved by rewriting", Stat.AnzErfZusfuehrungenZiele);
   if (Z_ZielModul == Z_MNF_Ziele)
     IO_DruckeStatistikZeile("Maximal size of elems in goal set",  Stat.MNFZ_MaxCountMNFSet);
#if MNF_STATISTIK   
   if (MNF_Initialised) {
     IO_DruckeKurzUeberschrift("MultipleNormalForms",'+');
     
     IO_DruckeStatistikZeile("Total Termpairs", Stat.MNF_AnzCalls);
     IO_DruckeStatistikZeile("Joinable Termpairs", Stat.MNF_AnzTermGleichOpp);
     IO_DruckeStatistikZeile("Immediatly joinable Termpairs", Stat.MNF_AnzDirektZus);
     IO_DruckeStatistikZeile("Reductions", Stat.MNF_AnzReduktionen);
     IO_DruckeStatistikZeile("Match queries with equations", Stat.MNF_AnzMatchVersucheE);
     IO_DruckeStatistikZeile("Match queries with rules", Stat.MNF_AnzMatchVersucheR);
     IO_DruckeStatistikZeile("Maximal Size of MNF-Set", Stat.MNF_MaxNachfolger);
     IO_DruckeStatistikZeile("Stops: MNF-set > maxSize", Stat.MNF_AbbruchMaxSize);
     IO_DruckeStatistikZeile("Total steps up with E", Stat.MNF_AnzEQUp);
     IO_DruckeStatistikZeile("Maximal steps up with E", Stat.MNF_MaxEQUp);
     IO_DruckeStatistikZeile("Total steps up with R", Stat.MNF_AnzAntiR);
   }
#endif
}

static void ST_Statistik_short(void)
{
  IO_DruckeKurzUeberschrift("Unselected Equations",'+');
  /*  IO_DruckeStatistikZeile("Generated", Stat.KPV_AnzKPGesamt);*/
  IO_DruckeStatistikZeile("Generated", Stat.KPV_AnzCPs 
			  /*Stat.KPV_AnzKPGesamt-Stat.KPV_AnzKPAusSpezifikation-Stat.KPV_AnzKPIROpfer*/);
  IO_DruckeStatistikZeile("Inserted", Stat.KPV_AnzKPInserted);
  IO_DruckeStatistikZeile("Selected", Stat.KPV_AnzKPBehandeltInVervollst);
  IO_DruckeStatistikZeile("Perm-Subsumed", Stat.KPV_AnzKPspaeterPermSubsummiert);
  IO_DruckeKurzUeberschrift("Sets of Rules and Equations",'+');
  IO_DruckeStatistikZeile("Total Number of Rules",Stat.RE_AnzRegelnGesamt);
  IO_DruckeStatistikZeile("Total Number of Equations",Stat.RE_AnzGleichungenGesamt);
#if NFB_STATISTIK
  IO_DruckeKurzUeberschrift("rewriting", '+');
  IO_DruckeStatistikZeile("Match Queries", Stat.AnzRedVersMitRegeln + Stat.AnzRedVersMitGleichgn);
  IO_DruckeStatistikZeile("Reductions", Stat.AnzRedErfRegeln + Stat.AnzRedErfGleichgn);
#endif
  
  if (Z_ZielModul == Z_MNF_Ziele){
    IO_DruckeKurzUeberschrift("Treatment of Goals", '.');
    IO_DruckeStatistikZeile("Maximal size of elems in goal set",  Stat.MNFZ_MaxCountMNFSet);
  }
#if MNF_STATISTIK   
  if (MNF_Initialised) {
    IO_DruckeKurzUeberschrift("MultipleNormalForms",'+');
    IO_DruckeStatistikZeile("MNF Reductions", Stat.MNF_AnzReduktionen);
    IO_DruckeStatistikZeile("Total steps up (R+E)", Stat.MNF_AnzEQUp+Stat.MNF_AnzAntiR);
  }
#endif
}

static void ST_Statistik_slatex_head(void)
{
  printf("CPs gen    & inserted   & selected   & perm-subs  & ");
  printf("Rules      & Equations  & ");
#if NFB_STATISTIK
  printf("Matches    & Reductions & ");
#endif
  
  if (Z_ZielModul == Z_MNF_Ziele){
    printf("G:MaxSize &");
  }
#if MNF_STATISTIK   
  if (MNF_Initialised) {
    printf("MNF: Red & Tot. Up & ");
  }
#endif
}

/* -------------------------------------------------------*/
/* -------------- Prozess-Statistik ----------------------*/
/* -------------------------------------------------------*/

static void ST_PlatzverbrauchAusgeben(BOOLEAN Kopf)
     /* Ausgabe des Platzverbrauchs im Latex-Tabellenformat */
{
#if defined (SUNOS4) || defined(MACOSX) || defined(DARWIN) /* || defined (LINUX) */
  if (Kopf) printf("max.res.mem ");
  else{
    struct rusage rusageBuf;
    
    if (getrusage(RUSAGE_SELF, &rusageBuf) == -1) printf(" <error>");
    else IO_DruckeStatistikZeile("bla",rusageBuf.ru_maxrss * getpagesize());
  }
#elif defined (LINUX)

  if (Kopf) {
    printf("vsize rss.mem ");
  } else {
    FILE *statFd;
    unsigned long vsize, rss, pagesize;

    statFd = fopen("/proc/self/statm", "r");
    if (statFd != NULL) {
      fscanf(statFd, "%ld %ld", &vsize, &rss); /* XXX nochmals ueberpruefen ! */
      fclose(statFd);
      pagesize = sysconf(_SC_PAGESIZE);
      vsize = vsize * pagesize;
      rss = rss * pagesize;
      if (0){
	printf(" %lu & %lu &", vsize, rss);
      }
      else {
	vsize = vsize / 1024;
	rss = rss / 1024;
	if (vsize > 10240){
	  printf(" %.1fM & %.1fM &", vsize/1024.0, rss/1024.0);
	}
	else {
	  printf(" %luk & %luk &", vsize, rss);
	}
      }
    } else {
      printf(" <error> & <error> &");
    }
  }  

#elif defined (SUNOS5)
  if (Kopf) printf("proc.size& res.mem &");
  else{
    static    int             fd;
    static    BOOLEAN         status;
    prusage_t       prusage;
    prpsinfo_t      prpsinfo;
    
    if (status == 0){
      if ((fd = open("/proc/self",O_RDONLY)) == -1){
        status = 2;
      }
      else {
        status = 1;
      }
    }
    if ((status == 2) ||
        (ioctl(fd, PIOCUSAGE, &prusage) == -1) ||
        (ioctl(fd, PIOCPSINFO, &prpsinfo) == -1))
      printf(" <error> & <error> &");
    else
      printf(" %ld & %ld &", prpsinfo.pr_size*sysconf(_SC_PAGESIZE),
              prpsinfo.pr_rssize *sysconf(_SC_PAGESIZE));
  }
#else
#error "ST_PlatzverbrauchAusgeben muss angepasst werden!"      
#endif
}

void ST_ProzessStatistik(void)
/* Ausgabe von Prozess-Daten (aus getrusage)  */
{

#if defined (SUNOS4) || defined (LINUX) || defined(MACOSX) || defined(DARWIN)
   struct rusage rusageBuf;
   
   if (getrusage(RUSAGE_SELF, &rusageBuf) == -1)
      our_assertion_failed ("IO_ProzessStatistik: getrusage fehlgeschlagen");
   
   IO_DruckeUeberschrift("Prozess-Statistik");
   IO_DruckeStatistikZeile("maximum resident set size/pages", (unsigned long int)rusageBuf.ru_maxrss);
   IO_DruckeStatistikZeile("system pagesize (may differ from HW-pagesize)",(unsigned long int) getpagesize());
   IO_DruckeStatistikZeile("==> maximum resident amount of memory (bytes)", rusageBuf.ru_maxrss * getpagesize());
   IO_DruckeStatistikZeile("memory in use by the process while running (integral value: pages * clock ticks)", (unsigned long int) rusageBuf.ru_idrss);
   IO_DruckeStatistikZeile("page faults serviced without any physical I/O activity", (unsigned long int) rusageBuf.ru_minflt);
   IO_DruckeStatistikZeile("page faults serviced with physical I/O activity", (unsigned long int) rusageBuf.ru_majflt);
   IO_DruckeStatistikZeile("times process was swapped out of main memory", (unsigned long int) rusageBuf.ru_nswap);
   IO_DruckeStatistikZeile("file system inputs (read)", (unsigned long int) rusageBuf.ru_inblock);
   IO_DruckeStatistikZeile("file system outputs (write)", (unsigned long int) rusageBuf.ru_oublock);
   IO_DruckeStatistikZeile("messages sent over sockets", (unsigned long int) rusageBuf.ru_msgsnd);
   IO_DruckeStatistikZeile("messages received from sockets", (unsigned long int) rusageBuf.ru_msgrcv);
   IO_DruckeStatistikZeile("signals delivered", (unsigned long int) rusageBuf.ru_nsignals);
   IO_DruckeStatistikZeile("context switches due to voluntarily giving up the processor before time slice completed", (unsigned long int) rusageBuf.ru_nvcsw);
   IO_DruckeStatistikZeile("context switches due to a higher priority process or exceeded time slice", (unsigned long int) rusageBuf.ru_nivcsw);
#elif defined (SUNOS5)
   int             fd;
   char            proc[40];
   prusage_t       prusage;
   prpsinfo_t      prpsinfo;
   
   sprintf(proc,"/proc/%d", (int)getpid());
   if ((fd = open(proc,O_RDONLY)) == -1) {
      our_assertion_failed ("IO_ProzessStatistik: open /proc fehlgeschlagen");
      }
   if (ioctl(fd, PIOCUSAGE, &prusage) == -1) {
      our_assertion_failed ("IO_ProzessStatistik: open prusage-ioctl fehlgeschlagen");
   }
   if (ioctl(fd, PIOCPSINFO, &prpsinfo) == -1) {
      our_assertion_failed ("IO_ProzessStatistik: open prpsinfo-ioctl fehlgeschlagen");
   }
   IO_DruckeUeberschrift("Prozess-Statistik");
   IO_DruckeStatistikZeile("system pagesize (may differ from HW-pagesize)",(unsigned long int) sysconf(_SC_PAGESIZE));
   IO_DruckeStatistikZeile("size of process /pages", (unsigned long int)prpsinfo.pr_size);
   IO_DruckeStatistikZeile("==> process size (bytes)", prpsinfo.pr_size*sysconf(_SC_PAGESIZE) );
   IO_DruckeStatistikZeile("resident set size/pages", (unsigned long int)prpsinfo.pr_rssize);
   IO_DruckeStatistikZeile("==> resident amount of memory (bytes)", prpsinfo.pr_rssize *sysconf(_SC_PAGESIZE) );
   IO_DruckeStatistikZeile("page faults serviced without any physical I/O activity", (unsigned long int) prusage.pr_minf);
   IO_DruckeStatistikZeile("page faults serviced with physical I/O activity", (unsigned long int) prusage.pr_majf);
   IO_DruckeStatistikZeile("times process was swapped out of main memory", (unsigned long int) prusage.pr_nswap);
   IO_DruckeStatistikZeile("file system inputs (read)", (unsigned long int) prusage.pr_inblk);
   IO_DruckeStatistikZeile("file system outputs (write)", (unsigned long int) prusage.pr_oublk);
   IO_DruckeStatistikZeile("messages sent over sockets", (unsigned long int) prusage.pr_msnd);
   IO_DruckeStatistikZeile("messages received from sockets", (unsigned long int) prusage.pr_mrcv);
   IO_DruckeStatistikZeile("signals delivered", (unsigned long int) prusage.pr_sigs);
   IO_DruckeStatistikZeile("context switches due to voluntarily giving up the processor before time slice completed", (unsigned long int) prusage.pr_vctx);
   IO_DruckeStatistikZeile("context switches due to a higher priority process or exceeded time slice", (unsigned long int) prusage.pr_ictx);
#else
#error "ST_ProzessStatistik muss angepasst werden!"      
#endif
}

typedef enum {Finished, Timeout, Error, Interrupt, NoMemory, GoalProved, AllProved, SomeRefuted} ReasonT;

static char* ReasonText[] = {"F", "T", "E", "I", "M", "G", "S", "R"};

static BOOLEAN st_PlatzverbrauchBestimmen(unsigned long *vsize, unsigned long *rss)
     /* Ausgabe des Platzverbrauchs im Latex-Tabellenformat */
{
  unsigned long pagesize = sysconf(_SC_PAGESIZE);
  *vsize = 0;
  *rss   = 0;

#if defined (SUNOS4) || defined(MACOSX) || defined(DARWIN)
  struct rusage rusageBuf;
  if (getrusage(RUSAGE_SELF, &rusageBuf) == -1) return FALSE;
  *rss   = rusageBuf.ru_maxrss * pagesize;
#elif defined (LINUX)
  FILE *statFd;
  statFd = fopen("/proc/self/statm", "r");
  if (statFd == NULL) return FALSE;
  fscanf(statFd, "%ld %ld", vsize, rss);
  fclose(statFd);
  *vsize = *vsize * pagesize;
  *rss   = *rss   * pagesize;
#elif defined (SUNOS5)
  static int      fd;
  prusage_t       prusage;
  prpsinfo_t      prpsinfo;
    
  if ((fd = open("/proc/self",O_RDONLY)) == -1) return FALSE;
  if ((ioctl(fd, PIOCUSAGE, &prusage) == -1) ||
      (ioctl(fd, PIOCPSINFO, &prpsinfo) == -1)) {
    close(fd);
    return FALSE;
  }
  close(fd);
  *vsize = prpsinfo.pr_size * pagesize;
  *rss   = prpsinfo.pr_rssize * pagesize;
#else
#error "st_PlatzverbrauchBestimmen muss angepasst werden!"      
#endif

  return TRUE;
}

static void st_WCTZeileAusgeben(ReasonT Situation)
{
  unsigned long vsize, rss, seconds, milliseconds;
return;  
  if (1){
    printf("\n%-10s ", "problem");
    printf("%10s ",    "CPs.gen");
    printf("%10s ",    "CPs.reexp");
    printf("%10s ",    "Select");
    printf("%10s ",    "R");
    printf("%10s ",    "E");
    printf("%7s %7s ", "vsize", "rss");
    printf("%12s ",    "process.time");
    printf("%12s ",    "wallclock");
    printf("%s",       "status");
  }

  printf("\n%-10s ", IO_SpezifikationsBasisname());
  printf("%10llu ",  Stat.KPV_AnzCPs);
  printf("%10llu ",  0);
  printf("%10llu ",  Stat.KPV_AnzKPBehandeltInVervollst);
  printf("%10llu ",  Stat.RE_AnzRegelnGesamt);
  printf("%10llu ",  Stat.RE_AnzGleichungenGesamt);

  st_PlatzverbrauchBestimmen(&vsize,&rss);
  if (0){
    printf("%10lu %10lu ", vsize, rss);
  }
  else {
    printf("%6.1fM %6.1fM ", vsize/(1024.0*1024.0), rss/(1024.0*1024.0));
  }

  milliseconds = (Dauer[User].tv_usec + Dauer[System].tv_usec + 500) / 1000;
  seconds      = Dauer[User].tv_sec + Dauer[System].tv_sec + milliseconds / 1000;
  milliseconds = milliseconds % 1000;

  printf("%7lu.%03lus ", seconds, milliseconds);

  milliseconds = (Dauer[Wall].tv_usec + 500) / 1000;
  seconds      = Dauer[Wall].tv_sec + milliseconds / 1000;
  milliseconds = milliseconds % 1000;

  printf("%7lu.%03lus ", seconds, milliseconds);

  if (Situation == Finished){
    if (Z_ZielGesamtStatus == Z_AlleWiederlegt || 
        Z_ZielGesamtStatus == Z_EinigeWiederlegt ) {
      Situation = SomeRefuted;
    }
    else {
      Situation = AllProved;
    }
  }
  printf("%6s",    ReasonText[Situation]);

  printf(" # wct");
  if (1 && seconds > 7200){
    unsigned long hours, days;
    hours = seconds / 3600;
    days  = hours / 24;
    if (days > 1){
      printf(" -- more than %ldd or %ldh", days, hours);
    }
    else if (hours > 1){
      printf(" -- more than %ldh", hours);
    }
  }
  printf("\n");
}

static void st_NeueStatistikAusgeben(ReasonT Situation)
{
  unsigned long vsize, rss, seconds, milliseconds;
  
  printf("\n%-16s%12s ", "Problem", IO_SpezifikationsBasisname());
  printf("\n%-16s%12llu ",  "CPs.gen", Stat.KPV_AnzCPs);
  printf("\n%-16s%12llu ",  "CPs.reexp", 0);
  printf("\n%-16s%12llu ",  "Select", Stat.KPV_AnzKPBehandeltInVervollst);
  printf("\n%-16s%12llu ",  "R", Stat.RE_AnzRegelnGesamt);
  printf("\n%-16s%12llu ",  "E", Stat.RE_AnzGleichungenGesamt);

  st_PlatzverbrauchBestimmen(&vsize,&rss);
  if (0){
    printf("\n%-16s%12lu \n%-16s%12lu ", "vsize", vsize, "rss", rss);
  }
  else {
    printf("\n%-16s%11.1fM \n%-16s%11.1fM ", "vsize", vsize/(1024.0*1024.0), "rss", rss/(1024.0*1024.0));
  }

  milliseconds = (Dauer[User].tv_usec + Dauer[System].tv_usec + 500) / 1000;
  seconds      = Dauer[User].tv_sec + Dauer[System].tv_sec + milliseconds / 1000;
  milliseconds = milliseconds % 1000;

  printf("\n%-16s%7lu.%03lus ", "process.time", seconds, milliseconds);

  milliseconds = (Dauer[Wall].tv_usec + 500) / 1000;
  seconds      = Dauer[Wall].tv_sec + milliseconds / 1000;
  milliseconds = milliseconds % 1000;

  printf("\n%-16s%7lu.%03lus ", "wallclock.time", seconds, milliseconds);

  if (Situation == Finished){
    if (Z_ZielGesamtStatus == Z_AlleWiederlegt || 
        Z_ZielGesamtStatus == Z_EinigeWiederlegt ) {
      Situation = SomeRefuted;
    }
    else {
      Situation = AllProved;
    }
  }
  printf("\n%-16s%12s",    "status", ReasonText[Situation]);

  printf("\n");
}

static void STEXZeileAusgeben(ReasonT Situation)
{
  IO_StatistikModusAufLatexSetzen();
  if (!PA_Slave()){
    printf("\n %-10s & ", "problem");
    ST_Statistik_slatex_head();
    ST_PlatzverbrauchAusgeben(TRUE);
    printf(" time & Result\n ");
  }
  DruckeBeispielTeX();
  ST_Statistik_short();
  ST_PlatzverbrauchAusgeben(FALSE);
  ZM_Statistik_latex();
  switch (Situation){
  case Finished:
    if (Z_ZielGesamtStatus == Z_AlleWiederlegt || Z_ZielGesamtStatus == Z_EinigeWiederlegt)
      printf(" & R ");
    else
      printf(" & S ");
    break;
  case Timeout:
    printf(" & T ");
    break;
  case Error:
    printf(" & E ");
    break;
  case NoMemory:
    printf(" & M ");
    break;
  case Interrupt:
    printf(" & I ");
    break;
  case GoalProved:
    printf(" & G ");
    break;
  default:
    break;
  }
  printf("\n");
  IO_StatistikModusAufNormalSetzen();
}

/* -------------------------------------------------------*/
/* -------------- Statistik ------------------------------*/
/* -------------------------------------------------------*/

static void AutoStatistik(ReasonT Situation, const char *Bemerkung);


/* -------------------------------------------------------*/
/* -------------- Statistik ------------------------------*/
/* -------------------------------------------------------*/


void ST_SuccessStatistik(const char *Bemerkung, BOOLEAN Resultat)
/* Ausgabe diverser Daten gemaess Aufruf.
*/
{
  AutoStatistik(Finished, Bemerkung);
}

#if COMP_POWER
void ST_PowerStatistik(const char *Bemerkung)
{
  DauerT Laufzeit = ZM_Laufzeit();
#if defined (SUNOS5)
  int fd;
  char proc[40];
  prusage_t prusage;
  prpsinfo_t prpsinfo;
  unsigned long int Prozessgroesse;
  sprintf(proc,"/proc/%d", (int)getpid());
  if ((fd = open(proc,O_RDONLY))==-1)
    our_assertion_failed ("IO_ProzessStatistik: open /proc fehlgeschlagen");
  if (ioctl(fd, PIOCUSAGE, &prusage)==-1)
    our_assertion_failed ("IO_ProzessStatistik: open prusage-ioctl fehlgeschlagen");
  if (ioctl(fd, PIOCPSINFO, &prpsinfo)==-1)
    our_assertion_failed ("IO_ProzessStatistik: open prpsinfo-ioctl fehlgeschlagen");
  Prozessgroesse = prpsinfo.pr_size*sysconf(_SC_PAGESIZE);
  Prozessgroesse = 1000*((Prozessgroesse+500)/1000);
#endif

  printf("\nWaldmeister states: %s.\n", Bemerkung);

  printf("\nStatistics:\n-----------\n");
  printf("Rules         %11lld\n", RE_AnzRegelnGesamt());
  printf("Equations     %11lld\n", RE_AnzGleichungenGesamt());
  printf("Critical pairs%11lld\n", GetKPV_AnzKPGesamt());
#if defined (SUNOS5)
  printf("Process size  %11lld\n", Prozessgroesse);
#endif
  printf("Search time [s]%9ld.%1ld\n",Laufzeit.Sekunden,Laufzeit.Zehntelsekunden);
}
#endif

void ST_ErrorStatistik(const char *Bemerkung, BOOLEAN Resultat)
{
  AutoStatistik(Error, Bemerkung);
}

void ST_NoMemory (const char *Bemerkung, BOOLEAN Resultat)
{
  AutoStatistik(NoMemory, Bemerkung);
}

void ST_TimeoutStatistik(const char *Bemerkung, BOOLEAN Resultat)
{
  AutoStatistik(Timeout, Bemerkung);
}

static void
ZwischenStatistik (ReasonT Situation, const char *Bemerkung)
{
   ZM_StoppGesamtZeit();
   SI_Usr1InterruptflagLoeschen ();
   SI_InterruptflagLoeschen ();
   AutoStatistik(Situation, Bemerkung);
   fflush (NULL);  /* Alle Output-puffer leeren */
   ZM_StartGesamtZeit();   
}

void ST_SignalUsrStatistik(const char *Bemerkung)
{
  ZwischenStatistik (Interrupt, Bemerkung);
}

void ST_GoalProvedStatistik(const char *Bemerkung)
{
  ZwischenStatistik (GoalProved, Bemerkung);
}



/* -------------------------------------------------------*/
/* -------------- Interne Statistik ----------------------*/
/* -------------------------------------------------------*/
unsigned int U1_AusdehnungKopierstapel (void);
unsigned int U1_AusdehnungKopierstapel2 (void);
unsigned int U2_AusdehnungKopierstapel (void);
extern unsigned long DomainGran;
extern int xUDimension;
extern int xUDimension2;

void ST_InterneStatistik(void)
{
   IO_DruckeUeberschrift("Interne Statistik");

   IO_DruckeStatistikZeile("Max. Expansion of Leaf-depth-array in R-tree", RE_Regelbaum.TiefeBlaetterDimension);
   IO_DruckeStatistikZeile("Max. Expansion of Leaf-depth-array in E-tree", RE_Gleichungsbaum.TiefeBlaetterDimension);
   IO_DruckeStatistikZeile("Maximale Ausdehnung der Unifikationsvariablenbank", -xUDimension);
   IO_DruckeStatistikZeile("Maximale Ausdehnung des Unifikationsstacks", -xUDimension);
   IO_DruckeStatistikZeile("Maximale Ausdehnung der drei U1_Endestapel", -xUDimension);
   IO_DruckeStatistikZeile("Maximale Ausdehnung des U1_Kopierstapels", U1_AusdehnungKopierstapel());
   IO_DruckeStatistikZeile("Maximale Ausdehnung des U1_Kopierstapels2", U1_AusdehnungKopierstapel2());
   IO_DruckeStatistikZeile("Maximale Ausdehnung der Unifikationsvariablenbank", -xUDimension);
   IO_DruckeStatistikZeile("Konstante Ausdehnung der Bank fuer Patternvariablen", SO_AnzahlFunktionsvariablen());
   IO_DruckeStatistikZeile("Maximale Ausdehnung des Unifikationsstacks", -xUDimension);
   IO_DruckeStatistikZeile("Maximale Ausdehnung der drei U2_Endestapel", -xUDimension);
   IO_DruckeStatistikZeile("Maximale Ausdehnung des U2_Kopierstapels", U2_AusdehnungKopierstapel());
   IO_DruckeStatistikZeile("Maximale Ausdehnung der Reduktionsvariablenbank",MO_AusdehnungVariablenbank());
   IO_DruckeStatistikZeile("Maximale Ausdehnung des MO_Kopierstapels", MO_AusdehnungKopierstapel());
   IO_DruckeStatistikZeile("Groesste aufgetretene Tiefe einer linken Seite", BO_MaximaleTiefe);
   IO_DruckeStatistikZeile("Maximale Ausdehnung der BO_Umbenennungsvariablenbank",BO_AusdehnungVariablenbank());
   IO_DruckeStatistikZeile("Maximale Ausdehnung des BO_Sprungstapels",BO_AusdehnungSprungstapel());
   IO_DruckeStatistikZeile("Maximale Ausdehnung des BO_Kopierstapels", BO_AusdehnungKopierstapel());
   IO_DruckeStatistikZeile("BK-successor-arrays allocated via malloc", BK_AnzEinzelnAllokierterNachfolgerArrays);
#if MNF_STATISTIK
   if (MNF_Initialised) {
     IO_DruckeStatistikZeile("Calls of HA_AddData", Stat.MNF_AnzCallsHAAddData);
     IO_DruckeStatistikZeile("Count MNF: same Terms", Stat.MNF_AnzTermGleich);
     IO_DruckeStatistikZeile("Count MNF: same Terms, same colour", Stat.MNF_AnzTermGleichCol);
     IO_DruckeStatistikZeile("Count MNF: same Terms, opp. colour", Stat.MNF_AnzTermGleichOpp);
     if (Stat.MNF_AnzCalls > 0)
       IO_DruckeStatistikZeile("Average Size of MNF-Set", Stat.MNF_SumAnzNachfolger/Stat.MNF_AnzCalls);
   }
#endif
#if HA_STATISTIK
   IO_DruckeStatistikZeile("Hash Collisions", Stat.HA_AnzBucketGleich);
   IO_DruckeStatistikZeile("keys1 equal (Hash)", Stat.HA_AnzKey1Gleich);
   IO_DruckeStatistikZeile("keys2 equal (Hash)", Stat.HA_AnzKey2Gleich);
   IO_DruckeStatistikZeile("Maximal Size of Hash", Stat.HA_MaxHashSize);
   IO_DruckeStatistikZeile("Maximal no. of elems in a bucket", Stat.HA_MaxElemsInBucket);
#endif
   
   IO_DruckeStatistikZeile("Maximale Ausdehnung des TO_Kopierstapels", TO_AusdehnungKopierstapel());
   IO_DruckeStatistikZeile("Maximale Ausdehnung des IO_Kopierstapels", IO_AusdehnungAusgabestapel());
}

/* -------------------------------------------------------*/
/* -------------- Konfiguration --------------------------*/
/* -------------------------------------------------------*/

void ST_DruckeKonfiguration(void)
/* Ausgabe der durch die defines in diesem Modul bestimmten Konfiguration. */
{
  struct utsname  utsbuf;

  IO_DruckeUeberschrift("Konfiguration");

  printf("%s", configuration_banner);
  
  IO_DruckeZeile('+');
  uname(&utsbuf);
  printf("Run executed on %s (%s, %s %s)\n", 
          utsbuf.nodename, utsbuf.machine, 
          utsbuf.sysname, utsbuf.release);
  IO_DruckeZeile('+');
  
#if IO_AUSGABEN
  printf("Ausgaben waehrend des Laufes AN, ");
#else
  printf("Ausgaben waehrend des Laufes AUS, ");
#endif
#if COMP_OPTIMIERUNG
  printf("Optimized Compilation.\n");
#endif
#if !COMP_OPTIMIERUNG
  printf("Non-Optimized Compilation.\n");
#endif
}

/* -------------------------------------------------------*/
/* -------------- Ergebnis-Menue -------------------------*/
/* -------------------------------------------------------*/

static void AutoStatistik(ReasonT Situation, const char *Bemerkung)
{
   st_WCTZeileAusgeben(Situation);
   
#if COMP_POWER
   switch (Situation){
   case Timeout:
     ST_PowerStatistik ("Run out of time");
     break;
   case NoMemory:
     ST_PowerStatistik ("Run out of memory");
     break;
   case Error:
     printf("Error: %s\n", Bemerkung);
     break;
   case Finished: 
   default: 
     break;
   }
#else
   switch (Situation){
   case Finished: 
     st_NeueStatistikAusgeben(Situation);
     if (Bemerkung != NULL){
       printf("\n\nWaldmeister states: %s.\n", Bemerkung);
     }
     break;
   case Timeout: 
     st_NeueStatistikAusgeben(Situation);
     printf("\n\n TIMEOUT: %s\n", Bemerkung);
     break;
   case Error:
   case NoMemory:
     st_NeueStatistikAusgeben(Situation);
     printf("\n\n ERROR: %s \n", Bemerkung);
     break;
   default: 
     break;
   }
#endif
}


