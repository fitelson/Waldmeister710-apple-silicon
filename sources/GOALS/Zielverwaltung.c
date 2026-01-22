/**********************************************************************
* Filename : Zielverwaltung.c
*
* Kurzbeschreibung : "Mittelschicht" Zielverwaltung,
*                    also gemeinsame Schnittstelle von NormaleZiele und
*                    MNF_Ziele nach oben
*
* Autoren : Andreas Jaeger
*
* 10.07.96: Erstellung aus der urspruenglichen Zielverwaltung heraus
* 22.02.97: Mehrfachaufrufclean. AJ.
**********************************************************************/

/* Konzept:
   Ziele werden in zwei einfachen linearen Listen verwaltet:
   ZielListe: Noch zu beweisende Ziele
   ZielFertigListe: Bewiesene Ziele

   Fuer jedes Ziel wird festgelegt von welcher Zielbehandlung es
   bearbeitet wird.
   */

/* TODO:
   - Paramodulation ordentlich machen (klappt fuer den Fall 1 Ziel)
   - Ausgaben ordentlich
   - Optimieren, doppelten Code eliminieren (teilweise gemacht)
   - Beobachtungslauf
   */

#include "compiler-extra.h"
#include "Ausgaben.h"
#include "Communication.h"
#include "DSBaumKnoten.h"
#include "DSBaumOperationen.h"
#include "DSBaumTest.h"
#include "KPVerwaltung.h"
#include "LispLists.h"
#include "MatchOperationen.h"
#include "MNF_Ziele.h"
#include "NFBildung.h"
#include "Parameter.h"
#include "PhilMarlow.h"
#include "NormaleZiele.h"
#include "RUndEVerwaltung.h"
#include "Statistiken.h"
#include "SymbolOperationen.h"
#include "TermOperationen.h"
#include "Termpaare.h"
#include "general.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include "ClasPatch.h"

/*********************/
/* globale Variablen */
/*********************/

unsigned short int AnzahlZiele;
static Z_BeobachtungsProzT  ZielBeobachtungsProz;
Z_ZielbehandlungsModulT Z_ZielModul = Z_NichtDefiniert;
Z_ZielGesamtStatusT Z_ZielGesamtStatus;


/* ------------------------------------------------------------------------- */
/*                              NEU-WIP                                      */
/* ------------------------------------------------------------------------- */

typedef struct GoalListTS {
   ZielePtrT   firstElem;
   ZielePtrT   lastElem;
} GoalListT;


/*********************/
/* globale Variablen */
/*********************/

/* Speicherverwaltung */
static SV_SpeicherManagerT Z_Speicher;

static GoalListT ZielListe;
static GoalListT ZielFertigListe;


/*
 * Forward-Declarationen
 */
static BOOLEAN TesteTrivialitaeten (void);


/* ------------------------------------------------------------------------- */
/*                               Narrowing                                   */
/* ------------------------------------------------------------------------- */

   
static void AddEqXXIsTrue (void) /* Aufnahme von eq(x,x) -> true */
{
   TermT links = TO_(SO_EQ,SO_ErstesVarSymb,SO_ErstesVarSymb),
         rechts = TO_(SO_TRUE);
   KPV_Axiom(links,rechts);
}

static void ParamodulationsZiel (ZielePtrT element) 
{
   /*
     Hat man existenzquantifizierte Ziele s(i) = t(i), 1<=i<=n, so
     macht man in der Theorie daraus Regeln eq(s(i),t(i)) -> false(i)
     und nimmt zusaetzlich die Regel eq(x,x) -> true auf. Als
     regulaere Zielmenge nimmt man true = false(i). Das
     existenzquantifizierte Ziel i ist genau dann gezeigt, wenn true
     und false(i) zusammengefuehrt wurden. Natuerlich ist es kein
     Problem, zusaetzlich gewoehnliche Ziele zu behandeln.
   */
   TermT Flunder1 = TO_(SO_TRUE),
         Flunder2 = TO_(SO_FALSE),
         links = TO_Termkopie (element->orginalLinks),
         rechts = TO_Termkopie (element->orginalRechts);
   NZ_ZielEinfuegen (element, Flunder1, Flunder2);
   TO_TermeLoeschen (Flunder1,Flunder2);

   Z_UngleichungEinmanteln(&links,&rechts);
   KPV_Axiom(links,rechts);
}


