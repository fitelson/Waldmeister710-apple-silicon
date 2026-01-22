/*=================================================================================================================
  =                                                                                                               =
  =  DSBaumOperationen.h                                                                                          =
  =                                                                                                               =
  =  Einfuegen und Loeschen von gerichtet normierten Termpaaren in Baeume vom Typ DSBaumT                         =
  =                                                                                                               =
  =                                                                                                               =
  =  22.03.94  Thomas.                                                                                            =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef DSBAUMOPERATIONEN_H
#define DSBAUMOPERATIONEN_H

#include "DSBaumKnoten.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "Termpaare.h"


/*=================================================================================================================*/
/*= I. Operationen fuer die Variablenumbenennung                                                                  =*/
/*=================================================================================================================*/

extern SymbolT BO_NeuesteBaumvariable;
#define /*SymbolT*/ BO_NeuesteBaumvariable()                                                                        \
  BO_NeuesteBaumvariable

extern unsigned int BO_MaximaleTiefe;

void BO_VariablenbaenkeAnpassen(SymbolT neuesteVariable);                 /* Das gehoert garantiert nicht hierher. */

void BO_TermpaarNormieren(TermT Term1, TermT Term2, unsigned int *TiefeTerm1);    /* Variablenumwandlung auf Term1 */
 /* durchfuehren und auf Term2 fortsetzen. Das Termende muss hier durch Vergleich mit NULL erkannt werden koennen. */

void BO_TermpaarNormierenAlsKP(TermT Term1, TermT Term2);
/* dito, aber nicht fuer die Aufnahme in den Baum -> keine Anpassungen in anderen Modulen */

unsigned int BO_AusdehnungKopierstapel(void);

unsigned int BO_AusdehnungVariablenbank(void);

unsigned int BO_AusdehnungSprungstapel(void);



/*=================================================================================================================*/
/*= VII. Einfuegen von Termen in einen Baum                                                                       =*/
/*=================================================================================================================*/

void BO_ObjektEinfuegen(DSBaumT *Baum, GNTermpaarT Regel, unsigned int TiefeLinkerSeite);
/* Objekt (Regel oder Gleichung) in Baum einfuegen. */



/*=================================================================================================================*/
/*= IX. Loeschen von Termen aus einem Baum                                                                        =*/
/*=================================================================================================================*/

void BO_ObjektEntfernen(DSBaumT *Baum, GNTermpaarT Regel);
/* Objekt aus dem Baum entfernen, jedoch nicht loeschen (auch Terme nicht). */



/*=================================================================================================================*/
/*= X. Initialisierung und Statistik                                                                              =*/
/*=================================================================================================================*/

void BO_DSBaumInitialisieren(DSBaumT *DSBaum);

void BO_InitApriori(void);

void BO_KnotenZaehlen(unsigned int *KnotenImBaum, unsigned int *KnotenGespart, DSBaumT Baum);

#endif
