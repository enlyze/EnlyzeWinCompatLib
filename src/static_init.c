/* This file provides routines used for thread-safe initialisation of static
 * variables when /Zc:threadSafeInit is used (enabled by default).
 *
 * This is intended to override the default implementations from the Microsoft
 * C++ Runtime which are compiled to target Windows Vista or later.
 *
 * Modeled on the reference implementation in thread_safe_statics.cpp in the
 * Microsoft C++ Runtime.
*/

#include <assert.h>
#include <limits.h>
#include <pthread.h>

static const int UNINITIALIZED = 0;
static const int INITIALIZING = -1;

static const int EPOCH_BASE = INT_MIN;

/* Exposed as a public symbol in the reference implementation, so exposed it
 * has to stay here too...
*/
int _Init_global_epoch = EPOCH_BASE;
__declspec(thread) int _Init_thread_epoch = EPOCH_BASE;

static pthread_mutex_t _Init_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _Init_thread_cond = PTHREAD_COND_INITIALIZER;

void __cdecl _Init_thread_lock()
{
        pthread_mutex_lock(&_Init_thread_mutex);
}

void __cdecl _Init_thread_unlock()
{
        pthread_mutex_unlock(&_Init_thread_mutex);
}

void __cdecl _Init_thread_wait_v2()
{
        pthread_cond_wait(&_Init_thread_cond, &_Init_thread_mutex);
}

void __cdecl _Init_thread_notify()
{
        pthread_cond_broadcast(&_Init_thread_cond);
}

void __cdecl _Init_thread_header(int* const pOnce)
{
    _Init_thread_lock();

    if (*pOnce == UNINITIALIZED)
    {
        *pOnce = INITIALIZING;
    }
    else
    {
        while (*pOnce == INITIALIZING)
        {
            _Init_thread_wait_v2();

            if (*pOnce == UNINITIALIZED)
            {
                *pOnce = INITIALIZING;
                _Init_thread_unlock();
                return;
            }
        }
        _Init_thread_epoch = _Init_global_epoch;
    }

    _Init_thread_unlock();
}

void __cdecl _Init_thread_abort(int* const pOnce)
{
    _Init_thread_lock();

    *pOnce = UNINITIALIZED;

    _Init_thread_unlock();
    _Init_thread_notify();
}

void __cdecl _Init_thread_footer(int* const pOnce)
{
    _Init_thread_lock();

    ++_Init_global_epoch;

    /* Probably unlikely condition... you would need to construct ~2 billion
     * static objects before the "epoch" would roll up to the "INITIALIZING"
     * constant and cause weird behaviour... the official implementation
     * technically has this bug too.
    */
    assert(_Init_global_epoch < INITIALIZING);

    *pOnce = _Init_global_epoch;
    _Init_thread_epoch = _Init_global_epoch;

    _Init_thread_unlock();
    _Init_thread_notify();
}
