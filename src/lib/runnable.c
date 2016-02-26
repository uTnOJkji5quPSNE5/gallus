#include "gallus_apis.h"
#include "gallus_runnable_internal.h"





#define DEFAULT_RUNNABLE_ALLOC_SZ	(sizeof(gallus_runnable_record))





gallus_result_t
gallus_runnable_create(gallus_runnable_t *rptr,
                        size_t alloc_sz,
                        gallus_runnable_proc_t func,
                        void *arg,
                        gallus_runnable_freeup_proc_t freeup_func) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  bool is_heap_allocd = false;

  if (rptr != NULL &&
      func != NULL) {
    if (*rptr == NULL) {
      size_t sz = (DEFAULT_RUNNABLE_ALLOC_SZ > alloc_sz) ?
                  DEFAULT_RUNNABLE_ALLOC_SZ : alloc_sz;
      *rptr = (gallus_runnable_t)malloc(sz);
      if (*rptr != NULL) {
        is_heap_allocd = true;
      } else {
        ret = GALLUS_RESULT_NO_MEMORY;
        goto done;
      }
    }

    (*rptr)->m_func = func;
    (*rptr)->m_arg = arg;
    (*rptr)->m_freeup_func = freeup_func;
    (*rptr)->m_is_heap_allocd = is_heap_allocd;

    ret = GALLUS_RESULT_OK;

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

done:
  if (ret != GALLUS_RESULT_OK) {
    if (is_heap_allocd == true) {
      free((void *)(*rptr));
    }
    if (rptr != NULL) {
      *rptr = NULL;
    }
  }

  return ret;
}


void
gallus_runnable_destroy(gallus_runnable_t *rptr) {
  if (rptr != NULL) {
    if ((*rptr)->m_freeup_func != NULL) {
      ((*rptr)->m_freeup_func)(rptr);
    }
    if ((*rptr)->m_is_heap_allocd == true) {
      (void)free((void *)(*rptr));
    }
    *rptr = NULL;
  }
}


gallus_result_t
gallus_runnable_start(const gallus_runnable_t *rptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (rptr != NULL && *rptr != NULL) {
    ret = ((*rptr)->m_func)(rptr, (*rptr)->m_arg);
  }

  return ret;
}
