#include <lk/kernel/thread.h>
#include <lk/kernel/timer.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct thread *_current_thread = NULL;
int ints_enabled = 0;
int fiqs_enabled = 0;

static void initial_thread_func(void)
{
    int ret;

    /* release the thread lock that was implicitly held across the reschedule */
    spin_unlock(&thread_lock);

    thread_t *ct = get_current_thread();
    ret = ct->entry(ct->arg);

    thread_exit(ret);
}

void arch_thread_initialize(struct thread* t) {
    // init context
    getcontext(&t->arch.context);

    // set stack
    t->arch.context.uc_stack.ss_sp = t->stack;
    t->arch.context.uc_stack.ss_size = t->stack_size;
    t->arch.context.uc_stack.ss_flags = 0;

    // disable return
    t->arch.context.uc_link = NULL;

    // set entrypoint
    makecontext(&t->arch.context, initial_thread_func, 0);
}

void arch_context_switch(struct thread *oldthread, struct thread *newthread) {
    swapcontext(&oldthread->arch.context, &newthread->arch.context);
}

void arch_idle(void) {
    thread_preempt();
}

void arch_dump_thread(thread_t *t) {

}

#if 0
void timer_initialize(lk_timer_t *timer) {
    assert(0);
}
void timer_set_oneshot(lk_timer_t *timer, lk_time_t delay, timer_callback callback, void *arg) {
    assert(0);
}
void timer_cancel(lk_timer_t *timer) {
    assert(0);
}
void timer_set_periodic(lk_timer_t *timer, lk_time_t period, timer_callback callback, void *arg) {
    assert(0);
}
#endif
