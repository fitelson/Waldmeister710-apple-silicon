/* **************************************************************************
 * Zapfhahn.h  Schnittstelle für einen Zapfen, der Fakten produziert.
 * 
 * Zapfen sind die einheitliche Schnittstelle zum Zugriff auf Fakten, die 
 * selektiert werden. Mögliche Quellen für diese sind: Axiome, P nach 
 * verschiedenen Heuristiken und der FIFO.
 * 
 * Jean-Marie Gaillourdet, 18.08.2003
 *
 * ************************************************************************** */

#ifndef ZAPFHAHN_H
#define ZAPFHAHN_H

#include "general.h"
#include "Id.h"
#include "Position.h"
#include "RUndEVerwaltung.h"

typedef struct ZapfhahnS *ZapfhahnT;

/* **************************************************************************
 * Selektiere
 *
 * Der Zapfhahn selektiert das nächste Faktum entsprechend seiner
 * Heuristik oder seines sonstigen Mechanismus zur Auswahl der Fakten.
 **************************************************************************** */
typedef BOOLEAN (*ZH_Selektiere)(ZapfhahnT z, SelectRecT* selRec);


/* **************************************************************************
 * BereitsSelektiert
 * 
 * Der Zapfhahn prüft, ob das beschriebene Faktum von diesem bereits
 * einmal geliefert wurde oder nicht.
 **************************************************************************** */
typedef BOOLEAN (*ZH_BereitsSelektiertWId)(ZapfhahnT z, WId* id);

/* **************************************************************************
 * BereitsSelektiert
 * 
 * Der Zapfhahn prüft, ob das beschriebene Faktum von diesem bereits
 * einmal geliefert wurde oder nicht.
 **************************************************************************** */
typedef BOOLEAN (*ZH_BereitsSelektiert)(ZapfhahnT z, IdMitTermenT tid);


/* **************************************************************************
 * Freigeben
 * 
 * Alle Datenstrukturen des Zapfhahn werden freigegeben. 
 * ************************************************************************** */
typedef void (*ZH_Freigeben)(ZapfhahnT z);

struct ZapfhahnS {
  ZH_Selektiere selektiere;
  ZH_BereitsSelektiert bereitsSelektiert;
  ZH_BereitsSelektiertWId bereitsSelektiertWId;
  ZH_Freigeben freigeben;
  void* privat; /* hier darf jeder Zapfhahn seine eigenen Daten verwalten. */
};

#endif /* ZAPFHAHN_H */
