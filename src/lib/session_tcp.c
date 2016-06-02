#include "gallus_apis.h"
#include "gallus_session.h"
#include "session_internal.h"

gallus_result_t session_tcp_init(gallus_session_t );

static ssize_t
read_tcp(gallus_session_t s, void *buf, size_t n) {
  return read(s->sock, buf, n);
}

static ssize_t
write_tcp(gallus_session_t s, void *buf, size_t n) {
  return write(s->sock, buf, n);
}

gallus_result_t
session_tcp_init(gallus_session_t s) {
  s->read = read_tcp;
  s->write = write_tcp;

  return GALLUS_RESULT_OK;
}
