/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"
#include "util.h"

/* counter for variable memory locations */
static int location = 0;

static ScopeList globalScope = NULL;
static char *funcName;
static int predScope = FALSE;
int noReturn = FALSE;


/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t,
               void (* preProc) (TreeNode *),
               void (* postProc) (TreeNode *) )
{ if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}

static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Type error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

static void voidValDeclError(TreeNode *t, char *name) {
    fprintf(listing, "Error: Variable Type cannot be Void at line %d (name : %s)\n", t->lineno, name);
    Error = TRUE;
}

static void undeclaredError(TreeNode *t, char *name) {
    fprintf(listing, "Error: Undelcared Symbol \"%s\" at line %d\n", name, t->lineno);
    Error = TRUE;
}

static void redefinedError(TreeNode *t, char *name) {
    fprintf(listing, "Error: Redefined Symbol \"%s\" at line %d\n", name, t->lineno);
    Error = TRUE;
}

static pushBuiltInFunc(void) {
    TreeNode *func;
    TreeNode *param;
    TreeNode *compStmt;
    TreeNode *typeSpec;
    
    
    typeSpec = newExpNode(IdK);
    typeSpec->type = Integer;
    
    compStmt = newStmtNode(CompK);
    compStmt->child[0] = NULL;
    compStmt->child[1] = NULL;
    
    
    // input function
    func = newDeclNode(FuncK);
    func->type = Integer;
    func->lineno = 0;
    func->attr.name = "input";
    
    param = newDeclNode(ParamK);
    param->type = Void;
    
    func->child[0] = param;
    func->child[1] = compStmt;
    
    st_insert("input", 0, getLocation(), func);

    
    // output function
    func = newDeclNode(FuncK);
    func->type = Void;
    func->lineno = 0;
    func->attr.name = "output";
    
    param = newDeclNode(ParamK);
    param->attr.name = "value";
    param->type = Integer;
    
    func->child[0] = param;
    func->child[1] = compStmt;
    
    st_insert("output", 0, getLocation(), func);
    ScopeList scope = sc_create("output");
    sc_push(scope);
    st_insert("value", 0, getLocation(), param);
    sc_pop();
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t)
{ if (t==NULL) return;
  else return;
}

/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */
static void insertNode( TreeNode * t) {
    switch (t->nodekind) {
            case StmtK:
                switch (t->kind.stmt) {
                    case CompK:
                        if (predScope) predScope = FALSE;
                        else {
                            ScopeList scope = sc_create(funcName);
                            sc_push(scope);
                            location++;
                        }
                        t->attr.scope = sc_top();
                    default:
                        break;
                }
                break;
            case ExpK:
                switch (t->kind.exp) {
                    case IdK:
                    case ArrayK:
                    case CallK:
                        if (st_lookup(t->attr.name) == -1) {
                            undeclaredError(t, t->attr.name);
                        }
                        else st_add_lineno(t->attr.name, t->lineno);
                        break;
                    default:
                        break;
                }
                break;
            case DeclK:
                switch (t->kind.decl) {
                    case FuncK:
                        funcName = t->attr.name;
                        if (st_lookup_now(funcName)) {
                            redefinedError(t, funcName);
                            break;
                        }
                        
                        st_insert(funcName, t->lineno, getLocation(), t);
                        sc_push(sc_create(funcName));
                        predScope = TRUE;
                        break;
                    case VarK:
                    {   char *name = t->attr.name;
                        if (t->type == Void) {
                            voidValDeclError(t, name);
                            break;
                        }
                        
                        if (t->child[0] != NULL) t->type = IntArray;

                        if (!st_lookup_now(name)) st_insert(name, t->lineno, getLocation(), t);
                        else {
                            redefinedError(t, name);
                        }
                    }
                        break;
                    case ParamK:
                        if (t->attr.name == NULL) {
                            if (t->type == Void) break;
                            else {/* parameter need name */}
                        } else if (t->type == Void && t->child[0] != NULL) {
                            typeError(t, "void array type is invalid");
                        } else if (st_lookup(t->attr.name) == -1) {
                            st_insert(t->attr.name, t->lineno, getLocation(), t);
                            if (t->child[0] != NULL) t->type = IntArray;
                        }
                        else {
                            redefinedError(t, t->attr.name);
                        }
                        break;
                }
                break;
            default:
                break;
    }
}



static void afterInsertNode(TreeNode *t) {
    if (t->nodekind == StmtK && t->kind.stmt == CompK) {
        sc_pop();
    }
}

static void beforeCheckNode(TreeNode *t) {
    if (t->nodekind == StmtK) {
        if (t->kind.stmt == CompK) sc_push(t->attr.scope);
    } else if (t->nodekind == DeclK) {
        if (t->kind.decl == FuncK) {
            funcName = t->attr.name;
            noReturn = (t->type == Void);
        }
    }
}

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree) {
    globalScope = sc_create("global");
    location = 0;
    
    sc_push(globalScope);
    pushBuiltInFunc();
    traverse(syntaxTree, insertNode, afterInsertNode);
    sc_pop();
    
    if (TraceAnalyze) {
        printSymTab(listing);
    }
}


