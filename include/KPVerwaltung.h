/* *************************************************************************
 * KPVerwaltung.h
 *
 * Modul zur Verwaltung der passiven Fakten.
 *
 * Bernd Loechner, 21.04.2001
 *
 * ************************************************************************* */
/**
 * ALLES, WAS IN DIESEM MODUL NICHT MIT KPV ANFAENGT, WIRD ALS MOMENTANE
 * "WART" BETRACHTET.
 **/

#ifndef KPVERWALTUNG_H
#define KPVERWALTUNG_H

#include "general.h"
#include "ClasHeuristics.h"
#include "RUndEVerwaltung.h"
#include "TermOperationen.h"
#include "Position.h"

/* --------------------------------------------------------------------------*/
/* ----- Typedefs -----------------------------------------------------------*/
/* --------------------------------------------------------------------------*/

typedef struct{
  WeightT w1, w2;
} WeightEntryT;

/* Reihenfolge der Werte nicht aendern - wird in NewClassification benoetigt */
typedef enum{CPWithFather, CPWithMother, Initial, KilledRE, 
	       ChaseManhattanCP, TWreceived, CPHistory_Last
} CPHistoryT;


/* --------------------------------------------------------------------------*/
/* ----- PARENTS ------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/

/* Altlast */
void KPV_NewParent(RegelOderGleichungsT re);
/* Altlast */
void KPV_FinishParent(RegelOderGleichungsT re);

/* Waisenmord 
 * alle Kinder von re sind zu entsorgen */
void KPV_KillParent(RegelOderGleichungsT re);

/* --------------------------------------------------------------------------*/
/* ----- SELECTION ----------------------------------------------------------*/
/* --------------------------------------------------------------------------*/

/* True, wenn der uebergebene Eintrag schon einmal durch die Heuristic geliefert wurde
 * Verwendung bisher ausschliesslich in der KPV V08 (vgl FIFO)*/
BOOLEAN KPV_alreadySelected(AbstractTime i, AbstractTime j, Position xp, BOOLEAN isCP, TermT linkeSeite, TermT rechteSeite);

/* getMin */
BOOLEAN KPV_Select(SelectRecT *selRec);

/* zu eliminieren */
void SUE_selectedCPRE(SelectRecT *selRec);

/* --------------------------------------------------------------------------*/
/* ----- KPs in KP-Menge aufnehmen ------------------------------------------*/
/* --------------------------------------------------------------------------*/

/* Axiom aufnehmen */
void KPV_Axiom(TermT lhs, TermT rhs);

/* ASet aufnehmen */
void KPV_insertASet(AbstractTime i);


/* Interreduktionsopfer wieder in P einfügen (evtl Boolean ob links oder rechts; nur für die Überholspur gedacht)
 * Diese gehören evtl. auf die Überholspur (alle,keine, oder nur die rechtsreduzierten)
 */
void KPV_IROpferBehandeln (RegelOderGleichungsT opfer);

/* --------------------------------------------------------------------------*/
/* ----- RECONSTRUCTION -----------------------------------------------------*/
/* --------------------------------------------------------------------------*/

void KPV_reconstruct(WId* wid, SelectRecT* selRec, BOOLEAN normalize);

/* --------------------------------------------------------------------------*/
/* ---- CLEANING WOMAN ------------------------------------------------------*/
/* --------------------------------------------------------------------------*/

/* wenn das Programm beendet wird */
void KPV_CLEANUP(void);

/* sollte mit KPV_PassiveFaktenWegwerfen zusammengefasst werden */
void KPV_SOFTCLEANUP(void);

/* P komplett leer räumen, als  wäre es neu */
void KPV_PassiveFaktenWegwerfen(void);

/* --------------------------------------------------------------------------*/
/* ----- Initialisierung/parameter ------------------------------------------*/
/* --------------------------------------------------------------------------*/

void KPV_InitAposteriori (void);

unsigned long int KPH_LebendeKPsZaehlen(void);

void KPV_CGHeuristikUmstellen(ClassifyProcT h);

#endif
