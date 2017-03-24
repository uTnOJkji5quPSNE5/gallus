#ifndef __GALLUS_BBQ2_H__
#define __GALLUS_BBQ2_H__





/**
 * @file gallus_bbq2.h
 */





#ifndef __GALLUS_BBQ2_T_DEFINED__
typedef struct gallus_bbq2_record *gallus_bbq2_t;
#endif /* ! __GALLUS_BBQ2_T_DEFINED__ */





__BEGIN_DECLS





/**
 * @details The signature of value free up functions called when
 * destroying a bbq.
 */
typedef void	(*gallus_bbq2_value_freeup_proc_t)(void **valptr);





/**
 * Create a bbq.
 *
 *     @param[in,out]	qptr	A pointer to a bbq to be created.
 *     @param[in]	numa_node	A NUMA node where to allocate.
 *     @param[in]	elemsize	A size of an element.
 *     @param[in]	maxelems	# of maximum elements.
 *     @param[in]	proc	A value free up function (\b NULL allowed).
 *
 *     @retval GALLUS_RESULT_OK               Succeeded.
 *     @retval GALLUS_RESULT_NO_MEMORY        Failed, no memory.
 *     @retval GALLUS_RESULT_ANY_FAILURES     Failed.
 */
gallus_result_t
gallus_bbq2_create(gallus_bbq2_t *qptr,
                   unsigned int numa_node,
                   size_t elemsize,
                   size_t maxelems,
                   gallus_bbq2_value_freeup_proc_t proc);


/**
 * Shutdown a bbq.
 *
 *    @param[in]	qptr	A pointer to a bbq to be shutdown.
 *    @param[in]	free_values	If \b true, all the values remaining
 *    in the bbq are freed if the value free up function given by
 *    the calling of the \b gallus_bbq2_create() is not \b NULL.
 */
void
gallus_bbq2_shutdown(gallus_bbq2_t *qptr, bool free_values);


/**
 * Destroy a bbq.
 *
 *    @param[in]  qptr    A pointer to a bbq to be destroyed.
 *    @param[in]  free_values  If \b true, all the values
 *    remaining in the bbq are freed if the value free up
 *    function given by the calling of the \b gallus_bbq2_create()
 *    is not \b NULL.
 *
 *    @details if \b *qptr is operational, shutdown it.
 */
void
gallus_bbq2_destroy(gallus_bbq2_t *qptr, bool free_values);


/**
 * Clear a bbq.
 *
 *     @param[in]  qptr        A pointer to a bbq.
 *     @param[in]  free_values  If \b true, all the values
 *     remaining in the bbq are freed if the value free up
 *     function given by the calling of the gallus_cbuffer_create()
 *     is not \b NULL.
 *
 *     @retval GALLUS_RESULT_OK                Succeeded.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
gallus_result_t
gallus_bbq2_clear(gallus_bbq2_t *qptr, bool free_values);


/**
 * Wake up all the waiters in a bbq.
 *
 *     @param[in]  qptr		A pointer to a bbq.
 *     @param[in]  nsec		Wait time (nanosec).
 *
 *     @retval GALLUS_RESULT_OK                Succeeded.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
gallus_result_t
gallus_bbq2_wakeup(gallus_bbq2_t *qptr, gallus_chrono_t nsec);


/**
 * Wait for gettable.
 *
 *     @param[in]  qptr		A pointer to a bbq.
 *     @param[in]  nsec		Wait time (nanosec).
 *
 *     @retval >0				# of the gettable elements.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
gallus_result_t
gallus_bbq2_wait_gettable(gallus_bbq2_t *qptr, gallus_chrono_t nsec);


/**
 * Wait for puttable.
 *
 *     @param[in]  qptr		A pointer to a bbq.
 *     @param[in]  nsec		Wait time (nanosec).
 *
 *     @retval >0				# of the puttable elements.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
gallus_result_t
gallus_bbq2_wait_puttable(gallus_bbq2_t *qptr, gallus_chrono_t nsec);





/**
 * Put an element at the tail of a bbq.
 *
 *     @param[in]  qptr       A pointer to a bbq.
 *     @param[in]  valptr     A pointer to an element.
 *     @param[in]  valsz      A size of an element.
 *     @param[in]  nsec       Wait time (nanosec).
 *
 *     @retval GALLUS_RESULT_OK                Succeeded.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
gallus_result_t
gallus_bbq2_put(gallus_bbq2_t *qptr,
                void **valptr,
                size_t valsz,
                gallus_chrono_t nsec);


/**
 * Put elements at the tail of a bbq.
 *
 *     @param[in]  qptr         A pointer to a bbq.
 *     @param[in]  valptr       A pointer to elements.
 *     @param[in]  n_vals       A # of elements to put.
 *     @param[in]  valsz        A size of an element.
 *     @param[in]  nsec         A wait time (in nsec).
 *     @param[out] n_actual_put A pointer to a # of elements successfully put (\b NULL allowed.)
 *
 *     @retval >=0 A # of elemets to put successfully.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 *
 *     @details If the \b nsec is less than zero, it blocks until all the
 *     elements specified by n_vals are put.
 *
 *     @details If the \b nsec is zero, it puts elements limited to a number
 *     of available rooms in the buffer (it could be zero.)
 *
 *     @details If the \b nsec is greater than zero, it puts elements limited
 *     to a number of available rooms in the buffer until the time specified
 *     by the \b nsec is expired. In this case if the actual number of
 *     elements put is less than the \b n_vals, it returns \b
 *     GALLUS_RESULT_TIMEDOUT.
 *
 *     @details If the \b n_actual_put is not a \b NULL, a number of elements
 *     actually put is stored.
 *
 *     @details If any errors occur while putting, it always returns an error
 *     result even if at least an element is successfully put, so check
 *     *n_actual_put if needed.
 *
 *     @details It is allowed that more then one thread simultaneously
 *     invokes this function, the atomicity of the operation is not
 *     guaranteed.
 */
