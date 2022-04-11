#include "gallus_apis.h"

#include <math.h>





typedef struct gallus_statistic_struct {
  const char *m_name;
  volatile size_t m_n;
  volatile int64_t m_min;
  volatile int64_t m_max;
  volatile int64_t m_sum;
  volatile int64_t m_sum2;
} gallus_statistic_struct;





static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static bool s_is_inited = false;
static gallus_hashmap_t s_stat_tbl;





static void s_ctors(void) __attr_constructor__(112);
static void s_dtors(void) __attr_destructor__(112);

static void s_destroy_stat(gallus_statistic_t s, bool delhash);
static void s_stat_freeup(void *arg);

static gallus_result_t s_reset_stat(gallus_statistic_t s);





static void
s_once_proc(void) {
  gallus_result_t r;

  if ((r = gallus_hashmap_create(&s_stat_tbl,
                                  GALLUS_HASHMAP_TYPE_STRING,
                                  s_stat_freeup)) != GALLUS_RESULT_OK) {
    gallus_perror(r);
    gallus_exit_fatal("can't initialize the stattistics table.\n");
  }

  s_is_inited = true;
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  gallus_msg_debug(10, "The statistics module is initialized.\n");
}


static inline void
s_final(void) {
  gallus_hashmap_destroy(&s_stat_tbl, true);
}


static void
s_dtors(void) {
  if (s_is_inited == true) {
    if (gallus_module_is_unloading() &&
        gallus_module_is_finalized_cleanly()) {
      s_final();

      gallus_msg_debug(10, "The stattitics module is finalized.\n");
    } else {
      gallus_msg_debug(10, "The stattitics module is not finalized "
                    "because of module finalization problem.\n");
    }
  }
}





static void
s_stat_freeup(void *arg) {
  if (likely(arg != NULL)) {
    gallus_statistic_t s = (gallus_statistic_t)arg;
    s_destroy_stat(s, false);
  }
}