/* ------------------------------------------------------------------------- */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static void InitZielBeobachtungsProz (ZielePtrT zielPtr) {

  switch (zielPtr->modul) {
  case Z_NormaleZiele:
  case Z_Paramodulation:
  case Z_Repeat:
    NZ_InitZielBeobachtungsProz (zielPtr);
    break;
  case Z_MNF_Ziele:
    MNFZ_InitZielBeobachtungsProz (zielPtr);
    break;
  case Z_NichtDefiniert:
    our_error ("Zielverwaltung nicht definiert!");
    break;     
  }
}


static void ListeEinfuegen (GoalListT *liste, ZielePtrT element) 
{
   if (liste->firstElem == NULL) {
      liste->firstElem = element;
      liste->lastElem = element;
      element->Vorg = NULL;
      element->Nachf = NULL;
   } else {
      element->Vorg = liste->lastElem;
      element->Nachf = NULL;
      liste->lastElem->Nachf = element;
      liste->lastElem = element;
   }
}


static void ListeEinfuegenSortiert (GoalListT *liste, ZielePtrT element) 
{
   ZielePtrT  listePtr;
   
   if (liste->firstElem == NULL) {
      /* erstes Element in Liste einfuegen */
      liste->firstElem = element;
      liste->lastElem = element;
      element->Vorg = NULL;
      element->Nachf = NULL;
   } else {
      /* Liste enthaelt bereits Elemente */
      listePtr = liste->firstElem;
      while (listePtr != NULL && (element->nummer > listePtr->nummer)) {
         listePtr = listePtr->Nachf;
      }
      /* Abbruch, also listePtr == NULL 
         oder element->nummer <= listePtr->nummer */
      if (listePtr == NULL) {
         /* Ende der Liste erreicht */
         element->Vorg = liste->lastElem;
         element->Nachf = NULL;
         liste->lastElem->Nachf = element;
         liste->lastElem = element;
      } else {
         /* ab in die Mitte */
         /* einfuegen vor listePtr */
         element->Vorg = listePtr->Vorg;
         element->Nachf = listePtr;
         if (listePtr == liste->firstElem) {
            /* Vorne einfuegen */
            listePtr->Vorg = element;
            liste->firstElem = element;
         } else {
            /* in der Mitte einfuegen */
            listePtr->Vorg->Nachf = element;
            listePtr->Vorg = element;
         }
      }  
   }
}


/* Element aus ZielListe nach ZielFertigListe umhaengen */
static void UmhaengenInListen (ZielePtrT element) 
{
   /* Letztes Element raus ? */
   if (ZielListe.firstElem == ZielListe.lastElem) {
      ZielListe.firstElem = NULL;
      ZielListe.lastElem = NULL;
   } else if (ZielListe.firstElem == element) {
      ZielListe.firstElem = element->Nachf;
      ZielListe.firstElem->Vorg = NULL;
   } else if (ZielListe.lastElem == element) {
      ZielListe.lastElem = element->Vorg;
      ZielListe.lastElem->Nachf = NULL;
   } else {
      element->Vorg->Nachf = element->Nachf;
      element->Nachf->Vorg = element->Vorg;
   }
   ListeEinfuegenSortiert (&ZielFertigListe, element);
   if (element->beobachtProz) {
     element->beobachtProz = NULL;
     if (ZielListe.firstElem) {
       ZielListe.firstElem->beobachtProz = ZielBeobachtungsProz;
       InitZielBeobachtungsProz (ZielListe.firstElem);
     }
   }
}


