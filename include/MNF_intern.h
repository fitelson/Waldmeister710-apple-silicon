/**********************************************************************
* Filename : MNF_intern.h
*
* Kurzbeschreibung : MultipleNormalFormen-Bildung: Interne Funktionen
*                    (Verwalten von Nachfolgermengen)
*
* Autoren : Andreas Jaeger
*
* 10.07.96: Erstellung
*
**********************************************************************/

#ifndef MNF_INTERN_H
#define MNF_INTERN_H

/* MNF_TEST: Tests der Implementierung, Werte:
   0: keine Tests
   1: interne Konsistenztests, zusaetzliche Statistik
   2:
   3: Testen, ob Hashvergleich ungleich ist
   */
#define MNF_TEST 0

#include "general.h"
#include "TermOperationen.h"
#include "Hash.h"
#include "RUndEVerwaltung.h"
#include "MNF.h"
#include "MNF_PQ.h"

typedef unsigned long int MNF_HashT;

typedef enum {noAnti, antiWOVar, antiBoundVar, antiFreeVar} MNF_AntiT;

typedef unsigned long int MNF_IdT;

typedef enum {
   MNF_red = 1,
   MNF_green = 2
} MNF_colourT;
   
typedef struct MNF_FlagsTS
{
   unsigned int irred    : 1;
   unsigned int expanded : 1;
   unsigned int newElem  : 1;
   unsigned int colour   : 3; /* was MNF_colourT */
   unsigned int vgl      : 2; /* was VergleichsT */
} MNF_FlagsT;

   
/* Struktur mit Informationen, welche fuer Beweisausgabe benoetigt werden */
typedef struct MNF_BOBInfoTS 
{
   MNF_IdT mnfID;        /* Nr des Terms */
   MNF_IdT parentID;     /* Nr des Elternterms */
   long int          RENummer;     /* Regel, die von parent auf Term angewandt 
                                    * wurde */
   TermT             redPosition;  /* Position des Elternterms an der 
                                    * reduziert wurde */
   SeitenT       RESeite;      /* angewandte Seite der Regel */
   AbstractTime      time;
   RegelOderGleichungsT re;
} MNF_BOBInfoT, *MNF_BOBInfoPtrT;

typedef struct MNF_ColTermTS {
   TermT	     term;		/* Term */
#if MNF_TEST   
   unsigned int      in;		/* Anzahl Vorgaenger */
   unsigned int      out;		/* Anzahl Nachfolger */
   unsigned int      outUp;             /* Anzahl Nachfolger hoch */
#endif
   unsigned int      anzGlSchritte;   /* Anzahl Gleichungsschritte von Start */
   unsigned int      anzAntiRSchritte;  /* Anzahl Regelschritte Up von Start */
   unsigned int     anzGesSchritte;    /* Gesamtanzahl Schritte von Start */
   MNF_FlagsT        flags;
   MNF_BOBInfoT      bob_info;
} MNF_ColTermT, *MNF_ColTermPtrT;


typedef enum {
  ExpandAll, ExpandR, ExpandRIrred, ExpandREIrred, ExpandRewrite
} MNF_ExpandT;


typedef struct MNF_MengeTS 
{
   HashT              HashTabelle;
   HashT              *LastPseudoExpandTabPtr;
   unsigned int       initialHashSize;
   void               *PQRed;
   void               *PQGreen;
   BOOLEAN            joinable;
   unsigned int       maxGlSchritte;
   unsigned int       maxAntiRSchritte;
   MNF_AntiT          antiKind;          /* Art der Regelschritte Up */
   BOOLEAN            useE;
   BOOLEAN            useR;
   BOOLEAN            useAnalyse;
   unsigned int       SwitchIrred;
   unsigned int       SwitchNF;
   unsigned int       CountNF;
   MNF_ExpandT        Expand;
   unsigned int       maxSize;
   unsigned int       size;
   void (*BearbeiteFunc)(void *, struct MNF_MengeTS *, int, MNF_ColTermPtrT);
   MNFPQ_ArtT         MNFPQ_Art;
   MNFPQ_GetNewPQT    MNFPQ_GetNewPQ;
   MNFPQ_PushT        MNFPQ_Push;
   MNFPQ_PopT         MNFPQ_Pop;
   MNFPQ_DeleteT      MNFPQ_Delete;
   MNFPQ_EmptyT       MNFPQ_Empty;
   MNFPQ_ClearT       MNFPQ_Clear;
   BOOLEAN            MNF_UseClassify;
   BOOLEAN            MNF_AddTryAllElements; 
     /* bei addgleichung/regel: Alle Elemente oder nur irreduzible probieren */
   MNF_ColTermPtrT    joinedCTerm1; 
     /* wenn joinable, ist dies der Ergebnis-Term: muss freigegeben werden */
     /* gdw deallocJoinedCTerm1 TRUE ist */
   MNF_ColTermPtrT    joinedCTerm2; 
    /* wenn joinable, ist dies der Ergebnis-Term, dieser muss */
    /* freigegeben werden, wenn deallocJoinedCTerm2 TRUE ist */
   BOOLEAN            deallocJoinedCTerm1;
   BOOLEAN            deallocJoinedCTerm2;
   /* Zeiger auf die initialen Elemente.
      initialCTermLeft ist immer gesetzt;
      initialCTermRight ist nur gesetzt, wenn die initialen Terme 
      nicht gleich sind
      */
   MNF_ColTermPtrT    initialCTermLeft;
   MNF_ColTermPtrT    initialCTermRight;
} MNF_MengeT, *MNF_MengePtrT;



