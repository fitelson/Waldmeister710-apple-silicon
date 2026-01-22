/**********************************************************************
* Filename : SpeicherVerwaltung.h
*
* Kurzbeschreibung : Datentypen und Funktionen fuer Freispeicherverwaltung
*
* Autoren : Max
*
* 20.01.94  erstellt.
* 03.03.94  rueberkopiert und angepasst fuer Waldmeister II. Arnim.
* 09.03.94  Statistik der Aufrufe eingebaut. Arnim.
* 17.02.95  Wesentliche Teile fertig entmaxt.
*           Debug-Betrieb verbessert. Thomas.
* 14.03.95  Nested-Betrieb fertiggestellt. Thomas.
* 21.02.97  Manager auf mmap-Basis; Defines _Mit/_Ohne_FreispeicherListen
*           sowie _Mit_/_Ohne_Nesting entfernt (ab Version >(CVS) 1.6)
*           SV_Speichertest nicht unter mmap erprobt (kitzlig)
*           
**********************************************************************/

#ifndef SPEICHERVERWALTUNG_H
#define SPEICHERVERWALTUNG_H


#include "general.h"
#include "SpeicherParameter.h"                      /* liefert den zugreifenden Module Element- und Block-Groessen */
#ifndef CCE_Source
#include "Statistiken.h"
#else
#define if_sst(Action)
#define if_sstF(Action) (void) 0
#endif



/*=================================================================================================================*/
/*= I. Defines                                                                                                    =*/
/*=================================================================================================================*/

/* Alternativen: 0,1 */
#define SV_TEST 0

/* Fuer ...Ein muss im makefile die Anweisung gegeben werden,
   die DEBUGLIBS mit einzulinken. Das Betriebssystem nimmt dann Ueber-
   pruefungen seiner Speicherverwaltung vor. Der Test kann mittels
   Aufruf von int malloc_verify(void) auch explizit aufgerufen werden;
   Rueckgabewerte sind 1 (ok) und 0.
*/




/*=================================================================================================================*/
/*= II. Typdeklarationen; Globalitaeten                                                                           =*/
/*=================================================================================================================*/

typedef struct mem_block_list {                                       /* Deklaration des Typs fuer Speicherbloecke */
  struct mem_block_list *next;                                               /* Zeiger auf naechsten Speicherblock */
  uintptr_t mem_block;                                               /* Beginn des Speicherbereichs fuer Elemente. */
} SpeicherblockT;               /* Der Typ von mem_block ist eigentlich beliebig. Die tatsaechliche Ausdehnung der */
                           /* Instanzen ist nach rechts verschieden und abhaengig von block_size und element_size. */

                                                           
typedef struct StichAdressTS {   /* Zum Aufbau einer verketteten Liste in den angeforderten Speicherbereichen. Die */
  struct StichAdressTS *next_free;     /* Listenelemente werden im Abstand der Groesse des Anforderungstyps in den */
} *StichAdressT;       /* jeweiligen Block geschrieben; ihre Adressen sind die Einstichspunkte in den Block. Daher */
                                               /* traegt die Liste der Stichadressen keine weiteren Informationen. */
struct SV_SpeicherManagerTS {
  unsigned long int block_size;                                      /* Anzahl der Elemente in einem Speicherblock */
  size_t element_size;                                                          /* Groesse eines Elements in Bytes */
  size_t mbl_size;             /* Groesse von block_size Elementen in Bytes zuzueglich Groesse eines Adresszeigers */
                                                     /* oder Inkrement in Bytes bei Verwendung von ManagerMemorium */
  StichAdressT next_free;                                                             /* Zeiger auf freies Element */
  SpeicherblockT *block_list;                                             /* Zeiger auf Liste von SpeicherBloecken */
  struct SV_SpeicherManagerTS *Nachf;                                              /* Zeiger in Liste von Managern */
  BOOLEAN Nested;
  const char *Name;
  if_sst(
    unsigned long Anforderungen;
    unsigned long Freigaben;
    unsigned long InsgesamtAngelegteBloecke;
    unsigned long MaximalGleichzeitigBelegteElemente;
  )
};

typedef struct SV_SpeicherManagerTS SV_SpeicherManagerT,
                                    SV_NestedSpeicherManagerT;

#if SV_TEST
#  define if_SVTest(Action) Action
#  define if_SVTestF(Action) Action
   void check_address(struct SV_SpeicherManagerTS *mbm, void *p);
   void malloc_verify(void);
