/* **************************************************************************
 * FIFO.c
 *
 * Ein Zapfhahn zum aufz�hlen aller CPs und IRs in FIFO Reihenfolge.
 * 
 * Jean-Marie Gaillourdet, 18.09.2003
 *
 * ************************************************************************** */

#include "Axiome.h"
#include "Ausgaben.h"
#include "FehlerBehandlung.h"
#include "Hauptkomponenten.h"
#include "RUndEVerwaltung.h"
#include "NFBildung.h"
#include "TermOperationen.h"  
#include "Unifikation1.h"
#include "Parameter.h"
#include "Position.h"
#include "FIFO.h"
#include "KPVerwaltung.h"
#include "DSBaumOperationen.h"
#include "Zielverwaltung.h"

/* TachoS speichert immer die letzte zur�ckgelieferte Regel/Gleichung */
/* struct TachoS { */
/*   / * Tacho verwaltet TRUE => kritische Paare, FALSE => kritische Ziele * / */
/*   BOOLEAN modusCP;   */
 
/*   / * f�r i = 0 bezeichnet j das aktuelle Axiom * / */
/*   AbstractTime i;  */
/*   AbstractTime j; */
/*   Position xp;  */
/*   int pos_max;  */
/* } Tacho; */


static BOOLEAN fifo_reconstruct( TachoT tacho, SelectRecT* res )
{
  AbstractTime memo;
  TermT lhs, rhs; 
  RegelOderGleichungsT vater, mutter; 

/*   IO_DruckeFlex("fifo_reconstruct(%u,%u,",tacho->i,tacho->j); */
/*   DruckePos(tacho->xp); */
/*   IO_DruckeFlex(")\n"); */
 
  if (POS_into_i(tacho->xp)) {

    vater = RE_getActiveWithBirthday(tacho->j); 
    if (POS_j_right(tacho->xp)) {
      vater = vater->Gegenrichtung;
    }

    mutter = RE_getActiveWithBirthday(tacho->i); 
    if (POS_i_right(tacho->xp)) { 
      mutter = mutter->Gegenrichtung; 
    } 

  } else { 

    vater = RE_getActiveWithBirthday(tacho->i); 
    if (POS_i_right(tacho->xp)) { 
      vater = vater->Gegenrichtung; 
    } 

    mutter = RE_getActiveWithBirthday(tacho->j);  
    if (POS_j_right(tacho->xp)) {  
      mutter = mutter->Gegenrichtung;  
    } 
  } 

  memo = HK_AbsTime;
  HK_AbsTime = tacho->i;

  if (U1_KPKonstruiert(mutter,POS_pos(tacho->xp),vater,&lhs,&rhs)) { 
    WId wid = {undefinedWeight(),{tacho->i,tacho->j,tacho->xp}};
    NF_Normalform2(PA_GenR(), PA_GenE(), lhs, rhs); 
    BO_TermpaarNormierenAlsKP(lhs,rhs);
    HK_AbsTime = memo;

    res->rhs = rhs;  
    res->lhs = lhs;  
    res->actualParent = mutter; /* XXX stimmt hier die Zuordnung ? */ 
    res->otherParent = vater;  
    res->tp = NULL;  
/*     res->result = HK_VariantentestUndVergleich(res->lhs, res->rhs);   */
    res->wasFIFO = TRUE;
    res->istGZFB = FALSE;
    res->wid = wid;
    return TRUE;  
  } else {  
    HK_AbsTime = memo;
    return FALSE;  
  }
}

static void fifo_calcMaxPos( TachoT tacho ) 
{
  int xbits = POS_xbits(tacho->xp);
  TermT teilTerm;

  if (XBIT_into_i(xbits)) {
    if (XBIT_i_right(xbits)) {
      teilTerm = RE_getActiveWithBirthday(tacho->i)->rechteSeite;
    } else {
      teilTerm = RE_getActiveWithBirthday(tacho->i)->linkeSeite;
    }
  } else {
    if (XBIT_j_right(xbits)) {
      teilTerm = RE_getActiveWithBirthday(tacho->j)->rechteSeite;
    } else {
      teilTerm = RE_getActiveWithBirthday(tacho->j)->linkeSeite;
    }
  }
  tacho->pos_max =  TO_Termlaenge(teilTerm)-1;
}



