#include <errno.h>
#include <unistd.h>

#include <emscripten.h>

/*
 * Simple sbrk implementation for emscripten.
 */

// The location in memory of the current brk, that is, the top of memory.
static void** DYNAMICTOP_PTR;

#define PTR_ADD(x, y) ((void*)(((char*)x) + y))
#define PTR_GT(x, y) (((char*)x) > ((char*)y))

// TODO: initialize this more efficiently by hardcoding the value at compile
//       time, when not linkable.
EMSCRIPTEN_KEEPALIVE void init_memory(void** value) {
  DYNAMICTOP_PTR = value;
}

int brk(void* new) {
  void* total = emscripten_get_total_memory();

#ifdef __EMSCRIPTEN_PTHREADS__

  // Perform a compare-and-swap loop to update the new dynamic top value. This is because
  // this function can becalled simultaneously in multiple threads.
  do {
    void* old = __c11_atomic_load((_Atomic size_t*)*DYNAMICTOP_PTR, __ATOMIC_SEQ_CST);
    // Asking to increase dynamic top to a too high value? In pthreads builds we cannot
    // enlarge memory, so this needs to fail.
    if (PTR_GT(new, total)) {
      // call enlarge, which will abort if ABORTING_MALLC
      int ret = emscripten_enlarge_memory();
      assert(ret == 0); // must have failed
      errno = ENOMEM;
      return -1;
    }
    // Attempt to update the dynamic top to new value. Another thread may have beat this thread to the update,
    // in which case we will need to start over by iterating the loop body again.
    
    void* seen = __c11_atomic_compare_exchange_strong((_Atomic size_t*)DYNAMICTOP_PTR, &old, new, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  } while(seen != old);

#else // singlethreaded build: (-s USE_PTHREADS=0)

  void* old = *DYNAMICTOP_PTR;
  *DYNAMICTOP_PTR = new;
  if (PTR_GT(new, total)) {
    if (emscripten_enlarge_memory() == 0) {
      *DYNAMICTOP_PTR = old;
      errno = ENOMEM;
      return -1;
    }
  }

#endif // __EMSCRIPTEN_PTHREADS__

  return 0;
}


void* sbrk(intptr_t increment) {
  void* old = *DYNAMICTOP_PTR;
  void* new = PTR_ADD(old, increment);
  if (brk(new) == -1) {
    return (void*)-1;
  }
  return old;
}

