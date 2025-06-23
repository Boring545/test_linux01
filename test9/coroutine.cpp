#include <setjmp.h>
#include <ucontext.h>
#include <stdio.h>
jmp_buf env;

void func(int arg)
{
    printf("func %d\n", arg);
    longjmp(env, ++arg);
}
void jmp_test()
{
    int ret = setjmp(env);

    if (ret == 0)
    {
        func(ret);
    }
    else if (ret == 1)
    {
        func(ret);
    }
    else if (ret == 2)
    {
        func(ret);
    }
    else if (ret == 3)
    {
        func(ret);
    }
}

ucontext_t ctx[2];
ucontext_t main_ctx;
int count = 0;
void func1()
{
    char data[1024];
    while (count++ < 20)
    {
        printf("1\n");
        swapcontext(&ctx[0],&ctx[1]);
        printf("3\n");
    }
}
void func2()
{
    char data[1024];
    while (count++ < 20)
    {
        printf("2\n");
        swapcontext(&ctx[1],&ctx[0]);
        printf("4\n");
    }
}
void ucontext_test()
{
    char stack1[2048] = {0};
    char stack2[2048] = {0};

    getcontext(&ctx[0]);
    ctx[0].uc_stack.ss_sp = stack1;
    ctx[0].uc_stack.ss_size = sizeof(stack1);
    ctx[0].uc_link = &main_ctx;
    makecontext(&ctx[0], func1, 0);

    getcontext(&ctx[1]);
    ctx[1].uc_stack.ss_sp = stack2;
    ctx[1].uc_stack.ss_size = sizeof(stack2);
    ctx[1].uc_link = &main_ctx;
    makecontext(&ctx[1], func2, 0);

    printf("swap!!\n");
    swapcontext(&main_ctx, &ctx[0]);
    printf("4\n");
}
int main()
{
    // jmp_test()
    ucontext_test();

    return 0;
}