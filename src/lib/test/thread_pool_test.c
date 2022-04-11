#include "gallus_apis.h"
#include "unity.h"
#include "gallus_task_internal.h"





typedef struct test_task_record {
  gallus_task_record task_;
  unsigned int sleep_time_;
} test_task_record;
typedef test_task_record *test_task_t;


static gallus_result_t
s_task_main(gallus_task_t *tptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  test_task_t t = NULL;

  gallus_msg_debug(1, "called.\n");

  if (likely(tptr != NULL && (t = (test_task_t)*tptr) != NULL)) {
    if (t->sleep_time_ > 0) {
      gallus_msg_debug(1, "sleep %d sec...\n", t->sleep_time_);
      sleep(t->sleep_time_);
      gallus_msg_debug(1, "sleep %d sec. done.\n", t->sleep_time_);
    }
    ret = 10LL;
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static void
s_task_finalize(gallus_task_t *tptr, bool is_cancelled) {
  (void)tptr;

  gallus_msg_debug(1, "called, %s.\n",
                (is_cancelled == true) ? "cancelled" : "clean exit");
}


static void
s_task_freeup(gallus_task_t *tptr) {
  (void)tptr;

  gallus_msg_debug(1, "called.\n");
}


static gallus_result_t
s_create_test_task(test_task_t *tptr, const char *name,
                   unsigned int sleep_time) {
  gallus_result_t ret = gallus_task_create((gallus_task_t *)tptr,
                                     sizeof(test_task_record),
                                     name,
                                     s_task_main,
                                     s_task_finalize,
                                     s_task_freeup);
  if (likely(ret == GALLUS_RESULT_OK)) {
    (*tptr)->sleep_time_ = sleep_time;
  }

  return ret;
}


static void
s_destroy_test_task(test_task_t *tptr) {
  gallus_task_destroy((gallus_task_t *)tptr);
}





void
setUp(void) {
  fflush(stdout);
  fflush(stderr);
  (void)global_state_set(GLOBAL_STATE_STARTED);  
}


void
tearDown(void) {
  fflush(stdout);
  fflush(stderr);
}





void
test_call_task_sync_single_wait_2sec(void) {
  gallus_thread_pool_t p = NULL;
  gallus_thread_pool_t p1 = NULL;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  test_task_t tsk = NULL;
  gallus_pooled_thread_t thd = NULL;
  const char *name = "test pool";

  ret = gallus_thread_pool_create(&p, name, 2);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "pool create error.");

  ret = gallus_thread_pool_get(name, &p1);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "get error.");
  TEST_ASSERT_EQUAL_MESSAGE(p, p1, 
                            "get (compare) error.");
  
  ret = s_create_test_task(&tsk, "test XX", 2);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "task create error.");

  ret = gallus_thread_pool_acquire_thread(&p, &thd, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "thread acquisition error.");

  ret = gallus_thread_pool_get_outstanding_thread_num(&p);
  TEST_ASSERT_EQUAL_MESSAGE(1, ret,
                            "pooled thread num error after acquisition.");

  ret = gallus_task_run((gallus_task_t *)&tsk, &thd,
                     GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "task start error.");

  ret = gallus_task_wait((gallus_task_t *)&tsk, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "task wait error.");

  ret = gallus_task_get_exit_code((gallus_task_t *)&tsk);
  TEST_ASSERT_EQUAL_MESSAGE(10, ret,
                            "exit code error.");

  ret = gallus_thread_pool_get_outstanding_thread_num(&p);
  TEST_ASSERT_EQUAL_MESSAGE(2, ret,
                            "pooled thread num error after release.");

  s_destroy_test_task(&tsk);

  gallus_thread_pool_destroy(&p);
}


