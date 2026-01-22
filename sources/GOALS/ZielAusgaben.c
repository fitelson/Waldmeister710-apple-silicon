/**********************************************************************
* Filename : ZielAusgaben.h
*
* Kurzbeschreibung : Ausgaben Zielverwaltung
*
* Autoren : Andreas Jaeger
*
* 04.02.99: Erstellung aus Ausgaben.h heraus
**********************************************************************/

#include "Ausgaben.h"
#include "compiler-extra.h"
#include "general.h"
#include <stdio.h>
#include "ZielAusgaben.h"


static void ZielBewiesenDrucken (void)
{
   printf( "\n" );
   printf( "        +--------------------------+\n" );
   printf( "        |   this proves the goal   |\n" );
   printf( "        +--------------------------+\n\n" );
}


void ZA_ZielReduziertDrucken (unsigned int Nummer MAYBE_UNUSEDPARAM, 
                              TermT links MAYBE_UNUSEDPARAM, 
                              TermT rechts MAYBE_UNUSEDPARAM)
{
  if (IO_level4() || IO_level3())
    IO_DruckeFlex("goal no. %u simplified: %t =? %t\n\n", 
                  Nummer, links, rechts);
  else if (IO_level2()) {
    printf("simplify goal:   %7u  ", Nummer);
    IO_DruckeFlex("%t =? %t\n", links, rechts);
  }
  else if (IO_level1()) {
    printf("Goal no. %u simplified\n\n", Nummer);
  }
}


void ZA_ZielZusammengefuehrtDrucken (void) 
{
  if (IO_level2() || IO_level4() || IO_level3())
    ZielBewiesenDrucken ();
  else if (IO_level1())
    printf("... and hence joined\n\n");
  else if (IO_level0())
    printf("goal joined\n");
}