gallus_result_t
gallus_bbq2_put_n(gallus_bbq2_t *qptr,
                  void **valptr,
                  size_t n_vals,
                  size_t valsz,
                  gallus_chrono_t nsec,
                  size_t *n_actual_put);


/**
 * Get the head element in a bbq.
 *
 *     @param[in]  qptr       A pointer to a bbq.
 *     @param[out] valptr     A pointer to an element.
 *     @param[in]  valsz      A size of an element.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval GALLUS_RESULT_OK                Succeeded.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
gallus_result_t
gallus_bbq2_get(gallus_bbq2_t *qptr,
                void **valptr,
                size_t valsz,
                gallus_chrono_t nsec);


/**
 * Get elements from the head of a bbq.
 *
 *     @param[in]  qptr         A pointer to a qptr.
 *     @param[out] valptr       A pointer to a element.
 *     @param[in]  n_vals_max   A maximum # of elemetns to get.
 *     @param[in]  n_at_least   A minimum # of elements to get until timeout.
 *     @param[in]  valsz        A size of an element.
 *     @param[in]  nsec         A wait time (in nsec).
 *     @param[out] n_actual_get A pointer to a # of elements successfully get (\b NULL allowed.)
 *
 *     @retval >=0 A # of acuired elements.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 *
 *     @details If the \b nsec is less than zero, it blocks until all the
 *     specified number (\b n_vals_max) of the elements are acquired.
 *
 *     @details If the \b nsec is zero, only the available elements at the
 *     moment (it could be zero) are acuqured.
 *
 *     @details If the \b nsec is greater than zero, only the elements
 *     which become available while in the time specified by the \b nsec
 *     are acquired.
 *
 *     @details If the \b nsec is greater than zero and the \b
 *     n_at_least is zero, it treats that the \b nsec is
 *     zero. Otherwise, if bothe the \b nsec and the \b n_at_least are
 *     greater than zero, it returns when the number of the elements
 *     sepcified by the \b n_at_least are acquired, even before the
 *     time period specified by the \b nsec is expired.
 *
 *     @details The \b valptr must point a sufficient size (\b valsz *
 *     \b n_vals_max) buffer enough to store elements.
 *
 *     @details If the \b n_actual_get is not a \b NULL, a number of elements
 *     actually get is stored.
 *
 *     @details If any errors occur while getting, it always returns an error
 *     result even if at least an element is successfully got, so check
 *     *n_actual_get if needed.
 *
 */
gallus_result_t
gallus_bbq2_get_n(gallus_bbq2_t *qptr,
                  void **valptr,
                  size_t n_vals_max,
                  size_t n_at_least,
                  size_t valsz,
                  gallus_chrono_t nsec,
                  size_t *n_actual_get);


