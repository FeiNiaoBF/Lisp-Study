#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define LASSERT(args, cond, err) \
  if (!(cond)) { lval_del(args); return lval_err(err); }



#ifdef _WIN32

#include <string.h>
static char buffer[2048];

char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

#else
#ifdef __linux__
#include <editline/readline.h>
#include <editline/history.h>
#endif

#ifdef __MACH__
#include <editline/readline.h>
#endif
#endif

// =========================================
// 创建可能的错误类型枚举
// enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUMS, LERR_BAD_FUNC};
// 创建可能的lval类型的枚举
enum {LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR};

// =========================================
// Number type


// Declare New lval Struct
typedef struct lval
{
  int lisptype;
  union{
  long lnum;
  double dnum;
  };
  char* err;
  char* sym;
  struct lval** cell;
  int count;
}lval;


// =========================================

// 语法树递归
// lval eval(mpc_ast_t* t);
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
lval* lval_add(lval* v, lval* x);
// 语法数求值
lval* lval_eval_sexpr(lval* v);
lval* lval_eval(lval* v);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* lval_join(lval* x, lval* y);
lval* builtin_op(lval* a, char* op);
lval* builtin(lval* a, char* func);
// head function
lval* builtin_head(lval* a);
// tail function
lval* builtin_tail(lval* a);
lval* builtin_list(lval* a);
lval* builtin_eval(lval* a);
lval* builtin_join(lval* a);
// create a new number type lval
lval* lval_num(long x);

// create a new error type lval
lval* lval_err(char* e);
lval* lval_sym(char* s);
lval* lval_sexpr(void);
void  lval_del(lval* v);

// create qexpr type
lval* lval_qexpr(void);

// print lavl
void lval_print(lval* v);
void lval_expr_print(lval* v, char open, char close);
void lval_println(lval* v);


// ===================MAIN======================

int main(int argc, char** argv) {

/* Create Some Parsers */
mpc_parser_t* Number   = mpc_new("number");
//mpc_parser_t* Letter   = mpc_new("letter");
mpc_parser_t* Symbol   = mpc_new("symbol");
mpc_parser_t* Sexpr    = mpc_new("sexpr");
mpc_parser_t* Qexpr    = mpc_new("qexpr");
mpc_parser_t* Expr     = mpc_new("expr");
mpc_parser_t* Lispy    = mpc_new("lispy");
//mpc_parser_t* String   = mpc_new("string");


/* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                               \
      number   : /-?[0-9]+\\.?[0-9]*/ ;             \
      symbol   : '+' | '-' | '*' | '/' | '%' | '^'  \
               | \"min\" | \"max\"                  \
               | \"list\" | \"head\" | \"tail\"     \
               | \"join\" | \"eval\" ;              \
      sexpr    : '(' <expr>* ')' ;                  \
      qexpr    : '{' <expr>* '}' ;                  \
      expr     : <number> | <symbol>                \
               | <sexpr> | <qexpr>  ;               \
      lispy    : /^/ <expr>* /$/ ;                  \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);


puts("MiLisp Version 0.0.2.2");
puts("Press <Ctrl+c> to Exit\n");

while(1) {
  char* input = readline("Lisp >>> ");
  add_history(input);
  mpc_result_t r;
  if(mpc_parse("<stdin>", input, Lispy, &r)) {
      // mpc_ast_print(r.output);
      // mpc_ast_delete(r.output);
      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  free(input);
  }

mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

return 0;
}


// ====================FUNC=====================

// computer
lval* lval_eval_sexpr(lval* v) {
  // Evaluate Children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }
  // Error Checking
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->lisptype == LVAL_ERR) {return lval_take(v, i);}
  }
  // Empty Expression
  if (v->count == 0) return v;
  // Single Exprssion
  if (v->count == 1) return lval_take(v, 0);
  // Ensure First Element is Symbol
  lval* lv = lval_pop(v, 0);
  if (lv->lisptype != LVAL_SYM) {
    lval_del(lv);
    lval_del(v);
    return lval_err("S-expression Does not start with symbol!");
  }

  // Call builtin with operator
  lval* result = builtin(v, lv->sym);
  lval_del(lv);
  return result;
}

lval* lval_eval(lval* v) {
  // Evaluate Sexpressions 
  if (v->lisptype == LVAL_SEXPR) { return lval_eval_sexpr(v); }
  // All other lval types remain the same 
  return v;
}

