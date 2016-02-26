#include "gallus_apis.h"


static inline const char *
myname(const char *argv0) {
  const char *p = (const char *)strrchr(argv0, '/');
  if (p != NULL) {
    p++;
  } else {
    p = argv0;
  }
  return p;
}


int
main(int argc, const char *const argv[]) {
  const char *nm = myname(argv[0]);
  (void)argc;

  gallus_msg("this should emitted to stderr.\n");

  /*
   * log to stderr.
   */
  if (IS_GALLUS_RESULT_OK(
        gallus_log_initialize(GALLUS_LOG_EMIT_TO_UNKNOWN, NULL,
                               false, true, 1)) == false) {
    gallus_msg_fatal("what's wrong??\n");
    /* not reached. */
  }
  gallus_dprint("debug to stderr.\n");

  /*
   * log to file.
   */
  if (IS_GALLUS_RESULT_OK(
        gallus_log_initialize(GALLUS_LOG_EMIT_TO_FILE, "./testlog.txt",
                               false, true, 10)) == false) {
    gallus_msg_fatal("what's wrong??\n");
    /* not reached. */
  }
  gallus_dprint("debug to file.\n");
  gallus_msg_debug(5, "debug to file, again.\n");

  if (IS_GALLUS_RESULT_OK(
        gallus_log_initialize(GALLUS_LOG_EMIT_TO_SYSLOG, nm,
                               false, false, 10)) == false) {
    gallus_msg_fatal("what's wrong??\n");
    /* not reached. */
  }
  gallus_msg_debug(5, "debug to syslog.\n");

  /*
   * log to stderr, again.
   */
  if (IS_GALLUS_RESULT_OK(
        gallus_log_initialize(GALLUS_LOG_EMIT_TO_UNKNOWN, NULL,
                               false, true, 1)) == false) {
    gallus_msg_fatal("what's wrong??\n");
    /* not reached. */
  }
  gallus_dprint("will exit 1 ...\n");
  gallus_exit_error(1, "exit 1 on purpose.\n");

  return 0;
}
