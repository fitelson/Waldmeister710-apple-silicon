#ifndef HAUPTKOMPONENTEN_H
#define HAUPTKOMPONENTEN_H

#include "general.h"
#include "TermOperationen.h"
#include "XFilesFuerPMh.h"

typedef enum {
  HK_VervollAbgebrochen, HK_Vervollstaendigt, HK_ZielBewiesen, 
  HK_StrategieWechseln, HK_Ueberholt, HK_Beendet
} HK_ErgebnisT;
  
extern BOOLEAN HK_fair;
extern AbstractTime HK_AbsTime;
extern AbstractTime HK_TurnTime;
  

#define HK_fair() HK_fair
#define HK_SetUnfair() (HK_fair = FALSE)

#define HK_getAbsTime_M() HK_AbsTime
#define HK_setAbsTime_M(val) (HK_AbsTime = (val))


void HK_GleichungenUndZieleHolen(void);

BOOLEAN HK_ZieleAufZusammenfuehrbarkeitTesten(void);

BOOLEAN HK_TestStrategieWechsel (void);

VergleichsT HK_VariantentestUndVergleich(TermT links, TermT rechts);

HK_ErgebnisT HK_Vervollstaendigung(StrategieT *Strategie);

HK_ErgebnisT HK_SlaveLoop(void);

#endif
