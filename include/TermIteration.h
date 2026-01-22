/*=================================================================================================================
  =                                                                                                               =
  =  TermIteration.h                                                                                              =
  =                                                                                                               =
  =  Werkzeug zum iterativen Durchlaufen von Termen                                                               =
  =                                                                                                               =
  =                                                                                                               =
  =  01.02.95  Thomas.                                                                                            =
  =  07.02.95  Thomas. Zweiten Stapel eingefuegt.                                                                 =
  =                                                                                                               =
  =================================================================================================================*/

#ifndef TERMITERATION_H
#define TERMITERATION_H


#include "general.h"
#include "SpeicherParameter.h"
#include "TermOperationen.h"


/* Realisierung eines Stapels, in dem die Termzellen mit offenen Termendezeigern mitgefuehrt werden nebst einem
   Zaehler, der angibt, wie hoch der momentane Argumentbedarf des Teilterms noch ist. 
   Der Stapel wird durch ein Feld dargestellt. Das Element 0 bleibt unbenutzt.

   Soll in einem Modul TermIteration verwendet werden, so muss dies mit TI_IterationDeklarieren(xy) im Implemen-
   tationsteil deklariert werden. xy steht dabei fuer das Namenskuerzel des benutzenden Moduls.
   Dadurch wird die Definition von entsprechenden Variablen in den Quelltext eingefuegt sowie eine Funktion
   unsigned int xy_AusdehnungKopierstapel(void) erzeugt fuer die Statistik. 
   Dieser Mechanismus macht es moeglich, dass jedes Modul einen eigenen Stapel verwendet.
   Die erste Ausdehnung ist fuer alle Felder dieselben und wird festgelegt durch TI_ErsteKopierstapelhoehe
   in SpeicherParameter.h.

   Ergaenzung 07.02.95: Das Modul stellt nun jedem Benutzer einen weiteren Stapel zur Verfuegung, der mit
   TI2_IterationDeklarieren(xy) deklariert wird. Alles weitere wie oben, jedoch Ansprechen jeweils beginnend mit
   dem Kuerzel TI2_ anstatt TI_.

*/


/*=================================================================================================================*/
/*= I. Eintragstyp                                                                                                =*/
/*=================================================================================================================*/

typedef struct {                                                          /* Eintragsstruktur wie oben vorgestellt */
  UTermT Teilterm;
  unsigned int Zaehler;
} TI_KSEintragsT;



/*=================================================================================================================*/
/*= II. Anwendbare Operationen etc.                                                                               =*/
/*=================================================================================================================*/

/* ---------------------------------- */
/* Operationen fuer den ersten Stapel */
/* ---------------------------------- */

#define /*BOOLEAN*/ TI_PopUndLeer(/*void*/)                                                                         \
  (!(--TI_KSIndex))


#define /*BOOLEAN*/ TI_PopUndNichtLeer(/*void*/)                                                                    \
  (--TI_KSIndex)


#define /*void*/ TI_Pop(/*void*/)                                                                                   \
  (--TI_KSIndex)


#define /*void*/ TI_Push(/*UTermT*/ Term)                                                                           \
( (void) ( TI_KSIndex==TI_KSDimension ?                                                                             \
             array_realloc(TI_Kopierstapel,(TI_KSDimension += TI_ErsteKopierstapelhoehe)+1,TI_KSEintragsT) : NULL ),\
  (void) ( TI_Kopierstapel[++TI_KSIndex].Teilterm = Term ),                                                         \
         ( TI_Kopierstapel[TI_KSIndex].Zaehler = TO_AnzahlTeilterme(Term) )                                         \
)


#define /*void*/ TI_PushMitAnz(/*UTermT*/ Term, /* unsigned int*/ Stelligkeit)                                      \
( (void) ( TI_KSIndex==TI_KSDimension ?                                                                             \
             array_realloc(TI_Kopierstapel,(TI_KSDimension += TI_ErsteKopierstapelhoehe)+1,TI_KSEintragsT) : NULL ),\
  (void) ( TI_Kopierstapel[++TI_KSIndex].Teilterm = Term ),                                                         \
         ( TI_Kopierstapel[TI_KSIndex].Zaehler = Stelligkeit )                                                      \
)


#define /*void*/ TI_PushOT(/*UTermT*/ Term)                                                                         \
( (void) ( TI_KSIndex==TI_KSDimension ?                                                                             \
             array_realloc(TI_Kopierstapel,(TI_KSDimension += TI_ErsteKopierstapelhoehe)+1,TI_KSEintragsT) : NULL ),\
         ( TI_Kopierstapel[++TI_KSIndex].Zaehler = TO_AnzahlTeilterme(Term) )                                       \
)


#define TI_TopZaehler (TI_Kopierstapel[TI_KSIndex].Zaehler)


#define TI_TopTeilterm (TI_Kopierstapel[TI_KSIndex].Teilterm)


