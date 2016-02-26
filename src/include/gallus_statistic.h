#ifndef __GALLUS_STATISTIC_H__
#define __GALLUS_STATISTIC_H__





/**
 *	@file	gallus_statistic.h
 */





typedef struct gallus_statistic_struct	*gallus_statistic_t;





/**
 * Create a statistic.
 *
 *	@param[in,out]	sptr	A pointer to a statistic.
 *	@param[in]	name	Name of the statistic.
 *
 *	@retval	GALLUS_RESULT_OK		Suceeded.
 *	@retval GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_statistic_create(gallus_statistic_t *sptr, const char *name);


/**
 * Find a statistic by name.
 *
 *	@param[out]	sptr	A pointer to a statistic.
 *	@param[in]	name	Name of the statistic.
 *
 *	@retval	GALLUS_RESULT_OK		Suceeded.
 *	@retval GALLUS_RESULT_NOT_FOUND	Failed, not found.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_statistic_find(gallus_statistic_t *sptr, const char *name);


/**
 * Destroy a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 */
void
gallus_statistic_destroy(gallus_statistic_t *sptr);


/**
 * Destroy a statistic by name.
 *
 *	@param[in]	name	Name of the statistic.
 */
void
gallus_statistic_destroy_by_name(const char *name);


/**
 * Record a value to a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	val	A value.
 *
 *	@retval	GALLUS_RESULT_OK		Suceeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_statistic_record(gallus_statistic_t *sptr, int64_t val);


/**
 * Reset a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *
 *	@retval	GALLUS_RESULT_OK		Suceeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_statistic_reset(gallus_statistic_t *sptr);


/**
 * Acquire # of the sample from a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *
 *	@retval	>=0				# of the sample.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_statistic_sample_n(gallus_statistic_t *sptr);


/**
 * Acquire the minimum value from a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	valptr	A pointer to a value.
 *
 *	@retval	GALLUS_RESULT_OK		Suceeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_statistic_min(gallus_statistic_t *sptr, int64_t *valptr);


/**
 * Acquire the maximum value from a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	valptr	A pointer to a value.
 *
 *	@retval	GALLUS_RESULT_OK		Suceeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_statistic_max(gallus_statistic_t *sptr, int64_t *valptr);


/**
 * Acquire the average of a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	valptr	A pointer to a value.
 *
 *	@retval	GALLUS_RESULT_OK		Suceeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_statistic_average(gallus_statistic_t *sptr, double *valptr);


/**
 * Acquire the standard deviation of a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	valptr	A pointer to a value.
 *	@param[in]	is_ssd	\b true) returns the Sample Standard Deviation (a.k.a. Unbiased Standard Deviation) instead of the standard deviation;
 *
 *	@retval	GALLUS_RESULT_OK		Suceeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.

 */
gallus_result_t
gallus_statistic_sd(gallus_statistic_t *sptr, double *valptr, bool is_ssd);





#endif /* ! __GALLUS_STATISTIC_H__ */
