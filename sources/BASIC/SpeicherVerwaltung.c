/**********************************************************************
* Filename : SpeicherVerwaltung.c
* Funktionen fuer Freispeicherverwaltung
*
* Autoren : Max Wolf, erstellt am : 20.01.94
*
* Am 03. und 04.03.94 kopiert und angepasst. Arnim
* Am 09.03. Speicher-Statistik eingefuehrt.
*
* 17.02.95  Wesentliche Teile fertig entmaxt.
*           Debug-Betrieb verbessert. Thomas.
* 14.03.95  Nested-Betrieb fertiggestellt. Thomas.
*
**********************************************************************/

#include <string.h>

#include "Ausgaben.h"
#include "compiler-extra.h"
#include "FehlerBehandlung.h"
#include "SpeicherVerwaltung.h"

#ifndef CCE_Source
#include "WaldmeisterII.h"
#include "Parameter.h"
#else
#define PA_allocLog() (FALSE)
#endif

/*=================================================================================================================*/
/*= II. Typdeklarationen;  Globalitaeten                                                                          =*/
/*=================================================================================================================*/

static const size_t Alignierung = sizeof(struct{char c; int i;})-sizeof(int);


#define StichAdrPlus(/*void**/ Zeiger,/*long int*/ Delta)                                                           \
 ((StichAdressT)((ADDRESS) (Zeiger) + Delta))

SV_SpeicherManagerT *ManagerListe;



/*=================================================================================================================*/
/*= III. SV_Speichertest                                                                                          =*/
/*=================================================================================================================*/

#if SV_TEST

#define /*unsigned long int*/ KleinsteAdresseImBlock(/*SpeicherblockT*/ Speicherblock, /*SV_SpeicherManagerT*/ mbm) \
  ((uintptr_t) &(Speicherblock->mem_block))

#define /*unsigned long int*/ GroessteAdresseImBlock(/*SpeicherblockT*/ Speicherblock, /*SV_SpeicherManagerT*/ mbm) \
  (((uintptr_t) &(Speicherblock->next)) + mbm->mbl_size - 1)

#define /*BOOLEAN*/ AdresseImBlock(/*void**/ Adresse, /*SpeicherblockT*/ Speicherblock, /*SV_SpeicherManagerT*/ mbm)\
  (KleinsteAdresseImBlock(Speicherblock,mbm)<=(uintptr_t) Adresse                                           \
    && (uintptr_t) Adresse<=GroessteAdresseImBlock(Speicherblock,mbm))

static void CheckManagers(void* Adresse)     /* Prueft bei allen bisher fuer die Speichermanager angelegten Bloek- */
{                    /* ken, ob adresse innerhalb eines solchen Blockes liegt. Wenn ja: Fehlermeldung und Abbruch. */
 SV_SpeicherManagerT *runny = ManagerListe;
  while (runny) {
    SpeicherblockT *Index = runny->block_list;
    while (Index) {
      if (AdresseImBlock(Adresse,Index,runny))
        our_error("Fehlerhafte Adresse in CheckManagers erkannt!");
      Index = Index->next;
    } 
    runny = runny->Nachf;
  }
}

void check_address(SV_SpeicherManagerT *mbm, void *Adresse)
{
  BOOLEAN AdresseGefunden = FALSE;
  SV_SpeicherManagerT *runny = ManagerListe;
  if (Adresse < (void *) 0x100){
    our_error("Fehlerhafte Adresse in check_address erkannt! (0)");
  }
  while (runny) {
    SpeicherblockT *Index = runny->block_list;
    while (Index) {
      if (AdresseImBlock(Adresse,Index,runny)){
        if (runny!=mbm)
          our_error("Fehlerhafte Adresse in check_address erkannt! (1)");
        else
          AdresseGefunden = TRUE;
      }
      Index = Index->next;
    }
    if (runny==mbm && !AdresseGefunden)
      our_error("Fehlerhafte Adresse in check_address erkannt! (2)");
    runny = runny->Nachf;
  }
  if (!AdresseGefunden)
    our_error("Fehlerhafte Adresse in check_address erkannt! (3)");
}


#else

#define /*void*/ CheckManagers(/*void**/ adresse)

#endif



/*=================================================================================================================*/
/*= IV. Zugriffe auf die Systemspeicherverwaltung                                                                 =*/
/*=================================================================================================================*/

