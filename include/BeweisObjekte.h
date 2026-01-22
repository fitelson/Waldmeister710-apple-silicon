#ifndef BEWEISOBJEKTE_H
#define BEWEISOBJEKTE_H

#include "general.h"
#include "RUndEVerwaltung.h"
#include "TermOperationen.h"

/* Fuer NFBildung: */
extern BOOLEAN Bob_do_callback;
void Bob_NFCallback(TermT ergebnis, UTermT betroffenerTeilterm, 
                    RegelOderGleichungsT angewendetesObjekt);

#define /*void*/ BOB_NFCallback(/*TermT*/ ergebnis,                          \
                                /*UTermT*/ betroffenerTeilterm,              \
                                /*RegelOderGleichungsT*/ angewendetesObjekt) \
     if (Bob_do_callback){                                                   \
       Bob_NFCallback(ergebnis, betroffenerTeilterm, angewendetesObjekt);    \
     }                                                                       \


/* Fuer MNF-Ausgaben: */
void BOB_UrspruenglichesZiel(TermT lhs, TermT rhs, unsigned long nummer);
void BOB_ZielBewiesen(TermT lhs, TermT rhs);
void BOB_PrepareNF(TermT lhs, TermT rhs, AbstractTime time);
void BOB_ClearNF(void);
void BOB_RedStep(TermT term, TermT andererTerm, RegelOderGleichungsT objekt, 
                 SeitenT seite, UTermT Stelle, SeitenT reseite);

void BOB_BeweisFuerNormalesZiel(TermT lhs, TermT rhs, unsigned long nummer, 
                                unsigned int notimes, AbstractTime *times);

void BOB_BeweisFuerAktivum(RegelOderGleichungsT re);
void BOB_BeweiseFuerAktiva(void);

void BOB_DruckeEndsystemFuerBeweisObjekt(void); 
/* ggf. Abschlussfakten drucken */

void BOB_SetScanMode(void);
void BOB_SetPrintMode(void);

void BOB_InitAposteriori(void);
void BOB_Beenden(void);

#endif
