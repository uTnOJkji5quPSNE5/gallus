#include "gallus_apis.h"

static gallus_result_t
s_main_proc(gallus_thread_t *selfptr, void *arg) {
  gallus_msg("Called with (%p, %p)\n", *selfptr, arg);
  return GALLUS_RESULT_OK;
}


static gallus_result_t
s_main_proc_with_sleep(gallus_thread_t *selfptr, void *arg) {
  gallus_msg("Called with (%p, %p)\n", *selfptr, arg);
  sleep(10);
  return GALLUS_RESULT_OK;
}

static void
s_freeup_proc(gallus_thread_t *selfptr, void *arg) {
  gallus_msg("Called with (%p, %p)\n", *selfptr, arg);
}

static void
s_finalize_proc(gallus_thread_t *selfptr,
                bool is_canceled, void *arg) {
  gallus_msg("Called with (%p, %s, %p)\n",
              *selfptr, BOOL_TO_STR(is_canceled), arg);
}

static void
s_check_cancelled(gallus_thread_t *thread, bool require) {
  bool canceled = !require;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  if ((ret = gallus_thread_is_canceled(thread, &canceled))
      != GALLUS_RESULT_OK) {
    gallus_perror(ret);
    gallus_msg_error("Can't check is_cancelled.\n");
  } else if (canceled != require) {
    gallus_msg_error(
      "is_cancelled() required %s, but %s.\n",
      BOOL_TO_STR(require), BOOL_TO_STR(canceled));
  }
}

