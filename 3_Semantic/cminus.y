/****************************************************/
/* File: tiny.y                                     */
/* The TINY Yacc/Bison specification file           */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedLineNo;  /* ditto */
static int savedNum;
static TreeNode * savedTree; /* stores syntax tree for later return */

static int yylex(void);
int yyerror(char *s);

%}

%token IF ELSE WHILE RETURN INT VOID
%token ID NUM
%token ASSIGN EQ LT LE GT GE NE PLUS MINUS TIMES OVER LPAREN RPAREN LBRACE RBRACE LCURLY RCURLY SEMI COMMA
%token ERROR

%right THEN ELSE

%% /* Grammar for TINY */

program     : decl_list
                 { savedTree = $1;}
            ;
decl_list   : decl_list decl
                { YYSTYPE t = $1;
                  if (t != NULL)
                  { while (t->sibling != NULL)
                      t = t->sibling;
                    t->sibling = $2;
                    $$ = $1; }
                  else $$ = $2;
                }
            | decl { $$ = $1; }
            ;
decl        : var_decl { $$ = $1; }
            | func_decl { $$ = $1; }
            ;
var_decl    : type_spec id SEMI
                  { $$ = newDeclNode(VarK);
                    $$->attr.name = savedName;
                    $$->lineno = savedLineNo;
                    $$->type = $1->type;
                  }
            | type_spec
              id
                  { $$ = newDeclNode(VarK);
                    $$->attr.name = savedName;
                    $$->lineno = savedLineNo;
                    $$->type = $1->type;
                  }
              LBRACE
              NUM {
                  $$ = $3;
                  $$->child[0] = newExpNode(ConstK);
                  $$->child[0]->attr.val = atoi(tokenString); }
              RBRACE SEMI { $$ = $6; }
            ;
type_spec   : INT
                { $$ = newExpNode(IdK);
                  $$->type = Integer;
                }
            | VOID { $$ = newExpNode(IdK);
		    $$->type = Void; }
            ;
func_decl   : type_spec id
                  { $$ = newDeclNode(FuncK);
                    $$->lineno = savedLineNo;
                    $$->attr.name = savedName; }
              LPAREN params RPAREN comp_stmt
                  { $$ = $3;
                    $$->child[0] = $5;
                    $$->child[1] = $7;
                    $$->type = $1->type;
                  }
            ;
params      : param_list { $$ = $1; }
            | VOID
                { $$ = newDeclNode(ParamK);
                  $$->type = Void;
                  $$->lineno = lineno;
            }
            ;
param_list  : param_list COMMA param
                { YYSTYPE t = $1;
                  if (t != NULL)
                  { while (t->sibling != NULL)
                      t = t->sibling;
                    t->sibling = $3;
                    $$ = $1; }
                    else $$ = $3;
                }
            | param { $$ = $1; }
            ;
param       : type_spec id
                 { $$ = newDeclNode(ParamK);
                   $$->attr.name = savedName;
                   $$->lineno = lineno;
                   $$->type = $1->type; }
            | type_spec id LBRACE RBRACE
                 { $$ = newDeclNode(ParamK);
                   $$->attr.name = savedName;
                   $$->lineno = savedLineNo;
                   $$->type = $1->type;
                   $$->child[0] = newExpNode(ConstK);
                   $$->child[0]->attr.val = -1; }
            ;
id          : ID { savedName = copyString(tokenString);
                   savedLineNo = lineno; }
            ;
comp_stmt   : LCURLY local_decl stmt_list RCURLY
                { $$ = newStmtNode(CompK);
                  $$->child[0] = $2;
                  $$->child[1] = $3; }
            ;
local_decl  : local_decl var_decl
                { YYSTYPE t = $1;
                  if (t != NULL)
                  { while (t->sibling != NULL)
                      t = t->sibling;
                    t->sibling = $2;
                    $$ = $1; }
                  else $$ = $2;
                }
            | { $$ = NULL; }
            ;
stmt_list   : stmt_list stmt
                { YYSTYPE t = $1;
                  if (t != NULL)
                  { while (t->sibling != NULL)
                      t = t->sibling;
                    t->sibling = $2;
                    $$ = $1; }
                  else $$ = $2;
                }
            | { $$ = NULL; }
            ;
