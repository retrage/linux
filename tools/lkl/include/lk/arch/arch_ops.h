/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <lk/assert.h>
#include <lk/arch/thread.h>
#include <lk/arch/arch_thread.h>

__BEGIN_CDECLS;

extern int ints_enabled;
extern int fiqs_enabled;

static inline void arch_enable_ints(void)
{
    spinlock_thread_data *t = get_current_spinlock_thread_data();

    ASSERT(t->old_ints_is_valid);
    t->old_ints_is_valid = false;
    t->old_ints_state = 0;
    ints_enabled = 1;
}

static inline void arch_disable_ints(void)
{
    spinlock_thread_data *t = get_current_spinlock_thread_data();

    ASSERT(!t->old_ints_is_valid);
    t->old_ints_state = 1;
    t->old_ints_is_valid = true;
    ints_enabled = 0;
}

static inline bool arch_ints_disabled(void)
{
    return !ints_enabled;
}

static inline void arch_enable_fiqs(void)
{
    fiqs_enabled = 1;
}

static inline void arch_disable_fiqs(void)
{
    fiqs_enabled = 0;
}

static inline bool arch_fiqs_disabled(void)
{
    return !fiqs_enabled;
}

static inline uint arch_curr_cpu_num(void) {
    return 0;
}

/* use a global pointer to store the current_thread */
extern struct thread *_current_thread;

static inline struct thread *get_current_thread(void)
{
    return _current_thread;
}

static inline void set_current_thread(struct thread *t)
{
    _current_thread = t;
}


__END_CDECLS;