#define /*unsigned int*/ TI_Stapelhoehe(/*void*/)                                                                   \
  TI_KSIndex


#define /*void*/ TI_KSLeeren(/*void*/)                                                                              \
  (TI_KSIndex = 0)


/* ----------------------------------- */
/* Operationen fuer den zweiten Stapel */
/* ----------------------------------- */

#define /*BOOLEAN*/ TI2_PopUndLeer(/*void*/)                                                                        \
  (!(--TI2_KSIndex))


#define /*BOOLEAN*/ TI2_PopUndNichtLeer(/*void*/)                                                                   \
  (--TI2_KSIndex)


#define /*void*/ TI2_Pop(/*void*/)                                                                                  \
  (--TI2_KSIndex)


#define /*void*/ TI2_Push(/*UTermT*/ Term)                                                                          \
( (void) ( TI2_KSIndex==TI2_KSDimension ?                                                                           \
            array_realloc(TI2_Kopierstapel,(TI2_KSDimension += TI_ErsteKopierstapelhoehe)+1,TI_KSEintragsT) : NULL ),\
  (void) ( TI2_Kopierstapel[++TI2_KSIndex].Teilterm = Term ),                                                       \
         ( TI2_Kopierstapel[TI2_KSIndex].Zaehler = TO_AnzahlTeilterme(Term) )                                       \
)


#define /*void*/ TI2_PushMitAnz(/*UTermT*/ Term, /* unsigned int*/ Stelligkeit)                                     \
( (void) ( TI2_KSIndex==TI2_KSDimension ?                                                                           \
             array_realloc(TI2_Kopierstapel,(TI2_KSDimension += TI_ErsteKopierstapelhoehe)+1,TI_KSEintragsT) : NULL ),\
  (void) ( TI2_Kopierstapel[++TI2_KSIndex].Teilterm = Term ),                                                       \
         ( TI2_Kopierstapel[TI2_KSIndex].Zaehler = Stelligkeit )                                                    \
)


#define /*void*/ TI2_PushOT(/*UTermT*/ Term)                                                                        \
( (void) ( TI2_KSIndex==TI2_KSDimension ?                                                                           \
             array_realloc(TI2_Kopierstapel,(TI2_KSDimension += TI_ErsteKopierstapelhoehe)+1,TI_KSEintragsT) : NULL ),\
         ( TI2_Kopierstapel[++TI2_KSIndex].Zaehler = TO_AnzahlTeilterme(Term)                                        \
)


#define TI2_TopZaehler (TI2_Kopierstapel[TI2_KSIndex].Zaehler)


#define TI2_TopTeilterm (TI2_Kopierstapel[TI2_KSIndex].Teilterm)


#define /*unsigned int*/ TI2_Stapelhoehe(/*void*/)                                                                  \
  TI2_KSIndex


#define /*void*/ TI2_KSLeeren(/*void*/)                                                                             \
  (TI2_KSIndex = 0)



/*=================================================================================================================*/
/*= III. Deklaration                                                                                              =*/
/*=================================================================================================================*/

/* ------------------------------------------------------- */
/*  Deklaration fuer den ersten Stapel                     */
/* ------------------------------------------------------- */

#define TI_IterationDeklarieren(Modulkuerzel)                                                                       \
                                                                                                                    \
  static unsigned int TI_KSDimension = 0;                       /* Index des letzten speicherbaren Stapelelements */\
  static TI_KSEintragsT *TI_Kopierstapel = NULL;                                         /* Zeiger auf Feldanfang */\
  static unsigned int TI_KSIndex = 0;                                 /* Index des letzten vorgenommenen Eintrags */\
                                                                                                                    \
  unsigned int Modulkuerzel ## _AusdehnungKopierstapel(void);           /* Funktionsprototyp mal erst deklarieren */\
  unsigned int Modulkuerzel ## _AusdehnungKopierstapel(void)                                                        \
  {                                                                                                                 \
   return TI_KSDimension;                                                                                           \
  }                                                                                                                 \


/* -------------------------------------------------------- */
/*  Deklaration fuer den zweiten Stapel                     */
/* -------------------------------------------------------- */

#define TI2_IterationDeklarieren(Modulkuerzel)                                                                      \
                                                                                                                    \
static unsigned int TI2_KSDimension = 0;                        /* Index des letzten speicherbaren Stapelelements */\
static TI_KSEintragsT *TI2_Kopierstapel = NULL;                                          /* Zeiger auf Feldanfang */\
static unsigned int TI2_KSIndex = 0;                                  /* Index des letzten vorgenommenen Eintrags */\
                                                                                                                    \
unsigned int Modulkuerzel ## _AusdehnungKopierstapel2(void);          /* Funktionsprototyp mal erst deklarieren */  \
unsigned int Modulkuerzel ## _AusdehnungKopierstapel2(void)                                                         \
{                                                                                                                   \
 return TI2_KSDimension;                                                                                            \
}                                                                                                                   \

#endif
