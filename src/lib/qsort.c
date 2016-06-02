#include <stdlib.h>

#include "gallus_qsort.h"

static __thread struct qsort_data {
  int (*cmp)(const void *, const void *, void *);
  void *arg;
};

static int
qsort_cmp(const void *a, const void *b) {
  return qdata.cmp(a, b, qdata.arg);
}

void
gallus_qsort_r(void *array,
                size_t nelem,
                size_t size,
                int (*cmp)(const void *, const void *, void *),
                void *arg) {
  qsort_data qdata = { cmp, arg };
  qsort(array, nelem, size, qsort_cmp);
}
