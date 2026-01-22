/* **************************************************************************
 * Axiome.c  globaler Speicher für aller Axiome
 * 
 * Jean-Marie Gaillourdet, 08.12.2003
 *
 * ************************************************************************** */

#include "Axiome.h"

#include "general.h"
#include "Ausgaben.h"
#include "TermOperationen.h"

/* Anzahl der neuen Plätze in einem Vergrößerungsschritt */
#define GROWTH_SIZE 10

static TermT* ax_linkeSeite;
static TermT* ax_rechteSeite;
static unsigned long ax_arraySize = 0;
static unsigned long ax_anzahlAxiome = 0;

void AX_AddAxiom(TermT linkeSeite, TermT rechteSeite) 
{
  char* msg = "extending AxiomArray";

  if (ax_arraySize == ax_anzahlAxiome) {
      ax_arraySize += GROWTH_SIZE;
      ax_linkeSeite = our_realloc(ax_linkeSeite,  
				  ax_arraySize * sizeof(TermT), 
				  msg);
      ax_rechteSeite = our_realloc(ax_rechteSeite, 
				   ax_arraySize * sizeof(TermT), 
				   msg);
  }

  ax_linkeSeite[ax_anzahlAxiome] = linkeSeite;
  ax_rechteSeite[ax_anzahlAxiome] =rechteSeite;
/*   IO_DruckeFlex("### Axiom %l: %t=%t hinzugefügt\n",ax_anzahlAxiome+1,linkeSeite,rechteSeite); */
  ax_anzahlAxiome += 1;
}


void AX_AxiomKopieren(unsigned long axnummer, TermT* linkeSeite, TermT* rechteSeite)
{   
    if (axnummer <= ax_anzahlAxiome) {
	*linkeSeite =  TO_Termkopie(ax_linkeSeite[axnummer-1]);
	*rechteSeite = TO_Termkopie(ax_rechteSeite[axnummer-1]);	
/* 	IO_DruckeFlex("### Axiom %d kopiert %t=%t\n", axnummer, *linkeSeite, *rechteSeite ); */
    } else {
	our_error("AX_AxiomKopieren: unknown axiom id\n");
    }
}

unsigned long AX_AnzahlAxiome( void )
{
    return ax_anzahlAxiome;
}

void AX_Cleanup( void ) 
{
  unsigned long i;


  for (i=0; i < ax_anzahlAxiome; i++) {
    TO_TermeLoeschen(ax_linkeSeite[i],ax_rechteSeite[i]);
  };
  our_dealloc(ax_linkeSeite);
  our_dealloc(ax_rechteSeite);
}

void AX_SoftCleanup( void )
{
  AX_Cleanup();
  ax_linkeSeite = NULL;
  ax_rechteSeite = NULL;
  ax_arraySize = 0;
  ax_anzahlAxiome = 0;
}
