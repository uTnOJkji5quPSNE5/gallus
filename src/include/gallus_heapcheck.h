#ifndef __GALLUS_HEAPCHECK_H__
#define __GALLUS_HEAPCHECK_H__





__BEGIN_DECLS


void
gallus_heapcheck_module_initialize(void);

bool
gallus_heapcheck_is_in_heap(const void *addr);

#if 0
bool
gallus_heapcheck_is_mallocd(const void *addr);
#endif


__END_DECLS





#endif /* __GALLUS_HEAPCHECK_H__ */

