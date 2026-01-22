/***********************************************************************************************************************
 * Schnittstelle fuer die Prozeduren zur Zeitmessung
 ***********************************************************************************************************************
 * Autor:     Juergen Sellentin. Implementiert am 13.02.94
 * Angepasst: Arnim am 04.03.94.  Makros und Funtionen eingesetzt.
 *            Arnim am 04.04.94.  Stack zur Verwaltung von geschachtelten Aufrufen eingebaut.
 *            Arnim bis 24.05.95. Komplett neues Konzept, gettimeofday und Zaehler eingebaut.  
 ***********************************************************************************************************************/


#ifndef ZEITMESSUNG_H
#define ZEITMESSUNG_H

#ifndef CCE_Source

#include "general.h"
#include "Parameter.h"
#include <sys/time.h>

void ZM_StartGesamtZeit(void);
void ZM_StoppGesamtZeit(void);

typedef struct {
  unsigned long int Sekunden,
                    Zehntelsekunden;
} DauerT;

DauerT ZM_Laufzeit(void); /* Summe aus System- und User-Zeit */

#endif

/* -------------------------------------------------------------------------- */
/* - Infrastructure for gettimeofday base time measurements ----------------- */
/* -------------------------------------------------------------------------- */

#ifndef ZM_TIME_MEASURE
#define ZM_TIME_MEASURE 0
/*#error "For using ZeitMessung.h ZM_TIME_MEASURE must be defined, either to 0 or to 1"*/
#endif

#if TIME_MEASURE

#define ZM_START(x)                             \
{                                               \
  struct timeval before, after;                 \
  if (TIME_MEASURE & (1<<x)){                       \
    gettimeofday(&before,NULL);                 \
    number ## x++;                              \
  }

#define ZM_STOP(x,r)                                                              \
  if (TIME_MEASURE & (1<<x)){                                                       \
    gettimeofday(&after,NULL);                                                  \
    if (before.tv_usec > after.tv_usec){                                        \
      after.tv_sec  = after.tv_sec - 1 - before.tv_sec;                         \
      after.tv_usec = after.tv_usec + 1000000 - before.tv_usec;                 \
    }                                                                           \
    else {                                                                      \
      after.tv_sec  = after.tv_sec - before.tv_sec;                             \
      after.tv_usec = after.tv_usec - before.tv_usec;                           \
    }                                                                           \
    printf("#%d %10ld %s %6d\n", x, after.tv_sec*1000000+after.tv_usec, r ? "Y" : "N", number ## x);  \
  }                                                                             \
}

#else
#define ZM_START(x) /* nix tun */
#define ZM_STOP(x,r)  /* nix tun */
#endif

#endif
