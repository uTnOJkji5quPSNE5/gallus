#include "gallus_apis.h"





#define SPIN_LOCK_INITIALIZER	0





typedef volatile int spin_lock_t;





static inline gallus_result_t
spin_lock_init(spin_lock_t *l) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    *l = SPIN_LOCK_INITIALIZER;
    mbar();
    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
spin_lock_cas(spin_lock_t *l) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    while (__sync_val_compare_and_swap(l, 0, 1) != 0) {
      ;
    }
    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
spin_trylock_cas(spin_lock_t *l) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    if (__sync_val_compare_and_swap(l, 0, 1) == 0) {
      ret = GALLUS_RESULT_OK;      
    } else {
      ret = GALLUS_RESULT_BUSY;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
spin_lock_atomic_incr(spin_lock_t *l) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    while (__sync_fetch_and_add(l, 1) != 0) {
      (void)__sync_fetch_and_sub(l, 1);
    }
    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
spin_trylock_atomic_incr(spin_lock_t *l) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    if (__sync_fetch_and_add(l, 1) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      (void)__sync_fetch_and_sub(l, 1);
      ret = GALLUS_RESULT_BUSY;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
spin_unlock(spin_lock_t *l) {
  return spin_lock_init(l);
}






int
main(int argc, const char *const argv[]) {
  size_t i;
  uint64_t starts[6];
  uint64_t ends[6];
  double elapsed[6];
  spin_lock_t l;
  gallus_spinlock_t ll;

  (void)argc;
  (void)argv;

  /* CAS */
  (void)spin_lock_init(&l);
  starts[0] = gallus_rdtsc();
  for (i = 0; i < 1000LL * 1000LL * 1000LL; i++) {
    (void)spin_lock_cas(&l);
    (void)spin_unlock(&l);
  }
  ends[0] = gallus_rdtsc();
  fprintf(stderr, "\t\tcas done.\n");

  /* atomic incr. */
  (void)spin_lock_init(&l);
  starts[1] = gallus_rdtsc();
  for (i = 0; i < 1000LL * 1000LL * 1000LL; i++) {
    (void)spin_lock_atomic_incr(&l);
    (void)spin_unlock(&l);
  }
  ends[1] = gallus_rdtsc();
  fprintf(stderr, "\t\tatom done.\n");

  /* CAS (contention) */
  (void)spin_lock_init(&l);
  if (spin_lock_cas(&l) != GALLUS_RESULT_OK) {
    exit(1);
  }
  starts[2] = gallus_rdtsc();
  for (i = 0; i < 1000LL * 1000LL * 1000LL; i++) {
    (void)spin_trylock_cas(&l);
  }
  ends[2] = gallus_rdtsc();
  fprintf(stderr, "\t\tcas(cont) done.\n");

  /* atomic incr. (contention) */
  (void)spin_lock_init(&l);
  if (spin_lock_atomic_incr(&l) != GALLUS_RESULT_OK) {
    exit(1);
  }
  starts[3] = gallus_rdtsc();
  for (i = 0; i < 1000LL * 1000LL * 1000LL; i++) {
    (void)spin_trylock_atomic_incr(&l);
  }
  ends[3] = gallus_rdtsc();
  fprintf(stderr, "\t\tatom(cont) done.\n");

  /* pthread */
  (void)gallus_spinlock_initialize(&ll);
  starts[4] = gallus_rdtsc();
  for (i = 0; i < 1000LL * 1000LL * 1000LL; i++) {
    (void)gallus_spinlock_lock(&ll);
    (void)gallus_spinlock_unlock(&ll);
  }
  ends[4] = gallus_rdtsc();  
  fprintf(stderr, "\t\tpthd done.\n");

  /* pthread (contention) */
  (void)gallus_spinlock_lock(&ll);
  starts[5] = gallus_rdtsc();
  for (i = 0; i < 1000LL * 1000LL * 1000LL; i++) {
    (void)gallus_spinlock_trylock(&ll);
  }
  ends[5] = gallus_rdtsc();
  fprintf(stderr, "\t\tpthd(cont) done.\n");

  for (i = 0; i < 6; i++) {
    elapsed[i] = (double)(ends[i] - starts[i]) / (1000.0 * 1000.0 * 1000.0);
  }

  fprintf(stdout, "cas:\t%20.6f\n", elapsed[0]);
  fprintf(stdout, "atom:\t%20.6f\n", elapsed[1]);
  fprintf(stdout, "pthd:\t%20.6f\n", elapsed[4]);
  fprintf(stdout, "cas(cont):\t%20.6f\n", elapsed[2]);
  fprintf(stdout, "atom(cont):\t%20.6f\n", elapsed[3]);
  fprintf(stdout, "pthd(cont):\t%20.6f\n", elapsed[5]);

  return 0;
}
