#include "gallus_apis.h"


#


static void
s_block_forever(void) {
  do {
    (void)select(0, NULL, NULL, NULL, NULL);
  } while (true);
}


static void
s_lag_sighandler(int sig) {
  (void)sig;
  gallus_msg_info("called.\n");
}


static void
s_posix_sighandler(int sig) {
  (void)sig;
  gallus_msg_info("called.\n");
}


static void
s_signal_quit(int sig) {
  (void)sig;
  gallus_msg_info("\n\ncalled. bye.\n\n");
  exit(0);
}





int
main(int argc, const char *const argv[]) {
  pid_t pid;
  (void)argv;

  (void)gallus_signal(SIGHUP, s_lag_sighandler, NULL);
  (void)gallus_signal(SIGINT, s_signal_quit, NULL);

  (void)signal(SIGHUP, s_posix_sighandler);
  (void)signal(SIGINT, s_signal_quit);

  if (argc > 1) {
    gallus_signal_old_school_semantics();
  }

  pid = fork();
  if (pid == 0) {
    s_block_forever();
  } else if (pid > 0) {
    int st;
    (void)waitpid(pid, &st, 0);
    return 0;
  } else {
    perror("fork");
    return 1;
  }
}
