#include "unity.h"
#include "gallus_apis.h"
#include "gallus_pipeline_stage.h"

#define LOOP_MAX 100
#define SLEEP usleep(0.1 * 1000 * 1000)

#include "test_lib_util.h"

#define THREAD_NUM 2

static uint64_t called_counter[THREAD_NUM][PIPELINE_FUNC_MAX];
static uint64_t save_called_counter[THREAD_NUM][PIPELINE_FUNC_MAX];
static volatile bool do_stop = false;
static gallus_mutex_t lock = NULL;

#define PWAIT_TIME_OUT (-1LL)

static inline void
p_lock(void) {
  if (lock != NULL) {
    gallus_mutex_lock(&lock);
  }
}

static inline void
p_unlock(void) {
  if (lock != NULL) {
    gallus_mutex_unlock(&lock);
  }
}

static void
called_counter_reset(void) {
  p_lock();
  memset(&called_counter, 0, sizeof(called_counter));
  p_unlock();
}

static void
save_called_counter_reset(void) {
  p_lock();
  memset(&save_called_counter, 0, sizeof(save_called_counter));
  p_unlock();
}

static void
called_func_count(enum func_type type, size_t *idx) {
  int i;

  p_lock();
  if (idx == NULL) {
    for (i = 0; i < THREAD_NUM; i++) {
      called_counter[i][type]++;
    }
  } else {
    switch (type) {
      case PIPELINE_FUNC_FETCH:
      case PIPELINE_FUNC_MAIN:
      case PIPELINE_FUNC_THROW:
        if (called_counter[*idx][type] != 0) {
          goto done;
        }
        break;
      default:
        break;
    }
    called_counter[*idx][type]++;
  }

done:
  p_unlock();
}

static bool
check_called_func_count(uint64_t (*counter)[PIPELINE_FUNC_MAX]) {
  bool ret = false;
  int i;

  p_lock();
  for (i = 0; i < THREAD_NUM; i++) {
    if (memcmp(&called_counter[i], &counter[i],
               sizeof(called_counter[i])) != 0) {
      ret = false;
      goto done;
    }
  }
  ret = true;

done:
  p_unlock();
  return ret;
}

static void
pipeline_pre_pause(const gallus_pipeline_stage_t *sptr) {
  (void)sptr;

  gallus_msg_debug(1, "called.\n");
  called_func_count(PIPELINE_FUNC_PRE_PAUSE, NULL);
}

static gallus_result_t
pipeline_setup(const gallus_pipeline_stage_t *sptr) {
  (void)sptr;

  gallus_msg_debug(1, "called.\n");
  called_func_count(PIPELINE_FUNC_SETUP, NULL);

  return GALLUS_RESULT_OK;
}

static gallus_result_t
pipeline_fetch(const gallus_pipeline_stage_t *sptr,
               size_t idx, void *buf, size_t max) {
  (void)sptr;
  (void)idx;
  (void)buf;
  (void)max;

  gallus_msg_debug(1, "called.\n");
  called_func_count(PIPELINE_FUNC_FETCH, &idx);

  return (do_stop == false) ? 1LL : 0LL;
}

static gallus_result_t
pipeline_main(const gallus_pipeline_stage_t *sptr,
              size_t idx, void *buf, size_t n) {
  (void)sptr;
  (void)buf;

  gallus_msg_debug(1, "called " PFSZ(u) ".\n", idx);

  called_func_count(PIPELINE_FUNC_MAIN, &idx);

  return (gallus_result_t)n;
}

static gallus_result_t
pipeline_throw(const gallus_pipeline_stage_t *sptr,
               size_t idx, void *buf, size_t n) {
  (void)sptr;
  (void)idx;
  (void)buf;

  gallus_msg_debug(1, "called.\n");
  called_func_count(PIPELINE_FUNC_THROW, &idx);

  return (gallus_result_t)n;
}

static gallus_result_t
pipeline_sched(const gallus_pipeline_stage_t *sptr,
               void *buf, size_t n, void *hint) {
  (void)sptr;
  (void)buf;
  (void)hint;

  gallus_msg_debug(1, "called.\n");
  called_func_count(PIPELINE_FUNC_SCHED, NULL);

  return (gallus_result_t)n;
}

static gallus_result_t
pipeline_shutdown(const gallus_pipeline_stage_t *sptr,
                  shutdown_grace_level_t l) {
  (void)sptr;

  gallus_msg_debug(1, "called with \"%s\".\n",
                    (l == SHUTDOWN_RIGHT_NOW) ? "right now" : "gracefully");
  called_func_count(PIPELINE_FUNC_SHUTDOWN, NULL);

  return GALLUS_RESULT_OK;
}

static void
pipeline_finalize(const gallus_pipeline_stage_t *sptr,
                  bool is_canceled) {
  (void)sptr;

  gallus_msg_debug(1, "%s.\n",
                    (is_canceled == false) ? "exit normally" : "canceled");
  called_func_count(PIPELINE_FUNC_FINALIZE, NULL);
}

