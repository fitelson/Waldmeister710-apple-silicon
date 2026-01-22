/*
01.03.94 Angelegt.
02.03.94 Weitergemacht. Optionen eingebaut. Arnim.
04.03.94 Aufruf mit symbolischen Dateinamen eingebaut. Arnim.
07.03.94 KPHeuristik als Aufrufoption eingebaut.
16.06.98 Komplett umgebaut. AJ.
*/

/* ------------------------------------------------*/
/* ------------- Includes -------------------------*/
/* ------------------------------------------------*/
#include "antiwarnings.h"
#include "compiler-extra.h"


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Ausgaben.h"
#include "BeweisObjekte.h"
#include "Dispatcher.h"
#include "DSBaumOperationen.h"
#include "DSBaumTest.h"
#include "FehlerBehandlung.h"
#include "FileMenue.h"
#include "Grundzusammenfuehrung.h"
#include "Hauptkomponenten.h"
#include "KPVerwaltung.h"
#include "MatchOperationen.h"
#include "MissMarple.h"
#include "MNF.h"
#include "Parameter.h"
#include "PhilMarlow.h"
#include "Praezedenz.h"
#include "RUndEVerwaltung.h"
#include "Signale.h"
#include "Statistiken.h"
#include "SymbolGewichte.h"
#include "Unifikation1.h"
#include "Unifikation2.h"
#include "VariablenMengen.h"
#include "WaldmeisterII.h"
#include "ZeitMessung.h"
#include "Zielverwaltung.h"
#include "general.h"
#include "wrapper.h"
#include "version.h"

#include <time.h>

/* ------------------------------------------------*/
/* -------- globale Variablen ---------------------*/
/* ------------------------------------------------*/

unsigned int WM_AnlaufNr;
BOOLEAN WM_LaufBeendet = FALSE;
BOOLEAN wm_ueberholt = FALSE;

/* ------------------------------------------------*/
/* -------- lokale Prozeduren ---------------------*/
/* ------------------------------------------------*/

/* zum Suchen in den angegebenen Beispielverzeichnissen.*/

static const char  *wm_LaufErgebnis;         /* Ergebnis des Laufes im Klartext */
static BOOLEAN Resultat;
static BOOLEAN RunProof = FALSE;         /* wird auf TRUE gesetzt, wenn Beweis konstruiert werden soll */

static void SpezifikationEinlesen(char *filename)
/* Nur Spezifikation einlesen, damit extModus bekannt wird. */
{
   parse_spec(filename);
}
   
static void AllesAprioriInitialisieren(void)
/* Alles, was VOR Einlesen der Spezifikation passieren mu\3 oder kann. Die Aufrufparameter sind aber noch nicht eingestellt! */
{

   IO_InitApriori();  /* initialisieren */
   PA_InitApriori();  /* Parameter definieren */
   SV_InitApriori();  /* Falls SV-Debug-Betrieb: Aufruf malloc_debug(2) */
   BK_InitApriori();  /* Speichermanager fuer innere und Blattknoten und Baumverwaltungseintraege */
   BO_InitApriori();  /* Variablenbank; Speichermanager fuer gerichtet normierte Termpaare */
   RE_InitApriori();  /* Variablenumbenennungsbank dynamisch erzeugen; NaechsteBaumvariable auf x1 setzen */
   MO_InitApriori();  /* Variablenbank fuer Matching dynamisch erzeugen */
   VM_InitApriori();  /* Speichermanager fuer Variablenmengen */
   U1_InitApriori();  /* Bindungsstack, Substitutions-Variablenbank, TermIteration */
   U2_InitApriori();  /* Bindungsstack, Substitutions-Variablenbank, TermIteration */
   TP_InitApriori();  /* Termpaar-Manager initialisieren. */
   BT_InitApriori();  /* Stapel in DSBaumTest initialisieren. */
   LL_InitApriori();  /* Listenzellenmanager einrichten */
}

static void SpezifikationGelesen(void)
/* Eingelesene Strukturen freigeben. */
{
  /*clear_spec();*/
}