void
test_call_task_sync_single_no_wait(void) {
  gallus_thread_pool_t p = NULL;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  test_task_t tsk = NULL;
  gallus_pooled_thread_t thd = NULL;

  ret = gallus_thread_pool_create(&p, "test pool", 2);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "pool create error.");

  ret = s_create_test_task(&tsk, "test XX", 0);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "task create error.");

  ret = gallus_thread_pool_acquire_thread(&p, &thd, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "thread acquisition error.");

  ret = gallus_thread_pool_get_outstanding_thread_num(&p);
  TEST_ASSERT_EQUAL_MESSAGE(1, ret,
                            "pooled thread num error after acquisition.");

  ret = gallus_task_run((gallus_task_t *)&tsk, &thd,
                     GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "task start error.");

  ret = gallus_task_wait((gallus_task_t *)&tsk, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "task wait error.");

  ret = gallus_task_get_exit_code((gallus_task_t *)&tsk);
  TEST_ASSERT_EQUAL_MESSAGE(10, ret,
                            "exit code error.");

  ret = gallus_thread_pool_get_outstanding_thread_num(&p);
  TEST_ASSERT_EQUAL_MESSAGE(2, ret,
                            "pooled thread num error after release.");

  s_destroy_test_task(&tsk);

  gallus_thread_pool_destroy(&p);
}


void
test_call_task_sync_multi_wait_2sec(void) {
  gallus_thread_pool_t p = NULL;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  test_task_t tsk = NULL;
  gallus_pooled_thread_t thd = NULL;

  ret = gallus_thread_pool_create(&p, "test pool", 1);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "pool create error.");

  /* 1 */
  ret = s_create_test_task(&tsk, "test XX", 2);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "task create error.");

  ret = gallus_thread_pool_acquire_thread(&p, &thd, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "thread acquisition error.");

  ret = gallus_thread_pool_get_outstanding_thread_num(&p);
  TEST_ASSERT_EQUAL_MESSAGE(0, ret,
                            "pooled thread num error after acquisition.");

  ret = gallus_task_run((gallus_task_t *)&tsk, &thd,
                     GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "task start error.");

  ret = gallus_task_wait((gallus_task_t *)&tsk, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "task wait error.");

  ret = gallus_task_get_exit_code((gallus_task_t *)&tsk);
  TEST_ASSERT_EQUAL_MESSAGE(10, ret,
                            "exit code error.");

  ret = gallus_thread_pool_get_outstanding_thread_num(&p);
  TEST_ASSERT_EQUAL_MESSAGE(1, ret,
                            "pooled thread num error after release.");

  s_destroy_test_task(&tsk);

  /* 2 */
  ret = s_create_test_task(&tsk, "test XX", 2);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "task create error.");

  ret = gallus_thread_pool_acquire_thread(&p, &thd, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "thread acquisition error.");

  ret = gallus_thread_pool_get_outstanding_thread_num(&p);
  TEST_ASSERT_EQUAL_MESSAGE(0, ret,
                            "pooled thread num error after acquisition.");

  ret = gallus_task_run((gallus_task_t *)&tsk, &thd,
                     GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "task start error.");

  ret = gallus_task_wait((gallus_task_t *)&tsk, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "task wait error.");

  ret = gallus_task_get_exit_code((gallus_task_t *)&tsk);
  TEST_ASSERT_EQUAL_MESSAGE(10, ret,
                            "exit code error.");

  ret = gallus_thread_pool_get_outstanding_thread_num(&p);
  TEST_ASSERT_EQUAL_MESSAGE(1, ret,
                            "pooled thread num error after release.");

  s_destroy_test_task(&tsk);


  gallus_thread_pool_destroy(&p);
}


