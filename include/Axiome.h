/* **************************************************************************
 * Axiome.h  globaler Speicher für aller Axiome
 * 
 * Jean-Marie Gaillourdet, 08.12.2003
 *
 * ************************************************************************** */

#ifndef AXIOME_H
#define AXIOME_H

#include "TermOperationen.h"


/* **************************************************************************
 * AX_AddAxiome
 * 
 * Hinzufügen eines neuen Axiomes zu der Menge der global bekannten Axiome.
 *
 * ************************************************************************** */
void AX_AddAxiom(TermT linkeSeite, TermT rechteSeite);

/* **************************************************************************
 * AX_GetAxiom
 * 
 * Hole das Axiom Nummer axnummer. Die Nummern beginnen mit 1. Die Nummern 
 * werden in chronologischer Reihenfolge vergeben.
 * ************************************************************************** */
void AX_AxiomKopieren(unsigned long axnummer, TermT* linkeSeite, 
		      TermT* rechteSeite);

/* **************************************************************************
 * AX_AnzahlAxiome
 * 
 * Liefert die Anzahl der momentan bekannten Axiome. 
 * ************************************************************************** */
unsigned long AX_AnzahlAxiome( void );


void AX_Cleanup( void );
void AX_SoftCleanup( void );

#endif /* AXIOME_H */