#  define /*void*/ check_addressF(/*struct SV_SpeicherManagerTS **/ mbm, /*void **/ p) check_address(mbm,p)
#elif !SV_TEST
#  define if_SVTest(Action)
#  define if_SVTestF(Action) (void) 0
#  define /*void*/ check_address(/*struct SV_SpeicherManagerTS **/ mbm, /*void **/ p)
#  define /*void*/ check_addressF(/*struct SV_SpeicherManagerTS **/ mbm, /*void **/ p) (void) 0
#else
#  error "SV_TEST muss definiert sein als 0 oder 1"
#endif


/*=================================================================================================================*/
/*= IV. Zugriffe auf die Systemspeicherverwaltung                                                                 =*/
/*=================================================================================================================*/

/* Die folgenden Funktionen sind den System-Funktionen malloc, free etc. vorgelagert, um ggf. Testaufrufe darum-
   lagern zu koennen. */

void *Xour_alloc(size_t size, const char* loc, int no);
#define our_alloc(x) Xour_alloc((x), __FILE__ , __LINE__)

#define /*Typ**/ struct_alloc(/*Typ*/ Typ)                                                                          \
  ((Typ*) our_alloc(sizeof(Typ)))

#define /*Typ**/ array_alloc(/*unsigned int*/ Anzahl, /*Typ*/ Typ)                                                  \
  ((Typ*) our_alloc((Anzahl)*sizeof(Typ)))

#define /*void*/ array_dealloc(/*void**/ memory_ptr)                                                                \
  our_dealloc(memory_ptr)

#define /*char**/ string_alloc(/*unsigned int*/ Anzahl)                                                             \
  (array_alloc(Anzahl+1,char))

#define /*char**/ string_realloc(/*char **/ ptr, /*unsigned int*/ Anzahl)                                           \
  (array_realloc(ptr,Anzahl+1,char))

#define /*Typ**/ array_realloc(/*void**/ ptr, /*unsigned int*/ Anzahl, /*Typ*/ Typ)                                 \
  (ptr = (Typ*)our_realloc((Typ*)(ptr),(Anzahl)*sizeof(Typ),"array_realloc"))
  
#define /*Typ**/ negarray_alloc(/*unsigned int*/ Anzahl, /*Typ*/ Typ, /*unsigned int*/ Delta)                       \
  (array_alloc(Anzahl,Typ)+(Delta))

#define /*Typ**/ negarray_realloc(/*void**/ ptr, /*unsigned int*/ Anzahl, /*Typ*/ Typ, /*unsigned int*/ Delta)      \
  (ptr = (Typ*)our_realloc((Typ*)(ptr)-(Delta),(Anzahl)*sizeof(Typ),"negarray_realloc")+(Delta))

#define /*Typ**/ posarray_alloc(/*unsigned int*/ Anzahl, /*Typ*/ Typ, /*unsigned int*/ Delta)                       \
  (array_alloc(Anzahl,Typ)-(Delta))

#define /*Typ**/ posarray_realloc(/*void**/ ptr, /*unsigned int*/ Anzahl, /*Typ*/ Typ, /*unsigned int*/ Delta)      \
  (ptr = (Typ*)our_realloc((Typ*)(ptr)+(Delta),(Anzahl)*sizeof(Typ),"posarray_realloc")-(Delta))


void our_dealloc (void *memory_ptr);

#define /*void*/ negarray_dealloc(/*void**/ memory_ptr, /*unsigned int*/ Delta)                                     \
  our_dealloc((memory_ptr)-(Delta))


void *our_realloc (void      *memory_ptr,                                /* Zeiger auf den "alten" Speicherbereich */
                   size_t   size,                                        /* neue Groesse                           */
                   const char *whatfor);                                   /* Speicher wofuer (fuer Fehlermeldung) */



/*=================================================================================================================*/
/*= V. Funktionen fuer gewoehnlichen Speichermanager                                                              =*/
/*=================================================================================================================*/