void
test_call_task_sync_multi_no_wait(void) {
  gallus_thread_pool_t p = NULL;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  test_task_t tsk = NULL;
  gallus_pooled_thread_t thd = NULL;

  ret = gallus_thread_pool_create(&p, "test pool", 1);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "pool create error.");

  /* 1 */
  ret = s_create_test_task(&tsk, "test XX", 0);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "task create error.");

  ret = gallus_thread_pool_acquire_thread(&p, &thd, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "thread acquisition error.");

  ret = gallus_thread_pool_get_outstanding_thread_num(&p);
  TEST_ASSERT_EQUAL_MESSAGE(0, ret,
                            "pooled thread num error after acquisition.");

  ret = gallus_task_run((gallus_task_t *)&tsk, &thd,
                     GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "task start error.");

  ret = gallus_task_wait((gallus_task_t *)&tsk, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "task wait error.");

  ret = gallus_task_get_exit_code((gallus_task_t *)&tsk);
  TEST_ASSERT_EQUAL_MESSAGE(10, ret,
                            "exit code error.");

  ret = gallus_thread_pool_get_outstanding_thread_num(&p);
  TEST_ASSERT_EQUAL_MESSAGE(1, ret,
                            "pooled thread num error after release.");

  s_destroy_test_task(&tsk);

  /* 2 */
  ret = s_create_test_task(&tsk, "test XX", 0);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "task create error.");

  ret = gallus_thread_pool_acquire_thread(&p, &thd, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "thread acquisition error.");

  ret = gallus_thread_pool_get_outstanding_thread_num(&p);
  TEST_ASSERT_EQUAL_MESSAGE(0, ret,
                            "pooled thread num error after acquisition.");

  ret = gallus_task_run((gallus_task_t *)&tsk, &thd,
                     GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "task start error.");

  ret = gallus_task_wait((gallus_task_t *)&tsk, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "task wait error.");

  ret = gallus_task_get_exit_code((gallus_task_t *)&tsk);
  TEST_ASSERT_EQUAL_MESSAGE(10, ret,
                            "exit code error.");

  ret = gallus_thread_pool_get_outstanding_thread_num(&p);
  TEST_ASSERT_EQUAL_MESSAGE(1, ret,
                            "pooled thread num error after release.");

  s_destroy_test_task(&tsk);


  gallus_thread_pool_destroy(&p);
}


void
test_call_task_async_multi_non_block_wait_2sec(void) {
  size_t n = 2;

  gallus_thread_pool_t p = NULL;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  test_task_t tsks[n];
  gallus_pooled_thread_t thd = NULL;
  size_t i;
  char buf[32];

  ret = gallus_thread_pool_create(&p, "test pool", n);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "pool create error.");

  for (i = 0; i < n; i++) {
    tsks[i] = NULL;
    snprintf(buf, sizeof(buf), "test %ld ", i);
    ret = s_create_test_task(&tsks[i], buf, 2);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                              "task create error.");
  }

  for (i = 0; i < n; i++) {
    ret = gallus_thread_pool_acquire_thread(&p, &thd, -1LL);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                              "thread acquisition error.");
    ret = gallus_task_run((gallus_task_t *)&tsks[i], &thd,
		       GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
			      "task start error.");
  }

  for (i = 0; i < n; i++) {
    ret = gallus_task_wait((gallus_task_t *)&tsks[i], -1LL);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                              "task wait error.");
    ret = gallus_task_get_exit_code((gallus_task_t *)&tsks[i]);
    TEST_ASSERT_EQUAL_MESSAGE(10, ret,
                            "exit code error.");
    s_destroy_test_task(&tsks[i]);
  }

  gallus_thread_pool_destroy(&p);
}


void
test_call_task_async_multi_non_block_no_wait(void) {
  size_t n = 2;

  gallus_thread_pool_t p = NULL;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  test_task_t tsks[n];
  gallus_pooled_thread_t thd = NULL;
  size_t i;
  char buf[32];

  ret = gallus_thread_pool_create(&p, "test pool", n);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "pool create error.");

  for (i = 0; i < n; i++) {
    tsks[i] = NULL;
    snprintf(buf, sizeof(buf), "test %ld ", i);
    ret = s_create_test_task(&tsks[i], buf, 0);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                              "task create error.");
  }

  for (i = 0; i < n; i++) {
    ret = gallus_thread_pool_acquire_thread(&p, &thd, -1LL);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                              "thread acquisition error.");
    ret = gallus_task_run((gallus_task_t *)&tsks[i], &thd,
		       GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
			      "task start error.");
  }

  for (i = 0; i < n; i++) {
    ret = gallus_task_wait((gallus_task_t *)&tsks[i], -1LL);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                              "task wait error.");
    ret = gallus_task_get_exit_code((gallus_task_t *)&tsks[i]);
    TEST_ASSERT_EQUAL_MESSAGE(10, ret,
                            "exit code error.");
    s_destroy_test_task(&tsks[i]);
  }

  gallus_thread_pool_destroy(&p);
}


