#ifndef __GALLUS_QSORT_H__
#define __GALLUS_QSORT_H__

#ifdef GALLUS_OS_LINUX
#define gallus_qsort_r qsort_r
#else


__BEGIN_DECLS

void
gallus_qsort_r(void *array,
	       size_t nelem,
	       size_t size,
	       int (*cmp)(const void *, const void *, void *),
	       void *arg);

__END_DECLS


#endif /* GALLUS_OS_LINUX */

#endif /* ! __GALLUS_QSORT_H__ */