static HK_ErgebnisT ModusCompletionProof (StrategieT *Strategie)
{
  HK_ErgebnisT Ergebnis;

  Ergebnis = HK_Vervollstaendigung(Strategie);

  DIS_End();

  switch (Ergebnis) {
  case HK_VervollAbgebrochen:   /* Failing completion */
    wm_LaufErgebnis = "Failing completion aborted";
    break;
  case HK_Vervollstaendigt:
    wm_LaufErgebnis = HK_fair() ? "System completed" : "Unfair completion run out of critical pairs";
    if (HK_fair()) {
      COM_markSuccess();
    }
    break;
  case HK_ZielBewiesen:
    wm_LaufErgebnis = Z_AnzahlZiele()==1 ? "Goal proved" : 
      Z_AnzahlBewiesenerZiele()==Z_AnzahlZiele() ? "All goals proved" : "One goal proved";
    COM_markSuccess();
    break;
  case HK_Ueberholt:
    wm_ueberholt = TRUE;
  case HK_StrategieWechseln:
    break;
  case HK_Beendet:
    break;
  }
  
  if (PA_ExtModus() == completion) {
     RE_AusgabeAktiva();
     HK_ZieleAufZusammenfuehrbarkeitTesten();
  }
  RunProof = 
    (PA_ExtModus() == completion && (Z_MindEinZielBewiesen() || HK_fair())) ||
    (PA_ExtModus() == proof && Z_MindEinZielBewiesen());
  /* Note: The proof construction tool cannot cope with refutations. */
  if ((Ergebnis != HK_StrategieWechseln) && !wm_ueberholt)
    Z_AusgabeZieleEnde(HK_fair() && (PA_ExtModus() != proof || !Z_ZieleBewiesen())); /* System konvergent? */

  return Ergebnis;
}


static HK_ErgebnisT RunModus (StrategieT *Strategie)
{
   switch (PA_ExtModus()) {
   case proof:
     if (!PR_PraezedenzTotal) {
       our_error ("Precedence is not total");
     }
     else if (Z_AnzahlZiele()==0) {
       our_error ("Proof mode without goals");
     }
     return ModusCompletionProof (Strategie);
     break;
   case completion:
     return ModusCompletionProof (Strategie);
     break;
   case confluence:
   case convergence:
   case reduction:
   case termination:
   default:
     our_error("Sorry! Waldmeister supports at the moment only the modi COMPLETION and PROOF!");
   }
   return HK_Beendet;
}


static void LeseSpezifikation (int argc, char *argv[])
{
   char *Spezifikation;

   Spezifikation = FM_SpezifikationErkannt(argc, argv);
  
   if (Spezifikation == NULL){
     printf("\n\nTerminated: no specification!\n\n");
     exit(1);
   }

   /*********************************
    Gewaehlte Spezifikation einlesen
   ********************************/

   IO_SpezifikationsnamenSetzen (Spezifikation);

   SpezifikationEinlesen (Spezifikation);
}

static void DoSoftCleanUp (void)
{
  RE_setActivaOffset((unsigned long) HK_getAbsTime_M());              /*da die Daten aus dem Array Activa bei einem*/
                /*Strategiewechsel einfach entfernt werden, muss dieser Offset gesetzt werden, damit nicht versucht*/
                                                                /* wird diese Speicherbereiche explizit freizugeben*/
  KPV_SOFTCLEANUP();
  GZ_SoftCleanup();
  MM_SoftCleanup();
  SV_AlleManagerLeeren();
}

/* optstr != "" */
static void ArgumenteUmbauen (char *opt_str, int *argc, char **argv[])
{
   int altargc = *argc;
   int i = 0;
   int newargc;
   char **newargv;

   BuildArgv (opt_str, (*argv)[0], &newargc, &newargv);
   
   *argc += newargc-1;
   *argv = our_realloc (*argv, sizeof(char *) * *argc, "ArgumenteUmbauen");
   /* Argumente nach hinten schieben */
   for (i = 1; i < altargc; i++)
      (*argv)[*argc-i] = (*argv)[altargc-i];

   for (i = 1; i < newargc; i++)
     (*argv)[i] = newargv[i];
   our_dealloc (newargv);
}

static char* werwowann_prefix = "";
static char* wer = "";

#ifdef HOST_NAME_MAX
#define MY_HOST_NAME_MAX (HOST_NAME_MAX > 255 ?  HOST_NAME_MAX : 255)
#else
#define MY_HOST_NAME_MAX 255
#endif

