#ifndef PARSE_UTIL_H
#define PARSE_UTIL_H

#include "SymbolOperationen.h"
#include "TermOperationen.h"

typedef unsigned int                            weight;

typedef enum { 
  completion, 
  confluence, 
  convergence, 
  proof, 
  reduction, 
  termination, 
  cce,
  acg,
  database 
} mode_type;
typedef enum {sort_sym, fun_sym, var_sym} sym_type;
typedef enum {eqn_mode, conc_mode, add_mode, del_mode, test_mode} eqmode_type;
typedef enum { 
  ordering_kbo, 
  ordering_lpo,
  ordering_cons
} ordering_type;

extern ordering_type PA_Ordering;
#define PA_Ordering() PA_Ordering

typedef long        func_symb;          /* Datentyp func_symb            */
typedef func_symb   func_symb_list;

typedef struct ord_chainS {
  func_symb         first;
  struct ord_chainS *rest;
} ord_chainT, *ord_chain;

typedef struct orderS {
  ord_chain     chain;
  struct orderS *rest;
} orderT, *order;

typedef struct orderdecS {
  ordering_type name;
  order         chain_list;
} orderdecT, *orderdec;

extern char *error_msg;
extern eqmode_type eqmode;
extern int arity;

void init_apriori(void);

int   add_symbol (char *name, sym_type tp, int no_args);
void  update_arities(int);
int   get_symbol_type (char *name);
TermT prepend(TermT, TermT);
TermT mkTerm(char *, TermT);

void set_ordering (ordering_type o, order chainlist);
void set_weight (char *name, int val);
order cons_chains (ord_chain chain, order chain_list);
ord_chain start_chain (char* name);
ord_chain cons_chain (char* name, ord_chain chain);

void end_of_decls(void);
void end_of_file(void);
void handle_equation(eqmode_type,TermT,TermT);
void print_symbols (void);

void expectNo(void);
void expectId(void);
int  noExpected(void);


void set_mode(mode_type m);

/* ====================================================================== */


/*---- Zugriff auf Funktionsdeklarationen -----------------------------------*/

int             sig_arity               ( func_symb );
func_symb_list  get_sig                 ( void );
func_symb       sig_first               ( func_symb_list );
func_symb_list  sig_rest                ( func_symb_list );
#define sig_is_fvar( func_symb ) 0

/*---- Zugriff auf Ordnungsinformationen ------------------------------------*/

orderdec        get_ordering            ( void );
ordering_type   ord_name                ( orderdec );
order           ord_chain_list          ( orderdec );
ord_chain       ord_first               ( order );
order           ord_rest                ( order );
func_symb       ord_chain_first         ( ord_chain );
ord_chain       ord_chain_rest          ( ord_chain );
weight          ord_func_weight         ( func_symb );

char*           map_func                ( func_symb );

#define SN_WMvonDISCOUNT(symbol) (symbol - 1)

#define PM_Existenzziele() FALSE
#define PA_FailingCompletion() FALSE
#define PA_Frischzellenkur() FALSE
#define PA_PermLemmata() FALSE
#define PA_FullPermLemmata() FALSE
#define PA_IdempotenzLemmata() FALSE
#define PA_PermsubAC() FALSE
#define PA_PermsubACI() FALSE
#define PA_doGZFB() FALSE
#define PA_doACG() TRUE
#define PA_CTreeVersion() 60

#define PA_CTreeP()         FALSE
#define PA_CTreeS()         FALSE
#define PA_maxNodeCount()   0
#define PA_mergeDCons()     FALSE
#define PA_optDCons()       TRUE
#define PA_preferD1()       TRUE
#define PA_sortDCons()      TRUE

#endif