void *Xour_alloc (size_t size, const char* loc, int no)
{
  void *rescue = (void *) malloc(size);
  if (PA_allocLog()){
    printf("our_alloc: %8d (%s:%d) -> %p\n", size, loc, no, rescue);
  }
  if (!rescue)
    our_out_of_mem_error("\nSorry, no more memory!\n\nBye...\n\n");
  CheckManagers(rescue);
  CheckManagers((ADDRESS)rescue+size-1);
  return rescue;
}


void our_dealloc (void *memory_ptr)
{
  CheckManagers(memory_ptr);
  if (PA_allocLog()){
    printf("our_dealloc: %p\n", memory_ptr);
  }
  free(memory_ptr);
}


void *our_realloc (void *memory_ptr,                 /* Zeiger auf den "alten" Speicherbereich (als VAR-Parameter) */
                   size_t size,                                                                    /* neue Groesse */
                   const char *whatfor)                                    /* Speicher wofuer (fuer Fehlermeldung) */
{
  void *rescue;
  CheckManagers(memory_ptr);
  rescue = (void *) realloc (memory_ptr, size);
  if (PA_allocLog()){
    printf("our_realloc: %8d: (%s) %p --> %p\n", size, whatfor, memory_ptr, rescue);
  }
  if (!rescue) {
    printf("Out of memory during call to realloc.");
    our_out_of_mem_error(whatfor);
  }
  CheckManagers(rescue);
  CheckManagers((ADDRESS)rescue+size-1);
  return rescue;
}



/*=================================================================================================================*/
/*= V., VI. Funktionen fuer gewoehnlichen und fuer Nested-Speichermanager                                         =*/
/*=================================================================================================================*/

#define /*unsigned long int*/ CeilQuot(/*unsigned long int*/ a, /*unsigned long int*/ b)                            \
  ( (b*(a/b)==a) ? a/b : a/b+1)                                                              /* Wert: ceiling(a/b) */


void SV_BeliebigenManagerInitialisieren(SV_SpeicherManagerT *Manager, size_t element_size,
  unsigned long int block_size, BOOLEAN Nested, const char Name[])
/* Manager ist Zeiger auf den zu initialisierenden Manager.
   element_size gibt die Groesse der zu speichernden Elemente in Bytes an. 
   block_size gibt die Anzahl der Elemente fuer einen Speicherblock an.
   Name ist der fuer die Statistik eingerichtete Name des Managers.
   Die Funktion ist mehrfachaufruf-clean in dem Sinn, dass weitere Aufrufe auf denselben Manager ignoriert werden.
   Ein Manager gilt genau dann als initialisiert, wenn die Komponente block_size einen von 0 verschiedenen Wert 
   hat. Folglich sind als statische Variablen deklarierte Manager initial nicht initialisiert. Bei dynamisch 
   allokierten Managern muss die Durchfuehrung der Initialisierung dagegen durch eine explizite Zuweisung des 
   Wertes 0 an block_size erzwungen werden.
*/
{
  if (!Manager->block_size) {
    size_t align_rest;
    Manager->Nested = Nested;                                                 /* Flag fuer Kategorie Nested setzen */
    if ((align_rest = element_size % Alignierung))                                                     /* Alignment */
      element_size += Alignierung - align_rest;
    if ((Manager->block_size = block_size) < 2)                     /* sonst keine Stichpunkte-Liste einschreibbar */
      our_error("Fehler: block_size muss mindestens zwei betragen.");

    Manager->element_size = element_size;
    Manager->mbl_size = sizeof(struct mem_block_list *)+element_size*block_size;
    Manager->next_free = NULL;
    Manager->block_list = NULL;
    Manager->Nachf = ManagerListe;
    ManagerListe = Manager;
    Manager->Name = Name;
    if_sst({
      Manager->Anforderungen =
      Manager->Freigaben =
      Manager->InsgesamtAngelegteBloecke =
      Manager->MaximalGleichzeitigBelegteElemente = 0;
    });
  }
}


void *SV_AllocMitNeuemBlock(SV_SpeicherManagerT *mbm)
{                              /* Anlegen eines neuen Blocks von Elementen, Rueckgabe des ersten freien Elementes. */
  void *first;
  first = SV_Blockdurchzeigerung(mbm,Xour_alloc(mbm->mbl_size, mbm->Name, mbm->element_size),NULL);
  return first;
}


