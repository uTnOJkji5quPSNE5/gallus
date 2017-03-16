#ifndef __GALLUS_PIPELINE_STAGE_H__
#define __GALLUS_PIPELINE_STAGE_H__





/**
 *	@file	gallus_pipeline_stage.h
 */





#include "gallus_pipeline_stage_funcs.h"





#ifndef PIPELINE_STAGE_T_DECLARED
typedef struct gallus_pipeline_stage_record 	*gallus_pipeline_stage_t;
#define PIPELINE_STAGE_T_DECLARED
#endif /* ! PIPELINE_STAGE_T_DECLARED */





__BEGIN_DECLS





/**
 * Create a pipeline stage.
 *
 *	@param[in,out] sptr A pointer to a stage.
 *	@param[in] alloc_size A memory allocation size for this object
 *	(in bytes)
 *	@param[in] name A name of the stage.
 *	@param[in] n_workers A # of the workers.
 *	@param[in] event_size A size of an event. (in bytes.)
 *	@param[in] max_batch_size A # of the maximum processed events at
 *	a time in each worker.
 *	@param[in] pre_pause_proc A pre-pause function.
 *	@param[in] sched_proc A schedule function.
 *	@param[in] setup_proc A setup function.
 *	@param[in] fetch_proc A fetch function.
 *	@param[in] main_proc A main function.
 *	@param[in] throw_proc A throw function.
 *	@param[in] shutdown_proc A shutdown function.
 *	@param[in] final_proc A finalization function.
 *	@param[in] freeup_proc A free-up function.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval GALLUS_RESULT_ALREADY_EXISTS	Failed, already exists.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b sptr is \b NULL, it allocates a memory area
 *	and the allocated area is always free'd by calling \b
 *	gallus_pipeline_stage_destroy(). In this case if the \b
 *	alloc_size is greater than the original object size, \b
 *	alloc_size bytes momory area is allocated. Otherwise if the \b
 *	sptr is not NULL the pointer given is used as is.
 */
gallus_result_t
gallus_pipeline_stage_create(gallus_pipeline_stage_t *sptr,
                              size_t alloc_size,
                              const char *name,
                              size_t n_workers,
                              size_t event_size,
                              size_t max_batch_size,
                              gallus_pipeline_stage_pre_pause_proc_t
                              pre_pause_proc,
                              gallus_pipeline_stage_sched_proc_t sched_proc,
                              gallus_pipeline_stage_setup_proc_t setup_proc,
                              gallus_pipeline_stage_fetch_proc_t fetch_proc,
                              gallus_pipeline_stage_main_proc_t main_proc,
                              gallus_pipeline_stage_throw_proc_t throw_proc,
                              gallus_pipeline_stage_shutdown_proc_t
                              shutdown_proc,
                              gallus_pipeline_stage_finalize_proc_t
                              final_proc,
                              gallus_pipeline_stage_freeup_proc_t
                              freeup_proc);


/**
 * Setup a pipeline stage.
 *
 *	@param[in] sptr A pointer to a stage.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_pipeline_stage_setup(const gallus_pipeline_stage_t *sptr);


/**
 * Set a post-start function to a stage.
 *
 *	@param[in] sptr	A pointer to a stage.
 *	@param[in] func	A post-start function.
 *	@param[in] arg	An argument for the \b func.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_pipeline_stage_set_post_start_hook(
  gallus_pipeline_stage_t *sptr,
  gallus_pipeline_stage_post_start_proc_t func,
  void *arg);


/**
 * Start a pipeline stage.
 *
 *	@param[in] sptr A pointer to a stage.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ALREADY_EXISTS	Failed, already exists.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_pipeline_stage_start(const gallus_pipeline_stage_t *sptr);


/**
 * Shutdown a pipeline stage.
 *
 *	@param[in] sptr A pointer to a stage.
 *	@param[in] l A shutdown graceful level.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval GALLUS_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_pipeline_stage_shutdown(const gallus_pipeline_stage_t *sptr,
                                shutdown_grace_level_t l);


/**
 * Cancel a pipeline stage.
 *
 *	@param[in] sptr A pointer to a stage.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval GALLUS_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_pipeline_stage_cancel(const gallus_pipeline_stage_t *sptr);


/**
 * Wait for a pipeline stage finishes.
 *
 *	@param[in]  sptrr	A pointer to a stage.
 *	@param[in]  nsec	Wait timeout (nano second).
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, will be not
 *	operational in anytime.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval GALLUS_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval GALLUS_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_pipeline_stage_wait(const gallus_pipeline_stage_t *sptr,
                            gallus_chrono_t nsec);


/**
 * Destroy a pipeline stage.
 *
 *	@param[in] sptr A pointer to a stage.
 *
 *	@details If the stage is not canceled/shutted down, the stage
 *	is canceled.
 */
void
gallus_pipeline_stage_destroy(gallus_pipeline_stage_t *sptr);


/**
 * Clean an internal state of a pipeline stage up after the
 * cancellation of the caller threads.
 *
 *	@param[in] sptr A pointer to a stage.
 */
void
gallus_pipeline_stage_cancel_janitor(const gallus_pipeline_stage_t *sptr);


