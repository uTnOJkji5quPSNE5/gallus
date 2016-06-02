#include "gallus_apis.h"





#define ONE_SEC		1000LL * 1000LL * 1000LL
#define REQ_TIMEDOUT	ONE_SEC





static volatile bool s_got_term_sig = false;


static void
s_term_handler(int sig) {
  gallus_result_t r = GALLUS_RESULT_ANY_FAILURES;
  global_state_t gs = GLOBAL_STATE_UNKNOWN;

  if ((r = global_state_get(&gs)) == GALLUS_RESULT_OK) {

    if ((int)gs >= (int)GLOBAL_STATE_STARTED) {

      shutdown_grace_level_t l = SHUTDOWN_UNKNOWN;
      if (sig == SIGTERM || sig == SIGINT) {
        l = SHUTDOWN_GRACEFULLY;
      } else if (sig == SIGQUIT) {
        l = SHUTDOWN_RIGHT_NOW;
      }
      if (IS_VALID_SHUTDOWN(l) == true) {
        gallus_msg_debug(5, "About to request shutdown(%s)...\n",
                          (l == SHUTDOWN_RIGHT_NOW) ?
                          "RIGHT_NOW" : "GRACEFULLY");
        if ((r = global_state_request_shutdown(l)) == GALLUS_RESULT_OK) {
          gallus_msg_debug(5, "the shutdown request accepted.\n");
        } else {
          gallus_perror(r);
          gallus_msg_error("can't request shutdown.\n");
        }
      }

    } else {
      if (sig == SIGTERM || sig == SIGINT || sig == SIGQUIT) {
        s_got_term_sig = true;
      }
    }

  }

}


static void
s_hup_handler(int sig) {
  (void)sig;
  gallus_msg_debug(5, "called. it's dummy for now.\n");
}


static gallus_chrono_t s_to = -1LL;


static inline void
usage(FILE *fd) {
  fprintf(fd, "usage:\n");
  fprintf(fd, "\t--help|-?\tshow this.\n");
  fprintf(fd, "\t-to #\t\tset shutdown timeout (in sec.).\n");
  gallus_module_usage_all(fd);
  (void)fflush(fd);
}


static void
parse_args(int argc, const char *const argv[]) {
  (void)argc;

  while (*argv != NULL) {
    if (strcmp("--help", *argv) == 0 ||
        strcmp("-?", *argv) == 0) {
      usage(stderr);
      exit(0);
    } else if (strcmp("-to", *argv) == 0) {
      if (IS_VALID_STRING(*(argv + 1)) == true) {
        int64_t tmp;
        argv++;
        if (gallus_str_parse_int64(*argv, &tmp) == GALLUS_RESULT_OK) {
          if (tmp >= 0) {
            s_to = ONE_SEC * tmp;
          }
        } else {
          fprintf(stderr, "can't parse \"%s\" as a number.\n", *argv);
          exit(1);
        }
      } else {
        fprintf(stderr, "A timeout # is not specified.\n");
        exit(1);
      }
    }
    argv++;
  }
}


int
main(int argc, const char *const argv[]) {
  gallus_result_t st = GALLUS_RESULT_ANY_FAILURES;

  (void)gallus_signal(SIGHUP, s_hup_handler, NULL);
  (void)gallus_signal(SIGINT, s_term_handler, NULL);
  (void)gallus_signal(SIGTERM, s_term_handler, NULL);
  (void)gallus_signal(SIGQUIT, s_term_handler, NULL);

  parse_args(argc - 1, argv + 1);

  if (s_to > 0) {
    if ((st = gallus_mainloop_set_shutdown_check_interval(s_to)) !=
        GALLUS_RESULT_OK) {
      gallus_perror(st);
      goto done;
    }
  }

  st = gallus_mainloop(argc, argv, NULL, NULL, false, false, false);

done:
  return (st == GALLUS_RESULT_OK) ? 0 : 1;
}
