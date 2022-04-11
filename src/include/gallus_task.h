#pragma once





/**
 *	@file	gallus_task.h
 */





#define GALLUS_TASK_DELETE_CONTEXT_AFTER_EXEC	1
#define GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC	2





__BEGIN_DECLS





typedef enum {
  GALLUS_TASK_STATE_UNKNOWN = 0,
  GALLUS_TASK_STATE_CONSTRUCTED = 1,
  GALLUS_TASK_STATE_ATTACHED = 2,
  GALLUS_TASK_STATE_SETUP = 3,
  GALLUS_TASK_STATE_RUNNING = 4,
  GALLUS_TASK_STATE_CLEAN_FINISHED = 5,
  GALLUS_TASK_STATE_HALTED = 6,
  GALLUS_TASK_STATE_CANCELLED = 7,
  GALLUS_TASK_STATE_RUNNER_SHUTDOWN = 8,
} gallus_task_state_t;


typedef struct gallus_task_record *gallus_task_t;


typedef gallus_result_t (*gallus_task_main_proc_t)(gallus_task_t *tptr);
typedef void (*gallus_task_finalize_proc_t)(gallus_task_t *tptr, bool is_cancelled);
typedef void (*gallus_task_freeup_proc_t)(gallus_task_t *tptr);





gallus_result_t
gallus_task_create(gallus_task_t *tptr, size_t sz, const char *name,
                gallus_task_main_proc_t main_func,
                gallus_task_finalize_proc_t finalize_func,
		gallus_task_freeup_proc_t freeup_func);


gallus_result_t
gallus_task_run(gallus_task_t *tptr, gallus_pooled_thread_t *ptptr, int flag);


void
gallus_task_finalize(gallus_task_t *tptr, bool is_cancelled);


gallus_result_t
gallus_task_wait(gallus_task_t *tptr, gallus_chrono_t to);


gallus_result_t
gallus_task_get_exit_code(gallus_task_t *tptr);


gallus_result_t
gallus_task_get_state(gallus_task_t *tptr,
		   gallus_task_state_t *stptr);


void
gallus_task_destroy(gallus_task_t *tptr);





__END_DECLS



