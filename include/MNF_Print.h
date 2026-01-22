/**********************************************************************
* Filename : MNF_Print.h
*
* Kurzbeschreibung : MultipleNormalFormen-Bildung: Ausgabefunktionen
*
* Autoren : Andreas Jaeger
*
* 18.11.96: Erstellung
*
**********************************************************************/

#ifndef MNF_PRINT_H
#define MNF_PRINT_H

#include "general.h"
#include "RUndEVerwaltung.h"
#include "TermOperationen.h"
#include "MNF.h"

/* Ausgaben */
/* -------- */

/* Ausgeben aller neuen Elemente. Jedes ausgegebene Element wird als
   alt erklaert, so dass jedes Element nur einmal ausgegeben wird. Die
   urspruenglichen Elemente werden nicht ausgeben.  Parameter:
   leftStr, rightStr: Strings, die vor jedem Element ausgegeben werden
   title wird ausgeben, wenn es neue Element gibt. Direkt nach title
   werden die Terme t1 und t2 ausgegeben
*/
void MNF_PrintMengeNew (struct MNF_MengeTS *menge, 
                        const char *leftStr, const char *rightStr,
			const char *title, TermT t1, TermT t2);

/* Ausgeben aller Elemente, Paramater leftStr, rightStr wie
   MNF_PrintMengeNew */
void MNF_PrintMengeComplete (struct MNF_MengeTS *menge, 
                             const char *leftStr, const char *rightStr);

/* Fuer Debugging: */
void MNF_PrintMenge (struct MNF_MengeTS *menge);

/* Ausgeben von minimaler Statistik */
void MNF_PrintMengeStatShort (struct MNF_MengeTS *menge);

/* Ausgeben von erweiterter Statistik */
void MNF_PrintMengeStatLong (struct MNF_MengeTS *menge);

/* Ausgeben des gejointen Elementes und aller Vorfahren  */
void MNF_PrintJoinedComplete (struct MNF_MengeTS *menge);

/* Ausgeben des gejointen Elementes  */
void MNF_PrintJoinedShort (struct MNF_MengeTS *menge);

void MNF_PrintJoinedBeweis (struct MNF_MengeTS *menge, unsigned long nummer);

struct MNF_ColTermTS;
/* Ausgabe eines CTerms mit allen Informationen */
void MNF_PrintColTerm (struct MNF_ColTermTS *colTermPtr);

#endif
