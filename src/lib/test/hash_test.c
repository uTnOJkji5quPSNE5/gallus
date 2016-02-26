#include "gallus_apis.h"
#include "unity.h"

#define TEST_ASSERT_EQUAL_GALLUS_STATUS(expected, result)\
  TEST_ASSERT_EQUAL_INT((expected), (result))

#define TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(expected, result, message)\
  TEST_ASSERT_EQUAL_INT_MESSAGE((expected), (result), (message))

#define TEST_ASSERT_NOT_EQUAL_GALLUS_STATUS_MESSAGE(expected, result, message)\
  TEST_ASSERT_NOT_EQUAL_INT_MESSAGE((expected), (result), (message))

typedef struct {
  uint64_t content;
} entry;

static void
delete_entry(void *p) {
  if (p != NULL) {
    entry *e = (entry *)p;
    free(e);
  }
}

static inline entry *
new_entry(uint64_t content) {
  entry *ret = (entry *)malloc(sizeof(entry));
  if (ret != NULL) {
    ret->content = content;
  }
  return ret;
}

gallus_hashmap_t ht = NULL;
size_t n_entry = 100;
uint64_t *keys = NULL;

void
setUp(void) {
  gallus_result_t rc;
  srand((unsigned int)time(NULL));

  keys = (uint64_t *)malloc(sizeof(uint64_t) * n_entry);
  if (keys == NULL) {
    exit(1);
  }
  if ((rc = gallus_hashmap_create(&ht,
                                   GALLUS_HASHMAP_TYPE_ONE_WORD,
                                   delete_entry)) != GALLUS_RESULT_OK) {
    gallus_perror(rc);
    exit(1);
  }
}

void
tearDown(void) {
  if (ht != NULL) {
    gallus_hashmap_destroy(&ht, true);
  }
  free((void *)keys);
}

void
test_hash_table_creation(void) {
  gallus_result_t rc;
  gallus_hashmap_t myht = NULL;

  if ((rc = gallus_hashmap_create(&myht,
                                   GALLUS_HASHMAP_TYPE_ONE_WORD,
                                   delete_entry)) != GALLUS_RESULT_OK) {
    TEST_FAIL_MESSAGE("creation failed");
    goto done;
  }
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_OK, rc,
      "create hashmap");

done:
  if (myht != NULL) {
    gallus_hashmap_destroy(&myht, true);
  }
}

void
test_hash_miss(void) {
  gallus_result_t rc;
  entry *e;
  size_t i;

  i = 1;
  rc = gallus_hashmap_find(&ht, (void *)i, (void *)&e);
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_NOT_FOUND, rc,
      "If key is not found in table, "
      "function should return GALLUS_RESULT_NOT_FOUND");
}

void
test_invalid_arguments(void) {
  gallus_result_t rc;
  entry *e;
  size_t i = 0;
  rc = gallus_hashmap_create(NULL, GALLUS_HASHMAP_TYPE_ONE_WORD, NULL);
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_INVALID_ARGS, rc,
      "gallus_hashmap_create(hmptr=NULL, ...) is invalid argument");

  rc = gallus_hashmap_find(NULL, (void *)i, (void *)&e);
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_INVALID_ARGS, rc,
      "gallus_hashmap_find(hmptr=NULL, ...) is invalid argument");

  rc = gallus_hashmap_find(&ht, (void *)i, (void *)NULL);
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_INVALID_ARGS, rc,
      "gallus_hashmap_find(..., valptr=NULL) is invalid argument");

  rc = gallus_hashmap_clear(NULL, true);
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_INVALID_ARGS, rc,
      "gallus_hashmap_find(hmptr=NULL, ...) is invalid argument");

}

