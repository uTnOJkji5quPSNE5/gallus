#ifndef __GALLUS_RUNNABLE_INTERNAL_H__
#define __GALLUS_RUNNABLE_INTERNAL_H__





#include "gallus_runnable_funcs.h"





typedef struct gallus_runnable_record {
  gallus_runnable_proc_t m_func;
  void *m_arg;
  gallus_runnable_freeup_proc_t m_freeup_func;
  bool m_is_heap_allocd;
} gallus_runnable_record;





#endif /* ! __GALLUS_RUNNABLE_INTERNAL_H__ */