/* Fuegt Ziel in Liste ein, dabei muss left != right sein */
void Z_EinZielEinfuegen (TermT left, TermT right, unsigned short int Index, 
                         Z_ZielbehandlungsModulT modul)
{
   ZielePtrT zielPtr;

   SV_Alloc (zielPtr, Z_Speicher, ZielePtrT);

   zielPtr->nummer = Index;
   zielPtr->orginalLinks = left;
   zielPtr->orginalRechts = right;
   zielPtr->status = Z_Open;
   zielPtr->zusammenfuehrungszeitpunkt = minimalAbstractTime();
   zielPtr->privat.NZ = NULL;
   if (Index == 1)
     zielPtr->beobachtProz = ZielBeobachtungsProz;
   else
     zielPtr->beobachtProz = NULL;
   zielPtr->modul = modul;
   ListeEinfuegen (&ZielListe, zielPtr);
   /* Modulspezifische Initialisierung */
   switch (modul) {
   case Z_NormaleZiele:
   case Z_Repeat:
     NZ_ZielEinfuegen (zielPtr, left, right);
     break;
   case Z_Paramodulation:
     ParamodulationsZiel (zielPtr);
     break;
   case Z_MNF_Ziele:
     MNFZ_ZielEinfuegen (zielPtr);
     break;
   case Z_NichtDefiniert:
     our_error ("Zielverwaltung nicht definiert!");
     break;
   }
}



void Z_ZieleAusSpezifikationHolen(void)
{
   ListenT ZielIndex;
   BOOLEAN SchonEinExistenzziel = FALSE;
   ZielePtrT zielePtr;

   AnzahlZiele = 0;
   for (ZielIndex=PM_Ziele; ZielIndex; Cdr(&ZielIndex)) {
      ListenT Eintrag = car(ZielIndex);
      TermT links = TO_Termkopie(car(Eintrag)),
            rechts = TO_Termkopie(cdr(Eintrag));
      /* Trivialitaeten werden spaeter in TesteTrivialitaeten behandelt */
      if ((TO_TermIstNichtGrund(links) || TO_TermIstNichtGrund(rechts))
          && !TO_TermeGleich(links,rechts)) { /* Existenzquantifizierung */
         if (PA_ExtModus() == completion)
            our_error("No existential quantification of goals possible in "
                      "COMPLETION mode.");
         if (SchonEinExistenzziel) 
            our_error("Existential quantification is possible for one goal "
                      "only.");
         AddEqXXIsTrue();
         SchonEinExistenzziel = TRUE;
         Z_EinZielEinfuegen (links, rechts, ++AnzahlZiele, Z_Paramodulation);
      }
      else {
         Z_EinZielEinfuegen(links, rechts, ++AnzahlZiele, Z_ZielModul);
         if (ZielBeobachtungsProz)
            (*ZielBeobachtungsProz)(links, rechts);
      }
   }
   if ((zielePtr = ZielListe.firstElem) && COM_PrintIt()) {
      printf( "\nGoals:\n------\n\n" );
      do {
        printf("(%4u)  ", zielePtr->nummer);
        if (zielePtr->modul == Z_Paramodulation)
          NZ_AusgabeZiel (zielePtr, Z_AusgabeInitial);
        else 
          IO_DruckeFlex("%t ?=? %t\n", 
                        zielePtr->orginalLinks, zielePtr->orginalRechts);
      } while ((zielePtr = zielePtr->Nachf));
   }
   if (AnzahlZiele)
     Z_ZielGesamtStatus = Z_Offen;
   else
     Z_ZielGesamtStatus = Z_KeineZiele;

   /* Trivialitaeten testen, Resultat ignorieren */
   (void)TesteTrivialitaeten ();
}



BOOLEAN Z_ZielmengeLeer(void)
/* Liefert TRUE, wenn es kein zu beweisendes Ziel mehr gibt
   (entweder wurden alle Ziele gezeigt, oder es gab keine) */
{
   return (ZielListe.firstElem == NULL);
}


void Z_ZielBeobachtungsProzAnmelden (Z_BeobachtungsProzT proz)
/* Anmeldung einer Prozedur proz, die aufgerufen wird, sobald sich das
   1. Ziel aendert.  Wurde das erste Ziel bewiesen, so wird proz mit
   dem naechsten Ziel aufgerufen.  proz wird insbesondere auch dann
   aufgerufen, sobald das erste Ziel eingefuegt wird.
*/
{
   ZielBeobachtungsProz = proz;
}

