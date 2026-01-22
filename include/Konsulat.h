/*=================================================================================================================
  =                                                                                                               =
  =  Konsulat.h                                                                                                   =
  =                                                                                                               =
  =                                                                                                               =
  =  25.10.98  Thomas.                                                                                            =
  =                                                                                                               =
  =================================================================================================================*/


#ifndef KONSULAT_H
#define KONSULAT_H

#include "general.h"
#include "TermOperationen.h"

extern BOOLEAN KO_Mehrstufig;
extern BOOLEAN KO_KonsultationErforderlich;
extern unsigned int KO_Stufe;
extern TermT KO_Kern,
             KO_Fixpunktkombinator;


void KO_InitApriori(void);
void KO_InitAposteriori(void);

void KO_Konsultieren(void);

#endif
