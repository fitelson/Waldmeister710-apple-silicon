/*=================================================================================================================
  =                                                                                                               =
  =  Unifikation1.h                                                                                               =
  =                                                                                                               =
  =                                                                                                               =
  =                                                                                                               =
  =  07.10.94  Thomas.                                                                                            =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef UNIFIKATION1_H
#define UNIFIKATION1_H

#include "general.h"
#include "TermOperationen.h"
#include "RUndEVerwaltung.h"


/*=================================================================================================================*/
/*= I. Variablenbank fuer Substitutionen; Stack angelegter Bindungen; sonstige globale Groessen                   =*/
/*=================================================================================================================*/

void U1_VariablenfelderAnpassen(SymbolT NeuesteBaumvariable);
/* Variablenbank und Stapel angelegter Bindungen so anpassen,
   dass Variablen x(1), x(2), ... , x(2*BO_NeuesteBaumvariable()) verdaut werden */


/*=================================================================================================================*/
/*= II. Operationen auf oder mit Substitutionen                                                                   =*/
/*=================================================================================================================*/

void U1_DruckeMGU(void);


/*=================================================================================================================*/
/*= III. Operationen zum flachen Unifizieren                                                                      =*/
/*=================================================================================================================*/

BOOLEAN U1_Unifiziert(UTermT Term1, UTermT Term2);
/* Sind Term1 und Term2 unifizierbar? Aufzurufen nur dann, wenn xU trivial */


/*=================================================================================================================*/
/*= XVI. KP-Bildung                                                                                               =*/
/*=================================================================================================================*/

void U1_KPsBildenZuFaktum(RegelOderGleichungsT Faktum);

void U1_KPsBildenZuZweiFakten(RegelOderGleichungsT Faktum1, RegelOderGleichungsT Faktum2);

/*=================================================================================================================*/
/*= XVIII. Rekonstruktion von kritischen Paaren                                                                   =*/
/*=================================================================================================================*/

void U1_KPRekonstruieren(RegelOderGleichungsT Vater, unsigned int Stellennummer, RegelOderGleichungsT Mutter,
/* Die Stellennummer wird ab Null gezaehlt. */                                      TermT *KPLinks, TermT *KPRechts);

void U1_KPAusTermenRekonstruieren(TermT VaterLinks, TermT VaterRechts, TermT MutterLinks, TermT MutterRechts, 
   unsigned int Stellennummer, TermT *KPLinks, TermT *KPRechts);                           /* dito  ohne Termpaare */

void U1_CC_KPRekonstruieren(RegelOderGleichungsT vater, Position pos, BOOLEAN rechts,
                            TermT *KPLinks, TermT *KPRechts, RegelOderGleichungsT* ACGleichung);

/*=================================================================================================================*/
/*= XIX. Mass fuer relative Unifizierbarkeit                                                                      =*/
/*=================================================================================================================*/

unsigned long int U1_Unifikationsmass(TermT links, TermT rechts);
/* links und rechts muessen vom Typ TermT sein. Rueckgabe 0 <=> Terme sind unifizierbar. */


/*=================================================================================================================*/
/*= XXI. Initialisierung                                                                                          =*/
/*=================================================================================================================*/

void U1_InitApriori(void);

void U1_InitAposteriori(void);


BOOLEAN U1_KPKonstruiert(RegelOderGleichungsT Vater, unsigned int Stellennummer, 
			 RegelOderGleichungsT Mutter, TermT *Links, TermT *Rechts);


#endif
