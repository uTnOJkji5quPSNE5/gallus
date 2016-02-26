#include "gallus_apis.h"





gallus_chrono_t
gallus_chrono_now(void) {
  gallus_chrono_t ret = 0;
  WHAT_TIME_IS_IT_NOW_IN_NSEC(ret);
  return ret;
}


gallus_result_t
gallus_chrono_to_timespec(struct timespec *dstptr,
                           gallus_chrono_t nsec) {
  if (dstptr != NULL) {
    NSEC_TO_TS(nsec, *dstptr);
    return GALLUS_RESULT_OK;
  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}


gallus_result_t
gallus_chrono_to_timeval(struct timeval *dstptr,
                          gallus_chrono_t nsec) {
  if (dstptr != NULL) {
    NSEC_TO_TV(nsec, *dstptr);
    return GALLUS_RESULT_OK;
  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}


gallus_result_t
gallus_chrono_from_timespec(gallus_chrono_t *dstptr,
                             const struct timespec *specptr) {
  if (dstptr != NULL &&
      specptr != NULL) {
    *dstptr = TS_TO_NSEC(*specptr);
    return GALLUS_RESULT_OK;
  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}


gallus_result_t
gallus_chrono_from_timeval(gallus_chrono_t *dstptr,
                            const struct timeval *specptr) {
  if (dstptr != NULL &&
      specptr != NULL) {
    *dstptr = TV_TO_NSEC(*specptr);
    return GALLUS_RESULT_OK;
  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}


gallus_result_t
gallus_chrono_nanosleep(gallus_chrono_t nsec,
                         gallus_chrono_t *remptr) {
  struct timespec t;
  struct timespec r;

  if (nsec < 0) {
    return GALLUS_RESULT_INVALID_ARGS;
  }

  NSEC_TO_TS(nsec, t);

  if (remptr == NULL) {
 retry:
    errno = 0;
    if (nanosleep(&t, &r) == 0) {
      return GALLUS_RESULT_OK;
    } else {
      if (errno == EINTR) {
        t = r;
        goto retry;
      } else {
        return GALLUS_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    errno = 0;
    if (nanosleep(&t, &r) == 0) {
      return GALLUS_RESULT_OK;
    } else {
      *remptr = TS_TO_NSEC(r);
      return GALLUS_RESULT_POSIX_API_ERROR;
    }
  }
}
