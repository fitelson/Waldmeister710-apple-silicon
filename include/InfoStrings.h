/* -------------------------------------------------------------------------------------------------------------*/
/* -                                                                                                            */
/* - InfoStrings.h                                                                                              */
/* - *************                                                                                              */
/* -                                                                                                            */
/* - Parameter-Uebergabe als String, in dem die einzelnen Daten durch ':' getrennt werden.                      */
/* -                                                                                                            */
/* - 14.03.96 Arnim. Angelegt.                                                                                  */
/* -                                                                                                            */
/* -                                                                                                            */
/* -------------------------------------------------------------------------------------------------------------*/
#ifndef INFOSTRINGS_H
#define INFOSTRINGS_H

#include "general.h"

typedef char *InfoStringT;

BOOLEAN IS_IsIn(const char *str, InfoStringT is);
/* str ist ein Teil von is, also is = ...:str:.. */

char *IS_ValueOf(const char *str, InfoStringT is);
/* Liefert val, falls is = ...:str=val:..., und NULL sonst. */



int IS_FindOption(InfoStringT defaultIS, InfoStringT userIS, int n, ...);
/* sucht zunaechst in userIS, dann in defaultIS nach den n angegebenen searchStrings.
   Die Nummer des zuerst gefundenen searchStrings wird zurueckgeliefert (beginnend bei 0).
   Wurde keiner gefunden, wird -1 zurueckgegeben.
*/

int IS_FindValuedOption(InfoStringT defaultIS, InfoStringT userIS, int n, ...);
/* wie vor, aber mit Wert-Optionen. */

char *IS_ValueOfOption(InfoStringT defaultIS, InfoStringT userIS, const char *str);
/* Gibt den Wert der zuvor mit IS_FindValuedOption identifizierten gueltigen Option zurueck.
   Im Fehlerfall wird NULL returniert.
*/

#endif
