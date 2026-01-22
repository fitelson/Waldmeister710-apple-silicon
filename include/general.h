/****************************************************************************************
 * Schnittstelle fuer Prozeduren und Typen, die in allen Komponenten benoetigt werden.
 ****************************************************************************************
 * Autor:    Arnim (kopiert von madprak1)
 * Stand:    16.03.94
 * 16.03.94  Funktions-Kopf von SpeicherLeerRaeumen eingebaut (passiert noch nix drin.)
 * 05.06.2002 erweitert um den Typ AbstactTime und macros hierauf
 ****************************************************************************************/
#ifndef GENERAL_H
#define GENERAL_H

#include "antiwarnings.h"
#include <string.h>
#include <limits.h>
#include <stdio.h>

#define IF_TERMZELLEN_JAGD 0
#if IF_TERMZELLEN_JAGD
#define ON_TERMZELLEN_JAGD(x) x
#else
#define ON_TERMZELLEN_JAGD(x) ((void) 0)/* nix */
#endif

/*********************************  KONSTANTEN *******************************************/

/* Strings, die mit fgets gelesen werden, haben die Laenge INPUT_STRLEN */
#define INPUT_STRLEN 100

#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif

/*********************************  TYPEN  **********************************************/


#ifdef __LCLINT__
typedef bool BOOLEAN;
# define __volatile
#elif defined(__APOGEE__)
# define FALSE 0
# define TRUE  1
typedef int BOOLEAN;
#else
typedef enum { FALSE = 0,
               TRUE = 1 
} BOOLEAN;
#endif


typedef enum { Groesser, Kleiner, Gleich, Unvergleichbar} VergleichsT;

typedef enum {GZ_zusammenfuehrbar, GZ_wertvoll, GZ_aussichtslos, 
	      GZ_zuTeuer, GZ_gescheitert, GZ_interred, GZ_undef} GZ_statusT;      /* Achtung: Reihenfolge wichtig! */

typedef char *STRING;

typedef char *ADDRESS;

typedef unsigned char byte;

#define BITS(type) (CHAR_BIT * sizeof(type))

/* Parameter fuer ParamInfo */
typedef enum {
   ParamEvaluate, ParamExplain
} ParamInfoArtT;

typedef unsigned long long int CounterT;

/* Typ fuer Gewichte in Classifikation */
typedef long int WeightT;
#define undefinedWeight() (LONG_MIN)
#define minimalWeight() (LONG_MIN+10)
#define maximalWeight() (LONG_MAX-10)

/* Typ der die abstakte Zeit in Schleifendurchlaeufen wiederspiegelt*/
typedef unsigned long int AbstractTime;
#define minimalAbstractTime() 0
#define maximalAbstractTime() ULONG_MAX

/*********************************  MAKROS**********************************************/


#define /*BOOLEAN*/ AdresseKleiner(/*void**/ Adresse1, /*void **/ Adresse2) \
  ((ADDRESS) Adresse1 < (ADDRESS) Adresse2)


#define forListe(Liste,Index,Block)                                                                                 \
  Index = Liste;                                                                                                    \
  while (Index) {                                                                                                   \
    Block                                                                                                           \
    Index = Index->Nachf;                                                                                           \
  }
/* Makro fuer elementweises Durchlaufen einer linearen Liste mit Hilfe einer abweisenden Schleife. Die Listenzel-
   len muessen mit Eintraegen namens "Nachf" verzeigert sein.                                                      */

#define min(x,y)                                                                                                    \
  ( ((x)<(y)) ? (x) : (y) )

#define max(x,y)                                                                                                    \
  ( ((x)>=(y)) ? (x) : (y) )

#define /*VergleichsT*/ IntegerVergleich(/*int*/ a, /*int*/ b)                                                      \
  ((a)>(b) ? Groesser : ((a)==(b) ? Gleich : Kleiner))
/* Vergleicht a und b und liefert das Ergebnis als VergleichsT zurueck */

#define /*unsigned long int*/ ptr2ul(/*void**/ Zeiger)                                                              \
  (*(uintptr_t *)(void*)&(Zeiger))
/* Einigermassen ANSI-konforme Umwandlung von Adressen zu unsigned long int; geht nur fuer L-values! */

#define /*unsigned int*/ minKXgrY(/*unsigned int*/ x, /*unsigned int*/ y)                                           \
  ((x)*((y)/(x)+1))

#define /*unsigned int*/ minKXgrglY(/*unsigned int*/ x, /*unsigned int*/ y)                                         \
  ((y)%(x) ? minKXgrY(x,y) : (y))


/*********************************  PROZEDUREN  *****************************************/

char *our_strnchr(const char *cs, char c, int n);
/* liefert Zeiger auf das erste c in den ersten n Zeichen von cs oder NULL, falls nicht vorhanden */

