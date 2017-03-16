#include "gallus_apis.h"
#include "unity.h"





void
setUp(void) {
}


void
tearDown(void) {
}





void
test_alloc_free_cpu(void) {
  void *p = gallus_malloc_on_cpu(4096, 0);

  TEST_ASSERT_NOT_NULL(p);

  gallus_free_on_cpu(p);
}


void
test_alloc_free_node(void) {
  void *p = gallus_malloc_on_numanode(4096, 0);

  TEST_ASSERT_NOT_NULL(p);

  gallus_free_on_numanode(p);
}
