#include "gallus_logger.h"

#ifdef perror
#undef perror
#endif /* perror */
#define perror(str)	gallus_msg_error("%s: %s\n", str, strerror(errno))
