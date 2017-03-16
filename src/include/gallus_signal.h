#ifndef __GALLUS_SIGNAL_H__
#define __GALLUS_SIGNAL_H__





/**
 *	@file gallus_signal.h
 */





__BEGIN_DECLS





#ifdef __FreeBSD__
#define sighandler_t sig_t
#endif /* __FreeBSD__ */
#ifdef __NetBSD__
typedef void (*sighandler_t)(int);
#endif /* __NetBSD__ */


#define SIG_CUR	__s_I_g_C_u_R__





void	__s_I_g_C_u_R__(int sig);





/**
 * A MT-Safe signal(2).
 *
 *	@param[in]	signum	A signal.
 *	@param[in]	newhdlr	A new signal handler.
 *	@param[out]	oldhdlrptr	A pointer to the current/old handler.
 *
 *	@retval	GALLUS_RSULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_POSIX_API_ERROR	Failed, POSIX error.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details if the \b newhdlr is \b SIG_CUR, the API just returns the
 *	old signal handler to \b *oldptr.
 */
gallus_result_t
gallus_signal(int signum, sighandler_t newhdlr, sighandler_t *oldhdlrptr);


/**
 * Fallback the signal handling mechanism to the good-old-school
 * semantics.
 */
void	gallus_signal_old_school_semantics(void);





__END_DECLS





#endif /* ! __GALLUS_SIGNAL_H__ */
