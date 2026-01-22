/**********************************************************************
* Filename : MNF.c
*
* Kurzbeschreibung : MultipleNormalFormen-Bildung
*                    (Verwalten von Nachfolgermengen)
*
* Autoren : Andreas Jaeger
*
* 10.07.96: Erstellung
* 22.02.97: Mehrfachaufrufclean. AJ.
*
*
*
* Benoetigt folgende Module:
*  - MatchOperationen: Reduktion der Terme
*  - Ausgaben: Ausgeben von Termen (wird beim Testen benutzt)
*  - NFBasis: Reduzieren
*  - RUndEVerwaltung: Zum Testen, ob Regelmenge leer ist
*  - Hash: Verwalten der Nachfolgermenge
*  - MNF_PQ: Verwalten der noch zu bearbeitenden Elemente
*  - MNF_Print: Ausgeben der MNF-Menge
*  - Hauptkomponenten: Ordnungsvergleich
*  - DSBaumOperationen: Normieren von Termpaaren
*
* Benutzte Globale Variablen aus anderen Modulen:
*   zur Zeit keine
*
**********************************************************************/

/* ToDo:
  - wie soll Subsumptionstest bei mnfanalyse geschehen?
  - CounterT in interne Structs einfuegen?
  */

/* Konzept fuer Mehrfachaufruf:

   InitAposteriori legt Speicher an
   CleanUp gibt Speicher frei und setzt Variablen wieder auf Initialzustand.
 */

#include "antiwarnings.h"
#include "general.h"
#include "compiler-extra.h"
#include "Hash.h"
#include "MNF_intern.h"
#include "MNF.h"
#include "MNF_Print.h"
#include "MNF_PQ.h"
#include "NFBildung.h"
#include "DSBaumOperationen.h"
#include "MatchOperationen.h"
#include "Ordnungen.h"
#include "Ausgaben.h"
#include "RUndEVerwaltung.h"
#include "Hauptkomponenten.h"
#include "SpeicherVerwaltung.h"
#include "Statistiken.h"
#include "Unifikation1.h"
#include "VariablenMengen.h"
#include <string.h>
#include "Parameter.h"

#include "Stack.h"
#include "Deque.h"


typedef enum {ARnoUse, ARuseAnti, ARuseUnifikation} AntiRegelUseT;

   
/* Funktionsprototypen */
static void MNF_BearbeiteRE(void *pq, MNF_MengeT *menge, int colour, 
                            MNF_ColTermPtrT acctCTerm);
static void MNF_BearbeiteR(void *pq, MNF_MengeT *menge, int colour, 
                           MNF_ColTermPtrT acctCTerm);
static void MNF_BearbeiteE(void *pq, MNF_MengeT *menge, int colour, 
                           MNF_ColTermPtrT acctCTerm);
static void MNF_BearbeiteDummy (void *pq, MNF_MengeT *menge, int colour, 
                                MNF_ColTermPtrT acctCTerm);
static void MNF_BearbeiteEineR (void *stack, MNF_MengeT *menge, int colour,
                               MNF_ColTermPtrT acctCTerm, RegelT Regel);
static unsigned int count_vars (TermT term) MAYBE_UNUSEDFUNC;
static void MNF_ConvertRegelToAntiRegel (MNF_AntiT antiKind,
                                         RegelT Regel,
                                         TermT *antiLinks, TermT *antiRechts,
                                         AntiRegelUseT *useAnti);
static void SetColTerm(/*@out@*/ MNF_ColTermPtrT *colTermPtr, TermT term,
                       int colour,
                       MNF_ColTermPtrT parentPtr,
                       VergleichsT Vergleich,
                       long int RENummer,
                       HashKeyT *Key1,
                       HashKeyT *Key2);
#define SetBOBInfo(bobInfo,redPosition,RESeite, parentPtr, RENummer,re) \
    SetBOBInfoI(bobInfo,redPosition,RESeite, parentPtr, RENummer,re)
     
static void SetBOBInfoI(MNF_BOBInfoPtrT bobInfo, TermT redPosition,
                        SeitenT RESeite,
                        MNF_ColTermPtrT parentPtr, long int RENummer,
                        RegelOderGleichungsT re);

static void DeallocData (MNF_ColTermT *colTerm);
static void MNF_PseudoExpand (MNF_MengeT *menge);
static void MNF_SearchInteresting (MNF_MengeT *menge);



/* Variablen fuer Initialisierungen */
BOOLEAN           MNF_Initialised = FALSE;


/* diverse Variablen */
static SV_SpeicherManagerT MNF_ColTermSpeicher;
static SV_SpeicherManagerT MNF_MengeSpeicher;


/* wir verwalten einen eigenen Zaehler fuer die Elemente, um diese eindeutig
   identifizieren zu koennen. */
static MNF_IdT current_mnfid;

/*****************************************************************************/
/* Initialisierung/Aufraeumen/Statistik des Moduls                           */
/*****************************************************************************/


void MNF_InitAposteriori (void) 
{
   HA_InitAposteriori ();
   DQ_InitAposteriori ();
   
   current_mnfid = 1;
   
   MNF_Initialised = TRUE;
   SV_ManagerInitialisieren (&MNF_ColTermSpeicher, MNF_ColTermT, 
                             MNF_ColTermBlock, "Coloured Terms (MNF)");
   SV_ManagerInitialisieren (&MNF_MengeSpeicher, MNF_MengeT, 
                             MNF_MengeSpeicherBlock, "MNF Set (MNF)");
}


void MNF_Cleanup (void) {
   if (!MNF_Initialised)
      return;
}



/*****************************************************************************/
/* Routinen fuer Kommunikation mit ADT Hash                                  */
/*****************************************************************************/
const unsigned long prim1 = 41698703L;
const unsigned long prim2 = 44278001L;
const unsigned long mult1 = 103L;
const unsigned long mult2 = 97L;

/* MNF_HashCode berechnet einen Fingerprint des Termes term.
   Dieser kann dann als Eingabe fuer die Hashfunktion benutzt werden.
   */
void MNF_HashCode (TermT term, MNF_HashT *hash1E, MNF_HashT *hash2E) {
   MNF_HashT    hash1=0;
   MNF_HashT    hash2=0;
   SymbolT      symbol;

   /* Hashfkt. muss evtl. noch ueberarbeitet werden */
   /* Achtung: Das kleinste Funktionssymbol hat den Wert 0,
      das groesste SO_GroesstesFktSymb.
      Um Eineindeutigkeit der Funktion zu garantieren, muss deswegen
      mit symbol + 1 gerechnet werden
      */
   TO_doSymbole(term, symbol, {
     if (SO_SymbIstFkt (symbol)) {
       hash1 = (hash1 * mult1 + symbol + 1) % prim1;
       hash2 = (hash2 * mult2 + symbol + 1) % prim2;
     }
     else{ /* Variablen sind negativ */
       hash1 = (hash1 * mult1 + SO_GroesstesFktSymb - symbol + 1) % prim1;
       hash2 = (hash2 * mult2 + SO_GroesstesFktSymb - symbol + 1) % prim2;
     }
   });
   *hash1E = hash1;
   *hash2E = hash2;
}