#define /*void*/ SV_ManagerInitialisieren(/*SV_SpeicherManagerT**/ mbm, /*Typ*/ ElementTyp,                         \
  /*unsigned long int*/ block_size, /*const char**/ Name)                                                           \
  /* element_size ist die Groesse der zu speichernden Elemente in Bytes an,                                         \
     block_size gibt die Anzahl der Elemente fuer einen Speicherblock an.                                           \
     In mbm wird die ADRESSE des Pointers uebergeben, der spaeter auf den Manager zeigen soll. Diese Funktion       \
     legt einen neuen Manager an und setzt den Pointer auf dessen Adresse.                                          \
     Die Funktion ist mehrfachaufruf-clean in dem Sinn, dass weitere Aufrufe auf denselben Manager                  \
     ignoriert werden. Ein Manager gilt genau dann als initialisiert, wenn die Komponente block_size einen von 0    \
     verschiedenen Wert hat. Folglich sind als statische Variablen deklarierte Manager initial nicht initialisiert. \
     Bei dynamisch allokierten Managern muss die Durchfuehrung der Initialisierung dagegen durch eine explizite     \
     Zuweisung des Wertes 0 an block_size erzwungen werden.                                                         \
  */                                                                                                                \
  SV_BeliebigenManagerInitialisieren(mbm,sizeof(ElementTyp),block_size,FALSE,Name)


#define /*void **/ SV_Alloc(/*L-Ausdruck*/ Index, /*SV_SpeicherManagerT*/ mbm, /*Typ*/ Zieltyp)                     \
{                                                                                                                   \
  if_SVTest(malloc_verify());                                                                                       \
  if_sst(MaximierenLong(&(mbm).MaximalGleichzeitigBelegteElemente, ++(mbm).Anforderungen-(mbm).Freigaben));         \
  if ((mbm).next_free) {                                        /* Falls noch Platz in bisher angelegten Bloecken */\
    check_address(&mbm,(mbm).next_free);                                                                            \
    (mbm).next_free = ((StichAdressT)(Index = (Zieltyp)(mbm).next_free))->next_free;                                \
  }                                                                                                                 \
  else                                                                                                              \
   Index = SV_AllocMitNeuemBlock(&mbm);                                                                             \
}  


#define /*void*/ SV_Dealloc(/*SV_SpeicherManagerT*/ mbm, /*void **/p)                                               \
{                                                                                                                   \
  if_SVTest(malloc_verify());                                                                                       \
  if_sst((mbm).Freigaben++;)                                                                                        \
  check_address(&mbm,p);                                                                                            \
  ((StichAdressT)p)->next_free = (mbm).next_free;                                                            \
  (mbm).next_free = (StichAdressT)p;                                                                         \
}

/*--- Hilfsfunktionen fuer obige Makros ---------------------------------------------------------------------------*/

void SV_BeliebigenManagerInitialisieren(SV_SpeicherManagerT *mbm, size_t element_size, 
  unsigned long int block_size, BOOLEAN Nested, const char Name[]);

void *SV_AllocMitNeuemBlock(SV_SpeicherManagerT *mbm);
/* Anlegen eines neuen Blocks von Elementen, Rueckgabe des ersten freien Elementes.*/

void *SV_Blockdurchzeigerung(SV_SpeicherManagerT *mbm, void *StartNeuerBlock, void *Waechter);     /* telling name */



/*=================================================================================================================*/
/*= VI. Funktionen fuer Nested-Speichermanager                                                                    =*/
/*=================================================================================================================*/

/* Konzept fuer NestedManagers:
   Dank der Freispeicherverwaltung sind die von einem Manager spaeter herausgegebenen Speicherbereiche
   untereinander verkettet. Diese Verkettung kann auch fuer die Zielstruktur erhalten werden. Funktionalitaet:
   -ElementTyp muss eine Struktur sein, deren erste Komponente "Nachf" heisst und vom Typ ElementTyp * ist.
   -Es koennen analog zu SV_Alloc mit SV_NestedAlloc einzelne Objekte entnommen werden.
   -Ausserdem ist es moeglich, in einer Abfolge
    IndexNeu = SV_ErstesNextFree(mbm);        -- Initialisierung
    ...
    SV_AufNextFree(mbm,IndexNeu);             -- beliebig oft hintereinander
                                                 IndexNeu muss dabei vor den Aufrufen stets auf das zuletzt erhal-
                                                 tene Objekt verweisen. Nach diesem Aufruf zeigt IndexNeu auf ein
                                                 neues Objekt, das auch zugleich als Nachfolger des bisher neu-
                                                 esten vermerkt ist.
    ...
    SV_LetztesNextFreeMelden(mbm,Zeiger);     -- Zum Abschluss. Zeiger muss dabei auf das naechste freie
                                                 Objekt zeigen.
   -Objekte koennen nur in Form einer ueber die erste Komponente verzeigerten Liste freigegeben werden.
    Das Freigeben reduziert sich dann fuer die Freispeicherverwaltung auf zwei Zuweisungen: Soll eine ueber die
    Komponente Nachf verkettete Liste freigegeben werden, so erhaelt die Freispeicherverwaltung Adresse des ersten
    und des letzten Elements und verwendet diese Verkettung fuer seine Stichpunkt-Liste.
*/