/* Liefert TRUE, wenn alle gewuenschten Ziele (gemaess Einstellungen
   alle oder nur das erste) bewiesen werden konnten.*/
BOOLEAN Z_ZieleBewiesen(void) 
{
   return (Z_ZielmengeLeer () ||
           ((GetAnzErfZusfuehrungenZiele() > 0)
            && !PA_allGoals()));
}

/* Anzahl bewiesener Ziele */
int Z_AnzahlBewiesenerZiele (void)
{
  return GetAnzErfZusfuehrungenZiele();
}

/* Anzahl spezifizierter Ziele */
int Z_AnzahlZiele (void) {
  return (int)AnzahlZiele;
}

/* Mindestens ein Ziel wurde bewiesen */
BOOLEAN Z_MindEinZielBewiesen (void)
{
  return GetAnzErfZusfuehrungenZiele() > 0;
}


void Z_ZielmengenBeobachtungsDurchlauf(Z_BeobachtungsDurchlaufProcT AP)   
/* Wendet AP auf jedes Ziel an, wobei AP die Ziele nicht veraendert.*/
{
   ZielePtrT zielePtr;

   zielePtr = ZielListe.firstElem;

   while (zielePtr) {
      switch (zielePtr->modul) {
      case Z_MNF_Ziele:
         MNFZ_ZielBeobachtung (zielePtr, AP);
         break;
      case Z_Paramodulation:
      case Z_NormaleZiele:
      case Z_Repeat:
         NZ_ZielBeobachtung (zielePtr, AP);
	 break;
      default:
         /* TODO */
         break;
      }
      zielePtr = zielePtr->Nachf;
   }
}

static void AusgabeStatus (ZielePtrT ziel) {

  switch (ziel->status) {
  case Z_Open:
    printf(" still open");
    break;
  case Z_Joined: 
    printf(" joined");
    break;
  case Z_Unified:
    printf(" unified");
    break;
  }
}


static void AusgabeListe (GoalListT *liste, const char *title, 
                          Z_AusgabeArtT Z_AusgabeArt) 
{
   ZielePtrT zielePtr;

   zielePtr = liste->firstElem;
   if (zielePtr != NULL) {
      if (Z_AusgabeArt != Z_AusgabeSpez)
         IO_DruckeKurzUeberschrift (title, '*');
      while (zielePtr) {
         if (Z_AusgabeArt != Z_AusgabeSpez) {
            printf("No. %4u\n", zielePtr->nummer);
            printf("--------\n");
            if (zielePtr->status != Z_Open) {
               AusgabeStatus (zielePtr);
               printf("\n");
            }
           IO_DruckeFlex(" original %t ?= %t\n", 
                         zielePtr->orginalLinks, zielePtr->orginalRechts);
         }
         switch (zielePtr->modul) {
         case Z_MNF_Ziele:
            MNFZ_AusgabeZiel (zielePtr, Z_AusgabeArt);
            break;
         case Z_NormaleZiele:
         case Z_Paramodulation:
	 case Z_Repeat:
            NZ_AusgabeZiel (zielePtr, Z_AusgabeArt);
            break;
         default:
            /* TODO */
            break;
         }
         zielePtr = zielePtr->Nachf;
      }
   }
}


void Z_AusgabeZielmenge (Z_AusgabeArtT Z_AusgabeArt)
{
   AusgabeListe (&ZielListe, "Open Goals", Z_AusgabeArt);
   AusgabeListe (&ZielFertigListe, "Proved Goals", Z_AusgabeArt);
}

static void AusgabeZieleEndeListe (GoalListT *liste, const char *title) 
{
   ZielePtrT zielePtr;


   zielePtr = liste->firstElem;
   if (zielePtr != NULL) {
     printf("\n%s:\n", title);
     while (zielePtr) {
       printf("No. %2u: ", zielePtr->nummer);
       if (zielePtr->modul == Z_Paramodulation) {
         NZ_AusgabeZiel (zielePtr, Z_AusgabeKurz);
       } else {
         IO_DruckeFlex(" %t ?= %t", 
                       zielePtr->orginalLinks, zielePtr->orginalRechts);
         if (zielePtr->status != Z_Open) {
           AusgabeStatus (zielePtr);
           printf(", current: ");
           switch (zielePtr->modul) {
           case Z_MNF_Ziele:
             MNFZ_AusgabeZiel (zielePtr, Z_AusgabeKurz);
             break;
           case Z_NormaleZiele:
	   case Z_Repeat:
             NZ_AusgabeZiel (zielePtr, Z_AusgabeKurz);
             break;
           default:
             break;
           }
         }
       }
       printf("\n");
       zielePtr = zielePtr->Nachf;
     }
   }
}

