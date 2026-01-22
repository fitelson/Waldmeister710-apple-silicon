/* **************************************************************************
 * Ring.c
 * 
 * 
 * Jean-Marie Gaillourdet, 11.09.2003
 *
 * ************************************************************************** */

#include "Ring.h"

#include "FehlerBehandlung.h"
#include "SpeicherVerwaltung.h"

static void rng_Anfangsstueck (RingT ring)
{
    int counter = 0;
    
    while (ring->Elemente[ring->AktuellesElement].Grenzwert == 0 
	   && counter < ring->AnzahlElemente) 
    {
	counter += 1;
	ring->AktuellerOffset = 0;
	ring->AktuellesElement++;
	if (ring->AktuellesElement == ring->AnzahlElemente) {
	    ring->AktuellesElement = 0;
	}
    }
}

void * RNG_getCurrent( RingT  ring ) 
{
  rng_Anfangsstueck(ring);
  return ring->current;
}

void* RNG_getNext( RingT  ring ) 
{
    rng_Anfangsstueck(ring);
    
    ring->current = ring->Elemente[ring->AktuellesElement].Daten;
    ring->AktuellerOffset++;

    if (ring->AktuellerOffset == ring->Elemente[ring->AktuellesElement].Grenzwert) {
	ring->AktuellerOffset = 0;
	ring->AktuellesElement++;
	if (ring->AktuellesElement == ring->AnzahlElemente) {
	    ring->AktuellesElement = 0;
	}
    }
    return ring->current;
}

void RNG_addElement( RingT ring, void* Daten, int Grenzwert )
{
    ring->Elemente = our_realloc( ring->Elemente, 
				  sizeof( struct RingElementS ) * (ring->AnzahlElemente+1), 
				  "Ringvergrößerung" );
    ring->Elemente[ring->AnzahlElemente].Daten = Daten;
    ring->Elemente[ring->AnzahlElemente].Grenzwert = Grenzwert;
    ring->AnzahlElemente++;
}

void RNG_removeElement( RingT ring, int ElementNumber )
{
    int i;
    
    if (ring->AnzahlElemente <= ElementNumber) {
	our_error( "RNG_removeElement: ungültige Elementnummer\n" );
    }

    for (i = ElementNumber+1; i < ring->AnzahlElemente; i++) {
	ring->Elemente[i-1] = ring->Elemente[i];
    }

    ring->AnzahlElemente -= 1;
    ring->Elemente = our_realloc( ring->Elemente, 
				  sizeof( struct RingElementS ) * (ring->AnzahlElemente+1), 
				  "Ringvergrößerung" );

    if (ring->AktuellesElement == ElementNumber) {
	ring->AktuellesElement += 1;
	ring->AktuellerOffset = 0;
	if (ring->AktuellesElement == ring->AnzahlElemente) {
	    ring->AktuellesElement = 0;
	}
    } else if (ring->AktuellesElement > ElementNumber) {
	ring->AktuellesElement -= 1;
    }
}


RingT RNG_make(void) 
{
    RingT result = our_alloc(sizeof(struct RingS));
    result->AnzahlElemente = 0;
    result->AktuellesElement = 0;
    result->AktuellerOffset = 0;
    result->Elemente = NULL;
    return result;
}

void RNG_destroy( RingT ring ) 
{
    our_dealloc(ring);
}

    
