/* %COPYRIGHT% */

#ifndef __GALLUS_TYPES_H__
#define __GALLUS_TYPES_H__





/**
 *	@file	gallus_types.h
 */





/**
 * @details The result type.
 */
typedef int64_t gallus_result_t;


/**
 * @details The flat nano second expression of the time, mainly
 * acquired by \b clock_gettime(). For arithmetic operations, the sign
 * extension is needed.
 */
typedef	int64_t	gallus_chrono_t;





#endif /* ! __GALLUS_TYPES_H__ */
