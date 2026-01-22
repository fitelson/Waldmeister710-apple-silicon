%{
#include "parse-util.h"
#define YYDEBUG 1
%}

%union{
   char      *ident;
   int       value;
   mode_type modus;
   TermT     term;
   TermT     term_list;
   order     chain_list;
   ord_chain chain;
}

%token NAME
%token MODE
%token SORT
%token SIGNATURE
%token ORDERING
%token VARIABLE
%token EQUATION
%token CONCLUSION
%token ADD
%token DELETE
%token TEST

%token COMPLETION
%token CONFLUENCE
%token CONVERGENCE
%token PROOF
%token REDUCTION
%token TERMINATION
%token CCE
%token ACG

%token LPO
%token KBO

%token <value> NUMBER
%token <ident> IDENT

%token EQ
%token ARROW

%token ENDOFLINE

%type <term_list> termlist
%type <term_list> args
%type <term> term
%type <modus> mode
%type <chain> precchain
%type <chain_list> precedence
%type <ident> fid
%start spec
%%

spec:     decls {end_of_decls();} data {end_of_file();}
    ;
decls:    namedecl modedecl sortdecl sigdecl orddecl vardecl
    ;
data:     
    |     eqns    data
    |     conclns data
    |     adds    data
    |     dels    data
    |     tests   data
    ;
namedecl: NAME IDENT {/* ignore name */}
    ;
modedecl: MODE mode  {set_mode($2);}
    ;
sortdecl: SORT sdecls
    ;
sigdecl:  SIGNATURE fdecls 
    ;
orddecl:  ORDERING odecl 
    ;
vardecl:  VARIABLE vdecls 
    ;
eqns:     EQUATION   {eqmode = eqn_mode;}   equationlist
    ;
adds:     ADD        {eqmode = add_mode;}   equationlist
    ;
dels:     DELETE     {eqmode = del_mode;}   equationlist
    ;
tests:    TEST       {eqmode = test_mode;}  equationlist
    ;
conclns:  CONCLUSION {eqmode = conc_mode;}  equationlist
    ;      

mode:     COMPLETION  {$$ = completion;} 
    |     CONFLUENCE  {$$ = confluence;} 
    |     CONVERGENCE {$$ = convergence;} 
    |     PROOF       {$$ = proof;} 
    |     REDUCTION   {$$ = reduction;} 
    |     TERMINATION {$$ = termination;} 
    |     CCE         {$$ = cce;} 
    |     ACG         {$$ = acg;} 
    ;

sdecls:   sdecl 
    |     sdecls sdecl
    ;
sdecl:    IDENT    { 
                       if (!add_symbol ($1, sort_sym, 0)) {
                         yyerror(error_msg);
                         YYERROR;
                       }
                   }
    ;
sid:      IDENT    {
                      if (get_symbol_type($1) != sort_sym) {
                        yyerror("sort identifier expected");
                        YYERROR;
                      }
                   }
    ;

fdecls:   fdecl {/* evtl. eps-Produktion ? */}
    |     fdecls fdecl
    ;
fdecl:    fidsdecl ':' {arity = 0;} sids ARROW sid {update_arities(arity);}
    ;
sids:
    |     sids sid {arity++;}
    ;
fidsdecl: fiddecl
    |     fidsdecl ',' fiddecl
    ;
fiddecl:  IDENT     { 
                       if (!add_symbol ($1, fun_sym, -1)) {
                         yyerror(error_msg);
                         YYERROR;
                       }
                    }
    ;
fid:      IDENT     {
                       if (get_symbol_type($1) != fun_sym) {
                         yyerror("function symbol expected");
                         YYERROR;
                       }
                       $$ = $1;
                    }
    ;

odecl:    LPO  precedence            {set_ordering(ordering_lpo,$2);}
    |     KBO  weightlist precedence {set_ordering(ordering_kbo,$3);}
    ;
precedence:                    {$$ = NULL;}
    |     precchain precedence {$$ = cons_chains($1, $2);}
    ;
precchain:
          fid                {$$ = start_chain($1);}
    |     fid '>' precchain  {$$ = cons_chain($1,$3);}
    ;
weightlist:
          weight
    |     weightlist ',' weight 
    ;
weight:   fid EQ {expectNo();} NUMBER {set_weight($1,$4);expectId();}
    ; 

vdecls: 
    |     vdecls vdecl
    ;
vdecl:    vidsdecl ':' sid
    ;
vidsdecl: viddecl
    |     vidsdecl ',' viddecl
    ;
viddecl:  IDENT     {      
                      if (!add_symbol ($1, var_sym, 0)) {
                        yyerror(error_msg);
                        YYERROR;
                      }
                    }
    ;

equationlist: 
    |     equationlist equation
    ;
equation: term EQ term {handle_equation(eqmode,$1,$3);}
    ;
term:     IDENT args { $$ = mkTerm ($1, $2);
                       if ($$ == NULL) {
                         yyerror(error_msg);
                         YYERROR;
                       }
                     }
    ;
args:     '(' termlist ')'  {$$ = $2;}
    |     '(' ')'           {$$ = NULL;}
    |                       {$$ = NULL;}
    ;
termlist: term ',' termlist {$$ = prepend($1,$3);}
    |     term              {$$ = $1;}
    ;
