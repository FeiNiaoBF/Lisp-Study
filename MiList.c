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

// 语法树递归
long eval(mpc_ast_t* t);
// 语法数求值
long eval_op(long x, char* op, long y);
// 语法函数
long eval_fun(char* fun, long x, long y);

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
      number   : /-?[0-9]+\\.[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' | '%' | '^'        \
               | \"min\" | \"max\" | \"add\" | \"sub\" | \"nul\" | \"div\" ;                      \
      expr     : <number> | '(' <operator> <expr>+ ')'    \
               | '(' <operator> <expr> ')' ;              \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Lispy);

puts("MiLisp Version 0.0.1.4\n");
puts("Press <Ctrl+c> to Exit\n");

while(1) {
  char* input = readline("Lisp >>> ");
  add_history(input);
  mpc_result_t r;
  if(mpc_parse("<stdin>", input, Lispy, &r)) {
      long result = eval(r.output);
      printf("%li\n", result);
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

long eval(mpc_ast_t* t) {
  /* If tagged as number return it directly. */ 
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  /* The operator is always second child. */
  char* op = t->children[1]->contents;
  if (strcmp(op, "-") == 0 && t->children_num == 4) {
    return -eval(t->children[2]);
  }
  long x = eval(t->children[2]);

  /* Handle min and max functions */
  if (strcmp(op, "min") == 0 || strcmp(op, "max") == 0) {
    for (int i = 3; i < t->children_num - 1; i++) {
      long y = eval(t->children[i]);
      if (strcmp(op, "min") == 0) {
        x = (x < y) ? x : y;
      } else {
        x = (x > y) ? x : y;
      }
    }
    return x;
  }

  /* Handle other operators */
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;  
}

  
long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) {return x + y;}
  if (strcmp(op, "-") == 0) {return x - y;}
  if (strcmp(op, "*") == 0) {return x * y;}
  if (strcmp(op, "/") == 0) {return x / y;}
  if (strcmp(op, "%") == 0) {return x % y;}
  if (strcmp(op, "^") == 0) {return pow(x, y);}
  return 0;
  }

long eval_fun(char* fun, long x, long y) {
  if (strcmp(fun, "min") == 0) {return x < y ? x : y;}
  if (strcmp(fun, "max") == 0) {return x > y ? x : y;}
  if (strcmp(fun, "add") == 0) {return eval_op(x, "+", y);}
  if (strcmp(fun, "sub") == 0) {return eval_op(x, "-", y);}
  if (strcmp(fun, "mul") == 0) {return eval_op(x, "*", y);}
  if (strcmp(fun, "div") == 0) {return eval_op(x, "/", y);}
  return 0;
}










