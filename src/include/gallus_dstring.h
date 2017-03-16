/**
 * @file	gallus_dstring.h
 */

#ifndef __GALLUS_DSTRING_H__
#define __GALLUS_DSTRING_H__





__BEGIN_DECLS





/**
 * @brief	gallus_dstring_t
 */
typedef struct dstring *gallus_dstring_t;

/**
 * Create a dynamic string.
 *
 *     @param[out]	ds	A pointer to a dynamic string to be created.
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 */
gallus_result_t
gallus_dstring_create(gallus_dstring_t *ds);

/**
 * Destroy a dynamic string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *
 *     @retval	void
 */
void
gallus_dstring_destroy(gallus_dstring_t *ds);

/**
 * Clear a dynamic string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
gallus_result_t
gallus_dstring_clear(gallus_dstring_t *ds);

/**
 * Append a dynamic string according to a format for \e va_list.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	format	A pointer to a format string.
 *     @param[in]	args	A pointer to a \e va_list.
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	GALLUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
gallus_result_t
gallus_dstring_vappendf(gallus_dstring_t *ds, const char *format,
                         va_list *args) __attr_format_printf__(2, 0);

/**
 * Append a dynamic string according to a format.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	format	A pointer to a format string.
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	GALLUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
gallus_result_t
gallus_dstring_appendf(gallus_dstring_t *ds, const char *format, ...)
__attr_format_printf__(2, 3);

/**
 * Prepend a dynamic string according to a format for \e va_list.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	format	A pointer to a format string.
 *     @param[in]	args	A pointer to a \e va_list.
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	GALLUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
gallus_result_t
gallus_dstring_vprependf(gallus_dstring_t *ds, const char *format,
                          va_list *args) __attr_format_printf__(2, 0);

/**
 * repend a dynamic string according to a format.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	format	A pointer to a format string.
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	GALLUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
gallus_result_t
gallus_dstring_prependf(gallus_dstring_t *ds, const char *format, ...)
    __attr_format_printf__(2, 3);

/**
 * Insert a dynamic string according to a format for \e va_list.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	offset	offset.
 *     @param[in]	format	A pointer to a format string.
 *     @param[in]	args	A pointer to a \e va_list.
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	GALLUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
gallus_result_t
gallus_dstring_vinsertf(gallus_dstring_t *ds,
                         size_t offset,
                         const char *format,
                         va_list *args) __attr_format_printf__(3, 0);

/**
 * Prepend a dynamic string according to a format.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	offset	offset.
 *     @param[in]	format	A pointer to a format string.
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	GALLUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
gallus_result_t
gallus_dstring_insertf(gallus_dstring_t *ds,
                        size_t offset,
                        const char *format, ...)
    __attr_format_printf__(3, 4);


/**
 * Concatenate dynamic strings.
 *
 *     @param[in,out]	dst_ds	A pointer to a dynamic string(dst).
 *     @param[in,out]	src_ds	A pointer to a dynamic string(src).
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	GALLUS_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
gallus_result_t
gallus_dstring_concat(gallus_dstring_t *dst_ds,
                       const gallus_dstring_t *src_ds);

/**
 * Get string from a dynamic string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[out]	ds	A pointer to a string.
 *
 *     @retval	GALLUS_RESULT_OK	Succeeded.
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
gallus_result_t
gallus_dstring_str_get(gallus_dstring_t *ds, char **str);

/**
 * Get size of a string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *
 *     @retval	>=0	Size of a string
 *     @retval	GALLUS_RESULT_ANY_FAILURES	Failed.
 *     @retval	GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
gallus_result_t
gallus_dstring_len_get(gallus_dstring_t *ds);

/**
 * A dynamic string is empty.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *
 *     @retval	true/false
 */
bool
gallus_dstring_empty(gallus_dstring_t *ds);





__END_DECLS





#endif /* __GALLUS_DSTRING_H__ */
