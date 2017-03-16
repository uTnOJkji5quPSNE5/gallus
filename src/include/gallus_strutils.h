#ifndef __GALLUS_STRUTILS_H__
#define __GALLUS_STRUTILS_H__





/**
 *	@file	gallus_strutils.h
 */





__BEGIN_DECLS





gallus_result_t
gallus_str_tokenize_with_limit(char *buf, char **tokens,
                                size_t max, size_t limit, const char *delm);

#define gallus_str_tokenize(_buf, _tokens, _max, _delm) \
  gallus_str_tokenize_with_limit((_buf), (_tokens), (_max), 0, (_delm))

gallus_result_t
gallus_str_tokenize_quote(char *buf, char **tokens,
                           size_t max, const char *delm, const char *quote);

gallus_result_t
gallus_str_unescape(const char *org, const char *escaped,
                     char **retptr);

gallus_result_t
gallus_str_escape(const char *in_str, const char *escape_chars,
                   bool *is_escaped, char **out_str);

gallus_result_t
gallus_str_trim_right(const char *org, const char *trim_chars,
                       char **retptr);

gallus_result_t
gallus_str_trim_left(const char *org, const char *trimchars,
                      char **retptr);

gallus_result_t
gallus_str_trim(const char *org, const char *trimchars,
                 char **retptr);





gallus_result_t
gallus_str_parse_int16_by_base(const char *buf, int16_t *val,
                                unsigned int base);
gallus_result_t
gallus_str_parse_int16(const char *buf, int16_t *val);
gallus_result_t
gallus_str_parse_uint16_by_base(const char *buf, uint16_t *val,
                                 unsigned int base);
gallus_result_t
gallus_str_parse_uint16(const char *buf, uint16_t *val);





gallus_result_t
gallus_str_parse_int32_by_base(const char *buf, int32_t *val,
                                unsigned int base);
gallus_result_t
gallus_str_parse_int32(const char *buf, int32_t *val);
gallus_result_t
gallus_str_parse_uint32_by_base(const char *buf, uint32_t *val,
                                 unsigned int base);
gallus_result_t
gallus_str_parse_uint32(const char *buf, uint32_t *val);





gallus_result_t
gallus_str_parse_int64_by_base(const char *buf, int64_t *val,
                                unsigned int base);
gallus_result_t
gallus_str_parse_int64(const char *buf, int64_t *val);
gallus_result_t
gallus_str_parse_uint64_by_base(const char *buf, uint64_t *val,
                                 unsigned int base);
gallus_result_t
gallus_str_parse_uint64(const char *buf, uint64_t *val);





gallus_result_t
gallus_str_parse_float(const char *buf, float *val);


gallus_result_t
gallus_str_parse_double(const char *buf, double *val);


gallus_result_t
gallus_str_parse_long_double(const char *buf, long double *val);


gallus_result_t
gallus_str_parse_bool(const char *buf, bool *val);





gallus_result_t
gallus_str_indexof(const char *str1, const char *str2);





__END_DECLS





#endif /* ! __GALLUS_STRUTILS_H__ */