static BOOLEAN fifo_incrementI( TachoT tacho ) 
{
  if (tacho->i < (HK_AbsTime-1) || tacho->i == 0) {
    if (tacho->i == 0) {
      tacho->i = 1 + tacho->startZeitpunkt;
    } else {
      tacho->i++;
    }
    tacho->j = 1 + tacho->startZeitpunkt;
    tacho->xp = 0;
    tacho->pos_max = -1;
    return TRUE;
  } else {
    return FALSE;
  }
}

static BOOLEAN fifo_incrementJ( TachoT tacho ) 
{
  if (tacho->j < tacho->i) {
    tacho->j++;
    tacho->xp = 0;
    tacho->pos_max = -1;
    return TRUE;
  } else {
    tacho->xp = 0;
    tacho->pos_max = -1;
    return FALSE;
  }
}

/* Teste ob die xbits ein g�ltiges KP beschreiben k�nnen 
   m�gliche Fehler: �berlappung mit Ungleichung
                    Drehen einer gerichteten Gleichung
*/
static BOOLEAN fifo_invalidXbits( TachoT tacho ) {
  RegelOderGleichungsT tp_i, tp_j;

  tp_i = RE_getActiveWithBirthday(tacho->i);
  tp_j = RE_getActiveWithBirthday(tacho->j);

  return  ((!POS_into_i(tacho->xp)) && RE_IstUngleichung(tp_i)) /* �berlappung in negatives i */ 
    || (( POS_into_i(tacho->xp)) && RE_IstUngleichung(tp_j))    /* �berlappung in negatives j */
    || ( TP_IstMonogleichung( tp_j ) && POS_j_right(tacho->xp))
    || ( TP_IstMonogleichung( tp_i ) && POS_i_right(tacho->xp))
    || ( TP_TermpaarIstRegel(tp_j) && POS_j_right(tacho->xp)) /* Regel j darf nicht gedreht werden */
    || ( TP_TermpaarIstRegel(tp_i) && POS_i_right(tacho->xp));/* Regel i darf nicht gedreht werden */
}


static BOOLEAN fifo_incrementPos( TachoT tacho )
{
  if (tacho->j == ((unsigned long) -1)) {
    /* in diesem Zustand gibt es keine Positionen zu erh�hen */
    return FALSE;
  }

  if (tacho->pos_max == ((unsigned long)-1)) {
    fifo_calcMaxPos(tacho);
  }


  if (POS_pos(tacho->xp) == tacho->pos_max) {

    do {
      if (POS_xbits(tacho->xp) >= 7) {
	/* die Position ist nicht mehr weiter zu erh�hen */
	return FALSE;
      }
      POS_set_xbits(tacho->xp,POS_xbits(tacho->xp)+1);

    } while (fifo_invalidXbits(tacho));

    POS_set_pos(tacho->xp,0);
    fifo_calcMaxPos(tacho);
    return TRUE;

  } else if (POS_pos(tacho->xp) < tacho->pos_max) {
    POS_set_pos(tacho->xp,POS_pos(tacho->xp)+1);
    return TRUE;
  } else {
    our_error( "FIFO.c: fifo_incrementPos : this point should never be reached!" );
    return FALSE;
  }
  return FALSE; /* compiler shut up */
  
}

static BOOLEAN fifo_increment( TachoT tacho ) {
  return fifo_incrementPos(tacho) || fifo_incrementJ(tacho) || fifo_incrementI(tacho);
}

static BOOLEAN fifo_selectPos( TachoT tacho, SelectRecT* next )
{
  BOOLEAN nothing_to_select = FALSE;
  SelectRecT temp = {NULL,NULL,NULL,NULL,NULL,0,FALSE,FALSE,0,0,0,0};
  do {
    if (nothing_to_select) {
      if (!fifo_incrementPos(tacho)) {
	/* es gibt nichts zu selektieren */
	return FALSE;
      }
    }

    nothing_to_select = !fifo_reconstruct(tacho,&temp);
  } while (nothing_to_select);
  *next = temp;
  return TRUE;
}


