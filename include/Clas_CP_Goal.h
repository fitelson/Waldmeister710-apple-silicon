/**********************************************************************
* Filename : Clas_CP_Goal.h
*
* Kurzbeschreibung : CP-In-Goal und Goal-In-CP Classifikationen
*
* Autoren : Thomas Hillenbrand, Andreas Jaeger
*
* 10.03.98: Erstellung aus SH2.c heraus
*
*
*
**********************************************************************/

#ifndef CLAS_CP_GOAL
#define CLAS_CP_GOAL

#include "ClasHeuristics.h"
#include "RUndEVerwaltung.h"
#include "TermOperationen.h"
#include "general.h"

WeightT CPinGoal (IdMitTermenT tid);

WeightT GoalinCP (IdMitTermenT tid);


void CIGICInit(WeightT cigic_single_match, WeightT cigic_no_match, WeightT cigic_min_struct);

#endif
