#include "unity.h"
#include "gallus_apis.h"
#include "gallus_thread_internal.h"

#define GET_TID_RETRY 10
#define N_ENTRY 100
#define Q_PUTGET_WAIT 10000000LL
#define N_LOOP 100
#define MAIN_SLEEP_USEC 2000LL
#define WAIT_NSEC 1000LL * 1000LL * 1000LL

typedef struct entry {
  int num;
} entry;

typedef GALLUS_BOUND_BLOCK_Q_DECL(test_bbq_t, entry *) test_bbq_t;

struct test_thread_record {
  gallus_thread_record m_thread;  /* must be on the head. */

  test_bbq_t m_queue;
  struct test_thread_record *m_dest_tt;

  int m_loop_times;
  useconds_t m_sleep_usec;
};

typedef struct test_thread_record *test_thread_t;


void setUp(void);
void tearDown(void);
static void s_qvalue_freeup(void **);
static bool
s_test_thread_create(test_thread_t *ttptr,
                     int n_entry,
                     int loop_times,
                     useconds_t sleep_usec,
                     test_thread_t dest_tt);
static gallus_result_t
s_thread_main(const gallus_thread_t *, void *);

static int s_entry_number = 0;

/*
 * setup & teardown. create/destroy BBQ.
 */
void
setUp(void) {
}

void
tearDown(void) {
}


/*
 * entry functions
 */
static entry *
s_new_entry(void) {
  entry *e = (entry *)malloc(sizeof(entry));
  if (e != NULL) {
    e->num = s_entry_number;
    s_entry_number++;
  }
  return e;
}

/*
 * bbq functions
 */
static void
s_qvalue_freeup(void **arg) {
  if (arg != NULL) {
    entry *eptr = (entry *)*arg;
    free(eptr);
  }
}

static gallus_result_t
s_put(test_bbq_t *qptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  entry *pptr = s_new_entry();
  ret = gallus_bbq_put(qptr, &pptr, entry *, Q_PUTGET_WAIT);
  gallus_msg_debug(10, "put %p %d ... %s\n",
                    pptr, (pptr == NULL) ? -1 : pptr->num,
                    gallus_error_get_string(ret));
  return ret;
}

static gallus_result_t
s_get(test_bbq_t *qptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  entry *gptr = NULL;
  ret = gallus_bbq_get(qptr, &gptr, entry *, Q_PUTGET_WAIT);
  gallus_msg_debug(10, "get %p %d ... %s\n",
                    gptr, (gptr == NULL) ? -1 : gptr->num,
                    gallus_error_get_string(ret));
  free(gptr);
  return ret;
}


/*
 * thread functions
 */
static gallus_result_t
s_thread_main(const gallus_thread_t *selfptr, void *arg) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  int i = 0;
  (void)arg;
  if (selfptr != NULL) {
    test_thread_t tt = (test_thread_t)*selfptr;
    if (tt != NULL) {
      for (i = 0; i < tt->m_loop_times; i++) {
        ret = s_get(&tt->m_queue);
        if (ret == GALLUS_RESULT_OK) {
          if (tt->m_dest_tt != NULL) {
            ret = s_put(&(tt->m_dest_tt->m_queue));
          } else {
            TEST_FAIL_MESSAGE("invalid thread");
          }
        }
        switch (ret) {
          case GALLUS_RESULT_OK:
          case GALLUS_RESULT_TIMEDOUT:
          case GALLUS_RESULT_NOT_OPERATIONAL:
            break;
          default:
            gallus_perror(ret);
            TEST_FAIL_MESSAGE("put error");
            break;
        }
        usleep(tt->m_sleep_usec);
      }
      ret = GALLUS_RESULT_OK;
      gallus_msg_debug(1, "done!\n");
    } else {
      TEST_FAIL_MESSAGE("invalid thread");
    }
  } else {
    TEST_FAIL_MESSAGE("invalid thread");
  }
  return ret;
}

static void
s_thread_finalize(const gallus_thread_t *selfptr,
                  bool is_canceled, void *arg) {
  (void)selfptr;
  (void)is_canceled;
  (void)arg;
  gallus_msg_debug(1, "finalize done\n");
}

static void
s_thread_freeup(const gallus_thread_t *selfptr, void *arg) {
  (void)arg;

  if (selfptr != NULL) {
    test_thread_t tt = (test_thread_t)*selfptr;
    if (tt != NULL) {
      gallus_bbq_destroy(&(tt->m_queue), true);
      tt->m_queue = NULL;
    }
  }
  gallus_msg_debug(1, "freeup done\n");
}


static inline bool
s_test_thread_create(test_thread_t *ttptr,
                     int n_entry,
                     int loop_times,
                     useconds_t sleep_usec,
                     test_thread_t dest_tt) {
  bool ret = false;
  gallus_result_t res = GALLUS_RESULT_ANY_FAILURES;

  if (ttptr != NULL) {
    /* init test_thread_t */
    test_thread_t tt;
    if (*ttptr == NULL) {
      tt = (test_thread_t)malloc(sizeof(*tt));
      if (tt == NULL) {
        goto done;
      }
    } else {
      tt = *ttptr;
    }
    /* init gallus_thread_t */
    res = gallus_thread_create((gallus_thread_t *)&tt,
                                s_thread_main,
                                s_thread_finalize,
                                s_thread_freeup,
                                "test", NULL);
    if (res != GALLUS_RESULT_OK) {
      gallus_perror(res);
      goto done;
    }
    gallus_thread_free_when_destroy((gallus_thread_t *)&tt);
    /* init bbq */
    res = gallus_bbq_create(&(tt->m_queue), entry *, n_entry, s_qvalue_freeup);
    if (res != GALLUS_RESULT_OK) {
      gallus_perror(res);
      free(tt);
      goto done;
    }
    /* init other */
    tt->m_loop_times = loop_times;
    tt->m_sleep_usec = sleep_usec;
    tt->m_dest_tt = dest_tt;
    *ttptr = tt;
    ret = true;
  }

done:
  return ret;
}


