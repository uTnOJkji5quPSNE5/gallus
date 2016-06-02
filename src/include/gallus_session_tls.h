#ifndef __GALLUS_SESSION_TLS_H__
#define __GALLUS_SESSION_TLS_H__





__BEGIN_DECLS


/**
 * Set a certificate store in a session_tls.
 *
 *  @param[in] new_certstore    A string pointer as the certificate directory.
 *
 *  @retval	GALLUS_RESULT_OK	Succeeded.
 *  @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	GALLUS_RESULT_NO_MEMORY Failed, no memory.
 *  @retval GALLUS_RESULT_NOT_ALLOWED Failed, permision denied.
 *  @retval GALLUS_RESULT_OUT_OF_RANGE Failed, too long.
 *  @retval GALLUS_RESULT_POSIX_API_ERROR Failed, posix API error.
 *
 *  @details Both of a NULL pointer and a NULL character cannot be set.
 *
 */
gallus_result_t
gallus_session_tls_set_ca_dir(const char *c);

/**
 * Set a certificate file for client in a session_tls.
 *
 *  @param[in] new_certfile    A string pointer as the certificate file.
 *
 *  @retval	GALLUS_RESULT_OK	Succeeded.
 *  @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	GALLUS_RESULT_NO_MEMORY Failed, no memory.
 *  @retval GALLUS_RESULT_NOT_ALLOWED Failed, permision denied.
 *  @retval GALLUS_RESULT_OUT_OF_RANGE Failed, too long.
 *  @retval GALLUS_RESULT_POSIX_API_ERROR Failed, posix API error.
 *
 *  @details Both of a NULL pointer and a NULL character cannot be set.
 *
 */
gallus_result_t
gallus_session_tls_set_client_cert(const char *c);

/**
 * Set a certificate file for server in a session_tls.
 *
 *  @param[in] new_certfile    A string pointer as the certificate file.
 *
 *  @retval	GALLUS_RESULT_OK	Succeeded.
 *  @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	GALLUS_RESULT_NO_MEMORY Failed, no memory.
 *  @retval GALLUS_RESULT_NOT_ALLOWED Failed, permision denied.
 *  @retval GALLUS_RESULT_OUT_OF_RANGE Failed, too long.
 *  @retval GALLUS_RESULT_POSIX_API_ERROR Failed, posix API error.
 *
 *  @details Both of a NULL pointer and a NULL character cannot be set.
 *
 */
gallus_result_t
gallus_session_tls_set_server_cert(const char *c);

/**
 * Set a private key for client in a session_tls.
 *
 *  @param[in] new_pvtkey    A string pointer as the private key.
 *
 *  @retval	GALLUS_RESULT_OK	Succeeded.
 *  @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	GALLUS_RESULT_NO_MEMORY Failed, no memory.
 *  @retval GALLUS_RESULT_NOT_ALLOWED Failed, permision denied.
 *  @retval GALLUS_RESULT_OUT_OF_RANGE Failed, too long.
 *  @retval GALLUS_RESULT_POSIX_API_ERROR Failed, posix API error.
 *
 *  @details Both of a NULL pointer and a NULL character cannot be set.
 *
 */
gallus_result_t
gallus_session_tls_set_client_key(const char *c);

/**
 * Set a private key for server in a session_tls.
 *
 *  @param[in] new_pvtkey    A string pointer as the private key.
 *
 *  @retval	GALLUS_RESULT_OK	Succeeded.
 *  @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	GALLUS_RESULT_NO_MEMORY Failed, no memory.
 *  @retval GALLUS_RESULT_NOT_ALLOWED Failed, permision denied.
 *  @retval GALLUS_RESULT_OUT_OF_RANGE Failed, too long.
 *  @retval GALLUS_RESULT_POSIX_API_ERROR Failed, posix API error.
 *
 *  @details Both of a NULL pointer and a NULL character cannot be set.
 *
 */
gallus_result_t
gallus_session_tls_set_server_key(const char *c);

/**
 * Set a trustpoint configuration in a session_tls.
 *
 *  @param[in] new_tpconf    A string pointer as the trustpoint configuration.
 *
 *  @retval	GALLUS_RESULT_OK	Succeeded.
 *  @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	GALLUS_RESULT_NO_MEMORY Failed, no memory.
 *  @retval GALLUS_RESULT_NOT_ALLOWED Failed, permision denied.
 *  @retval GALLUS_RESULT_OUT_OF_RANGE Failed, too long.
 *  @retval GALLUS_RESULT_POSIX_API_ERROR Failed, posix API error.
 *
 *  @details Both of a NULL pointer and a NULL character cannot be set.
 *
 */
gallus_result_t
gallus_session_tls_set_trust_point_conf(const char *c);

/**
 * Get a certificate store in a session_tls.
 *
 *  @param[in]  c       A double pointer to a string.
 *  @param[out]  c       The pointer as certificate directory.
 *
 *  @retval	GALLUS_RESULT_OK	Succeeded.
 *  @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *
 */
gallus_result_t
gallus_session_tls_get_ca_dir(char **c);

/**
 * Get a certificate file for client in a session_tls.
 *
 *  @param[in]  c       A double pointer to a string.
 *  @param[out]  c       The pointer as certificate file.
 *
 *  @retval	GALLUS_RESULT_OK	Succeeded.
 *  @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *
 */
gallus_result_t
gallus_session_tls_get_client_cert(char **c);

/**
 * Get a certificate file for server in a session_tls.
 *
 *  @param[in]  c       A double pointer to a string.
 *  @param[out]  c       The pointer as certificate file.
 *
 *  @retval	GALLUS_RESULT_OK	Succeeded.
 *  @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *
 */
gallus_result_t
gallus_session_tls_get_server_cert(char **c);

/**
 * Get a private key for client in a session_tls.
 *
 *  @param[in]  c       A double pointer to a string.
 *  @param[out]  c       The pointer as private key.
 *
 *  @retval	GALLUS_RESULT_OK	Succeeded.
 *  @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *
 */
gallus_result_t
gallus_session_tls_get_client_key(char **c);

/**
 * Get a private key for server in a session_tls.
 *
 *  @param[in]  c       A double pointer to a string.
 *  @param[out]  c       The pointer as private key.
 *
 *  @retval	GALLUS_RESULT_OK	Succeeded.
 *  @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *
 */
gallus_result_t
gallus_session_tls_get_server_key(char **c);

/**
 * Get a trustpoint configuration in a session_tls.
 *
 *  @param[in]  c       A double pointer to a string.
 *  @param[out]  c       The pointer as the trustpoint configuration.
 *
 *  @retval	GALLUS_RESULT_OK	Succeeded.
 *  @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *  @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *
 */
gallus_result_t
gallus_session_tls_get_trust_point_conf(char **c);

/**
 * Set default trust point check function.
 *
 *  @param[in]   s     A session.
 *  @param[in]   func  trust point check function.
 *
 */
void
gallus_session_tls_set_certcheck_default(gallus_result_t
    (*func)(const char *, const char *));

/**
 * Set trust point check function to a session.
 *
 *  @param[in]   s     A session.
 *  @param[in]   func  trust point check function for this session.
 *
 */
void
gallus_session_tls_set_certcheck(gallus_session_t s, gallus_result_t
                                  (*func)(const char *, const char *));


__END_DECLS





#endif /* ! __GALLUS_SESSION_TLS_H__ */