stmt        : exp_stmt { $$ = $1; }
            | comp_stmt { $$ = $1; }
            | select_stmt { $$ = $1; }
            | iter_stmt { $$ = $1; }
            | return_stmt { $$ = $1; }
            ;
exp_stmt    : exp SEMI
                { $$ = $1; }
            | SEMI { $$ = NULL;}/* no action */
            ;
select_stmt : IF LPAREN exp RPAREN stmt %prec THEN
                { $$ = newStmtNode(IfK);
                  $$->child[0] = $3;
                  $$->child[1] = $5; }
            | IF LPAREN exp RPAREN stmt ELSE stmt
                { $$ = newStmtNode(IfK);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                  $$->child[2] = $7; }
            ;
iter_stmt   : WHILE LPAREN exp RPAREN stmt
                { $$ = newStmtNode(WhileK);
                  $$->child[0] = $3;
                  $$->child[1] = $5; }
            ;
return_stmt : RETURN SEMI
                { $$ = newStmtNode(ReturnK); }
            | RETURN exp SEMI
                { $$ = newStmtNode(ReturnK);
                  $$->child[0] = $2; }
            ;
exp         : var ASSIGN exp
                { $$ = newExpNode(AssignK);
                  $$->child[0] = $1;
                  $$->child[1] = $3; }
            | simple_exp { $$ = $1; }
            ;
var         : id
                { $$ = newExpNode(IdK);
                  $$->attr.name = savedName;
                }
            | id
                { $$ = newExpNode(ArrayK);
                  $$->attr.name = savedName;
                }
              LBRACE exp RBRACE
                { $$ = $2;
                  $$->child[0] = $4; }
            ;
simple_exp  : add_exp relop add_exp
                { $$ = $2;
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                }
            | add_exp { $$ = $1; }
            ;
relop       : LE
                { $$ = newExpNode(OpK);
                  $$->attr.op = LE; }
            | LT
                { $$ = newExpNode(OpK);
                  $$->attr.op = LT; }
            | GT
                { $$ = newExpNode(OpK);
                  $$->attr.op = GT; }
            | GE
                { $$ = newExpNode(OpK);
                  $$->attr.op = GE; }
            | EQ
                { $$ = newExpNode(OpK);
                  $$->attr.op = EQ; }
            | NE
                { $$ = newExpNode(OpK);
                 $$->attr.op = NE; }
            ;
add_exp     : add_exp addop term
                { $$ = $2;
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                }
            | term { $$ = $1; }
            ;
addop       : PLUS
                { $$ = newExpNode(OpK);
                  $$->attr.op = PLUS; }
            | MINUS
                { $$ = newExpNode(OpK);
                  $$->attr.op = MINUS; }
            ;
term        : term mulop factor
                { $$ = $2;
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                }
            | factor { $$ = $1;}
            ;
mulop       : TIMES
                { $$ = newExpNode(OpK);
                  $$->attr.op = TIMES; }
            | OVER
                { $$ = newExpNode(OpK);
                  $$->attr.op = OVER; }
            ;
factor      : LPAREN exp RPAREN { $$ = $2;}
            | var { $$ = $1; }
            | call { $$ = $1; }
            | NUM
                { $$ = newExpNode(ConstK);
                  $$->attr.val = atoi(tokenString); }
            ;
call        : id
                { $$ = newExpNode(CallK);
                  $$->attr.name = savedName;
                }
              LPAREN args RPAREN
                { $$ = $2;
                  $$->child[0] = $4; }
            ;
args        : arg_list { $$ = $1; }
            | { $$ = NULL; }
            ;
arg_list    : arg_list COMMA exp
                { YYSTYPE t = $1;
                  if (t != NULL)
                  { while (t->sibling != NULL)
                      t = t->sibling;
                    t->sibling = $3;
                    $$ = $1; }
                    else $$ = $3;
                }
            | exp { $$ = $1; }
            ;
%%

int yyerror(char * message)
{ fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}


