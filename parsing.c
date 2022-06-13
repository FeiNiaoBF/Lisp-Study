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
// 固定大小的数组缓冲区
static char input[2048];

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
    puts("Lispy Version 0.0.2-0");
    // puts("Author: Yeelight");
    puts("Prsee Ctrl+c to Exit\n");

    /* In a never ending loop */
    while (1)
    {
        /* Output our prompt */
        /*输出我们的提示*/
        fputs("lispy> ", stdout);

        /* Read a line of user input of maximum size 2048 */
        /*读取一行最大大小为2048的用户输入*/
        fgets(input, 2048, stdin);

        /* Attempt to Parse the user Input */
        /* 尝试解析用户输入 */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r))
        {
            /* On Success Print the AST */
            mpc_ast_print(r.output);
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

    return 0;
}