/* MNF_ColTermT, int */
#define MNF_setcolour(colTerm, colour) \
        ((colTerm).flags.colour = colour)

/* MNF_ColTermT ->  */
#define MNF_setexpanded(colTerm) \
           ((colTerm).flags.expanded = 1)

/* MNF_ColTermT  ->  */
#define MNF_setnirred(colTerm) \
     ((colTerm).flags.irred = 0)

/* MNF_ColTermT  ->  */
#define MNF_setirred(colTerm) \
     ((colTerm).flags.irred = 1)
     
/* Bit 1 sagt aus, ob das Element neu ist. Wenn das Bit gesetzt ist,
   ist der Term alt */
/* MNF_ColTermT ->  */
#define MNF_setnew(colTerm) \
           ((colTerm).flags.newElem = 1)
     
/* MNF_ColTermT ->  BOOLEAN */
#define MNF_tnew(colTerm) \
           ((colTerm).flags.newElem)

/* MNF_ColTermT ->  */
#define MNF_setold(colTerm) \
           ((colTerm).flags.newElem = 0)
     
/* MNF_ColTermT -> BOOLEAN */
#define MNF_texpanded(colTerm) \
	((colTerm).flags.expanded)

/* MNF_ColTermT -> int */
#define MNF_tcolour(colTerm) \
	((colTerm).flags.colour)

/* MNF_ColTermT -> BOOLEAN */
#define MNF_tcolour_red(colTerm) \
	((colTerm).flags.colour == MNF_red)

/* MNF_ColTermT -> BOOLEAN */
#define MNF_tcolour_green(colTerm) \
     ((colTerm).flags.colour== MNF_green)
     
/* MNF_ColTermT, MNF_ColTermT -> BOOLEAN */
#define MNF_tirred(colTerm) \
	((colTerm).flags.irred) && ((colTerm).flags.expanded)

/* MNF_ColTermT, MNF_ColTermT -> BOOLEAN */
#define MNF_tcolour_eq(colTerm1, colTerm2) \
	  (MNF_tcolour (colTerm1) == MNF_tcolour (colTerm2))

/* int,int -> BOOLEAN */
#define  MNF_colour_eq(colour1, colour2) \
	  ((colour1) == (colour2))

/* folgende Defines muesen geaendert werden, wenn mit mehr als mit 2 Farben
   gearbeitet wird */
/*int, int -> BOOLEAN */
#define MNF_colour_opp(colour1, colour2) \
	  ((colour1) != (colour2))

#define MNF_tcolour_opp(colTerm1, colTerm2) \
	  (MNF_tcolour (colTerm1) != MNF_tcolour (colTerm2))

void MNF_HashCode (TermT term, MNF_HashT *hash1E, MNF_HashT *hash2E);

/* prozedur zum Testen: liefert TRUE, wenn zusammenfuehrbar */
BOOLEAN MNF_BildungTest (TermT s, TermT t);

/* Folgende Funktionen dienen dazu eine Nachfolgermenge ueber laengere
   Zeit zu verwalten. Es gibt also Funktionen zum
   - Initialisieren,
   - Deinitialisiren
   - "Interreduzieren" mit neuer Regel/Gleichung
   - "Interreduzieren" mit Regelmenge/Gleichungsmenge
   - Feststellen, ob Elemente zusammenfuehrbar sind
*/

void MNF_InitNM (/*@out@*/MNF_MengeT *menge, TermT s, TermT t,
		 int maxGlSchritte, BOOLEAN useR, BOOLEAN useE, 
                 unsigned int maxSize);

void MNF_DeleteNM (MNF_MengeT *menge);

void MNF_ExpandNM (MNF_MengeT *menge);

/* Aus MNF_PQ.h, wegen gegenseitigem Include in den Files */
void MNFPQ_InitMenge (MNF_MengeT *menge);

void MNF_InitFirstMenge (MNF_MengeT *menge);

void MNF_InitMenge (MNF_MengeT *menge, HashSizeT HashSize);

void MNF_PutFirstElementsInMenge(MNF_MengeT *menge, TermT links, TermT rechts);

void MNF_ClearNM (MNF_MengeT *menge);

#endif /* MNF_INTERN_H */