static BOOLEAN CmpData (MNF_ColTermT *colTerm1, MNF_ColTermT *colTerm2) 
{
   BOOLEAN vgl;
   
   vgl = TO_TermeGleich(colTerm1->term, colTerm2->term);
#if MNF_TEST > 3
   if (!vgl) {
      MNF_HashT    hash1, hash2;
      printf("\n\a Terme sind unterschiedlich!\a\n");
      IO_DruckeFlex("Term1: %t", colTerm1->term);
      MNF_HashCode (colTerm1->term, &hash1, &hash2);
      printf("Key1: %lu, Key2: %lu\n", hash1, hash2);
      IO_DruckeFlex("Term1: %t", colTerm2->term);
      MNF_HashCode (colTerm2->term, &hash1, &hash2);
      printf("Key1: %lu, Key2: %lu\n", hash1, hash2);
      exit(7);
   }
#endif   
   return vgl;
   
   
}


static BOOLEAN AddData (MNF_ColTermT *colTermOld, MNF_ColTermT *colTermNew)
{
   BOOLEAN        retVal = FALSE;

   MNF_IncAnzTermGleich;
   /* Sind Farben gleich? */
   if (MNF_tcolour_eq(*colTermOld, *colTermNew)) {
#if MNF_TEST
      (colTermOld->in)++;
#endif
      if (colTermOld->anzGesSchritte > colTermNew->anzGesSchritte) {
         /* mnfID des alten muss erhalten bleiben */
         colTermOld->anzGesSchritte       = colTermNew->anzGesSchritte;
         colTermOld->anzGlSchritte        = colTermNew->anzGlSchritte;
         colTermOld->anzAntiRSchritte     = colTermNew->anzAntiRSchritte;
         colTermOld->bob_info.parentID    = colTermNew->bob_info.parentID;
         colTermOld->bob_info.RESeite     = colTermNew->bob_info.RESeite;
         colTermOld->bob_info.redPosition = colTermNew->bob_info.redPosition;
         colTermOld->bob_info.RENummer    = colTermNew->bob_info.RENummer;
         colTermOld->bob_info.time        = colTermNew->bob_info.time;
         colTermOld->bob_info.re          = colTermNew->bob_info.re;      
         colTermOld->flags.vgl = colTermNew->flags.vgl;
      }
      MNF_IncAnzTermGleichCol;
   }
   else if (MNF_tcolour_opp(*colTermOld, *colTermNew)) {
      MNF_IncAnzTermGleichOpp;
      retVal=TRUE;
   }
   else {
      our_error ("Fuer mehr als zwei Farben fehlt hier noch die "
                 "Implementierung");
   }
   
   return retVal;
}


static void DeallocData (MNF_ColTermT *colTerm) 
{
     TO_TermLoeschen (colTerm->term);
     SV_Dealloc (MNF_ColTermSpeicher, colTerm);     
}


/****************************************************************************/
/* Diverse Routinen                                                         */
/****************************************************************************/


static MNF_IdT MNF_getID (void) 
{
   return current_mnfid;
}

static void MNF_incID (void) 
{
   current_mnfid++;
}

static void SetBOBInfoI(MNF_BOBInfoPtrT bobInfo, TermT redPosition,
                        SeitenT RESeite,
                        MNF_ColTermPtrT parentPtr, long int RENummer,
                        RegelOderGleichungsT re)
{
   bobInfo->mnfID = MNF_getID ();
   bobInfo->redPosition = redPosition;
   bobInfo->RESeite = RESeite;
   bobInfo->RENummer = RENummer;
   bobInfo->time = HK_getAbsTime_M(); /* ??? */
   bobInfo->re = re;
   if (parentPtr)
      bobInfo->parentID = parentPtr->bob_info.mnfID;
   else {  
      bobInfo->parentID = 0;
   }
}

static MNF_ColTermPtrT CopyColTerm (MNF_ColTermPtrT colTermPtr) 
{
   MNF_ColTermPtrT copyCTermPtr;

   SV_Alloc(copyCTermPtr,MNF_ColTermSpeicher,MNF_ColTermPtrT);
   
   *copyCTermPtr = *colTermPtr;
   copyCTermPtr->term = TO_Termkopie (colTermPtr->term);
   copyCTermPtr->bob_info.redPosition = NULL; /* Hack, damit nichts passiert */
   return copyCTermPtr;
}

   
static void SetColTerm(/*@out@*/ MNF_ColTermPtrT *colTermPtr, TermT term,
                       int colour,
                       MNF_ColTermPtrT parentPtr,
                       VergleichsT Vergleich,
                       long int RENummer,
                       HashKeyT *Key1,
                       HashKeyT *Key2)
{
   SV_Alloc(*colTermPtr,MNF_ColTermSpeicher,MNF_ColTermPtrT);
   (*colTermPtr)->term=term;
#if MNF_TEST
   (*colTermPtr)->in = 0;
   (*colTermPtr)->out = 0;
   (*colTermPtr)->outUp = 0;
#endif
   (*colTermPtr)->flags.irred = 1;
   (*colTermPtr)->flags.expanded = 0;
   (*colTermPtr)->flags.newElem = 1;
   MNF_setcolour(**colTermPtr, colour);
   MNF_HashCode (term, Key1, Key2);
   if (parentPtr) {
      (*colTermPtr)->anzGesSchritte = parentPtr->anzGesSchritte + 1;
       (*colTermPtr)->flags.vgl = Vergleich;
       if (RENummer < 0) { /* gleichung */
          (*colTermPtr)->anzAntiRSchritte = parentPtr->anzAntiRSchritte;
          switch (Vergleich) {
          case Groesser: /* old > new */
             (*colTermPtr)->anzGlSchritte = parentPtr->anzGlSchritte;
             MNF_setnirred (*parentPtr);
#if MNF_TEST
             parentPtr->out++;
#endif
             break;
          case Kleiner: /* old < new */
             (*colTermPtr)->anzGlSchritte = parentPtr->anzGlSchritte+1;
#if MNF_TEST
             parentPtr->outUp++;
#endif
             MNF_MAximizeMaxEQUp((*colTermPtr)->anzGlSchritte);
             break;
          default:
          case Unvergleichbar:
             (*colTermPtr)->anzGlSchritte = parentPtr->anzGlSchritte;
#if MNF_TEST
             parentPtr->outUp++;
#endif
             break;
          }
       } else {
          /* Regel */
          (*colTermPtr)->anzGlSchritte = parentPtr->anzGlSchritte;
          switch (Vergleich) {
          case Groesser: /* old > new */
             (*colTermPtr)->anzAntiRSchritte = parentPtr->anzAntiRSchritte;
             MNF_setnirred (*parentPtr);
#if MNF_TEST
             parentPtr->out++;
#endif
             break;
          case Kleiner: /* old < new */
             (*colTermPtr)->anzAntiRSchritte = parentPtr->anzAntiRSchritte+1;
#if MNF_TEST
             parentPtr->outUp++;
#endif
             break;
          default:
          case Unvergleichbar:
             (*colTermPtr)->anzAntiRSchritte = parentPtr->anzAntiRSchritte;
#if MNF_TEST
             parentPtr->outUp++;
#endif
             break;
          }
       }
   }
   else {
      (*colTermPtr)->anzGesSchritte = 0;
      (*colTermPtr)->flags.vgl = Gleich;
      (*colTermPtr)->anzGlSchritte = 0;
      (*colTermPtr)->anzAntiRSchritte = 0;
   }
}


