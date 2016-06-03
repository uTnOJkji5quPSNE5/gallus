#include "gallus_apis.h"





#include "ecode.c"





static ssize_t s_n_error_strs =
  sizeof(s_error_strs) / sizeof(const char *);





const char *
gallus_error_get_string(gallus_result_t r) {
  if (r < 0) {
    ssize_t ar = -r;
    if (ar < s_n_error_strs) {
      return s_error_strs[ar];
    } else {
      return "??? error string out of range ???";
    }
  } else if (r == 0) {
    return s_error_strs[0];
  } else {
    return "Greater than zero, means not errors??";
  }
}

