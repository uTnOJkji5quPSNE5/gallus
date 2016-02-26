#ifndef __GALLUS_BBQ_H__
#define __GALLUS_BBQ_H__





/**
 * @file gallus_bbq.h
 */





#include "gallus_cbuffer.h"





#ifndef __GALLUS_BBQ_T_DEFINED__
typedef gallus_cbuffer_t gallus_bbq_t;
#endif /* ! __GALLUS_BBQ_T_DEFINED__ */


/**
 * @deprecated Existing just for a backward compatibility.
 */
#define GALLUS_BOUND_BLOCK_Q_DECL(name, type) gallus_bbq_t





/**
 * Create a bounded blocking queue.
 *
 *     @param[out] bbqptr         A pointer to a queue to be created.
 *     @param[in]  type           A type of a value of the queue.
 *     @param[in]  maxelem        A maximum # of the value the queue holds.
 *     @param[in]  proc           A value free up function (\b NULL allowed).
 *
 *     @retval GALLUS_RESULT_OK               Succeeded.
 *     @retval GALLUS_RESULT_NO_MEMORY        Failed, no memory.
 *     @retval GALLUS_RESULT_ANY_FAILURES     Failed.
 */
#define gallus_bbq_create(bbqptr, type, length, proc)        \
  gallus_cbuffer_create((bbqptr), type, (length), (proc))


/**
 * Shutdown a bounded blocking queue.
 *
 *    @param[in]  bbqptr    A pointer to a queue to be shutdown.
 *    @param[in]  free_values  If \b true, all the values
 *    remaining in the queue are freed if the value free up
 *    function given by the calling of the gallus_cbuffer_create()
 *    is not \b NULL.
 */
#define gallus_bbq_shutdown(bbqptr, free_values)       \
  gallus_cbuffer_shutdown((bbqptr), (free_values))


/**
 * Destroy a bounded blocking queue.
 *
 *    @param[in]  bbqptr    A pointer to a queue to be destroyed.
 *    @param[in]  free_values  If \b true, all the values
 *    remaining in the queue are freed if the value free up
 *    function given by the calling of the gallus_cbuffer_create()
 *    is not \b NULL.
 *
 *    @details if \b bbq is operational, shutdown it.
 */
#define gallus_bbq_destroy(bbqptr, free_values)       \
  gallus_cbuffer_destroy((bbqptr), (free_values))


/**
 * Clear a bounded blocking queue.
 *
 *     @param[in]  bbqptr       A pointer to a queue
 *     @param[in]  free_values  If \b true, all the values
 *     remaining in the queue are freed if the value free up
 *     function given by the calling of the gallus_cbuffer_create()
 *     is not \b NULL.
 *
 *     @retval GALLUS_RESULT_OK                Succeeded.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
#define gallus_bbq_clear(bbqptr, free_values)  \
  gallus_cbuffer_clear((bbqptr), (free_values))


/**
 * Wake up all the waiters in a bounded blocking queue.
 *
 *     @param[in]  bbqptr	A pointer to a queue
 *     @param[in]  nsec		Wait time (nanosec).
 *
 *     @retval GALLUS_RESULT_OK                Succeeded.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
#define gallus_bbq_wakeup(bbqptr, nsec) \
  gallus_cbuffer_wakeup((bbqptr), (nsec))


/**
 * Wait for gettable.
 *
 *     @param[in]  bbqptr	A pointer to a queue.
 *     @param[in]  nsec		Wait time (nanosec).
 *
 *     @retval >0				# of the gettable elements.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
#define gallus_bbq_wait_gettable(bbqptr, nsec) \
  gallus_cbuffer_wait_gettable((bbqptr), (nsec))


/**
 * Wait for puttable.
 *
 *     @param[in]  bbqptr	A pointer to a queue.
 *     @param[in]  nsec		Wait time (nanosec).
 *
 *     @retval >0				# of the puttable elements.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
#define gallus_bbq_wait_puttable(bbqptr, nsec) \
  gallus_cbuffer_wait_puttable((bbqptr), (nsec))





/**
 * Put a value into a bounded blocking queue.
 *
 *     @param[in]  bbqptr     A pointer to a queue.
 *     @param[in]  valptr     A pointer to a value.
 *     @param[in]  type       A type of the value.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval GALLUS_RESULT_OK                Succeeded.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
#define gallus_bbq_put(bbqptr, valptr, type, nsec)        \
  gallus_cbuffer_put((bbqptr), (valptr), type, (nsec))


/**
 * Put elements at the tail of a bounded blocking queue.
 *
 *     @param[in]  bbqptr       A pointer to a queue.
 *     @param[in]  valptr       A pointer to elements.
 *     @param[in]  n_vals       A # of elements to put.
 *     @param[in]  type         A type of the element.
 *     @param[in]  nsec         A Wait time (in nsec).
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
 *     of available rooms in the queue (it could be zero.)
 *
 *     @details If the \b nsec is greater than zero, it puts elements limited
 *     to a number of available rooms in the queue until the time specified
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
#define gallus_bbq_put_n(bbqptr, valptr, n_vals, type, nsec,           \
                          n_actual_put)                                 \
gallus_cbuffer_put_n((bbqptr), (void **)(valptr),                    \
                      (n_vals), type, (nsec),                         \
                      (n_actual_put))


/**
 * Get a value from a bounded blocking queue.
 *
 *     @param[in]  bbqptr     A pointer to a queue.
 *     @param[out] valptr     A pointer to a value.
 *     @param[in]  type       A type of the value.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval GALLUS_RESULT_OK                Succeeded.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
#define gallus_bbq_get(bbqptr, valptr, type, nsec)   \
  gallus_cbuffer_get((bbqptr), (valptr), type, (nsec))


/**
 * Get elements from the head of a bounded blocking queue.
 *
 *     @param[in]  bbqptr       A pointer to a queue.
 *     @param[out] valptr       A pointer to an element.
 *     @param[in]  n_vals_max   A maximum # of elemetns to get.
 *     @param[in]  n_at_least   A minimum # of elements to get until timeout.
 *     @param[in]  type         A type of the element.
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
 */
