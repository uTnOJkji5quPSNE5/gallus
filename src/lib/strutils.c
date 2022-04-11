#include "gallus_apis.h"





#define skip_spaces(s)                                  \
  while (*(s) != '\0' && isspace((int)*(s)) != 0) {     \
    (s)++;                                              \
  }


#define trim_spaces(b, e)                             \
  while ((e) >= (b) && ((int)*(e) == '\0' ||          \
                        (isspace((int)*(e)) != 0))) { \
    *(e)-- = '\0';                                    \
  }





static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static bool s_is_inited = false;

static MP_INT s_0;
static MP_INT s_1;
static MP_INT s_neg1;

static MP_INT s_SHRT_MIN;
static MP_INT s_SHRT_MAX;
static MP_INT s_USHRT_MAX;

static MP_INT s_INT_MIN;
static MP_INT s_INT_MAX;
static MP_INT s_UINT_MAX;

static MP_INT s_LONG_MIN;
static MP_INT s_LONG_MAX;
static MP_INT s_ULONG_MAX;

static MP_INT s_LLONG_MIN;
static MP_INT s_LLONG_MAX;
static MP_INT s_ULLONG_MAX;

static MP_INT s_mult[8];
static MP_INT s_mult_i[8];
static gallus_hashmap_t s_mult_tbl;

static void s_ctors(void) __attr_constructor__(103);
static void s_dtors(void) __attr_destructor__(103);





static void
s_once_proc(void) {
  size_t i;
  size_t j;
  gallus_result_t r;
  void *v;
  char buf[64];

  if (mpz_init_set_str(&s_0, "0", 10) != 0) {
    gallus_exit_fatal("can't initialize '0' to an MP_INT.\n");
  }
  if (mpz_init_set_str(&s_1, "1", 10) != 0) {
    gallus_exit_fatal("can't initialize '1' to an MP_INT.\n");
  }
  if (mpz_init_set_str(&s_neg1, "-1", 10) != 0) {
    gallus_exit_fatal("can't initialize '-1' to an MP_INT.\n");
  }


  if ((r = gallus_hashmap_create(&s_mult_tbl,
                                  GALLUS_HASHMAP_TYPE_STRING, NULL)) !=
      GALLUS_RESULT_OK) {
    gallus_perror(r);
    gallus_exit_fatal("can't initialize an SI prefix multiplier table.\n");
  }

  for (i = 0; i < 8; i++) {
    if (mpz_init_set_str(&(s_mult[i]), "1000", 10) != 0) {
      gallus_exit_fatal("can't initialize '1000' to an MP_INT.\n");
    }
    if (mpz_init_set_str(&(s_mult_i[i]), "1024", 10) != 0) {
      gallus_exit_fatal("can't initialize '1024' to an MP_INT.\n");
    }
    for (j = 0; j < i; j++) {
      mpz_mul(&(s_mult[i]), &(s_mult[i]), &(s_mult[0]));
      mpz_mul(&(s_mult_i[i]), &(s_mult_i[i]), &(s_mult_i[0]));
    }
  }

  v = (void *)&(s_mult[0]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"k", &v, false);
  v = (void *)&(s_mult_i[0]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"ki", &v, false);

  v = (void *)&(s_mult[1]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"m", &v, false);
  v = &(s_mult_i[1]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"mi", &v, false);

  v = (void *)&(s_mult[2]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"g", &v, false);
  v = (void *)&(s_mult_i[2]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"gi", &v, false);

  v = (void *)&(s_mult[3]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"t", &v, false);
  v = (void *)&(s_mult_i[3]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"ti", &v, false);

  v = (void *)&(s_mult[4]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"p", &v, false);
  v = (void *)&(s_mult_i[4]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"pi", &v, false);

  v = (void *)&(s_mult[5]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"x", &v, false);
  v = (void *)&(s_mult_i[5]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"xi", &v, false);

  v = (void *)&(s_mult[6]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"z", &v, false);
  v = (void *)&(s_mult_i[6]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"zi", &v, false);

  v = (void *)&(s_mult[7]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"y", &v, false);
  v = (void *)&(s_mult_i[7]);
  (void)gallus_hashmap_add(&s_mult_tbl, (void *)"yi", &v, false);


  snprintf(buf, sizeof(buf), "%d", SHRT_MIN);
  if (mpz_init_set_str(&s_SHRT_MIN, buf, 10) != 0) {
    gallus_exit_fatal("can't initialize SHRT_MIN to an MP_INT.\n");
  }
  snprintf(buf, sizeof(buf), "%d", SHRT_MAX);
  if (mpz_init_set_str(&s_SHRT_MAX, buf, 10) != 0) {
    gallus_exit_fatal("can't initialize SHRT_MAX to an MP_INT.\n");
  }
  snprintf(buf, sizeof(buf), "%u", USHRT_MAX);
  if (mpz_init_set_str(&s_USHRT_MAX, buf, 10) != 0) {
    gallus_exit_fatal("can't initialize USHRT_MAX to an MP_INT.\n");
  }

  snprintf(buf, sizeof(buf), "%d", INT_MIN);
  if (mpz_init_set_str(&s_INT_MIN, buf, 10) != 0) {
    gallus_exit_fatal("can't initialize INT_MIN to an MP_INT.\n");
  }
  snprintf(buf, sizeof(buf), "%d", INT_MAX);
  if (mpz_init_set_str(&s_INT_MAX, buf, 10) != 0) {
    gallus_exit_fatal("can't initialize INT_MAX to an MP_INT.\n");
  }
  snprintf(buf, sizeof(buf), "%u", UINT_MAX);
  if (mpz_init_set_str(&s_UINT_MAX, buf, 10) != 0) {
    gallus_exit_fatal("can't initialize UINT_MAX to an MP_INT.\n");
  }

  snprintf(buf, sizeof(buf), "%ld", LONG_MIN);
  if (mpz_init_set_str(&s_LONG_MIN, buf, 10) != 0) {
    gallus_exit_fatal("can't initialize LONG_MIN to an MP_INT.\n");
  }
  snprintf(buf, sizeof(buf), "%ld", LONG_MAX);
  if (mpz_init_set_str(&s_LONG_MAX, buf, 10) != 0) {
    gallus_exit_fatal("can't initialize LONG_MAX to an MP_INT.\n");
  }
  snprintf(buf, sizeof(buf), "%lu", ULONG_MAX);
  if (mpz_init_set_str(&s_ULONG_MAX, buf, 10) != 0) {
    gallus_exit_fatal("can't initialize ULONG_MAX to an MP_INT.\n");
  }

  snprintf(buf, sizeof(buf), "%lld", LLONG_MIN);
  if (mpz_init_set_str(&s_LLONG_MIN, buf, 10) != 0) {
    gallus_exit_fatal("can't initialize LLONG_MIN to an MP_INT.\n");
  }
  snprintf(buf, sizeof(buf), "%lld", LLONG_MAX);
  if (mpz_init_set_str(&s_LLONG_MAX, buf, 10) != 0) {
    gallus_exit_fatal("can't initialize LLONG_MAX to an MP_INT.\n");
  }
  snprintf(buf, sizeof(buf), "%llu", ULLONG_MAX);
  if (mpz_init_set_str(&s_ULLONG_MAX, buf, 10) != 0) {
    gallus_exit_fatal("can't initialize ULLONG_MAX to an MP_INT.\n");
  }

  s_is_inited = true;
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();
  gallus_msg_debug(5, "The string utility module initialized.\n");
}


