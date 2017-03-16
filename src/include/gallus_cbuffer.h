#ifndef __GALLUS_CBUFFER_H__
#define __GALLUS_CBUFFER_H__





/**
 * @file gallus_cbuffer.h
 */





#ifndef __GALLUS_CBUFFER_T_DEFINED__
typedef struct gallus_cbuffer_record 	*gallus_cbuffer_t;
#endif /* ! __GALLUS_CBUFFER_T_DEFINED__ */





__BEGIN_DECLS





/**
 * @details The signature of value free up functions called when
 * destroying a circular buffer.
 */
typedef void	(*gallus_cbuffer_value_freeup_proc_t)(void **valptr);





gallus_result_t
gallus_cbuffer_create_with_size(gallus_cbuffer_t *cbptr,
                                 size_t elemsize,
                                 int64_t maxelems,
                                 gallus_cbuffer_value_freeup_proc_t proc);
/**
 * Create a circular buffer.
 *
 *     @param[in,out]	cbptr	A pointer to a circular buffer to be created.
 *     @param[in]	type	Type of the element.
 *     @param[in]	maxelems	# of maximum elements.
 *     @param[in]	proc	A value free up function (\b NULL allowed).
 *
 *     @retval GALLUS_RESULT_OK               Succeeded.
 *     @retval GALLUS_RESULT_NO_MEMORY        Failed, no memory.
 *     @retval GALLUS_RESULT_ANY_FAILURES     Failed.
 */
#define gallus_cbuffer_create(cbptr, type, maxelems, proc)              \
  gallus_cbuffer_create_with_size((cbptr), sizeof(type), (maxelems), (proc))


/**
 * Shutdown a circular buffer.
 *
 *    @param[in]	cbptr	A pointer to a circular buffer to be shutdown.
 *    @param[in]	free_values	If \b true, all the values remaining
 *    in the buffer are freed if the value free up function given by
 *    the calling of the gallus_cbuffer_create() is not \b NULL.
 */
void
gallus_cbuffer_shutdown(gallus_cbuffer_t *cbptr,
                         bool free_values);


/**
 * Destroy a circular buffer.
 *
 *    @param[in]  cbptr    A pointer to a circular buffer to be destroyed.
 *    @param[in]  free_values  If \b true, all the values
 *    remaining in the buffer are freed if the value free up
 *    function given by the calling of the gallus_cbuffer_create()
 *    is not \b NULL.
 *
 *    @details if \b cbuf is operational, shutdown it.
 */
void
gallus_cbuffer_destroy(gallus_cbuffer_t *cbptr,
                        bool free_values);


