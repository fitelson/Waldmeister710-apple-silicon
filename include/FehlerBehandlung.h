/* Datei: error_handling.h
 * Zweck: Stellt vier verschiedene Behandlungsm"oglichkeiten f"ur Fehler zur Verf"ugung
 * Autor: Arnim Buch
 * Stand: 28.02.94
 * Ersterstellung: 28.02.94.  ACHTUNG: Bisher geben alle Prozeduren nur die Meldungen aus
 *                                     und verabschieden sich.
 * Aenderungen:
 * 
 * 
 * 
 * 
 * 
 * 
 */

#ifndef FEHLERBEHANDLUNG_H
#define FEHLERBEHANDLUNG_H


#include "general.h"

/* -------------------------------------------------------------------------------*/
/* --------------- Allgemeine Fehlermeldungen  -----------------------------------*/
/* -------------------------------------------------------------------------------*/

void our_assertion_failed (const char *hoppala);
/* Bringt zum Ausdruck, wenn eine eigentlich unm"ogliche Konstellation erreicht wird.
   (Implementierungs-Fehler). */

void our_warning(const char *autsch);
/* Gibt einfach den uebergebenen Text aus und f"ahrt im Programm fort. */

void our_error(const char *aua);
/* Gibt die angegebene Fehlermeldung aus und gibt die Macht an den Benutzer zurueck.*/

void our_out_of_mem_error(const char *aua);
/* Gibt die angegebene Fehlermeldung aus und gibt die Macht an den Benutzer zurueck.
   Spezialfall: Kein Speicher frei
 */

void our_fatal_error(const char *aechhh);
/* Gibt die Fehlermeldung aus und verabschiedet sich aus dem Programm. */

void not_yet_implemented(const char *functionName);
/* Gibt "Not yet implemeted: <function_name> aus, und TSCHUEEESSSSSSSSS */

#endif /* FEHLERBEHANDLUNG_H */