static void printAnzahl (const char *status, const char *was, 
                         const char *were, int Anzahl, int AnzahlGesamt)
{
  if (Anzahl) {
    if (Anzahl == 1 && AnzahlGesamt == 1)
      printf("which %s %s", was, status);
    else if (Anzahl == AnzahlGesamt)
      printf("all %s %s", were, status);
    else if (Anzahl == 1)
      printf("1 %s %s", was, status);
    else
      printf("%d %s %s", Anzahl, were, status);
  }
}

/* Zusammenfassung der Ziele mit Statistik, completed ist TRUE wenn
   das System vervollstaendig ist, so ist unterscheidbar, ob die nicht
   bewiesenen Ziele Wiederlegt wurden oder nicht */
void Z_AusgabeZieleEnde (BOOLEAN completed)
{
  unsigned short int AnzahlBewiesen, AnzahlOpen;

  if (completed)
    AusgabeZieleEndeListe (&ZielListe, "Refuted Goals");
  else
    AusgabeZieleEndeListe (&ZielListe, "Open Goals");

  AusgabeZieleEndeListe (&ZielFertigListe, "Proved Goals");

  AnzahlBewiesen = GetAnzErfZusfuehrungenZiele();
  AnzahlOpen = AnzahlZiele - AnzahlBewiesen;

  switch (AnzahlZiele) {
  case 0:
    printf("No goals were specified.\n");
    return;
  case 1:
    printf("1 goal was specified, ");
    break;
  default:
    printf("%hu goals were specified, of which ", AnzahlZiele);
    break;
  }
  
  printAnzahl ("proved", "was", "were", AnzahlBewiesen, AnzahlZiele);
  if (AnzahlBewiesen != AnzahlZiele && AnzahlBewiesen > 0)
    printf(" and ");
  if (completed) {
    printAnzahl ("refuted", "was", "were", AnzahlOpen, AnzahlZiele);
  } else {
    printAnzahl ("still open", "is", "are", AnzahlOpen, AnzahlZiele);
  }
  printf(".\n");
  if (AnzahlZiele) {
    if (AnzahlBewiesen == AnzahlZiele)
      Z_ZielGesamtStatus = Z_AlleBewiesen;
    else if (AnzahlBewiesen == 0 && completed)
      Z_ZielGesamtStatus = Z_AlleWiederlegt;
    else if (AnzahlBewiesen && completed)
      Z_ZielGesamtStatus = Z_EinigeWiederlegt;
    else if (AnzahlBewiesen && !completed)
      Z_ZielGesamtStatus = Z_EinigeBewiesen;
    else
      Z_ZielGesamtStatus = Z_Offen;
  }
}

/* Parameter sagt, ob return TRUE ausgefuehrt wird oder nicht */
#define ErledigtErfolgsTest(/*BOOLEAN*/abort_if_result_true)    \
      if (result) {                                             \
         ZielePtrT nachf;                                       \
         if (zielePtr->status == Z_Joined)                      \
            IncAnzErfZusammenfuehrungenZiele;                   \
         nachf = zielePtr->Nachf;                               \
         UmhaengenInListen (zielePtr);                          \
         zielePtr = nachf;                                      \
         if (!PA_allGoals() && abort_if_result_true) \
            return TRUE;                                        \
         if (PA_statisticsAfterProved())                              \
           ST_GoalProvedStatistik ("Proved a Goal");            \
      } else {                                                  \
         zielePtr = zielePtr->Nachf;                            \
      }

   
