#pragma once





/**
 *	@file	gallus_poolable.h
 */





__BEGIN_DECLS





typedef struct gallus_poolable_record	*gallus_poolable_t;


typedef enum {
  GALLUS_POOLABLE_STATE_UNKNOWN = 0,
  GALLUS_POOLABLE_STATE_CONSTRUCTED = 1,
  GALLUS_POOLABLE_STATE_OPERATIONAL = 2,
  GALLUS_POOLABLE_STATE_SHUTDOWNING = 3,
  GALLUS_POOLABLE_STATE_CANCELLING = 4,
  GALLUS_POOLABLE_STATE_NOT_OPERATIONAL = 5,
} gallus_poolable_state_t;
  

typedef gallus_result_t
(*gallus_poolable_construct_proc_t)(gallus_poolable_t *self,
				 void *carg);

typedef gallus_result_t
(*gallus_poolable_setup_proc_t)(gallus_poolable_t *self);

typedef gallus_result_t
(*gallus_poolable_shutdown_proc_t)(gallus_poolable_t *self,
				shutdown_grace_level_t lvl);

typedef gallus_result_t
(*gallus_poolable_cancel_proc_t)(gallus_poolable_t *self);

typedef gallus_result_t
(*gallus_poolable_teardown_proc_t)(gallus_poolable_t *self, bool is_cancelled);

typedef gallus_result_t
(*gallus_poolable_wait_proc_t)(gallus_poolable_t *self, gallus_chrono_t nsec);

typedef void
(*gallus_poolable_destruct_proc_t)(gallus_poolable_t *self);


typedef struct gallus_poolable_methods_record {
  gallus_poolable_construct_proc_t m_construct;
  gallus_poolable_setup_proc_t m_setup;
  gallus_poolable_shutdown_proc_t m_shutdown;
  gallus_poolable_cancel_proc_t m_cancel;
  gallus_poolable_teardown_proc_t m_teardown;
  gallus_poolable_wait_proc_t m_wait;
  gallus_poolable_destruct_proc_t m_destruct;
} gallus_poolable_methods_record;
typedef gallus_poolable_methods_record *gallus_poolable_methods_t;





gallus_result_t
gallus_poolable_create_with_size(gallus_poolable_t *pptr,
                              bool is_executor,
                              gallus_poolable_methods_t m,
                              void *carg, size_t sz);


gallus_result_t
gallus_poolable_setup(gallus_poolable_t *pptr);


gallus_result_t
gallus_poolable_shutdown(gallus_poolable_t *pptr, shutdown_grace_level_t lvl);


gallus_result_t
gallus_poolable_cancel(gallus_poolable_t *pptr);


gallus_result_t
gallus_poolable_teardown(gallus_poolable_t *pptr, bool is_cancelled);


gallus_result_t
gallus_poolable_wait(gallus_poolable_t *pptr, gallus_chrono_t nsec);


void
gallus_poolable_destroy(gallus_poolable_t *pptr);


gallus_result_t
gallus_poolable_is_operational(gallus_poolable_t *pptr, bool *bptr);


gallus_result_t
gallus_poolable_get_status(gallus_poolable_t *pptr, gallus_poolable_state_t *st);





__END_DECLS
