/***** BEGIN LICENSE BLOCK *****
 * Version: CPL 1.0/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Common Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.eclipse.org/legal/cpl-v10.html
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * Copyright (C) 2010 Wayne Meissner, Tim Felgentreff
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the CPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the CPL, the GPL or the LGPL.
 ***** END LICENSE BLOCK *****/

#include "ruby.h"
#include "jruby.h"
#include "JLocalEnv.h"
#include "JUtil.h"
#include "errno.h"

using namespace jruby;

static VALUE jruby_select(void* data) {
    void** data_array = (void**)data;
    int max                 = (long)(data_array[0]);
    fd_set* read            = (fd_set*)(data_array[1]);
    fd_set* write           = (fd_set*)(data_array[2]);
    fd_set* except          = (fd_set*)(data_array[3]);
    struct timeval* timeout = (struct timeval*)(data_array[4]);

    int ret = select(max, read, write, except, timeout);
    xfree(data);
    if (ret < 0) {
        printf("jruby_select: An error occured: %d\n", errno);
    }
    return (VALUE)ret;
}

RUBY_DLLSPEC VALUE rb_thread_critical = 0;

extern "C" VALUE
rb_thread_local_aref(VALUE thread, ID id)
{
    return callMethod(thread, "[]", 1, ID2SYM(id));
}

extern "C" VALUE
rb_thread_local_aset(VALUE thread, ID id, VALUE value)
{
    return callMethod(thread, "[]=", 2, ID2SYM(id), value);
}

extern "C" int
rb_thread_alone()
{
    return rb_ary_size(callMethodA(rb_cThread, "list", 0, NULL)) < 2;
}

extern "C" void
rb_thread_schedule()
{
    callMethod(rb_cThread, "pass", 0);
}

extern "C" int
rb_thread_select(int max, fd_set * read, fd_set * write, fd_set * except, struct timeval *timeout)
{
    JLocalEnv env;

    if (!read && !write && !except) {
        // Just sleep for the specified amount of time
        long interval = timeout ? (timeout->tv_sec * 1000 + timeout->tv_usec / 1000) : 0;
        env->CallStaticVoidMethod(JRuby_class, JRuby_threadSleep, getRuntime(), (jint)interval);
        checkExceptions(env);
        return 0;
    } else {
        void** data = (void**)xmalloc(sizeof(void*) * 5);
        data[0] = reinterpret_cast<void*>(max);
        data[1] = (void*)read;
        data[2] = (void*)write;
        data[3] = (void*)except;
        data[4] = (void*)timeout;

        VALUE ret = rb_thread_blocking_region(jruby_select, (void*)data, NULL, NULL);
        return (int)ret;
    }
}

extern "C" VALUE
rb_thread_current(void)
{
    return callMethod(rb_cThread, "current", 0);
}

extern "C" VALUE
rb_thread_create(VALUE (*fn)(ANYARGS), void* arg)
{
    JLocalEnv env;
    jobject ret = env->CallStaticObjectMethod(JRuby_class, JRuby_newThread, getRuntime(),
                p2j((void*)fn), valueToObject(env, (arg == NULL ? rb_ary_new() : (VALUE)arg)));
    checkExceptions(env);
    return objectToValue(env, ret);
}

extern "C" VALUE
rb_thread_blocking_region(rb_blocking_function_t func, void* data, rb_unblock_function_t ub_func, void* data2)
{
    JLocalEnv env;
    jlong jret = env->CallStaticLongMethod(JRuby_class, JRuby_nativeBlockingRegion, getRuntime(),
            p2j((void *)func), p2j(data),
            p2j((void *)ub_func), p2j(data2));
    checkExceptions(env);

    return (VALUE)(j2p(jret));
}

extern "C" void
rb_thread_wait_fd_rw(int fd, int read)
{
    int result = 0;

    if (fd < 0) {
        rb_raise(rb_eIOError, "closed stream");
    }

    if (rb_thread_alone()) return;

    while (result <= 0) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);

        if (read) {
            result = rb_thread_select(fd + 1, &set, 0, 0, 0);
        } else {
            result = rb_thread_select(fd + 1, 0, &set, 0, 0);
        }

        if (result < 0) {
            rb_sys_fail(0);
        }
    }
}

extern "C" void
rb_thread_wait_fd(int f)
{
    rb_thread_wait_fd_rw(f, 1);
}

extern "C" int
rb_thread_fd_writable(int f)
{
    rb_thread_wait_fd_rw(f, 0);
    return Qtrue;
}

extern "C" void
rb_thread_wait_for(struct timeval time) {
    rb_thread_select(0, 0, 0, 0, &time);
}

extern "C" VALUE
rb_thread_wakeup(VALUE thread) {
    return callMethodA(thread, "wakeup", 0, NULL);
}