void
test_hash_table_add(void) {
  gallus_result_t rc;
  entry *e, *ne;
  size_t i;
  uint64_t new_value = 10;

  for (i = 0; i < n_entry; i++) {
    e = new_entry(i);
    rc = gallus_hashmap_add(&ht, (void *)i, (void **)&e, false);
    TEST_ASSERT_EQUAL_GALLUS_STATUS(GALLUS_RESULT_OK, rc);
    TEST_ASSERT_NULL_MESSAGE(e,
                             "If hashmap_add function occurred no confliction, "
                             "valptr should be set to NULL");
  }

  for (i = 0; i < n_entry; i++) {
    rc = gallus_hashmap_find(&ht, (void *)i, (void *)&e);
    TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_OK, rc,
        "If key is found in table, "
        "then hashmap_find function return GALLUS_RESULT_OK");
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(i, e->content,
                                     "check a value of hash entry");
  }

  i = 0;
  ne = new_entry(new_value);
  rc = gallus_hashmap_add(&ht, (void *)i, (void **)&ne, false);
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_ALREADY_EXISTS, rc,
      "If key already exists and overwrite is not allowed, "
      "then hashmap_add function return GALLUS_RESULT_ALREADY_EXISTS");
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(i, ne->content,
                                   "If hashmap_add function occurred a confliction, "
                                   "valptr should be set to former value");
  rc = gallus_hashmap_find(&ht, (void *)i, (void *)&e);
  TEST_ASSERT_EQUAL(GALLUS_RESULT_OK, rc);
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(e->content, i,
                                   "Failed hashmap_add function shouldn't change value");
}

void
test_hash_table_overwrite(void) {
  gallus_result_t rc;
  entry *e, *ne;
  size_t i;
  uint64_t new_value = 10;

  for (i = 0; i < n_entry; i++) {
    e = new_entry(i);
    rc = gallus_hashmap_add(&ht, (void *)i, (void **)&e, false);
    TEST_ASSERT_EQUAL_GALLUS_STATUS(GALLUS_RESULT_OK, rc);
    TEST_ASSERT_NULL_MESSAGE(e,
                             "If hashmap_add function occurred no confliction, "
                             "valptr should be set to NULL");
  }

  for (i = 0; i < n_entry; i++) {
    rc = gallus_hashmap_find(&ht, (void *)i, (void *)&e);
    TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_OK, rc,
        "If key is found in table, "
        "then hashmap_find function return GALLUS_RESULT_OK");
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(i, e->content,
                                     "check a value of hash entry");
  }

  i = 0;
  ne = new_entry(new_value);
  rc = gallus_hashmap_add(&ht, (void *)i, (void **)&ne, true);
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_OK, rc,
      "If key already exists and overwrite is allowed, "
      "then hashmap_add function return GALLUS_RESULT_OK");
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(i, ne->content,
                                   "If hashmap_add function occurred a confliction, "
                                   "valptr should be set to former value");
  rc = gallus_hashmap_find(&ht, (void *)i, (void *)&e);
  TEST_ASSERT_EQUAL(GALLUS_RESULT_OK, rc);
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(new_value, e->content,
                                   "Succeeded hashmap_add function should change value");
}

void
test_hash_table_delete(void) {
  gallus_result_t rc;
  entry *e, *ne;
  size_t i;

  for (i = 0; i < n_entry; i++) {
    e = new_entry(i);
    rc = gallus_hashmap_add(&ht, (void *)i, (void **)&e, false);
    TEST_ASSERT_EQUAL_GALLUS_STATUS(GALLUS_RESULT_OK, rc);
  }

  i = 50;
  rc = gallus_hashmap_delete(&ht, (void *)i, (void **)&ne, true);
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_OK, rc,
      "If key exists, "
      "then hashmap_delete function return GALLUS_RESULT_OK");
  rc = gallus_hashmap_find(&ht, (void *)i, (void *)&e);
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_NOT_FOUND, rc,
      "Succeeded hashmap_delete function should delete a pair");

  i = 51;
  rc = gallus_hashmap_delete(&ht, (void *)i, (void **)&ne, false);
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_OK, rc,
      "If key exists, "
      "then hashmap_delete function return GALLUS_RESULT_OK");
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(i, ne->content,
                                   "If an argument 'free_values' is false, "
                                   "hashmap_delete function shouldn't free entries");
  rc = gallus_hashmap_find(&ht, (void *)i, (void *)&ne);
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_NOT_FOUND, rc,
      "Succeeded hashmap_delete function should delete a pair");
  rc = gallus_hashmap_delete(&ht, (void *)i, (void **)&ne, false);
  TEST_ASSERT_EQUAL_GALLUS_STATUS_MESSAGE(GALLUS_RESULT_OK, rc,
      "If key does not exist, "
      "then hashmap_delete function return GALLUS_RESULT_OK");
}