static void wm_WerWoWann(void)
{
  int retval;
  pid_t p;
  char hostname[MY_HOST_NAME_MAX+1];
  time_t time_zeit;
  char zeit[32]; /* ctime verwendet 26 */
  char *doofesNewline;

  p = getpid();
  retval = gethostname(hostname,MY_HOST_NAME_MAX);
  if (retval != 0){
    strcpy(hostname, "<unknown>");
  }
  else {
    hostname[MY_HOST_NAME_MAX] = '\0';
  }
  time_zeit = time(NULL);
  if (time_zeit == ((time_t)-1)){
    strcpy(zeit, "<unknown>");
  }
  else {
    ctime_r(&time_zeit,zeit);
    doofesNewline = strchr(zeit,'\n');
    if (doofesNewline != NULL){
      *doofesNewline = '\0'; /* schliessendes \n ueberschreiben */
    }
  }
  printf("%s: %s running on host %s with pid %u. Time is %s at the moment.\n",
         werwowann_prefix, wer, hostname, p, zeit);
}

static void druckeAufrufzeile (int argc, char *argv[])
{
  int i;
  printf("COMMANDLINE:");
  for (i = 0; i < argc; i++){
    printf(" %s", (argv[i] != NULL) ? argv[i] : "(null)");
  }
  printf("\n");
}

/* ------------------------------------------------*/
/* ---------- DAS HAUPTPROGRAMM -------------------*/
/* ------------------------------------------------*/

#define AFFINITY 0 /* 0,1 */
#if AFFINITY
#include <sched.h>
#include <linux/unistd.h>
#endif

int    
#if COMP_POWER
    WM_main
#elif DO_OMEGA
       C_main
#else
       main
