/**********************************************************************
* Filename : ClasHeuristics
*
* Kurzbeschreibung : Heuristiken zur Classifikation
*
* Autoren : Andreas Jaeger
*
* 10.03.98: Erstellung
*
*
*
**********************************************************************/
#ifndef CLASHEURISTICS
#define CLASHEURISTICS

#include "Id.h"
#include "RUndEVerwaltung.h"
#include "TermOperationen.h"

/* Classifikationsroutinen haben alle folgende Aufrufsyntax:
   wtPtr: WeightEntryT
   lhs: TermT
   rhs: TermT
   actualParent: RegelOderGleichungsT
   otherParent: RegelOderGleichungsT
   actualPosition: UTermT : Position in actualParent?
Wo?-> CG/CP
   type ist ClassifyProcT
(WeightEntryT *wtPtr, TermT lhs, TermT rhs, RegelOderGleichungsT actualParent, RegelOderGleichungsT otherParent, UTermT actualPosition);
*/


typedef WeightT (* ClassifyProcT) (IdMitTermenT tid); 

WeightT CH_AddWeight (IdMitTermenT tid);
WeightT CH_OrdWeight (IdMitTermenT tid);
WeightT CH_GtWeight (IdMitTermenT tid);
WeightT CH_MixWeight (IdMitTermenT tid);
WeightT CH_MixWeight2 (IdMitTermenT tid);
WeightT CH_MaxWeight (IdMitTermenT tid);
WeightT CH_Unifikationsmass (IdMitTermenT tid);
WeightT CH_Fifo (IdMitTermenT tid);
WeightT CH_Golem (IdMitTermenT tid);
WeightT CH_PolyWeight (IdMitTermenT tid);
WeightT CH_PolyWeight2 (IdMitTermenT tid);

#endif