/**
 * Clear a circular buffer.
 *
 *     @param[in]  cbptr        A pointer to a circular buffer
 *     @param[in]  free_values  If \b true, all the values
 *     remaining in the buffer are freed if the value free up
 *     function given by the calling of the gallus_cbuffer_create()
 *     is not \b NULL.
 *
 *     @retval GALLUS_RESULT_OK                Succeeded.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
gallus_result_t
gallus_cbuffer_clear(gallus_cbuffer_t *cbptr,
                      bool free_values);


/**
 * Wake up all the waiters in a circular buffer.
 *
 *     @param[in]  cbptr	A pointer to a circular buffer.
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
gallus_cbuffer_wakeup(gallus_cbuffer_t *cbptr, gallus_chrono_t nsec);


/**
 * Wait for gettable.
 *
 *     @param[in]  cbptr	A pointer to a circular buffer.
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
gallus_cbuffer_wait_gettable(gallus_cbuffer_t *cbptr, 
                              gallus_chrono_t nsec);


/**
 * Wait for puttable.
 *
 *     @param[in]  cbptr	A pointer to a circular buffer.
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
gallus_cbuffer_wait_puttable(gallus_cbuffer_t *cbptr, 
                              gallus_chrono_t nsec);





gallus_result_t
gallus_cbuffer_put_with_size(gallus_cbuffer_t *cbptr,
                              void **valptr,
                              size_t valsz,
                              gallus_chrono_t nsec);
/**
 * Put an element at the tail of a circular buffer.
 *
 *     @param[in]  cbptr      A pointer to a circular buffer
 *     @param[in]  valptr     A pointer to an element.
 *     @param[in]  type       Type of a element.
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
#define gallus_cbuffer_put(cbptr, valptr, type, nsec)                  \
  gallus_cbuffer_put_with_size((cbptr), (void **)(valptr), sizeof(type), \
                                (nsec))


gallus_result_t
gallus_cbuffer_put_n_with_size(gallus_cbuffer_t *cbptr,
                                void **valptr,
                                size_t n_vals,
                                size_t valsz,
                                gallus_chrono_t nsec,
                                size_t *n_actual_put);
/**
 * Put elements at the tail of a circular buffer.
 *
 *     @param[in]  cbptr        A pointer to a circular buffer
 *     @param[in]  valptr       A pointer to elements.
 *     @param[in]  n_vals       A # of elements to put.
 *     @param[in]  type         A type of the element.
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
#define gallus_cbuffer_put_n(cbptr, valptr, n_vals, type, nsec,        \
                              n_actual_put)                             \
gallus_cbuffer_put_n_with_size((cbptr), (void **)(valptr),           \
                                (n_vals), sizeof(type), (nsec),       \
                                (n_actual_put))


gallus_result_t
gallus_cbuffer_get_with_size(gallus_cbuffer_t *cbptr,
                              void **valptr,
                              size_t valsz,
                              gallus_chrono_t nsec);
/**
 * Get the head element in a circular buffer.
 *
 *     @param[in]  cbptr      A pointer to a circular buffer
 *     @param[out] valptr     A pointer to an element.
 *     @param[in]  type       A type of the element.
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
#define gallus_cbuffer_get(cbptr, valptr, type, nsec)                  \
  gallus_cbuffer_get_with_size((cbptr), (void **)(valptr), sizeof(type), \
                                (nsec))


gallus_result_t
gallus_cbuffer_get_n_with_size(gallus_cbuffer_t *cbptr,
                                void **valptr,
                                size_t n_vals_max,
                                size_t n_at_least,
                                size_t valsz,
                                gallus_chrono_t nsec,
                                size_t *n_actual_get);
/**
 * Get elements from the head of a circular buffer.
 *
 *     @param[in]  cbptr        A pointer to a circular buffer.
 *     @param[out] valptr       A pointer to a element.
 *     @param[in]  n_vals_max   A maximum # of elemetns to get.
 *     @param[in]  n_at_least   A minimum # of elements to get until timeout.
 *     @param[in]  type         A type of a element.
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
 *     @details The \b valptr must point a sufficient size (\b sizeof(type) *
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
#define gallus_cbuffer_get_n(cbptr, valptr, n_vals_max, n_at_least,    \
                              type, nsec, n_actual_get)                 \
gallus_cbuffer_get_n_with_size((cbptr), (void **)(valptr),           \
                                (n_vals_max), (n_at_least),           \
                                sizeof(type), (nsec), (n_actual_get))


gallus_result_t
gallus_cbuffer_peek_with_size(gallus_cbuffer_t *cbptr,
                               void **valptr,
                               size_t valsz,
                               gallus_chrono_t nsec);
/**
 * Peek the head element of a circular buffer.
 *
 *     @param[in]  cbptr      A pointer to a circular buffer
 *     @param[out] valptr     A pointer to a element.
 *     @param[in]  type       A type of the element.
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
#define gallus_cbuffer_peek(cbptr, valptr, type, nsec)                \
  gallus_cbuffer_peek_with_size((cbptr), (void **)(valptr), sizeof(type), \
                                 (nsec))

gallus_result_t
gallus_cbuffer_peek_n_with_size(gallus_cbuffer_t *cbptr,
                                 void **valptr,
                                 size_t n_vals_max,
                                 size_t n_at_least,
                                 size_t valsz,
                                 gallus_chrono_t nsec,
                                 size_t *n_actual_get);
/**
 * Peek elements from the head of a circular buffer.
 *
 *     @param[in]  cbptr         A pointer to a circular buffer.
 *     @param[out] valptr        A pointer to a element.
 *     @param[in]  n_vals_max    A maximum # of elemetns to get.
 *     @param[in]  n_at_least    A minimum # of elements to get until timeout.
 *     @param[in]  type          A type of the element.
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
#define gallus_cbuffer_peek_n(cbptr, valptr, n_vals_max, n_at_least,   \
                               type, nsec, n_actual_peek)               \
gallus_cbuffer_peek_n_with_size((cbptr), (void **)(valptr),          \
                                 (n_vals_max), (n_at_least),          \
                                 sizeof(type), (nsec), (n_actual_peek))





/**
 * Get a # of elements in a circular buffer.
 *	@param[in]   cbptr    A pointer to a circular buffer
 *
 *	@retval	>=0	A # of elements in the circular buffer.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_cbuffer_size(gallus_cbuffer_t *cbptr);


/**
 * Get the remaining capacity of a circular buffer.
 *	@param[in]   cbptr    A pointer to a circular buffer
 *
 *	@retval	>=0	The remaining capacity of the circular buffer.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_cbuffer_remaining_capacity(gallus_cbuffer_t *cbptr);


/**
 * Get the maximum capacity of a circular buffer.
 *	@param[in]   cbptr    A pointer to a circular buffer
 *
 *	@retval	>=0	The maximum capacity of the circular buffer.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_cbuffer_max_capacity(gallus_cbuffer_t *cbptr);





/**
 * Returns \b true if the circular buffer is full.
 *
 *    @param[in]   cbptr    A pointer to a circular buffer.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_cbuffer_is_full(gallus_cbuffer_t *cbptr, bool *retptr);


/**
 * Returns \b true if the circular buffer is empty.
 *
 *    @param[in]   cbptr    A pointer to a circular buffer
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_cbuffer_is_empty(gallus_cbuffer_t *cbptr, bool *retptr);


/**
 * Returns \b true if the circular buffer is operational.
 *
 *    @param[in]   cbptr    A pointer to a circular buffer
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_cbuffer_is_operational(gallus_cbuffer_t *cbptr, bool *retptr);


/**
 * Cleanup an internal state of a circular buffer after thread
 * cancellation.
 *	@param[in]	cbptr	A pointer to a circular buffer
 */
void
gallus_cbuffer_cancel_janitor(gallus_cbuffer_t *cbptr);





__END_DECLS





#endif  /* ! __GALLUS_CBUFFER_H__ */
