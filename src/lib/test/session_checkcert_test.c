#include "gallus_apis.h"
#include "../session_tls.c"
#include "unity.h"





void
setUp(void) {
}


void
tearDown(void) {
}





void
test_load_failure(void) {
  gallus_result_t rc;

  gallus_session_tls_set_trust_point_conf("./not yet exists.");
  rc = s_check_certificates("aaa", "bbb");
  TEST_ASSERT_EQUAL(rc, GALLUS_RESULT_NOT_FOUND);
}


void
test_load(void) {
  gallus_result_t rc;

  gallus_session_tls_set_trust_point_conf("./test-load.conf");

  rc = s_check_certificates("aaa", "bbb");
  TEST_ASSERT_EQUAL(rc, GALLUS_RESULT_NOT_ALLOWED);
}


void
test_deny_selfsigned(void) {
  gallus_result_t rc = s_check_certificates("same", "same");
  TEST_ASSERT_EQUAL(rc, GALLUS_RESULT_NOT_ALLOWED);
}


void
test_match_full(void) {
  const char *issuer =
    "/C=JP/ST=Somewhere/L=Somewhere/O=Somewhere/OU=Somewhere/CN=Middle "
    "of Nowhere's tier I CA/emailAddress=ca@example.net";
  const char *subject =
    "/C=JP/ST=Somewhere/L=Somewhere/O=Somewhere/OU=Somewhere/CN=John "
    "\"774\" Doe/emailAddress=john.774.doe@example.net";

  gallus_result_t rc = s_check_certificates(issuer, subject);
  TEST_ASSERT_EQUAL(rc, GALLUS_RESULT_OK);
}


void
test_match_regex_allow_issuer(void) {
  gallus_result_t rc = s_check_certificates("aaa/CN="
                        "The Evil Genius CA",
                        NULL);
  TEST_ASSERT_EQUAL(rc, GALLUS_RESULT_OK);
}


void
test_match_regex_deny_subject(void) {
  gallus_result_t rc;

  gallus_session_tls_set_trust_point_conf("./test-regex.conf");

  rc = s_check_certificates(NULL,
                            "aaa/CN=The Evil Genius hehehe");
  TEST_ASSERT_EQUAL(rc, GALLUS_RESULT_NOT_ALLOWED);
}


void
test_match_regex_allow_self_signed(void) {
  gallus_result_t rc;
  const char *self = "/C=JP/CN=John \"774\" Doe";

  rc = s_check_certificates(self, self);
  TEST_ASSERT_EQUAL(rc, GALLUS_RESULT_OK);

  rc = s_check_certificates(self, NULL);
  TEST_ASSERT_EQUAL(rc, GALLUS_RESULT_NOT_ALLOWED);

  rc = s_check_certificates(NULL, self);
  TEST_ASSERT_EQUAL(rc, GALLUS_RESULT_NOT_ALLOWED);
}

