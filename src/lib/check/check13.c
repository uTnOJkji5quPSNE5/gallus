#include "gallus_apis.h"


static gallus_result_t
s_main_proc(const gallus_thread_t *selfptr, void *arg) {
  volatile size_t a = 0;

  (void)selfptr;
  (void)arg;

  while (true) {
    a++;
  }

  return GALLUS_RESULT_OK;
}


int
main(int argc, char const *const argv[]) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  int cpu = 0;
  gallus_thread_t thd = NULL;

  if (argc > 1) {
    int tmp;
    if (gallus_str_parse_int32(argv[1], &tmp) == GALLUS_RESULT_OK &&
        tmp > 0) {
      cpu = tmp;
    }
  }

  if ((ret = gallus_thread_create(&thd,
                                   s_main_proc,
                                   NULL,
                                   NULL,
                                   (const char *) "thread",
                                   (void *) NULL)) == GALLUS_RESULT_OK) {
    if ((ret = gallus_thread_set_cpu_affinity(&thd,
               -1)) == GALLUS_RESULT_OK &&
        (ret = gallus_thread_set_cpu_affinity(&thd,
               cpu)) == GALLUS_RESULT_OK) {
      if ((ret = gallus_thread_start(&thd, false)) == GALLUS_RESULT_OK) {
        ret = gallus_thread_wait(&thd, -1LL);
      }
    }
  }

  if (ret != GALLUS_RESULT_OK) {
    gallus_perror(ret);
    return 1;
  } else {
    return 0;
  }
}
