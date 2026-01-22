/**********************************************************************
* Filename : Signale.h
*
* Kurzbeschreibung : Signalbehandlung (ohne SIGSEGV -> SpeicherVerwaltung)
*
* Autoren : Andreas Jaeger
*
* 02.06.98: Erstellung aus WaldmeisterII.c heraus
*
**********************************************************************/

#include "antiwarnings.h"
#include "compiler-extra.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <stddef.h>

#include "Communication.h"
#include "FehlerBehandlung.h"
#include "Parameter.h"
#include "Signale.h"
#include "Statistiken.h"
#include "WaldmeisterII.h"
#include "ZeitMessung.h"

/* ----------------------------------------------------------------*/
/* -------- globale Variablen (siehe Signale.h) -------------------*/
/* ----------------------------------------------------------------*/

BOOLEAN volatile SI_IRFlag;
BOOLEAN volatile SI_Usr1IRFlag;

/* Signalhandler */
/* ============= */
#if defined (SUNOS4)
/* SUNOS4 kennt SA_RESTART nicht, da dies standardmaessig gesetzt
   ist, stattdessen gibt's SA_INTERRPUT mit der negierten Bedeutung */
# define SIGNAL_DEFINITION(name)    static void name (void)
# define SA_RESTART 0
#else
# define SIGNAL_DEFINITION(name)    static void name (int signal MAYBE_UNUSEDPARAM)
#endif

SIGNAL_DEFINITION (ontimeout)
{
   if (!WM_LaufBeendet){           /* Falls gerade eben Beweis gefunden ==> Lauf regulaer beenden */
      ZM_StoppGesamtZeit();
      ST_TimeoutStatistik("Timeout", FALSE);

      
      /* exit-Werte:
	 0 - success;
	 1 - failure;
	 2 - timeout;
	 3 - error;
      */
      COM_kill();
      COM_wait();
      exit(2);
   } else
     return;
}


/* Ausgeben der AutoStatistik */
SIGNAL_DEFINITION (onusr1)
{
   SI_InterruptflagSetzen();
   SI_Usr1InterruptflagSetzen();
}

SIGNAL_DEFINITION (onusr2)
{
  return;
}

SIGNAL_DEFINITION (onxcpu)
{
#if !COMP_COMP
  fputs ("Waldmeister hates SIGXCPU\n", stdout);
#endif
  COM_kill();
  COM_wait();
  exit (2);
}


SIGNAL_DEFINITION (onsegv)
{
#if 0
      /* mehr ausgeben, da es sonst zu einer Endlosschleife kommen kann (auf Linux). Prozedur wurde ausgelagert, */
  fputs("Unrecoverable Segmentation Fault\n",stderr);                              /* mal austesten, ob das klappt */
  abort ();                                                                        /* weniger Aufwand als exit(-1) */
#else
  fputs(COM_Prefix,stderr);
  fputs("Unrecoverable Segmentation Fault\n",stderr); 
  if (COM_forked) {
    if (COM_Vadder){
      COM_wakeOther();
      COM_wait();
    }
    else {
      COM_wakeOther();
    }
  }
  exit(5);
#endif
}

SIGNAL_DEFINITION (onterm)
{
   if (!WM_LaufBeendet){           /* Falls gerade eben Beweis gefunden ==> Lauf regulaer beenden */
      ZM_StoppGesamtZeit();
      ST_TimeoutStatistik("Got SIGTERM", FALSE);

      /* exit-Werte:
	 0 - success;
	 1 - failure;
	 2 - timeout;
	 3 - error;
      */
      COM_kill();
      COM_wait();
      exit(2);

   }
   else
      return;
}

SIGNAL_DEFINITION (onrest)
{
  if (COM_forked) {
    if (COM_Vadder){
      COM_wakeOther();
      COM_wait();
    }
    else {
      COM_wakeOther();
    }
  }
  exit(5);
}

/* Signalbehandlung:
   Es werden SIGUSR1, SIGINT, SIGVTALRM, SIGALRM behandelt.
   SIGSEGV wird in Strahlmemorium behandelt.
   Reaktion auf SIGUSR1 ist Ausgabe von einigen Infos zum Lauf - ohne Anzuhalten.
   Reaktion auf SIGINT ist Ausgabe des Menues.
   
   Reaktion auf Interrupt erfolgt nur, wenn 
   - Interaktion gewuenscht ist,
   - nicht in Datei geschrieben wird und
   - es sich um einen Vervollstaendigungs- oder Proof-Lauf handelt.
*/


