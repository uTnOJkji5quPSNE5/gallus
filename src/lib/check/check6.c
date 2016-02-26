#include "gallus_apis.h"
#include "gallus_thread_internal.h"





#define NTHDS	8


static gallus_bbq_t s_q = NULL;
static gallus_thread_t s_thds[NTHDS] = { 0 };


static gallus_result_t
s_main(const gallus_thread_t *tptr, void *arg) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  (void)arg;

  if (tptr != NULL) {
    gallus_result_t r;
    void *val;

    while (true) {
      val = (void *)false;
      if ((r = gallus_bbq_get(&s_q, &val, bool, -1LL)) !=
          GALLUS_RESULT_OK) {
        if (r == GALLUS_RESULT_NOT_OPERATIONAL) {
          r = GALLUS_RESULT_OK;
        } else {
          gallus_perror(r);
        }
        break;
      }
    }

    ret = r;
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static void
s_finalize(const gallus_thread_t *tptr, bool is_canceled, void *arg) {
  (void)arg;
  (void)tptr;

  gallus_msg_debug(1, "called. %s.\n",
                    (is_canceled == false) ? "finished" : "canceled");
  if (is_canceled == true) {
    gallus_bbq_cancel_janitor(&s_q);
  }
}


int
main(int argc, const char *const argv[]) {
  int ret = 1;
  gallus_result_t r;

  (void)argc;
  (void)argv;

  /*
   * Following both two cases must succeeded.
   */

  /*
   * 1) Shutdown the queue makes threads exit normally.
   */
  if ((r = gallus_bbq_create(&s_q, bool, 20, NULL)) == GALLUS_RESULT_OK) {
    size_t i;

    for (i = 0; i < NTHDS; i++) {
      s_thds[i] = NULL;
      if ((r = gallus_thread_create(&s_thds[i],
                                     s_main,
                                     s_finalize,
                                     NULL,
                                     "getter", NULL)) != GALLUS_RESULT_OK) {
        gallus_perror(r);
        goto done;
      }
    }

    for (i = 0; i < NTHDS; i++) {
      if ((r = gallus_thread_start(&s_thds[i], false)) != GALLUS_RESULT_OK) {
        gallus_perror(r);
        goto done;
      }
    }

    sleep(1);

    /*
     * Shutdown the queue.
     */
    gallus_bbq_shutdown(&s_q, true);

    /*
     * Then threads exit. Delete all of them.
     */
    for (i = 0; i < NTHDS; i++) {
      if ((r = gallus_thread_wait(&s_thds[i], -1LL)) == GALLUS_RESULT_OK) {
        gallus_thread_destroy(&s_thds[i]);
      }
    }

    /*
     * Delete the queue.
     */
    gallus_bbq_destroy(&s_q, true);
  }

  /*
   * 2) Cancel all the threads and delete the queue.
   */
  s_q = NULL;
  if ((r = gallus_bbq_create(&s_q, bool, 20, NULL)) == GALLUS_RESULT_OK) {
    size_t i;

    for (i = 0; i < NTHDS; i++) {
      s_thds[i] = NULL;
      if ((r = gallus_thread_create(&s_thds[i],
                                     s_main,
                                     s_finalize,
                                     NULL,
                                     "getter", NULL)) != GALLUS_RESULT_OK) {
        gallus_perror(r);
        goto done;
      }
    }

    for (i = 0; i < NTHDS; i++) {
      if ((r = gallus_thread_start(&s_thds[i], false)) != GALLUS_RESULT_OK) {
        gallus_perror(r);
        goto done;
      }
    }

    sleep(1);

    /*
     * Cancel and destroy all the thread.
     */
    for (i = 0; i < NTHDS; i++) {
      if ((r = gallus_thread_cancel(&s_thds[i])) == GALLUS_RESULT_OK) {
        gallus_thread_destroy(&s_thds[i]);
      }
    }

    /*
     * Delete the queue.
     */
    gallus_bbq_destroy(&s_q, true);
  }

  ret = 0;

done:
  return ret;
}
