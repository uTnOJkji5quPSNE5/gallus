#ifndef __GALLUS_LOGGER_H__
#define __GALLUS_LOGGER_H__





/**
 * @file	gallus_logger.h
 */





typedef enum {
  GALLUS_LOG_LEVEL_UNKNOWN = 0,
  GALLUS_LOG_LEVEL_DEBUG,
  GALLUS_LOG_LEVEL_INFO,
  GALLUS_LOG_LEVEL_NOTICE,
  GALLUS_LOG_LEVEL_WARNING,
  GALLUS_LOG_LEVEL_ERROR,
  GALLUS_LOG_LEVEL_FATAL
} gallus_log_level_t;


typedef enum {
  GALLUS_LOG_EMIT_TO_UNKNOWN = 0,
  GALLUS_LOG_EMIT_TO_FILE,
  GALLUS_LOG_EMIT_TO_SYSLOG
} gallus_log_destination_t;





__BEGIN_DECLS


/**
 * Initialize the logger.
 *
 *	@param[in]	dst	Where to log;
 *	\b GALLUS_LOG_EMIT_TO_UNKNOWN: stderr,
 *	\b GALLUS_LOG_EMIT_TO_FILE: Any regular file,
 *	\b GALLUS_LOG_EMIT_TO_SYSLOG: syslog
 *	@param[in]	arg	For \b GALLUS_LOG_EMIT_TO_FILE: a file name,
 *	for \b GALLUS_LOG_EMIT_TO_SYSLOG: An identifier for syslog.
 *	@param[in]	multi_process	If the \b dst is
 *	\b GALLUS_LOG_EMIT_TO_FILE, use \b true if the application shares
 *	the log file between child processes.
 *	@param[in]	emit_date	Use \b true if date is needed in each
 *	line header.
 *	@param[in]	debug_level	A debug level.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_log_initialize(gallus_log_destination_t dst,
                       const char *arg,
                       bool multi_process,
                       bool emit_date,
                       uint16_t debug_level);


/**
 * Re-initialize the logger.
 *
 *	@details Calling this function implies 1) close opened log
 *	file. 2) re-open the log file, convenient for the log rotation.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t	gallus_log_reinitialize(void);


/**
 * Finalize the logger.
 */
void	gallus_log_finalize(void);


/**
 * Set the debug level.
 *
 *	@param[in]	lvl	A debug level.
 */
void	gallus_log_set_debug_level(uint16_t lvl);


/**
 * Get the debug level.
 *
 *	@returns	The debug level.
 */
uint16_t	gallus_log_get_debug_level(void);


/**
 * Check where the log is emitted to.
 *
 *	@param[out]	arg	A pointer to the argument that was passed via the \b gallus_log_get_destination(). (NULL allowed.)
 *
 *	@returns The log destination.
 *
 *	@details If the \b arg is specified as non-NULL pointer, the
 *	returned \b *arg must not be free()'d nor modified.
 */
gallus_log_destination_t
gallus_log_get_destination(const char **arg);


/**
 * Set/Unset multi-process mode
 *
 *	@param[in]	v	\btrue multi-process, \bfalse single-process.
 */
void
gallus_log_set_multi_process(bool v);


/**
 * Get current multi-process mode
 *
 *	@param[in]	v	a pointer to return value.
 *
 *	@returns Always GALLUS_REULT_OK
 */
gallus_result_t
gallus_log_get_multi_process(bool *v);


/**
 * The main logging workhorse: not intended for direct use.
 */
void	gallus_log_emit(gallus_log_level_t log_level,
                       uint64_t debug_level,
                       const char *file,
                       int line,
                       const char *func,
                       const char *fmt, ...)
__attr_format_printf__(6, 7);


__END_DECLS





#ifdef __GNUC__
#define __PROC__	__PRETTY_FUNCTION__
#else
#define	__PROC__	__func__
#endif /* __GNUC__ */


/**
 * Emit a debug message to the log.
 *
 *	@param[in]	level	A debug level (int).
 */
#define gallus_msg_debug(level, ...) \
  gallus_log_emit(GALLUS_LOG_LEVEL_DEBUG, (uint64_t)(level), \
                   __FILE__, __LINE__, __PROC__, __VA_ARGS__)


/**
 * Emit an informative message to the log.
 */
#define gallus_msg_info(...) \
  gallus_log_emit(GALLUS_LOG_LEVEL_INFO, 0LL, __FILE__, __LINE__, \
                   __PROC__, __VA_ARGS__)


/**
 * Emit a notice message to the log.
 */
#define gallus_msg_notice(...) \
  gallus_log_emit(GALLUS_LOG_LEVEL_NOTICE, 0LL, __FILE__, __LINE__, \
                   __PROC__, __VA_ARGS__)


/**
 * Emit a warning message to the log.
 */
#define gallus_msg_warning(...) \
  gallus_log_emit(GALLUS_LOG_LEVEL_WARNING, 0LL, __FILE__, __LINE__, \
                   __PROC__, __VA_ARGS__)


/**
 * Emit an error message to the log.
 */
#define gallus_msg_error(...) \
  gallus_log_emit(GALLUS_LOG_LEVEL_ERROR, 0LL, __FILE__, __LINE__, \
                   __PROC__, __VA_ARGS__)


/**
 * Emit a fatal message to the log.
 */
#define gallus_msg_fatal(...) \
  gallus_log_emit(GALLUS_LOG_LEVEL_FATAL, 0LL, __FILE__, __LINE__, \
                   __PROC__, __VA_ARGS__)


/**
 * Emit an arbitarary message to the log.
 */
#define gallus_msg(...) \
  gallus_log_emit(GALLUS_LOG_LEVEL_UNKNOWN, 0LL, __FILE__, __LINE__, \
                   __PROC__, __VA_ARGS__)


/**
 * The minimum level debug emitter.
 */
#define gallus_dprint(...) \
  gallus_msg_debug(1LL, __VA_ARGS__)


/**
 * Emit a readable error message for the errornous result.
 *
 *	@param[in]	s	A result code (gallus_result_t)
 */
#define gallus_perror(s) \
  do {                                                                  \
    (s == GALLUS_RESULT_POSIX_API_ERROR) ?                             \
    gallus_msg_error("GALLUS_RESULT_POSIX_API_ERROR: %s.\n",      \
                      strerror(errno)) :                            \
    gallus_msg_error("%s.\n", gallus_error_get_string((s)));      \
  } while (0)


/**
 * Emit an error message and exit.
 *
 *	@param[in]	ecode	An exit code (int)
 */
#define gallus_exit_error(ecode, ...) {        \
    gallus_msg_error(__VA_ARGS__);             \
    exit(ecode);                                \
  }


/**
 * Emit a fatal message and abort.
 */
#define gallus_exit_fatal(...) {                   \
    gallus_msg_fatal(__VA_ARGS__);                 \
    abort();                                        \
  }





#endif /* ! __GALLUS_LOGGER_H__ */