#endif
            (int argc, char *argv[])
{
  HK_ErgebnisT Ergebnis;
  int WM_argc;
  char **WM_argv,**WM_argv_copy;

#if 0
  {
    werwowann_prefix = "START";
    wer = argv[0];
    wm_WerWoWann();
    if(!COMP_COMP)printf("WM BANNER: %s\n",wm_banner);
    werwowann_prefix = "STOP";
    atexit(wm_WerWoWann);
    if(!COMP_COMP)druckeAufrufzeile(argc,argv);
  }
#endif
  
#if AFFINITY
  {
    unsigned long new_mask = 2;
    unsigned int len = sizeof(new_mask);
    unsigned long cur_mask;
    pid_t p = getpid();
    int ret;

    ret = sched_getaffinity(p, len, &cur_mask);
    printf("pid = %d, sched_getaffinity = %d, cur_mask = %08lx\n", p, ret, cur_mask);

    ret = sched_setaffinity(p, len, &new_mask);
    printf("pid = %d, sched_setaffinity = %d, new_mask = %08lx\n", p, ret, new_mask);

    ret = sched_getaffinity(p, len, &cur_mask);
    printf("pid = %d, sched_getaffinity = %d, cur_mask = %08lx\n", p, ret, cur_mask);

  }
#endif

  /* Wir benutzen eigene Variablen um argc, argv beliebig manipulieren zu koennen */
  WM_argc = argc;
  WM_argv = our_alloc (sizeof (char *) * WM_argc);
  WM_argv_copy = our_alloc (sizeof (char *) * WM_argc);
  memcpy (WM_argv, argv, sizeof (char *) * WM_argc);
  
  /************************
    Vorab-Initialisierung
   *************************/

  WM_AnlaufNr = 1;
  AllesAprioriInitialisieren(); /* Alle Module initialisieren. */
  
  /************************************************************
    Aufruf abarbeiten: Spezifikation und Parameter einstellen
   *************************************************************/
  
  PA_Vorbehandeln(WM_argc, WM_argv);                                /* interaction, protocol, automodus etc. */
  LeseSpezifikation(WM_argc, WM_argv);
  
  if (MASTER_RECHNET_MIT /*&& !PA_Slave()*/){
    printf("NICE 10 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
    nice(10);
  }
  if (PA_Automodus())
    memcpy (WM_argv_copy, WM_argv, sizeof (char *) * WM_argc);
  
  PA_InitAposterioriEarly ();
  PM_SpezifikationAnalysieren();

  

  /************************************************************
    Schleife fuer StrategieWechsel
   ************************************************************/
  do {
    StrategieT *Strategie = NULL;    
    if (PA_Automodus()) {
      Strategie = PM_Strategie(WM_AnlaufNr);
      if ((WM_AnlaufNr == 1) && (Strategie->Schrittzahl != 0)){ /* mehr als eine Strategie */
        HK_TurnTime = Strategie->Schrittzahl;
        COM_init();
        if (COM_fork()){
          ZM_StartGesamtZeit();                                               /* Gesamtzeit starten */
          if (COM_Vadder){
	    WM_AnlaufNr++; /* Vadder rechnet zweite Strategie */
	    Strategie = PM_Strategie(WM_AnlaufNr);
          }
          else {/* also Sohn */
 	    Strategie->Schrittzahl = 0;
          }
        }
        else { 
          if (WM_ErsterAnlauf()) {
            ZM_StartGesamtZeit();                                               /* Gesamtzeit starten */
          }
        }
      }
      else {
        if (WM_ErsterAnlauf()) {
          ZM_StartGesamtZeit();                                               /* Gesamtzeit starten */
        }
      }
      if (!WM_ErsterAnlauf()) {
        WM_argc = argc;
        memcpy (WM_argv, WM_argv_copy, sizeof (char *) * WM_argc);
      }
      ArgumenteUmbauen (Strategie->Optionen, &WM_argc, &WM_argv);
      if (!WM_ErsterAnlauf())
        AllesAprioriInitialisieren();
    }
    else {
	ZM_StartGesamtZeit();                                               /* Gesamtzeit starten */
    }
    
    PA_AufrufParameterEinstellen(WM_argc, WM_argv, get_mode());

    PA_InitAposteriori();                                  /* System gemaess Parameter-Einstellung initialisieren. */
    if (!PA_Slave()){
      DIS_Start(WM_argc, WM_argv);
    }

    if (WM_ErsterAnlauf() && COM_PrintIt()) 
      IO_DruckeKopf();                                                                /* Programm-Kopf ausgeben */

    SI_SignalsSetzen (); 
    if (COMP_DIST && PA_Slave()){
      Ergebnis = HK_SlaveLoop();
    }
    else {
      HK_GleichungenUndZieleHolen();                        /* Gleichungen und Ziele in Systemstrukturen uebernehmen */
      SpezifikationGelesen();
      if (WM_ErsterAnlauf() && COM_PrintIt())
	PM_StrategienDrucken();
    
      /*************************************************************************
       * Eigentlicher Programm-Lauf nach Art der Spezifikationsdatei
       *************************************************************************/
      Ergebnis = RunModus (Strategie);
    }
    if (Ergebnis == HK_StrategieWechseln) {
      DoSoftCleanUp ();
    }
    ++WM_AnlaufNr;
  }  while (Ergebnis == HK_StrategieWechseln);
  
  
  WM_LaufBeendet = TRUE;

  ZM_StoppGesamtZeit();  
  SI_SignalsDeaktivieren ();
  
  /**************************************************
    Ausgabe der Ergebnisse
    ***************************************************/
  
  if (!wm_ueberholt){
    BOB_DruckeEndsystemFuerBeweisObjekt(); /* ggf. Abschlussfakten drucken */
    BOB_Beenden();                 /* Beweis-Protokoll abschliessen */
  }

  /**************************************************
    Statistiken
    ***************************************************/
  
#if COMP_POWER
  ST_PowerStatistik(wm_LaufErgebnis);
#else
  ST_SuccessStatistik(wm_LaufErgebnis, Resultat);
#endif
  
  /**************************************************
    Speicher aufraeumen
    ***************************************************/
  COM_stopComputing();
  COM_wakeOther();
  COM_wait();

#if !COMP_COMP
  IO_AusgabenAus();                                                          /* verbliebene Strukturen loeschen */
  RE_RUndETest();                                     /* falls eincompiliert, Mengenkonsistenztest durchfuehren */
  
  if (!COMP_OPTIMIERUNG) {
    Z_Cleanup();    /* Ziele loeschen */
    MNF_Cleanup();
    KPV_CLEANUP();         /* muss vor dem Loeschen der R- bzw. E-Menge passieren */
    MM_Cleanup();
    GZ_Cleanup();
    PR_CleanUp();
    SG_CleanUp();
    RE_deleteActivesAndIndex();
    TO_FeldVariablentermeLeeren(); /* ... */
    PM_ArchivLeeren();
  }
  else {
    PM_ArchivLeeren();
    DoSoftCleanUp ();
  }
  
  IO_AusgabenEin();
  
  SO_CleanUp();

  if (!COMP_OPTIMIERUNG)
    SV_Statistik(FALSE, TRUE);      /* FALSE: kein System-Fehler aufgetreten */
  
  IO_CleanUp(); /* falls Ausgaben umgelenkt waren, die Protokoll-Datei schliessen */
  
  /**************************************************
    Ausgaben beenden
    ***************************************************/
  
  
#endif

  /* Programm-Ende */
#if COMP_POWER
  /* return = 0 bedeutet alles ok */
  return (RunProof == FALSE);
#else
  return (0);
#endif
}

