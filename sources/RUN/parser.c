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
//                      gefolgtvon der aktuellen Funktionssymbolnummer
//                      gemaess FunctionCounter. Die Zugriffsfunktion
//                      lautet create_const.
//
//                      Martin
//

  - Aenderungen mit $4711 im Kommentar ermoeglichen die Verarbeitung
    von "-" als Symbol. Thomas
  - 07.03.95 Datei so modifiziert, dass Compiler keine warnings mehr ausspuckt. Arnim
  - 07.03.95 Von WaldmeisterII nicht benoetigte Funktionalitaet entfernt. Arnim
  - 23.10.97 Erweiterung um  Datenbank-Dateien. Aenderungen mit $4712 im Kommentar. Thomas
  - 15.02.97 Alle Meldungen uebersetzt, kleinere Aenderungen.

//=============================================================================
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "parser.h"
#include "antiwarnings.h"
#include "compiler-extra.h"
#include "Ausgaben.h"
#include "general.h"

/*
//-----------------------------------------------------------------------------
//     Defines
//-----------------------------------------------------------------------------
*/

/* interne Tests fuer parser:
   Werte 0 fuer aus, 1 fuer ein
   */
#define     PS_TEST 0

#define     ALIGN         8         /* Alignment von Rechneradressen*/

#define     CR          '\n'        /* Zeilenvorschub               */
#define     TAB         '\t'        /* Tabulator                    */
#define     LENGTH      4096        /* Maximale Zeilenlaenge        */
#define     IDENTLENGTH  25         /* Maximale Laenge eines Ident. */

#define     VTSIZE       16         /* Groesse der Hashtables fuer  */
                                    /*   Variablenbaum              */
                                    /*  2er Potenz !                */
#define     VTMASK       15         /* Eins kleiner als VTSIZE      */
#define     SUBSIZE      16         /* Groesse der Hashtables fuer  */
                                    /*   Substitutionen             */
                                    /*  2er Potenz !                */
#define     SUBMASK      15         /* Eins kleiner als VTSIZE      */

#define     LISTALLOC   10          /* Min. Allocierung von         */
                                    /* Listenelementen              */
#define     TREEALLOC   10          /* Min. Allocierung von         */
                                    /* Baumknoten (vartree)         */
#define     SUBSTALLOC  100         /* Min. Allocierung von         */
                                    /* Substitutionszellen          */
#define     TERMALLOC  1000         /* Min. Allocierung von         */
                                    /* Func-Termelementen           */
#define     PAIRALLOC  1000         /* Min. Allocierung von         */
                                    /* Termpaare                    */
#define     VARALLOC   1000         /* Min. Allocierung von         */
                                    /* Var-Termelementen            */
#define     MAXFUNCTION 100         /* Maximalzahl von Funktionen   */
#define     MAXSORT     100         /* Maximalzahl von Sorten       */
#define     BUFFERSIZE 9999         /* Stringlaenge fuer Puffer     */

#define     ERR_SORT_ID "ERROR_SORT"  /* Name fuer ERROR_SORT       */

#if 0
/* ist in general.h definiert */
#define     min(x,y)       ((x < y) ? x : y)
#define     max(x,y)       ((x > y) ? x : y)
#endif

#define     Allocate(size) (void *)(malloc (size))
#define     flush()        fflush (stdout)

#define     deb(x) { printf( "%s\n", x ); flush (); }

/*
//-----------------------------------------------------------------------------
//     Typvereinbarungen
//-----------------------------------------------------------------------------
*/

char *mode_names[] = { 
  "completion", 
  "confluence", 
  "convergence", 
  "proof",
  "reduction",
  "termination",
  "database" 
};

/*---- print-Modus ----------------------------------------------------------*/

typedef enum { flat, varsort, full }    modus;

/*---- list -----------------------------------------------------------------*/

typedef struct lc { long        data;
                    long        info;
                    int         weight;
                    sortT       sort;
                    char        ident[IDENTLENGTH];
                    void        *pointer1;
                    void        *pointer2;
                    struct lc   *next;          } listcell;

typedef struct    { listcell    *first;
                    listcell    *last;
                    listcell    *act1;
                    listcell    *act2;          } list;

#define EmptyList { NULL, NULL, NULL, NULL }


/*---- vartree --------------------------------------------------------------*/

typedef struct vtree { variable      code[VTSIZE];
                       variable      info[VTSIZE];
                       short         count[VTSIZE];
                       struct vtree* son;

#if PS_TEST
                         short       debug;
#endif
                                                        } vartree;

/*---- sort -----------------------------------------------------------------*/

typedef struct { char       ident[IDENTLENGTH];
                 int        counter, max;
                 char       *var;                       } SortInfo;


/*---- term -----------------------------------------------------------------*/

typedef struct { char               ident[IDENTLENGTH];
                 short              arity;
                 int                weight;
                 struct termcell    *freelist;
                 list               signature;          } SigInfo;

#define extended        999             /* Erweiterung von Modus !      */

/*---- scanner --------------------------------------------------------------*/

typedef enum { SUNKNOWN, SIDENT,
               SEQUAL, SLESS, SGREATER,
               SCRSYM, SCOLLON, SCOMMA, SBRACKET_L, SBRACKET_R,
               SARROW,
               SMODE, SCOMPLETION, SCONVERGENCE, SREDUCTION, SPROOF, SCONFLUENCE, STERMINATION,
                 SDATABASE, /*$4712*/
               SNAME,
               SSORTS,
               SSIGNATURE,
               SORDERING, SKBO, SLPO, 
               SPATTERNS, /*$4712*/
               SVARIABLES,
               SEQUATIONS,
               SCONCLUSION                                       } Symbol;

typedef struct { char *symbol; /*char   symbol[16];*/
                 Symbol code;       } SymbolEntry;


/*
//-----------------------------------------------------------------------------
//     Variablen
//-----------------------------------------------------------------------------
*/

/*---- list -----------------------------------------------------------------*/

static listcell *freelist;


/*---- vartree --------------------------------------------------------------*/

static  vartree *freevartree;


/*---- sort -----------------------------------------------------------------*/

static sortT       SortCounter;
static SortInfo    Sorts [MAXSORT];   /*={ {"VOID", 0, 0, NULL}, 
                                         { NULL , 0, 0, NULL} }*/
                                /* Initialisierung erfolgt in parse_spec */

/*---- signature ------------------------------------------------------------*/

static bool        *is_fvar; /*$4712*/
static int         is_fvar_size; /*$4712*/


/*---- term -----------------------------------------------------------------*/

static func_symb   FunctionCounter;
static variable    VariableCounter;

static SigInfo     Signature [MAXFUNCTION];

static char        varstring[7]="Vxyzuvw";     /* Variablen fuer Pretty-Print */
static const int   varstrlen = 6;    /* Anzahl der Variablen        */


static int        min_weight; /* Minimales Gewicht eines Funktionssymbols */

static int        term_counter;

/*---- scanner --------------------------------------------------------------*/

static  bool    EOI;    /* Scanner: Ist true, falls Ende der Eingabe erreicht*/

static  SymbolEntry  SymbolTab[19]; /*$4712*/

static  FILE         *input          ; 
static  bool         inputopen       ; 
static  bool         uppercase       ; 
static  char         ScanChar        ; 
static  char         scantext[LENGTH]; 
static  short        scanline        ; 
static  short        scanpos         ; 
static  short        scanlength      ; 


/*---- parser ---------------------------------------------------------------*/

static  mode         spec_mode;
static  char         spec_name[IDENTLENGTH]; 
static  list         spec_sorts            ; 
static  list         spec_signature        ; 
static  ordering     spec_ordering_mode; 
static  list         spec_ordering         ; 
static  list         spec_variables        ; 
static  list         spec_equations        ; 
static  list         spec_conclusions      ; 


/*---- ausgabe --------------------------------------------------------------*/

static  list         list_of_equations;  
static  list         list_of_conclusions;
                                        
static  vartree      *v_tree;            