static inline void
s_final(void) {
  size_t i;

  mpz_clear(&s_0);
  mpz_clear(&s_1);
  mpz_clear(&s_neg1);

  for (i = 0; i < 8; i++) {
    mpz_clear(&(s_mult[i]));
    mpz_clear(&(s_mult_i[i]));
  }

  mpz_clear(&s_SHRT_MIN);
  mpz_clear(&s_SHRT_MAX);
  mpz_clear(&s_USHRT_MAX);

  mpz_clear(&s_INT_MIN);
  mpz_clear(&s_INT_MAX);
  mpz_clear(&s_UINT_MAX);

  mpz_clear(&s_LONG_MIN);
  mpz_clear(&s_LONG_MAX);
  mpz_clear(&s_ULONG_MAX);

  mpz_clear(&s_LLONG_MIN);
  mpz_clear(&s_LLONG_MAX);
  mpz_clear(&s_ULLONG_MAX);

  gallus_hashmap_destroy(&s_mult_tbl, true);
}


static void
s_dtors(void) {
  if (s_is_inited == true) {
    if (gallus_module_is_unloading() &&
        gallus_module_is_finalized_cleanly()) {
      s_final();

      gallus_msg_debug(5, "The string utility module finalized.\n");
    } else {
      gallus_msg_debug(10, "The string utility module is not finalized "
                    "because of module finalization problem.\n");
    }
  }
}