static BOOLEAN fifo_selectJ( TachoT tacho, SelectRecT* next )
{
  BOOLEAN nothing_to_select = FALSE;
  
  do {
    if (nothing_to_select) {
      if (!fifo_incrementJ(tacho)) {
	/* es gibt nichts zu selektieren */
	return FALSE;
      }
    }

    /* passt der Modus zu dem Vorzeichen von j ? */
    if (tacho->modusCP) {
      if (RE_IstUngleichung(RE_getActiveWithBirthday(tacho->j))) {
	nothing_to_select = TRUE;
	continue;
      }
    } else {
      RegelOderGleichungsT faktum_i = RE_getActiveWithBirthday(tacho->i);
      RegelOderGleichungsT faktum_j = RE_getActiveWithBirthday(tacho->j);

      if ((RE_IstUngleichung(faktum_i) && RE_IstUngleichung(faktum_j))
	  || (RE_IstGleichung(faktum_i) && RE_IstGleichung(faktum_j))) {
	nothing_to_select = TRUE;
	continue;
      }
    }

    if (RE_getActiveWithBirthday(tacho->j)->lebtNoch || !PA_Waisenmord()) {      
      nothing_to_select = !fifo_selectPos( tacho, next );
    } else {
      nothing_to_select = TRUE;
    }
    
  } while (nothing_to_select);
  return TRUE;
}


static BOOLEAN fifo_selectIROpfer( TachoT tacho, SelectRecT* next )
{
  AbstractTime memo;
  TermT reproLhs, reproRhs;
  RegelOderGleichungsT iropfer;

  iropfer = RE_getActiveWithBirthday( tacho->i );
  tacho->j = TP_getTodesTag_M(iropfer);
  tacho->xp = 0;

  memo = HK_AbsTime;
  HK_AbsTime = TP_getTodesTag_M(iropfer);

  reproLhs = TO_Termkopie(TP_LinkeSeite(iropfer));
  reproRhs = TO_Termkopie(TP_RechteSeite(iropfer));

  NF_Normalform2( PA_GenR(), PA_GenE(), reproLhs, reproRhs);
  BO_TermpaarNormierenAlsKP( reproLhs, reproRhs );

  HK_AbsTime = memo;

  next->tp = NULL;
  next->actualParent = iropfer;
  next->otherParent = NULL;
  next->lhs = reproLhs;
  next->rhs = reproRhs;
  next->wid.w = undefinedWeight();
  return TRUE;
}



static BOOLEAN fifo_selectNextAxiom( TachoT tacho, SelectRecT* next ) 
{
  tacho->j++;

  if (tacho->i != 0) {
    /* f�r neue Axiome zur�ckspringen */
    if (tacho->anzahlBekannterAxiome < AX_AnzahlAxiome()) {
      WId wid = {0,{0,0,tacho->j}};
      tacho->anzahlBekannterAxiome++;
      AX_AxiomKopieren(tacho->anzahlBekannterAxiome, &(next->lhs),&(next->rhs));
      
      /* nur zum Modus passende Axiome werden akzeptiert */
      if (Z_IstGemanteltesTermpaar(next->lhs,next->rhs) == tacho->modusCP) {
	TO_TermeLoeschen(next->lhs,next->rhs);
	return FALSE;
      }
      next->wid = wid;
      return TRUE;
    }
  } else {
    if (tacho->j <= AX_AnzahlAxiome()) {
      WId wid = {0,{0,0,tacho->j}};
      tacho->anzahlBekannterAxiome++;
      AX_AxiomKopieren(tacho->j,&(next->lhs),&(next->rhs));
      /* nur zum Modus passende Axiome werden akzeptiert */
      if (Z_IstGemanteltesTermpaar(next->lhs,next->rhs) == tacho->modusCP) {
	TO_TermeLoeschen(next->lhs,next->rhs);
	return FALSE;
      }
      next->wid = wid;
      return TRUE;
    }
  }
  fifo_incrementI(tacho);
  return FALSE;
}


