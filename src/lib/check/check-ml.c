#include "gallus_apis.h"


int
main(int argc, const char * const argv[]) {
  size_t n_workers = 1;

  if (IS_VALID_STRING(argv[1]) == true) {
    size_t tmp = 1;
    gallus_result_t r = gallus_str_parse_uint64(argv[1], &tmp);
    if (r == GALLUS_RESULT_OK && tmp <= 4) {
      n_workers = tmp;
    }
  }

  (void)gallus_mainloop_set_callout_workers_number(n_workers);
  (void)gallus_mainloop_with_callout(argc, argv, NULL, NULL,
                                    false, false, true);
  (void)global_state_request_shutdown(SHUTDOWN_GRACEFULLY);
  gallus_mainloop_wait_thread();

  return 0;
}
