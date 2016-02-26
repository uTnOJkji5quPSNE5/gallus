#ifndef __TEST_LIB_UTIL_H__
#define __TEST_LIB_UTIL_H__

#ifndef LOOP_MAX
#define LOOP_MAX 100
#endif /* LOOP_MAX */

#ifndef SLEEP
#define SLEEP usleep(0.1 * 1000 * 1000)
#endif /* SLEEP */

#define TEST_ASSERT_COUNTER(b, c, msg) {                              \
    int _i;                                                           \
    for (_i= 0; _i < LOOP_MAX; _i++) {                                \
      bool _ret = check_called_func_count(c);                         \
      if (_ret == b) {                                                \
        break;                                                        \
      }                                                               \
      SLEEP;                                                          \
    }                                                                 \
    if (_i == LOOP_MAX) {                                             \
      TEST_FAIL_MESSAGE(msg);                                         \
    }                                                                 \
  }

enum func_type {
  PIPELINE_FUNC_PRE_PAUSE,
  PIPELINE_FUNC_SCHED,
  PIPELINE_FUNC_SETUP,
  PIPELINE_FUNC_FETCH,
  PIPELINE_FUNC_MAIN,
  PIPELINE_FUNC_THROW,
  PIPELINE_FUNC_SHUTDOWN,
  PIPELINE_FUNC_FINALIZE,
  PIPELINE_FUNC_FREEUP,
  PIPELINE_FUNC_MAINT,

  PIPELINE_FUNC_MAX,
};

#endif /* ! __TEST_LIB_UTIL_H__ */
