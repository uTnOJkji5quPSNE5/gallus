#ifndef __GALLUS_THREAD_H__
#define __GALLUS_THREAD_H__





/**
 *	@file	gallus_thread.h
 */





#if SIZEOF_PTHREAD_T == SIZEOF_INT64_T
#define GALLUS_INVALID_THREAD	(pthread_t)~0LL
#elif SIZEOF_PTHREAD_T == SIZEOF_INT
#define GALLUS_INVALID_THREAD	(pthread_t)~0
#endif /* SIZEOF_PTHREAD_T == SIZEOF_INT64_T ... */





__BEGIN_DECLS





typedef struct gallus_thread_record 	*gallus_thread_t;


/**
 * @details The signature of thread finalization functions.
 *
 * @details In these functions <em> \b DO \b NOT </em> destroy/\b
 * free() anything, but just "make it sure" the threads are ready to
 * quit without any side effects that will affect futher operation
 * (e.g.: releasing the global lock, closing file descriptors, etc.).
 *
 * @details If \b is_canceled is \b true, it indicates that the thread
 * is canceled.
 */
typedef void
(*gallus_thread_finalize_proc_t)(const gallus_thread_t *selfptr,
                                  bool is_canceled,
                                  void *arg);


/**
 * @details The signature of thread free up functions.
 *
 * @details In thees functions, just free up any other dynamic
 * allocated objects which are needed to be freed up.
 *
 * @details And <em> \b DO \b NOT </em> free() the \b selfptr itself.
 */
typedef void
(*gallus_thread_freeup_proc_t)(const gallus_thread_t *selfptr,
                                void *arg);


/**
 * @details The signature of thread main workhorses.
 */
typedef gallus_result_t
(*gallus_thread_main_proc_t)(const gallus_thread_t *selfptr,
                              void *arg);




/**
 * Create a thread.
 *
 *	@param[in,out]	thdptr		A pointer to a thread.
 *	@param[in]	mainproc	A pointer to a main procedure.
 *	(\b NULL not allowed)
 *	@param[in]	finalproc	A pointer to a finalize procedure.
 *	(\b NULL allowed)
 *	@param[in]	freeproc	A pointer to a freeup procedure.
 *	(\b NULL allowed)
 *	@param[in]	name		A name of a thread to be created.
 *	@param[in]	arg		An auxiliary argumnet for the
 *	\b mainproc. (\b NULL allowed)
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b thdptr is \b NULL, it allocates a memory area
 *	and the allocated area is always free'd by calling \b
 *	gallus_thread_destroy(). Otherwise the pointer given is used
 *	as is.
 */
gallus_result_t
gallus_thread_create(gallus_thread_t *thdptr,
                      gallus_thread_main_proc_t mainproc,
                      gallus_thread_finalize_proc_t finalproc,
                      gallus_thread_freeup_proc_t freeproc,
                      const char *name,
                      void *arg);


/**
 * Create a thread with an allocation size.
 *
 *	@param[in,out]	thdptr		A pointer to a thread.
 *	@param[in]	alloc_size	A memory allocation size for
 *	this object (in bytes)
 *	@param[in]	mainproc	A pointer to a main procedure.
 *	(\b NULL not allowed)
 *	@param[in]	finalproc	A pointer to a finalize procedure.
 *	(\b NULL allowed)
 *	@param[in]	freeproc	A pointer to a freeup procedure.
 *	(\b NULL allowed)
 *	@param[in]	name		A name of a thread to be created.
 *	@param[in]	arg		An auxiliary argumnet for the
 *	\b mainproc. (\b NULL allowed)
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b thdptr is \b NULL, it allocates a memory area
 *	and the allocated area is always free'd by calling \b
 *	gallus_thread_destroy(). In this case if the \b
 *	alloc_size is greater than the original object size, \b
 *	alloc_size bytes momory area is allocated. Otherwise if the \b
 *	thdptr is not NULL the pointer given is used as is.
 */
gallus_result_t
gallus_thread_create_with_size(gallus_thread_t *thdptr,
                                size_t alloc_size,
                                gallus_thread_main_proc_t mainproc,
                                gallus_thread_finalize_proc_t finalproc,
                                gallus_thread_freeup_proc_t freeproc,
                                const char *name,
                                void *arg);


