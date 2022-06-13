/*
 *
 *   这是一个个人学习项目
 *   有关于Lisp的解释器
 *   作者：Yeelight
 *   找我：https://feiniaobf.github.io/
 *
 *
 */

#include "mpc.h"
#include <stdio.h>
#include <math.h>
// 固定大小的数组缓冲区
// static char input[2048];

#ifdef _WIN32
static char buffer[2048];

char *readline(char *prompt);
void add_history(char *unused);

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

/*=================Error Handling==================*/
/* 创建lval结构 */
typedef struct
{
    int type; // lval 类型
    long num; // 数值大小
    int err;  // 错误类型
} lval;

/* 创建可能错误类型的枚举 */
enum
{
    LERR_DIV_ZERO, // 不能被0整除
    LERR_BAD_OP,   // 操作符未知
    LERR_BAD_NUM   //操作数过大
};
/* 创建可能 lval 类型的枚举 */
enum
{
    LVAL_NUM, // lval 表示数值
    LVAL_ERR  // lval 表示错误
};
/* 有关lval的函数声明定义 */
/* 创建一个新的数字类型 lval */
lval lval_num(long x);
/* 创建一个新的错误类型 lval*/
lval lval_err(int x);
/* 打印lval类型 */
void lval_print(lval v);
/* 打印lval+换行*/
void lval_println(lval v);

/* Use operator string to see which operation to perform */
/*使用操作符字符串查看要执行的操作*/
lval eval_op(lval x, char *op, lval y);

/* recursive evaluation function */
/* 递归求值函数 */
lval eval(mpc_ast_t *t);

int main(int argc, char **argv)
{
    /* Create Some Parsers */
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Operator = mpc_new("operator");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' ;                  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
              Number, Operator, Expr, Lispy);

    /* Print Version and Exit Information */
    puts("Lispy Version 0.0.4-1");
    puts("Prsee Ctrl+c to Exit\n");

    /* In a never ending loop */
    while (1)
    {
        /* Output our prompt */
        /*输出我们的提示*/
        // fputs("lispy> ", stdout);
        char *input = readline("lispy> ");
        add_history(input);
        /* Attempt to Parse the user Input */
        /* 尝试解析用户输入 */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r))
        {

            lval result = eval(r.output);
            lval_println(result);
            // printf("%li\n", result);
            mpc_ast_delete(r.output);
        }
        else
        {
            /* Otherwise Print the Error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    /* Undefine and delete our parsers */
    mpc_cleanup(5, Number, Operator, Expr, Lispy);

    return 0;
}

char *readline(char *prompt)
{
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char *cpy = malloc(strlen(buffer) + 1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy) - 1] = '\0';
    return cpy;
}
void add_history(char *unused) {}

lval eval_op(lval x, char *op, lval y)
{
    /* If either value is an error return it */
    if (x.type == LVAL_ERR)
    {
        return x;
    }
    if (y.type == LVAL_ERR)
    {
        return y;
    }

    /* Otherwise do maths on the number values */
    if (strcmp(op, "+") == 0)
    {
        return lval_num(x.num + y.num);
    }
    if (strcmp(op, "-") == 0)
    {
        return lval_num(x.num - y.num);
    }
    if (strcmp(op, "*") == 0)
    {
        return lval_num(x.num * y.num);
    }
    if (strcmp(op, "/") == 0)
    {
        /* If second operand is zero return error */
        return y.num == 0
                   ? lval_err(LERR_DIV_ZERO)
                   : lval_num(x.num / y.num);
    }

    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t *t)
{
    /* If tagged as number return it directly. */
    if (strstr(t->tag, "number"))
    {
        /* Check if there is some error in conversion */
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    /* The operator is always second child. */
    char *op = t->children[1]->contents;
    lval x = eval(t->children[2]);

    /* The operator is always second child. */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr"))
    {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

/* 创建一个新的数字类型 lval */
lval lval_num(long x)
{
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}
/* 创建一个新的错误类型 lval*/
lval lval_err(int x)
{
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}
/* 打印lval类型 */
void lval_print(lval v)
{
    switch (v.type)
    {
    /* 类型为数值*/
    case LVAL_NUM:
        printf("%li", v.num);
        break;
    /* 类型为错误*/
    case LVAL_ERR:
        if (v.err == LERR_DIV_ZERO)
        {
            printf("Error: Division By Zero!\n");
        }
        if (v.err == LERR_BAD_OP)
        {
            printf("Error: Invalid Operator!\n");
        }
        if (v.err == LERR_BAD_NUM)
        {
            printf("Error: Invalid Number!!\n");
        }
        break;
    }
}

void lval_println(lval v)
{
    lval_print(v);
    putchar('\n');
}