#define gallus_bbq_get_n(bbqptr, valptr, n_vals_max, n_at_least,       \
                          type, nsec, n_actual_get)                     \
gallus_cbuffer_get_n((bbqptr), (void **)(valptr),                    \
                      (n_vals_max), (n_at_least),                     \
                      type, (nsec), (n_actual_get))


/**
 * Peek the first value from a bounded blocking queue.
 *
 *     @param[in]  bbqptr     A pointer to a queue.
 *     @param[out] valptr     A pointer to a value.
 *     @param[in]  type       A type of the value.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval GALLUS_RESULT_OK                Succeeded.
 *     @retval GALLUS_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval GALLUS_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval GALLUS_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval GALLUS_RESULT_ANY_FAILURES      Failed.
 */
#define gallus_bbq_peek(bbqptr, valptr, type, nsec)            \
  gallus_cbuffer_peek((bbqptr), (valptr), type, (nsec))


/**
 * Peek elements from the head of a bounded blocking queue.
 *
 *     @param[in]  bbqptr        A pointer to a queue.
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
#define gallus_bbq_peek_n(bbqptr, valptr, n_vals_max, n_at_least,      \
                           type, nsec, n_actual_peek)                   \
gallus_cbuffer_peek_n((cbptr), (void **)(valptr),                    \
                       (n_vals_max), (n_at_least),                    \
                       sizeof(type), (nsec), (n_actual_peek))





/**
 * Get a # of values in a bounded blocking queue.
 *	@param[in]   bbqptr    A pointer to a queue.
 *
 *	@retval	>=0	A # of values in the queue.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
#define gallus_bbq_size(bbqptr)                \
  gallus_cbuffer_size((bbqptr))


/**
 * Get the remaining capacity of bounded blocking queue.
 *	@param[in]   bbqptr    A pointer to a queue.
 *
 *	@retval	>=0	The remaining capacity of the queue.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
#define gallus_bbq_remaining_capacity(bbqptr)        \
  gallus_cbuffer_remaining_capacity((bbqptr))


/**
 * Get the maximum capacity of bounded blocking queue.
 *	@param[in]   bbqptr    A pointer to a queue.
 *
 *	@retval	>=0	The maximum capacity of the queue.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
#define gallus_bbq_max_capacity(bbqptr)        \
  gallus_cbuffer_max_capacity((bbqptr))





/**
 * Returns \b true if the bounded blocking queue is full.
 *
 *    @param[in]   bbqptr   A pointer to a queue.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
#define gallus_bbq_is_full(bbqptr, retptr)     \
  gallus_cbuffer_is_full((bbqptr), (retptr))


/**
 * Returns \b true if the bounded blocking queue is empty.
 *
 *    @param[in]   bbqptr   A pointer to a queue.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
#define gallus_bbq_is_empty(bbqptr, retptr)    \
  gallus_cbuffer_is_empty((bbqptr), (retptr))


/**
 * Returns \b true if the bounded blocking queue is operational.
 *
 *    @param[in]   cbptr    A pointer to a queue.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
#define gallus_bbq_is_operational(bbqptr, retptr)    \
  gallus_cbuffer_is_operational((bbqptr), (retptr))


/**
 * Cleanup an internal state of a circular buffer after thread
 * cancellation.
 *	@param[in]	cbptr	A pointer to a circular buffer
 */
#define gallus_bbq_cancel_janitor(cbptr)       \
  gallus_cbuffer_cancel_janitor((cbptr))






#endif  /* ! __GALLUS_BBQ_H__ */