static BOOLEAN fifo_selectNextNonAxiom( TachoT tacho, SelectRecT* next )
{
  BOOLEAN nothing_to_select = FALSE;
  
  fifo_increment(tacho);

  do {
    if (nothing_to_select) {
      if (!fifo_incrementI(tacho)) {
	/* es gibt nichts zu selektieren */
	return FALSE;
      }
    }

    /* passt der Modus zu dem Vorzeichen von i ? */
    if ((tacho->modusCP) && (RE_IstUngleichung(RE_getActiveWithBirthday(tacho->i)))) {
      nothing_to_select = TRUE;
      continue;
    }

    /* sind wir direkt nach dem selektieren eines IROpfer ? */
    if (tacho->i < tacho->j) {
      nothing_to_select = TRUE;
      continue;
    }

    if (RE_getActiveWithBirthday(tacho->i)->lebtNoch || !PA_Waisenmord()) {      
      nothing_to_select = !fifo_selectJ( tacho, next ); 
    } else {
      nothing_to_select = !fifo_selectIROpfer( tacho, next );
    }

  } while (nothing_to_select);
  next->wid.id.i = tacho->i;
  next->wid.id.j = tacho->j;
  next->wid.id.xp = tacho->xp;
  return TRUE;
}


static BOOLEAN fifo_Selektiere( ZapfhahnT z, SelectRecT* next )
{
  BOOLEAN result;
  TachoT tacho = (TachoT) z->privat;

/*   IO_DruckeFlex("### Zapfhahn(%s,FIFO)\n",tacho->modusCP ? "CP" : "CG"); */

  tacho->new_initialized = FALSE;

  /* wiederhole bis wir eine Gleichung haben, die noch nicht selektiert wurde
     oder der der Tacho nicht mehr weiter hochgez�hlt werden kann */
  do {
    if (tacho->i == 0 || tacho->anzahlBekannterAxiome != AX_AnzahlAxiome()) {
      result = fifo_selectNextAxiom( tacho, next );
      if (!result) {
	continue;
      }
    } else {
      result = fifo_selectNextNonAxiom( tacho, next );
    }
    if (!result) {
	break;
    } else {
      tacho->blockAlreadySelected = TRUE;
      if (KPV_alreadySelected(tacho->i,tacho->j,tacho->xp,
				   tacho->modusCP,next->lhs,next->rhs)) {
      TO_TermeLoeschen(next->lhs,next->rhs);
      result = FALSE;
      }
      tacho->blockAlreadySelected = FALSE;
    }
  } while (!result);

/*   IO_DruckeFlex("### selektiert: %w\n",&(next->wid)); */
  
  return result;
}

static BOOLEAN fifo_BereitsSelektiertWId( ZapfhahnT z, WId* wid )
{
  BOOLEAN result;
  TachoT tacho = (TachoT) z->privat;

  if (tacho->blockAlreadySelected) return FALSE;

  if (tacho->new_initialized) {
    /* ansonsten w�rde bei einem Aufruf von alreadySelected vor dem
       ersten selectNextElement immer true zur�ckgegeben werden 
    */
    return FALSE;
  } else { 
    result =  (tacho->i > WID_i(*wid))  
      || ((tacho->i == WID_i(*wid)) && (tacho->j > WID_j(*wid)))  
      || ((tacho->i == WID_i(*wid)) && (tacho->j == WID_j(*wid)) && (tacho->xp >= WID_xp(*wid))); 
    return result;
  }
}

static BOOLEAN fifo_BereitsSelektiert( ZapfhahnT z, IdMitTermenT tid )
{
  WId wid = {0,{ID_i(tid->id),ID_j(tid->id),ID_xp(tid->id)}};
  if (((TachoT)z->privat)->blockAlreadySelected) return FALSE;
  return fifo_BereitsSelektiertWId(z,&wid);
}

static void fifo_freeTacho( ZapfhahnT z ) 
{
  our_dealloc(z->privat);
  our_dealloc(z);
}

ZapfhahnT FIFO_mkTacho( BOOLEAN modusCP )
{
    ZapfhahnT z = our_alloc( sizeof(struct ZapfhahnS) );    
    TachoT tacho = our_alloc( sizeof(struct TachoS) );

    tacho->modusCP = modusCP;
    tacho->i = 0;
    tacho->j = 0;
    tacho->xp = -1;
    tacho->new_initialized = TRUE;
    tacho->startZeitpunkt = HK_AbsTime;
    tacho->anzahlBekannterAxiome = 0;
    tacho->blockAlreadySelected = FALSE;

    z->privat = tacho;
    z->freigeben = fifo_freeTacho;
    z->selektiere = fifo_Selektiere;
    z->bereitsSelektiert = fifo_BereitsSelektiert;
    z->bereitsSelektiertWId = fifo_BereitsSelektiertWId;

    return z;
}