/*
 * checking thread
 */
static inline bool
s_check_is_canceled(test_thread_t *ttptr,
                    gallus_result_t require_ret,
                    bool require_result) {
  bool result = !require_result;
  gallus_result_t ret;
  int i;
  for (i = 0; i < GET_TID_RETRY; i++) {
    ret = gallus_thread_is_canceled((gallus_thread_t *)ttptr, &result);
    if (ret == require_ret) {
      break;
    }
  }
  if (ret == require_ret) {
    if (ret == GALLUS_RESULT_OK) {
      result = (result == require_result);
    } else {
      result = true;
    }
  } else {
    gallus_perror(ret);
    TEST_FAIL_MESSAGE("is_canceled failed");
    result = false;
  }
  return result;
}

static inline bool
s_check_get_thread_id(test_thread_t *ttptr, gallus_result_t require_ret) {
  bool result = false;
  gallus_result_t ret;
  pthread_t tid;
  int i;
  for (i = 0; i < GET_TID_RETRY; i++) {
    ret = gallus_thread_get_pthread_id((gallus_thread_t *)ttptr, &tid);
    if (ret == require_ret) {
      break;
    }
  }
  if (ret == require_ret) {
    if (ret == GALLUS_RESULT_OK) {
      TEST_ASSERT_NOT_EQUAL(tid, GALLUS_INVALID_THREAD);
    }
    result = true;
  } else {
    if (require_ret == GALLUS_RESULT_OK
        && ret == GALLUS_RESULT_ALREADY_HALTED) {
      gallus_msg_warning(
        "test requires GALLUS_RESULT_OK, but result is GALLUS_ALREADY_HALTED. "
        "Modify value of WAIT_NSEC and MAIN_SLEEP_USEC of thread_test.c, please try again.\n");
      result = true;
    } else {
      gallus_perror(ret);
      TEST_FAIL_MESSAGE("get_thread_id failed");
      result = false;
    }
  }
  return result;
}


static inline bool
s_check_start(test_thread_t *ttptr, gallus_result_t require_ret) {
  bool result = false;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  ret = gallus_thread_start((gallus_thread_t *)ttptr, false);
  if (ret == require_ret) {
    if (ret == GALLUS_RESULT_OK) {
      result = s_check_get_thread_id(ttptr, GALLUS_RESULT_OK);
    } else {
      result = true;
    }
  } else {
    gallus_perror(ret);
    TEST_FAIL_MESSAGE("start failed");
    result = false;
  }
  return result;
}

static inline bool
s_check_wait(test_thread_t *ttptr, gallus_result_t require_ret) {
  bool result = false;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  ret = gallus_thread_wait((gallus_thread_t *)ttptr, WAIT_NSEC);
  if (ret != require_ret) {
    gallus_perror(ret);
    TEST_FAIL_MESSAGE("wait failed");
    result = false;
  } else {
    result = true;
  }
  return result;
}

static inline bool
s_check_cancel(test_thread_t *ttptr,
               gallus_result_t require_ret) {
  bool result = false;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  ret = gallus_thread_cancel((gallus_thread_t *)ttptr);
  if (ret != require_ret) {
    s_check_is_canceled(ttptr, GALLUS_RESULT_OK, true);
    gallus_perror(ret);
    TEST_FAIL_MESSAGE("cancel failed");
    result = false;
  } else {
    result = true;
  }

  return result;
}



/*
 * tests
 */

void
test_creation(void) {
  test_thread_t tt1 = NULL, tt2 = NULL;
  if (s_test_thread_create(&tt1, N_ENTRY, N_LOOP, MAIN_SLEEP_USEC,
                           NULL) != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }
  if (s_test_thread_create(&tt2, N_ENTRY, N_LOOP, MAIN_SLEEP_USEC, tt1)
      != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }

destroy:
  gallus_thread_destroy((gallus_thread_t *)&tt1);
  gallus_thread_destroy((gallus_thread_t *)&tt2);
}

void
test_putget(void) {
  int i = 0;
  test_thread_t tt1 = NULL, tt2 = NULL;
  if (s_test_thread_create(&tt1, N_ENTRY, N_LOOP, MAIN_SLEEP_USEC, NULL)
      != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }
  if (s_test_thread_create(&tt2, N_ENTRY, N_LOOP, MAIN_SLEEP_USEC, tt1)
      != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }
  tt1->m_dest_tt = tt2;
  for (i = 0; i < 10; i++) {
    s_put(&tt1->m_queue);
    s_put(&tt2->m_queue);
  }

  if (s_check_start(&tt1, GALLUS_RESULT_OK) == false) {
    goto destroy;
  }
  if (s_check_start(&tt2, GALLUS_RESULT_OK) == false) {
    goto destroy;
  }

  usleep(MAIN_SLEEP_USEC * (N_LOOP - 3));

  if (s_check_wait(&tt1, GALLUS_RESULT_OK) == false) {
    goto destroy;
  }
  if (s_check_wait(&tt2, GALLUS_RESULT_OK) == false) {
    goto destroy;
  }

destroy:
  gallus_thread_destroy((gallus_thread_t *)&tt1);
  gallus_thread_destroy((gallus_thread_t *)&tt2);
}
