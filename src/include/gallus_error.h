#ifndef __GALLUS_ERROR_H__
#define __GALLUS_ERROR_H__





/**
 *	@file	gallus_error.h
 */





#define GALLUS_RESULT_OK		0


/*
 * Note:
 *
 *	Add error #s after this and add messages that corresponds to
 *	the error #s into s_error_strs[] in lib/error.c, IN ORDER.
 */


#define GALLUS_RESULT_ANY_FAILURES	-1
#define GALLUS_RESULT_POSIX_API_ERROR	-2
#define GALLUS_RESULT_NO_MEMORY	-3
#define GALLUS_RESULT_NOT_FOUND	-4
#define GALLUS_RESULT_ALREADY_EXISTS	-5
#define GALLUS_RESULT_NOT_OPERATIONAL	-6
#define GALLUS_RESULT_INVALID_ARGS	-7
#define GALLUS_RESULT_NOT_OWNER	-8
#define GALLUS_RESULT_NOT_STARTED	-9
#define GALLUS_RESULT_TIMEDOUT		-10
#define GALLUS_RESULT_ITERATION_HALTED	-11
#define GALLUS_RESULT_OUT_OF_RANGE	-12
#define GALLUS_RESULT_NAN		-13
#define GALLUS_RESULT_OFP_ERROR	-14
#define GALLUS_RESULT_ALREADY_HALTED	-15
#define GALLUS_RESULT_INVALID_OBJECT	-16
#define GALLUS_RESULT_CRITICAL_REGION_NOT_CLOSED	-17
#define GALLUS_RESULT_CRITICAL_REGION_NOT_OPENED	-18
#define GALLUS_RESULT_INVALID_STATE_TRANSITION		-19
#define GALLUS_RESULT_SOCKET_ERROR		-20
#define GALLUS_RESULT_BUSY		-21
#define GALLUS_RESULT_STOP		-22
#define GALLUS_RESULT_SNMP_API_ERROR		-23
#define GALLUS_RESULT_TLS_CONN_ERROR		-24
#define GALLUS_RESULT_EINPROGRESS		-25
#define GALLUS_RESULT_OXM_ERROR		-26
#define GALLUS_RESULT_UNSUPPORTED	-27
#define GALLUS_RESULT_QUOTE_NOT_CLOSED		-28
#define GALLUS_RESULT_NOT_ALLOWED		-29
#define GALLUS_RESULT_NOT_DEFINED	-30
#define GALLUS_RESULT_WAKEUP_REQUESTED	-31
#define GALLUS_RESULT_TOO_MANY_OBJECTS	-32
#define GALLUS_RESULT_DATASTORE_INTERP_ERROR	-33
#define GALLUS_RESULT_EOF		-34
#define GALLUS_RESULT_NO_MORE_ACTION		-35
#define GALLUS_RESULT_TOO_LARGE	-36
#define GALLUS_RESULT_TOO_SMALL	-37
#define GALLUS_RESULT_TOO_LONG		-38
#define GALLUS_RESULT_TOO_SHORT	-39
#define GALLUS_RESULT_ADDR_RESOLVER_FAILURE		-40
#define GALLUS_RESULT_OUTPUT_FAILURE	-41
#define GALLUS_RESULT_INVALID_STATE	-42
#define GALLUS_RESULT_INVALID_NAMESPACE	-43
#define GALLUS_RESULT_INTERRUPTED	-44





__BEGIN_DECLS


/**
 * Get a human readable error message from an API result code.
 *
 *	@param[in]	err	A result code.
 *
 *	@returns	A human readable error message.
 */
const char 	*gallus_error_get_string(gallus_result_t err);


__END_DECLS





#endif /* ! __GALLUS_ERROR_H__ */