/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t) {
    switch (t->nodekind) {
        case ExpK:
            switch (t->kind.exp) {
                case AssignK:
                    if (t->child[0]->type == Void || t->child[1]->type == Void)
                        typeError(t->child[0], "void can't be assigned");
                    else if ((t->child[0]->type == IntArray && t->child[0]->child[0] == NULL) || (t->child[1]->type == IntArray && t->child[1]->child[0] == NULL))
                        typeError(t->child[0], "var can't assigned array, array can't assigned var");
                    else t->type = Integer;
                    break;
                case OpK: {
                    TreeNode *lhs = t->child[0];
                    TreeNode *rhs = t->child[1];
                    ExpType l = lhs->type;
                    ExpType r = rhs->type;
                    
                    if (l == IntArray && lhs->child[0] != NULL) {
                        l = Integer;
                    }
                    if (r == IntArray && rhs->child[0] != NULL) {
                        r = Integer;
                    }
                    
                    if (l == Void || r == Void)
                        typeError(t, "lhs and rhs must be int");
                    else if (l == IntArray && r == IntArray)
                        typeError(t, "array can't be operand");
                    else if (l != r) {
                        typeError(t, "array can't be operand");
                    } else t->type = Integer;
                    
                    break;
                }
                case CallK: {
                    BucketList b = st_lookup_bucket(t->attr.name);
                    if (b == NULL) break;
                    
                    TreeNode *func = b->treeNode;
                    TreeNode *args = t->child[0];
                    TreeNode *params = params = func->child[0];
                    
                    
                    if (func->kind.decl != FuncK) {
                        typeError(t, "this is not function");
                        break;
                    }
                    
                    if (args == NULL) {
                        if (params != NULL && params->type != Void) {
                            typeError(t, "invalid function call: not matching # of arg <-> params )");
                            break;
                        }
                    }
                    
                    while (args != NULL) {
                        ExpType argType = args->type;
                        if (argType == IntArray && args->child[0] != NULL) argType = Integer;
                        if (params == NULL) {
                            typeError(t, "invalid function call: not mathing # of args <-> params");
                            break;
                        }
                        else if (argType != params->type) {
                            typeError(t, "invalid function call: not mathing type(arg <-> param))");
                            break;
                        }
                        else if (argType == Void) {
                            typeError(t, "invalid function call: argument can't be Void");
                            break;
                        }
                        else {
                            args = args->sibling;
                            params = params->sibling;
                        }
                    }
                    t->type = func->type;
                    break;
                }
                case ConstK:
                    t->type = Integer;
                    break;
                case IdK:
                case ArrayK: {
                    BucketList b = st_lookup_bucket(t->attr.name);
                    if (b == NULL) break;
                    
                    TreeNode *var = b->treeNode;
                    
                    if (t->kind.exp == ArrayK) {
                        if (var->nodekind == DeclK && var->kind.decl == VarK && var->child[0] == NULL) {
                            typeError(t, "array need index");
                            break;
                        } else if (t->child[0]->type != Integer) {
                            typeError(t, "indices must be integer type");
                        } else t->type = Integer;
                    } else t->type = var->type;
                }
                    break;
                default:
                    break;
            }
            break;
        case StmtK:
            switch (t->kind.stmt) {
                case CompK:
                    sc_pop();
                    break;
                case IfK:
                    if (t->child[0]->type != Integer)
                        typeError(t->child[0], "condition type must be int");
                    break;
                case WhileK:
                    if (t->child[0]->type != Integer)
                        typeError(t->child[0], "condition type must be int");
                    break;
                case ReturnK: {
                    TreeNode *func = st_lookup_bucket(funcName)->treeNode;
                    if (func->type == Void) {
                        if (t->child[0] != NULL) {
                            typeError(t, "void function has no return val");
                        }
                    } else if (func->type == Integer) {
                        if (t->child[0] == NULL || t->child[0]->type == Void) {
                            typeError(t, "non-void function has return val");
                        } else if (t->child[0]->type == IntArray && t->child[0]->child[0] == NULL) {
                            typeError(t, "array can't be return");
                        }
                        noReturn = TRUE;
                    }
                    break;
                }
                default:
                    break;
            }
            break;

        case DeclK:
            switch (t->kind.decl) {
                case VarK:
                    if (t->type == Void) {
                        voidValDeclError(t, t->attr.name);
                    }
                    break;
                case FuncK:
                    if (!noReturn)
                        typeError(t, "int function need return stmt");
                    break;
                case ParamK:
                    break;
                default:
                    break;
            }
        default:
            break;
    }
        return;
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree) {
    sc_push(globalScope);
    traverse(syntaxTree,beforeCheckNode,checkNode);
    sc_pop();
}