static void MNF_StatNM (MNF_MengeT *menge MAYBE_UNUSEDPARAM) 
{

   MNF_IncSumAnzNachfolger(menge->size);
   MNF_IncAnzCalls;
   MNF_MaximizeMaxNachfolger(menge->size);
}


void MNF_ClearNM (MNF_MengeT *menge)
{
   MNF_StatNM (menge);
   
   HA_Clear(&menge->HashTabelle);

   menge->MNFPQ_Clear (menge->PQRed);
   menge->MNFPQ_Clear (menge->PQGreen);
   if (menge->deallocJoinedCTerm1)
      DeallocData (menge->joinedCTerm1);
   
   if (menge->deallocJoinedCTerm2)
      DeallocData (menge->joinedCTerm2);
   menge->deallocJoinedCTerm1 = FALSE;
   menge->deallocJoinedCTerm2 = FALSE;
   menge->joinable = FALSE;
   menge->size = 0;
} 


/* Testen, ob TermUni mit einem Term der Farbe MitColour unifizierbar ist */
static BOOLEAN MNF_Unifiziert (MNF_MengeT *menge, TermT TermUni, 
                               unsigned short MitColour) 
{
   MNF_ColTermPtrT       colTermPtr1 = NULL;
   HashEntryPtrT         TraversePtr1;

   HA_InitTraverseN (&menge->HashTabelle, &TraversePtr1);
   while (!HA_EndTraverseN (&TraversePtr1)) {
      HA_NextTraverseN (&TraversePtr1, (void **)&colTermPtr1);

      if (MNF_colour_opp (MNF_tcolour(*colTermPtr1), MitColour)) {
         if (U1_Unifiziert (colTermPtr1->term, TermUni)) {
            /* TODO */
            printf("\n\n\a Unifiziert!!!!!!!!!!!!!!!!!!!!!!\n\n");
            menge->joinedCTerm1 = colTermPtr1;
            menge->joinedCTerm2 = NULL;
            menge->deallocJoinedCTerm1 = FALSE;
            menge->deallocJoinedCTerm2 = FALSE;
            menge->joinable = TRUE;
            return TRUE;
         }
      }
   }
   return FALSE;
}



/* liefert TRUE, wenn Elemente zusammengefuehrt werden konnten */
static BOOLEAN
MNF_BearbeiteInsertElem (MNF_ColTermPtrT parentCTerm, long int RENummer,
                         TermT newTerm,
                         void *stack, MNF_MengeT *menge, int colour,
                         VergleichsT Vergleich, TermT redPosition,
                         SeitenT RESeite,
                         RegelOderGleichungsT re
                         )
{
   MNF_ColTermT         *oldCData = NULL;
   MNF_ColTermT         *newCTerm;
   BOOLEAN              Joined = FALSE;
   HashKeyT             Key1, Key2;

   SetColTerm (&newCTerm, newTerm, colour, parentCTerm, Vergleich, 
               RENummer, &Key1, &Key2);
   SetBOBInfo (&(newCTerm->bob_info), redPosition,
               RESeite, parentCTerm, RENummer, re);
   HA_AddData(&(menge->HashTabelle), Key1, Key2, newCTerm, (void **)&oldCData);
   MNF_IncAnzCallsHAAddData;
   if (oldCData != NULL) {
      /* Datensatz existiert schon: Also beide Saetze mergen 
         und Speicher freigeben */
      Joined = AddData (oldCData, newCTerm);
      if (Joined) {
         menge->joinedCTerm1 = oldCData;
         menge->joinedCTerm2 = newCTerm;
         menge->deallocJoinedCTerm1 = FALSE;
         menge->deallocJoinedCTerm2 = TRUE;
         menge->joinable = TRUE;
      }
      else
         DeallocData (newCTerm);
   } else {
      MNF_incID();
      menge->size++;
      menge->MNFPQ_Push(stack, newCTerm);
   }
   return Joined;
}

static BOOLEAN
MNF_AddVarianten (MNF_ColTermPtrT acctCTerm, long int RENummer,
                  TermT newTerm,
                  void *stack, MNF_MengeT *menge, int colour,
                  TermT redPosition,
                  SeitenT RESeite,
                  RegelOderGleichungsT re)
{
   SymbolT sym;
   BOOLEAN ret;

   /* Term ist kein Grundterm */
   SO_forFktSymbole(sym) {
      if (SO_IstSkolemSymbol(sym)) {
         UTermT teilTerm;
         TermT term = TO_Termkopie (newTerm);
         IO_DruckeFlex ("%t\n", term);
         TO_doStellen (term, teilTerm,
                       {
                          if (SO_SymbIstVar (TO_TopSymbol(teilTerm)))
                             TO_SymbolBesetzen(teilTerm, sym);
                       });
         ret = MNF_BearbeiteInsertElem(acctCTerm, RENummer,
                                       term, 
                                       stack, menge, colour, Groesser, 
                                       redPosition, RESeite,
                                       re);
         if (ret)
            return TRUE;
      }
   }
   return FALSE;
}

static void
MNF_VMMengeVonTerm (TermT term, VMengenT *VMenge) 
{
   SymbolT      symbol;

   VM_NeueMengeSeiLeer (*VMenge);
   TO_doSymbole(term, symbol,
                {if (SO_SymbIstVar (symbol))
                   VM_ElementEinfuegen(VMenge,symbol);
                }
                );
}


/* Testet, ob die Variablen von Term1 eine Teilmenge der Variablen
   von Term2 sind */
static BOOLEAN
MNF_VMTeilmengenP (TermT Term1, TermT Term2) 
{
   VMengenT       Menge1, Menge2;
   BOOLEAN        teilmenge;

   if (count_vars (Term1) != 0) {
      MNF_VMMengeVonTerm (Term1, &Menge1);
      MNF_VMMengeVonTerm (Term2, &Menge2);
      teilmenge = VM_Teilmenge (&Menge1, &Menge2);
      VM_MengeLoeschen (&Menge1);
      VM_MengeLoeschen (&Menge2);
      return teilmenge;
   }
   else
      return TRUE;
}


static BOOLEAN
MNF_BearbeiteInsertElemE (MNF_ColTermPtrT acctCTerm, long int RENummer,
                          TermT newTerm,
                          void *stack, MNF_MengeT *menge, int colour,
                          TermT redPosition,
                          SeitenT RESeite,
                          RegelOderGleichungsT re)
{
   VergleichsT    Resultat;

   if (!MNF_VMTeilmengenP (newTerm, acctCTerm->term)) {
      TO_TermLoeschen (newTerm);
      return FALSE;
   }
   
   Resultat = ORD_VergleicheTerme(acctCTerm->term,newTerm);

   if ((Resultat == Kleiner) && 
       (acctCTerm->anzGlSchritte < menge->maxGlSchritte)) {
      MNF_IncAnzEQUp;
      return (MNF_BearbeiteInsertElem (acctCTerm, RENummer, newTerm,
                                       stack, menge, colour, Resultat, 
                                       redPosition, RESeite,
                                       re));
   }
   else if ((Resultat == Groesser)) {
      return (MNF_BearbeiteInsertElem (acctCTerm, RENummer, newTerm,
                                       stack, menge, colour, Resultat, 
                                       redPosition, RESeite,
                                       re));
   }
   
   TO_TermLoeschen (newTerm);
   return FALSE;
}


