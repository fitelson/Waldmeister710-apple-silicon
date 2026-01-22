/* **************************************************************************
 * Ring.h  
 * 
 * Jean-Marie Gaillourdet, 01.09.2003
 *
 * ************************************************************************** */


#ifndef RING_H
#define RING_H

typedef struct RingElementS {
  int Grenzwert;
  void* Daten;
} RingElementT;


struct RingS {
  int AnzahlElemente;
  int AktuellesElement;
  int AktuellerOffset;
  void *current;
  RingElementT *Elemente;
};

typedef struct RingS *RingT;

void * RNG_getCurrent( RingT  ring );
void* RNG_getNext( RingT ring );

void RNG_addElement( RingT ring, void* Daten, int Grenzwert );
void RNG_removeElement( RingT ring, int ElementNumber );

RingT RNG_make(void);
void RNG_destroy( RingT ring );

#define RNG_ForAll(/*RingT*/ ring,index,code)					\
{										\
    int rng_index;								\
    for (rng_index = 0; rng_index < (ring)->AnzahlElemente; rng_index++) {	\
        index = (ring)->Elemente[rng_index].Daten;				\
	code;									\
    }										\
}

#endif /* RING_H */