/* durch alle Ziele durchlaufen und auf Trivialitaeten testen. */
static BOOLEAN TesteTrivialitaeten (void)
{
   ZielePtrT zielePtr;
   BOOLEAN result;
   
   zielePtr = ZielListe.firstElem;
   while (zielePtr) {
      switch (zielePtr->modul) {
      case Z_MNF_Ziele:
         result = MNFZ_TesteZielErledigt (zielePtr);
         break;
      case Z_NormaleZiele:
      case Z_Repeat:
         result = NZ_TesteZielErledigt (zielePtr);
         break;
      case Z_Paramodulation:
      default:
         result = FALSE;
         break;
      }
      ErledigtErfolgsTest (FALSE);
   }
   return TRUE;
}

/* Alle Ziele auf Reduzierbarkeit mit dem neuen Faktum pruefen. Falls
   (je nach Modus) ein bzw. alle Ziele zusammengefuehrt werden
   konnten, wird TRUE zurueckgegeben, sonst FALSE. Zusammengefuehrte
   Ziele werden aus der Zielmenge entfernt.*/

BOOLEAN Z_ZieleErledigtMitNeuemFaktum(RegelOderGleichungsT Faktum)
{
   ZielePtrT zielePtr;
   BOOLEAN   result = FALSE;

   zielePtr = ZielListe.firstElem;
   while (zielePtr) {
      switch (zielePtr->modul) {
      case Z_MNF_Ziele:
         result = MNFZ_ZielErledigtMitNeuemFaktum (zielePtr, Faktum);
         break;
      case Z_Paramodulation:
      case Z_NormaleZiele:
      case Z_Repeat:
        result = NZ_ZielErledigtMitNeuemFaktum (zielePtr, Faktum);
        break;
      default:
         /* TODO */
         break;
      }
      ErledigtErfolgsTest (TRUE);
   }
   return Z_ZieleBewiesen ();
}

/* Falls nicht nach jeder Einfuegung die Ziele behandelt wurden,
   muessen sie am Ende der Vervollstaendigung nochmals durchlaufen werden.
 */
BOOLEAN Z_FinalerZielDurchlauf(void)
{
   ZielePtrT zielePtr;
   BOOLEAN   result = FALSE;

   if (PA_ExtModus() == proof)
      return FALSE; /* Kein Durchlauf => keine Aenderung gegenueber vorher. */

   zielePtr = ZielListe.firstElem;

   while (zielePtr) {
      switch (zielePtr->modul) {
      case Z_MNF_Ziele:
         result = MNFZ_ZielFinalerDurchlauf (zielePtr);
         break;
      case Z_Paramodulation:
      case Z_NormaleZiele:
      case Z_Repeat:
         result = NZ_ZielFinalerDurchlauf (zielePtr);
         break;
      default:
         /* TODO */
         break;
      }
      ErledigtErfolgsTest (FALSE);
   }
   return Z_ZielmengeLeer ();
}

static void Z_CleanupListe (GoalListT *liste) 
{
   ZielePtrT zielePtr, nachf;

   zielePtr = liste->firstElem;

   while (zielePtr) {
      switch (zielePtr->modul) {
      case Z_MNF_Ziele:
         MNFZ_LoeschePrivateData (zielePtr);
         break;
      case Z_Paramodulation:
      case Z_NormaleZiele:
      case Z_Repeat:
         NZ_LoeschePrivateData (zielePtr);
         break;
      default:
         /* TODO */
         break;
      }
      TO_TermLoeschen (zielePtr->orginalLinks);
      TO_TermLoeschen (zielePtr->orginalRechts);
      
      nachf = zielePtr->Nachf;
      SV_Dealloc (Z_Speicher, zielePtr);
      zielePtr = nachf;
   }
}

/* Alle Ziele incl. Termen wegwerfen. */
void Z_Cleanup (void) 
{
   Z_CleanupListe (&ZielListe);
   Z_CleanupListe (&ZielFertigListe);
}

/*===========================================================================
  =  Zielbehandlung                                                         =
  ===========================================================================*/