int
main(int argc, char const *const argv[]) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  int i;
  int n = 10000;
  gallus_thread_t thread = NULL;
  pthread_t tid = GALLUS_INVALID_THREAD;

  gallus_log_set_debug_level(10);

  if (argc > 1) {
    int tmp;
    if (gallus_str_parse_int32(argv[1], &tmp) == GALLUS_RESULT_OK &&
        tmp > 0) {
      n = tmp;
    }
  }

  /* create, start, wait, destroy */
  for (i = 0; i < n; i++) {
    thread = NULL;

    if ((ret = gallus_thread_create(
                 (gallus_thread_t *)&thread,
                 (gallus_thread_main_proc_t)s_main_proc,
                 (gallus_thread_finalize_proc_t)s_finalize_proc,
                 (gallus_thread_freeup_proc_t)s_freeup_proc,
                 (const char *) "thread",
                 (void *) NULL)) != GALLUS_RESULT_OK) {
      gallus_perror(ret);
      gallus_exit_fatal("Can't create a thread.\n");
    } else {
      fprintf(stderr, "create a thread [%d]\n", i);
      if ((ret = gallus_thread_get_pthread_id(&thread, &tid))
          != GALLUS_RESULT_NOT_STARTED) {
        gallus_perror(ret);
        goto done;
      }
    }

    if ((ret = gallus_thread_start(&thread, false)) != GALLUS_RESULT_OK) {
      gallus_perror(ret);
      gallus_exit_fatal("Can't start the thread.\n");
    } else {
      ret = gallus_thread_get_pthread_id(&thread, &tid);
      if (ret == GALLUS_RESULT_OK ||
          ret == GALLUS_RESULT_ALREADY_HALTED) {
        fprintf(stderr, " start thread:  %lu\n", tid);
      } else {
        gallus_perror(ret);
        goto done;
      }
    }

    s_check_cancelled(&thread, false);

    if ((ret = gallus_thread_wait(&thread, 1000LL * 1000LL * 1000LL)) !=
        GALLUS_RESULT_OK) {
      gallus_perror(ret);
      gallus_msg_error("Can't wait the thread.\n");
    } else {
      if ((ret = gallus_thread_get_pthread_id(&thread, &tid))
          == GALLUS_RESULT_OK) {
        gallus_msg_error(
          "Thread has been completed, but I can get thread ID, %lu\n",
          tid);
      } else if (ret != GALLUS_RESULT_ALREADY_HALTED) {
        gallus_perror(ret);
        goto done;
      }
    }

    s_check_cancelled(&thread, false);

    gallus_thread_destroy(&thread);
    fprintf(stderr, "destroy the thread [%d]\n", i);

    fprintf(stderr, "\n");
  }

  /* create, start, cancel, destroy */
  for (i = 0; i < n; i++) {
    thread = NULL;

    if ((ret = gallus_thread_create(
                 (gallus_thread_t *)&thread,
                 (gallus_thread_main_proc_t)s_main_proc_with_sleep,
                 (gallus_thread_finalize_proc_t)s_finalize_proc,
                 (gallus_thread_freeup_proc_t)s_freeup_proc,
                 (const char *) "thread",
                 (void *) NULL)) != GALLUS_RESULT_OK) {
      gallus_perror(ret);
      gallus_exit_fatal("Can't create a thread.\n");
    } else {
      fprintf(stderr, "create a thread [%d]\n", i);

      if ((ret = gallus_thread_get_pthread_id(&thread, &tid))
          != GALLUS_RESULT_NOT_STARTED) {
        gallus_perror(ret);
        goto done;
      }
    }

    if ((ret = gallus_thread_start(&thread, false)) != GALLUS_RESULT_OK) {
      gallus_perror(ret);
      gallus_exit_fatal("Can't start the thread.\n");
    } else {
      ret = gallus_thread_get_pthread_id(&thread, &tid);
      if (ret == GALLUS_RESULT_OK ||
          ret == GALLUS_RESULT_ALREADY_HALTED) {
        fprintf(stderr, " start thread:  %lu\n", tid);
      } else {
        gallus_perror(ret);
        goto done;
      }
    }

    s_check_cancelled(&thread, false);

    if ((ret = gallus_thread_cancel(&thread)) !=
        GALLUS_RESULT_OK) {
      gallus_perror(ret);
      gallus_msg_error("Can't cancel the thread.\n");
    } else {
      if ((ret = gallus_thread_get_pthread_id(&thread, &tid))
          == GALLUS_RESULT_OK) {
        gallus_msg_error(
          "Thread has been canceled, but I can get thread ID, %lu\n",
          tid);
      } else if (ret != GALLUS_RESULT_NOT_STARTED &&
                 ret != GALLUS_RESULT_ALREADY_HALTED) {
        gallus_perror(ret);
        goto done;
      }
    }

    s_check_cancelled(&thread, true);

    gallus_thread_destroy(&thread);
    fprintf(stderr, "destroy the thread [%d]\n", i);

    fprintf(stderr, "\n");
  }

  /* create, start, destroy */
  for (i = 0; i < n; i++) {
    thread = NULL;

    if ((ret = gallus_thread_create(
                 (gallus_thread_t *)&thread,
                 (gallus_thread_main_proc_t)s_main_proc_with_sleep,
                 (gallus_thread_finalize_proc_t)s_finalize_proc,
                 (gallus_thread_freeup_proc_t)s_freeup_proc,
                 (const char *) "thread",
                 (void *) NULL)) != GALLUS_RESULT_OK) {
      gallus_perror(ret);
      gallus_exit_fatal("Can't create a thread.\n");
    } else {
      fprintf(stderr, "create a thread [%d]\n", i);

      if ((ret = gallus_thread_get_pthread_id(&thread, &tid))
          != GALLUS_RESULT_NOT_STARTED) {
        gallus_perror(ret);
        goto done;
      }
    }

    if ((ret = gallus_thread_start(&thread, false)) != GALLUS_RESULT_OK) {
      gallus_perror(ret);
      gallus_exit_fatal("Can't start the thread.\n");
    } else {
      ret = gallus_thread_get_pthread_id(&thread, &tid);
      if (ret == GALLUS_RESULT_OK ||
          ret == GALLUS_RESULT_ALREADY_HALTED) {
        fprintf(stderr, " start thread:  %lu\n", tid);
      } else {
        gallus_perror(ret);
        goto done;
      }
    }

    gallus_thread_destroy(&thread);
    fprintf(stderr, "destroy the thread [%d]\n", i);

    fprintf(stderr, "\n");
  }

done:
  return (ret == GALLUS_RESULT_OK) ? 0 : 1;
}
