#include <assert.h>
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

// Initialization.
// TODO: this could be an extern global, once lld supports us importing them,
//       https://bugs.llvm.org/show_bug.cgi?id=40364
//       (it already works for asm.js)
EMSCRIPTEN_KEEPALIVE void __init_sbrk(void** value) {
  DYNAMICTOP_PTR = value;
}

int brk(void* new) {
  void* total = emscripten_get_total_memory();

#ifdef __EMSCRIPTEN_PTHREADS__

  // Perform a compare-and-swap loop to update the new dynamic top value. This is because
  // this function can becalled simultaneously in multiple threads.
  void* seen;
  void* old;
  do {
    old = (void*)__c11_atomic_load((_Atomic size_t*)*DYNAMICTOP_PTR, __ATOMIC_SEQ_CST);
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
    
    __c11_atomic_compare_exchange_strong((_Atomic size_t*)DYNAMICTOP_PTR, &seen, new, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
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