static inline const MP_INT *
s_get_prefix_multiplier(char *str) {
  const MP_INT *ret = NULL;
  size_t len = strlen(str);

  if (len > 0) {
    bool got_i = false;
    const char *key = NULL;
    char *endp;
    size_t l_c;
    bool do_trim_last = false;

    endp = &str[len];
    trim_spaces(str, endp);

    len = strlen(str);
    if (len < 1) {
      goto done;
    }

    if (((int)(str[len - 1])) == 'i' ||
        ((int)(str[len - 1])) == 'I') {
      got_i = true;
      len--;
      if (len < 1) {
        /*
         * Got only 'i'/'I'.
         */
        goto done;
      }
      endp = &(str[len]);
      *endp = '\0';
      trim_spaces(str, endp);
    }

    len = strlen(str);
    if (len < 1) {
      goto done;
    }
    endp = &str[len];
    trim_spaces(str, endp);

    l_c = len - 1;
    switch ((int)str[l_c]) {
      case 'K' : case 'k': {
        key = (got_i == true) ? "ki" : "k";
        do_trim_last = true;
        break;
      }
      case 'M' : case 'm': {
        key = (got_i == true) ? "mi" : "m";
        do_trim_last = true;
        break;
      }
      case 'G' : case 'g': {
        key = (got_i == true) ? "gi" : "g";
        do_trim_last = true;
        break;
      }
      case 'T' : case 't': {
        key = (got_i == true) ? "ti" : "t";
        do_trim_last = true;
        break;
      }
      case 'P' : case 'p': {
        key = (got_i == true) ? "pi" : "p";
        do_trim_last = true;
        break;
      }
      case 'X' : case 'x': {
        key = (got_i == true) ? "xi" : "x";
        do_trim_last = true;
        break;
      }
      case 'Z' : case 'z': {
        key = (got_i == true) ? "zi" : "z";
        do_trim_last = true;
        break;
      }
      case 'Y' : case 'y': {
        key = (got_i == true) ? "yi" : "y";
        do_trim_last = true;
        break;
      }
    }

    if (do_trim_last == true) {
      endp = &(str[l_c]);
      *endp = '\0';
      trim_spaces(str, endp);
    }
    if (strlen(str) < 1) {
      goto done;
    }

    if (key != NULL) {
      void *val = NULL;
      gallus_result_t r;
      if ((r = gallus_hashmap_find(&s_mult_tbl, (void *)key, &val)) ==
          GALLUS_RESULT_OK &&
          val != NULL) {
        ret = val;
      } else {
        gallus_exit_fatal("can't find a multiplier for '%s', "
                           "must not happen.\n", key);
      }
    } else {
      ret = &s_1;
    }
  }

done:
  return ret;
}


static inline bool
s_parse_bigint_by_base(const char *str, MP_INT *valptr, unsigned int base) {
  bool ret = false;
  bool is_neg = false;

  skip_spaces(str);
  if (IS_VALID_STRING(str) == true) {
    switch ((int)str[0]) {
      case '-': {
        is_neg = true;
        str++;
        break;
      }
      case '+': {
        str++;
        break;
      }
    }
  }

  skip_spaces(str);
  if (IS_VALID_STRING(str) == true) {
    char *buf = strdup(str);
    if (buf != NULL) {
      size_t len;
      if ((len = strlen(buf)) > 0) {
        const MP_INT *mul = NULL;
        char *endp = &(buf[len]);

        trim_spaces(buf, endp);
        mul = s_get_prefix_multiplier(buf);
        if (mul != NULL) {
          MP_INT tmp;
          if (mpz_init_set_str(&tmp, buf, (int)base) == 0) {
            mpz_mul(valptr, &tmp, mul);
            if (is_neg == true) {
              mpz_neg(valptr, valptr);
            }
            ret = true;
          }
          mpz_clear(&tmp);
        }
      }
      free((void *)buf);
    }
  }

  return ret;
}


static inline bool
s_parse_bigint(const char *str, MP_INT *valptr) {
  /*
   * str := str2 | str8 | str10 | str16
   * str2 :=
   *	[[:space:]]*[\-\+][0\\]b[[:space:]]*[01]+[[:space:]]*([kKmMgGtTpP]+[i]*)*
   * str8 :=
   *	[[:space:]]*[\-\+][\\]0[[:space:]]*[0-7]+[[:space:]]*([kKmMgGtTpP]+[i]*)*
   * str10 :=
   *	[[:space:]]*[\-\+][[:space:]]*[0-9]+[[:space:]]*([kKmMgGtTpP]+[i]*)*
   * str16 :=
   *	[[:space:]]*[\-\+][0\\]x[[:space:]]*[0-9a-fA-F]+[[:space:]]*([kKmMgGtTpP]+[i]*)*
   */

  bool ret = false;
  bool is_neg = false;
  unsigned int base = 10;

  skip_spaces(str);
  if (IS_VALID_STRING(str) == true) {
    switch ((int)str[0]) {
      case '-': {
        is_neg = true;
        str++;
        break;
      }
      case '+': {
        str++;
        break;
      }
    }
  }

  skip_spaces(str);
  if (strncasecmp(str, "0x", 2) == 0 ||
      strncasecmp(str, "\\x", 2) == 0) {
    base = 16;
    str += 2;
  } else if (strncasecmp(str, "\\0", 2) == 0) {
    base = 8;
    str += 2;
  } else if (strncasecmp(str, "\\b", 2) == 0) {
    base = 2;
    str += 2;
  } else if (str[0] == 'H' || str[0] == 'h') {
    base = 16;
    str += 1;
  } else if (str[0] == 'B' || str[0] == 'b') {
    base = 2;
    str += 1;
  }

  skip_spaces(str);
  if (IS_VALID_STRING(str) == true) {
    char *buf = strdup(str);
    if (buf != NULL) {
      if (s_parse_bigint_by_base(buf, valptr, base) == true) {
        if (is_neg == true) {
          mpz_neg(valptr, valptr);
        }
        ret = true;
      }
      free((void *)buf);
    }
  }

  return ret;
}


