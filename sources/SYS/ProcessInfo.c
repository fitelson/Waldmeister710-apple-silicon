/**********************************************************************
* Filename : ProcessInfo.c
*
* Kurzbeschreibung : Prozessinformationen: Systemmodul
*
* Autoren : Andreas Jaeger
*
* 03.09.96: Erstellung
*
**********************************************************************/

#define HAVE_GETRUSAGE 1
#define HAVE_TIMES 1
#define HAVE_CLOCK 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_TIME 1

/* Dieser Typ muss noch in einen Header */
typedef struct {
   long wm_sec;  /* Sekunden */
   long wm_usec; /* Mikrosekunden */
} zeit_t;

#if HAVE_GETRUSAGE
#include <sys/resource.h>
#elif HAVE_TIMES
#include <sys/times.h>
#include <limits.h>
#elif HAVE_CLOCK
#include <time.h>
#endif

#if HAVE_GETTIMEOFDAY
#include <sys/time.h>
#elif HAVE_TIME
#include <time.h>
#endif


/***********************************************************************

  GetCPUTime:
  Liefert System und Userzeit, welche der Prozess benoetigt hat.

 ***********************************************************************/
#if HAVE_GETRUSAGE
void PI_GetCPUTime (zeit_t *user, zeit_t *sys)
{
   /* getrusage ist BSD-Funktion, welche auf den meisten
      Unix-Varianten vorhanden ist. */
   struct rusage rusageBuf;

   getrusage (RUSAGE_SELF, &rusageBuf);
   user->wm_sec = rusageBuf.ru_utime.tv_sec;
   user->wm_usec = rusageBuf.ru_utime.tv_usec;

   sys->wm_sec = rusageBuf.ru_stime.tv_sec;
   sys->wm_usec = rusageBuf.ru_stime.tv_usec;
}

#elif HAVE_TIMES

#define MILLION 1000000
void PI_GetCPUTime (zeit_t *user, zeit_t *sys)
{
   /* times ist definiert in Posix.1 */
   struct tms tmsbuf;

   /* times liefert Realtime als Resultat zurueck,
      wir ignorieren es aber hier.  */
   (void) times (&tmsbuf);
   user->wm_sec = (tmsbuf.tms_utime / CLK_TCK);
   user->wm_usec = (tmsbuf.tms_utime % CLK_TCK) * (MILLION / CLK_TCK);
   sys->wm_sec = (tmsbuf.tms_stime / CLK_TCK);
   sys->wm_usec = (tmsbuf.tms_stime % CLK_TCK) * (MILLION / CLK_TCK);
}

#elif HAVE_CLOCK
void PI_GetCPUTime (zeit_t *user, zeit_t *sys)
{
   /* clock ist definiert in ISO C89,
      hat aber nicht alle Funktionalitaet.  */
   clock_t process_time;

   process_time = clock ();

   user->wm_sec = (process_time / CLOCKS_PER_SECOND);
   user->wm_usec = (process_time % CLOCKS_PER_SECOND)
                   * (MILLION / CLOCKS_PER_SECOND);
   /* clock liefert nur eine Gesamtzeit */
   sys->wm_sec = 0;
   sys->wm_usec = 0;
}
#else
void PI_GetCPUTime (zeit_t *user, zeit_t *sys)
{
   /* Wir wissen nichts, also gibt's 'ne Informatikerantwort: */
   user->wm_sec = 42;
   user->wm_usec = 0;
   sys->wm_sec = 0;
   sys->wm_usec = 0;
}
#endif


/***********************************************************************

  PI_GetWallClock liefert Wall Clock Time

 ***********************************************************************/
#if HAVE_GETTIMEOFDAY
/* gettimeofday ist eine BSD Funktion, welche auf vielen
   Unixen existiert.  */
void PI_GetWallClock (zeit_t *wall)
{
   struct timeval tp;

   gettimeofday (&tp, NULL);
   wall->wm_sec = tp->tv_sec;
   wall->wm_usec = tp->tv_usec;
}
#elif HAVE_TIME
/* time ist eine ISO C89 Funktion.  */
void PI_GetWallClock (zeit_t *wall)
{
   time_t jetzt;

   jetzt = time (NULL);

   wall->wm_sec = (long) jetzt;
   wall->wm_usec = 0;
}
#else
void PI_GetWallClock (zeit_t *wall)
{
   wall->wm_sec = 42;
   wall->wm_usec = 0;
}
#endif