#define INIT_VARIABLES()                                                                                                   \
list_of_equations.first = list_of_equations.last  = list_of_equations.act1 = list_of_equations.act2=NULL;                  \
list_of_conclusions.first = list_of_conclusions.last  = list_of_conclusions.act1 = list_of_conclusions.act2=NULL;          \
v_tree                    = NULL;                                                                                          \
spec_name[0]     = '\0';                                                                                                   \
spec_sorts.first=spec_sorts.last=spec_sorts.act1=spec_sorts.act2=NULL;                                                     \
spec_signature.first=spec_signature.last=spec_signature.act1=spec_signature.act2=NULL;                                     \
spec_ordering.first=spec_ordering.last=spec_ordering.act1=spec_ordering.act2=NULL;                                         \
spec_variables.first=spec_variables.last=spec_variables.act1=spec_variables.act2=NULL;                                     \
spec_equations.first=spec_equations.last=spec_equations.act1=spec_equations.act2=NULL;                                     \
spec_conclusions.first=spec_conclusions.last=spec_conclusions.act1=spec_conclusions.act2=NULL;                             \
input           = NULL;                                                                                                    \
inputopen        = false;                                                                                                  \
uppercase        = false;                                                                                                  \
ScanChar         = ' ';                                                                                                    \
scantext[0] = '\0';                                                                                                        \
scanline         = 0;                                                                                                      \
scanpos          = 0;                                                                                                      \
scanlength       = 0;                                                                                                      \
SymbolTab[ 0].symbol = "MODE"      ; SymbolTab[ 0].code= SMODE       ;                                                     \
SymbolTab[ 1].symbol = "COMPLETION"; SymbolTab[ 1].code= SCOMPLETION ;                                                     \
SymbolTab[ 2].symbol = "CONVERGENCE";SymbolTab[ 2].code= SCONVERGENCE;                                                     \
SymbolTab[ 3].symbol = "PROOF"     ; SymbolTab[ 3].code= SPROOF      ;                                                     \
SymbolTab[ 4].symbol = "CONFLUENCE"; SymbolTab[ 4].code= SCONFLUENCE ;                                                     \
SymbolTab[ 5].symbol = "TERMINATION";SymbolTab[ 5].code= STERMINATION;                                                     \
SymbolTab[ 6].symbol = "REDUCTION" ; SymbolTab[ 6].code= SREDUCTION  ;                                                     \
SymbolTab[ 7].symbol = "NAME"      ; SymbolTab[ 7].code= SNAME       ;                                                     \
SymbolTab[ 8].symbol = "SORTS"     ; SymbolTab[ 8].code= SSORTS      ;                                                     \
SymbolTab[ 9].symbol = "SIGNATURE" ; SymbolTab[ 9].code= SSIGNATURE  ;                                                     \
SymbolTab[10].symbol = "ORDERING"  ; SymbolTab[10].code= SORDERING   ;                                                     \
SymbolTab[11].symbol = "KBO"       ; SymbolTab[11].code= SKBO        ;                                                     \
SymbolTab[12].symbol = "LPO"       ; SymbolTab[12].code= SLPO        ;                                                     \
SymbolTab[13].symbol = "VARIABLES" ; SymbolTab[13].code= SVARIABLES  ;                                                     \
SymbolTab[14].symbol = "EQUATIONS" ; SymbolTab[14].code= SEQUATIONS  ;                                                     \
SymbolTab[15].symbol = "CONCLUSION"; SymbolTab[15].code= SCONCLUSION ;                                                     \
SymbolTab[16].symbol = "DATABASE"  ; SymbolTab[16].code= SDATABASE   ;  /*$4712*/                                          \
SymbolTab[17].symbol = "PATTERNS"  ; SymbolTab[17].code= SPATTERNS   ;  /*$4712*/                                          \
SymbolTab[18].symbol = ""          ; SymbolTab[18].code= 0           ;  /*$4712*/                                          \
Signature [0].ident[0]='X';                                                                                                \
Signature [0].ident[1]='\0';                                                                                               \
Signature [0].arity=-2;                                                                                                    \
Signature [0].weight=-1;                                                                                                   \
Signature [0].freelist = NULL;                                                                                             \
Signature [0].signature.first=Signature [0].signature.last=Signature [0].signature.act1=Signature [0].signature.act2=NULL; \
min_weight = -1;term_counter = 0;                                                                                          \
FunctionCounter   = 0;                                                                                                     \
VariableCounter   = 0;                                                                                                     \
freelist       = NULL;                                                                                                     \
freevartree    = NULL;                                                                                                     \
SortCounter       = 0;                                                                                                     \
is_fvar = NULL; /*$4712*/                                                                                                  \
is_fvar_size = 0;

/*
//-----------------------------------------------------------------------------
//     Prototypen
//-----------------------------------------------------------------------------
*/

/*---- error ----------------------------------------------------------------*/

static    void        error            (const char *funcname, const char *errormsg );


/*---- list -----------------------------------------------------------------*/

static    listcell    *new_listcell    ( long data, char *id, void *p1, void *p2 );
static    void        delete_listcell  ( listcell *cell );
static    void        append           ( list *alist, listcell *cell );


/*---- vartree --------------------------------------------------------------*/

static    vartree     *VTnew       ( variable code, variable info );
static    void        VTdelete     ( vartree *cell );
static    void        VTrecdelete  ( vartree *cell );

static    void        VTclear      ( vartree **root );
static    void        VTadd        ( vartree **root, variable code, variable info );
static    variable    VTfind       ( vartree *root,  variable code );
static    short       VTfindcount  ( vartree *root,  variable code );
static    bool        VTless       ( vartree *vt1, vartree *vt2 );
static    bool        VTpartof     ( vartree *vt1, vartree *vt2 );
static    void        VTprint      ( vartree *root );


/*---- sort -----------------------------------------------------------------*/

#define   SortID(s)   Sorts[s].ident

static    sortT        FindSort ( char *ident );
static    sortT        NewSort  ( char *ident );


/*---- term -----------------------------------------------------------------*/

#define   FIdent(fn)      Signature[fn].ident
#define   FArity(fn)      Signature[fn].arity
#define   FWeight(fn)     Signature[fn].weight
#define   FFreeList(fn)   Signature[fn].freelist
#define   NewVariable     --VariableCounter
#define   func_symbp(t)   (t->code > 0)

static    func_symb   FindFunction   ( char *ident );
static    func_symb   NewFunction    ( char *ident );
static    void        SetArguments   ( func_symb fn, short arguments );
static    void        SetWeight      ( func_symb fn, int weight );
static    bool        CheckSignature ( void );
static    void        pretty_print   ( term t, modus mode, 
                                       vartree **vars, variable *counter );
static    void        super_print    ( term t, vartree **vars, variable *counter,
                                       char *fout, char *sout );
static    term        copy_newvar    ( term t, vartree **vars );

static    void        printvar       ( FILE *stream, sortT s, int j );

static    void        var_trace      ( term t );

/*---- scanner --------------------------------------------------------------*/

static    void        NextChar     ( void );
static    char        Preview      ( void );
static    void        Skip         ( void );
static    void        SkipLine     ( void );
static    bool        ScanIdent    ( char *ident );
static    Symbol      ScanSymbol   ( char *ident );

static    void        OpenInput    ( char *filename );
static    void        CloseInput   ( void );
static    bool        IsNumber     ( char *ident );
static    Symbol      GetSymbol    ( char *ident );
static    void        PrintScanText( void  );


/*---- parser ---------------------------------------------------------------*/

static    void       SyntaxError    ( const char *error );
static    void       CheckError     ( bool cond, const char *msg );
static    term       read_term      ( char *ident, Symbol *sym );

static    variable   find_var       ( char *ident );
static    char       *find_var_string( char *ident ); /*$4712*/
static    sortT      find_var_sort  ( char *ident );


static    Symbol     read_sorts        ( void );
static    Symbol     read_signature    ( void );
static    Symbol     read_ordering     ( void );
static    Symbol     read_variables    ( void );
static    Symbol     read_equations    ( void );
static    Symbol     read_conclusions  ( void );

static    void       complete_sig   ( listcell *cell );
static    void       do_sig         ( void );
static    void       complete_vars  ( listcell *cell );
static    void       complete_vars2 ( listcell *cell );


/*
//-----------------------------------------------------------------------------
//     Funktionen
//-----------------------------------------------------------------------------
*/

/*
//=============================================================================
//      error
//=============================================================================
*/

/*
//-----------------------------------------------------------------------------
//  Funktion:       error ( const char *funcname, const char *errormsg )
//
//  Parameter:      funcname    Name der unterbrochenen Funktion
//                  errormsg    Klartext-Fehlermeldung
//-----------------------------------------------------------------------------
*/

static void    error ( const char *funcname, const char *errormsg )
{
    printf( "\n" );
    printf( "******\n" );
    printf( "******  An error occured while parsing:\n" );
    printf( "******\n" );
    printf( "******  Function:     %s\n", funcname );
    printf( "******  Message:      %s\n", errormsg );
    printf( "******\n\n" );
    flush  ();

    exit ( -1 );
}


/*
//=============================================================================
//      LIST
//=============================================================================
*/

/*
//-----------------------------------------------------------------------------
//  Funktion:       new_listcell ( long data, char *id, void *p1, void *p2 )
//
//  Parameter:      ...
//-----------------------------------------------------------------------------
*/

static listcell  *new_listcell ( long data, char *id, void *p1, void *p2 )
{
    register listcell   *ptr, *next;
    register short      i;
             int        size;

    if (!freelist)
    {
        size = LISTALLOC * sizeof (listcell);
        ptr  = freelist = Allocate (size);
        if (!ptr)
            error ( "new_listcell", "Not enough memory for list entry." );

        for ( i = 1; i < LISTALLOC; i++ )
        {
            next = ptr;
            ptr  = ptr->next = ++next;
        }
        ptr->next = NULL;
    }

    ptr           = freelist;
    freelist      = freelist->next;
    ptr->data     = data;
    ptr->weight   = 0;
    ptr->sort     = 0 ;
    ptr->pointer1 = p1;
    ptr->pointer2 = p2;
    ptr->next     = NULL;
    strcpy ( ptr->ident, id );

    return ptr;
}

/*
//-----------------------------------------------------------------------------
//  Funktion:       delete_listcell ( listcell *cell )
//
//  Parameter:      cell    ein Listenelement
//-----------------------------------------------------------------------------
*/

static void  delete_listcell ( listcell *cell )
{
    cell->next = freelist;
    freelist = cell;
}




/*
//-----------------------------------------------------------------------------
//  Funktion:       append ( list *alist, listcell *cell )
//
//  Parameter:      alist   Liste, auf der die Operation durchgefuehrt
//                          werden soll.
//                  cell    anzuhaengendes Element
//-----------------------------------------------------------------------------
*/

static void  append ( list *alist, listcell *cell )
{
    cell->next = NULL;
    if (alist->last)
    {
        alist->last->next = cell;
        alist->last = cell;
    }
    else
    {
        alist->first = alist->last = cell;
    }
}



/*
//=============================================================================
//      VARTREE
//=============================================================================
*/

/*
//-----------------------------------------------------------------------------
//  Funktion:       VTnew      ( variable code, variable info )
//
//  Parameter:      code        Code fuer die Variable
//                  info        Information ueber Variable
//
//  globale Werte:  freevartree Freiliste
//-----------------------------------------------------------------------------
*/