void
test_call_task_async_multi_block_wait_2sec(void) {
  size_t n = 2;

  gallus_thread_pool_t p = NULL;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  test_task_t tsks[n];
  gallus_pooled_thread_t thd = NULL;
  size_t i;
  char buf[32];

  ret = gallus_thread_pool_create(&p, "test pool", n - 1);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "pool create error.");

  for (i = 0; i < n; i++) {
    tsks[i] = NULL;
    snprintf(buf, sizeof(buf), "test %ld ", i);
    ret = s_create_test_task(&tsks[i], buf, 2);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                              "task create error.");
  }

  for (i = 0; i < n; i++) {
    ret = gallus_thread_pool_acquire_thread(&p, &thd, -1LL);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                              "thread acquisition error.");
    ret = gallus_task_run((gallus_task_t *)&tsks[i], &thd,
		       GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
			      "task start error.");
  }

  for (i = 0; i < n; i++) {
    ret = gallus_task_wait((gallus_task_t *)&tsks[i], -1LL);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                              "task wait error.");
    ret = gallus_task_get_exit_code((gallus_task_t *)&tsks[i]);
    TEST_ASSERT_EQUAL_MESSAGE(10, ret,
                            "exit code error.");
    s_destroy_test_task(&tsks[i]);
  }

  gallus_thread_pool_destroy(&p);
}


void
test_call_task_async_multi_block_no_wait(void) {
  size_t n = 2;

  gallus_thread_pool_t p = NULL;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  test_task_t tsks[n];
  gallus_pooled_thread_t thd = NULL;
  size_t i;
  char buf[32];
    
  ret = gallus_thread_pool_create(&p, "test pool", n - 1);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "pool create error.");

  for (i = 0; i < n; i++) {
    tsks[i] = NULL;
    snprintf(buf, sizeof(buf), "test %ld ", i);
    ret = s_create_test_task(&tsks[i], buf, 0);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                              "task create error.");
  }

  for (i = 0; i < n; i++) {
    ret = gallus_thread_pool_acquire_thread(&p, &thd, -1LL);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                              "thread acquisition error.");
    ret = gallus_task_run((gallus_task_t *)&tsks[i], &thd,
		       GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
			      "task start error.");
  }

  for (i = 0; i < n; i++) {
    ret = gallus_task_wait((gallus_task_t *)&tsks[i], -1LL);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                              "task wait error.");
    ret = gallus_task_get_exit_code((gallus_task_t *)&tsks[i]);
    TEST_ASSERT_EQUAL_MESSAGE(10, ret,
                            "exit code error.");
    s_destroy_test_task(&tsks[i]);
  }

  gallus_thread_pool_destroy(&p);
}





