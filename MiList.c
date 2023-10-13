#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// #define VERSION 0.0.1.2
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

// =============================
// 创建可能的错误类型枚举
enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUMS, LERR_BAD_FUNC};
// 创建可能的lval类型的枚举
enum {LVAL_NUM, LVAL_ERR};

// =============================
// Number type


// Declare New lval Struct
typedef struct lval
{
  int lisptype;
  union{
  long lnum;
  double dnum;
  int err;
  };
}lval;



// 语法树递归
lval eval(mpc_ast_t* t);
// 负数语法
// lval eval_neg(lval v);
// 语法数求值
lval eval_op(lval x, char* op, lval y);
// 语法函数
// long eval_fun(char* fun, long x, long y);
// create a new number type lval
lval lval_num(double x);
// create a new error type lval
lval lval_err(int e);

// print lavl
void lval_print(lval lv);
void lval_println(lval lv);




int main(int argc, char** argv) {

/* Create Some Parsers */
mpc_parser_t* Number   = mpc_new("number");
//mpc_parser_t* Letter   = mpc_new("letter");
mpc_parser_t* Operator = mpc_new("operator");
//mpc_parser_t* Function = mpc_new("function");
mpc_parser_t* Expr     = mpc_new("expr");
mpc_parser_t* Lispy    = mpc_new("lispy");
//mpc_parser_t* String   = mpc_new("string");


/* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+\\.?[0-9]*/ ;                             \
      operator : '+' | '-' | '*' | '/' | '%' | '^'        \
               | \"min\" | \"max\" ;                      \
      expr     : <number> | '(' <operator> <expr>+ ')' | '-' <expr>;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Lispy);


puts("MiLisp Version 0.0.1.6");
puts("Press <Ctrl+c> to Exit\n");

while(1) {
  char* input = readline("Lisp >>> ");
  add_history(input);
  mpc_result_t r;
  if(mpc_parse("<stdin>", input, Lispy, &r)) {
      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  free(input);
  }
mpc_cleanup(4, Number, Operator, Expr, Lispy);
return 0;
}

lval eval(mpc_ast_t* t) {
  /* If tagged as number return it directly. */ 
  if (strstr(t->tag, "number")) {
    errno = 0;
    double x = strtod(t->contents, NULL);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUMS);
  }

  /* The operator is always second child. */
  char* op = t->children[1]->contents;
  // The op is '-'
  // if (strcmp(op, "-") == 0 ) { //&& t->children_num == 4
  //   return eval_neg(eval(t->children[2]));
  // }
  lval x = eval(t->children[2]);

  /* Handle min and max functions */
  // if (strcmp(op, "min") == 0 || strcmp(op, "max") == 0) {
  //   for (int i = 3; i < t->children_num - 1; i++) {
  //     lval y = eval(t->children[i]);
  //     if (strcmp(op, "min") == 0) {
  //       x = (x < y) ? x : y;
  //     } else {
  //       x = (x > y) ? x : y;
  //     }
  //   }
  //   return x;
  // }

  /* Handle other operators */
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;  
}

  
lval eval_op(lval x, char* op, lval y) {
if (strcmp(op, "+") == 0) { return lval_num(x.dnum + y.dnum); }
  if (strcmp(op, "-") == 0) { return lval_num(x.dnum - y.dnum); }
  if (strcmp(op, "*") == 0) { return lval_num(x.dnum * y.dnum); }
  if (strcmp(op, "/") == 0) {
    return y.dnum == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.dnum / y.dnum);
  }
  if (strcmp(op, "%") == 0) {
    return y.dnum == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(fmod(x.dnum, y.dnum));
  }
  if (strcmp(op, "^") == 0) {
    return lval_num(pow(x.dnum, y.dnum));
  }
  if (strcmp(op, "min") == 0) {
    return lval_num(x.dnum < y.dnum ? x.dnum : y.dnum);
  }
  if (strcmp(op, "max") == 0) {
    return lval_num(x.dnum > y.dnum ? x.dnum : y.dnum);
  }

  return lval_err(LERR_BAD_OP);
}

// long eval_fun(char* fun, long x, long y) {
//   if (strcmp(fun, "min") == 0) {return x < y ? x : y;}
//   if (strcmp(fun, "max") == 0) {return x > y ? x : y;}
//   if (strcmp(fun, "add") == 0) {return eval_op(x, "+", y);}
//   if (strcmp(fun, "sub") == 0) {return eval_op(x, "-", y);}
//   if (strcmp(fun, "mul") == 0) {return eval_op(x, "*", y);}
//   if (strcmp(fun, "div") == 0) {return eval_op(x, "/", y);}
//   return 0;
// }

// lval eval_neg(lval v){
//   v.num = ~(v.num) + 1;
//   return v;
// }

lval lval_num(double x) {
  lval v;
  v.lisptype = LVAL_NUM;
  v.dnum = x;
  return v;
}


lval lval_err(int e) {
  lval v;
  v.lisptype = LVAL_ERR;
  v.err = e;
  return v;
}

void lval_print(lval lv) {
  switch (lv.lisptype) {
    case LVAL_NUM: 
        printf("%.2lf", lv.dnum);
      break;
    case LVAL_ERR:
      if(lv.err == LERR_DIV_ZERO) 
        printf("Error: Division By Zero!");
      if(lv.err == LERR_BAD_OP)
        printf("Error: Invalid Operator!");
      if(lv.err == LERR_BAD_NUMS)
        printf("Error: Invalid Number!");
      break;
  }
}

void lval_println(lval lv) {
  lval_print(lv);
  putchar('\n');
}










