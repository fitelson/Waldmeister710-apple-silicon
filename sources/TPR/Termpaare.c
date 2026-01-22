/*=================================================================================================================
  =                                                                                                               =
  =  Termpaare.c                                                                                                  =
  =                                                                                                               =
  =  Realisierung von Termtupeln                                                                                  =
  =                                                                                                               =
  =                                                                                                               =
  =  22.03.95 Thomas.                                                                                             =
  =                                                                                                               =
  =================================================================================================================*/


#include "Termpaare.h"

SV_SpeicherManagerT TP_FreispeicherManager;

void TP_InitApriori(void)
{
  SV_ManagerInitialisieren(&TP_FreispeicherManager,struct GNTermpaarTS,TP_AnzRegelnJeBlock,"Regeln/Gleichungen");
}
