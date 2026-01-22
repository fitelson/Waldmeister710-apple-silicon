/*=================================================================================================================
  =                                                                                                               =
  =  LPO.h                                                                                                        =
  =                                                                                                               =
  =  Realisation von Test- und Entscheidungsfunktion fuer LPO                                                     =
  =                                                                                                               =
  =                                                                                                               =
  =  11.03.94 Thomas                                                                                              =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef LPO_H
#define LPO_H

#include "general.h"
#include "TermOperationen.h"

BOOLEAN LP_LPOGroesser(UTermT Term1, UTermT Term2);
  
VergleichsT LP_LPO(UTermT Term1, UTermT Term2);

#endif