/* Bearbeite holt ein Element von "stack" und bildet alle direkten
   Nachfolger. Die Nachfolger werden in den Stack "stack" und in
   die HashTabelle eingefuegt.
   MNF_Bearbeite endet, wenn alle Nachfolger gebildet oder
   wenn die Elemente zusammengefuehrt werden konnten
   */
static void MNF_BearbeiteDummy  (void *pq MAYBE_UNUSEDPARAM,
                                 MNF_MengeT *menge MAYBE_UNUSEDPARAM,
                                 int colour MAYBE_UNUSEDPARAM,
                                 MNF_ColTermPtrT acctCTerm MAYBE_UNUSEDPARAM)
{
   ;
}


static void MNF_BearbeiteE(void *stack, MNF_MengeT *menge, int colour, 
                           MNF_ColTermPtrT acctCTerm)
{
   TermT        Kopie;
   GleichungsT  gleichung;
   UTermT  subTerm  = acctCTerm->term;
   UTermT  nullTerm = TO_NULLTerm(subTerm);
   
   MNF_setexpanded(*acctCTerm);
   do {
      if (TO_TermIstNichtVar(subTerm)) {
         MO_AllGleichungMatchInit(subTerm);
         MNF_IncAnzMatchVersucheE;
         while (MO_AllGleichungMatchNext (FALSE, &gleichung)) {
            MNF_IncAnzReduktionen;
            Kopie=MO_AllTermErsetztNeu(acctCTerm->term, subTerm);
            /* Kopie muss freigegeben werden */
            if (MNF_BearbeiteInsertElemE (acctCTerm, gleichung->Nummer, Kopie,
                                          stack, menge, colour, subTerm,
                                          TP_RichtungAusgezeichnet(gleichung)
                                          ? linkeSeite : rechteSeite,
                                          gleichung))
               return;
            MNF_IncAnzMatchVersucheE;
         }
      }
      subTerm = TO_Schwanz(subTerm);
   } while (subTerm != nullTerm);
}


static void MNF_BearbeiteR (void *stack, MNF_MengeT *menge, int colour, 
                            MNF_ColTermPtrT acctCTerm)
{
   TermT   Kopie;
   RegelT  regel;
   UTermT  subTerm  = acctCTerm->term;
   UTermT  nullTerm = TO_NULLTerm(subTerm);
   MNF_setexpanded(*acctCTerm);
   do {
      if (TO_TermIstNichtVar(subTerm)) {
         MO_AllRegelMatchInit(subTerm);
         MNF_IncAnzMatchVersucheR;
         while (MO_AllRegelMatchNext (&regel)) {
            MNF_IncAnzReduktionen;
            Kopie=MO_AllTermErsetztNeu(acctCTerm->term, subTerm);
            /* Kopie muss freigegeben werden! */
            if (MNF_BearbeiteInsertElem (acctCTerm, regel->Nummer, Kopie,
                                         stack, menge, colour, Groesser, 
                                         subTerm,
                                         linkeSeite,
                                         regel))
               return;
            MNF_IncAnzMatchVersucheR;
         }
      }
      subTerm = TO_Schwanz(subTerm);
   } while (subTerm != nullTerm);
}





static void MNF_BearbeiteRE (void *stack, MNF_MengeT *menge, int colour, 
                             MNF_ColTermPtrT acctCTerm) 
{
   
   MNF_BearbeiteR (stack, menge, colour, acctCTerm);
   if (!menge->joinable) {
      MNF_BearbeiteE (stack, menge, colour, acctCTerm);
   }
}


/* versucht eine Regel auf den Term anzuwenden */
static void MNF_BearbeiteEineR (void *stack, MNF_MengeT *menge, int colour,
                               MNF_ColTermPtrT acctCTerm, RegelT Regel)
{
   static int zaehler = 0;
   
   TermT                newTerm;
   TermT                antiRechts = NULL, antiLinks = NULL;
   AntiRegelUseT        useAnti = ARnoUse;
   UTermT  subTerm  = acctCTerm->term;
   UTermT  nullTerm = TO_NULLTerm(subTerm);

   if (acctCTerm->anzAntiRSchritte < menge->maxAntiRSchritte)
      MNF_ConvertRegelToAntiRegel (menge->antiKind,
                                   Regel,
                                   &antiLinks, &antiRechts, &useAnti);

   do {
      if (TO_TermIstNichtVar(subTerm)) {
         MNF_IncAnzMatchVersucheR;
         if (MO_TermpaarPasstMitResultat(Regel->linkeSeite, Regel->rechteSeite,
                                         acctCTerm->term, &newTerm, subTerm)) {
            MNF_IncAnzReduktionen;
            /* newTerm muss freigegeben werden */
            if (MNF_BearbeiteInsertElem(acctCTerm, Regel->Nummer,
                                        newTerm,
                                        stack, menge, colour, Groesser, 
                                        subTerm,
                                        linkeSeite,
                                        Regel)) { /* XXX BL */
               if (useAnti != ARnoUse) {
                  TO_TermLoeschen (antiLinks);
                  TO_TermLoeschen (antiRechts);
               }
               return;
            }
            
         }
         switch (useAnti) {
         case ARuseAnti:
            if ((acctCTerm->anzAntiRSchritte < menge->maxAntiRSchritte)
                && MO_TermpaarPasstMitResultat(antiLinks, antiRechts,
                                               acctCTerm->term, &newTerm, 
                                               subTerm)) {
               MNF_IncAnzReduktionen;
               /* newTerm muss freigegeben werden */
               /* Aufruf stimmt (?) noch nicht, da hier gegen Ordnung
                  gearbeitet wird */
               MNF_IncAnzAntiR;
               if (MNF_BearbeiteInsertElem(acctCTerm, Regel->Nummer,
                                           newTerm, 
                                           stack, menge, colour, Kleiner, 
                                           subTerm,
                                           rechteSeite,
                                           Regel)) { /* XXX BL */
                  TO_TermLoeschen (antiLinks);
                  TO_TermLoeschen (antiRechts);
                  return;
               }
            }
            break;
         case ARuseUnifikation:
            if (MO_TermpaarPasstMitResultat(antiLinks, antiRechts, 
                                            acctCTerm->term, &newTerm, 
                                            subTerm)) {
               if (zaehler++ < 2 &&
                   MNF_AddVarianten (acctCTerm, Regel->Nummer, newTerm,
                                     stack, menge, colour, subTerm,
                                     rechteSeite,
                                     Regel))
                  break;
               if (MNF_Unifiziert (menge, newTerm, MNF_tcolour(*acctCTerm))) {
                  TO_TermLoeschen (newTerm);
                  TO_TermLoeschen (antiLinks);
                  TO_TermLoeschen (antiRechts);
                  return;
               }
               TO_TermLoeschen (newTerm);
            }
            break;
         case ARnoUse:
         default:
            break;
         }
      }
      subTerm = TO_Schwanz(subTerm);
   } while (subTerm != nullTerm);
   if (useAnti != ARnoUse) {
      TO_TermLoeschen (antiLinks);
      TO_TermLoeschen (antiRechts);
   }
}