static void
pipeline_freeup(const gallus_pipeline_stage_t *sptr) {
  (void)sptr;

  gallus_msg_debug(1, "called.\n");
  called_func_count(PIPELINE_FUNC_FREEUP, NULL);
}

static void
check_no_call_fmt(void) {
  uint64_t counter_zero[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  };

  /* save current counter. */
  memcpy(save_called_counter, called_counter, sizeof(save_called_counter));
  TEST_ASSERT_COUNTER(true, save_called_counter,
                      "save_called_counter error.");
  /* reset counter. */
  called_counter_reset();

  SLEEP;

  /* check no call fetch/main/throw func. */
  TEST_ASSERT_COUNTER(true, counter_zero,
                      "counter_zero error.");
  /* store current counter. */
  memcpy(called_counter, save_called_counter, sizeof(called_counter));
}

static gallus_result_t
pipeline_maint(const gallus_pipeline_stage_t *sptr, void *arg) {
  (void)sptr;
  (void)arg;

  gallus_msg_debug(1, "called.\n");
  called_func_count(PIPELINE_FUNC_MAINT, NULL);
  check_no_call_fmt();

  return GALLUS_RESULT_OK;
}

static void
pipeline_stage_create(gallus_pipeline_stage_t *sptr,
                      gallus_pipeline_stage_pre_pause_proc_t pre_pause_proc,
                      gallus_pipeline_stage_sched_proc_t sched_proc,
                      gallus_pipeline_stage_setup_proc_t setup_proc,
                      gallus_pipeline_stage_fetch_proc_t fetch_proc,
                      gallus_pipeline_stage_main_proc_t main_proc,
                      gallus_pipeline_stage_throw_proc_t throw_proc,
                      gallus_pipeline_stage_shutdown_proc_t shutdown_proc,
                      gallus_pipeline_stage_finalize_proc_t finalize_proc,
                      gallus_pipeline_stage_freeup_proc_t freeup_proc) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  ret = gallus_pipeline_stage_create(sptr, 0,
                                      "test_gallus_pipeline_stage_create",
                                      THREAD_NUM,
                                      sizeof(void *), 1024,
                                      pre_pause_proc,
                                      sched_proc,
                                      setup_proc,
                                      fetch_proc,
                                      main_proc,
                                      throw_proc,
                                      shutdown_proc,
                                      finalize_proc,
                                      freeup_proc);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_create error.");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, *sptr,
                                "stage is NULL.");
}

static void
pipeline_stage_create_start(gallus_pipeline_stage_t *sptr,
                            gallus_pipeline_stage_pre_pause_proc_t pre_pause_proc,
                            gallus_pipeline_stage_sched_proc_t sched_proc,
                            gallus_pipeline_stage_setup_proc_t setup_proc,
                            gallus_pipeline_stage_fetch_proc_t fetch_proc,
                            gallus_pipeline_stage_main_proc_t main_proc,
                            gallus_pipeline_stage_throw_proc_t throw_proc,
                            gallus_pipeline_stage_shutdown_proc_t shutdown_proc,
                            gallus_pipeline_stage_finalize_proc_t finalize_proc,
                            gallus_pipeline_stage_freeup_proc_t freeup_proc) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  pipeline_stage_create(sptr,
                        pre_pause_proc,
                        sched_proc,
                        setup_proc,
                        fetch_proc,
                        main_proc,
                        throw_proc,
                        shutdown_proc,
                        finalize_proc,
                        freeup_proc);

  ret = gallus_pipeline_stage_start(sptr);
  SLEEP;
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_start error.");

  ret = global_state_set(GLOBAL_STATE_STARTED);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "global_state_set error.");
  SLEEP;
}

void
setUp(void) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  do_stop = false;
  called_counter_reset();
  save_called_counter_reset();

  ret = gallus_mutex_create(&lock);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_mutex_create error.");
}

void
tearDown(void) {
  gallus_mutex_destroy(&lock);
  lock = NULL;
}

