/* **************************************************************************
 * ZapfAnlage.c  Dispatcher für die verschiedenen Zapfen
 * 
 * Die ZapfAnlage verwaltet die verschiedenen Aktiven Zapfen in zwei
 * verschiedenen Queues: high und low
 * Solange in einer der Zapfen der high-Queue noch Fakten verfügbar sind, 
 * werden diese selektiert. Wenn dies nicht der Fall ist, wird ein Zapfen 
 * aus der low-Queue ausgewählt, der das nächste Faktum liefert.
 * 
 * Jean-Marie Gaillourdet, 18.08.2003
 *
 * ************************************************************************** */

#include "ZapfAnlage.h"

#include "Ausgaben.h"
#include "FehlerBehandlung.h"
#include "Ring.h"
#include "Parameter.h"

int AnzahlWichtigerZapfhaehne = 0;
static ZapfhahnT* wichtigeZapfhaehne = NULL;

static RingT Wurzel = NULL;
static int MaxSelektionsVersuche = 0;

static ZapfhahnT NaechsterNormalerZapfhahn()
{
  return (ZapfhahnT) RNG_getCurrent( (RingT) RNG_getCurrent( Wurzel ) );
}

void ZPA_Weiterdrehen(void)
{
  RNG_getNext( (RingT) RNG_getNext( Wurzel ) );
}

BOOLEAN ZPA_Selektiere(SelectRecT *selRec) {
  int i, SelektionsVersuche;

  /* finde noch zu selektierende Fakten in der high priority queue */
  for (i=0; i<AnzahlWichtigerZapfhaehne; i++) {
      if (wichtigeZapfhaehne[i]->selektiere( wichtigeZapfhaehne[i], selRec )) {
	  KPV_IncAnzBehandelterKPsVervollst();
	  return TRUE;
      }
  }

  /* finde noch zu selektierende Fakten in der low priority queue */
  for (SelektionsVersuche = 0; SelektionsVersuche < MaxSelektionsVersuche; SelektionsVersuche++ ) {
    ZapfhahnT z;

    z = NaechsterNormalerZapfhahn();
    if (z->selektiere( z, selRec )) {
	KPV_IncAnzBehandelterKPsVervollst();
	return TRUE;
    }
    else {
      ZPA_Weiterdrehen();
    }
  };
  return FALSE;
}

BOOLEAN ZPA_BereitsSelektiert(IdMitTermenT tid) 
{
    int i;
    RingT r;
    ZapfhahnT z;
    
    for (i=0; i<AnzahlWichtigerZapfhaehne; i++) 
    {
	if (wichtigeZapfhaehne[i]->bereitsSelektiert(wichtigeZapfhaehne[i],tid)) {
	    return TRUE;
	}
    };    
    
    RNG_ForAll(Wurzel,r, {
	RNG_ForAll(r,z, {
	    if (z->bereitsSelektiert(z,tid)) {
		return TRUE;
	    }	    
	});
    });
    
    return FALSE;
}

BOOLEAN ZPA_BereitsSelektiertWId(WId* id) 
{
    int i;
    RingT r;
    ZapfhahnT z;

    for (i=0; i<AnzahlWichtigerZapfhaehne; i++) 
    {
	if (wichtigeZapfhaehne[i]->bereitsSelektiertWId(wichtigeZapfhaehne[i],id)) {
	    return TRUE;
	}
    };    
    
    RNG_ForAll(Wurzel,r, {
	RNG_ForAll(r,z, {
	    if (z->bereitsSelektiertWId(z,id)) {
		return TRUE;
	    }	    
	});
    });

    return FALSE;
}


void ZPA_Initialisieren(void) 
{
    Wurzel = RNG_make();
    
    RNG_addElement(Wurzel,RNG_make(),PA_modulo()-PA_threshold());
    RNG_addElement(Wurzel,RNG_make(),PA_threshold());
}

void ZPA_Aufrauemen(void)
{
    RingT r;
    ZapfhahnT z;
    int i;
    
    for (i = 0; i < AnzahlWichtigerZapfhaehne; i++) {
	wichtigeZapfhaehne[i]->freigeben(wichtigeZapfhaehne[i]);
    };
    our_dealloc(wichtigeZapfhaehne);
    AnzahlWichtigerZapfhaehne = 0;
    wichtigeZapfhaehne = NULL;

    RNG_ForAll(Wurzel,r, {
	RNG_ForAll(r,z,{
	    z->freigeben(z);
	});
	RNG_destroy(r);
    });
    RNG_destroy(Wurzel);
    Wurzel = NULL;
    MaxSelektionsVersuche = 0;
}

void ZPA_ZapfhahnAnmelden( ZapfhahnT z, BOOLEAN bevorzugt, int Gewicht, int welcherRing )
{
    if (bevorzugt) {
	AnzahlWichtigerZapfhaehne++;
	wichtigeZapfhaehne = our_realloc(wichtigeZapfhaehne,
					 AnzahlWichtigerZapfhaehne*sizeof(ZapfhahnT),
					 "wichtige Zapfhähne");
	wichtigeZapfhaehne[AnzahlWichtigerZapfhaehne-1] = z;
    } else if (Gewicht > 0) {
	RNG_addElement( (RingT) Wurzel->Elemente[welcherRing].Daten, z, Gewicht );
	MaxSelektionsVersuche += Gewicht;
    }
}

void ZPA_ZapfhahnAbmelden( ZapfhahnT z )
{
    int i,j;
    RingT r;
    ZapfhahnT x;
    
    for (i = 0; i < AnzahlWichtigerZapfhaehne; i++) {
	if (wichtigeZapfhaehne[i] == z) {
	    for (j = i+1; j < AnzahlWichtigerZapfhaehne; j++) {
		wichtigeZapfhaehne[j-1] = wichtigeZapfhaehne[j];
	    }
	    AnzahlWichtigerZapfhaehne -= 1;
	    wichtigeZapfhaehne = our_realloc(wichtigeZapfhaehne,
					     AnzahlWichtigerZapfhaehne*sizeof(ZapfhahnT),
					     "wichtige Zapfhähne");
	    return;
	}
    }

    RNG_ForAll(Wurzel,r,{
	i = 0;
	RNG_ForAll(r,x,{
	    if (x == z) {
		RNG_removeElement(r,i);
		return;
	    };
	    i += 1;
	});
    });
    our_error("ZPA_ZapfhahnAbmelden: abzumeldender Zapfhahn nicht gefunden\n");
}