/* versucht eine Gleichung auf den Term anzuwenden */
static void MNF_BearbeiteEineGl (void *stack, MNF_MengeT *menge, int colour,
                                 MNF_ColTermPtrT acctCTerm, 
                                 GleichungsT Gleichung)
{
   TermT                newTerm;
   UTermT  subTerm  = acctCTerm->term;
   UTermT  nullTerm = TO_NULLTerm(subTerm);
   
   do {
      if (TO_TermIstNichtVar(subTerm)) {
         MNF_IncAnzMatchVersucheE;
         if (MO_GleichungsrichtungPasstMitFreienVarsOhneOrdnung(
                     Gleichung,acctCTerm->term,&newTerm, subTerm)) {
            /* newTerm muss freigegeben werden */
            MNF_IncAnzReduktionen;
            if (MNF_BearbeiteInsertElemE (acctCTerm, Gleichung->Nummer, 
                                          newTerm,
                                          stack, menge, colour, subTerm,
                                          linkeSeite,
                                          Gleichung))
               return;
         }
         /* jetzt links und rechts Vertauschen und das ganze Nochmal */
         /* Gleichung->Gegenrichtung->linkeSeite, rechteSeite */
         else if (TP_IstKeineMonogleichung(Gleichung) &&
                  MO_GleichungsrichtungPasstMitFreienVarsOhneOrdnung(
                      TP_Gegenrichtung(Gleichung),acctCTerm->term, &newTerm, 
                      subTerm)) {
           GleichungsT GGleichung = TP_Gegenrichtung(Gleichung);
           MNF_IncAnzReduktionen;
           if (MNF_BearbeiteInsertElemE (acctCTerm, 
                                         GGleichung->Nummer, newTerm,
                                         stack, menge, colour, subTerm,
                                         rechteSeite,
                                         GGleichung))
             return;
         }
      }
      subTerm = TO_Schwanz(subTerm);
   } while (subTerm != nullTerm);
}


static BOOLEAN MNF_AnalyseNM (MNF_MengeT *menge) {

   if (menge->Expand != ExpandRewrite) {
      if (menge->size > menge->SwitchNF) {
         menge->Expand = ExpandRewrite;
         MNF_SearchInteresting (menge);
         return TRUE;
      }
      else if (menge->size > menge->SwitchIrred) {
         menge->MNF_AddTryAllElements = FALSE;
      }
   }
   return FALSE;
}


/* Expandieren der Nachfolgermenge */
void MNF_ExpandNM (MNF_MengeT *menge) {
   MNF_ColTermPtrT   acctCTerm;
   BOOLEAN           lastRedWasIrred = FALSE;
   BOOLEAN           lastGreenWasIrred = FALSE;

   for(;;) {
      if (!(menge->MNFPQ_Empty)(menge->PQRed)) {
         acctCTerm = (MNF_ColTermPtrT)(menge->MNFPQ_Pop) (menge->PQRed, 
                                                          lastRedWasIrred);
         menge->BearbeiteFunc(menge->PQRed, menge, MNF_red, acctCTerm);
         lastRedWasIrred = MNF_tirred (*acctCTerm);
         if (menge->joinable)
            break;
      }
      if (!(menge->MNFPQ_Empty)(menge->PQGreen)) {
         acctCTerm = (MNF_ColTermPtrT)(menge->MNFPQ_Pop)(menge->PQGreen, 
                                                         lastGreenWasIrred);
         menge->BearbeiteFunc(menge->PQGreen, menge, MNF_green, acctCTerm);
         lastGreenWasIrred = MNF_tirred (*acctCTerm);
         if (menge->joinable)
            break;
      }
      if ((menge->maxSize != 0) && (menge->size > menge->maxSize)) {
         MNF_IncAbbruchMaxSize;
         break;
      }
      if (menge->useAnalyse && MNF_AnalyseNM (menge))
         break;
      if (menge->MNFPQ_Empty(menge->PQRed) && 
          menge->MNFPQ_Empty(menge->PQGreen))
         break;
   }
}


/*****************************************************************************/
/* interne Abfrageroutinen                                                   */
/*****************************************************************************/

static
BOOLEAN MNF_JoinedNM (MNF_MengeT *menge) {
   return (menge->joinable);
}


/*****************************************************************************/
/* exportierte Abfrageroutinen                                               */
/*****************************************************************************/

BOOLEAN MNF_JoinedNMenge (MNF_MengeT *menge) 
{
   return menge->joinable;
}

   
/* Der zurueckgegebene Term darf von der aufgerufenen Funktion
   nicht freigegeben werden. Evtl. muss eine Termkopie gemacht werden */
TermT MNF_GetJoinedTerm (MNF_MengeT *menge)
{
   return menge->joinedCTerm1->term;
}

/* GetJoinedTermpair liefert das zusammengefuehrte bzw. subsummierte Termpaar
   zurueck */
/* Die zurueckgegebenen Terme duerfen von der aufgerufenen Funktion
   nicht freigegeben werden. Evtl. muss eine Termkopie gemacht werden */
void MNF_GetJoinedTermpair (MNF_MengeT *menge, TermT *left, TermT *right) 
{
   MNF_ColTermPtrT       leftPtr, rightPtr;

   /* joinedCTerm1 existiert immer, joinedCTerm2 nicht unbedingt */
   if (menge->joinedCTerm2 == NULL) {
      if (MNF_tcolour_red (*menge->joinedCTerm1)) {
         rightPtr = menge->joinedCTerm1;
         leftPtr = menge->initialCTermLeft;
      } else {
         leftPtr = menge->joinedCTerm1;
         rightPtr = menge->initialCTermRight;
      }
   } else {
      if (MNF_tcolour_red (*menge->joinedCTerm1)) {
         rightPtr = menge->joinedCTerm1;
         leftPtr = menge->joinedCTerm2;
      } else {
         rightPtr = menge->joinedCTerm2;
         leftPtr = menge->joinedCTerm1;
      }
   }
   
   *left = leftPtr->term;
   *right = rightPtr->term;
}


unsigned int MNF_GetSize (MNF_MengeT *menge) 
{
   return menge->size;
}


/*****************************************************************************/
/* Hilfsroutinen                                                             */
/*****************************************************************************/

void MNF_InitFirstMenge (MNF_MengeT *menge) 
{
   menge->PQRed = NULL;
   menge->PQGreen = NULL;
   menge->joinable = FALSE;
   menge->useR = TRUE;
   menge->useE = FALSE;
   menge->maxGlSchritte = 0;
   menge->maxAntiRSchritte = 0;
   menge->antiKind = noAnti;
   menge->maxSize = 1000;
   menge->size = 0;
   menge->BearbeiteFunc = MNF_BearbeiteR;
   menge->MNFPQ_Art = MNFPQ_stack;
   menge->MNF_UseClassify = FALSE;
   menge->MNF_AddTryAllElements = TRUE;
   menge->initialHashSize = 4095;
   menge->initialCTermLeft = NULL;
   menge->initialCTermRight = NULL;
}


