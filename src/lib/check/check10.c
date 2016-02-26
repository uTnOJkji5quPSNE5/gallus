#include "gallus_apis.h"





static volatile bool s_do_stop = false;


static void
s_pre_pause(const gallus_pipeline_stage_t *sptr) {
  (void)sptr;

  gallus_msg_debug(1, "called.\n");
}


static gallus_result_t
s_setup(const gallus_pipeline_stage_t *sptr) {
  (void)sptr;

  gallus_msg_debug(1, "called.\n");

  return GALLUS_RESULT_OK;
}


static gallus_result_t
s_fetch(const gallus_pipeline_stage_t *sptr,
        size_t idx, void *buf, size_t max) {
  (void)sptr;
  (void)idx;
  (void)buf;
  (void)max;

  return (s_do_stop == false) ? 1LL : 0LL;
}


static gallus_result_t
s_main(const gallus_pipeline_stage_t *sptr,
       size_t idx, void *buf, size_t n) {
  (void)sptr;
  (void)buf;

  gallus_msg_debug(1, "called " PFSZ(u) ".\n", idx);

  return (gallus_result_t)n;
}


static gallus_result_t
s_throw(const gallus_pipeline_stage_t *sptr,
        size_t idx, void *buf, size_t n) {
  (void)sptr;
  (void)idx;
  (void)buf;

  return (gallus_result_t)n;
}


static gallus_result_t
s_sched(const gallus_pipeline_stage_t *sptr,
        void *buf, size_t n, void *hint) {
  (void)sptr;
  (void)buf;
  (void)hint;

  return (gallus_result_t)n;
}


static gallus_result_t
s_shutdown(const gallus_pipeline_stage_t *sptr,
           shutdown_grace_level_t l) {
  (void)sptr;

  gallus_msg_debug(1, "called with \"%s\".\n",
                    (l == SHUTDOWN_RIGHT_NOW) ? "right now" : "gracefully");

  return GALLUS_RESULT_OK;
}


static void
s_finalize(const gallus_pipeline_stage_t *sptr,
           bool is_canceled) {
  (void)sptr;

  gallus_msg_debug(1, "%s.\n",
                    (is_canceled == false) ? "exit normally" : "canceled");
}


static void
s_freeup(const gallus_pipeline_stage_t *sptr) {
  (void)sptr;

  gallus_msg_debug(1, "called.\n");
}


static void
s_post_start(const gallus_pipeline_stage_t *sptr, size_t idx, void *arg) {
  (void)sptr;

  gallus_msg_debug(1, "called for " PFSZ(u) " with %p.\n", idx, arg);
}





int
main(int argc, const char *const argv[]) {
  gallus_result_t st = GALLUS_RESULT_ANY_FAILURES;
  gallus_pipeline_stage_t s = NULL;

  size_t nthd = 1;

  (void)argc;

  if (IS_VALID_STRING(argv[1]) == true) {
    size_t tmp;
    if (gallus_str_parse_uint64(argv[1], &tmp) == GALLUS_RESULT_OK &&
        tmp > 0LL) {
      nthd = tmp;
    }
  }

  fprintf(stdout, "Creating...\n");
  st = gallus_pipeline_stage_create(&s, 0, "a_test",
                                     nthd,
                                     sizeof(void *), 1024,
                                     s_pre_pause,
                                     s_sched,
                                     s_setup,
                                     s_fetch,
                                     s_main,
                                     s_throw,
                                     s_shutdown,
                                     s_finalize,
                                     s_freeup);
  if (st == GALLUS_RESULT_OK) {
    fprintf(stdout, "Created.\n");
    fprintf(stdout, "Setting up...\n");
    st = gallus_pipeline_stage_setup(&s);
    if (st == GALLUS_RESULT_OK) {
      fprintf(stdout, "Set up.\n");
      fprintf(stdout, "Starting...\n");
      st = gallus_pipeline_stage_start(&s);
      if (st == GALLUS_RESULT_OK) {
        fprintf(stdout, "Started.\n");
        sleep(1);
        fprintf(stdout, "Opening the front door...\n");
        st = global_state_set(GLOBAL_STATE_STARTED);
        if (st == GALLUS_RESULT_OK) {
          fprintf(stdout, "The front door is open.\n");
          sleep(1);
          fprintf(stdout, "Shutting down...\n");
          st = gallus_pipeline_stage_shutdown(&s, SHUTDOWN_GRACEFULLY);
          if (st == GALLUS_RESULT_OK) {
            fprintf(stdout, "Shutdown accepted.\n");
            sleep(1);
            s_do_stop = true;
            fprintf(stdout, "Waiting shutdown...\n");
            st = gallus_pipeline_stage_wait(&s, 1000LL * 1000LL * 1000LL);
            if (st == GALLUS_RESULT_OK) {
              fprintf(stdout, "OK, Shutdown.\n");
            }
          }
        }
      }
    }
  }

  if (st != GALLUS_RESULT_OK) {
    gallus_perror(st);
  }

  fprintf(stdout, "Destroying...\n");
  gallus_pipeline_stage_destroy(&s);
  fprintf(stdout, "Destroyed.\nn");

  return (st == GALLUS_RESULT_OK) ? 0 : 1;
}