#define /*void*/ SV_NestedManagerInitialisieren(/*SV_NestedSpeicherManagerT*/ mbm, /*Typ*/ ElementTyp,              \
  /*unsigned long int*/ block_size, /*char[]*/ Name)          /* Muss wegen des Tests als Makro formuliert werden */\
{                                                                                                                   \
   ElementTyp Element;                                                                                              \
   ElementTyp *ElementZeiger = &Element;                                                                            \
   if ((void*)ElementZeiger!=&(ElementZeiger->Nachf))                                                               \
     our_error("Fehler: Struktur fuer NestedManager nicht geeignet");                                               \
   SV_BeliebigenManagerInitialisieren(&mbm,sizeof(ElementTyp),block_size,TRUE,Name);                                \
}
/* Mehrfachaufruf-clean in dem Sinn, dass weitere Aufrufe auf denselben Manager ignoriert werden.
   Ein Manager gilt genau dann als initialisiert, wenn die Komponente block_size einen von 0 verschiedenen Wert 
   hat. Folglich sind als statische Variablen deklarierte Manager initial nicht initialisiert. Bei dynamisch 
   allokierten Managern muss die Durchfuehrung der Initialisierung dagegen durch eine explizite Zuweisung des 
   Wertes 0 an block_size erzwungen werden.
 */


#define /*void**/ SV_ErstesNextFree(/*SV_NestedSpeicherManagerT*/ mbm, /*Typ*/ Zieltyp)                             \
( /*if_sstF(mbm.Anforderungen++),*/                                                                                     \
  (Zieltyp) (mbm.next_free ? mbm.next_free : SV_AllocMitNeuemBlock(&mbm))                                           \
)


#define SV_AufNextFree(/*SV_NestedSpeicherManagerT*/ mbm, /*void**/ Index, /*Typ*/ Zieltyp)                         \
{                                                                                                                   \
  StichAdressT NeuesObjekt = ((StichAdressT)Index)->next_free;                                                      \
  Index = (Zieltyp) (NeuesObjekt ? NeuesObjekt                                                                      \
    : (((StichAdressT)Index)->next_free = SV_AllocMitNeuemBlock(&mbm)));                                            \
  /*if_sst(mbm.Anforderungen++);*/                                                                                      \
}                                                                                                                 
                                                                                                                    

#define /*void*/ SV_LetztesNextFreeMelden(/*SV_NestedSpeicherManagerT*/ mbm, /*void**/ Zeiger)                      \
  mbm.next_free = (StichAdressT)(Zeiger);                                                                   
                                                                                                                    
                                                                                                                    
#define /*void*/ SV_NestedDealloc(/*SV_NestedSpeicherManagerT*/ mbm, /*void **/ ErstesElement,                      \
                                                                                           /*void **/LetztesElement)\
(      /* Variante zum Freigeben verketteter Elemente. Erstes und letztes Element muessen nicht verschieden sein. */\
  check_addressF(&mbm,ErstesElement),                                                                               \
  check_addressF(&mbm,LetztesElement),                                                                              \
  (LetztesElement)->Nachf = (void*) mbm.next_free,                                                 \
  mbm.next_free = (StichAdressT)ErstesElement                                                                \
)


#define /*void**/ SV_NestedAlloc(/*L-Ausdruck*/ Index, /*SV_NestedSpeicherManagerT*/ mbm, /*Typ*/ Zieltyp)          \
  SV_Alloc(Index,mbm,Zieltyp)



/*=================================================================================================================*/
/*= VII. Leeren der Freispeicherverwaltung                                                                        =*/
/*=================================================================================================================*/

void SV_AlleManagerLeeren(void);
/* Inhalt aller Manager weggeben, aber Verwaltungsstrukturen behalten */



/*=================================================================================================================*/
/*= VIII. Initialisierung und Statistik                                                                           =*/
/*=================================================================================================================*/

unsigned long int SV_AnzNochBelegterElemente(SV_NestedSpeicherManagerT mbm);
/* funktioniert sowohl fuer gewoehnlichen als auch fuer Nested-Speichermanager */

void SV_InitApriori(void);                                       /* Falls SV-Debug-Betrieb: Aufruf malloc_debug(2) */


#endif
