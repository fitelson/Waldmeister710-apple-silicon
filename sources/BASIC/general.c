/****************************************************************************************
 * Prozeduren die in allen Komponenten benoetigt werden.
 ****************************************************************************************
 * Autor:    Arnim (von madprak1 kopiert)
 * Stand:    28.02.1994
 ****************************************************************************************/

#include "antiwarnings.h"
#include "general.h"
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "FehlerBehandlung.h"
#include "SpeicherVerwaltung.h"



char *our_strnchr(const char *cs, char c, int n)
{
  char *strchr_ = strchr(cs,c);
  return strchr_ && strchr_<cs+n ? strchr_ : NULL;
}

char *our_strdup(const char *inp)
{
  size_t len = strlen(inp) + 1;
  char *res = our_alloc(len);
  strncpy(res,inp,len);
  return res;
}


unsigned int LaengeNatZahl(unsigned long int n)
{
  unsigned int Laenge=1;
  while ((n /= 10))
    Laenge++;
  return Laenge;
}


void Maximieren(unsigned int *AltesMaximum, unsigned int KandidatNeuesMaximum)
{
  if (KandidatNeuesMaximum>*AltesMaximum)
    *AltesMaximum = KandidatNeuesMaximum;
}

void MaximierenLong(unsigned long int *AltesMaximum, unsigned long int KandidatNeuesMaximum)
{
  if (KandidatNeuesMaximum>*AltesMaximum)
    *AltesMaximum = KandidatNeuesMaximum;
}

void MaximierenLLong(unsigned long long int *AltesMaximum, unsigned long long int KandidatNeuesMaximum)
{
  if (KandidatNeuesMaximum>*AltesMaximum)
    *AltesMaximum = KandidatNeuesMaximum;
}

void MaximumAddieren(unsigned int *AlteSumme, unsigned int KandidatMaximum1, unsigned int KandidatMaximum2)
{
    *AlteSumme += max(KandidatMaximum1,KandidatMaximum2);
}

unsigned long int Maximum(int anz, ...)
/* Berechnet das Maximum der Argumentliste. Dabei muss das erste Argument die Anzahl der folgenden Argumente angeben,
   die Argumente selbst muessen unsigned long int sein. */
{
   va_list ap;
   int i;
   unsigned long int ulival, max = 0;

   va_start(ap, anz);
   for (i=0; i<anz; i++){
      ulival = va_arg(ap, unsigned long int);
      if (ulival>max)
         max = ulival;
   }
   va_end(ap);
   return max;
}

void Minimieren(unsigned int *AltesMinimum, unsigned int KandidatNeuesMinimum)
{
  if (KandidatNeuesMinimum<*AltesMinimum)
    *AltesMinimum = KandidatNeuesMinimum;
}

unsigned int fac(unsigned int n)
{
  unsigned int Ergebnis = 1;
  while (n>1)
    Ergebnis *= n--;
  return Ergebnis;
}


BOOLEAN WerteDa(int anz, ...)
/* Gibt TRUE zurueck, wenn einer der uebergebenen Werte > 0 ist
   Die Werte sind vom Typ unsigned long int. */
{
   va_list ap;
   int i;
   unsigned long int ulival;
   BOOLEAN ok=FALSE;

   va_start(ap, anz);
   for (i=0; i<anz; i++){
      ulival = va_arg(ap, unsigned long int);
      if (ulival>0){
         ok=TRUE;
         break;
      }
   }
   va_end(ap);
   return ok;
}


void chop (char *str)
/* Loeschen des letzten Zeichens */
{
   size_t len = strlen (str);

   if (len > 0)
      str[len-1] = '\0';
}


BOOLEAN IstPrim(unsigned int n)
{
  if (n<2)
    return FALSE;
  else if (!(n%2))
    return n==2;
  else {
    unsigned int i,
                 N = floor(sqrt(n));
    for(i=3; i<=N; i += 2)
      if (!(n%i))
        return FALSE;
    return TRUE;
  }
}


unsigned int PrimzahlNr(unsigned int n)
{ /* Liefert - unter Vernachlaessigung mehrerer Jahrhunderte Zahlentheorie - die Primzahl Nr. n,
     gezaehlt ab 0. Daher PrimzahlNr(0)=2. */
  if (n) {
    unsigned int i = 3;
    while (--n)
      while (!IstPrim(i += 2));                            /* terminiert, da unendlich viele Primzahlen existieren */
    return i;
  }
  else
    return 2;
}


