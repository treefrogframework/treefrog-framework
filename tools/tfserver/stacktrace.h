// Copyright (c) 2000 - 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Modified by AOYAMA Kazuharu
//
// Routines to extract the current stack trace.  These functions are
// thread-safe.

#pragma once
#include "gconfig.h"

_START_GOOGLE_NAMESPACE_


// Wrapper of __sync_val_compare_and_swap. If the GCC extension isn't
// defined, we try the CPU specific logics (we only support x86 and
// x86_64 for now) first, then use a naive implementation, which has a
// race condition.
template <typename T>
inline T sync_val_compare_and_swap(T *ptr, T oldval, T newval)
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    T ret;
    __asm__ __volatile__("lock; cmpxchg %1, (%2);"
                         : "=a"(ret)
                         // GCC may produces %sil or %dil for
                         // constraint "r", but some of apple's gas
                         // dosn't know the 8 bit registers.
                         // We use "q" to avoid these registers.
                         : "q"(newval), "q"(ptr), "a"(oldval)
                         : "memory", "cc");
    return ret;
#else
    // This has a race condition...
    T ret = *ptr;
    if (ret == oldval) {
        *ptr = newval;
    }
    return ret;
#endif
}


// This is similar to the GetStackFrames routine, except that it returns
// the stack trace only, and not the stack frame sizes as well.
// Example:
//      main() { foo(); }
//      foo() { bar(); }
//      bar() {
//        void* result[10];
//        int depth = GetStackFrames(result, 10, 1);
//      }
//
// This produces:
//      result[0]       foo
//      result[1]       main
//           ....       ...
//
// "result" must not be NULL.
extern int GetStackTrace(void **result, int max_depth, int skip_count);

_END_GOOGLE_NAMESPACE_