void SI_SignalsSetzen (void) 
{
   struct itimerval timer;
   struct sigaction new_action;
   sigset_t block_mask;
   
   sigemptyset (&new_action.sa_mask);

   new_action.sa_handler = onterm;
   new_action.sa_flags =  SA_RESTART;
   sigaction (SIGTERM, &new_action, NULL);

   new_action.sa_handler = onsegv;
   new_action.sa_flags =  SA_RESTART;
   sigaction (SIGSEGV, &new_action, NULL);

   /* Solaris will anscheinend noch SIGXCPU behandelt haben */
   /* ausserdem fuer den Wettbewerb noetig */
   /* ansonsten klappt das ganze nicht :-( */
   new_action.sa_handler = onxcpu;
   new_action.sa_flags =  SA_RESETHAND|SA_RESTART; /* Signalhandler wird nur einmal aufgerufen */

   sigaction (SIGXCPU, &new_action, NULL);


   if (PA_ZeitlimitGesetzt() && PA_Zeitlimit () != 0){
      timer.it_interval.tv_sec  =  0;
      timer.it_interval.tv_usec =  0;
      
      timer.it_value.tv_sec  =  PA_Zeitlimit();
      timer.it_value.tv_usec =  0;

      new_action.sa_handler = ontimeout;
      new_action.sa_flags =  SA_RESETHAND|SA_RESTART; /* Signalhandler wird nur einmal aufgerufen */
      sigaction (SIGVTALRM, &new_action, NULL);
      setitimer (ITIMER_VIRTUAL, &timer, NULL);
   }
   if (PA_RealZeitlimitGesetzt() && PA_RealZeitlimit () != 0){
      /* Problem: Wenn Zeitlimit nicht erreicht wird, z.B. wegen
         Swapping oder anderer Systemaktivitaeten, dann laeuft der Prozess
         ein mehrfaches der gewuenschten Zeit. Deswegen wird noch ein
         zweiter Timer mit einem Timelimit in realer Zeit gesetzt,
         der dafuer sorgt, dass der Prozess wirklich terminiert.
         Achtung: ITIMER_REAL wird auch von alarm gebraucht - also
         auf keinen Fall alarm benutzen!
         */

      timer.it_interval.tv_sec  =  0;
      timer.it_interval.tv_usec =  0;

      timer.it_value.tv_sec  =  PA_RealZeitlimit();
      timer.it_value.tv_usec =  0;

      new_action.sa_handler = ontimeout;
      new_action.sa_flags =  SA_RESETHAND|SA_RESTART; /* Signalhandler wird nur einmal aufgerufen */

      sigaction (SIGALRM, &new_action, NULL);
      setitimer (ITIMER_REAL, &timer, NULL);

   }

   /* fuer den Notfall gibt's hier noch ein hartes limit auf CPU-Zeit */
   if ((PA_RealZeitlimitGesetzt() && PA_RealZeitlimit () != 0)
       || (PA_ZeitlimitGesetzt() && PA_Zeitlimit () != 0)) {
     struct rlimit limit;
     
     limit.rlim_cur = max(PA_RealZeitlimit (), PA_Zeitlimit ()) * 3;
     limit.rlim_max = limit.rlim_cur;
     setrlimit (RLIMIT_CPU, &limit);

   }

   if (PA_MemlimitGesetzt() && PA_Memlimit() != 0) {
     struct rlimit limit;
     
     limit.rlim_cur = PA_Memlimit() * 1024 * 1024;
     limit.rlim_max = limit.rlim_cur + 10 * 1024*1024;/* plus Reserve */
     setrlimit (RLIMIT_DATA, &limit);
     
     /* AS == VLIMIT: incl/nur? mmap */
#ifdef RLIMIT_AS
     setrlimit (RLIMIT_AS, &limit);
#endif
#ifdef RLIMIT_VLIMIT
     setrlimit (RLIMIT_VLIMIT, &limit);
#endif

     /* kein Core-File erzeugen */
     limit.rlim_cur = 0;
     limit.rlim_max = limit.rlim_cur;
     setrlimit (RLIMIT_CORE, &limit);
   }
   
   SI_InterruptflagLoeschen();  /* Mit FALSE initialisieren */
   SI_Usr1InterruptflagLoeschen();  /* Mit FALSE initialisieren */
   new_action.sa_handler = onusr1;
   new_action.sa_flags =  SA_RESTART; /* interrupted system calls werden neu gestartet */
   sigaction (SIGUSR1, &new_action, NULL);

   new_action.sa_handler = onusr2;
   new_action.sa_flags =  SA_RESTART; /* interrupted system calls werden neu gestartet */
   sigaction (SIGUSR2, &new_action, NULL);

   sigemptyset (&block_mask);
   /* Block other terminal-generated signals while handler runs. */
   sigaddset (&block_mask, SIGUSR2);


   new_action.sa_handler = onrest;
   new_action.sa_flags =  SA_RESTART; /* interrupted system calls werden neu gestartet */
   sigaction (SIGPIPE, &new_action, NULL);
}

void SI_SignalsDeaktivieren (void)
{
   struct itimerval timer;
   struct sigaction new_action;

   timer.it_interval.tv_sec  =  0;
   timer.it_interval.tv_usec =  0;
   
   timer.it_value.tv_sec  =  0;
   timer.it_value.tv_usec =  0;

   sigemptyset (&new_action.sa_mask);
   new_action.sa_handler = SIG_DFL;
   new_action.sa_flags =  0;

   if (PA_ZeitlimitGesetzt()){
      setitimer (ITIMER_VIRTUAL, &timer, NULL);
      sigaction (SIGVTALRM, &new_action, NULL);
   }

   if (PA_RealZeitlimitGesetzt()){
      setitimer (ITIMER_REAL, &timer, NULL);
      sigaction (SIGALRM, &new_action, NULL);
   }
   
   SI_InterruptflagLoeschen();  /* Mit FALSE initialisieren */
   sigaction (SIGINT, &new_action, NULL);
   
   sigaction (SIGUSR1, &new_action, NULL);
}