void
test_hash_table_clear(void) {
  gallus_result_t rc;
  entry *e, *ne;
  size_t i;

  for (i = 0; i < n_entry; i++) {
    e = new_entry(i);
    rc = gallus_hashmap_add(&ht, (void *)i, (void **)&e, false);
    TEST_ASSERT_EQUAL_GALLUS_STATUS(GALLUS_RESULT_OK, rc);
  }

  i = 50;
  rc = gallus_hashmap_find(&ht, (void *)i, (void *)&ne);
  TEST_ASSERT_EQUAL_GALLUS_STATUS(GALLUS_RESULT_OK, rc);
  rc = gallus_hashmap_clear(&ht, false);
  TEST_ASSERT_EQUAL_GALLUS_STATUS(GALLUS_RESULT_OK, rc);
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(i, ne->content,
                                   "If an argument 'free_values' is false, "
                                   "hashmap_clear function shouldn't free entries");
  free(ne);

  for (i = 0; i < n_entry; i++) {
    e = new_entry(i);
    rc = gallus_hashmap_add(&ht, (void *)i, (void **)&e, false);
    TEST_ASSERT_EQUAL_GALLUS_STATUS(GALLUS_RESULT_OK, rc);
  }
  rc = gallus_hashmap_clear(&ht, true);
  TEST_ASSERT_EQUAL_GALLUS_STATUS(GALLUS_RESULT_OK, rc);
}

void
test_hash_table_size(void) {
  gallus_result_t size;
  gallus_result_t rc;
  entry *e, *ne;
  size_t i;

  size = gallus_hashmap_size(&ht);
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(0, size,
                                   "Initial size of hash is 0");

  i = 0;
  e = new_entry(i);
  rc = gallus_hashmap_add(&ht, (void *)i, (void **)&e, false);
  TEST_ASSERT_EQUAL_GALLUS_STATUS(GALLUS_RESULT_OK, rc);

  size = gallus_hashmap_size(&ht);
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(1, size,
                                   "If add 1 pair, then size is 1");

  rc = gallus_hashmap_delete(&ht, (void *)i, (void **)&ne, true);
  TEST_ASSERT_EQUAL_GALLUS_STATUS(GALLUS_RESULT_OK, rc);
  size = gallus_hashmap_size(&ht);
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(0, size,
                                   "If add 1 pair and delete it, then size is 0");

  for (i = 0; i < 100; i++) {
    e = new_entry(i);
    rc = gallus_hashmap_add(&ht, (void *)i, (void **)&e, false);
    TEST_ASSERT_EQUAL_GALLUS_STATUS(GALLUS_RESULT_OK, rc);
  }
  size = gallus_hashmap_size(&ht);
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(100, size,
                                   "If add 100 pair, then size is 100");

  for (i = 0; i < 100; i++) {
    rc = gallus_hashmap_delete(&ht, (void *)i, (void **)&ne, true);
    TEST_ASSERT_EQUAL_GALLUS_STATUS(GALLUS_RESULT_OK, rc);
  }
  size = gallus_hashmap_size(&ht);
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(0, size,
                                   "If add 100 pair and delete it, then size is 0");
}
