/* -------------------------------------------------------------------------------------------------------------*/
/* -                                                                                                            */
/* - NFBildung.h                                                                                                */
/* -                                                                                                            */
/* - Enthaelt die Operationen zur Normalform-Bildung.                                                            */
/* -                                                                                                            */
/* -------------------------------------------------------------------------------------------------------------*/

#ifndef NFBILDUNG_H
#define NFBILDUNG_H

#include "general.h"
#include "InfoStrings.h"
#include "TermOperationen.h"
#include "RUndEVerwaltung.h"


/*-------------------------------------------------------------------------------------------------------------------------*/
/*------ Anwendbarkeit eines bestimmten Objektes irgendwo auf einen Term --------------------------------------------------*/
/*-------------------------------------------------------------------------------------------------------------------------*/
/* Folgende Prozeduren verfolgen die Vorordnungs-Strategie, um die ihnen uebergebenen Terme auf Reduzierbarkeit mit dem    */
/* uebergebenen Objekt zu pruefen.                                                                                         */
/*-------------------------------------------------------------------------------------------------------------------------*/

BOOLEAN NF_ObjektAnwendbar(RegelOderGleichungsT Objekt, UTermT Term, unsigned int len);

BOOLEAN NF_TermpaarAnwendbar(TermT lhs, TermT rhs, UTermT Term); /* setzt voraus, dass lhs/rhs varnomiert !! */

/* -------------------------------------------------------------------------------------------------------------*/
/* --------- Initialisierung -----------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

void NF_InitAposteriori(InfoStringT cancellationUSR);

/* -------------------------------------------------------------------------------------------------------------*/
/* --------- Normalformbildungs-Prozeduren ---------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

#define /*BOOLEAN*/ NF_NormalformR(/*Term*/ Term)  NF_Normalform(TRUE, FALSE, Term)

#define /*BOOLEAN*/ NF_NormalformE(/*Term*/ Term)  NF_Normalform(FALSE, TRUE, Term)

#define /*BOOLEAN*/ NF_NormalformRE(/*Term*/ Term) NF_Normalform(TRUE, TRUE, Term)

extern
BOOLEAN (*NF_Normalform)(BOOLEAN doR, BOOLEAN doE, TermT Term);

BOOLEAN NF_NormalformstRE(RegelOderGleichungsT Objekt, TermT Term);

#define /*BOOLEAN*/ NF_NormalformR2(/*Term*/ lhs, /*TermT*/ rhs)   \
  NF_Normalform2(TRUE, FALSE, lhs, rhs)

#define /*BOOLEAN*/ NF_NormalformE2(/*Term*/ lhs, /*TermT*/ rhs)   \
  NF_Normalform2(FALSE, TRUE, lhs, rhs)

#define /*BOOLEAN*/ NF_NormalformRE2(/*Term*/ lhs, /*TermT*/ rhs)  \
  NF_Normalform2(TRUE, TRUE, lhs, rhs)

BOOLEAN NF_Normalform2(BOOLEAN doR, BOOLEAN doE, TermT lhs, TermT rhs);

BOOLEAN NF_geschuetzteNormalformRE(TermT Term, TermT Schutz);
/* bringt Term mit RE auf Normalform, bei toplevel werden nur Regeln/Gleichungen verwendet, die in der 
 * Dreiecksordnung kleiner sind. */
 
/* -------------------------------------------------------------------------------------------------------------*/
/* --------- Kapur-Test ----------------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

BOOLEAN NF_TermInnenReduzibel(BOOLEAN doR, BOOLEAN doE, UTermT Term);
BOOLEAN NF_TermReduzibel(BOOLEAN doR, BOOLEAN doE, UTermT Term);

#endif
