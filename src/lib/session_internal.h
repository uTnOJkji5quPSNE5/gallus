#ifndef __SESSION_INTERNAL_H__
#define __SESSION_INTERNAL_H__





#define SESSION_BUFSIZ 4096

struct session {
  /* socket descriptor */
  int sock;
  int family;
  int type;
  int protocol;
  session_type_t session_type;
  short events; /* for session_event_poll */
  short revents; /* for session_event_poll */
  struct session_buf {
    char *rp;
    char *ep;
    char buf[SESSION_BUFSIZ];
  } rbuf;
  /* function pointers */
  gallus_result_t (*connect)(gallus_session_t s, const char *host,
                              const char *port);
  gallus_result_t (*accept)(gallus_session_t s1, gallus_session_t *s2);
  ssize_t (*read)(gallus_session_t, void *, size_t);
  ssize_t (*write)(gallus_session_t, void *, size_t);
  void (*close)(gallus_session_t);
  void (*destroy)(gallus_session_t);
  gallus_result_t (*connect_check)(gallus_session_t);
  /* protocol depended context object */
  void *ctx;
};





#endif /* ! __SESSION_INTERNAL_H__ */