void
test_setup_start_pause_resume_shutdown_destroy(void) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_pipeline_stage_t stage = NULL;
  uint64_t counter_setup[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
  };
  uint64_t counter_start[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
  };
  uint64_t counter_shutdown[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0},
  };
  uint64_t counter_destroy[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
  };

  /* create */
  pipeline_stage_create(&stage,
                        NULL,
                        pipeline_sched,
                        pipeline_setup,
                        pipeline_fetch,
                        pipeline_main,
                        pipeline_throw,
                        pipeline_shutdown,
                        pipeline_finalize,
                        pipeline_freeup);

  /* setup */
  ret = gallus_pipeline_stage_setup(&stage);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_setup error.");
  TEST_ASSERT_COUNTER(true, counter_setup,
                      "counter_setup error.");
  /* reset counter. */
  called_counter_reset();

  /* start */
  ret = gallus_pipeline_stage_start(&stage);
  SLEEP;
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_start error.");
  ret = global_state_set(GLOBAL_STATE_STARTED);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "global_state_set error.");
  SLEEP;
  TEST_ASSERT_COUNTER(true, counter_start,
                      "counter_start error.");

  /* pause */
  ret = gallus_pipeline_stage_pause(&stage, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_pause error.");
  SLEEP;
  /* check no call */
  check_no_call_fmt();

  /* resume */
  ret = gallus_pipeline_stage_resume(&stage);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_resume error.");
  SLEEP;
  /* check called fetch/main/throw func. */
  TEST_ASSERT_COUNTER(true, counter_start,
                      "counter_start error.");

  /* shutdown */
  ret = gallus_pipeline_stage_shutdown(&stage, SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_shutdown error.");
  do_stop = true;

  /* wait */
  ret = gallus_pipeline_stage_wait(&stage, PWAIT_TIME_OUT);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_wait error.");
  TEST_ASSERT_COUNTER(true, counter_shutdown,
                      "counter_shutdown error.");

  /* reset counter. */
  called_counter_reset();

  /* destroy */
  gallus_pipeline_stage_destroy(&stage);
  TEST_ASSERT_COUNTER(true, counter_destroy,
                      "counter_destroy error.");
}

void
test_setup_start_pause_resume_shutdown_destroy_2(void) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_pipeline_stage_t stage = NULL;
  uint64_t counter_setup[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
  };
  uint64_t counter_start[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
  };
  uint64_t counter_pre_pause[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {1, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 0, 0, 0, 0},
  };
  uint64_t counter_shutdown[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0},
  };
  uint64_t counter_destroy[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
  };

  /* create */
  pipeline_stage_create(&stage,
                        pipeline_pre_pause,
                        pipeline_sched,
                        pipeline_setup,
                        pipeline_fetch,
                        pipeline_main,
                        pipeline_throw,
                        pipeline_shutdown,
                        pipeline_finalize,
                        pipeline_freeup);

  /* setup */
  ret = gallus_pipeline_stage_setup(&stage);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_setup error.");
  TEST_ASSERT_COUNTER(true, counter_setup,
                      "counter_setup error.");
  /* reset counter. */
  called_counter_reset();

  /* start */
  ret = gallus_pipeline_stage_start(&stage);
  SLEEP;
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_start error.");
  ret = global_state_set(GLOBAL_STATE_STARTED);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "global_state_set error.");
  SLEEP;
  TEST_ASSERT_COUNTER(true, counter_start,
                      "counter_start error.");

  /* pause */
  ret = gallus_pipeline_stage_pause(&stage, -1LL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_pause error.");
  SLEEP;
  /* check called pre_pause. */
  TEST_ASSERT_COUNTER(true, counter_pre_pause,
                      "counter_pre_pause error.");

  /* reset counter. */
  called_counter_reset();

  /* resume */
  ret = gallus_pipeline_stage_resume(&stage);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_resume error.");
  SLEEP;
  /* check called fetch/main/throw func. */
  TEST_ASSERT_COUNTER(true, counter_start,
                      "counter_start error.");

  /* shutdown */
  ret = gallus_pipeline_stage_shutdown(&stage, SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_shutdown error.");
  do_stop = true;

  /* wait */
  ret = gallus_pipeline_stage_wait(&stage, PWAIT_TIME_OUT);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_wait error.");
  TEST_ASSERT_COUNTER(true, counter_shutdown,
                      "counter_shutdown error.");

  /* reset counter. */
  called_counter_reset();

  /* destroy */
  gallus_pipeline_stage_destroy(&stage);
  TEST_ASSERT_COUNTER(true, counter_destroy,
                      "counter_destroy error.");
}

void
test_maintenance(void) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_pipeline_stage_t stage = NULL;
  uint64_t counter_start[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
  };
  uint64_t counter_maint[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {1, 0, 0, 1, 1, 1, 0, 0, 0, 1},
    {1, 0, 0, 1, 1, 1, 0, 0, 0, 1},
  };
  uint64_t counter_shutdown[THREAD_NUM][PIPELINE_FUNC_MAX] = {
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0},
  };

  /* create data. */
  pipeline_stage_create_start(&stage,
                              pipeline_pre_pause,
                              pipeline_sched,
                              pipeline_setup,
                              pipeline_fetch,
                              pipeline_main,
                              pipeline_throw,
                              pipeline_shutdown,
                              pipeline_finalize,
                              pipeline_freeup);

  ret = gallus_pipeline_stage_schedule_maintenance(&stage, pipeline_maint,
        NULL);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_schedule_maintenance error.");
  TEST_ASSERT_COUNTER(true, counter_maint,
                      "counter_maint error.");

  /* reset counter. */
  called_counter_reset();
  SLEEP;

  /* check called fetch/main/throw func. */
  TEST_ASSERT_COUNTER(true, counter_start,
                      "counter_start error.");

  ret = gallus_pipeline_stage_shutdown(&stage, SHUTDOWN_GRACEFULLY);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_shutdown error.");

  do_stop = true;
  ret = gallus_pipeline_stage_wait(&stage, PWAIT_TIME_OUT);
  TEST_ASSERT_EQUAL_MESSAGE(GALLUS_RESULT_OK, ret,
                            "gallus_pipeline_stage_wait error.");
  TEST_ASSERT_COUNTER(true, counter_shutdown,
                      "counter_shutdown error.");

  gallus_pipeline_stage_destroy(&stage);
}