lval* lval_pop(lval* v, int i) {
  // Find the item at i
  lval* x = v->cell[i];
  // shift memory after the item at i over the top
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
  v->count--;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

// del v
lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}
// 求值函数
lval* builtin_op(lval* a, char* op) {
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->lisptype != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }
  lval* x = lval_pop(a, 0);
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->lnum = ~x->lnum + 1;
  }
  while (a->count > 0) {
    /* Pop the next element */
    lval* y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) { x->lnum += y->lnum; }
    if (strcmp(op, "-") == 0) { x->lnum -= y->lnum; }
    if (strcmp(op, "*") == 0) { x->lnum *= y->lnum; }
    if (strcmp(op, "%") == 0) { x->lnum %= y->lnum; }
    if (strcmp(op, "^") == 0) { x->lnum = pow(x->lnum, y->lnum); }
    if (strcmp(op, "/") == 0) {
      if (y->lnum == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Division By Zero!"); break;
      }
      x->lnum /= y->lnum;
    }
    lval_del(y);
  }
  lval_del(a); 
  return x;
}


// Read number 
lval* lval_read_num(mpc_ast_t* t){
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE
          ? lval_num(x)
          : lval_err("Invalid Number!");
}

// Read type
lval* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) return lval_read_num(t);
  if (strstr(t->tag, "symbol")) return lval_sym(t->contents);
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) {x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr"))  {x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr"))  {x = lval_qexpr(); }
  // Fill this list with any valid expression contained within
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }  
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->tag, "regex") == 0)  { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }
  return x;
}

// Add sub-lval
lval* lval_add(lval* v, lval* x){
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

// NUmber type
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->lisptype = LVAL_NUM;
  v->lnum = x;
  return v;
}

// Construct a pointer to a new Error lval
lval* lval_err(char* e) {
  lval* v = malloc(sizeof(lval));
  v->lisptype = LVAL_ERR;
  v->err = malloc(strlen(e) + 1);
  strcpy(v->err, e);
  return v;
}

// Construct a pointer to a new Symbol lval 
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->lisptype = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

// A pointer to a new empty Sexpr lval */
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->lisptype = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_qexpr(void){
  lval* v = malloc(sizeof(lval));
  v->lisptype = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}


// A function to free memory
void lval_del(lval* v) {
  switch (v->lisptype)
  {
  case LVAL_NUM: 
    break;
  case LVAL_ERR:
    free(v->err);
    break;
  case LVAL_SYM:
    free(v->sym);
    break;
  case LVAL_QEXPR:
  case LVAL_SEXPR:
    for(int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }
    free(v->cell);
    break;
  default:
    break;
  }
  free(v);
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for(int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch (v->lisptype) {
    case LVAL_NUM:   printf("%li", v->lnum); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    default:
      break;  
  }
  
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

lval* builtin_head(lval* a) {
  // Only one Qexpr
  LASSERT(a, a->count == 1,
    "Function 'head' passed too many arguments!");
  // can only be Qexpr type
  LASSERT(a, a->cell[0]->lisptype == LVAL_QEXPR,
    "Function 'head' passed incorrect type!");
  // Can not be empty 
  LASSERT(a, a->cell[0]->count != 0,
    "Function 'head' passed {}!");

  lval* v = lval_take(a, 0);
  while (v->count > 1)
  {
    lval_del(lval_pop(v, 1));
  }
  return v;
}

lval* builtin_tail(lval* a) {
    // Only one Qexpr
  LASSERT(a, a->count == 1,
    "Function 'tail' passed too many arguments!");
  // can only be Qexpr type
  LASSERT(a, a->cell[0]->lisptype == LVAL_QEXPR,
    "Function 'tail' passed incorrect type!");
  // Can not be empty 
  LASSERT(a, a->cell[0]->count != 0,
    "Function 'tail' passed {}!");

  lval* v = lval_take(a, 0);
  // del first
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lval* a) {
  a->lisptype = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lval* a) {
  LASSERT(a, a->count == 1,
    "Function 'eval' passed too many arguments!");
  LASSERT(a, a->cell[0]->lisptype == LVAL_QEXPR,
    "Function 'eval' passed incorrect type");

  lval* x = lval_take(a, 0);
  x->lisptype = LVAL_SEXPR;
  return lval_eval(x);
}

lval* builtin_join(lval* a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT(a, a->cell[i]->lisptype == LVAL_QEXPR,
      "Function 'join' passed incorrect type");
  }
  lval* x = lval_pop(a, 0);
  while (a->count)
  {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval* lval_join(lval* x, lval* y) {
  while (y->count)
  {
    x = lval_add(x, lval_pop(y, 0));
  }
  lval_del(y);
  return x;
}


lval* builtin(lval* a, char* func) {
  if (strcmp("list", func) == 0) { return builtin_list(a); }
  if (strcmp("head", func) == 0) { return builtin_head(a); }
  if (strcmp("tail", func) == 0) { return builtin_tail(a); }
  if (strcmp("join", func) == 0) { return builtin_join(a); }
  if (strcmp("eval", func) == 0) { return builtin_eval(a); }
  if (strstr("+-/*", func)) { return builtin_op(a, func); }
  lval_del(a);
  return lval_err("Unknown Function!");
}