static inline bool
s_mpz_ge(MP_INT *z1, MP_INT *z2) {
  return ((mpz_cmp(z1, z2) > 0) || (mpz_cmp(z1, z2) == 0)) ?
         true : false;
}


static inline bool
s_mpz_le(MP_INT *z1, MP_INT *z2) {
  return ((mpz_cmp(z1, z2) < 0) || (mpz_cmp(z1, z2) == 0)) ?
         true : false;
}


static inline gallus_result_t
s_parse_bigint_by_base_in_range(const char *str, MP_INT *valptr,
                                unsigned int base,
                                MP_INT *lower, MP_INT *upper) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (s_parse_bigint_by_base(str, valptr, base) == true) {
    if (s_mpz_ge(valptr, lower) == true &&
        s_mpz_le(valptr, upper) == true) {
      ret = GALLUS_RESULT_OK;
    } else {
      ret = GALLUS_RESULT_OUT_OF_RANGE;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_parse_bigint_in_range(const char *str, MP_INT *valptr,
                        MP_INT *lower, MP_INT *upper) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (s_parse_bigint(str, valptr) == true) {
    if (s_mpz_ge(valptr, lower) == true &&
        s_mpz_le(valptr, upper) == true) {
      ret = GALLUS_RESULT_OK;
    } else {
      ret = GALLUS_RESULT_OUT_OF_RANGE;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





#ifdef GALLUS_ARCH_32_BITS
static inline uint64_t
s_bignum_2_uint64_t(MP_INT *v) {
  uint64_t ret;
  uint32_t lo, hi;
  MP_INT posv;
  MP_INT z_lo;
  MP_INT z_hi;
  bool is_neg = (mpz_sgn(v) < 0) ? true : false;

  mpz_init(&z_lo);
  mpz_init(&z_hi);
  if (is_neg == true) {
    mpz_init(&posv);
    mpz_neg(&posv, v);
    v = &posv;
  }

  mpz_tdiv_q_2exp(&z_hi, v, 32);
  mpz_tdiv_r_2exp(&z_lo, v, 32);

  hi = mpz_get_ui(&z_hi);
  lo = mpz_get_ui(&z_lo);
  mpz_clear(&z_hi);
  mpz_clear(&z_lo);

  ret = ((uint64_t)hi << 32) | (uint64_t)lo;

  if (is_neg == true) {
    ret--;
    ret = ~ret;
    mpz_clear(&posv);
  }

  return ret;
}
#endif /* GALLUS_ARCH_32_BITS */


gallus_result_t
gallus_str_parse_int64_by_base(const char *buf, int64_t *val,
                                unsigned int base) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL &&
      base > 1) {
    MP_INT v;
    mpz_init(&v);

    if ((ret = s_parse_bigint_by_base_in_range(buf, &v, base,
               &s_LLONG_MIN, &s_LLONG_MAX)) ==
        GALLUS_RESULT_OK) {

#ifdef GALLUS_ARCH_64_BITS
      *val = (int64_t)mpz_get_si(&v);
#else
      *val = (int64_t)s_bignum_2_uint64_t(&v);
#endif /* GALLUS_ARCH_64_BITS */

    }

    mpz_clear(&v);
  }

  return ret;
}


gallus_result_t
gallus_str_parse_uint64_by_base(const char *buf, uint64_t *val,
                                 unsigned int base) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL &&
      base > 1) {
    MP_INT v;
    mpz_init(&v);

    if ((ret = s_parse_bigint_by_base_in_range(buf, &v, base,
               &s_0, &s_ULLONG_MAX)) ==
        GALLUS_RESULT_OK) {

#ifdef GALLUS_ARCH_64_BITS
      *val = (uint64_t)mpz_get_ui(&v);
#else
      *val = s_bignum_2_uint64_t(&v);
#endif /* GALLUS_ARCH_64_BITS */

    }

    mpz_clear(&v);
  }

  return ret;
}