unsigned int LaengeNatZahl(unsigned long int n);

void Maximieren(unsigned int *AltesMaximum, unsigned int KandidatNeuesMaximum);
/* *AltesMaximum := max(*AltesMaximum,KandidatNeuesMaximum) */

void MaximierenLong(unsigned long int *AltesMaximum, unsigned long int KandidatNeuesMaximum);
/* *AltesMaximum := max(*AltesMaximum,KandidatNeuesMaximum) */

void MaximierenLLong(unsigned long long int *AltesMaximum, unsigned long long int KandidatNeuesMaximum);
/* *AltesMaximum := max(*AltesMaximum,KandidatNeuesMaximum) */

void MaximumAddieren(unsigned int *AlteSumme, unsigned int KandidatMaximum1, unsigned int KandidatMaximum2);
/* *AlteSumme := *AlteSumme + max(KandidatMaximum1,KandidatMaximum2) */

unsigned long int Maximum(int anz, ...);
/* Berechnet das Maximum der Argumentliste. Dabei muss das erste Argument die Anzahl der folgenden Argumente angeben,
   die Argumente selbst muessen unsigned long int sein. */

void Minimieren(unsigned int *AltesMinimum, unsigned int KandidatNeuesMinimum);
/* *AltesMinimum := max(*AltesMinimum,KandidatNeuesMinimum) */

unsigned int fac(unsigned int n);
/* Fakultaet */

BOOLEAN WerteDa(int anz, ...);
/* Gibt TRUE zurueck, wenn einer der anz uebergebenen Werte > 0 ist.
   Die Werte sind vom Typ unsigned long int. */

void chop (char *str);
/* Loeschen des letzten Zeichens */

#ifdef SUNOS4
#define our_sprintf(arg) strlen(sprintf arg)
#else
#define our_sprintf(arg) sprintf arg
#endif

BOOLEAN IstPrim(unsigned int n);
/* Ist n eine Primzahl? */

unsigned int PrimzahlNr(unsigned int n);
/* Liefert die Primzahl Nr. n, gezaehlt ab 0. Daher PrimzahlNr(0)=2. */

void BuildArgv (char *opt_str, char *argv0, int *argc, char **argv[]);
/* Umwandeln eines durch Leerzeichen getrennten Strings (Achtung: keine doppelten Leerzeichen)
   in argc/argv fuer Programmaufrufe
   argv0 enthaelt den Programmnamen
   argv wird mit our_dealloc angelegt
*/
  
char *our_strdup(const char *inp);

typedef enum{linkeSeite, rechteSeite} SeitenT;


/********************************** WM for Power Users ***************************/

#if COMP_POWER
#define normal_power(normal, power) power
#else
#define normal_power(normal, power) normal
#endif

#if COMP_POWER
# define normal_power_comp(normal, power, comp) power
#elif COMP_COMP
# define normal_power_comp(normal, power, comp) comp
#else
# define normal_power_comp(normal, power, comp) normal
#endif


/********************************** Paarungsfunktionen ***************************/

#define DefinePairing(/*{"short","","long"}*/ Length, /*{"s_","_","l_"}*/ Prefix)                                   \
unsigned Length int u ## Prefix ##pair(unsigned Length int x, unsigned Length int y);                               \
unsigned Length int u ## Prefix ##unpair_x(unsigned Length int n);                                                  \
unsigned Length int u ## Prefix ##unpair_y(unsigned Length int n);                                                  \
signed Length int s ## Prefix ##pair(signed Length int x, signed Length int y);                                     \
signed Length int s ## Prefix ##unpair_x(signed Length int n);                                                      \
signed Length int s ## Prefix ##unpair_y(signed Length int n);

DefinePairing(short,s_)
DefinePairing(,_)
DefinePairing(long,l_)



/*********************************** Optimierter Inline-Code fuer GCC ************/

#if  defined (__GNUC__) && !defined(__clang__)
extern __inline__ void Maximieren (unsigned int *AltesMaximum, unsigned int KandidatNeuesMaximum)
{
  if (KandidatNeuesMaximum>*AltesMaximum)
    *AltesMaximum = KandidatNeuesMaximum;
}

extern __inline__  void MaximierenLong (unsigned long int *AltesMaximum,
					unsigned long int KandidatNeuesMaximum)
{
  if (KandidatNeuesMaximum>*AltesMaximum)
    *AltesMaximum = KandidatNeuesMaximum;
}

extern __inline__ void MaximumAddieren (unsigned int *AlteSumme, unsigned int KandidatMaximum1,
					unsigned int KandidatMaximum2)
{
    *AlteSumme += max(KandidatMaximum1, KandidatMaximum2);
}

  
#endif

#endif /* GENERAL_H */