/**
 * Set a CPU affinity of a worker in a pipeline stage.
 *
 *	@param[in]	sptr	A pointer to a stage.
 *	@param[in]	idx	A worker index.
 *	@param[in]	cpu	A cpu # where the worker thread is bound to.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval GALLUS_RESULT_ALREADY_HALTED	Failed, thread already halted.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details This API is available only on Linux and eqivalent to
 *	CPU_SET(3). If users needed to set more than one cpu, the
 *	users could call the API several times with differnt \b cpu
 *	value.
 *
 *	@details If \b cpu < 0, clear all the affinity bits.
 *
 *	@details The default affinity mask is acquired from the master
 *	thread. So if users need to set a fresh mask, firstly the
 *	users must call the API with a negative \b cpu value to clear
 *	the default mask, then call the API with appropriate \b cpu
 *	value.
 *
 *	@details Users can call this API before and after starting the
 *	stage.
 */
#ifdef GALLUS_OS_LINUX
gallus_result_t
gallus_pipeline_stage_set_worker_cpu_affinity(
  const gallus_pipeline_stage_t *sptr,
  size_t idx, int cpu);
#endif /* GALLUS_OS_LINUX */





/**
 * Submit a batch to a pipeline stage.
 *
 *	@param[in]  sptr	A pointer to a stage.
 *	@param[in]  evbuf	A buffer of the events to be thrown.
 *	@param[in]  n_evs	A # of events in the \b evbuf.
 *	@param[in]  hint	An arbitrary scheduling hint.
 *
 *	@retval	>=0	# of events thrown.
 *	@retval <0	Failed.
 *
 * @details This function is intended to be invoked in the throw
 * function of stages in order to submit a batch to next stages.
 */
gallus_result_t
gallus_pipeline_stage_submit(const gallus_pipeline_stage_t *sptr,
                              void *evbuf,
                              size_t n_evs,
                              void *hint);





/**
 * Pause a pipeline stage.
 *
 *	@param[in]  sptrr	A pointer to a stage.
 *	@param[in]  nsec	Timeout for pause completion (nano second).
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, will be not
 *	operational in anytime.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval GALLUS_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval GALLUS_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If a stage is paused, the stage is resumed by calling
 *	of the \b gallus_pipeline_stage_resume().
 *
 *	@details <em> Don't call this function in
 *	gallus_pipeline_stage_*_proc() since no one can't resume if
 *	all the workers were paused and If you did whole the stage
 *	must deaalock. </em>
 */
gallus_result_t
gallus_pipeline_stage_pause(const gallus_pipeline_stage_t *sptr,
                             gallus_chrono_t nsec);


/**
 * Resume a paused pipeline stage.
 *
 *	@param[in]  sptrr	A pointer to a stage.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval GALLUS_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_pipeline_stage_resume(const gallus_pipeline_stage_t *sptr);


/**
 * Schedule a maintenance task to a pipeline stage.
 *
 *	@param[in]  sptr	A pointer to a stage.
 *	@param[in]  arg		An argument.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid stage.
 *	@retval GALLUS_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the maintenace function of the stage is not \b
 *	NULL, it is invoked with the \b arg. It is guaranteed that The
 *	maintenance function is calle only for a single worker and the
 *	caller of this API is not blocked since the maintenance
 *	function is executed in the worker's context.
 */
gallus_result_t
gallus_pipeline_stage_schedule_maintenance(
  const gallus_pipeline_stage_t *sptr,
  gallus_pipeline_stage_maintenance_proc_t func, void *arg);


/**
 * Find a pipeline stage by name.
 *
 *	@param[in] name A name of the pipline stage to find.
 *	@param[out] retptr A reference to the found stage pointer.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_NOT_FOUND	Failed, stage not found.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_pipeline_stage_find(const char *name,
                            gallus_pipeline_stage_t *retptr);


/**
 * Get a # of workers in the stage.
 *
 *	@param[in]  sptr	A pointer to a stage.
 *
 *	@retval >=0				Succeeded, A # of 
 *						the workers in the stage.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_pipeline_stage_get_worker_nubmer(gallus_pipeline_stage_t *sptr);


/**
 * Get an event/batch buffer of a pipeline stage worker.
 *
 *	@param[in]	sptr	A pointer to a stage. 
 *	@param[in]	index	A worker index.
 *	@param[out]	buf	A returned address.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_pipeline_stage_get_worker_event_buffer(gallus_pipeline_stage_t *sptr,
                                               size_t index, void **buf);


/**
 * Set an event/batch buffer of a pipeline stage worker.
 *
 *	@param[in]	sptr	A pointer to a stage. 
 *	@param[in]	index	A worker index.
 *	@param[in]	buf	A buffer.
 *	@param[in]	freeup_proc	A free up function for the \b buf.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_pipeline_stage_set_worker_event_buffer(
    gallus_pipeline_stage_t *sptr,
    size_t index,
    void *buf,
    gallus_pipeline_stage_event_buffer_freeup_proc_t freeup_proc);


/**
 * Get name a pipeline stage.
 *
 *	@param[in]	sptr	A pointer to a stage. 
 *	@param[out]	name	A pointer for the returned name.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_pipeline_stage_get_name(const gallus_pipeline_stage_t *sptr,
                                const char **name);





__END_DECLS





#endif /* __GALLUS_PIPELINE_STAGE_H__ */
