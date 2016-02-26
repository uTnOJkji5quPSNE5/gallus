#include "gallus_apis.h"
#include "gallus_thread_internal.h"


#


typedef struct {
  gallus_thread_record m_thd;	/* must be on the head. */

  gallus_mutex_t m_lock;

  bool m_is_operational;
  bool m_stop_requested;
  size_t m_n_data;
  int *m_data;
} test_thread_record;
typedef test_thread_record *test_thread_t;





static bool s_is_deleted = false;





static void
s_test_thread_finalize(const gallus_thread_t *tptr, bool is_canceled,
                       void *arg) {
  (void)arg;

  if (tptr != NULL) {
    test_thread_t tt = (test_thread_t)*tptr;

    if (tt != NULL) {
      gallus_mutex_lock(&(tt->m_lock));
      {
        tt->m_is_operational = false;
      }
      gallus_mutex_unlock(&(tt->m_lock));

      gallus_msg_debug(1, "enter (%s).\n",
                        (is_canceled == true) ? "canceled" : "exited");
    }
  }
}


static void
s_test_thread_destroy(const gallus_thread_t *tptr, void *arg) {
  (void)arg;

  if (tptr != NULL) {
    test_thread_t tt = (test_thread_t)*tptr;

    if (tt != NULL) {
      (void)gallus_mutex_lock(&(tt->m_lock));
      {
        gallus_msg_debug(1, "enter.\n");
        if (tt->m_is_operational == false) {
          gallus_msg_debug(1, "freeup.\n");
          free((void *)tt->m_data);
          tt->m_data = NULL;
        }
        s_is_deleted = true;
      }
      (void)gallus_mutex_unlock(&(tt->m_lock));
    }

    gallus_mutex_destroy(&(tt->m_lock));
  }
}


static gallus_result_t
s_test_thread_main(const gallus_thread_t *tptr, void *arg) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  (void)arg;
  if (tptr != NULL) {
    test_thread_t tt = (test_thread_t)*tptr;

    if (tt != NULL) {
      while (tt->m_stop_requested == false) {
        gallus_msg_debug(1, "will sleep a sec...\n");
        (void)gallus_chrono_nanosleep(1000 * 1000 * 1000, NULL);
      }
      ret = GALLUS_RESULT_OK;
    }
  }

  return ret;
}


static inline bool
s_initialize(test_thread_t tt, size_t n) {
  bool ret = false;

  if (tt != NULL) {
    tt->m_data = (int *)malloc(n * sizeof(int));
    if (tt->m_data != NULL) {
      gallus_result_t r;
      if ((r = gallus_mutex_create(&(tt->m_lock))) != GALLUS_RESULT_OK) {
        gallus_perror(r);
        goto initfailure;
      }
      if ((r = gallus_thread_create((gallus_thread_t *)&tt,
                                     s_test_thread_main,
                                     s_test_thread_finalize,
                                     s_test_thread_destroy,
                                     "test", NULL)) != GALLUS_RESULT_OK) {
        gallus_perror(r);
        goto initfailure;
      }
      tt->m_n_data = n;
      tt->m_is_operational = false;
      tt->m_stop_requested = false;

      ret = true;
    }
  initfailure:
    if (ret != true) {
      free((void *)tt->m_data);
    }
  }

  return ret;
}





static inline bool
test_thread_create(test_thread_t *ttptr, size_t n) {
  bool ret = false;

  if (ttptr != NULL) {
    test_thread_t tt;
    if (*ttptr == NULL) {
      tt = (test_thread_t)malloc(sizeof(*tt));
      if (tt == NULL) {
        goto done;
      }
    } else {
      tt = *ttptr;
    }
    ret = s_initialize(tt, n);
    if (*ttptr == NULL && ret == true) {
      *ttptr = tt;
      (void)gallus_thread_free_when_destroy((gallus_thread_t *)&tt);
    }
  }

done:
  return ret;
}