BOOLEAN Z_InitAposteriori(void)
{
   SV_ManagerInitialisieren(&Z_Speicher, ZieleT, Z_SpeicherBlock, "Zielmenge");
   AnzahlZiele = 0;

   ZielListe.firstElem = NULL;
   ZielListe.lastElem = NULL;
   ZielFertigListe.firstElem = NULL;
   ZielFertigListe.lastElem = NULL;

   SetAnzErfZusfuehrungenZiele(0);

   ZielBeobachtungsProz = NULL;
 
   switch (PA_zielBehandlung()){
   default:
   case PA_snf: 
     Z_ZielModul = Z_NormaleZiele; 
     break;
   case PA_rnf: 
     Z_ZielModul = Z_Repeat;
     break;
   case PA_mnf: 
     Z_ZielModul = Z_MNF_Ziele;  
     MNFZ_InitAposteriori ();
     break;
   }
   return TRUE;
}



void Z_UngleichungEinmanteln(UTermT *links, UTermT *rechts)
{
   TermT Neu;
   TO_NachfBesetzen(TO_TermEnde(*links),*rechts);
   TO_TermzelleHolen(Neu);
   TO_SymbolBesetzen(Neu,SO_EQ);
   TO_NachfBesetzen(Neu,*links);
   TO_EndeBesetzen(Neu,TO_TermEnde(*rechts));
   *links = Neu;
   *rechts = TO_(SO_FALSE);
}


BOOLEAN Z_IstGemantelteUngleichung(TermT links, TermT rechts)
{
  if (PM_Existenzziele()) {
    SymbolT f = TO_TopSymbol(links),
            g = TO_TopSymbol(rechts);
    return (f==SO_EQ && g==SO_FALSE) || (f==SO_FALSE && g==SO_EQ);
  }
  else
    return FALSE;
}

BOOLEAN Z_IstGemanteltesTermpaar(TermT links, TermT rechts)
{
  return (TO_TopSymbol(links) == SO_EQ) ||
         (TO_TopSymbol(rechts) == SO_EQ);
}


void Z_UngleichungEntmanteln(UTermT *OhneMantelLinks, 
                             UTermT *OhneMantelRechts, 
                             TermT MitMantelLinks, TermT MitMantelRechts)
{
  TermT MitMantel = 
    SO_SymbGleich(TO_TopSymbol(MitMantelLinks),SO_EQ) ? MitMantelLinks 
                                                      : MitMantelRechts;
  *OhneMantelLinks = TO_ErsterTeilterm(MitMantel);
  *OhneMantelRechts = TO_NaechsterTeilterm(*OhneMantelLinks);
}


BOOLEAN Z_UngleichungEntmantelt(UTermT *OhneMantelLinks, 
                                UTermT *OhneMantelRechts, 
                                TermT MitMantelLinks, TermT MitMantelRechts)
{
  if (PM_Existenzziele()) {
    SymbolT f = TO_TopSymbol(MitMantelLinks),
            g = TO_TopSymbol(MitMantelRechts);
    TermT MitMantel = f==SO_EQ && g==SO_FALSE ? MitMantelLinks : 
                      f==SO_FALSE && g==SO_EQ ? MitMantelRechts : NULL;
    if (MitMantel) {
      *OhneMantelLinks = TO_ErsterTeilterm(MitMantel);
      *OhneMantelRechts = TO_NaechsterTeilterm(*OhneMantelLinks);
      return TRUE;
    }
  }
  return FALSE;
}


BOOLEAN Z_IstSkolemvarianteZuUngleichung(TermT links, TermT rechts)
{
  TermT s, t;
  if (Z_UngleichungEntmantelt(&s,&t, links,rechts)) {
    RegelT Ungleichung;
    RE_forRegelnRobust(Ungleichung, {
      TermT u; TermT v;
      if (RE_UngleichungEntmantelt(&u,&v, Ungleichung) && 
          MO_TupelSkolemAllgemeiner(v,u, s,t))
        return TRUE;
    })
  }
  return FALSE;
}


BOOLEAN Z_IstGrundungleichung(TermT links, TermT rechts)
{
  return Z_IstGemantelteUngleichung(links,rechts) &&
    TO_TermIstGrund(links) && TO_TermIstGrund(rechts);
}