gallus_result_t
gallus_str_parse_int64(const char *buf, int64_t *val) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL) {
    MP_INT v;
    mpz_init(&v);

    if ((ret = s_parse_bigint_in_range(buf, &v,
                                       &s_LLONG_MIN, &s_LLONG_MAX)) ==
        GALLUS_RESULT_OK) {

#ifdef GALLUS_ARCH_64_BITS
      *val = (int64_t)mpz_get_si(&v);
#else
      *val = (int64_t)s_bignum_2_uint64_t(&v);
#endif /* GALLUS_ARCH_64_BITS */

    }

    mpz_clear(&v);
  }

  return ret;
}


gallus_result_t
gallus_str_parse_uint64(const char *buf, uint64_t *val) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL) {
    MP_INT v;
    mpz_init(&v);

    if ((ret = s_parse_bigint_in_range(buf, &v,
                                       &s_0, &s_ULLONG_MAX)) ==
        GALLUS_RESULT_OK) {

#ifdef GALLUS_ARCH_64_BITS
      *val = (uint64_t)mpz_get_ui(&v);
#else
      *val = s_bignum_2_uint64_t(&v);
#endif /* GALLUS_ARCH_64_BITS */

    }

    mpz_clear(&v);
  }

  return ret;
}





gallus_result_t
gallus_str_parse_int32_by_base(const char *buf, int32_t *val,
                                unsigned int base) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL &&
      base > 1) {
    MP_INT v;
    mpz_init(&v);

    if ((ret = s_parse_bigint_by_base_in_range(buf, &v, base,
               &s_INT_MIN, &s_INT_MAX)) ==
        GALLUS_RESULT_OK) {

#ifdef GALLUS_ARCH_64_BITS
      int64_t v64 = (int64_t)mpz_get_si(&v);
      *val = (int32_t)v64;
#else
      int64_t v64 = (int64_t)s_bignum_2_uint64_t(&v);
      *val = (int32_t)v64;
#endif /* GALLUS_ARCH_64_BITS */

    }

    mpz_clear(&v);
  }

  return ret;
}


gallus_result_t
gallus_str_parse_uint32_by_base(const char *buf, uint32_t *val,
                                 unsigned int base) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL &&
      base > 1) {
    MP_INT v;
    mpz_init(&v);

    if ((ret = s_parse_bigint_by_base_in_range(buf, &v, base,
               &s_0, &s_UINT_MAX)) ==
        GALLUS_RESULT_OK) {

#ifdef GALLUS_ARCH_64_BITS
      uint64_t v64 = (uint64_t)mpz_get_ui(&v);
      *val = (uint32_t)v64;
#else
      uint64_t v64 = s_bignum_2_uint64_t(&v);
      *val = (uint32_t)v64;
#endif /* GALLUS_ARCH_64_BITS */

    }

    mpz_clear(&v);
  }

  return ret;
}


gallus_result_t
gallus_str_parse_int32(const char *buf, int32_t *val) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL) {
    MP_INT v;
    mpz_init(&v);

    if ((ret = s_parse_bigint_in_range(buf, &v,
                                       &s_INT_MIN, &s_INT_MAX)) ==
        GALLUS_RESULT_OK) {

#ifdef GALLUS_ARCH_64_BITS
      int64_t v64 = (int64_t)mpz_get_si(&v);
      *val = (int32_t)v64;
#else
      int64_t v64 = (int64_t)s_bignum_2_uint64_t(&v);
      *val = (int32_t)v64;
#endif /* GALLUS_ARCH_64_BITS */

    }

    mpz_clear(&v);
  }

  return ret;
}


gallus_result_t
gallus_str_parse_uint32(const char *buf, uint32_t *val) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL) {
    MP_INT v;
    mpz_init(&v);

    if ((ret = s_parse_bigint_in_range(buf, &v,
                                       &s_0, &s_UINT_MAX)) ==
        GALLUS_RESULT_OK) {

#ifdef GALLUS_ARCH_64_BITS
      uint64_t v64 = (uint64_t)mpz_get_ui(&v);
      *val = (uint32_t)v64;
#else
      uint64_t v64 = s_bignum_2_uint64_t(&v);
      *val = (uint32_t)v64;
#endif /* GALLUS_ARCH_64_BITS */

    }

    mpz_clear(&v);
  }

  return ret;
}





gallus_result_t
gallus_str_parse_int16_by_base(const char *buf, int16_t *val,
                                unsigned int base) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL &&
      base > 1) {
    MP_INT v;
    mpz_init(&v);

    if ((ret = s_parse_bigint_by_base_in_range(buf, &v, base,
               &s_SHRT_MIN, &s_SHRT_MAX)) ==
        GALLUS_RESULT_OK) {

#ifdef GALLUS_ARCH_64_BITS
      int64_t v64 = (int64_t)mpz_get_si(&v);
      *val = (int16_t)v64;
#else
      int64_t v64 = (int64_t)s_bignum_2_uint64_t(&v);
      *val = (int16_t)v64;
#endif /* GALLUS_ARCH_64_BITS */

    }

    mpz_clear(&v);
  }

  return ret;
}


