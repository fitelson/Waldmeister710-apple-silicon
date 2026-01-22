
/* Datei: FehlerBehandlung.c
 * Zweck: Stellt verschiedene Behandlungsm"oglichkeiten f"ur Fehler zur Verf"ugung
 * Autor: Arnim Buch
 * Stand: 28.02.94
 * Ersterstellung: 28.02.94
 * Aenderungen:
 * 
 * 
 * 
 * 
 * 
 * 
 */

/* ACHTUNG !!!!!!!!!!!!!!

Um nach einem Fehler auf die Oberfl"ache zur"uckspringen zu k"onnen, ist vor(!) dem Aufruf von
create_base der aktuelle Zustand mit der Befehls-Sequenz

  if (setjmp(Environment_before_Computation_of_GB) == 0){   %R"ucksprung f"ur Fehler setzen. 
    create_base(...);
    }
  else{
    <Fehler ist aufgetreten>;  %hierhin nach Auftreten eines Fehlers gesprungen
  };

zu speichern. In der Fehler-Routine ist dann folgendes aufzurufen, um den Zustand vor dem Aufruf
 wiederherszustellen:

  longjmp (Environment_before_Computation_of_GB,1); % nach Fehler zur"uck auf Top-Level 

Genaueres findet Ihr im Anhang (ich  glaube, in Abschnitt B) vom Kerningham-Ritchie.
Diese Geschichten habe ich im Polynom-Parser benutzt, und dort funktionieren sie auch.
(Siehe parser.c ).

Ach ja, die Typen etc.:
#include <setjmp.h>   % ist die zu importierende Datei.

jmp_buf ist der Typ der Environment_before_... -Variablen.

So, damit mue/3te es eigentlich schon klappen.


Arnim, 21.02.94
*/

#include "antiwarnings.h"
#include <sys/resource.h>

#include "Ausgaben.h"
#include "FehlerBehandlung.h"
#include "general.h"
#include <stdio.h>
#include "SpeicherVerwaltung.h"

#ifndef CCE_Source
#include "Statistiken.h"           /* nach our_error wird die Statistik ausgegeben */
#include "ZeitMessung.h"
#include "Communication.h"
#endif



/* ================================================================================= */

void our_assertion_failed(const char *hoppala)
{
  fputs ("Assertion failed :\n", stderr);
  fputs (hoppala, stderr);
  exit(6);
}

/**************************************************************************************/

void our_warning(const char *autsch)
{
  fprintf (stderr, "Warning: %s\n", autsch);
}

/**************************************************************************************/

void our_error(const char *aua)
{
#ifndef CCE_Source
   ZM_StoppGesamtZeit();
   ST_ErrorStatistik(aua, FALSE);
   COM_kill();/* really ? */
   COM_wait();
#else
   fprintf(stderr,"%s\n", aua);
#endif
   exit(4);
}


void our_out_of_mem_error(const char *aua)
{
#ifndef CCE_Source
   if (PA_MemlimitGesetzt() && PA_Memlimit() != 0) {
      struct rlimit limit;
      /* Speicherlimit hochsetzen, sonst klappt nichts mehr */
      getrlimit (RLIMIT_DATA, &limit);
      limit.rlim_cur = limit.rlim_max;
      setrlimit (RLIMIT_DATA, &limit);
#ifdef RLIMIT_AS
      getrlimit (RLIMIT_AS, &limit);
      limit.rlim_cur = limit.rlim_max;
      setrlimit (RLIMIT_AS, &limit);
#endif
#ifdef RLIMIT_VLIMIT      
      getrlimit (RLIMIT_VLIMIT, &limit);
      limit.rlim_cur = limit.rlim_max;
      setrlimit (RLIMIT_VLIMIT, &limit);
#endif
   }
   
   ZM_StoppGesamtZeit();
   ST_NoMemory (aua, FALSE);
   COM_kill();/* really ? */
   COM_wait();
#endif
   exit(11);
}

/**************************************************************************************/

void not_yet_implemented(const char *functionName)
{
  fprintf (stderr, "Not implemented: %s", functionName);
  exit(5);
}

/**************************************************************************************/

void our_fatal_error(const char *aechhh)
{
  fputs ("Fatal error: ", stderr);
  fputs (aechhh, stderr);
  fflush(stderr);
  exit(3);
}