void *SV_Blockdurchzeigerung(SV_SpeicherManagerT *mbm, void *StartNeuerBlock, void *Waechter)
{
  StichAdressT first = (StichAdressT) &((SpeicherblockT *)StartNeuerBlock)->mem_block,           /* erstes Element */
               pastlast = StichAdrPlus(first,mbm->block_size*mbm->element_size),         /* hinter letztem Element */
               IndexVorg = StichAdrPlus(first,mbm->element_size),                               /* zweites Element */
               Index = StichAdrPlus(IndexVorg,mbm->element_size);                               /* drittes Element */
  if_sst(mbm->InsgesamtAngelegteBloecke++);
  ((SpeicherblockT *)StartNeuerBlock)->next = mbm->block_list;
  mbm->block_list = StartNeuerBlock;
  mbm->next_free = IndexVorg;   /* globales next_free = 2. Element. Fuer Nesting unnoetig; sonst wichtig, wichtig! */
  check_address(mbm,first);
  first->next_free = IndexVorg;                                  /* fuer Nesting wichtig, wichtig! Sonst unnoetig. */
  while (Index<pastlast)                                         /* next_free Zeiger im neuen Block initialisieren */
    Index = StichAdrPlus(IndexVorg = IndexVorg->next_free = Index,mbm->element_size);
  IndexVorg->next_free = Waechter;                   /* Nachfolger des letzten Einstichpunktes je nach Betriebsart */
  return first;
}



/*=================================================================================================================*/
/*= VII. Leeren der Freispeicherverwaltung                                                                        =*/
/*=================================================================================================================*/

void SV_AlleManagerLeeren(void)              /* Inhalt aller Manager weggeben, aber Verwaltungsstrukturen behalten */
{
  SV_SpeicherManagerT *Index = ManagerListe;
  while (Index) {
    SpeicherblockT *Index2 = Index->block_list;                   /* alle Speicherbloecke dieses Managers weggeben */
    while (Index2) {
      SpeicherblockT *Index2Nachf = Index2->next;
      our_dealloc(Index2);
      Index2 = Index2Nachf;
    }
    Index->block_list = NULL;                                                             /* keine Bloecke mehr da */
    Index->next_free = NULL;                                                     /* und keine Speicherstueckelchen */
    Index = Index->Nachf;                                                           /* naechsten Manager vornehmen */
  }
  if_sst({
    SV_SpeicherManagerT *Manager = ManagerListe;
    while (Manager) {
      Manager->Anforderungen =
      Manager->Freigaben =
      Manager->InsgesamtAngelegteBloecke =
      Manager->MaximalGleichzeitigBelegteElemente = 0;
      Manager = Manager->Nachf;
    }
  });
}


/*=================================================================================================================*/
/*= VIII. Initialisierung und Statistik                                                                           =*/
/*=================================================================================================================*/

void SV_InitApriori(void) {
#ifndef CCE_Source
  if (WM_ErsterAnlauf()) {
#if SV_TEST
    /*    malloc_debug(2);*/
#endif
  }
#endif
}



unsigned long int SV_AnzNochBelegterElemente(SV_NestedSpeicherManagerT mbm MAYBE_UNUSEDPARAM)
{
#if DM_STATISTIK
  if (mbm.Nested) {
#if 0
    StichAdressT StichpunktIndex = mbm.next_free;
    unsigned long int AnzFreierElemente = 0;
    while (StichpunktIndex)                                                /* Laenge der next_free-Liste ermitteln */
    {
      AnzFreierElemente++;
      StichpunktIndex = StichpunktIndex->next_free;
    }                                                  /* und von der Laenge der maximal moeglichen Liste abziehen */
#else
    /* copes with cyclic lists, cf. Steele, Common Lisp, The Language, 2nd ed, p. 414 (discussion of list-length) */
    StichAdressT fast = mbm.next_free;
    StichAdressT slow = fast;
    unsigned long int AnzFreierElemente = 0;
    while (fast){
      AnzFreierElemente++;
      fast = fast->next_free;
      if (fast == NULL) break;
      AnzFreierElemente++;
      fast = fast->next_free;
      slow = slow->next_free;
      if (fast == slow){/* cyclic */
        unsigned long int cycleLength = 1;
        fast = fast->next_free;
        while (fast != slow){
          cycleLength++;
          fast = fast->next_free;
        }
        printf("Cyclic term cell list, cycle length = %ld\n", cycleLength);
        break;
      }
    }
#endif
    return mbm.InsgesamtAngelegteBloecke*mbm.block_size - AnzFreierElemente;
  }
  else
    return mbm.Anforderungen-mbm.Freigaben;
#else
  return 0;
#endif
}

#if SV_TEST
void malloc_verify(void){our_error("wo sind denn die malloc-verify-Libs???"}
#endif