static vartree *VTnew      ( variable code, variable info )
{
    register vartree    *ptr, *next;
    register short      i;
             int        size;
             short      index;

    if (!freevartree)
    {
        size = TREEALLOC * sizeof (vartree);
        ptr = freevartree = Allocate ( size );
        if (!ptr)
            error ( "VTnew", "Not enough memory for variable tree." );

        for ( i = 1; i < TREEALLOC; i++ )
        {
            next = ptr;
#if PS_TEST
               ptr->debug = 0;
#endif 
            ptr = ptr->son = ++next;
        }
        ptr->son = NULL;
    }

    ptr         = freevartree;
    freevartree = freevartree->son;

    for ( i = 0; i < VTSIZE; i++ )
    {
        ptr->code[i]  =    0;
        ptr->info[i]  =    0;
        ptr->count[i] =    0;
        ptr->son      = NULL;
    }
    index = code & VTMASK;
    ptr->code[index]  = code;
    ptr->info[index]  = info;
    ptr->count[index] =    1;

#if PS_TEST
       if (ptr->debug)
          error ( "vartree:new", "MEMORY-ERROR." );
       ptr->debug++;
#endif

    return ptr;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       VTdelete ( vartree *cell )
//
//  Parameter:      cell    ein Knoten
//-----------------------------------------------------------------------------
*/

static void VTdelete ( vartree *cell )
{
#if PS_TEST
       cell->debug--;
       if (cell->debug)
           error ( "VTdelete", "MEMORY-ERROR." );
#endif

    cell->son    = freevartree;
    freevartree  = cell;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       VTrecdelete ( vartree *cell )
//
//  Parameter:      cell    ein Knoten
//-----------------------------------------------------------------------------
*/

static void VTrecdelete ( vartree *cell )
{
    if (cell)
    {
        if (cell->son)
            VTrecdelete (cell->son);

        VTdelete ( cell );
    }
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       VTclear ( vartree **root )
//
//  Parameter:      root    Referenz auf Wurzel der Variablenbaums
//-----------------------------------------------------------------------------
*/

static void  VTclear ( vartree **root )
{
    VTrecdelete ( *root );
    *root = NULL;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       VTadd   ( vartree **root, variable code, variable info )
//
//  Parameter:      root    Referenz auf Wurzel der Variablenbaums
//                  code    Code fuer die Variable
//                  info    Information zur Variable
//-----------------------------------------------------------------------------
*/

static void  VTadd ( vartree **root, variable code, variable info )
{
    register vartree    **ptr;
    register short      index;

    ptr = root;
    index = code & VTMASK;
    while (*ptr)
    {
        if (!((*ptr)->code[index]))
        {
            (*ptr)->code[index]  = code;
            (*ptr)->info[index]  = info;
            (*ptr)->count[index] =    1;
            return;
        }
        ptr = &((*ptr)->son);
    }
    *ptr = VTnew ( code, info );
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       VTfind  ( vartree *root, variable code )
//
//  Parameter:      root    Wurzel der Variablenbaums
//                  code    gesuchter Variablencode
//-----------------------------------------------------------------------------
*/

static variable  VTfind  ( vartree *root, variable code )
{
    register short      index;

    index = code & VTMASK;
    while (root)
    {
        if (code == root->code[index])
            return root->info[index];

        root = root->son;
    }
    return 0;
}



/*
//-----------------------------------------------------------------------------
//  Funktion:       VTfindcount  ( vartree *root, variable code )
//
//  Parameter:      root    Wurzel der Variablenbaums
//                  code    gesuchter Variablencode
//-----------------------------------------------------------------------------
*/

static short  VTfindcount ( vartree *root, variable code )
{
    register short      index;

    index = code & VTMASK;
    while (root)
    {
        if (code == root->code[index])
            return root->count[index];

        root = root->son;
    }
    return 0;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       VTless    ( vartree *vt1, vartree *vt2 )
//
//  Parameter:      vt1, vt2    zu vergleichende Variablenbaeume
//
//  Rueckgabewert:  true    Alle in vt1 vorkommenden Variablen kommen
//                          mindestens genausooft in vt2 vor.
//                  false   sonst
//-----------------------------------------------------------------------------
*/

static bool  VTless ( vartree *vt1, vartree *vt2 )
{
    short       i;
    variable    x, y;

    if (!(vt1))
        return true;

    for ( i = 0 ; i < VTSIZE; i++ )
    {
        if ((x = vt1->code[i]) != 0)
        {
            if ((y = VTfindcount (vt2, x)) <= 0)
                return false;

            if (y < vt1->count[i])
                return false;
        }
        if (!VTless (vt1->son, vt2))
            return false;
    }
    return true;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       VTpartof ( vartree *vt1, vartree *vt2 );
//
//  Parameter:      vt1, vt2    zu vergleichende Variablenbaeume
//
//  Rueckgabewert:  true    Alle in vt1 vorkommenden Variablen kommen
//                          mindestens einmal in vt2 vor.
//                  false   sonst
//-----------------------------------------------------------------------------
*/

static bool  VTpartof ( vartree *vt1, vartree *vt2 )
{
    short       i;
    variable    x;

    if (!(vt1))
        return true;

    for ( i = 0 ; i < VTSIZE; i++ )
    {
        if ((x = vt1->code[i]) != 0)
        {
            if (!VTfind (vt2, x))
                return false;
        }

        if (!VTpartof (vt1->son, vt2))
            return false;
    }
    return true;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       VTprint ( vartree *root )
//
//  Parameter:      root    Wurzel der Variablenbaums
//-----------------------------------------------------------------------------
*/

static void  VTprint ( vartree *root )
{
    short   i;

    if (root)
    for ( i = 0 ; i < VTSIZE ; i++ )
        if (root->code[i])
        {
            printf( "%4ld %4ld %4d\n",
                     (long)(-root->code[i]),
                     (long)(root->info[i]),
                     (int)(root->count[i]) );
        }
        VTprint ( root->son );
}


/*
//=============================================================================
//      SORT
//=============================================================================
*/

/*
//-----------------------------------------------------------------------------
//  Funktion:       FindSort ( char *ident )
//
//  Parameter:      ident       Identifier der Sorte
//-----------------------------------------------------------------------------
*/

static sortT  FindSort ( char *ident )
{
    short   i = 1;

    while (i <= SortCounter)
    {
        if (!strcmp (ident, Sorts[i].ident))
            return i;
        i++;
    }
    return 0;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       NewSort ( char *ident )
//
//  Parameter:      ident       Identifier der Sorte
//-----------------------------------------------------------------------------
*/

static sortT  NewSort ( char *ident )
{
    if (FindSort ( ident ))
        error ( "NewSort", "NewSort\n" "Sort already defined." );

    if (++SortCounter >= MAXSORT)
        error ( "NewSort", "NewSort\n" "Too many sorts." );

    strcpy ( Sorts[SortCounter].ident, ident );

    return SortCounter;
}



/*
//=============================================================================
//      TERM
//=============================================================================
*/

/*
//-----------------------------------------------------------------------------
//  Funktion:       FindFunction ( char *ident )
//
//  Parameter:      ident       Identifier der Funktion
//-----------------------------------------------------------------------------
*/

static func_symb  FindFunction ( char *ident )
{
    short   i = 1;

    while (i <= FunctionCounter)
    {
        if (!strcmp (ident, Signature[i].ident))
            return i;
        i++;
    }
    return 0;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       NewFunction ( char *ident )
//
//  Parameter:      ident       Identifier der Funktion
//-----------------------------------------------------------------------------
*/

static func_symb  NewFunction ( char *ident )
{
    if (FindFunction ( ident ))
        error ( "NewFunction", "Function already defined." );

    if (++FunctionCounter >= MAXFUNCTION)
        error ( "NewFunction", "Too many function symbols." );

    strcpy ( Signature[FunctionCounter].ident, ident );
    Signature[FunctionCounter].arity        = -1;
    Signature[FunctionCounter].weight       = 0;
    Signature[FunctionCounter].freelist     = NULL;

    return FunctionCounter;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       SetArguments ( func_symb fn, short arity )
//
//  Parameter:      fn          Codenummer der Funktion
//                  arity       Anzahl der Argumente
//-----------------------------------------------------------------------------
*/

static void  SetArguments ( func_symb fn, short arity )
{
    if ((fn <= 0) || (fn > FunctionCounter))
        error ( "SetArguments", "Funktion number out of range." );

    if (Signature[fn].arity == -1)
        Signature[fn].arity = arity;
    else
    if (Signature[fn].arity != arity)
    {
        printf( "******  Function: `%s'\n", Signature[fn].ident );
        error ( "SetArguments",
		"Function is used with different arity than defined." );
    }
}


static void  SetWeight ( func_symb fn, int weight )
{
    if ((fn <= 0) || (fn > FunctionCounter))
        error ( "SetWeight", "Function code out of range." );

    if (Signature[fn].weight == 0)
        Signature[fn].weight = weight;
    else
    if (Signature[fn].weight != weight)
    {
        printf( "******  Function: `%s'\n", Signature[fn].ident );
        error ( "SetWeight",
                "Function defined with different weights." );
    }

    if ( (weight < min_weight) || (min_weight == -1) )
        min_weight = weight;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       CheckSignature
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static bool  CheckSignature ( void )
{
    short   i;
    bool    result  = true;

    for (i = 1; i <= FunctionCounter; i++ )
        if (Signature[i].arity < 0)
        {
          printf( "******  Warning: `%s' is not used in equations.\n",
                   Signature[i].ident );
          result    = false;
        }

    return result;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       map_func ( func_symb code )
//
//  Parameter:      code    Codenummer der Funktion
//-----------------------------------------------------------------------------
*/

char  *map_func ( func_symb code )
{
    return Signature[code].ident;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       new_termcell  ( func_symb code )
//
//  Parameter:      code    Codenummer der Funktion/Variable
//                          > 0     Funktion
//                          < 0     Variable
//
//  Anmerkung:      Als globales Symbol vereinbart (nicht static)
//                  aber im Header nicht sichtbar !!!
//                  Speziell fuer spaetere Erweiterung !
//-----------------------------------------------------------------------------
*/

static term    new_termcell ( func_symb code )
{
    term       ptr, next;
    short      i;
    func_symb  acode;
    long       acount;
    long       asize;

    term_counter++;

    acode = max(0, code);

    if (!FFreeList(acode))
    {
        if (code <= 0)
        {
            acount = TERMALLOC;
            asize  =   sizeof (struct termcell)
                     + sizeof (term);
        }
        else
        {
            if (FArity(code) < 0)
                error ( "newtermcell", "Less than zero arguments." );

            acount = TERMALLOC;
            asize  =   sizeof (struct termcell)
                     + max(1, FArity(code)) * sizeof (term);
        }
        asize  = ((asize+ALIGN-1)/ALIGN) * ALIGN;
        ptr = Signature[acode].freelist = Allocate ( acount * asize );
        if (!ptr)
            error ( "newtermcell", "Out of memory." );

        for ( i = 1; i < acount; i++ )
        {
            next = (term)(ptr2ul(ptr) + asize);
#if PS_TEST
               ptr->debug = 0;
#endif
            ptr  = ptr->argument[0] = next;
        }
        ptr->argument[0] = NULL;
    }

    ptr              = FFreeList(acode);
    FFreeList(acode) = ptr->argument[0];
    ptr->code        = code;
    ptr->arity       = FArity(acode);

#if PS_TEST
       if (ptr->debug)
           error ( "newtermcell", "MEMORY-ERROR." );
       ptr->debug++;
#endif

    return ptr;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       delete_termcell  ( term t )
//
//  Parameter:      t       Pointer auf TermZelle
//-----------------------------------------------------------------------------
*/

static void    delete_termcell ( term t )
{
    short   acode;

    term_counter--;

#if PS_TEST
       t->debug--;
       if (t->debug)
           error ( "delete_termcell", "MEMORY-ERROR." );
#endif
    acode          = max(t->code, 0);
    t->argument[0] = FFreeList(acode);
    FFreeList(acode) = t;
}

/*
//-----------------------------------------------------------------------------
//  Funktion:       term_delete  ( term t )
//
//  Parameter:      t       Pointer auf Term
//-----------------------------------------------------------------------------
*/

static void    term_delete ( term t )
{
    short   i;

    if (t->arity > 0)
        for ( i = 0; i < t->arity; term_delete (t->argument[i++]) );

    delete_termcell ( t );
}



/*
//-----------------------------------------------------------------------------
//  Funktion:       occur ( variable var, term *t )
//
//  Parameter:      var         Kennummer der Variablen
//                  t           Pointer auf zu untersuchenden Term
//
//  Anmerkung:      im Moment nicht benoetigt !
//-----------------------------------------------------------------------------
*/

static bool  occur  ( variable var, term t )
{
    short   i;

    if (t->code == var)
        return true;

    for (i = 0; i < t->arity; i++ )
        if (occur (var, t->argument[i]))
            return true;

    return false;
}

/*
//-----------------------------------------------------------------------------
//  Funktion:       term_eq ( term t1, term t2 )
//
//  Parameter:      t1, t2  zu vergleichende Terme
//-----------------------------------------------------------------------------
*/

bool    term_eq ( term t1, term t2 )
{
    short   i;

    if (t1->code != t2->code)
        return false;

    if (func_symbp (t1))
    {
        for (i = 0; i < t1->arity; i++ )
            if (!term_eq (t1->argument[i], t2->argument[i]))
                return false;
    }

    return true;
}




/*
//-----------------------------------------------------------------------------
//  Funktion:       pretty_print ( term t, modus mode, 
//                                 vartree **vars, variable *counter )
//
//  Parameter:      t       Pointer auf Term
//                  mode    verschiedene Schalter
//                  vars    Pointer auf den aktuellen Variablenbaum
//                  counter Zaehler fuer Variablen
//-----------------------------------------------------------------------------
*/

static void  pretty_print ( term t, modus mode, vartree **vars, variable *counter )
{
    short       i = 0;
    variable    v;

    if (variablep(t))
    {
        if ((v = VTfind (*vars, t->code)) == 0)
            VTadd ( vars, t->code, v = ++*counter );

        if (v > varstrlen)
            printf( "x%ld", v-varstrlen );
        else
            printf( "%c", varstring[v] );

        if (mode == varsort)
            printf( ":%s",  SortID(t->sort) );
    }
    else
    {
        printf( Signature[t->code].ident );
        if (mode == extended)
            printf( ":%s",  SortID(t->sort) );

        if (t->arity > 0)
        {
            printf( " (" );
            while ( i+1 < t->arity )
            {
                pretty_print ( t->argument[i++], mode, vars, counter );
                printf( "," );
            }
            pretty_print ( t->argument[i], mode, vars, counter );
            printf( ")" );
        }
    }
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       super_print ( term *t, vartree **vars, variable *counter,
//                                char *fout, char *sout )
//
//  Parameter:      t       Pointer auf Term
//                  vars    Pointer auf den aktuellen Variablenbaum
//                  counter Zaehler fuer Variablen
//                  fout    Puffer fuer Termausgabe
//                  sout    Puffer fuer Sortenausgabe
//-----------------------------------------------------------------------------
*/

static void  super_print ( term t, vartree **vars, variable *counter, 
                           char *fout, char *sout )
{
    short       i = 0;
    unsigned int lenf, lens;
    variable    v;
    static char *space = "                              ";
    char        var[20] = { 0 };

    if (variablep(t))
    {
        if ((v = VTfind (*vars, t->code)) == 0)
            VTadd ( vars, t->code, v = ++*counter );

        if (v > varstrlen)
            sprintf ( var, "x%ld", v-varstrlen );
        else
            sprintf ( var, "%c",  varstring[v] );

        strcat ( fout, var );
        strcat ( sout, SortID (t->sort) );
        lenf = strlen ( fout );
        lens = strlen ( sout );
        if (lenf < lens)
            strncat ( fout, space, lens-lenf );
        else if (lens < lenf)
            strncat ( sout, space, lenf-lens );
    }
    else
    {
        strcat ( fout, Signature[t->code].ident );
        strcat ( sout, SortID (t->sort) );
        lenf = strlen ( fout );
        lens = strlen ( sout );
        if (lenf < lens)
            strncat ( fout, space, lens-lenf );
        else if (lens < lenf)
            strncat ( sout, space, lenf-lens );


        if (t->arity > 0)
        {
            strcat ( fout, " (" );
            strcat ( sout, "  " );
            while ( i+1 < t->arity )
            {
                super_print ( t->argument[i++], vars, counter, fout, sout );
                strcat ( fout, "," );
                strcat ( sout, " " );
            }
            super_print ( t->argument[i], vars, counter, fout, sout );
            strcat ( fout, ")" );
            strcat ( sout, " " );
        }
    }
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       fprettyprint ( FILE *stream,
//                                 term *t, vartree **vars, variable *counter )
//
//  Parameter:      stream  Ausgabestream
//                  t       Pointer auf Term
//                  vars    Pointer auf den aktuellen Variablenbaum
//                  counter Zaehler fuer Variablen
//
//  Beschreibung:   Verbesserte Ausgabe eines Terms.
//                  Hilsfunktion fuer prettyterm
//                  Wird auch fuer Ausgabe von Termpaaren exportiert
//                  Ausgabe aus Stream
//-----------------------------------------------------------------------------
*/

static void    fprettyprint ( FILE *stream, term t, vartree **vars, variable *counter )
{
    short       i = 0;
    variable    v;

    if (variablep(t))
    {
        if ((v = VTfind (*vars, t->code)) == 0)
            VTadd ( vars, t->code, v = ++*counter );

        if (v > varstrlen)
            fprintf ( stream, "x%ld", v-varstrlen );
        else
            fprintf ( stream, "%c", varstring[v] );
    }
    else
    {
        fprintf ( stream, map_func(t->code) );
        if (t->arity > 0)
        {
            fprintf ( stream, " (" );
            while ( i+1 < t->arity )
            {
                fprettyprint ( stream, t->argument[i++], vars, counter );
                fprintf ( stream, "," );
            }
            fprettyprint ( stream, t->argument[i], vars, counter );
            fprintf ( stream, ")" );
        }
    }
}



/*
//-----------------------------------------------------------------------------
//  Funktion:       newvars ( term t )
//
//  Parameter:      t       Pointer auf Term
//                  vars    Pointer auf den aktuellen Variablenbaum
//-----------------------------------------------------------------------------
*/

static void  newvars ( term t, vartree **vars )
{
    short       i = 0;
    variable    v;

    if (variablep(t))
    {
        if ((v = VTfind (*vars, t->code)) == 0)
            VTadd ( vars, t->code, v = NewVariable );

        t->code = v;
    }
    else
    {
        for ( i = 0; i < t->arity; newvars (t->argument[i++], vars) );
    }
}

/*
//-----------------------------------------------------------------------------
//  Funktion:       new_variables  ( term t1, term t2 )
//
//  Parameter:      t1, t2    Pointer auf Term
//-----------------------------------------------------------------------------
*/

static void  new_variables ( term t1, term t2 )
{
    vartree     *vars    = NULL;

    newvars ( t1, &vars );
    if (t2)
        newvars ( t2, &vars );
    VTclear ( &vars );
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       create_var  ( sortT s )
//
//  Parameter:      s       Sorte
//-----------------------------------------------------------------------------
*/

term  create_var ( sortT s )
{
    term    t = new_termcell ( NewVariable );
            t->sort = s;

    return  t;
}



/*
//-----------------------------------------------------------------------------
//  Funktion:       copy_newvar ( term t, vartree **vars )
//
//  Parameter:      t       Pointer auf Term
//                  vars    Pointer auf den aktuellen Variablenbaum
//-----------------------------------------------------------------------------
*/

static term  copy_newvar  ( term t, vartree **vars )
{
    term        ptr;
    short       i;
    variable    v;

    ptr = new_termcell (t->code);
    ptr->sort = t->sort;

    if (variablep(ptr))
    {
        if ((v = VTfind (*vars, ptr->code)) == 0)
            VTadd ( vars, ptr->code, v = NewVariable );

        ptr->code = v;
    }
    else
        for ( i = 0; i < t->arity; i++ )
            ptr->argument[i] = copy_newvar (t->argument[i], vars);

    return ptr;
}


static void  printvar ( FILE *stream, sortT s, int j )
{
    if (Sorts[s].var)
    {
        if (j)
            fprintf ( stream, "%s_%d",   Sorts[s].var, j );
        else
            fprintf ( stream, "%s",   Sorts[s].var );
    }
    else
        fprintf ( stream, "V_%s_%d", Sorts[s].ident, j );
}


static void print_v ( FILE *stream, term t )
{
    short       i;

    if (variablep (t))
    {
        printvar ( stream, t->sort, VTfind (v_tree, t->code)-1 );
    }
    else
    {
        fprintf ( stream, "%s", Signature[t->code].ident );
        if (t->arity)
        {
             fprintf ( stream, " (" );
             for ( i = 0; i < t->arity;  i++)
             {
                  print_v ( stream, t->argument[i] );
                  if ((i+1) < t->arity)
                     fprintf ( stream, "," );
             }
             fprintf ( stream, ")" );
        }
    }
}

static void  var_trace ( term t )
{
    short       i;
    int         v;

    if (variablep (t))
    {
        if (!(VTfind (v_tree, t->code)))
        {
            v = (Sorts[t->sort].counter+=1);
            Sorts[t->sort].max = max ( Sorts[t->sort].max, v );
            VTadd ( &v_tree, t->code, v );
        }
    }
    else
    {
        for ( i = 0; i < t->arity; 
             var_trace (t->argument[i++]) );
    }
}






/*
//=============================================================================
//      SCANNER
//=============================================================================
*/

/*
//-----------------------------------------------------------------------------
//  Funktion:       NextChar
//
//  Parameter:      -keine-
//
//  Beschreibung:   Holt wenn moeglich das naechste Zeichen der Eingabe
//                  CR = Zeilenende
//-----------------------------------------------------------------------------
*/

static void  NextChar ()
{
    register int    i = 0;

    EOI = EOI || (feof (input) && (ScanChar == CR));

    if (!EOI)
    {
        if (ScanChar == CR)
        {
            while ( scantext[i] )
                scantext[i++] = 0;
            fgets ( scantext, (int)(sizeof scantext), input );
            scanline++;
            scanlength = strlen ( scantext );
            scanpos    = 0;
        }

        ScanChar   = (scanlength > scanpos) ? scantext[scanpos++]
                                            : CR;

#ifdef DEBUG
            putchar ( ScanChar );
#endif
        EOI = (feof (input) && (ScanChar == CR));
        if (EOI)
            ScanChar = CR;

        ScanChar = (uppercase) ? toupper (ScanChar)
                               : ScanChar;
    }
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       Preview
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static char  Preview  ( void )
{
    if (EOI || (ScanChar == CR))
        return CR;
    return scantext[scanpos];
}



/*
//-----------------------------------------------------------------------------
//  Funktion:       Skip
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static void  Skip ()
{
    while (!EOI && ((ScanChar == ' ') || (ScanChar == TAB) || ScanChar == '\r'))
        NextChar();
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       SkipLine
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static void  SkipLine ()
{
    while (ScanChar != CR)
        NextChar();
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       ScanIdent ( char *ident )
//
//  Parameter:      ident    - Zeiger auf Speicher fuer Ident
//-----------------------------------------------------------------------------
*/

static bool  ScanIdent ( char *ident )
{
    static const char Ergaenzungen[] = "_+~*$|\\^/";

    Skip ();

    if (!isalnum ((int)ScanChar) && !strchr(Ergaenzungen,ScanChar)
                            && ((ScanChar != '-') || Preview() == '>')
/* $4711 Hinzufuegungen Thomas et al. bis 27.11.98 */
                                                ) 
        return false;

    while (isalnum ((int)ScanChar) || strchr(Ergaenzungen,ScanChar)
                              || ((ScanChar == '-') && Preview() != '>')
/* $4711 Hinzufuegungen Thomas et al. bis 27.11.98 */
                                                  )
    {
        *ident++ = ScanChar;
        NextChar();
    }

    *ident = 0;
    return true;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       ScanSymbol ( char *ident )
//
//  Parameter:      Zeiger auf Speicher fuer Identifier
//
//  Rueckgabewert:  Klassifizierung der gelesenen Zeichen
//
//  Beschreibung:   Einlesen und Klassifizieren eines Symbols
//-----------------------------------------------------------------------------
*/

static Symbol   ScanSymbol ( char *ident )
{
    if (ScanIdent ( ident ))
        return SIDENT;

    switch (ScanChar)
    {
    case '-' :  if (Preview() == '>')  {
                    NextChar ();
                    NextChar ();
                    return SARROW;
                }
                return SUNKNOWN;
    case '#' :  SkipLine ();
                return SCRSYM;
    case '%' :  SkipLine ();
                return SCRSYM;
    case '=' :  NextChar ();
                return SEQUAL;
    case '>' :  NextChar ();
                return SGREATER;
    case '<' :  NextChar ();
                return SLESS;
    case CR  :  NextChar ();
                return SCRSYM;
    case ':' :  NextChar ();
                return SCOLLON;
    case ',' :  NextChar ();
                return SCOMMA;
    case '(' :  NextChar ();
                return SBRACKET_L;
    case ')' :  NextChar ();
                return SBRACKET_R;
    default  :  printf("Character is `%c'\n", ScanChar);
                error ( "ScanSymbol", "Unknown symbol." );
       
                return SUNKNOWN;
    }
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       OpenInput ( char *filename )
//
//  Parameter:      Dateiname der Spezifikation
//-----------------------------------------------------------------------------
*/

static void  OpenInput ( char *filename )
{
    if (inputopen)
        error ( "OpenInput", "Input file already open" );

    input = fopen ( filename, "r" );
    if (!input)
        error ( "OpenInput", "Cannot open input file." );

    inputopen = true;
    scanline = 0;

    EOI = feof ( input );
    if (!EOI)
    {
        fgets ( scantext, (int)(sizeof scantext), input );
        scanline++;
        scanpos    = 0;
        scanlength = strlen ( scantext );
        ScanChar   = (scanlength) ? ' '
                                  : CR;
    }
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       CloseInput
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static void  CloseInput ()
{
    if (inputopen)
        fclose ( input );

    input = NULL;
    inputopen = false;
    EOI = true;
}




/*
//-----------------------------------------------------------------------------
//  Funktion:       IsNumber ( char *ident )
//
//  Parameter:      ident    - Zeiger auf Speicher fuer Ident
//
//  Rueckgabewert:  true     falls ident eine Ziffernfolge darstellt
//                  false    sonst
//
//  Beschreibung:   Prueft, ob ident eine Ziffernfolge ist.
//-----------------------------------------------------------------------------
*/

static bool  IsNumber ( char *ident )
{
    short int    i = 0;

    if ((ident[0] == '+') || (ident[0] == '-'))
        i++;

    while (ident[i] && isdigit ((int)ident[i]))
        i++;

    return ident[i] ? false : true;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       GetSymbol ( char *ident )
//
//  Parameter:      Zeiger auf Speicher fuer Identifier
//
//  Rueckgabewert:  Klassifizierung der gelesenen Zeichen
//-----------------------------------------------------------------------------
*/

static Symbol  GetSymbol ( char *ident )
{
    register short  i;
    register Symbol sym;
    char   bident[IDENTLENGTH];

    sym = ScanSymbol ( ident );

    if (sym != SIDENT)
        return sym;

    i = 0;
    while (ident[i])
    {
        if (islower ((int)(bident[i] = ident[i])))
            bident[i] = toupper (bident[i]);
        i++;
    }
    bident[i] = 0;

    i = 0;
    while (SymbolTab[i].code)
    {
        if (! strcmp(bident, SymbolTab[i].symbol ))
            return SymbolTab[i].code;
        i++;
    }

    return SIDENT;
}



/*
//-----------------------------------------------------------------------------
//  Funktion:       PrintScanText
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static void  PrintScanText ( void  )
{
    printf( "****  (Line %d) %s\n", (int)scanline, scantext );
}


/*
//=============================================================================
//      PARSER
//=============================================================================
*/


/*
//-----------------------------------------------------------------------------
//  Funktion:       SyntaxError
//
//  Parameter:      error       Fehlermeldung
//-----------------------------------------------------------------------------
*/

static void  SyntaxError (const char *error )
{
    PrintScanText ();
    printf( "****  Error in file.\n" );
    printf( "****  %s\n", error );
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       CheckError
//
//  Parameter:      cond    Bedingung fuer Fehlerfall
//                  msg     Fehlermeldung
//-----------------------------------------------------------------------------
*/

static void  CheckError ( bool cond, const char *msg )
{
    if (cond)
    {
        SyntaxError ( msg );
        CloseInput ();
        exit ( 1 );
    }
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       read_term
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static term  read_term ( char *ident, Symbol *sym )
{
    func_symb    func;
    variable    var;
    term        t;
    short       i = 1;

    if ((func = FindFunction (ident)) == 0)
    {
        var  = find_var  ( ident );
        CheckError (!var, "Unknown variable.");
        t       = new_termcell ( var );
        t->sort = find_var_sort ( ident );
        t->RealVariableName = find_var_string(ident); /*$4712*/

        *sym = GetSymbol ( ident );
        return t;
    }

    t = new_termcell ( func );

    if (FArity (func))
    {
        CheckError (GetSymbol (ident) != SBRACKET_L, "`(' expected." );
        for (i =0; i < t->arity; i++)
        {
            *sym = GetSymbol ( ident );
            t->argument[i] = read_term ( ident, sym );
            if ((i+1) < t->arity)
            {
                CheckError ( *sym != SCOMMA, "`,' expected." );
            }
        }
        CheckError (*sym != SBRACKET_R, "`)' expected." );
    }

    *sym = GetSymbol (ident);
    return t;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       find_var
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static variable  find_var ( char *ident )
{
    listcell    *ptr1 = spec_variables.first;
    while (ptr1)
    {
        listcell    *ptr2 = ptr1;
        while (ptr2)
        {
            if (!strcmp (ptr2->ident, ident))
                return ptr2->data;
            ptr2 = ptr2->pointer1;
        }
        ptr1 = ptr1->next;
    }
    return 0;
}

static char *find_var_string(char *ident) /*$4712*/
{
  listcell *ptr1 = spec_variables.first;
  while (ptr1) {
    listcell *ptr2 = ptr1;
    while (ptr2) {
      if (!strcmp (ptr2->ident, ident))
        return ptr2->ident;
      ptr2 = ptr2->pointer1;
    }
    ptr1 = ptr1->next;
  }
  return NULL;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       find_var_sort
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static sortT  find_var_sort ( char *ident )
{
    listcell    *ptr1 = spec_variables.first;
    while (ptr1)
    {
        listcell    *ptr2 = ptr1;
        while (ptr2)
        {
            if (!strcmp (ptr2->ident, ident))
                return ptr2->sort;
            ptr2 = ptr2->pointer1;
        }
        ptr1 = ptr1->next;
    }
    return 0;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       read_sorts
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static Symbol  read_sorts ( void )
{
    Symbol      sym;
    char        ident[IDENTLENGTH];
    sortT       sort;
    listcell    *ptr;

    while ((sym = GetSymbol (ident)) == SCRSYM)
        CheckError ( EOI, "Unexpected end of file." );
    
    CheckError ( sym == SSIGNATURE, "Add least one sort has to be used.");
    CheckError ( sym != SIDENT, "Sort identifier expected.");

    while (sym != SSIGNATURE)
    {
        ptr = new_listcell ( 0, "", NULL, NULL );
        /* Im Gegensatz zu anderen Listen werden die Listen von Sorten nicht */
        /* ueber den Pointer next miteinander verkettet, sondern ueber den   */
        /* Pointer "pointer1". Dies ergab sich bei der Umstellung der Funk-  */
        /* tionen von Funktionssymbolen mit meheren verschiedenen Funktions- */
        /* deklarationen auf genau eine. Wenn ich es nicht an dieser Stelle  */
        /* geaendert haette, haette die gesamte Verwaltung der Signatur ge-  */
        /* aendert werden muessen, was mir heuer ( April `93) zuviel Arbeit  */
        /* ist. Auswirkungen hat die auch auf die Funktion sorts_rest.       */
        /* Martin Kronenburg                                                 */

        ptr->next = NULL;
        if ( spec_sorts.last )
        { /* nicht der erste Eintrag */
          spec_sorts.last->pointer1 = ptr;
          spec_sorts.last = ptr;
        }
        else  /* erster Eintrag */
          spec_sorts.first = spec_sorts.last = ptr;
          
        if (!(sort = FindSort ( ident )))
            sort = NewSort ( ident );
        else
          CheckError ( true, "Duplicate sort entry.");

        ptr->data = sort;

        while ((sym = GetSymbol (ident)) == SCRSYM)
            CheckError ( EOI, "Unexpected end of file." );

        CheckError ( (sym != SIDENT) && (sym != SSIGNATURE),
		     "Sort identifier or signature expected");
    }

    return sym;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       set_fvar
//
//  Parameter:      Wochentag und Aussentemperatur
//-----------------------------------------------------------------------------
*/                                                                    /*$4712*/
static void set_fvar(func_symb func, bool fvar)
{
  if (!is_fvar)
    is_fvar = (bool *) malloc(((is_fvar_size = max(15,func))+1)*sizeof(bool));
  else if (func>is_fvar_size)
    is_fvar = (bool *) realloc(is_fvar, ((is_fvar_size = max(16+is_fvar_size,func))+1)*sizeof(bool));
  is_fvar[func] = fvar;
}

bool sig_is_fvar(func_symb func)
{
  return is_fvar[func];
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       read_signature    
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/                                                                    /*$4712*/

static Symbol  read_signature ( void )
{
    Symbol      sym;
    char        ident[IDENTLENGTH];
    func_symb    func;
    sortT       sort;
    listcell    *ptr;
    bool        in_patterns = false;

    while ((sym = GetSymbol (ident)) == SCRSYM)
        CheckError ( EOI, "Unexpected end of file." );

    while (!EOI && (
      (spec_mode!=database && sym!= SORDERING)  ||  
      (spec_mode==database && ( in_patterns==false ||
                               (in_patterns==true  && sym!=SVARIABLES )))))
    {
        if (spec_mode==database && in_patterns==false && sym==SPATTERNS)
            in_patterns=true;
        else {
            ptr = new_listcell ( 0, "", NULL, NULL );
            append ( &spec_signature, ptr );
            while (sym == SIDENT)
            {
                if (!(func = FindFunction ( ident ))) {
                    func = NewFunction ( ident );
                    set_fvar(func, in_patterns);
                }
                else
                  CheckError ( true, "Duplicate function definition.");
    
                ptr->data = func;
                while ((sym = GetSymbol (ident)) == SCOMMA)
                {
                    CheckError ( ((sym = GetSymbol (ident)) != SIDENT),
                                 "Identifier (function) expected." );    
    
                    if (!(func = FindFunction ( ident ))) {
                        func = NewFunction ( ident );
                        set_fvar(func, in_patterns);
                    }
                    else
                      CheckError ( true, "Duplicate function definition.");
    
                    append ( &spec_signature, ptr = new_listcell (func, "", NULL, NULL));
                }
    
                CheckError ( (sym != SCOLLON), "`,' or `:' expected." );
    
            }
    
            sym = GetSymbol (ident);
            while (sym == SIDENT)
            {
                CheckError (!(sort = FindSort ( ident )), "Unknown sort." );
                ptr = ptr->pointer1 = new_listcell ( sort, "", NULL, NULL );
    
                if ((sym = GetSymbol (ident)) == SCOMMA)
                    sym = GetSymbol (ident);
            }
    
            CheckError ( (sym != SARROW), "Identifier or `->' expected." );
            sym = GetSymbol (ident); 
            CheckError ( (sym != SIDENT), "Identifier expected." );
            CheckError (!(sort = FindSort ( ident )), "Unknown sort." );
            ptr = ptr->pointer1 = new_listcell ( sort, "", NULL, NULL );

            sym = GetSymbol (ident); 
            CheckError ( (sym != SCRSYM), "Identifier or new line expected." );
        }

        while ((sym = GetSymbol (ident)) == SCRSYM)
            CheckError ( EOI, "Unexpected end of file." );
    }

    return sym;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       read_ordering
//
//  Parameter:      -keine-
//  Bemerkungen ( MK ) : Die einzelnen Sortenketten werden in der Liste spec_ordering
//                       ueber den Pointer next verkettet.
//                       Die Komponenten der einzelnen Ordnungsketten werden 
//                       ueber den Pointer pointer1 verkettet.
//-----------------------------------------------------------------------------
*/

static Symbol  read_ordering ( void )
{
    Symbol      sym;
    char        ident[IDENTLENGTH];
    listcell    *ptr = NULL;
    func_symb   func;

    while ((sym = GetSymbol (ident)) == SCRSYM)
        CheckError ( EOI, "Unexpected end of file." );

    switch (sym)
    {
      case SKBO:  spec_ordering_mode = ordering_kbo;
                  break;
      case SLPO:  spec_ordering_mode = ordering_lpo;
                  break;
      default:    CheckError ( true, "`KBO' or `LPO' expected." );
                  break;
    }
    while ((sym = GetSymbol (ident)) == SCRSYM)
        CheckError ( EOI, "Unexpected end of file." );

    if (sym == SCOLLON)
        while ((sym = GetSymbol (ident)) == SCRSYM)
            CheckError ( EOI, "Unexpected end of file." );

    if (sym == SIDENT)
    {
        CheckError (!(func = FindFunction ( ident )), "Undeclared function." );
        sym = GetSymbol (ident);
        CheckError ( ( spec_ordering_mode == ordering_kbo ) && ( sym != SEQUAL ), "`=' expected." );

        while (!EOI && (sym == SEQUAL))
        {
            CheckError (!(sym = GetSymbol (ident)) || !(IsNumber(ident)),
                        "Number expected." );
            SetWeight ( func, atoi(ident) );
            fflush (stdout);
            sym = GetSymbol (ident);
            CheckError( (sym != SCOMMA) && (sym != SCRSYM), "`,' or new line expected.");

            if ( sym == SCOMMA )  /* weitere Gewichte bei der Kbo */
            {
              while ((sym = GetSymbol (ident)) == SCRSYM)
                CheckError ( EOI, "Unexpected end of file." );
              
              CheckError (!(func = FindFunction ( ident )),"Undeclared function." );
              CheckError ( (sym = GetSymbol (ident)) != SEQUAL, "`=' expected.");
            }
            else  /* es folgen keine Gewichte mehr */
            {
              while ((sym = GetSymbol (ident)) == SCRSYM)
                CheckError ( EOI, "Unexpected end of file." );

              if (sym != SIDENT)
                  return sym;  /* keine Praezedens auf den Funktionssymbolen */

              CheckError (!(func = FindFunction ( ident )),
                           "Undeclared function." );
              sym = GetSymbol (ident);
              CheckError ( sym != SGREATER, "`>' expected.");
            }
        }

        /* Es folgen die Vorordnungsketten */
        CheckError ( sym != SGREATER, "`>' expected.");
        while ((!EOI && (sym == SGREATER)) || (sym == SCRSYM) )
        {
            if (sym == SGREATER)
            {
                if (!ptr) /* Wenn ptr == NULL, so beginnt eine neue Ordnungskette */
                {         /* Dies ist am Anfang und nach einem Zeilenumbruch der Fall */
                    ptr = new_listcell ( 0, "", NULL, NULL );
                    append ( &spec_ordering, ptr );
                }
                ptr->data = func;
                ptr->pointer1 = new_listcell ( 0, "", NULL, NULL );
                ptr = ptr->pointer1;
                sym = GetSymbol (ident);
            }
            else if (sym == SCRSYM)
            {
                ptr->data = func; /* Wichtig -> letztes Funktionssymbol wird */
                                  /* in die Kette eingetragen */

                ptr = NULL; /* Neue Ordnungskette beginnt */
                while ((sym = GetSymbol (ident)) == SCRSYM)
                    CheckError ( EOI, "Unexpected end of file." );
            }
    
            if (sym == SIDENT)
            {
                CheckError (!(func = FindFunction ( ident )), "Undeclared function." );
                sym = GetSymbol (ident);
            }
        }
    }
    return sym;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       read_variables
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static Symbol  read_variables ( void )
{
    Symbol      sym;
    char        ident[IDENTLENGTH];
    sortT       sort;

    listcell    *ptr;

    while ((sym = GetSymbol (ident)) == SCRSYM)
        CheckError ( EOI, "Unexpected end of file." );

    while (!EOI && (sym != SEQUATIONS))
    {
        ptr = new_listcell ( 0, "", NULL, NULL );
        append ( &spec_variables, ptr );
        while (sym == SIDENT)
        {
            ptr->data = NewVariable;
            ptr->sort = 0;
            strcpy ( ptr->ident, ident );

            if ((sym = GetSymbol (ident)) == SCOMMA)
            {
                ptr = ptr->pointer1 = new_listcell ( 0, "", NULL, NULL );
                sym = GetSymbol (ident);
            }
        }

        CheckError ( (sym != SCOLLON), "Identifier or `:' expected." );
        CheckError ( ((sym = GetSymbol (ident)) != SIDENT), "Identifier expected." );
        CheckError (!(sort = FindSort ( ident )), "Undeclared sort." );
        ptr->pointer1 = new_listcell ( sort, "", NULL, NULL );

        while ((sym = GetSymbol (ident)) == SCRSYM)
            CheckError ( EOI, "Unexpected end of file." );
    }

    return sym;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       read_equations
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static Symbol  read_equations ( void )
{
    Symbol      sym;
    char        ident[IDENTLENGTH];
    term        t1, t2;

    while ((sym = GetSymbol (ident)) == SCRSYM)
        CheckError ( EOI, "Unexpected end of file." );

    while (!EOI && (sym == SIDENT))
    {
        t1 = read_term ( ident, &sym );
        CheckError ( sym != SEQUAL, "`=' expected." );
        sym = GetSymbol (ident);
        t2 = read_term ( ident, &sym );
        new_variables ( t1, t2 );
        append ( &spec_equations, new_listcell (0, "", t1, t2) );

        CheckError ( sym != SCRSYM, "New line expected." );

        while (!EOI && ((sym = GetSymbol (ident)) == SCRSYM));
    }

    return sym;
}

/*****************************************************************************/
/*                                                                           */
/*  Funktion     :  read_conclusions                                         */
/*                                                                           */
/*  Parameter    :  keine                                                    */
/*                                                                           */
/*  Returnwert   :                                                           */
/*                                                                           */
/*  Beschreibung :                                                           */
/*                                                                           */
/*  Globale Var. :                                                           */
/*                                                                           */
/*  Externe Var. :                                                           */
/*                                                                           */
/*  Aenderungen  :                                                           */
/*                                                                           */
/*****************************************************************************/

static Symbol read_conclusions ( void )
{
  Symbol      sym;
  char        ident[IDENTLENGTH];
  term        t1, t2;

  if ( spec_mode == proof )
    while ((sym = GetSymbol (ident)) == SCRSYM)
      CheckError ( EOI, "Unexpected end of file -->  goal equations expected." );
  else if ( spec_mode == database ) /*$4712*/
    while ((sym = GetSymbol (ident)) == SCRSYM)
      CheckError ( EOI, "Unexpected end of file --> conclusion(s) expected." );
  else
    while (!EOI && ((sym = GetSymbol (ident)) == SCRSYM));

  while (!EOI && !(spec_mode==database && sym==SEQUATIONS)) /*$4712*/
  {
    t1 = read_term ( ident, &sym );

    CheckError ( sym != SEQUAL, "`=' expected." );
    sym = GetSymbol (ident);
    t2 = read_term ( ident, &sym );
    new_variables ( t1, t2 );

    append ( &spec_conclusions, new_listcell (0, "", t1, t2) );

    CheckError ( sym != SCRSYM, "New line expected." );

    while (!EOI && ((sym = GetSymbol (ident)) == SCRSYM));
  }
  return sym;
} /* Ende von read_conclusions */


/*
//-----------------------------------------------------------------------------
//  Funktion:       complete_sig
//
//  Parameter:      sig         Pointer auf Signaturliste
//-----------------------------------------------------------------------------
*/

static  void  complete_sig ( listcell *sig )
{
    listcell    *ptr;
    short       counter = 0;
    sortT       s = 0; /* Initialisierung dazugefuegt, 08.03.95. Arnim.*/

    if (!sig)
        return;

    complete_sig (sig->next);
    /* Umbiegen der Zeiger mit Sorten-Liste fuer das Funktionssymbol, falls */
    /* eine Sortenliste fuer mehrere Funktionssymbole angegeben wurde.      */
    if ((!sig->pointer1) && (sig->next))
        sig->pointer1 = sig->next->pointer1;

    /* Bestimmen der Zielsorte und Zaehlen der Anzahl der Argumente. Dabei  */
    /* wird das Funktionssymbol und die Zielsorte mitgezaehlt --> bei       */
    /* SetArguments wird 2 subtrahiert.                                     */
    ptr = sig;
    while (ptr)
    {
        s = ptr->data;
        ptr = ptr->pointer1;
        counter++;
    }
    /* s beinhalte jetzt die Ergebnissorte des Funktionssymbols */
    /* in der Listenzelle, in der in data die Nummer des Funktionssymbols */
    /* steht erhaelt daher in sort auch die Ergebnissorte beim ersten     */
    /* Durchlauf der naechsten while-Schleife.                            */
    /* Ansonsten ist mir (MK) die folgende while-Schleife sehr suspekt,   */
    /* da der pointer1, ja evtl. von meheren Funktionssymbolen benutzt    */
    /* wird, und daher die Eintraege permanent ueberschrieben werden.     */
    /* Jedoch werden die Sorteneintraege der Argumentsorten, die ja in    */
    /* der Komponente data stehen nicht veraendert.                       */
    
    ptr = sig;
    while (ptr)
    {
        ptr->sort = s;
        ptr->info = sig->data;
        ptr = ptr->pointer1;
    }

    SetArguments ( sig->data, counter-2 );
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       do_sig
//
//  Beschreibung :  Hier wird die Information ueber die Signatur aus der Variablen
//                  spec_signature in die Variable Signature nach Funktionssymbolen
//                  sortiert transferiert ( Dies ist wichtig, wenn man bei 
//                  Sortenhierarchien eine mehrfach-Deklaration der Funktions-
//                  symbole zulaesst!
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

static  void  do_sig ( void )
{
    listcell    *ptr, *sig, *np;
    short       c;

    sig = spec_signature.first;
    while (sig)
    {
        /* Es wird die Zielsorte, die ja auch noch am Ende von pointer1 */
        /* haengt abgeschnitten!                                        */
        ptr = sig;
        for (c=0; c < Signature[sig->data].arity; c++, ptr = ptr->pointer1 );
        ptr->pointer1 = NULL; /* naemlich hier */

        /* In der neuen Listzelle steht in data das Funktionssymbol und */
        /* der pointer1 zeigt auf die Liste mit den Argument- und Zielsorten */
        np = new_listcell (sig->data, "", sig->pointer1, NULL);

        np->sort = sig->sort; /* Zielsorte wird eingetragen */

        /* Eintragen gemaess Funktionssymbolnummer */
        append ( &(Signature[sig->data].signature), np );
        sig = sig->next;
    }
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       complete_vars
//
//  Parameter:      vars        Pointer auf Signaturliste
//-----------------------------------------------------------------------------
*/

static  void  complete_vars ( listcell *vars )
{
    listcell    *ptr;
    sortT       s;

    if (!vars)
        return;

    complete_vars (vars->next);

    ptr = vars;
    while (ptr->pointer1)
        ptr = ptr->pointer1;
    s = ptr->data;

    ptr = vars;
    while (ptr->pointer1)
    {
        if (ptr->data != find_var (ptr->ident))
        {
            printf( "Multiple definition of variable `%s'!\n", ptr->ident );
            exit ( 1 );
        }
        ptr->sort = s;
        ptr = ptr->pointer1;
    }
}


static  void  complete_vars2 ( listcell *vars )
{
    sortT       s;

    while (vars)
    {
        s = vars->sort;
        if (!(Sorts[s].var))
            Sorts[s].var = vars->ident;
        vars = vars->next;
    }
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       get_spec_name
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

char  *get_spec_name ( void )
{
    return spec_name;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       get_mode
//
//  Parameter:      -keine-
//-----------------------------------------------------------------------------
*/

mode  get_mode ( void )
{
    return spec_mode;
}


/*
//-----------------------------------------------------------------------------
//  Funktionen:     get_sorts
//                  sorts_first
//                  sorts_rest
//-----------------------------------------------------------------------------
*/

sortdec  get_sorts ( void )
{
    return spec_sorts.first;
}


sortT    sorts_first ( sortdec apointer)
{
    listcell    *cell = (listcell*)(apointer);

    if (!cell) return ERROR_SORT;
    return (sortT) cell->data;
}


sortdec  sorts_rest ( sortdec apointer )
{
    listcell    *cell = (listcell*)(apointer);
    
    if (!cell) return NULL;

/* Warum hier pointer1 zurueckgeliefert wird, erfaehrt man in einem Kommentar */
/* in der Funktion read_sorts.                                                */
/* Martin Kronenburg                                                          */

    return cell->pointer1;
}



/*
//-----------------------------------------------------------------------------
//  Funktionen:     get_sig
//                  get_sig_func
//                  sig_first
//                  sig_rest
//                  sig_func
//                  sig_arity
//                  sig_arg
//                  sig_sort
//-----------------------------------------------------------------------------
*/

/*@functiondec  get_sig ( func_symb code )
{
    return Signature[code].signature.first;
}@*/



/*@funcdec  sig_first ( functiondec apointer )
{
    listcell    *cell = (listcell*)(apointer);

    if (!cell) return 0;
    return cell;
}@*/


/*@functiondec  sig_rest ( functiondec apointer )
{
    listcell    *cell = (listcell*)(apointer);

    if (!cell) return 0;
    return cell->next;
}@*/


int  sig_arity  ( func_symb code )
{
    return ((code > 0) && (code <= FunctionCounter)) ? Signature[code].arity : 0;
}


sortdec  sig_arg  ( func_symb code )
{
    return Signature[code].signature.first->pointer1;
}


sortT sig_sort ( func_symb code )
{
    return Signature[code].signature.first->sort;
}


func_symb_list  get_sig ( void )
{
    return (FunctionCounter) ? 1 : 0;
}


func_symb  sig_first ( func_symb_list dummy )
{
    return (FunctionCounter) ? dummy : 0;
}


func_symb_list  sig_rest ( func_symb_list code )
{
    return (code && (code < FunctionCounter)) ? code+1 : 0;
}


/*
//-----------------------------------------------------------------------------
//  Funktionen:     get_ordering
//                  ord_name
//                  ord_chain_list
//                  ord_first
//                  ord_rest
//                  ord_chain_first
//                  ord_chain_rest
//                  ord_func_weight
//-----------------------------------------------------------------------------
*/

orderdec  get_ordering ( void )
{
    return NULL;
}


ordering  ord_name ( orderdec apointer MAYBE_UNUSEDPARAM )
{
    return spec_ordering_mode;
}


order  ord_chain_list ( orderdec apointer MAYBE_UNUSEDPARAM )
{
    return spec_ordering.first;
}


ord_chain  ord_first ( order apointer )
{
    return apointer;
}


order  ord_rest ( order apointer )
{
    listcell    *cell = (listcell*)(apointer);

    return cell->next;
}


func_symb  ord_chain_first ( ord_chain apointer )
{
    listcell    *cell = (listcell*)(apointer);
    
    return cell->data;
}


ord_chain  ord_chain_rest ( ord_chain apointer )
{
    listcell    *cell = (listcell*)(apointer);

    return cell->pointer1;
}


weight  ord_func_weight ( func_symb fn )
{
    return Signature[fn].weight;
}


/*
//-----------------------------------------------------------------------------
//  Funktionen:     get_equations
//                  get_conclusions
//                  eq_first_lhs
//                  eq_first_rhs
//                  eq_rest
//-----------------------------------------------------------------------------
*/

eq_list  get_equations ( void )
{
    return spec_equations.first;
}

eq_list  get_conclusions ( void )
{
    return spec_conclusions.first;
}


term  eq_first_lhs ( eq_list apointer )
{
    return ((listcell*)(apointer))->pointer1;
}


term  eq_first_rhs ( eq_list apointer )
{
    return ((listcell*)(apointer))->pointer2;
}


eq_list  eq_rest ( eq_list apointer )
{
    return ((listcell*)(apointer))->next;
}


/*
//-----------------------------------------------------------------------------
//  Funktion:       parse_spec ( char *filename )
//
//  Parameter:      filename    Dateiname
//-----------------------------------------------------------------------------
*/

void  parse_spec ( char *filename )
{
    char        ident[IDENTLENGTH];
    Symbol      sym;

    INIT_VARIABLES();

    strcpy(Sorts[0].ident, "VOID");
    Sorts[0].counter = 0;
    Sorts[0].max = 0;
    Sorts[0].var = NULL;

    strcpy(Sorts[1].ident, "");
    Sorts[1].counter = 0;
    Sorts[1].max = 0;
    Sorts[1].var = NULL;

    OpenInput ( filename );

    while ((sym = GetSymbol (ident)) == SCRSYM)
        CheckError ( EOI, "Unexpected end of file." );

    CheckError ( (sym != SNAME), "`NAME' expected." );
    CheckError ( (GetSymbol (spec_name) != SIDENT), "Identifier expected." );
    CheckError ( (GetSymbol (ident) != SCRSYM), "New line expected." );

    while ((sym = GetSymbol (ident)) == SCRSYM)
        CheckError ( EOI, "Unexpected end of file." );

    CheckError ( (sym != SMODE), "`MODE' expected." );
    switch (GetSymbol (ident))
    {
      case SCOMPLETION:   spec_mode = completion;
                          break;
      case SCONVERGENCE:  spec_mode = convergence;
                          break;
      case SREDUCTION:    spec_mode = reduction;
                          break;
      case SCONFLUENCE:   spec_mode = confluence;
                          break;
      case SPROOF:        spec_mode = proof;
                          break;
      case STERMINATION:  spec_mode = termination;
                          break;
      case SDATABASE:     spec_mode = database;      /*$4712*/
                          break;
      default:            CheckError ( true, "Unknown mode." );
                          break;
    }

    while ((sym = GetSymbol (ident)) == SCRSYM)
        CheckError ( EOI, "Unexpected end of file." );

    CheckError ( (sym != SSORTS), "`SORTS' expected." );
    sym = read_sorts ();

    CheckError ( (sym != SSIGNATURE), "`SIGNATURE' expected." );
    sym = read_signature ();
    complete_sig  ( spec_signature.first );
    do_sig ();

    if (spec_mode!=database) {  /*$4712*/
        CheckError ( (sym != SORDERING), "`ORDERING' expected." );
        sym = read_ordering ();
    }

    CheckError ( (sym != SVARIABLES), "`VARIABLES' expected." );
    sym = read_variables ();

    complete_vars  ( spec_variables.first );
    complete_vars2 ( spec_variables.first );

    CheckError ( (sym != SEQUATIONS), "`EQUATIONS' expected." );
    sym = read_equations ();

    CheckError ( (sym != SCONCLUSION), "`CONCLUSION' expected." );
    sym = read_conclusions ();

    while (spec_mode==database && !EOI) { /*$4712*/
      CheckError ( (sym != SEQUATIONS), "`EQUATIONS' expected." );
      append ( &spec_equations, new_listcell (0, "", NULL, NULL) );
      sym = read_equations ();
      CheckError ( (sym != SCONCLUSION), "`CONCLUSION' expected." );
      append ( &spec_conclusions, new_listcell (0, "", NULL, NULL) );
      sym = read_conclusions ();
    }

    CloseInput ();
    CheckSignature ();
  }



  /*
  //-----------------------------------------------------------------------------
  //  Funktion:       clear_spec
  //-----------------------------------------------------------------------------
  */

  void  clear_spec ( void )
  {
      listcell    *p, *n;
      int         i;
      INIT_VARIABLES();

      n = list_of_conclusions.first;
      while ((p=n))
      {
          n = p->next;
          term_delete (p->pointer1);
          term_delete (p->pointer2);
          delete_listcell (p);
      }
      list_of_conclusions.first =
      list_of_conclusions.last  =
      list_of_conclusions.act1  =
      list_of_conclusions.act2  = NULL;

      n = list_of_equations.first;
      while ((p=n))
      {
          n = p->next;
          term_delete (p->pointer1);
          term_delete (p->pointer2);
          delete_listcell (p);
      }
      list_of_equations.first =
      list_of_equations.last  =
      list_of_equations.act1  =
      list_of_equations.act2  = NULL;

      
      VTclear ( &v_tree );

      for ( i = 0; i<= SortCounter; 
            Sorts[i++].max = 0 );

      if (is_fvar)
        free(is_fvar);

  }

/* Funktionen, um eine eingelesene Spezifikation wieder zu schreiben */
void printSorts (FILE *file) 
{
   int i;
   
   for (i = 1; i < SortCounter; i++) {
      fprintf (file, "%s ", Sorts[i].ident);
   }
   fprintf (file, "%s\n", Sorts[SortCounter].ident);
}

void printSig (FILE *file) 
{
   int         i;
   listcell    *ptr;
   int         ziel;
   
   for (i = 1; i <= FunctionCounter; i++) {
      fprintf (file, "\t%s :", Signature[i].ident);
      ptr = Signature[i].signature.first;
      ziel = ptr->sort;
      for (ptr = ptr->pointer1; ptr != NULL;
	   ptr = ptr->pointer1) {
	 fprintf (file, " %s", Sorts[ptr->data].ident);
      }
      fprintf (file, " -> %s\n", Sorts[ziel].ident);
   }
}

void printOrdering (FILE *file) 
{
   /* lieber mit Zugriffsfunktionen durchgehen, als mit Datenstrukturen selber */
   ord_chain ketten, aktKette;
   int i;
   int first = 1;


   for (i = 1; i <= FunctionCounter; i++) {
      if (Signature[i].weight != 0) {
	 if (first)
	    fprintf (file, "\t");
	 else
	    fprintf (file, ", ");
	 first = 0;
	 fprintf (file, "%s = %d", Signature[i].ident, Signature[i].weight);
      }
   }
   if (!first)
      fprintf (file, "\n");
      
   for (ketten = ord_chain_list(get_ordering()); ketten != NULL; ketten = ord_rest(ketten)) {
      aktKette = ord_first(ketten);
      fprintf (file, "\t%s", Signature[ord_chain_first(aktKette)].ident);
      for (aktKette = ord_chain_rest(aktKette); aktKette != NULL; aktKette = ord_chain_rest(aktKette))
	 fprintf (file, " > %s",  Signature[ord_chain_first(aktKette)].ident);
      fprintf (file, "\n");
   }
}

void printSort (FILE *file, sortT s)
{
   fprintf (file, "%s", Sorts[s].ident);
}

void  print_term ( term t )
{
    vartree     *vars    = NULL;
    variable    counter  = 0;

    pretty_print ( t, flat, &vars, &counter );
    VTclear ( &vars );
}

void  print_pair ( term left, char *c, term right)
{
    vartree     *vars    = NULL;
    variable    counter  = 0;

    pretty_print ( left,  flat, &vars, &counter );
    printf( c );
    pretty_print ( right, flat, &vars, &counter );
    putchar ( '\n' );
    VTclear ( &vars );
}
