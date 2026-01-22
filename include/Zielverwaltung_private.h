/**********************************************************************
* Filename : Zielverwaltung_private
*
* Kurzbeschreibung : "Mittelschicht" Zielverwaltung,
*                    Schnittstelle nach unten, diese Datei wird
*                    nur von den Zielverwaltungs-Modulen importiert
*
* Autoren : Andreas Jaeger
*
* 10.07.96: Erstellung aus der urspruenglichen Zielverwaltung heraus
**********************************************************************/

#ifndef ZIELVERWALTUNG_PRIVATE_H
#define ZIELVERWALTUNG_PRIVATE_H

#include "general.h"
#include "TermOperationen.h"
#include "Zielverwaltung.h"

typedef enum {
   Z_Open, Z_Joined, Z_Unified
} Z_ZielStatus;

struct privateNormaleZieleTS;
struct privateMNFZieleTS;

typedef struct ZieleTS {
   struct ZieleTS      *Nachf;
   struct ZieleTS      *Vorg;
   unsigned short int  nummer;
   TermT               orginalLinks;
   TermT               orginalRechts;
   Z_BeobachtungsProzT beobachtProz;
   Z_ZielStatus        status;
   AbstractTime        zusammenfuehrungszeitpunkt;
   Z_ZielbehandlungsModulT modul;
   union
   {
      struct privateNormaleZieleTS *NZ;
      struct privateMNFZieleTS     *MNF;
   } privat;
} ZieleT, *ZielePtrT;

#endif  /* ZIELVERWALTUNG_PRIVATE_H */