void MNF_InitMenge (MNF_MengeT *menge, HashSizeT HashSize) 
{
  BOOLEAN R = menge->useR,
          E = menge->useE;
   menge->BearbeiteFunc = R&&E ? MNF_BearbeiteRE : 
                             R ? MNF_BearbeiteR : 
                             E ? MNF_BearbeiteE : MNF_BearbeiteDummy;

   
   HA_New (&(menge->HashTabelle),  (HA_CmpFuncT) &CmpData, 
           (HA_DeallocFuncT) &DeallocData,
           HashSize);

   MNFPQ_InitMenge (menge);
   
   menge->PQRed  = menge->MNFPQ_GetNewPQ();
   menge->PQGreen= menge->MNFPQ_GetNewPQ();

}


void MNF_PutFirstElementsInMenge(MNF_MengeT *menge, TermT links, TermT rechts) 
{
   /* Achtung: Speicher freigeben fuer sk,tk, wenn diese direkt
      zusammenfuehrbar sind */
   TermT        sk = TO_Termkopie(links),
                tk = TO_Termkopie(rechts);
   MNF_ColTermT *colSk, *colTk;
   MNF_ColTermPtrT oldDataPtr = NULL;
   HashKeyT     Key1T, Key2T, Key1S, Key2S;

   /* sk:linker Term, GRUEN gefaerbt */
   SetColTerm (&colSk, sk, MNF_green, NULL, Gleich, 0, &Key1S, &Key2S);
   SetBOBInfo (&(colSk->bob_info), NULL, linkeSeite, NULL, 0, NULL);
   MNF_incID();
   MNF_setold (*colSk);
   menge->MNFPQ_Push(menge->PQGreen, colSk);
   HA_AddData(&(menge->HashTabelle), Key1S, Key2S, colSk, 
              (void **) &oldDataPtr);
   MNF_IncAnzCallsHAAddData;
   menge->initialCTermLeft = colSk;
   

   /* tk: rechter Term, ROT gefaerbt */
   SetColTerm (&colTk, tk, MNF_red, NULL, Gleich, 0, &Key1T, &Key2T);
   SetBOBInfo (&(colTk->bob_info), NULL, rechteSeite, NULL, 0, NULL);
   MNF_setold (*colTk);
   
   HA_AddData(&(menge->HashTabelle), Key1T, Key2T, colTk, 
              (void **) &oldDataPtr);
   MNF_IncAnzCallsHAAddData;
   if (oldDataPtr == NULL) {
      menge->MNFPQ_Push (menge->PQRed, colTk);
      MNF_incID();
      /* initialCTermRight wird unten gesetzt */
      menge->initialCTermRight = colTk;
      menge->joinable = FALSE;
      menge->size=2;
      menge->joinedCTerm1 = NULL;
      menge->joinedCTerm2 = NULL;
      menge->deallocJoinedCTerm1 = FALSE;
      menge->deallocJoinedCTerm2 = FALSE;
      /* evtl. auf Rewrite umstellen */
      if (menge->useAnalyse)
	(void) MNF_AnalyseNM (menge);
   } else { /* sk, tk sind gleich */
      menge->initialCTermRight = NULL;
      menge->joinable = TRUE;
      menge->joinedCTerm1 = colSk;
      menge->joinedCTerm2 = colTk;
      /* colSk ist im Hash drin, colTk nicht. Deswegen muss colTk separat
         freigegeben werden */
      menge->deallocJoinedCTerm1 = FALSE;
      menge->deallocJoinedCTerm2 = TRUE;
      menge->size=1;
      /* Normalerweise wuerde hier MNF_AddData aufgerufen,
         aber da die Situation klar ist (opposite colour),
         muss hier nur die Statistik korrigiert werden */
      MNF_IncAnzDirektZusammenfuehrbar;
      MNF_IncAnzTermGleichOpp;
      MNF_IncAnzTermGleich;
   }
}




static unsigned int  count_vars (TermT term) 
{
   SymbolT      symbol;
   unsigned int  count=0;

   TO_doSymbole(term, symbol,
                {if (SO_SymbIstVar (symbol))
                      count++;
                }
                );
   return count;
}


static void MNF_ConvertRegelToAntiRegel (MNF_AntiT antiKind,
                                         RegelT Regel MAYBE_UNUSEDPARAM,
                                         TermT *antiLinks MAYBE_UNUSEDPARAM, 
                                         TermT *antiRechts MAYBE_UNUSEDPARAM,
                                         AntiRegelUseT *useAnti)
{
   BOOLEAN res;

   *useAnti = ARnoUse;
   switch (antiKind) {
   case antiWOVar:
   /* Wenn beide Seiten keine Variablen enthalten,
      da dies eine Regel ist, reicht es zu pruefen, ob die linke
      Seite keine Variablen enthaelt.
      */
      res = (count_vars (Regel->linkeSeite) == 0);
      if (res) {
         *antiLinks  = TO_Termkopie(Regel->rechteSeite);
         *antiRechts = TO_Termkopie(Regel->linkeSeite);
         *useAnti = ARuseAnti;
      }
      break;
   case antiBoundVar:
      /* Wenn alle Variablen auf beiden Seiten auftauchen, dann koennen
         wir's versuchen */
      /* Testen, ob Variablen von linker Seite Teilmenge von rechts sind,
         dann ist unsere Bedingung erfuellt */
      res = MNF_VMTeilmengenP (Regel->linkeSeite, Regel->rechteSeite);
      if (res) {
         *antiLinks  = TO_Termkopie(Regel->rechteSeite);
         *antiRechts = TO_Termkopie(Regel->linkeSeite);
         /*
           Fuer Matching muessen Terme normiert sein:
           */
         BO_TermpaarNormierenAlsKP(*antiLinks, *antiRechts);
         *useAnti = ARuseAnti;
      }
      break;
   case antiFreeVar:
      res = MNF_VMTeilmengenP (Regel->linkeSeite, Regel->rechteSeite);
      *antiLinks  = TO_Termkopie(Regel->rechteSeite);
      *antiRechts = TO_Termkopie(Regel->linkeSeite);

      /* Fuer Matching muessen Terme normiert sein: */
      BO_TermpaarNormierenAlsKP(*antiLinks, *antiRechts);
      if (res) {
         *useAnti = ARuseAnti;
      } else {
         *useAnti = ARuseUnifikation;
      }
      break;
   case noAnti:
   default:
      break;
   }
}


/*****************************************************************************/
/* Zielverwaltung                                                            */
/*****************************************************************************/

/* ja, ja, die folgenden zwei Routinen sind haesslich, lokalisieren
   aber den Einfluss von PA_*. BL. */

static MNFPQ_ArtT mnf_PA2MNF_PQ_Art(PA_PQArtT art)
{
  switch (art){
  case PA_PQstack: return MNFPQ_stack;
  case PA_PQdeque: return MNFPQ_deque;
  default:
  case PA_PQfifo:  return MNFPQ_fifo;
  }
}

