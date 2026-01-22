/**********************************************************************
* Filename : Ordnungen.h
*
* Kurzbeschreibung : Ordnungen
*                    Generieren von Ordnungen fuer Beispiele
*
* Autoren : Andreas Jaeger
*
* 14.12.96: Erstellung
*
*
* Benoetigt folgende Module:
*
* Benutzte Globale Variablen aus anderen Modulen:
*
**********************************************************************/
#ifndef ORDNUNGEN_H
#define ORDNUNGEN_H

#include "compiler-extra.h"
#include "general.h"
#include "TermOperationen.h"

#ifndef CCE_Source
#include "InfoStrings.h"
#else
#include "parse-util.h"
#endif


/* ORD-1 */
/*-------*/

VergleichsT ORD_VergleicheTerme(UTermT term1, UTermT term2);
/* Vergleicht die beiden angegebenen Terme nach der eingestellten Ordnung. */


/* ORD-1a*/
/*-------*/
/* Die beiden folgenden Funktionen unterscheiden sich lediglich in der verwendeten Zeitmessung
   (unterschiedliche Zaehler).*/

BOOLEAN ORD_TermGroesserRed(UTermT term1, UTermT term2);
/* Testet, ob term1 groesser als term2 ist (bzgl. der eingestellten Ordnung). */

BOOLEAN ORD_TermGroesserUnif(UTermT term1, UTermT term2);
/* Testet, ob term1 groesser als term2 ist (bzgl. der eingestellten Ordnung). */

#ifndef CCE_Source
void ORD_ParamInfo(ParamInfoArtT what, InfoStringT DEF, InfoStringT USR);
BOOLEAN ORD_InitAposteriori (InfoStringT DEF, InfoStringT USR);
void ORD_OrdnungUmstellen(ordering Ordnung);
#else
BOOLEAN ORD_InitAposteriori (/*InfoStringT DEF, InfoStringT USR*/);
void ORD_OrdnungUmstellen(ordering_type Ordnung);
#endif


void ORD_aufZwangsGrundUmstellen(void);
void ORD_vonZwangsGrundZurueckstellen(void);

BOOLEAN ORD_OrdIstcLPO(void);

extern BOOLEAN ORD_OrdIstLPO;
extern BOOLEAN ORD_OrdIstKBO;
extern BOOLEAN ORD_OrdnungTotal;

#endif /* ORDNUNGEN_H */
