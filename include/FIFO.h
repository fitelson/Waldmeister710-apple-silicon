/* *************************************************************************
 * FIFO.h
 *
 * Modul zum Abarbeiten der FIFO Strategie auf den passiven Fakten. Es wird
 * immer nur der aktuelle Stand gespeichert.
 *
 * Bernd Loechner, Jean-Marie Gaillourdet 13.5.2003
 *
 * ************************************************************************* */

#ifndef FIFO_H
#define FIFO_H

#include "Position.h"
#include "RUndEVerwaltung.h"
#include "Zapfhahn.h"

/* TachoS speichert immer die letzte zurückgelieferte Regel/Gleichung */
struct TachoS {
  /* Tacho verwaltet TRUE => kritische Paare, FALSE => kritische Ziele */
  BOOLEAN modusCP;  
 
  /* für i = 0 bezeichnet j das aktuelle Axiom */
  AbstractTime i; 
  AbstractTime j;
  Position xp; 
  unsigned long pos_max; 
  BOOLEAN new_initialized;
  AbstractTime startZeitpunkt;
  unsigned long int anzahlBekannterAxiome;
  BOOLEAN blockAlreadySelected;
} Tacho;

typedef struct TachoS *TachoT;

/* Erstellt einen neuen Tacho
 * modusCP TRUE  => Tacho zählt kritische Paare auf
 *         FALSE => Tacho zählt kritische Ziele auf
 */
ZapfhahnT FIFO_mkTacho( BOOLEAN modusCP );


#endif
