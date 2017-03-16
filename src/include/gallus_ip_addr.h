/**
 * @file	gallus_ip_addr.h
 */

#ifndef __GALLUS_IP_ADDR_H__
#define __GALLUS_IP_ADDR_H__

#define GALLUS_ADDR_STR_MAX NI_MAXHOST





__BEGIN_DECLS





/**
 * @brief	gallus_ip_address_t
 */
typedef struct ip_address gallus_ip_address_t;

/**
 * Create a gallus_ip_address_t.
 *
 *     @param[in]	name	Host name.
 *     @param[in]	is_ipv4_addr	IPv4 flag.
 *     @param[out]	ip	A pointer to a \e gallus_ip_address_t structure.
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 */
gallus_result_t
gallus_ip_address_create(const char *name, bool is_ipv4_addr,
                          gallus_ip_address_t **ip);

/**
 * Destroy a gallus_ip_address_t
 *
 *     @param[in]	ip	A pointer to a \e gallus_ip_address_t structure.
 *
 *     @retval	void
 */
void
gallus_ip_address_destroy(gallus_ip_address_t *ip);

/**
 * Copy a gallus_ip_address_t.
 *
 *     @param[in]	name	Host name.
 *     @param[in]	src	A pointer to a \e gallus_ip_address_t structure (src).
 *     @param[out]	dst	A pointer to a \e gallus_ip_address_t structure (dst).
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 */
gallus_result_t
gallus_ip_address_copy(const gallus_ip_address_t *src,
                        gallus_ip_address_t **dst);

/**
 * Equals a gallus_ip_address_t.
 *
 *     @param[in]	ip1	A pointer to a \e gallus_ip_address_t structure (src).
 *     @param[in]	ip2	A pointer to a \e gallus_ip_address_t structure (dst).
 *
 *     @retval  true/false
 */
bool
gallus_ip_address_equals(const gallus_ip_address_t *ip1,
                          const gallus_ip_address_t *ip2);

/**
 * Get IP addr string.
 *
 *     @param[in]	ip	A pointer to a \e gallus_ip_address_t structure.
 *     @param[out]	addr_str	IP addr string..
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 */
gallus_result_t
gallus_ip_address_str_get(const gallus_ip_address_t *ip,
                           char **addr_str);

/**
 * Get sockaddr structure.
 *
 *     @param[in]	ip	A pointer to a \e gallus_ip_address_t structure.
 *     @param[out]	saddr	A pointer to a \e sockaddr structure
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 */
gallus_result_t
gallus_ip_address_sockaddr_get(const gallus_ip_address_t *ip,
                                struct sockaddr **saddr);

/**
 * Get length of sockaddr structure.
 *
 *     @param[in]	ip	A pointer to a \e gallus_ip_address_t structure.
 *     @param[out]	saddr_len	A pointer to a \e length of sockaddr structure
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
gallus_result_t
gallus_ip_address_sockaddr_len_get(const gallus_ip_address_t *ip,
                                    socklen_t *saddr_len);


/**
 * Is IPv4.
 *
 *     @param[in]	ip	A pointer to a \e gallus_ip_address_t structure.
 *     @param[out]	is_ipv4	A pointer to a \e is_ipv4.
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
gallus_result_t
gallus_ip_address_is_ipv4(const gallus_ip_address_t *ip,
                           bool *is_ipv4);





__END_DECLS





#endif /* __GALLUS_IP_ADDR_H__ */
