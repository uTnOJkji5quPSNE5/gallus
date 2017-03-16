#ifndef __GALLUS_CHRONO_H__
#define __GALLUS_CHRONO_H__





/**
 *	@file	gallus_chrono.h
 */





__BEGIN_DECLS





gallus_chrono_t
gallus_chrono_now(void);


gallus_result_t
gallus_chrono_to_timespec(struct timespec *dstptr,
                           gallus_chrono_t nsec);


gallus_result_t
gallus_chrono_to_timeval(struct timeval *dstptr,
                          gallus_chrono_t nsec);


gallus_result_t
gallus_chrono_from_timespec(gallus_chrono_t *dstptr,
                             const struct timespec *specptr);


gallus_result_t
gallus_chrono_from_timeval(gallus_chrono_t *dstptr,
                            const struct timeval *valptr);


gallus_result_t
gallus_chrono_nanosleep(gallus_chrono_t nsec,
                         gallus_chrono_t *remptr);


#ifdef __GNUC__
#if defined(GALLUS_CPU_X86_64) || defined(GALLUS_CPU_I386)
static inline uint64_t
gallus_rdtsc(void) {
  uint32_t eax, edx;
  __asm__ volatile ("rdtsc" : "=a" (eax), "=d" (edx));
  return (((uint64_t)edx) << 32) | ((uint64_t)eax);
}
#else
#warning reading TSC thingies is not supported on this platform.
static inline uint64_t
gallus_rdtsc(void) {
  gallus_msg_warning("reading TSC thingies is not supported on this "
                      "platform.\n");
  return 0LL;
}
#endif /* GALLUS_CPU_X86_64 || GALLUS_CPU_I386 */
#else
#warning reading TSC thingies is not supported with this compiler.
static inline uint64_t
gallus_rdtsc(void) {
  gallus_msg_warning("reading TSC thingies is not supported with "
                      "this compiler.\n");
  return 0LL;
}
#endif /* __GNUC__ */





__END_DECLS





#endif /* ! __GALLUS_CHRONO_H__ */
