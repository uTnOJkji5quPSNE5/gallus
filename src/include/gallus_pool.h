#pragma once





/**
 *	@file	gallus_pool.h
 */





__BEGIN_DECLS





typedef struct gallus_pool_record	*gallus_pool_t;


typedef enum {
  GALLUS_POOL_TYPE_UNKNOWN = 0,
  GALLUS_POOL_TYPE_QUEUE = 1,
  GALLUS_POOL_TYPE_INDEX = 2,
} gallus_pool_type_t;





gallus_result_t
gallus_pool_get_pool(const char *name, gallus_pool_t *pptr);


gallus_result_t
gallus_pool_create(gallus_pool_t *pptr, size_t sz, const char *name,
		gallus_pool_type_t type,
		bool is_executor,
		size_t n_max_objs,
		size_t pobj_size,
		gallus_poolable_methods_t m);


gallus_result_t
gallus_pool_add_poolable_by_index(gallus_pool_t *pptr, uint64_t index, void *args);


gallus_result_t
gallus_pool_acquire_poolable(gallus_pool_t *pptr, gallus_chrono_t to,
			  gallus_poolable_t *pobjptr);


gallus_result_t
gallus_pool_acquire_poolable_by_index(gallus_pool_t *pptr, uint64_t index,
				   gallus_chrono_t to,
				   gallus_poolable_t *pobjptr);


gallus_result_t
gallus_pool_release_poolable(gallus_poolable_t *pobjptr);


gallus_result_t
gallus_pool_get_outstanding_obj_num(gallus_pool_t *pptr);


gallus_result_t
gallus_pool_wakeup(gallus_pool_t *pptr, gallus_chrono_t to);


gallus_result_t
gallus_pool_shutdown(gallus_pool_t *pptr,  shutdown_grace_level_t lvl,
		  gallus_chrono_t to);


gallus_result_t
gallus_pool_cancel(gallus_pool_t *pptr);


gallus_result_t
gallus_pool_wait(gallus_pool_t *pptr, gallus_chrono_t to);


void
gallus_pool_destroy(gallus_pool_t *pptr);


__END_DECLS