gallus_result_t
gallus_str_parse_uint16_by_base(const char *buf, uint16_t *val,
                                 unsigned int base) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL &&
      base > 1) {
    MP_INT v;
    mpz_init(&v);

    if ((ret = s_parse_bigint_by_base_in_range(buf, &v, base,
               &s_0, &s_USHRT_MAX)) ==
        GALLUS_RESULT_OK) {

#ifdef GALLUS_ARCH_64_BITS
      uint64_t v64 = (uint64_t)mpz_get_ui(&v);
      *val = (uint16_t)v64;
#else
      uint64_t v64 = s_bignum_2_uint64_t(&v);
      *val = (uint16_t)v64;
#endif /* GALLUS_ARCH_64_BITS */

    }

    mpz_clear(&v);
  }

  return ret;
}


gallus_result_t
gallus_str_parse_int16(const char *buf, int16_t *val) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL) {
    MP_INT v;
    mpz_init(&v);

    if ((ret = s_parse_bigint_in_range(buf, &v,
                                       &s_SHRT_MIN, &s_SHRT_MAX)) ==
        GALLUS_RESULT_OK) {

#ifdef GALLUS_ARCH_64_BITS
      int64_t v64 = (int64_t)mpz_get_si(&v);
      *val = (int16_t)v64;
#else
      int64_t v64 = (int64_t)s_bignum_2_uint64_t(&v);
      *val = (int16_t)v64;
#endif /* GALLUS_ARCH_64_BITS */

    }

    mpz_clear(&v);
  }

  return ret;
}


gallus_result_t
gallus_str_parse_uint16(const char *buf, uint16_t *val) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL) {
    MP_INT v;
    mpz_init(&v);

    if ((ret = s_parse_bigint_in_range(buf, &v,
                                       &s_0, &s_USHRT_MAX)) ==
        GALLUS_RESULT_OK) {

#ifdef GALLUS_ARCH_64_BITS
      uint64_t v64 = (uint64_t)mpz_get_ui(&v);
      *val = (uint16_t)v64;
#else
      uint64_t v64 = s_bignum_2_uint64_t(&v);
      *val = (uint16_t)v64;
#endif /* GALLUS_ARCH_64_BITS */

    }

    mpz_clear(&v);
  }

  return ret;
}