static inline gallus_result_t
s_create_stat(gallus_statistic_t *sptr, const char *name) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL &&
             IS_VALID_STRING(name) == true)) {
    gallus_statistic_t s = (gallus_statistic_t)malloc(sizeof(*s));
    const char *m_name = strdup(name);
    *sptr = NULL;

    if (likely(s != NULL && IS_VALID_STRING(m_name) == true)) {
      void *val = (void *)s;
      if (likely((ret = gallus_hashmap_add(&s_stat_tbl, (void *)m_name,
                                            &val, false)) ==
                 GALLUS_RESULT_OK)) {
         s->m_name = m_name;
         s_reset_stat(s);
         *sptr = s;
      }
    } else {
      ret = GALLUS_RESULT_NO_MEMORY;
    }

    if (unlikely(ret != GALLUS_RESULT_OK)) {
      free((void *)s);
      free((void *)m_name);
    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_destroy_stat(gallus_statistic_t s, bool delhash) {
  if (likely(s != NULL)) {
    if (delhash == true) {
      (void)gallus_hashmap_delete(&s_stat_tbl,
                                   (void *)s->m_name, NULL, false);
    }
    if (s->m_name != NULL) {
      free((void *)s->m_name);
    }
    free((void *)s);
  }
}


static inline gallus_result_t
s_find_stat(gallus_statistic_t *sptr, const char *name) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL &&
             IS_VALID_STRING(name) == true)) {
    void *val = NULL;

    *sptr = NULL;

    ret = gallus_hashmap_find(&s_stat_tbl, (void *)name, &val);
    if (likely(ret == GALLUS_RESULT_OK)) {
      *sptr = (gallus_statistic_t)val;
    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_reset_stat(gallus_statistic_t s) {
  if (likely(s != NULL)) {
    s->m_n = 0LL;
    s->m_min = LLONG_MAX;
    s->m_max = LLONG_MIN;
    s->m_sum = 0LL;
    s->m_sum2 = 0LL;
    return GALLUS_RESULT_OK;
  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}


static inline gallus_result_t
s_record_stat(gallus_statistic_t s, int64_t val) {
  /*
   * TODO:
   *
   *	Well. should I use a mutex to provide the atomicity? I wonder
   *	four or more atomic operations costs more than takig a mutex
   *	lock or not.
   */

  if (likely(s != NULL)) {
    int64_t sum2;

    sum2 = val * val;

    (void)__sync_add_and_fetch(&(s->m_n), 1);
    (void)__sync_add_and_fetch(&(s->m_sum), val);
    (void)__sync_add_and_fetch(&(s->m_sum2), sum2);

    gallus_atomic_update_min(int64_t, &(s->m_min), LLONG_MAX, val);
    gallus_atomic_update_max(int64_t, &(s->m_max), LLONG_MIN, val);

    return GALLUS_RESULT_OK;
  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}


static inline gallus_result_t
s_get_n_stat(gallus_statistic_t s) {
  if (likely(s != NULL)) {
    return (gallus_result_t)(__sync_fetch_and_add(&(s->m_n), 0));
  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}


static inline gallus_result_t
s_get_min_stat(gallus_statistic_t s, int64_t *valptr) {
  if (likely(s != NULL && valptr != NULL)) {
    *valptr = (__sync_fetch_and_add(&(s->m_min), 0));
    return GALLUS_RESULT_OK;
  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}


static inline gallus_result_t
s_get_max_stat(gallus_statistic_t s, int64_t *valptr) {
  if (likely(s != NULL && valptr != NULL)) {
    *valptr = (__sync_fetch_and_add(&(s->m_max), 0));
    return GALLUS_RESULT_OK;
  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}


static inline gallus_result_t
s_get_avg_stat(gallus_statistic_t s, double *valptr) {
  if (likely(s != NULL && valptr != NULL)) {
    int64_t n = (int64_t)__sync_fetch_and_add(&(s->m_n), 0); 

    if (n > 0) {
      double sum = (double)__sync_fetch_and_add(&(s->m_sum), 0);
      *valptr = sum / (double)n;
    } else {
      *valptr = 0.0;
    }

    return GALLUS_RESULT_OK;

  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}


static inline gallus_result_t
s_get_sd_stat(gallus_statistic_t s, double *valptr, bool is_ssd) {
  if (likely(s != NULL && valptr != NULL)) {
    int64_t n = (int64_t)__sync_fetch_and_add(&(s->m_n), 0);

    if (n == 0) {
      *valptr = 0.0;
    } else {
      double sum = (double)__sync_fetch_and_add(&(s->m_sum), 0);
      double sum2 = (double)__sync_fetch_and_add(&(s->m_sum2), 0);
      double avg = sum / (double)n;
      double ssum = 
          sum2 -
          2.0 * avg * sum +
          avg * avg * (double)n;

      if (is_ssd == true) {
        if (n >= 2) {
          *valptr = sqrt((ssum / (double)(n - 1)));
        } else {
          *valptr = 0.0;
        }
      } else {
        if (n >= 1) {
          *valptr = sqrt(ssum / (double)n);
        } else {
          *valptr = 0.0;
        }
      }        
    }

    return GALLUS_RESULT_OK;

  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}





/*
 * Exported APIs
 */


gallus_result_t
gallus_statistic_create(gallus_statistic_t *sptr, const char *name) {
  return s_create_stat(sptr, name);
}


gallus_result_t
gallus_statistic_find(gallus_statistic_t *sptr, const char *name) {
  return s_find_stat(sptr, name);
}


void
gallus_statistic_destroy(gallus_statistic_t *sptr) {
  if (likely(sptr != NULL && *sptr != NULL)) {
    s_destroy_stat(*sptr, true);
  }
}


void
gallus_statistic_destroy_by_name(const char *name) {
  if (likely(IS_VALID_STRING(name) == true)) {
    gallus_statistic_t s = NULL;
    if (likely(s_find_stat(&s, name) == GALLUS_RESULT_OK &&
               s != NULL)) {
      s_destroy_stat(s, true);
    }
  }
}


gallus_result_t
gallus_statistic_record(gallus_statistic_t *sptr, int64_t val) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    ret = s_record_stat(*sptr, val);
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_statistic_reset(gallus_statistic_t *sptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    ret = s_reset_stat(*sptr);
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_statistic_sample_n(gallus_statistic_t *sptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    ret = s_get_n_stat(*sptr);
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_statistic_min(gallus_statistic_t *sptr, int64_t *valptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && valptr != NULL)) {
    ret = s_get_min_stat(*sptr, valptr);
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_statistic_max(gallus_statistic_t *sptr, int64_t *valptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && valptr != NULL)) {
    ret = s_get_max_stat(*sptr, valptr);
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_statistic_average(gallus_statistic_t *sptr, double *valptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && valptr != NULL)) {
    ret = s_get_avg_stat(*sptr, valptr);
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_statistic_sd(gallus_statistic_t *sptr, double *valptr, bool is_ssd) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && valptr != NULL)) {
    ret = s_get_sd_stat(*sptr, valptr, is_ssd);
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
