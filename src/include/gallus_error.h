#ifndef __GALLUS_ERROR_H__
#define __GALLUS_ERROR_H__





/**
 *	@file	gallus_error.h
 */





#include "gallus_ecode.h"





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