static MNF_AntiT mnf_PA2MNF_Anti(PA_MNFAntiT anti)
{
  switch (anti){
  default:
  case PA_noAnti:       return noAnti;
  case PA_antiWOVar:    return antiWOVar;
  case PA_antiBoundVar: return antiBoundVar;
  case PA_antiFreeVar:  return antiFreeVar;
  }
}

struct MNF_MengeTS *MNF_InitNMenge (TermT s, TermT t) 
{
   MNF_MengePtrT mengePtr;

   SV_Alloc(mengePtr,MNF_MengeSpeicher,MNF_MengePtrT);
   MNF_InitFirstMenge (mengePtr);
   mengePtr->useE             = PA_useE();
   mengePtr->useR             = PA_useR();
   mengePtr->BearbeiteFunc    = PA_useR()&&PA_useE() ? MNF_BearbeiteRE : 
                                        PA_useR()    ? MNF_BearbeiteR : 
                                        PA_useE()    ? MNF_BearbeiteE : 
                                                       MNF_BearbeiteDummy;
   mengePtr->useAnalyse       = PA_useAnalyse();
   mengePtr->SwitchIrred      = PA_SwitchIrred();
   mengePtr->SwitchNF         = PA_SwitchNF();
   mengePtr->CountNF          = PA_CountNF();
   mengePtr->MNFPQ_Art        = mnf_PA2MNF_PQ_Art(PA_PQ_Art());
   mengePtr->maxSize          = PA_MNFmaxSize();
   mengePtr->maxGlSchritte    = PA_maxGlSchritte();
   mengePtr->maxAntiRSchritte = PA_maxAntiRSchritte();
   mengePtr->antiKind         = mnf_PA2MNF_Anti(PA_antiKind());
   mengePtr->MNF_AddTryAllElements = PA_TryAllElements();
   mengePtr->Expand           = ExpandAll;
   mengePtr->LastPseudoExpandTabPtr = NULL;
   MNF_InitMenge (mengePtr, mengePtr->initialHashSize);
   MNF_PutFirstElementsInMenge (mengePtr, s, t);
   return mengePtr;
}


void MNF_ExpandNMenge (struct MNF_MengeTS *menge) 
{
   if (menge->Expand == ExpandRewrite)
      MNF_PseudoExpand (menge);
   else
      MNF_ExpandNM (menge);
}


void MNF_DeleteNMenge (MNF_MengeT *menge) 
{
   MNF_StatNM (menge);

   HA_Delete(&menge->HashTabelle);
   if (menge->LastPseudoExpandTabPtr) {
      HA_Delete (menge->LastPseudoExpandTabPtr);
      our_dealloc (menge->LastPseudoExpandTabPtr);
   }
   menge->MNFPQ_Delete(menge->PQRed);
   menge->MNFPQ_Delete(menge->PQGreen);

   if (menge->deallocJoinedCTerm1)
      DeallocData (menge->joinedCTerm1);

   if (menge->deallocJoinedCTerm2)
      DeallocData (menge->joinedCTerm2);
   SV_Dealloc (MNF_MengeSpeicher, menge);
}


static void
MNF_AddRENM (MNF_MengeT *menge, RegelOderGleichungsT RE,
             void (*MNF_BearbeiteEineREProc) (void *, MNF_MengeT *, int, 
                                              MNF_ColTermPtrT,  
                                              RegelOderGleichungsT))
{
   MNF_ColTermPtrT       colTermPtr = NULL;
   HashEntryPtrT         TraversePtr1, TraversePtr2;
   
   /* Durchlauf durch alle Daten. Problem ist, dass in
      MNF_BearbeiteEinR Elemente eingefuegt werden, die wir im Moment
      nicht testen wollen. Da aber nur alle bereits expandierten
      Elemente das texpanded-Flag gesetzt haben, sind diese leicht
      heraus zufiltern.  Falls zwischendurch die Elemente
      zusammenfuehrbar waren, dann hatten wir Glueck und brechen ab */
   if (menge->MNF_AddTryAllElements) {
      HA_InitTraverseN(&menge->HashTabelle, &TraversePtr1);

      while (!HA_EndTraverseN(&TraversePtr1) && !menge->joinable) {
         HA_NextTraverseN(&TraversePtr1, (void **)&colTermPtr);
         /* Es werden immer nur bereits expandierte Elemente untersucht,
            nicht expandierte werden in MNF_ExpandNM untersucht */
         if (MNF_texpanded (*colTermPtr)) {
            if (MNF_tcolour (*colTermPtr) == MNF_red) {
              MNF_BearbeiteEineREProc (menge->PQRed, menge, MNF_red, 
                                       colTermPtr, RE);
            }
            else if (MNF_tcolour (*colTermPtr) == MNF_green) {
               MNF_BearbeiteEineREProc (menge->PQGreen, menge, MNF_green, 
                                        colTermPtr, RE);
            }
         }
      }
   }
   else {
      /* nur "interessante Elemente ansehen */
      HA_InitTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2);
      if (!HA_EndTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2))
        HA_NextTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2, 
                          (void **)&colTermPtr);
      while (!HA_EndTraverseI (&menge->HashTabelle, &TraversePtr1, 
                               &TraversePtr2) 
             && !menge->joinable) {
         /* wir testen nur Normalformen und Ursprungselemente */
         if ((colTermPtr->flags.irred) || (colTermPtr->anzGesSchritte == 0)) {
            /* nicht expandierte Elemente werden hier nicht betrachtet */
            if (MNF_texpanded (*colTermPtr)) {
               if (MNF_tcolour (*colTermPtr) == MNF_red) {
                  MNF_BearbeiteEineREProc (menge->PQRed, menge, MNF_red, 
                                           colTermPtr, RE);
               }
               else if (MNF_tcolour (*colTermPtr) == MNF_green) {
                  MNF_BearbeiteEineREProc (menge->PQGreen, menge, MNF_green, 
                                           colTermPtr, RE);
               }
            }
            HA_NextTraverseI (&menge->HashTabelle, &TraversePtr1, 
                              &TraversePtr2, (void **)&colTermPtr);
         }
         /* Elemente, die weder Normalformen noch Ursprungselemente
            sind, werden aus der Schlange geloescht */
         else {
            HA_DelAndNextTraverseI (&menge->HashTabelle, &TraversePtr1, 
                                    &TraversePtr2, (void **)&colTermPtr);
         }
      }
   }
}



void MNF_AddRegelNM (MNF_MengeT *menge, RegelT Regel)
{
   if (menge->Expand != ExpandRewrite)
      MNF_AddRENM (menge, Regel, MNF_BearbeiteEineR);

}


void MNF_AddGleichungNM (MNF_MengeT *menge, GleichungsT Gleichung) 
{
   if (menge->Expand != ExpandRewrite)
      MNF_AddRENM (menge, Gleichung, MNF_BearbeiteEineGl);
}


/*****************************************************************************/
/* Test-Routinen                                                             */
/*****************************************************************************/


