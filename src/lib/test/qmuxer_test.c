#include "unity.h"
#include "gallus_apis.h"


void
setUp(void) {
}

void
tearDown(void) {
}


void
test_zero_polls(void) {
  gallus_result_t ret = gallus_qmuxer_poll(NULL, NULL, 0,
                         1000 * 1000 * 1000);
  TEST_ASSERT_EQUAL(ret, GALLUS_RESULT_TIMEDOUT);
}


void
test_null_polls(void) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_qmuxer_t qmx = NULL;

  ret = gallus_qmuxer_create(&qmx);
  TEST_ASSERT_EQUAL(ret, GALLUS_RESULT_OK);

  if (ret == GALLUS_RESULT_OK) {
    gallus_bbq_t q = NULL;
    ret = gallus_bbq_create(&q, uint32_t, 1000, NULL);
    TEST_ASSERT_EQUAL(ret, GALLUS_RESULT_OK);

    if (ret == GALLUS_RESULT_OK) {
      gallus_qmuxer_poll_t polls[1];
      ret = gallus_qmuxer_poll_create(&polls[0], q,
                                       GALLUS_QMUXER_POLL_READABLE);
      TEST_ASSERT_EQUAL(ret, GALLUS_RESULT_OK);

      if (ret == GALLUS_RESULT_OK) {

        ret = gallus_qmuxer_poll(&qmx, polls, 0, 1000 * 1000 * 1000);
        TEST_ASSERT_EQUAL(ret, GALLUS_RESULT_TIMEDOUT);

        ret = gallus_qmuxer_poll(&qmx, NULL, 1, 1000 * 1000 * 1000);
        TEST_ASSERT_EQUAL(ret, GALLUS_RESULT_TIMEDOUT);

        ret = gallus_qmuxer_poll(NULL, polls , 0, 1000 * 1000 * 1000);
        TEST_ASSERT_EQUAL(ret, GALLUS_RESULT_TIMEDOUT);

        ret = gallus_qmuxer_poll(NULL, NULL , 1, 1000 * 1000 * 1000);
        TEST_ASSERT_EQUAL(ret, GALLUS_RESULT_TIMEDOUT);

        gallus_qmuxer_poll_destroy(&polls[0]);
      }

      gallus_bbq_destroy(&q, true);
    }

    gallus_qmuxer_destroy(&qmx);
  }
}
