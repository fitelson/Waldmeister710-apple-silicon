/*=================================================================================================================
  =                                                                                                               =
  =  LispLists.c                                                                                                  =
  =                                                                                                               =
  =  Simple Listen aus cons-Zellen zur Verwendung in zeit- und platzunkritischen Situationen.                     =
  =  Ob der Simplizitaet diene der Quelltext als Dokumentation.                                                   =
  =                                                                                                               =
  =  27.04.98 Extrahiert aus Ordnungsgenerator. Thomas.                                                           =
  =                                                                                                               =
  =================================================================================================================*/


#include "LispLists.h"
#include "SpeicherVerwaltung.h"
#include <stdarg.h>
#include "WaldmeisterII.h"

static SV_SpeicherManagerT LL_FreispeicherManager;

ListenT cons(void *car, void *cdr)
{
  ListenT Zelle;
  SV_Alloc(Zelle,LL_FreispeicherManager,ListenT);
  Zelle->car = car;
  Zelle->cdr = cdr;
  return Zelle;
}

ListenT consM(void *car, void *cdr)
{
  ListenT Zelle = struct_alloc(struct ListenTS);
  Zelle->car = car;
  Zelle->cdr = cdr;
  return Zelle;
}


void free_cell(ListenT Zelle)
{
  SV_Dealloc(LL_FreispeicherManager,Zelle);
}

void free_cellM(ListenT Zelle)
{
  our_dealloc(Zelle);
}


void Cons(void *car, ListenT *cdr)
{
  *cdr = cons(car,*cdr);
}

void ConsM(void *car, ListenT *cdr)
{
  *cdr = consM(car,*cdr);
}


ListenT Cdr(ListenT *Zelle)
{
  return (*Zelle = cdr(*Zelle));
}


ListenT swap(ListenT Zelle)
{
  return cons(cdr(Zelle),car(Zelle));
}


unsigned int Listenlaenge(ListenT Liste)
{
  unsigned int Laenge;
  for(Laenge = 0; Liste; Liste = cdr(Liste))
    Laenge++;
  return Laenge;
}


ListenT ListenelementNr(unsigned int N, ListenT Liste)
{
  while (--N)
    Liste = cdr(Liste);
  return car(Liste);
}


BOOLEAN member(void *Element, ListenT Liste)
{
  while (Liste) {
    if (Element==car(Liste))
      return TRUE;
    Cdr(&Liste);
  }
  return FALSE;
}


ListenT insert(void *Element, ListenT Liste)
{
  return member(Element,Liste) ? Liste : cons(Element,Liste);
}


ListenT append(ListenT Liste1, ListenT Liste2)
{
  if (Liste1) {
    ListenT Index = Liste1;
    while (cdr(Index))
      Index = cdr(Index);
    cdr(Index) = Liste2;
    return Liste1;
  }
  else
    return Liste2;
}


void mapc(void (*Funktion)(void*), ListenT Liste)
{
  while (Liste) {
    (*Funktion)(car(Liste));
    Cdr(&Liste);
  }
}


void reverse(ListenT *Liste)
{
  if (*Liste) {
    ListenT Vorg = NULL,
            Index = *Liste,
            weiter;
    do {
      weiter = cdr(Index);
      Index->cdr = Vorg;
      Vorg = Index;
    } while ((Index = weiter));
    *Liste = Vorg;
  }
}



void filter(BOOLEAN (*Funktion)(ListenT), ListenT Liste, ListenT *Sieger, ListenT *Verlierer)
{
  *Sieger = *Verlierer = NULL;
  while (Liste) {
    ListenT Element = car(Liste);
    if ((*Funktion)(Element))
      *Sieger = cons(Element, *Sieger);
    else
      *Verlierer = cons(Element, *Verlierer);
    Cdr(&Liste);
  }
}


void maparrange3(VergleichsT (*Funktion)(ListenT), ListenT Liste,
  ListenT *Vergleichbare, ListenT *Unvergleichbare)
{
  *Vergleichbare = *Unvergleichbare = NULL;
  while (Liste) {
    ListenT Element = car(Liste);
    switch ((*Funktion)(Element)) {
    case Groesser:
      *Vergleichbare = cons(Element, *Vergleichbare);
      break;
    case Kleiner:
      *Vergleichbare = cons(swap(Element), *Vergleichbare);
      break;
    default:
      *Unvergleichbare = cons(Element, *Unvergleichbare);
    }
    Cdr(&Liste);
  }
}


void maparrange4(VergleichsT (*Funktion)(ListenT), ListenT Liste,
    ListenT *EchtVergleichbare, ListenT *Gleiche, ListenT *Unvergleichbare)
  {
    *EchtVergleichbare = *Gleiche = *Unvergleichbare = NULL;
    while (Liste) {
      ListenT Element = car(Liste);
      switch ((*Funktion)(Element)) {
      case Groesser:
        *EchtVergleichbare = cons(Element, *EchtVergleichbare);
        break;
      case Kleiner:
        *EchtVergleichbare = cons(swap(Element), *EchtVergleichbare);
        break;
      case Gleich:
        *Gleiche = cons(Element, *Gleiche);
        break;
      default:
        *Unvergleichbare = cons(Element, *Unvergleichbare);
      }
      Cdr(&Liste);
    }
  }


ListenT list(unsigned int AnzElemente, ...)
{
  va_list Argumente;
  ListenT Liste = NULL;
  va_start(Argumente,AnzElemente);
  if (AnzElemente) {
    void *Element = va_arg(Argumente,void*);
    ListenT Index = cons(Element,NULL);
    Liste = Index;
    while (--AnzElemente) {
      Element = va_arg(Argumente,void*);
      Index = cdr(Index) = cons(Element,NULL);
    }
  }
  va_end(Argumente);
  return Liste;
}


void free_list(ListenT Liste)
{
  while (Liste) {
    ListenT Nachf = cdr(Liste);
    free_cell(Liste);
    Liste = Nachf;
  }
}

void free_listM(ListenT Liste)
{
  while (Liste) {
    ListenT Nachf = cdr(Liste);
    free_cellM(Liste);
    Liste = Nachf;
  }
}


void LL_InitApriori(void)
{
  SV_ManagerInitialisieren(&LL_FreispeicherManager, struct ListenTS, LL_Blockgroesse, "Conszellen fuer Lisp-artige Listen");
}