#define NibbleBits(/*{"short","","long"}*/ Length, Dummy)                                                           \
  (BITS(unsigned Length int)/2)                    /* Es wird vorausgesetzt, dass die Bitzahlen stets gerade sind. */
#define NibbleUMax(/*{"short","","long"}*/ Length, Dummy)                                                           \
  ((1ul<<NibbleBits(Length,0))-1)
#define NibbleSMin(/*{"short","","long"}*/ Length, Dummy)                                                           \
  (-(signed)(NibbleUMax(Length,0)>>1)-1)
#define NibbleSMax(/*{"short","","long"}*/ Length, Dummy)                                                           \
  ((signed)(NibbleUMax(Length,0)>>1))
#define ConstructPairing(/*{"short","","long"}*/ Length, /*{"s_","_","l_"}*/ Prefix)                                \
                                                                                                                    \
unsigned Length int u ## Prefix ##pair(unsigned Length int x, unsigned Length int y)                                \
{ if (x>NibbleUMax(Length,0) || y>NibbleUMax(Length,0))                                                             \
    our_error("range excess in unsigned integer pairing");                                                          \
  return (x<<NibbleBits(Length,0))+y; }                                                                             \
                                                                                                                    \
unsigned Length int u ## Prefix ##unpair_x(unsigned Length int n)                                                   \
{ return n>>NibbleBits(Length,0); }                                                                                 \
                                                                                                                    \
unsigned Length int u ## Prefix ##unpair_y(unsigned Length int n)                                                   \
{ return n & NibbleUMax(Length,0); }                                                                                \
                                                                                                                    \
signed Length int s ## Prefix ##pair(signed Length int x, signed Length int y)                                      \
{ unsigned Length int x_, y_;                                                                                       \
  if (!(NibbleSMin(Length,0)<=x && x<=NibbleSMax(Length,0) && NibbleSMin(Length,0)<=y && y<=NibbleSMax(Length,0)))  \
    our_error("range excess in signed integer pairing");                                                            \
  x_ = (unsigned Length int)(x & NibbleUMax(Length,0));                                                             \
  y_ = (unsigned Length int)(y & NibbleUMax(Length,0));                                                             \
  return (signed Length int)((x_<<NibbleBits(Length,0))+y_); }                                                      \
                                                                                                                    \
signed Length int s ## Prefix ##unpair_x(signed Length int n)                                                       \
{ unsigned Length int x_ = u ## Prefix ##unpair_x((unsigned Length int) n);                                         \
  return (signed Length int)(x_|(x_ & 1<<(NibbleBits(Length,0)-1) ? NibbleUMax(Length,0)<<NibbleBits(Length,0):0));}\
signed Length int s ## Prefix ##unpair_y(signed Length int n)                                                       \
{ unsigned Length int y_ = u ## Prefix ##unpair_y((unsigned Length int) n);                                         \
  return (signed Length int)(y_|(y_ & 1<<(NibbleBits(Length,0)-1) ? NibbleUMax(Length,0)<<NibbleBits(Length,0):0));}\

ConstructPairing(short,s_)
ConstructPairing(,_)
ConstructPairing(long,l_)



/* Umwandeln eines durch Leerzeichen getrennten Strings (Achtung: keine doppelten Leerzeichen)
   in argc/argv fuer Programmaufrufe
   argv0 enthaelt den Programmnamen
*/
void BuildArgv (char *opt_str, char *argv0, int *argc, char **argv[])
{
  /* durch das our_strdup in dopt_str gibt's hier ein Memoryleak - aber ich sehe
     z.Zt. keine Moeglichkeit den String wieder freizugeben */
  int count = 0;
  char *str;
  char *dopt_str = our_strdup (opt_str);
  char **new_argv;
  int i;
  
  for (str = dopt_str; *str != '\0'; str++) {
    if (*str == ' ')
      ++count;
  }

  /* build new argv */
  new_argv = our_alloc ((count + 3) * sizeof (char *));
  new_argv [0] = argv0;
  new_argv [1] = dopt_str;
  i = 2;
  for (str = dopt_str; *str != '\0'; str++ ) {
     if (*str == ' ') {
        *str = '\0';
        new_argv [i] = str+1;
        ++i;
     }
  }
  new_argv [count+2] = NULL;
  *argv = new_argv;
  *argc = count + 2;
}

