/* **************************************************************************
 * TankStutzen.h  Schnittstelle für einen Zapfen, der Fakten produziert.
 * 
 * 
 * Jean-Marie Gaillourdet, 29.08.2003
 *
 * ************************************************************************** */

#ifndef TANKSTUTZEN_H
#define TANKSTUTZEN_H

#include "Id.h"

typedef struct TankstutzenS *TankstutzenT;


/* **************************************************************************
 * FaktenEinfügen 
 * 
 * Übergibt dem TankStutzen eine Menge von neuen Fakten, die dieser nach
 * seiner eigenen Entscheidung übernehmen oder verwerfen kann. Damit
 * das gesamte System aber vollständig bleibt, muss jedes Faktum von
 * mindestens einem TankStutzen übernommen werden.
 *
 * Der übergebene buffer darf von dem TankStutzen umsortiert werden. Dabei
 * dürfen jedoch keine Fakten verloren gehen oder hinzugefügt werden.
 * ************************************************************************** */
typedef void (*TS_FaktenEinfuegen)(TankstutzenT t, TermT l, TermT r, Id id ); 


/* **************************************************************************
 * IROpferEinfuegen( TankstutzenT t, RegelOderGleichunsT opfer )
 * 
 * Fuege das opfer in den Tankstutzen t ein.
 * ************************************************************************** */
typedef void (*TS_IROpferEinfuegen)(TankstutzenT t, IdMitTermenT tid);

/* **************************************************************************
 * Freigeben
 * 
 * Alle Datenstrukturen des Tankstutzen werden freigegeben. 
 * ************************************************************************** */
typedef void (*TS_Freigeben)(TankstutzenT t);

struct TankstutzenS {
    TS_FaktenEinfuegen faktenEinfuegen;
    TS_IROpferEinfuegen iROpferEinfuegen;
    TS_Freigeben freigeben;
    void* privat;
};
#endif /* TANKSTUTZEN_H */

