/**********************************************************************
* Filename : ClasPatch
*
* Kurzbeschreibung : Classificationsheuristiken - Spielwiese
*                    
* Autoren : Andreas Jaeger
*
* 25.03.98: Erstellung
*
*
*
**********************************************************************/

#ifndef CLASPATCH
#define CLASPATCH

#include "Id.h"

void CPA_CPInit (int no);

void CPA_CGInit (int no);

WeightT CPA_CPPatch (IdMitTermenT tid);

WeightT CPA_CGPatch (IdMitTermenT tid);

#endif 
