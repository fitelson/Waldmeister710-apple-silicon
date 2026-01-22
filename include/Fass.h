/* **************************************************************************
 * Fass.h 
 * 
 * Jedes Fass steht für eine Instanz, der Menge P sortiert nach einer 
 * individuellen Heuristik. Damit es zur Selektion von Fakten genutzt werden 
 * kann, stellt es die Schnittstelle Zapfhahn zur Verfügung. Zum Empfangen von
 * neuen kritischen Paaren wird die Schnittstelle Tankstutzen implementiert.
 * 
 * Jean-Marie Gaillourdet, 02.09.2003
 *
 * ************************************************************************** */

#ifndef FASS_H
#define FASS_H


#include "ClasHeuristics.h"
#include "Tankstutzen.h"
#include "Zapfhahn.h"

typedef struct FassS *FassT;
typedef enum {CP, CG} HeapType;


/* **************************************************************************
 * FA_mkFass()
 * 
 * Erstelle ein neues Fass.
 * 
 * TODO: Parameterübergabe zur Auswahl der Heuristik
 * ************************************************************************** */
FassT FA_mkFass(HeapType type, ClassifyProcT classifcation);

/* **************************************************************************
 * FA_mkZapfhahn( FassT fass, ZapfhahnT z )
 *
 * Zu dem übergebenen Fass wird ein Zapfhahn erstellt, der in z abgespeichert 
 * wird.
 * ************************************************************************** */
ZapfhahnT FA_mkZapfhahn( FassT fass );

/* **************************************************************************
 * FA_mkTankstutzen( FassT fass, TankstutzenT t )
 *
 * Zu dem übergebenen Fass wird ein Tankstutzen erstellt, der in t
 * abgespeichert wird.
 * ************************************************************************** */
TankstutzenT FA_mkTankstutzen( FassT fass );

/* Im uebergebenen Fass wird die Klassifikation umgestellt. */

void FA_changeClass(FassT fass, ClassifyProcT classifcation);

#endif /* FASS_H */