/**
 * Start a thread.
 *
 *	@param[in]	thdptr		A pointer to a thread.
 *	@param[in]	autodelete	If \b true, the is deleted
 *	automatically when finished.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ALREADY_EXISTS	Failed, already exists.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_thread_start(const gallus_thread_t *thdptr,
                     bool autodelete);


/**
 * Cancel a thread.
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval GALLUS_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_thread_cancel(const gallus_thread_t *thdptr);


/**
 * Wait for a thread finishes.
 *
 *	@param[in]  thdptr	A pointer to a thread.
 *	@param[in]  nsec	Wait time (nanosec).
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, will be not
 *	operational in anytime.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval GALLUS_RESULT_NOT_OWNER	Failed, not the owner.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval GALLUS_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_thread_wait(const gallus_thread_t *thdptr,
                    gallus_chrono_t nsec);


/**
 * Destroy thread.
 *
 *     @param[in] thdptr         A pointer to a thread.
 *
 *    @details if thread is running, cancel it.
 */
void
gallus_thread_destroy(gallus_thread_t *thdptr);


/**
 * Let the \b gallus_thread_destroy() call free(3) for a thread
 * itself even the thread is not allocated by \b gallus_thread_create().
 *
 *	@param[in] thdptr	A pointer to a thread.
 *
 *	@retval GALLUS_RESULT_OK               Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval GALLUS_RESULT_INVALID_ARGS     Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES     Failed.
 */
gallus_result_t
gallus_thread_free_when_destroy(gallus_thread_t *thdptr);


/**
 * Get a pthread id of a thread.
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *	@param[out]	tidptr	A pointer to a thread id.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval GALLUS_RESULT_NOT_STARTED	Failed, thread not started.
 *	@retval GALLUS_RESULT_ALREADY_HALTED	Failed, thread already halted.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_thread_get_pthread_id(const gallus_thread_t *thdptr,
                              pthread_t *tidptr);


/**
 * Set a CPU affinity of a thread.
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *	@param[in]	cpu	A cpu # where the thread is bound to.
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
 *	thread.
 */
gallus_result_t
gallus_thread_set_cpu_affinity(const gallus_thread_t *thdptr,
                                int cpu);


/**
 * Get a CPU affinity of a thread.
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval GALLUS_RESULT_ALREADY_HALTED	Failed, thread already halted.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_NOT_DEFINED	Failed, Not defined/sepcified.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details This API is available only on Linux and eqivalent to
 *	CPU_SET(3).
 *
 *	@details If users called the \b
 *	gallus_thread_set_cpu_affinity(), this API returns the
 *	smallest # of the CPU set by the \b
 *	gallus_thread_set_cpu_affinity().
 *
 *	@details Users can call this API before and after starting the
 *	thread.
 */
gallus_result_t
gallus_thread_get_cpu_affinity(const gallus_thread_t *thdptr);


/**
 * Set a result code to a thread.
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *	@param[in]	code	A result code to set.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_thread_set_result_code(const gallus_thread_t *thdptr,
                               gallus_result_t code);


/**
 * Get a result code of a thread.
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *	@param[out]	codeptr	A pointer to a result code.
 *	@param[in]	nsec	Wait time (nanosec).
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, posix API error.
 *	@retval GALLUS_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_thread_get_result_code(const gallus_thread_t *thdptr,
                               gallus_result_t *codeptr,
                               gallus_chrono_t nsec);


/**
 * Returns \b true if the thread is cancelled.
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *	@param[out]	retptr	A pointer to a result.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval GALLUS_RESULT_INVALID_ARGS     Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES     Failed.
 */
gallus_result_t
gallus_thread_is_canceled(const gallus_thread_t *thdptr,
                           bool *retptr);


/**
 * Returns \b true if the thread is valid (not destroyed).
 *
 *	@param[in]	thdptr	A pointer to a thread.
 *	@param[out]	retptr	A pointer to a result.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_OBJECT	Failed, invalid thread.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_thread_is_valid(const gallus_thread_t *thdptr,
                        bool *retptr);


/**
 * Preparation for fork(2).
 *
 *	@details We believe we don't need this, but in case.
 *
 *	@details This function is provided for the pthread_atfork(3)'s
 *	last argument. If we have to, functions for the first and
 *	second arguments for the pthread_atfork(3) would be provided
 *	as well, in later.
 */
void
gallus_thread_atfork_child(const gallus_thread_t *thdptr);


/**
 * Initialize the thread module.
 *
 *	@details No need to call this explicitly UNTIL you have to
 *	write any modules that could be required ordered
 *	initialization.
 */
void
gallus_thread_module_initialize(void);


/**
 * Finalize the thread module.
 *
 *	@details No need to call this explicitly UNTIL you have to
 *	write any modules that could be required ordered
 *	finalization.
 */
void
gallus_thread_module_finalize(void);


gallus_result_t
gallus_thread_get_name(const gallus_thread_t *thdptr, char *buf, size_t n);


gallus_result_t
gallus_thread_set_name(const gallus_thread_t *thdptr, const char *name);





__END_DECLS





#endif /* ! __GALLUS_THREAD_H__ */
