#pragma once





/**
 *	@file	gallus_pooled_thread.h
 */





__BEGIN_DECLS





typedef enum {
  GALLUS_TASK_RUNNER_STATE_UNKNOWN = 0,
  GALLUS_TASK_RUNNER_STATE_CREATED = 1,
  GALLUS_TASK_RUNNER_STATE_NOT_ASSIGNED = 2,
  GALLUS_TASK_RUNNER_STATE_ASSIGNED = 3,
  GALLUS_TASK_RUNNER_STATE_RUNNING = 4,
  GALLUS_TASK_RUNNER_STATE_FINISH = 5,
} gallus_task_runner_state_t;


typedef struct gallus_task_runner_thread_record *gallus_task_runner_thread_t;
typedef struct gallus_pooled_thread_record *gallus_pooled_thread_t;
typedef struct gallus_thread_pool_record *gallus_thread_pool_t;





gallus_result_t
gallus_thread_pool_create(gallus_thread_pool_t *pptr, const char *name, size_t n);


gallus_result_t
gallus_thread_pool_acquire_thread(gallus_thread_pool_t *pptr,
			       gallus_pooled_thread_t *ptptr,
			       gallus_chrono_t to);


gallus_result_t
gallus_thread_pool_release_thread(gallus_pooled_thread_t *ptptr);


gallus_result_t
gallus_thread_pool_get_outstanding_thread_num(gallus_thread_pool_t *pptr);


gallus_result_t
gallus_thread_pool_wakeup(gallus_thread_pool_t *pptr, gallus_chrono_t to);


gallus_result_t
gallus_thread_pool_shutdown_all(gallus_thread_pool_t *pptr,
			     shutdown_grace_level_t lvl,
			     gallus_chrono_t to);


gallus_result_t
gallus_thread_pool_cancel_all(gallus_thread_pool_t *pptr);


gallus_result_t
gallus_thread_pool_wait_all(gallus_thread_pool_t *pptr, gallus_chrono_t to);


void
gallus_thread_pool_destroy(gallus_thread_pool_t *pptr);


gallus_result_t
gallus_thread_pool_get(const char *name, gallus_thread_pool_t *pptr);





__END_DECLS
