/***********************************************************************************************************************
 * Autor:      Juergen Sellentin, 13.02.94
 * Angepasst : Arnim, 04.03.94
 * Stackverwaltung zugunsten eines Arrays 'rausgeworfen. 11.04.94, Arnim.
 ***********************************************************************************************************************
 * 
 * Bemerkungen zur Zeitmessung:
 * ----------------------------
 *
 * Die Zeitmessungen werden mit der Systemfunktion "getrusage" realisiert. Diese protokolliert die seit dem Beginn des
 * Prozesses verbrauchte Zeit (als user- und system- Anteil). 
 * Die Genauigkeit dieser Messung haengt sehr stark von der update-Frequenz von getrusage ab !!!
 *
 ***********************************************************************************************************************/

#include "antiwarnings.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "compiler-extra.h"
#include "Ausgaben.h"
#include "FehlerBehandlung.h"
#include "ZeitMessung.h"
#include "SpeicherVerwaltung.h"
#include "WaldmeisterII.h"




/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++ Zeitmessung +++++++++++++++++++++++++++++++++++++ */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */


#define AnzMikrosekProSek 1000000


static void ZeitdifferenzAddieren(struct timeval *Summe, struct timeval Start, struct timeval Ende)
{
  Summe->tv_sec += Ende.tv_sec - Start.tv_sec;
   if ((Summe->tv_usec += Ende.tv_usec - Start.tv_usec)<0) {  
     Summe->tv_usec += AnzMikrosekProSek;
     Summe->tv_sec--;
   }
   else if (Summe->tv_usec >= AnzMikrosekProSek) {
     Summe->tv_usec -= AnzMikrosekProSek;
     Summe->tv_sec++;
   }
}


enum Zeitsorten {User,System,Wall};

struct timeval Start[3];
struct timeval Dauer[3];

void ZM_StartGesamtZeit(void)
{
   struct rusage rusageBuf;

   if (getrusage (RUSAGE_SELF, &rusageBuf) == -1){
      perror("Start time: getrusage didn't work"); 
      our_assertion_failed ("Start time: getrusage didn't work");
   }
   Start[User] = rusageBuf.ru_utime, Start[System] = rusageBuf.ru_stime;
   if (gettimeofday(&Start[Wall], NULL) == -1){
     perror("Start time: gettimeofday didn't work"); 
     our_assertion_failed ("Start time: gettimeofday didn't work");
   }
}

void ZM_StoppGesamtZeit(void)
{  
   struct timeval Ende;
   struct rusage rusageBuf;

   if (getrusage (RUSAGE_SELF, &rusageBuf) == -1)
      our_assertion_failed ("Stop time: getrusage didn't work");
   if (gettimeofday(&Ende, NULL) == -1)
      our_assertion_failed ("Stop time: gettimeofday didn't work");

   ZeitdifferenzAddieren(&Dauer[User],Start[User],rusageBuf.ru_utime);   /* user-time update */
   ZeitdifferenzAddieren(&Dauer[System],Start[System],rusageBuf.ru_stime);   /* system-time update */
   ZeitdifferenzAddieren(&Dauer[Wall], Start[Wall], Ende);
}


DauerT ZM_Laufzeit(void) /* Summe aus System- und User-Zeit */
{
  DauerT t;
  t.Zehntelsekunden = (Dauer[User].tv_usec+Dauer[System].tv_usec+50000)/100000;
  t.Sekunden = Dauer[User].tv_sec+Dauer[System].tv_sec+t.Zehntelsekunden/10;
  t.Zehntelsekunden %= 10;
  return t;
}
