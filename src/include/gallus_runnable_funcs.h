#ifndef __GALLUS_RUNNABLE_FUNCS_H__
#define __GALLUS_RUNNABLE_FUNCS_H__




/**
 *	@file	gallus_runnable_funcs.h
 */





#ifndef RUNNABLE_T_DECLARED
typedef struct gallus_runnable_record 	*gallus_runnable_t;
#define RUNNABLE_T_DECLARED
#endif /* ! RUNNABLE_T_DECLARED */





/**
 * A procedure for a runnable.
 *
 *	@param[in]	rptr	A runnable.
 *	@param[in]	arg	An argument.
 *
 * @details If any resoures acquired in the function must be released
 * before returning. Doing this must be the functions's responsibility.
 *
 * @details This function must not loop infinitely, but must return in
 * appropriate amount of execution time.
 */
typedef gallus_result_t (*gallus_runnable_proc_t)(
    const gallus_runnable_t *rptr,
    void *arg);


/**
 * Free a runnable up.
 *
 *	@param[in]	rptr	A pointer to a runnable.
 *
 * @details Don't free the \b *rptr itself up.
 */
typedef void (*gallus_runnable_freeup_proc_t)(gallus_runnable_t *rptr);





#endif /* ! __GALLUS_RUNNABLE_FUNCS_H__ */
