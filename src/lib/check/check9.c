#include "gallus_apis.h"





int
main(int argc, const char *const argv[]) {
  gallus_result_t n_tokens;
  char *tokens[1024];
  ssize_t i;

  const char *a =
    "\\\\"
    "'that\\'s what I want.'"
    "\"that's what I want, too\""
    "\"who is the \\\"bad\\\"\""
    ;
  char *x;
  char *y;
  gallus_result_t len;

  (void)argc;
  (void)argv;

  x = strdup(a);
  n_tokens = gallus_str_tokenize_quote(x, tokens, 1024, "\t\r\n ", "\"'");
  if (n_tokens > 0) {

    for (i = 0; i < n_tokens; i++) {
      fprintf(stderr, "<%s>\n", tokens[i]);
    }

    fprintf(stderr, "\n");

    for (i = 0; i < n_tokens; i++) {
      if ((len = gallus_str_unescape(tokens[i], "\"'", &y)) >= 0) {
        fprintf(stderr, "[%s]\n", y);
        free((void *)y);
      } else {
        gallus_perror(len);
      }
    }

  } else {
    gallus_perror(n_tokens);
  }

  fprintf(stderr, "\n");

  x = strdup(a);
  n_tokens = gallus_str_tokenize(x, tokens, 1024, "\t\r\n ");
  if (n_tokens > 0) {
    for (i = 0; i < n_tokens; i++) {
      fprintf(stderr, "<%s>\n", tokens[i]);
    }
  } else {
    gallus_perror(n_tokens);
  }

  if ((len = gallus_str_unescape("\"aaa nnn\"", "\"'", &y)) >= 0) {
    fprintf(stderr, "(%s)\n", y);
    free((void *)y);
  } else {
    gallus_perror(len);
  }

  return 0;
}
