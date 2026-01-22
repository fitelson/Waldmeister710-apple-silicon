/*=================================================================================================================
  =                                                                                                               =
  =  LispLists.h                                                                                                  =
  =                                                                                                               =
  =  Simple Listen aus cons-Zellen zur Verwendung in zeit- und platzunkritischen Situationen.                     =
  =  Ob der Simplizitaet diene der Quelltext als Dokumentation.                                                   =
  =                                                                                                               =
  =  27.04.98 Extrahiert aus Ordnungsgenerator. Thomas.                                                           =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef LISPLISTS_H
#define LISPLISTS_H

#include "general.h"

typedef struct ListenTS {
  void *car,
       *cdr;
} *ListenT;

void LL_InitApriori(void);


#define /*void**/ car(/*ListenT*/ Zelle)                                                                            \
  ((Zelle)->car)

#define /*void**/ cdr(/*ListenT*/ Zelle)                                                                            \
  ((Zelle)->cdr)

ListenT cons(void *car, void *cdr);
ListenT consM(void *car, void *cdr);

void free_cell(ListenT Zelle);
void free_cellM(ListenT Zelle);

void Cons(void *car, ListenT *cdr);
void ConsM(void *car, ListenT *cdr);

ListenT Cdr(ListenT *Zelle);

ListenT swap(ListenT Zelle);

unsigned int Listenlaenge(ListenT Liste);

ListenT ListenelementNr(unsigned int N, ListenT Liste);

BOOLEAN member(void *Element, ListenT Liste);

ListenT insert(void *Element, ListenT Liste);

ListenT append(ListenT Liste1, ListenT Liste2);

void mapc(void (*Funktion)(void*), ListenT Liste);

void reverse(ListenT *Liste);

void filter(BOOLEAN (*Funktion)(ListenT), ListenT Liste, ListenT *Sieger, ListenT *Verlierer);

void maparrange3(VergleichsT (*Funktion)(ListenT), ListenT Liste,
  ListenT *Vergleichbare, ListenT *Unvergleichbare);

void maparrange4(VergleichsT (*Funktion)(ListenT), ListenT Liste,
    ListenT *EchtVergleichbare, ListenT *Gleiche, ListenT *Unvergleichbare);

ListenT list(unsigned int AnzElemente, ...);

void free_list(ListenT Liste);
void free_listM(ListenT Liste);

#endif
