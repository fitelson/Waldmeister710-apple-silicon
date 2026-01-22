/*=================================================================================================================
  =                                                                                                               =
  =  VariablenMengen.h                                                                                            =
  =                                                                                                               =
  =  Funktionen bzw. Makros zur Darstellung von Variablenmengen als Bitsets                                       =
  =                                                                                                               =
  =  Zweck: Einsatz bei Vorpruefung der Terme in Ordnungen                                                        =
  =  Zur Verfuegung gestellt werden Einfuegen von Elementen in Mengen sowie Mengenvergleich.                      =
  =                                                                                                               =
  =  24.02.94 Thomas. Als separates Modul geschrieben.                                                            =
  =  28.02.94 Thomas. Ausfuehrlich getestet und fuer Include umgearbeitet.                                        =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef VARIABLENMENGEN_H
#define VARIABLENMENGEN_H

#include "general.h"
#include "SymbolOperationen.h"

#define VAR_SETS_UNLIMITED 1 /* 0, 1 */
#if VAR_SETS_UNLIMITED

/*=================================================================================================================*/
/*= 0. Typdeklarationen                                                                                           =*/
/*=================================================================================================================*/

typedef unsigned long int VMengenteilT;                                                       /* Einzelnes Bitfeld */

typedef struct VMengenTS {                                                   /* Typen fuer Listen ueber Bitfeldern */
  VMengenteilT Teil;
  struct VMengenTS *Nachf;
} VMengenT;                                                                    /* nach aussen sichtbarer Mengentyp */



/*=================================================================================================================*/
/*= I. Initialisierung                                                                                            =*/
/*=================================================================================================================*/

void VM_InitApriori(void);



/*=================================================================================================================*/
/*= II. Elementare Operationen auf Mengen und Elementen                                                           =*/
/*=================================================================================================================*/

#define /*void*/ VM_NeueMengeSeiLeer(/*VMengenT*/ Menge)                     /* initialisiert neue Menge als leer */\
  (Menge).Teil = (VMengenteilT) 0L;                                                                                 \
  (Menge).Nachf = NULL;

void VM_MengeLeeren(VMengenT *Menge);                                            /* fuer Mengen beliebiger Groesse */

void VM_MengeLoeschen(VMengenT *Menge);                                                      /* Speicher freigeben */

void VM_ElementEinfuegen(VMengenT *Menge, SymbolT Symbol);                           /* Element in Menge einfuegen */  

BOOLEAN VM_EnthaltenIn(VMengenT *Menge, SymbolT Symbol);                          /* Ist Symbol Element von Menge? */

BOOLEAN VM_NichtEnthaltenIn(VMengenT *Menge, SymbolT Symbol);               /* Ist Symbol nicht Element von Menge? */



/*=================================================================================================================*/
/*= III. Funktionen zum Vergleichen von Mengen                                                                    =*/
/*=================================================================================================================*/

BOOLEAN VM_Teilmenge(VMengenT *Menge1, VMengenT *Menge2);                  /* Test, ob Menge1 Teilmenge von Menge2 */

VergleichsT VM_MengenVergleich(VMengenT *Menge1, VMengenT *Menge2);           /* vierwertige Entscheidungsfunktion */


#else



/*=================================================================================================================*/
/*= 0. Typdeklarationen                                                                                           =*/
/*=================================================================================================================*/

typedef unsigned long int VMengenT;


/*=================================================================================================================*/
/*= I. Initialisierung                                                                                            =*/
/*=================================================================================================================*/

#define /*void*/ VM_InitApriori(/*void*/) /* nix tun */



/*=================================================================================================================*/
/*= II. Elementare Operationen auf Mengen und Elementen                                                           =*/
/*=================================================================================================================*/

#define /*void*/ VM_NeueMengeSeiLeer(/*VMengenT*/ Menge)  ((Menge) = 0L)

#define /*void*/ VM_MengeLeeren(/*VMengenT **/ Menge) ((*Menge) = 0L)

#define /*void*/ VM_MengeLoeschen(/*VMengenT **/ Menge) /* nix tun */

#define /*void*/ VM_ElementEinfuegen(/*VMengenT **/ Menge, /*SymbolT*/ Symbol) (*Menge |= (1L << (-Symbol)))

#define /*BOOLEAN*/ VM_EnthaltenIn(/*VMengenT **/ Menge, /*SymbolT*/ Symbol) ((BOOLEAN) (((*Menge) >> (-Symbol)) & 1L))

#define /*BOOLEAN*/ VM_NichtEnthaltenIn(/*VMengenT **/ Menge, /*SymbolT*/ Symbol)((BOOLEAN) ((((*Menge) >> (-Symbol)) & 1L) ^ 1L))

/*=================================================================================================================*/
/*= III. Funktionen zum Vergleichen von Mengen                                                                    =*/
/*=================================================================================================================*/

#define /*BOOLEAN*/ NichtKleinerGleich(/*VMengenT*/ Teil1, /*VMengenT*/ Teil2)                          \
  ((Teil1) & ~(Teil2))               /* testet, ob Bitfeld der 1. Menge Teilmenge eines Bitfelds der zweiten Menge */

#define /*BOOLEAN*/ VM_Teilmenge(/*VMengenT **/ Menge1, /*VMengenT **/ Menge2) (!NichtKleinerGleich(*Menge1,*Menge2))

#define /*VergleichsT*/ VM_MengenVergleich(/*VMengenT **/ Menge1, /*VMengenT **/ Menge2)	\
 ( VM_Teilmenge(Menge1,Menge2) ?									\
  (VM_Teilmenge(Menge2,Menge1) ? Gleich : Kleiner) :						\
  (VM_Teilmenge(Menge2,Menge1) ? Groesser : Unvergleichbar))

#endif

#endif
