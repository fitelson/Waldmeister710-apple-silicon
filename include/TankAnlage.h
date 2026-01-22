/* **************************************************************************
 * TankAnlage.h
 * 
 * 
 * Jean-Marie Gaillourdet, 10.09.2003
 *
 * ************************************************************************** */
#ifndef TANKANLAGE_H
#define TANKANLAGE_H

#include "Id.h"

#include "RUndEVerwaltung.h"
#include "Tankstutzen.h"

/* Callback für Unifikation */
void KPV_insertCP(TermT lhs, TermT rhs, AbstractTime i, AbstractTime j, Position xp);


void TA_Expandiere( Id id );
void TA_IROpferMelden( RegelOderGleichungsT opfer );
void TA_insertCP(TermT lhs, TermT rhs, AbstractTime i, AbstractTime j, Position xp);

void TA_TankstutzenAnmelden( TankstutzenT t );
void TA_TankstutzenAbmelden( TankstutzenT t );

void TA_Aufrauemen(void);

#endif /* TANKANLAGE_H */
