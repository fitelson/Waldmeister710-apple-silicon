/* **************************************************************************
 * AxiomZapfhahn.c  Aufzählung aller Axiome
 * 
 * Jean-Marie Gaillourdet, 21.08.2003
 *
 * ************************************************************************** */


#include "AxiomZapfhahn.h"

#include "Axiome.h"
#include "FehlerBehandlung.h"
#include "TermOperationen.h"
#include "KissKDHeap.h"
#include "KPVerwaltung.h"
#include "Zielverwaltung.h"


/*struct fuer die axiome*/
typedef struct {
  KKDHeapT heap;
  ClassifyProcT classify;
  BOOLEAN modusCP;
  unsigned long anzahlBekannterAxiome;
} LeftRightArray;


static void axz_NeueAxiomeEinfuegen( ZapfhahnT z );



/* diese Funktion wird hoffentlich geinlined */
static LeftRightArray* axiome(ZapfhahnT z) 
{  
  return (LeftRightArray*) z->privat; 
}

/* Invariante: 
 *
 * Nur an den Stellen i >= naechstesAxiom und i < size liegen noch nicht
 * selektierte Axiome.
 */

static BOOLEAN AXZ_Selektiere(ZapfhahnT z, SelectRecT* selRec) 
{
  selRec->tp = NULL;
  selRec->actualParent = NULL;
  selRec->otherParent = NULL;
  selRec->wasFIFO = FALSE;
  selRec->istGZFB = FALSE;
  selRec->result = 0;

/*   IO_DruckeFlex("### AXZ (CP:%b) Selektiere\n", axiome(z)->modusCP); */

  axz_NeueAxiomeEinfuegen(z);

  if (axiome(z)->heap == NULL) {
      our_error("momentarilly not supported\n");
  } else {
    KElementT min;
    if (KKDH_deleteMin(axiome(z)->heap,&min,0)) {
	KPV_reconstruct(&min.wid,selRec,TRUE);
      return TRUE;
    }
  }
  return FALSE;
}

static BOOLEAN AXZ_bereitsSelektiertWId( ZapfhahnT z, WId* id ) 
{
  if (!WID_isAxiom(*id)) {
    return FALSE;
  } else if (WID_xp(*id) > axiome(z)->anzahlBekannterAxiome) {
    return FALSE;
  } else {
    if (axiome(z)->heap == NULL) {
      our_error("momentarilly not supported\n");
    } else {
      KElementT min;
      if (KKDH_getMin(axiome(z)->heap,&min,0) 
	  && WID_GREATER(min.wid,*id)) {
	return TRUE;
      }
    }
  }
  return FALSE;
}

static BOOLEAN AXZ_bereitsSelektiert( ZapfhahnT z, IdMitTermenT tid ) 
{
    WId wid = {0,{ID_i(tid->id),ID_j(tid->id),ID_xp(tid->id)}};
    return AXZ_bereitsSelektiertWId(z,&wid);
}

static void AXZ_freigeben( ZapfhahnT z ) 
{
  our_dealloc(axiome(z));
}
     
void AXZ_mkZapfhahn( ZapfhahnT z, BOOLEAN modusCP, ClassifyProcT classify ) 
{
  z->selektiere = AXZ_Selektiere;
  z->bereitsSelektiert = AXZ_bereitsSelektiert;
  z->bereitsSelektiertWId = AXZ_bereitsSelektiertWId;
  z->freigeben = AXZ_freigeben;
  
  z->privat = our_alloc( sizeof(LeftRightArray) );
  axiome(z)->modusCP = modusCP;
  axiome(z)->anzahlBekannterAxiome = 0;
  axiome(z)->classify = classify;
  if (classify != NULL) {
    axiome(z)->heap = KKDH_mkHeap(10,1);
  } else {
    axiome(z)->heap = NULL;
  }
  
}

static void axz_NeueAxiomeEinfuegen( ZapfhahnT z ) 
{
    TermT lhs,rhs;
    unsigned long i;
    
    for (i=axiome(z)->anzahlBekannterAxiome+1; i <= AX_AnzahlAxiome(); i++) {
	AX_AxiomKopieren(i,&lhs,&rhs);
	AXZ_AxiomHinzufuegen(z,i,lhs,rhs);
	TO_TermeLoeschen(lhs,rhs);
    }
}


void AXZ_AxiomHinzufuegen( ZapfhahnT z, unsigned long axnummer, 
			   TermT linkeSeite, TermT rechteSeite ) 
{
  BOOLEAN istZiel;

  axiome(z)->anzahlBekannterAxiome += 1;
  istZiel = Z_IstGemantelteUngleichung(linkeSeite,rechteSeite);
    
  if (istZiel == !(axiome(z)->modusCP)) {

    if (axiome(z)->classify != NULL) {
      KElementT x;
      IdMitTermenS tid = {linkeSeite,rechteSeite,{0,0,axnummer},istZiel};

      WID_w(x.wid) = axiome(z)->classify(&tid);
      WID_id(x.wid) = tid.id;
      x.observer = NULL;
      KKDH_insert(axiome(z)->heap,x);

    } else {
	our_error("Hilfe\n");
    }
    
  } else {
/*     IO_DruckeFlex("### AXZ (CP:%b) ist nicht zuständig für: %t = %t\n", axiome(z)->modusCP, linkeSeite, rechteSeite ); */
  }
}