void
test_shutdown(void) {
  size_t n = 4;

  gallus_thread_pool_t p = NULL;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  test_task_t tsks[n];
  gallus_pooled_thread_t thd = NULL;
  size_t i;
  char buf[32];
  gallus_task_state_t s;

#if 0
  gallus_log_set_debug_level(5);
#endif

  ret = gallus_thread_pool_create(&p, "test pool", n);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "pool create error.");

  for (i = 0; i < n; i++) {
    tsks[i] = NULL;
    snprintf(buf, sizeof(buf), "test %ld ", i);
    ret = s_create_test_task(&tsks[i], buf, 2);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                              "task create error.");
  }

  for (i = 0; i < n; i++) {
    ret = gallus_thread_pool_acquire_thread(&p, &thd, -1LL);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                              "thread acquisition error.");
    ret = gallus_task_run((gallus_task_t *)&tsks[i], &thd,
		       GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
			      "task start error.");
  }

  ret = gallus_thread_pool_shutdown_all(&p, SHUTDOWN_RIGHT_NOW, 1000LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                            "pool shutdown error.");

  for (i = 0; i < n; i++) {
    ret = gallus_task_wait((gallus_task_t *)&tsks[i], -1LL);
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                              "task wait error.");
    ret = gallus_task_get_exit_code((gallus_task_t *)&tsks[i]);
    if (ret == GALLUS_RESULT_NOT_STARTED) {
      ret = 10;
    }
    TEST_ASSERT_EQUAL_MESSAGE(10, ret,
                            "exit code error.");
    ret = gallus_task_get_state((gallus_task_t *)&tsks[i], &s);
    switch (s) {
      case GALLUS_TASK_STATE_CLEAN_FINISHED:
      case GALLUS_TASK_STATE_HALTED:
      case GALLUS_TASK_STATE_RUNNER_SHUTDOWN:
        ret = GALLUS_RESULT_OK;
        break;
      default:
        ret = GALLUS_RESULT_ANY_FAILURES;
    }
    TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                              "task state error.");

    s_destroy_test_task(&tsks[i]);
  }

  gallus_thread_pool_destroy(&p);
}


void
test_cancel(void) {
  char *e;
  if (IS_VALID_STRING((e = getenv("INIHIBIT_CANCEL_TEST"))) == true) {
    if (strcasecmp(e, "yes") == 0 ||
        strcasecmp(e, "true") == 0 ||
        strcasecmp(e, "1") == 0) {
      TEST_ASSERT_EQUAL(1, 1);
    } else {
      size_t n = 4;

      gallus_thread_pool_t p = NULL;
      gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
      test_task_t tsks[n];
      gallus_pooled_thread_t thd = NULL;
      size_t i;
      char buf[32];
      gallus_task_state_t s;

#if 0
      gallus_log_set_debug_level(5);
#endif

      ret = gallus_thread_pool_create(&p, "test pool", n);
      TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                                "pool create error.");

      for (i = 0; i < n; i++) {
        tsks[i] = NULL;
        snprintf(buf, sizeof(buf), "test %ld ", i);
        ret = s_create_test_task(&tsks[i], buf, 2);
        TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                                  "task create error.");
      }

      for (i = 0; i < n; i++) {
        ret = gallus_thread_pool_acquire_thread(&p, &thd, -1LL);
        TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                                  "thread acquisition error.");
        ret = gallus_task_run((gallus_task_t *)&tsks[i], &thd,
                           GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC);
        TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                                  "task start error.");
      }

      ret = gallus_thread_pool_cancel_all(&p);
      TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret, 
                                "pool cancellation error.");

      for (i = 0; i < n; i++) {
        ret = gallus_task_wait((gallus_task_t *)&tsks[i], -1LL);
        TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                                  "task wait error.");
        ret = gallus_task_get_exit_code((gallus_task_t *)&tsks[i]);
        if (ret == GALLUS_RESULT_NOT_STARTED) {
          ret = 10;
        }
        TEST_ASSERT_EQUAL_MESSAGE(10, ret,
                                  "exit code error.");
        ret = gallus_task_get_state((gallus_task_t *)&tsks[i], &s);
        switch (s) {
          case GALLUS_TASK_STATE_CLEAN_FINISHED:
          case GALLUS_TASK_STATE_HALTED:
          case GALLUS_TASK_STATE_CANCELLED:
          case GALLUS_TASK_STATE_RUNNER_SHUTDOWN:
            ret = GALLUS_RESULT_OK;
            break;
          default:
            ret = GALLUS_RESULT_ANY_FAILURES;
        }
        TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                                  "task state error.");

        s_destroy_test_task(&tsks[i]);
      }

      gallus_thread_pool_destroy(&p);
    }
  }
}
