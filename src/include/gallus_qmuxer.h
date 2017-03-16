#ifndef __GALLUS_QMUXER_H__
#define __GALLUS_QMUXER_H__





/**
 * @file gallus_qmuxer.h
 */





__BEGIN_DECLS





typedef struct gallus_cbuffer_record 	*gallus_cbuffer_t;
#define __GALLUS_CBUFFER_T_DEFINED__
typedef struct gallus_qmuxer_record *gallus_qmuxer_t;
#define __GALLUS_QMUXER_T_DEFINED__
typedef gallus_cbuffer_t gallus_bbq_t;
#define __GALLUS_BBQ_T_DEFINED__


typedef struct gallus_qmuxer_poll_record *gallus_qmuxer_poll_t;
#define __GALLUS_QMUXER_POLL_T_DEFINED__


typedef enum {
  GALLUS_QMUXER_POLL_UNKNOWN = 0,
  GALLUS_QMUXER_POLL_READABLE = 0x1,
  GALLUS_QMUXER_POLL_WRITABLE = 0x2,
  GALLUS_QMUXER_POLL_BOTH = 0x3
} gallus_qmuxer_poll_event_t;





/**
 * Create a queue muxer.
 *
 *	@param[in,out]	qmxptr	A pointer to a queue muxer to be created.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_qmuxer_create(gallus_qmuxer_t *qmxptr);


/**
 * Destroy a queue muxer.
 *
 *	@param[in]	qmxptr	A pointer to a queue muxer to be destroyed.
 */
void
gallus_qmuxer_destroy(gallus_qmuxer_t *qmxptr);





/**
 * Create a polling object.
 *
 *	@param[in,out]	mpptr	A pointer to a polling onject to be created.
 *	@param[in]	bbq	A queue to be polled.
 *	@param[in]	type	A type of event to poll;
 *	\b GALLUS_QMUXER_POLL_READABLE ) poll the queue readable;
 *	\b GALLUS_QMUXER_POLL_WRITABLE ) poll the queue writable;
 *	\b GALLUS_QMUXER_POLL_BOTH ) poll the queue readable and/or writable.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_qmuxer_poll_create(gallus_qmuxer_poll_t *mpptr,
                           gallus_bbq_t bbq,
                           gallus_qmuxer_poll_event_t type);


/**
 * Destroy a polling object.
 *
 *	@param[in]	mpptr	A pointer to a polling object to be destroyed.
 */
void
gallus_qmuxer_poll_destroy(gallus_qmuxer_poll_t *mpptr);


/**
 * Reset internal status of a polling object.
 *
 *	@param[in]	mpptr	A pointer to a polling object to be reset.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_qmuxer_poll_reset(gallus_qmuxer_poll_t *mpptr);


/**
 * Set a queue to a polling object.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *	@param[in]	bbq	A queue to be polled (\b NULL allowed).
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b bbq is \b NULL, the polling object is not
 *	used as an polling object and just avoided to be polled even
 *	the polling object is on the first parameter for the \b
 *	gallus_qmuxer_poll()
 */
gallus_result_t
gallus_qmuxer_poll_set_queue(gallus_qmuxer_poll_t *mpptr,
                              gallus_bbq_t bbq);


/**
 * Get a queue from a polling object.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *	@param[in]	bbqptr	A pointer to a queue to be returned.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details I have no guts to unify all the setter/getter APIs'
 *	parameter at this moment. Don't get me wrong.
 */
gallus_result_t
gallus_qmuxer_poll_get_queue(gallus_qmuxer_poll_t *mpptr,
                              gallus_bbq_t *bbqptr);


/**
 * Set a polling event type of a polling onject.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *	@param[in]	type	A type of event to poll;
 *	\b GALLUS_QMUXER_POLL_READABLE ) poll the queue readable;
 *	\b GALLUS_QMUXER_POLL_WRITABLE ) poll the queue writable;
 *	\b GALLUS_QMUXER_POLL_BOTH ) poll the queue readable and/or writable.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_qmuxer_poll_set_type(gallus_qmuxer_poll_t *mpptr,
                             gallus_qmuxer_poll_event_t type);


/**
 * Returns # of the values in the queue of a polling ofject.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *
 *	@retval	>=0	A # of values in the queue.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_qmuxer_poll_size(gallus_qmuxer_poll_t *mpptr);


/**
 * Returns the remaining capacity of the queue in a polling ofject.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *
 *	@retval	>=0	A # of values in the queue.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_qmuxer_poll_remaining_capacity(gallus_qmuxer_poll_t *mpptr);


/**
 * Wait for an event on any specified poll objects.
 *
 *	@param[in]	polls	An array of pointer of poll objects.
 *	@param[in]	npolls	A # of the poll objects.
 *	@param[in]	nsec	Time to block (in nsec).
 *
 *	@retval	> 0	A # of poll objects having event.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details Note: for performance, we don't take a giant lock
 *	that blocks all the operations of queues in the poll objects
 *	when checkcing events. Because of this, there is slight
 *	possibility of dropping events. In order to avoid this, you
 *	better set appropriate timeout value in \b nsec even
 *	specifying a negative value to the \b nsec is allowed.
 *
 *	@details This API doesn't return zero/GALLUS_RESULT_OK.
 */
gallus_result_t
gallus_qmuxer_poll(gallus_qmuxer_t *qmxptr,
                    gallus_qmuxer_poll_t const polls[],
                    size_t npolls,
                    gallus_chrono_t nsec);




/**
 * Cleanup an internal state of a qmuxer after thread
 * cancellation.
 *	@param[in]	qmxptr	A pointer to a qmuxer
 */
void
gallus_qmuxer_cancel_janitor(gallus_qmuxer_t *qmxptr);





__END_DECLS





#endif  /* ! __GALLUS_QMUXER_H__ */