static inline void
test_thread_request_stop(test_thread_t tt) {
  if (tt != NULL) {
    (void)gallus_mutex_lock(&(tt->m_lock));
    {
      tt->m_stop_requested = true;
    }
    (void)gallus_mutex_unlock(&(tt->m_lock));
  }
}





int
main(int argc, const char *const argv[]) {
  test_thread_record ttr;
  test_thread_t tt;
  bool is_canceled;
  int ret = 1;

  (void)argc;
  (void)argv;

  fprintf(stderr, "Pthread wrapper check: start\n\n");

  /*
   * Heap object
   */
  fprintf(stderr, "Heap object, regular: start\n");
  tt = NULL;
  if (test_thread_create(&tt, 100) != true) {
    gallus_exit_fatal("Can't create a test thread object.\n");
  }
  if (gallus_thread_start((gallus_thread_t *)&tt, false) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't start a thread.\n");
  }
  gallus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  test_thread_request_stop(tt);
  if (gallus_thread_wait((gallus_thread_t *)&tt, -1) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't wait the thread.\n");
  }
  gallus_thread_destroy((gallus_thread_t *)&tt);
  fprintf(stderr, "Heap object, regular: end\n\n");

  /*
   * Stack object
   */
  fprintf(stderr, "Stack object, regular: start\n");
  tt = &ttr;
  if (test_thread_create(&tt, 100) != true) {
    gallus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (gallus_thread_start((gallus_thread_t *)&tt, false) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't start a thread.\n");
  }
  gallus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  test_thread_request_stop(tt);
  if (gallus_thread_wait((gallus_thread_t *)&tt, -1) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't wait the thread.\n");
  }
  gallus_thread_destroy((gallus_thread_t *)&tt);
  fprintf(stderr, "Stack object, regular: end\n\n");

  /*
   * Cancel
   */
  fprintf(stderr, "Stack object, cancel: start\n");
  tt = &ttr;
  if (test_thread_create(&tt, 100) != true) {
    gallus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (gallus_thread_start((gallus_thread_t *)&tt, false) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't start a thread.\n");
  }
  gallus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  if (gallus_thread_cancel((gallus_thread_t *)&tt) != GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't cancel the thread.\n");
  }
  if (gallus_thread_wait((gallus_thread_t *)&tt, -1) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't wait the thread.\n");
  }
  if (gallus_thread_is_canceled((gallus_thread_t *)&tt, &is_canceled) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("The returned value must be \"canceled\".\n");
  }
  gallus_thread_destroy((gallus_thread_t *)&tt);
  fprintf(stderr, "Stack object, cancel: end\n\n");

  /*
   * Heap object auto deletion
   */
  fprintf(stderr, "Heap object, auto deletion: start\n");
  s_is_deleted = false;
  tt = NULL;
  if (test_thread_create(&tt, 100) != true) {
    gallus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (gallus_thread_start((gallus_thread_t *)&tt, true) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't start a thread.\n");
  }
  gallus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  test_thread_request_stop(tt);
  gallus_chrono_nanosleep(2LL * 1000LL * 1000LL * 1000LL, NULL);
  if (s_is_deleted == false) {
    gallus_exit_fatal("The thread object must be freed and "
                       "the flag must be true.\n");
  }
  fprintf(stderr, "Heap object, auto deletion: end\n\n");

  /*
   * Stack object auto deletion
   */
  fprintf(stderr, "Stack object, auto deletion: start\n");
  s_is_deleted = false;
  tt = &ttr;
  if (test_thread_create(&tt, 100) != true) {
    gallus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (gallus_thread_start((gallus_thread_t *)&tt, true) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't start a thread.\n");
  }
  gallus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  test_thread_request_stop(tt);
  gallus_chrono_nanosleep(2LL * 1000LL * 1000LL * 1000LL, NULL);
  if (s_is_deleted == false) {
    gallus_exit_fatal("The thread object must be freed and "
                       "the flag must be true.\n");
  }
  fprintf(stderr, "Stack object, auto deletion: end\n\n");

  /*
   * Heap object cancel, auto deletion
   */
  fprintf(stderr, "Heap object, cancel, auto deletion: start\n");
  s_is_deleted = false;
  tt = NULL;
  if (test_thread_create(&tt, 100) != true) {
    gallus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (gallus_thread_start((gallus_thread_t *)&tt, true) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't start a thread.\n");
  }
  gallus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  if (gallus_thread_cancel((gallus_thread_t *)&tt) != GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't cancel the thread.\n");
  }
  gallus_chrono_nanosleep(2LL * 1000LL * 1000LL * 1000LL, NULL);
  if (s_is_deleted == false) {
    gallus_exit_fatal("The thread object must be freed and "
                       "the flag must be true.\n");
  }
  fprintf(stderr, "Heap object, cancel, auto deletion: end\n\n");

  /*
   * Stack object cancel, auto deletion
   */
  fprintf(stderr, "Stack object, cancel, auto deletion: start\n");
  s_is_deleted = false;
  tt = &ttr;
  if (test_thread_create(&tt, 100) != true) {
    gallus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (gallus_thread_start((gallus_thread_t *)&tt, true) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't start a thread.\n");
  }
  gallus_chrono_nanosleep(5LL * 1000LL * 1000LL * 1000LL, NULL);
  if (gallus_thread_cancel((gallus_thread_t *)&tt) != GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't cancel the thread.\n");
  }
  gallus_chrono_nanosleep(2LL * 1000LL * 1000LL * 1000LL, NULL);
  if (s_is_deleted == false) {
    gallus_exit_fatal("The thread object must be freed and "
                       "the flag must be true.\n");
  }
  fprintf(stderr, "Stack object, cancel, auto deletion: end\n\n");

  /*
   * Timed wait
   */
  fprintf(stderr, "Timed wait: start\n");
  tt = &ttr;
  if (test_thread_create(&tt, 100) != true) {
    gallus_exit_fatal("Can't initialize a test thread object.\n");
  }
  if (gallus_thread_start((gallus_thread_t *)&tt, false) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't start a thread.\n");
  }
  if (gallus_thread_wait((gallus_thread_t *)&tt,
                          5LL * 1000LL * 1000LL * 1000LL) !=
      GALLUS_RESULT_TIMEDOUT) {
    gallus_exit_fatal("Must be timed out.\n");
  }
  test_thread_request_stop(tt);
  if (gallus_thread_wait((gallus_thread_t *)&tt, -1) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't wait the thread.\n");
  }
  gallus_thread_destroy((gallus_thread_t *)&tt);
  fprintf(stderr, "Timed wait: end\n\n");

  /*
   * Force destroy
   */
  fprintf(stderr, "Force destroy: start\n");
  s_is_deleted = false;
  tt = NULL;
  if (test_thread_create(&tt, 100) != true) {
    gallus_exit_fatal("Can't create a test thread object.\n");
  }
  if (gallus_thread_start((gallus_thread_t *)&tt, false) !=
      GALLUS_RESULT_OK) {
    gallus_exit_fatal("Can't start a thread.\n");
  }
  gallus_thread_destroy((gallus_thread_t *)&tt);
  if (s_is_deleted == false) {
    gallus_exit_fatal("The thread object must be freed and "
                       "the flag must be true.\n");
  }
  fprintf(stderr, "Force destroy: end\n\n");

  /*
   * Force destroy, not started
   */
  fprintf(stderr, "Force destroy, not started: start\n");
  s_is_deleted = false;
  tt = NULL;
  if (test_thread_create(&tt, 100) != true) {
    gallus_exit_fatal("Can't create a test thread object.\n");
  }
  gallus_thread_destroy((gallus_thread_t *)&tt);
  if (s_is_deleted == false) {
    gallus_exit_fatal("The thread object must be freed and "
                       "the flag must be true.\n");
  }
  fprintf(stderr, "Force destroy, not started: end\n\n");

  ret = 0;
  fprintf(stderr, "Pthread wrapper check: end\n");

  return ret;
}