BOOLEAN MNF_BildungTest (TermT s, TermT t)
{
   BOOLEAN         retVal;
   MNF_MengePtrT   menge;

   SV_Alloc(menge,MNF_MengeSpeicher,MNF_MengePtrT);
   MNF_InitFirstMenge (menge);
   MNF_PutFirstElementsInMenge (menge, s, t);
   
   retVal = MNF_JoinedNM (menge);

   if (!retVal) {
      MNF_ExpandNM (menge);
      retVal = MNF_JoinedNM (menge);
   }
   MNF_DeleteNMenge (menge);

   return retVal;
}

/*****************************************************************************/
/* Work-In-Progress                                                          */
/*****************************************************************************/

static void MNF_FilterInteresting (MNF_MengeT *menge, 
                                   MNF_ColTermPtrT *colTermPtr,
                                   HashEntryPtrT *TraversePtr1, 
                                   HashEntryPtrT *TraversePtr2,
                                   unsigned int  *countColour, 
                                   unsigned int  *countTotal) 
{

   if ((*colTermPtr)->anzGesSchritte == 0) {
      HA_NextTraverseI (&menge->HashTabelle, TraversePtr1, TraversePtr2, 
                        (void **)colTermPtr);
      /* Ursprungselement wird nicht aufaddiert, ist direkt am Anfang
         in Summe drin */
      (*countTotal)++;
   }
   else if (((*colTermPtr)->flags.irred) && (*countColour < menge->CountNF)) {
      HA_NextTraverseI (&menge->HashTabelle, TraversePtr1, TraversePtr2, 
                        (void **)colTermPtr);
      (*countTotal)++;
      (*countColour)++;
   }
   else
      HA_DelAndNextTraverseI (&menge->HashTabelle, TraversePtr1, TraversePtr2,
                              (void **)colTermPtr);
   
}

/* alle nicht interessanten Elemente rausschmeissen um Menge klein zu halten */
static void MNF_SearchInteresting (MNF_MengeT *menge)
{
   MNF_ColTermPtrT       colTermPtr = NULL;
   HashEntryPtrT         TraversePtr1, TraversePtr2;
   unsigned int          count = 0;
   /* Die Zaehler countL/R zaehlen die Elemente pro Farbe, die
      eingefuegt sind. Da die Ursprungselemente immer eingefuegt
      werden, wird der Zaehler direkt auf 1 gesetzt - dadurch ist der
      Rest des Programmes auch einfacher
      */
   unsigned int          countL = 1;
   unsigned int          countR = 1;
   
   
   /* nur "interessante Elemente ansehen */
   HA_InitTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2);
   if (!HA_EndTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2))
      HA_NextTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2, 
                        (void **)&colTermPtr);
   while (!HA_EndTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2)
          && !menge->joinable) {
      /* wir testen nur Normalformen und Ursprungselemente */
      if (MNF_tcolour_red(*colTermPtr)) {
        MNF_FilterInteresting (menge, &colTermPtr, &TraversePtr1, 
                               &TraversePtr2, &countR, &count);
      }
      else if (MNF_tcolour_green (*colTermPtr)) {
         MNF_FilterInteresting (menge, &colTermPtr, &TraversePtr1, 
                                &TraversePtr2, &countL, &count);
      }
   }
   /*printf("SearchInteresting: count is %lu.\n", count);*/
}

				  



static void MNF_PseudoExpand (MNF_MengeT *menge)
{
   MNF_ColTermPtrT       colTermPtr = NULL;
   MNF_ColTermPtrT       newCTerm;
   HashEntryPtrT         TraversePtr1, TraversePtr2;
   TermT                 termCopy;
   MNF_ColTermPtrT       oldCData;
   BOOLEAN               Joined;
   BOOLEAN               reduced;
   HashKeyT              Key1, Key2;
   

   if (menge->LastPseudoExpandTabPtr) {
      HA_Clear (menge->LastPseudoExpandTabPtr);
   } else {
      /* 1021 ist eine Primzahl */
      menge->LastPseudoExpandTabPtr = our_alloc (sizeof(HashT));
      HA_New (menge->LastPseudoExpandTabPtr,  (HA_CmpFuncT) &CmpData,
	      (HA_DeallocFuncT) &DeallocData, 1021);
   }

   /* nur "interessante Elemente ansehen */
   HA_InitTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2);
   if (!HA_EndTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2))
      HA_NextTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2, 
                        (void **)&colTermPtr);
   while (!HA_EndTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2)
          && !menge->joinable) {
      termCopy = TO_Termkopie (colTermPtr->term);
      reduced = NF_NormalformRE(termCopy);
      if (reduced) {
         /* Reduktion war erfolgreich */
         SetColTerm (&newCTerm, termCopy, MNF_tcolour(*colTermPtr), 
                     colTermPtr, Groesser, 0, &Key1, &Key2);
         if (MNF_tcolour_red(*colTermPtr))
            SetBOBInfo(&(newCTerm->bob_info), NULL, rechteSeite, 
                       colTermPtr, 0, NULL);
         else 
            SetBOBInfo(&(newCTerm->bob_info), NULL, linkeSeite, 
                       colTermPtr, 0, NULL);
         /* Haben wir diesen Wert bereits in HashTabelle?
            dann ist oldCData != NULL */
         HA_CheckData (&(menge->HashTabelle), Key1, Key2, newCTerm, 
                       (void **)&oldCData);
         if (oldCData == NULL) {
            /* einfuegen in lokalen Hash */
            HA_AddData (menge->LastPseudoExpandTabPtr, Key1, Key2, 
                        newCTerm, (void **)&oldCData);
            if (oldCData != NULL) {
               /* Datensatz existiert schon: Also beide Saetze mergen
                  und Speicher freigeben */
               Joined = AddData (oldCData, newCTerm);
               if (Joined) {
               /* oldCData ist in hash gespeichert - soll aber in
                  menge->joinedCTerm1 rein, und oldCData wird spaeter
                  freigegeben.  Deswegen muss eine Kopie gemacht
                  werden */
                  menge->joinedCTerm1 = CopyColTerm (oldCData);
                  menge->joinedCTerm2 = newCTerm;
                  menge->deallocJoinedCTerm1 = TRUE;
                  menge->deallocJoinedCTerm2 = TRUE;
                  menge->joinable = TRUE;
               }
               else {
                  /* Term freigeben */
                  DeallocData (newCTerm);
               }
            } else { /* oldCData == NULL, Element also nicht in
                        menge->HashTabelle */
               ; /* vorerst gibt's dann nichts zu tun */
            }
         }
         else { /* oldCData != NULL */
            MNF_IncAnzTermGleich;
            if (MNF_tcolour_opp(*newCTerm, *oldCData)) {
               /* joined mit Element in menge->hashTabelle */
               MNF_IncAnzTermGleichOpp;
               menge->joinedCTerm1 = oldCData;
               menge->joinedCTerm2 = newCTerm;
               menge->deallocJoinedCTerm1 = FALSE;
               menge->deallocJoinedCTerm2 = TRUE;
               menge->joinable = TRUE;
            }
            else {
               MNF_IncAnzTermGleichCol;
               DeallocData (newCTerm);
            }
         }
      }
      else {
         /* wurde nicht reduziert */
         TO_TermLoeschen (termCopy);
      }
      HA_NextTraverseI (&menge->HashTabelle, &TraversePtr1, &TraversePtr2, 
                        (void **)&colTermPtr);
   }
}