/**
 * Peek the head element of a bbq.
 *
 *     @param[in]  qptr       A pointer to a bbq.
 *     @param[out] valptr     A pointer to a element.
 *     @param[in]  valsz      A size of an element.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval GALLUS_RESULT_OK                Succeeded.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
gallus_result_t
gallus_bbq2_peek(gallus_bbq2_t *qptr,
                 void **valptr,
                 size_t valsz,
                 gallus_chrono_t nsec);


/**
 * Peek elements from the head of a bbq.
 *
 *     @param[in]  qptr          A pointer to a bbq.
 *     @param[out] valptr        A pointer to a element.
 *     @param[in]  n_vals_max    A maximum # of elemetns to get.
 *     @param[in]  n_at_least    A minimum # of elements to get until timeout.
 *     @param[in]  valsz         A size of an element.
 *     @param[in]  nsec          A wait time (in nsec).
 *     @param[out] n_actual_peek A pointer to a # of elements successfully get (\b NULL allowed.)
 *
 *     @retval >=0 A # of peeked elements.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 *
 *     @details If the \b nsec is less than zero, it blocks until all the
 *     specified number (\b n_vals_max) of the elements are peeked.
 *
 *     @details If the \b nsec is zero, only the available elements at the
 *     moment (it could be zero) are peeked.
 *
 *     @details If the \b nsec is greater than zero, only the elements
 *     which become available while in the time specified by the \b nsec
 *     are peeked.
 *
 *     @details If the \b nsec is greater than zero and the \b
 *     n_at_least is zero, it treats that the \b nsec is
 *     zero. Otherwise, if bothe the \b nsec and the \b n_at_least are
 *     greater than zero, it returns when the number of the elements
 *     sepcified by the \b n_at_least are peeked, even before the
 *     time period specified by the \b nsec is expired.
 *
 *     @details The \b valptr must point a sufficient size (\b sizeof(type) *
 *     \b n_vals_max) buffer enough to store elements.
 *
 *     @details If the \b n_actual_peek is not a \b NULL, a number of elements
 *     actually get is stored.
 *
 *     @details If any errors occur while peeking, it always returns an error
 *     result even if at least an element is successfully got, so check
 *     *n_actual_peek if needed.
 */
gallus_result_t
gallus_bbq2_peek_n(gallus_bbq2_t *qptr,
                   void **valptr,
                   size_t n_vals_max,
                   size_t n_at_least,
                   size_t valsz,
                   gallus_chrono_t nsec,
                   size_t *n_actual_get);





/**
 * Get a # of elements in a bbq.
 *	@param[in]   qptr    A pointer to a bbq.
 *
 *	@retval	>=0	A # of elements in the bbq.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_bbq2_size(gallus_bbq2_t *qptr);


/**
 * Get the remaining capacity of a bbq.
 *	@param[in]   qptr    A pointer to a bbq.
 *
 *	@retval	>=0	The remaining capacity of the bbq.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_bbq2_remaining_capacity(gallus_bbq2_t *qptr);


/**
 * Get the maximum capacity of a bbq.
 *	@param[in]   qptr    A pointer to a bbq.
 *
 *	@retval	>=0	The maximum capacity of the bbq.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_bbq2_max_capacity(gallus_bbq2_t *qptr);





/**
 * Returns \b true if the bbq is full.
 *
 *    @param[in]   qptr     A pointer to a bbq.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_bbq2_is_full(gallus_bbq2_t *qptr, bool *retptr);


/**
 * Returns \b true if the bbq is empty.
 *
 *    @param[in]   cbptr    A pointer to a bbq.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_bbq2_is_empty(gallus_bbq2_t *qptr, bool *retptr);


/**
 * Returns \b true if the bbq is operational.
 *
 *    @param[in]   qptr     A pointer to a circular buffer
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_bbq2_is_operational(gallus_bbq2_t *qptr, bool *retptr);


/**
 * Cleanup an internal state of a bbq after thread
 * cancellation.
 *	@param[in]	qptr	A pointer to a bbq.
 */
void
gallus_bbq2_cancel_janitor(gallus_bbq2_t *cbptr);





__END_DECLS





#endif  /* ! __GALLUS_BBQ2_H__ */
