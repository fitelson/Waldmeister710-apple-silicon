#ifndef CCE_Source
/*
//=============================================================================
//      Projekt:        PRAKTIKUM REDUKTIONSSYSTEME SS 93
//
//      Autoren :       Werner Pitz, 1992
//                      Martin Kronenburg, 1993
//-----------------------------------------------------------------------------
//      Anmerkung:      Dieses Modul ist mit absoluter Sicherheit kein
//                      gutes Beispiel fuer den Entwurf eines C-Programms.
//                      Insbesondere die Art und die Verwendung der
//                      Datenstrukturen spiegelt den Einfluss von LISP
//                      waehrend der Spezifikationsphase wieder.
//                      Eine wesentlich bessere Schnittstelle waere denkbar,
//                      sofern man auf die "Kompatibilitaet" der Zugriffs-
//                      funktionen verzichten wuerde. Dadruch waere die
//                      Trickserei mit den Listenzellen vollkommen
//                      ueberfluessig.
//
//                      Werner.
//
//                      Die Veraenderunegn, die ich fuer das Soommersemester
//                      93 vornehmen musste, haben mit Sicherheit nicht
//                      zu einer groesseren Uebersichtlichkeit des Programms
//                      beigetragen. Ich hoffe jedoch, dass die von mir
//                      eingefuegten Kommentare zum Verstaendnis beitragen,
//                      sollte sich mal ein anderer mit disem Parser be-
//                      schaeftigen.
//
//                      Martin
//
//
//                      Anfang Juli 93 wurde noch das Konzept der Skolem-
//                      konstanten hinzugefuegt. Sie beginnen mit "@_",
//                      gefolgt von der aktuellen Funktionssymbolnummer
//                      gemaess FunctionCounter. Die Zugriffsfunktion
//                      lautet create_const.
//
//                      Martin
//
//=============================================================================
*/

#ifndef    PARSER_H
#define    PARSER_H

#include <stdio.h>

/*
//-----------------------------------------------------------------------------
//     Konstanten und Makros
//-----------------------------------------------------------------------------
*/

#define ERROR_SORT 0

/*
//-----------------------------------------------------------------------------
//     Typvereinbarungen
//-----------------------------------------------------------------------------
*/

#ifndef __LCLINT__ 
#ifndef __cplusplus
typedef enum { false, true } bool;      /* Datentyp bool                */
#endif /* __cplusplus */
#endif /* __LCLINT */

typedef long        func_symb;          /* Datentyp func_symb            */
typedef func_symb   variable;           /* Variable ist gleicher Typ !  */

typedef short       sortT;



typedef enum { 
  completion, 
  confluence, 
  convergence, 
  proof, 
  reduction, 
  termination, 
  database 
} mode;    /*$4712*/
/* Reihenfolge nie nicht aendern!!!! Arnim, 05.03.95 */

extern char *mode_names[];

typedef void*                                   sortdec;
typedef void*                                   sort_spec;
typedef void*                                   sort_chain;

typedef void*                                   func_symbdec;
typedef void*                                   functiondec;
typedef func_symb                               func_symb_list;
typedef void*                                   orderdec;
typedef enum { 
  ordering_kbo, 
  ordering_lpo, 
  ordering_empty,
  ordering_col3,
  ordering_col4,
  ordering_cons
}                                               ordering;
typedef void*                                   order;
typedef void*                                   ord_chain;
typedef unsigned int                            weight;
typedef void*                                   eq_list;
typedef void*                                   term_list;


/*---- Termstruktur --------------------------------------------------------*/

typedef struct termcell { func_symb         code;
                          short             arity;
                          sortT             sort;
                          char              *RealVariableName; /*$4712*/
                                                               /* Uuuh! How horrible! */
                          struct termcell   *argument[1]; } *term;

/*
//-----------------------------------------------------------------------------
//     Zugriffsfunktionen
//-----------------------------------------------------------------------------
*/

void            parse_spec              ( char* );
char*           get_spec_name           ( void );
mode            get_mode                ( void );

/*---- Zugriff auf Sorten ---------------------------------------------------*/

sortdec         get_sorts               ( void );
sortT           sorts_first             ( sortdec );
sortdec         sorts_rest              ( sortdec );
#define         sort_nr(sorte)          ( sorte )

/*---- Zugriff auf Funktionsdeklarationen -----------------------------------*/

sortdec         sig_arg                 ( func_symb );
sortT           sig_sort                ( func_symb );
int             sig_arity               ( func_symb );
func_symb_list  get_sig                 ( void );
func_symb       sig_first               ( func_symb_list );
func_symb_list  sig_rest                ( func_symb_list );
bool            sig_is_fvar             ( func_symb );

/*---- Zugriff auf Ordnungsinformationen ------------------------------------*/

orderdec        get_ordering            ( void );
ordering        ord_name                ( orderdec );
order           ord_chain_list          ( orderdec );
ord_chain       ord_first               ( order );
order           ord_rest                ( order );
func_symb       ord_chain_first         ( ord_chain );
ord_chain       ord_chain_rest          ( ord_chain );
weight          ord_func_weight         ( func_symb );

/*---- Variablen ------------------------------------------------------------*/

term            create_var              ( sortT);

/*---- Gleichungen ----------------------------------------------------------*/

eq_list         get_equations           ( void );
eq_list         get_conclusions         ( void );
term            eq_first_lhs            ( eq_list );
term            eq_first_rhs            ( eq_list );
eq_list         eq_rest                 ( eq_list );

/*---- Terme ----------------------------------------------------------------*/
#define         term_func(term)         ( (term)->code )
#define         term_arg(term,nat)      ( (term)->argument[nat] )
bool            term_eq                 ( term, term );
#define         variablep(term)         ( (term)->code < 0 )

/*---- Ende -----------------------------------------------------------------*/
void            clear_spec              ( void );

/*---- Ausgabefunktionen ----------------------------------------------------*/
char*           map_func                ( func_symb );
void            print_term              ( term );
void            print_pair              ( term, char*, term );

/*---- Ausgabefkt. um Spezifikation zu schreiben ----------------------------*/
void printOrdering (FILE *file) ;
void printSig (FILE *file);
void printSorts (FILE *file);
void printSort (FILE *file, sortT s);

#endif
#endif
