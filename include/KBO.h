/*

07.03.94 Angelegt. Arnim.

*/

#ifndef KBO_H
#define KBO_H

/* ------------------------------------------------*/
/* ------------- Includes -------------------------*/
/* ------------------------------------------------*/


#include "general.h"
#include "TermOperationen.h"

/* -------------------------------------------------------*/
/* --------------- Initialisieren ------------------------*/
/* -------------------------------------------------------*/

/* void KB_InitApriori(void); */

void KB_InitAposteriori(void);
/* Richtet den Speicher-Manager ein. */
/* Testet, ob die Funktionsgewichte der Definition entsprechend eingestellt sind. */

extern struct KB_FliegengewichtS {
  BOOLEAN Vorhanden;  /* Gibt es ein einstelliges praezedenzmaximales nullgewichtiges Funktionssymbol? */
  SymbolT Symbol;     /* Wenn ja, welches? */
} KB_Fliegengewicht;
 
/* -------------------------------------------------------*/
/* --------------- Aufrufe -------------------------------*/
/* -------------------------------------------------------*/

BOOLEAN KB_KBOGroesser(UTermT term1, UTermT term2);

VergleichsT KB_KBOVergleich(UTermT term1, UTermT term2);
/* Entscheidungsfunktion, die das Groessenverhaeltnis von term1 und term2 nach der KBO bestimmt. */

BOOLEAN KB_KBOGroesser2(UTermT term1, UTermT term2);

VergleichsT KB_KBOVergleich2(UTermT term1, UTermT term2);
/* Entscheidungsfunktion, die das Groessenverhaeltnis von term1 und term2 nach der KBO bestimmt. */
#endif
