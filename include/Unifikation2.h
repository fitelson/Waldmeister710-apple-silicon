/*=================================================================================================================
  =                                                                                                               =
  =  Unifikation2.h                                                                                               =
  =                                                                                                               =
  =                                                                                                               =
  =                                                                                                               =
  =  04.11.97  Thomas.                                                                                            =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef UNIFIKATION2_H
#define UNIFIKATION2_H

#include "SymbolOperationen.h"
#include "TermOperationen.h"


/*=================================================================================================================*/
/*= I. Variablenbaenke fuer Substitutionen                                                                        =*/
/*=================================================================================================================*/

void U2_VariablenfelderAnpassen(SymbolT NeuesteVariable);
void U2_FunktionsvariablenEintragen(void);


/*=================================================================================================================*/
/*= II. Operationen auf oder mit Substitutionen                                                                   =*/
/*=================================================================================================================*/

void U2_SubstAufIdentitaetSetzen(void);

extern unsigned int U2_AnzBindungen;
#define /*unsigned int*/ U2_AnzBindungen()                                                                          \
  U2_AnzBindungen

void U2_BindungenAufhebenBis(unsigned int AnzBindungen);

void U2_DruckeMGU(void);

BOOLEAN U2_VarGebunden(SymbolT x);

/*=================================================================================================================*/
/*= III. Unifizieren                                                                                              =*/
/*=================================================================================================================*/

BOOLEAN U2_WeiterUnifiziert(UTermT Term1, UTermT Term2); 
/* Die Terme werden nicht als variablendisjunkt vorausgesetzt. */

BOOLEAN U2_DoppeltWeiterUnifiziert(UTermT Term1, UTermT Term2,  UTermT Term3, UTermT Term4);
/* Term1=?Term2 und Term3=?Term4 */

#define /*BOOLEAN*/ U2_NeuUnifiziert(/*UTermT*/ Term1, /*UTermT*/ Term2)                                            \
  (U2_SubstAufIdentitaetSetzen(), U2_WeiterUnifiziert(Term1,Term2))


/*=================================================================================================================*/
/*= IV. Erzeugung von Termkopien unter Substitution                                                               =*/
/*=================================================================================================================*/

TermT U2_MGU(TermT Term);

/*=================================================================================================================*/
/*= V. Initialisierung                                                                                            =*/
/*=================================================================================================================*/

void U2_InitApriori(void);

#endif