gallus_result_t
gallus_str_parse_float(const char *buf, float *val) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL) {

    size_t len = strlen(buf);

    if (len > 0) {
      char *eptr = NULL;
      float tmp = strtof(buf, &eptr);
      if (eptr == (buf + len)) {
        ret = GALLUS_RESULT_OK;
        *val = tmp;
      } else {
        ret = GALLUS_RESULT_NAN;
      }
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_str_parse_double(const char *buf, double *val) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL) {

    size_t len = strlen(buf);

    if (len > 0) {
      char *eptr = NULL;
      double tmp = strtod(buf, &eptr);
      if (eptr == (buf + len)) {
        ret = GALLUS_RESULT_OK;
        *val = tmp;
      } else {
        ret = GALLUS_RESULT_NAN;
      }
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_str_parse_long_double(const char *buf, long double *val) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

#ifdef HAVE_STRTOLD
  if (IS_VALID_STRING(buf) == true &&
      val != NULL) {

    size_t len = strlen(buf);

    if (len > 0) {
      char *eptr = NULL;
      long double tmp = strtold(buf, &eptr);
      if (eptr == (buf + len)) {
        ret = GALLUS_RESULT_OK;
        *val = tmp;
      } else {
        ret = GALLUS_RESULT_NAN;
      }
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

#else
  double d;

  ret = gallus_str_parse_double(buf, &d);
  if (ret == GALLUS_RESULT_OK) {
    *val = (long double)d;
  }
#endif /* HAVE_STRTOLD */

  return ret;
}


gallus_result_t
gallus_str_parse_bool(const char *buf, bool *val) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(buf) == true &&
      val != NULL) {
    if (strcasecmp(buf, "true") == 0 ||
        strcasecmp(buf, "yes") == 0 ||
        strcasecmp(buf, "on") == 0 ||
        strcmp(buf, "1") == 0) {
      *val = true;
      ret = GALLUS_RESULT_OK;
    } else if (strcasecmp(buf, "false") == 0 ||
               strcasecmp(buf, "no") == 0 ||
               strcasecmp(buf, "off") == 0 ||
               strcmp(buf, "0") == 0) {
      *val = false;
      ret = GALLUS_RESULT_OK;
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





gallus_result_t
gallus_str_tokenize_with_limit(char *buf, char **tokens,
                                size_t max, size_t limit, const char *delm) {
  gallus_result_t n = 0;
  size_t no_delm = 0;

  if (IS_VALID_STRING(buf) == true &&
      IS_VALID_STRING(delm) == true) {

    while (*buf != '\0' && (size_t)n < max) {

      while (strchr(delm, (int)*buf) != NULL && *buf != '\0') {
        /*
         * Increent the pointer while *buf is a delimiter.
         */
        buf++;
      }
      if (*buf == '\0') {
        break;
      }
      tokens[n] = buf;

      /* split limit check. */
      if (limit != 0 && limit <= (size_t) n) {
        n++;
        goto done;
      }

      no_delm = 0;
      while (strchr(delm, (int)*buf) == NULL && *buf != '\0') {
        no_delm++;
        buf++;
      }
      if (*buf == '\0') {
        if (no_delm > 0) {
          n++;
        }
        break;
      }
      *buf = '\0';
      n++;
      if (*(buf + 1) == '\0') {
        break;
      } else {
        buf++;
      }

    }
  } else {
    n  = GALLUS_RESULT_INVALID_ARGS;
  }
done:
  return n;
}


gallus_result_t
gallus_str_tokenize_quote(char *buf, char **tokens,
                           size_t max, const char *delm, const char *quote) {
  gallus_result_t n = 0;
  size_t no_delm = 0;
  int cur_quote = 0;

  if (IS_VALID_STRING(buf) == true &&
      IS_VALID_STRING(delm) == true &&
      IS_VALID_STRING(quote) == true) {

    while (*buf != '\0' && (size_t)n < max) {

      while (*buf != '\0' && strchr(delm, (int)*buf) != NULL) {
        /*
         * Increent the pointer while *buf is a delimiter.
         */
        buf++;
      }
      if (*buf == '\0') {
        break;
      }
      tokens[n] = buf;

      no_delm = 0;
      cur_quote = 0;

      while (*buf != '\0' && strchr(delm, (int)*buf) == NULL) {

        if (strchr(quote, (int)*buf) != NULL) {
          /*
           * Quoted
           */
          cur_quote = (int)*buf;
          /*
           * terminate the current token.
           */
          *buf = '\0';

          buf++;
          if (*buf != '\0') {
            if (no_delm > 0) {
              if ((size_t)n < max) {
                n++;
              } else {
                goto done;
              }
            }
            tokens[n] = buf;

            /*
             * Skip to unquote point.
             */
            while (*buf != '\0') {
              buf = strchr(buf, cur_quote);
              if (buf != NULL) {
                if ((int)(*(buf - 1)) == '\\') {
                  /*
                   * The current quote letter is escaped by '\\' so
                   * we are still in the qutoted string.
                   */
                  buf++;
                  continue;
                } else {
                  /*
                   * The string is unqouted for now and increent the
                   * token count.
                   */
                  no_delm = 1;
                  goto got_token;
                }
              } else {
                n = GALLUS_RESULT_QUOTE_NOT_CLOSED;
                goto done;
              }
            }
          } else {
            /*
             * Quotation stated at the tail of the string.
             */
            n = GALLUS_RESULT_QUOTE_NOT_CLOSED;
            goto done;
          }

        } else {
          /*
           * Not quoted.
           */
          no_delm++;
          buf++;
        }

      }

    got_token:
      if (*buf == '\0') {
        if (no_delm > 0) {
          n++;
        }
        break;
      }
      *buf = '\0';
      n++;
      if (*(buf + 1) == '\0') {
        break;
      } else {
        buf++;
      }

    }

  } else {
    n = GALLUS_RESULT_INVALID_ARGS;
  }

done:
  return n;
}


gallus_result_t
gallus_str_unescape(const char *org, const char *escaped,
                     char **retptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(org) == true &&
      IS_VALID_STRING(escaped) == true) {
    size_t l = strlen(org);
    char *end = (char *)org + l;;

    if (retptr != NULL) {
      *retptr = NULL;
    }

    if (strchr(escaped, (int)*org) == NULL &&
        (strchr(escaped, (int)(*(end - 1))) == NULL ||
         (l >= 2 &&
          strchr(escaped, (int)(*(end - 1))) != NULL &&
          *(end - 2) == '\\'))) {
      bool got_bs = false;
      char *buf = (char *)malloc(l + 1);

      if (buf != NULL) {
        char *p = buf;

        while (*org != '\0') {
          if (got_bs == true) {
            if (strchr(escaped, (int)*org) != NULL) {
              *p++ = *org;
            } else if (*org == '\\') {
              /*
               * push '\\' twice.
               */
              *p++ = '\\';
              *p++ = '\\';
            } else {
              *p++ = '\\';
              *p++ = *org;
            }
            got_bs = false;
          } else if (*org == '\\') {
            got_bs = true;
          } else {
            *p++ = *org;
          }

          org++;
        }

        *p = '\0';

        ret = (gallus_result_t)strlen(buf);
        if (retptr != NULL && ret > 0) {
          *retptr = buf;
        } else {
          free(buf);
        }

      } else {
        ret = GALLUS_RESULT_NO_MEMORY;
      }
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_str_escape(const char *in_str, const char *escape_chars,
                   bool *is_escaped, char **out_str) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  size_t len = 0;
  size_t escapes_len = 0;
  char *in_str_cpy = NULL;
  char *cp = NULL;
  char *str = NULL;

  if (IS_VALID_STRING(in_str) == true &&
      IS_VALID_STRING(escape_chars) == true &&
      out_str != NULL && *out_str == NULL) {
    in_str_cpy = strdup(in_str);

    if (in_str_cpy != NULL) {
      cp = in_str_cpy;
      while (*cp != '\0') {
        if (strchr(escape_chars, (int) *cp) == NULL) {
          len++;
        } else {
          escapes_len += 2;
        }
        cp++;
      }
      if (escapes_len == 0) {
        /* not include escape characters. */
        *out_str = strdup(in_str_cpy);
        if (*out_str != NULL) {
          if (is_escaped != NULL) {
            *is_escaped = false;
          }
          ret = GALLUS_RESULT_OK;
        } else {
          ret = GALLUS_RESULT_NO_MEMORY;
        }
      } else {
        *out_str = (char *) malloc(sizeof(char) *
                                   (len + escapes_len + 1));
        if (*out_str != NULL) {
          str = *out_str;
          cp = in_str_cpy;
          while (*cp != '\0') {
            if (strchr(escape_chars, (int) *cp) == NULL) {
              *str = *cp;
              str++;
            } else {
              *str = '\\';
              str++;
              *str = *cp;
              str++;
            }
            cp++;
          }
          (*out_str)[len + escapes_len] = '\0';

          if (is_escaped != NULL) {
            *is_escaped = true;
          }
          ret = GALLUS_RESULT_OK;
        } else {
          ret = GALLUS_RESULT_NO_MEMORY;
        }
      }
    } else {
      ret = GALLUS_RESULT_NO_MEMORY;
    }

    /* free. */
    if (ret != GALLUS_RESULT_OK) {
      free(*out_str);
      *out_str = NULL;
    }
    free(in_str_cpy);
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_str_trim_right(const char *org, const char *trimchars,
                       char **retptr) {
  gallus_result_t n = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(org) == true &&
      IS_VALID_STRING(trimchars) == true &&
      retptr != NULL) {
    char *buf = strdup(org);

    if (buf != NULL) {
      char *ed = buf + strlen(buf) - 1;

      *retptr = NULL;

      while (ed >= buf) {
        if (strchr(trimchars, (int)*ed) != NULL) {
          *ed = '\0';
          ed--;
        } else {
          break;
        }
      }

      if ((n = (gallus_result_t)strlen(buf)) > 0) {
        *retptr = buf;
      } else {
        free(buf);
      }
    } else {
      n = GALLUS_RESULT_NO_MEMORY;
    }
  } else {
    n = GALLUS_RESULT_INVALID_ARGS;
  }

  return n;
}

gallus_result_t
gallus_str_trim_left(const char *org, const char *trimchars,
                      char **retptr) {
  gallus_result_t n = GALLUS_RESULT_ANY_FAILURES;
  char *buf = NULL;

  if (IS_VALID_STRING(org) == true &&
      IS_VALID_STRING(trimchars) == true &&
      retptr != NULL) {
    char *st = (char *) org;

    *retptr = NULL;

    while (*st != '\0') {
      if (strchr(trimchars, (int)*st) != NULL) {
        st++;
      } else {
        break;
      }
    }

    if ((n = (gallus_result_t)strlen(st)) > 0) {
      buf = strdup(st);
      if (buf != NULL) {
        *retptr = buf;
      } else {
        n = GALLUS_RESULT_NO_MEMORY;
      }
    }
  } else {
    n = GALLUS_RESULT_INVALID_ARGS;
  }

  return n;
}

gallus_result_t
gallus_str_trim(const char *org, const char *trimchars,
                 char **retptr) {
  gallus_result_t n = GALLUS_RESULT_ANY_FAILURES;
  char *tmp_str = NULL;

  if ((n = gallus_str_trim_right(org, trimchars, &tmp_str)) > 0) {
    n = gallus_str_trim_left(tmp_str, trimchars, retptr);
  }

  if (n == 0) {
    *retptr = tmp_str;
  } else {
    free(tmp_str);
  }

  return n;
}

gallus_result_t
gallus_str_indexof(const char *str1, const char *str2) {
  char *ptr = NULL;
  int64_t ret = 0;

  if (str1 != NULL && str2 != NULL) {
    ptr = strstr(str1, str2);
    if (ptr != NULL) {
      ret = ptr - str1;
    } else {
      ret = GALLUS_RESULT_NOT_FOUND;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

