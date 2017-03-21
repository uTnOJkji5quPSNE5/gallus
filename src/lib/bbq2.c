#include "gallus_apis.h"





struct gallus_bbq2_record;
typedef struct gallus_bbq2_record *gallus_bbq2_t;
typedef void    (*gallus_bbq2_value_freeup_proc_t)(void **valptr);


typedef enum {
  wait_unknown = 0,
  wait_put = 1,
  wait_get
} wait_dir_t;


typedef struct {
  bool m_done;
  size_t m_n_add;
} memcpy_result_t;


typedef struct gallus_bbq2_record {
  int m_numa_node;

  gallus_mutex_t m_muxlock;

  gallus_spinlock_t m_spinlock;

  gallus_mutex_t m_put_lock;
  gallus_cond_t m_put_cond;

  gallus_mutex_t m_get_lock;
  gallus_cond_t m_get_cond;

  volatile uint64_t m_r_idx;
  volatile uint64_t m_w_idx;

  volatile size_t m_n_elements;
  volatile size_t m_n_elements_temp;

  volatile size_t m_cur_put_seq;
  volatile size_t m_done_put_seq;
  size_t m_n_max_putters;
  memcpy_result_t *m_put_results;

  volatile size_t m_n_get_waiters;
  volatile size_t m_n_put_waiters;

  gallus_bbq2_value_freeup_proc_t m_del_proc;

  size_t m_element_size;

  int64_t m_n_max_elements;

  volatile bool m_is_operational;
  volatile bool m_is_awakened;

  gallus_cond_t m_awaken_cond;

  char m_data[0];
} gallus_bbq2_record;





static inline void
s_spinlock(gallus_bbq2_t q) {
  if (likely(q != NULL)) {
    (void)gallus_spinlock_lock(&(q->m_spinlock));
  }
}


static inline void
s_spinunlock(gallus_bbq2_t q) {
  if (likely(q != NULL)) {
    (void)gallus_spinlock_unlock(&(q->m_spinlock));
  }
}


static inline void
s_muxlock(gallus_bbq2_t q) {
  if (likely(q != NULL)) {
    (void)gallus_mutex_lock_lock(&(q->m_muxlock));
  }
}


static inline void
s_muxunlock(gallus_bbq2_t q) {
  if (likely(q != NULL)) {
    (void)gallus_mutex_unlock(&(q->m_muxlock));
  }
}





static inline char *
s_data_addr(gallus_bbq2_t q, uint64_t idx) {
  if (likely(q != NULL)) {
    return
        q->m_data +
        (idx % q->m_n_max_elements) * q->m_element_size;
  } else {
    return NULL;
  }
}


static inline void
s_freeup_all_values(gallus_bbq2_t q) {
  if (likely(q != NULL &&
             q->m_del_proc != NULL)) {
    uint64_t i;
    char *addr;

    for (i = q->m_r_idx; i < q->m_w_idx; i++) {
      addr = s_data_addr(q, i);
      if (addr != NULL) {
        q->m_del_proc((void **)addr);
      }
    }
  }
}


static inline void
s_awaken_all(gallus_bbq2_t q) {
  if (likely(q != NULL)) {
    (void)gallus_mutex_lock(&(q->m_put_lock));
    (void)gallus_mutex_lock(&(q->m_get_lock));
    (void)gallus_mutex_lock(&(q->m_muxlock));
    {
      gallus_cond_destroy(&(q->m_put_cond));
      gallus_cond_destroy(&(q->m_get_cond));
      q->m_is_awakened = false;
      gallus_cond_destroy(&((*qptr)->m_awaken_cond));
    }
    (void)gallus_mutex_unlock(&(q->m_muxlock));
    (void)gallus_mutex_unlock(&(q->m_get_lock));
    (void)gallus_mutex_unlock(&(q->m_put_lock));
  }
}


static inline void
a_clean(gallus_bbq2_t q, bool free_values) {
  if (likely(q != NULL)) {
    if (free_values == true) {
      s_freeup_all_values(q);
    }
    q->m_r_idx = 0;
    q->m_w_idx = 0;
    (void)memset((void *)(q->m_data), 0,
                 q->m_element_size * q->m_n_max_elements);
    q->m_n_elements = 0;
    q->m_n_elements_temp = 0;
    q->m_cur_put_seq = 0;
    q->m_done_put_seq = 0;
    q->m_n_get_waiters = 0;
    q->m_n_put_waiters = 0;
    q->m_is_operational = true;

    s_awaken_all(q);
  }
}


static inline void
s_shutdown(gallus_bbq2_t q, bool free_values) {
  if (likely(q != NULL &&
             q->m_is_operational == true)) {
    s_clean(q, free_values);
    q->m_is_operational = false;
  }
}


static inline size_t
s_copyin(gallus_bbq2_t q, void *buf, size_t n) {
  size_t max_rooms;
  size_t max_n;
  uint64_t my_w_idx;
  size_t my_put_seq;
  size_t copy_remains;
  size_t i;
  size_t post_scan_idx;
  size_t idx;
  char *dst;
  
  s_spinlock(q);
  {
    mbar();

    max_rooms = q->m_n_max_elements - q->m_n_elements_temp;
    max_n = (max_rooms < n) ? max_rooms : n;

    if (likely(max_n > 0)) {
      my_w_idx = q->m_w_idx;
      q->m_w_idx += n_max;
      q->m_n_elements_temp += n_max;
      my_put_seq = q->m_cur_put_seq;
      q->m_cur_put_seq++;

      mbar();
    }
  }
  s_spinunlock(q);

  if (likely(max_n > 0)) {
    /*
     * Copy the buf into the queue.
     */
    idx = my_w_idx % q->m_n_max_elements;
    dst = q->m_data + idx * q->m_element_size;
    post_scan_idx = my_put_seq % m_n_max_putters;

    q->m_put_results[post_scan_idx].m_done = false;
    q->m_put_results[post_scan_idx].m_seq = my_put_seq;
    q->m_put_results[post_scan_idx].m_n_add = max_n;

    if (likely((idx + max_n) <= q->m_n_max_elements)) {
      (void)memcpy((void *)dst, buf,
                   (size_t)max_n * q->m_element_size);
    } else {
      size_t max_n_0 = q->m_n_max_elements - idx;
      size_t max_n_0_sz = (size_t)max_n_0 * q->m_element_size;
      char *src1 = (char *)buf + max_n_0_sz;
      size_t max_n_1 = max_n - max_n_0;

      (void)memcpy((void *)dst, buf, max_n_0_sz);
      (void)memcpy((void *)(q->m_data), (void *)src1,
                   max_n_1 * q->m_element_size);
    }

    q->m_put_results[post_scan_idx].m_done = false;

    s_spinlock(q);
    {
      /*
       * We could have unaccumulated memcpy result. Scan and accumulate.
       */
      copy_remains = my_put_seq - q->m_done_put_seq;

      for (i = copy_remains; i >= 0; i--) {
        idx = (post_scan_idx + q->m_n_max_putters - i) % q->m_n_max_putters;
        if (q->m_put_results[idx].m_done == true) {
          (void)__sync_add_and_fetch(&(q->m_n_eleents),
                                     q->m_put_results[idx].m_n_add);
          q->m_done_put_seq = q->m_put_results[idx].m_seq;
        } else {
          /*
           * memcpy that sequence # is younger than this still
           * running. Just leave it alone for now.
           */
          goto unlock;
        }
      }

      mbar();
    }
 unlock:
    s_spinunlock(q);

    /*
     * And wake all the getters.
     */
    (void)gallus_cond_notify(&(q->m_cond_get), true);
  }

  return max_n;
}


static inline int64_t
s_copyout(gallus_bbq2_t q, void *buf, size_t n, bool do_incr) {
  size_t my_r_idx;
  size_t idx;
  char *src;
  size_t max_elem = __sync_fetch_and_add(&(q->m_n_elements), 0);
  size_t max_n = (n < max_elem) ? n : max_elem;
  
  if (likely(max_n > 0)) {
    /*
     * Copy to buf from the queue.
     */

    s_spinlock(q);
    {
      mbar();

      my_r_idx = q->m_r_idx;

      if (likely(do_incr == true)) {
        q->m_r_idx += n_max;
        q->m_n_elements_temp -= n_max;
        (void)__sync_sub_and_fetch(&(q->m_n_elements), n_max);

        mbar();
      }
    }
    s_spinunlock(q);

    idx =  my_r_idx % q->m_max_elements;
    src = q->m_data + idx * q->m_element_size;

    if (likely((idx + max_n) <= q->m_n_max_elements)) {
      (void)memcpy(buf, (void *)src, max_n * q->m_element_size);
    } else {
      size_t max_n_0 = q->m_n_max_elements - idx;
      size_t max_n_0_sz = (size_t)max_n_0 * q->m_element_size;
      char *dst1 = (char *)buf + max_n_0_sz;
      size_t max_n_1 = max_n - max_n_0;

      (void)memcpy(buf, (void *)src, max_n_0_sz);
      (void)memcpy((void *)dst1, (void *)(q->m_data),
                   max_n_1 * q->m_element_size);
    }

    if (likely(do_incr == true)) {
      /*
       * And wake all the putters.
       */
      (void)gallus_cond_notify(&(q->m_cond_put), true);
    }
  }

  return max_n;
}


static inline gallus_result_t
s_wait_io_ready(gallus_bbq2_t q,
                wait_dir_t dir,
                gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  size_t n_waiters;

  if (likely(q != NULL &&
             cptr != NULL &&
             dir != wait_unknown)) {
    if (nsec != 0LL) {

      gallus_msg_debug(5, "wait " PF64(d) " nsec.\n",
                        nsec);

      if (wait_dir == wait_put) {
        (void)gallus_mutex_lock(&(q->m_put_lock));
        {
          (void)__sync_fetch_and_add(&(q->m_n_put_waiters), 1);
          ret = gallus_cond_wait(&(q->m_put_cond), &(q->m_put_lock), nsec);
          (void)__sync_sub_and_fetch(&(q->m_n_put_waiters), 1);
        }
        (void)gallus_mutex_unlock(&(q->m_put_lock));
      } else if (wait_dir == wait_get) {
        (void)gallus_mutex_lock(&(q->m_get_lock));
        {
          (void)__sync_fetch_and_add(&(q->m_n_get_waiters), 1);
          ret = gallus_cond_wait(&(q->m_get_cond), &(q->m_get_lock), nsec);
          (void)__sync_sub_and_fetch(&(q->m_n_get_waiters), 1);
        }
        (void)gallus_mutex_unlock(&(q->m_get_lock));
      }

      if (q->m_is_awakened == true) {
        gallus_msg_debug(5, "awakened while sleeping " PF64(d) " nsec.\n",
                          nsec);
      } else {
        gallus_msg_debug(5, "wait " PF64(d) " nsec done.\n",
                          nsec);
      }

      if (ret == GALLUS_RESULT_OK &&
          q->m_is_awakened == true) {
        /*
         * A waker wakes all the waiting threads up. Note that the
         * waker is waiting for a notification that the all the
         * waiters are awakened and leave this function.
         */
        if (likely(q->m_is_operational == true)) {
          n_waiters = __sync_fetch_and_add(&(q->m_n_put_waiters), 0);
          n_waiters += __sync_fetch_and_add(&(q->m_n_get_waiters), 0);
          if (n_waiters == 0) {

            /*
             * All the waiters are gone. Wake the waker up.
             */

            gallus_msg_debug(5, "sync wakeup who woke me up.\n");

            (void)gallus_mutex_lock(&(q->m_awaken_lock));
            {
              q->m_is_awakened = false;
              (void)gallus_cond_notify(&(q->m_awaken_cond), true);
            }
            (void)gallus_mutex_unlock(&(q->m_awaken_lock));
          }

          ret = GALLUS_RESULT_WAKEUP_REQUESTED;

        } else {
          (void)gallus_mutex_lock(&(q->m_awaken_lock));
          {
            q->m_is_awakened = false;
            (void)gallus_cond_notify(&(q->m_cond_awakened), true);
          }
          (void)gallus_mutex_unlock(&(q->m_awaken_lock));

          ret = GALLUS_RESULT_NOT_OPERATIONAL;
        }
      }

    } else {
      ret = GALLUS_RESULT_OK;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_wait_puttable(gallus_bbq2_t q, gallus_chrono_t nsec) {
  return s_wait_io_ready(q, wait_put, nsec);
}


static inline gallus_result_t
s_wait_gettable(gallus_bbq2_t q, gallus_chrono_t nsec) {
  return s_wait_io_ready(q, wait_get, nsec);
}


static inline gallus_result_t
s_put_n(gallus_bbq2_t *qptr,
        void *valptr,
        size_t n_vals,
        size_t valsz,
        gallus_chrono_t nsec,
        size_t *n_actual_put) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_bbq2_t q = NULL;

  if (likely(qptr != NULL && (q = *qptr) != NULL &&
             valptr != NULL &&
             valsz == q->m_element_size)) {

    if (likely(n_vals > 0)) {

      size_t n_copyin = 0LL;

      if (nsec == 0LL) {
        /*
         * Just put and return.
         */
        if (q->m_is_operational == true) {
          n_copyin = s_copyin(q, valptr, n_vals);
          ret = (gallus_result_t)n_copyin;
        } else {
          ret = GALLUS_RESULT_NOT_OPERATIONAL;
        }

      } else if (nsec < 0LL) {
        /*
         * Repeat putting until all the data are put.
         */
     check_inf:
        mbar();
        if (likely(q->m_is_operational == true)) {
          n_copyin += s_copyin(q,
                               (void *)((char *)valptr +
                                        ((size_t)n_copyin * valsz)),
                               n_vals - (size_t)n_copyin);
          if (n_copyin < n_vals) {
            /*
             * Need to repeat.
             */
            if (q->m_n_elements >= q->m_n_max_elements) {
              /*
               * No vacancy. Need to wait for someone get data from
               * the buffer.
               */
              if ((ret = s_wait_puttable(q, -1LL)) == GALLUS_RESULT_OK) {
                goto check_inf;
              } else {
                /*
                 * Any errors occur while waiting.
                 */
                if (ret == GALLUS_RESULT_TIMEDOUT) {
                  /*
                   * Must not happen.
                   */
                  gallus_msg_fatal("Timed out must not happen here.\n");
                }
              }
            } else {
              /*
               * The buffer still has rooms but it couldn't put all
               * the data??  Must not happen??
               */
              gallus_msg_fatal("Couldn't put all the data even rooms "
                               "available. Must not happen.\n");
            }
          } else {
            /*
             * Succeeded.
             */
            ret = (gallus_result_t)n_copyin;
          }
        } else {
          ret = GALLUS_RESULT_NOT_OPERATIONAL;
        }

      } else {
        /*
         * Repeat putting until all the data are put or the spcified
         * time limit is expired.
         */
        gallus_chrono_t copy_start;
        gallus_chrono_t wait_end;
        gallus_chrono_t to = nsec;

     check_to:
        mbar();
        if (likely(q->m_is_operational == true)) {
          WHAT_TIME_IS_IT_NOW_IN_NSEC(copy_start);
          n_copyin += s_copyin(q,
                               (void *)((char *)valptr +
                                        ((size_t)n_copyin * valsz)),
                               n_vals - (size_t)n_copyin);
          if (n_copyin < n_vals) {
            /*
             * Need to repeat.
             */
            if (q->m_n_elements >= q->m_n_max_elements) {
              /*
               * No vacancy. Need to wait for someone get data from
               * the buffer.
               */
              if ((ret = s_wait_puttable(q, to)) == GALLUS_RESULT_OK) {
                WHAT_TIME_IS_IT_NOW_IN_NSEC(wait_end);
                to -= (wait_end - copy_start);
                if (to > 0LL) {
                  goto check_to;
                }
                ret = GALLUS_RESULT_TIMEDOUT;
              }
            } else {
              /*
               * The buffer still has rooms but it couldn't put all
               * the data??  Must not happen??
               */
              gallus_msg_fatal("Couldn't put all the data even rooms "
                               "available. Must not happen.\n");
            }
          } else {
            /*
             * Succeeded.
             */
            ret = (gallus_result_t)n_copyin;
          }
        } else {
          ret = GALLUS_RESULT_NOT_OPERATIONAL;
        }

      }

      if (n_actual_put != NULL) {
        *n_actual_put = n_copyin;
      }

    } else {
      if (n_actual_put != NULL) {
        *n_actual_put = 0LL;
      }
      ret = GALLUS_RESULT_OK;
    }

  } else {
    if (n_actual_put != NULL) {
      *n_actual_put = 0LL;
    }
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_get_n(gallus_bbq2_t *qptr,
        void *valptr,
        size_t n_vals_max,
        size_t n_at_least,
        size_t valsz,
        gallus_chrono_t nsec,
        size_t *n_actual_get,
        bool do_incr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_bbq2_t q = NULL;

  if (likely(qptr != NULL && (q = *qptr) != NULL &&
             valptr != NULL &&
             valsz == q->m_element_size)) {

    if (likely(n_vals_max > 0)) {

      size_t n_copyout = 0LL;

      if (nsec == 0LL) {
        /*
         * Just get and return.
         */
        if (likely(q->m_is_operational == true)) {
          n_copyout = s_copyout(q, valptr, n_vals_max, do_incr);
          ret = (gallus_result_t)n_copyout;
        } else {
          ret = GALLUS_RESULT_NOT_OPERATIONAL;
        }
        
      } else if (nsec < 0LL) {
        /*
         * Repeat getting until all the required # of the data are
         * got.
         */
     check_inf:
        mbar();
        if (likely(q->m_is_operational == true)) {
          n_copyout += s_copyout(q,
                                 (void *)((char *)valptr +
                                          ((size_t)n_copyout * valsz)),
                                 n_vals_max - (size_t)n_copyout,
                                 do_incr);
          if (n_copyout < n_vals_max) {
            /*
             * Need to repeat.
             */
            if (q->m_n_elements < 1LL) {
              /*
               * No data. Need to wait for someone put data to the
               * buffer.
               */
              if ((ret = s_wait_gettable(q, -1LL)) == GALLUS_RESULT_OK) {
                goto check_inf;
              } else {
                /*
                 * Any errors occur while waiting.
                 */
                if (ret == GALLUS_RESULT_TIMEDOUT) {
                  /*
                   * Must not happen.
                   */
                  gallus_msg_fatal("Timed out must not happen here.\n");
                }
              }
            } else {
              /*
               * The buffer still has data but it couldn't get all the
               * data??  Must not happen??
               */
              gallus_msg_fatal("Couldn't get all the data even the data "
                               "available. Must not happen.\n");
            }
          } else {
            /*
             * Succeeded.
             */
            ret = (gallus_result_t)n_copyout;
          }
        } else {
          ret = GALLUS_RESULT_NOT_OPERATIONAL;
        }

      } else {
        /*
         * Repeat getting until all the required # of the data are
         * got or the spcified time limit is expired.
         */
        gallus_chrono_t copy_start;
        gallus_chrono_t wait_end;
        gallus_chrono_t to = nsec;

     check_to:
        mbar();
        if (likely(q->m_is_operational == true)) {
          WHAT_TIME_IS_IT_NOW_IN_NSEC(copy_start);
          n_copyout += s_copyout(q,
                                 (void *)((char *)valptr +
                                          ((size_t)n_copyout * valsz)),
                                 n_vals_max - (size_t)n_copyout,
                                 do_incr);
          if (n_copyout < n_at_least) {
            /*
             * Need to repeat.
             */
            if (q->m_n_elements < 1LL) {
              /*
               * No data. Need to wait for someone put data to the
               * buffer.
               */
              if ((ret = s_wait_gettable(q, to)) == GALLUS_RESULT_OK) {
                WHAT_TIME_IS_IT_NOW_IN_NSEC(wait_end);
                to -= (wait_end - copy_start);
                if (to > 0LL) {
                  goto check_to;
                }
                ret = GALLUS_RESULT_TIMEDOUT;
              }
            } else {
              /*
               * The buffer still has data but it couldn't get all the
               * data??  Must not happen??
               */
              gallus_msg_fatal("Couldn't get all the data even the data "
                               "available. Must not happen.\n");
            }
          } else {
            /*
             * Succeeded.
             */
            ret = n_copyout;
          }
        } else {
          ret = GALLUS_RESULT_NOT_OPERATIONAL;
        }

      }

      if (n_actual_get != NULL) {
        *n_actual_get = (size_t)n_copyout;
      }

    } else {
      if (n_actual_get != NULL) {
        *n_actual_get = 0LL;
      }
      ret = GALLUS_RESULT_OK;
    }

  } else {
    if (n_actual_get != NULL) {
      *n_actual_get = 0LL;
    }
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





gallus_result_t
gallus_bbq2_create_with_size(gallus_bbq2_t *qptr,
                                 size_t elemsize,
                                 int64_t maxelems,
                                 gallus_bbq2_value_freeup_proc_t proc) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qptr != NULL &&
      elemsize > 0 &&
      maxelems > 0) {
    gallus_bbq2_t q = (gallus_bbq2_t)malloc(
                             sizeof(*q) + elemsize * (size_t)(maxelems + N_EMPTY_ROOM));

    *qptr = NULL;

    if (q != NULL) {
      if (((ret = gallus_mutex_create(&(q->m_lock))) ==
           GALLUS_RESULT_OK) &&
          ((ret = gallus_cond_create(&(q->m_cond_put))) ==
           GALLUS_RESULT_OK) &&
          ((ret = gallus_cond_create(&(q->m_cond_get))) ==
           GALLUS_RESULT_OK) &&
          ((ret = gallus_cond_create(&(q->m_cond_awakened))) ==
           GALLUS_RESULT_OK)) {
        q->m_r_idx = 0;
        q->m_w_idx = 0;
        q->m_n_elements = 0;
        q->m_n_waiters = 0;
        q->m_n_max_elements = maxelems;
        q->m_n_max_allocd_elements = maxelems + N_EMPTY_ROOM;
        q->m_element_size = elemsize;
        q->m_del_proc = proc;
        q->m_is_operational = true;
        q->m_is_awakened = false;
        q->m_qmuxer = NULL;
        q->m_type = GALLUS_QMUXER_POLL_UNKNOWN;

        *qptr = q;

        ret = GALLUS_RESULT_OK;

      } else {
        free((void *)q);
      }
    } else {
      ret = GALLUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
gallus_bbq2_shutdown(gallus_bbq2_t *qptr,
                         bool free_values) {
  if (qptr != NULL &&
      *qptr != NULL) {

    s_spinlock(*qptr);
    {
      s_shutdown(*qptr, free_values);
    }
    s_spinunlock(*qptr);

  }
}


void
gallus_bbq2_destroy(gallus_bbq2_t *qptr,
                    bool free_values) {
  if (likely(qptr != NULL &&
             *qptr != NULL)) {

    s_spinlock(*qptr);
    {
      s_shutdown(*qptr, free_values);
      gallus_cond_destroy(&((*qptr)->m_put_cond));
      gallus_cond_destroy(&((*qptr)->m_get_cond));
      gallus_cond_destroy(&((*qptr)->m_awaken_cond));
    }
    s_spinunlock(*qptr);

    gallus_mutex_destroy(&((*qptr)->m_lock));

    free((void *)*qptr);
    *qptr = NULL;
  }
}


gallus_result_t
gallus_bbq2_clear(gallus_bbq2_t *qptr,
                      bool free_values) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qptr != NULL &&
      *qptr != NULL) {

    s_spinlock(*qptr);
    {
      s_clean(*qptr, free_values);
      if ((*qptr)->m_qmuxer != NULL &&
          NEED_WAIT_READABLE((*qptr)->m_type) == true) {
        qmuxer_notify((*qptr)->m_qmuxer);
      }
      (void)gallus_cond_notify(&((*qptr)->m_cond_put), true);
    }
    s_spinunlock(*qptr);

    ret = GALLUS_RESULT_OK;

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_bbq2_wakeup(gallus_bbq2_t *qptr, gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qptr != NULL &&
      *qptr != NULL) {
    size_t n_waiters;

    s_spinlock(*qptr);
    {
      n_waiters = __sync_fetch_and_add(&((*qptr)->m_n_waiters), 0);

      if (n_waiters > 0) {
        if ((*qptr)->m_is_operational == true) {
          if ((*qptr)->m_is_awakened == false) {
            /*
             * Wake all the waiters up.
             */
            (*qptr)->m_is_awakened = true;
            (void)gallus_cond_notify(&((*qptr)->m_get_cond), true);
            (void)gallus_cond_notify(&((*qptr)->m_put_cond), true);

            if (nsec != 0LL) {
              /*
               * Then wait for one of the waiters wakes this thread up.
               */
           recheck:
              mbar();
              if ((*qptr)->m_is_operational == true) {
                if ((*qptr)->m_is_awakened == true) {

                  gallus_msg_debug(5, "sync wait a waiter wake me up...\n");

                  if ((ret = gallus_cond_wait(&((*qptr)->m_cond_awakened),
                                               &((*qptr)->m_lock), nsec)) ==
                      GALLUS_RESULT_OK) {
                    goto recheck;
                  }

                  gallus_msg_debug(5, "a waiter woke me up.\n");

                } else {
                  ret = GALLUS_RESULT_OK;
                }
              } else {
                ret = GALLUS_RESULT_NOT_OPERATIONAL;
              }
            } else {
              ret = GALLUS_RESULT_OK;
            }
          } else {
            ret = GALLUS_RESULT_OK;
          }
        } else {
          ret = GALLUS_RESULT_NOT_OPERATIONAL;
        }
      } else {
        ret = GALLUS_RESULT_OK;
      }

    }
    s_spinunlock(*qptr);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_bbq2_wait_gettable(gallus_bbq2_t *qptr,
                              gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qptr != NULL && *qptr != NULL) {

    s_spinlock(*qptr);
    {
      if ((*qptr)->m_n_elements > 0) {
        ret = (*qptr)->m_n_elements;
      } else {
        ret = s_wait_gettable(*qptr, nsec);
        if (ret == GALLUS_RESULT_OK) {
          ret = (*qptr)->m_n_elements;
        }
      }
    }
    s_spinunlock(*qptr);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_bbq2_wait_puttable(gallus_bbq2_t *qptr,
                              gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  int64_t remains;

  if (qptr != NULL && *qptr != NULL) {

    s_spinlock(*qptr);
    {
      remains = (*qptr)->m_n_max_elements - (*qptr)->m_n_elements;
      if (remains > 0) {
        ret = (gallus_result_t)remains;
      } else {
        ret = s_wait_puttable(*qptr, nsec);
        if (ret == GALLUS_RESULT_OK) {
          ret = (*qptr)->m_n_max_elements - (*qptr)->m_n_elements;
        }
      }
    }
    s_spinunlock(*qptr);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





gallus_result_t
gallus_bbq2_put_with_size(gallus_bbq2_t *qptr,
                              void **valptr,
                              size_t valsz,
                              gallus_chrono_t nsec) {
  int64_t n = s_put_n(qptr, (void *)valptr, 1LL, valsz, nsec, NULL);
  return (n == 1LL) ? GALLUS_RESULT_OK : n;
}


gallus_result_t
gallus_bbq2_put_n_with_size(gallus_bbq2_t *qptr,
                                void **valptr,
                                size_t n_vals,
                                size_t valsz,
                                gallus_chrono_t nsec,
                                size_t *n_actual_put) {
  return s_put_n(qptr, (void *)valptr, n_vals, valsz, nsec, n_actual_put);
}


gallus_result_t
gallus_bbq2_get_with_size(gallus_bbq2_t *qptr,
                              void **valptr,
                              size_t valsz,
                              gallus_chrono_t nsec) {
  int64_t n = s_get_n(qptr, (void *)valptr, 1LL, 1LL,
                      valsz, nsec, NULL, true);
  return (n == 1LL) ? GALLUS_RESULT_OK : n;
}


gallus_result_t
gallus_bbq2_get_n_with_size(gallus_bbq2_t *qptr,
                                void **valptr,
                                size_t n_vals_max,
                                size_t n_at_least,
                                size_t valsz,
                                gallus_chrono_t nsec,
                                size_t *n_actual_get) {
  return s_get_n(qptr, (void *)valptr, n_vals_max, n_at_least,
                 valsz, nsec, n_actual_get, true);
}


gallus_result_t
gallus_bbq2_peek_with_size(gallus_bbq2_t *qptr,
                               void **valptr,
                               size_t valsz,
                               gallus_chrono_t nsec) {
  int64_t n = s_get_n(qptr, (void *)valptr, 1LL, 1LL,
                      valsz, nsec, NULL, false);
  return (n == 1LL) ? GALLUS_RESULT_OK : n;
}


gallus_result_t
gallus_bbq2_peek_n_with_size(gallus_bbq2_t *qptr,
                                 void **valptr,
                                 size_t n_vals_max,
                                 size_t n_at_least,
                                 size_t valsz,
                                 gallus_chrono_t nsec,
                                 size_t *n_actual_get) {
  return s_get_n(qptr, (void *)valptr, n_vals_max, n_at_least,
                 valsz, nsec, n_actual_get, false);
}





gallus_result_t
gallus_bbq2_size(gallus_bbq2_t *qptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qptr != NULL &&
      *qptr != NULL) {

    s_spinlock(*qptr);
    {
      if ((*qptr)->m_is_operational == true) {
        ret = (*qptr)->m_n_elements;
      } else {
        ret = GALLUS_RESULT_NOT_OPERATIONAL;
      }
    }
    s_spinunlock(*qptr);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_bbq2_remaining_capacity(gallus_bbq2_t *qptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qptr != NULL &&
      *qptr != NULL) {

    s_spinlock(*qptr);
    {
      if ((*qptr)->m_is_operational == true) {
        ret = (*qptr)->m_n_max_elements - (*qptr)->m_n_elements;
      } else {
        ret = GALLUS_RESULT_NOT_OPERATIONAL;
      }
    }
    s_spinunlock(*qptr);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_bbq2_max_capacity(gallus_bbq2_t *qptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qptr != NULL &&
      *qptr != NULL) {

    s_spinlock(*qptr);
    {
      if ((*qptr)->m_is_operational == true) {
        ret = (*qptr)->m_n_max_elements;
      } else {
        ret = GALLUS_RESULT_NOT_OPERATIONAL;
      }
    }
    s_spinunlock(*qptr);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_bbq2_is_full(gallus_bbq2_t *qptr, bool *retptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qptr != NULL &&
      *qptr != NULL &&
      retptr != NULL) {
    *retptr = false;

    s_spinlock(*qptr);
    {
      if ((*qptr)->m_is_operational == true) {
        *retptr = ((*qptr)->m_n_elements >= (*qptr)->m_n_max_elements) ?
                  true : false;
        ret = GALLUS_RESULT_OK;
      } else {
        ret = GALLUS_RESULT_NOT_OPERATIONAL;
      }
    }
    s_spinunlock(*qptr);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_bbq2_is_empty(gallus_bbq2_t *qptr, bool *retptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qptr != NULL &&
      *qptr != NULL &&
      retptr != NULL) {
    *retptr = false;

    s_spinlock(*qptr);
    {
      if ((*qptr)->m_is_operational == true) {
        *retptr = ((*qptr)->m_n_elements == 0) ? true : false;
        ret = GALLUS_RESULT_OK;
      } else {
        ret = GALLUS_RESULT_NOT_OPERATIONAL;
      }
    }
    s_spinunlock(*qptr);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_bbq2_is_operational(gallus_bbq2_t *qptr, bool *retptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qptr != NULL &&
      *qptr != NULL &&
      retptr != NULL) {
    *retptr = false;

    s_spinlock(*qptr);
    {
      *retptr = (*qptr)->m_is_operational;
      ret = GALLUS_RESULT_OK;
    }
    s_spinunlock(*qptr);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
gallus_bbq2_cancel_janitor(gallus_bbq2_t *qptr) {
  if (qptr != NULL &&
      *qptr != NULL) {
    s_spinunlock(*qptr);
  }
}


#if 0
gallus_result_t
bbq2_setup_for_qmuxer(gallus_bbq2_t q,
                         gallus_qmuxer_t qmx,
                         ssize_t *szptr,
                         ssize_t *remptr,
                         gallus_qmuxer_poll_event_t type,
                         bool is_pre) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (q != NULL &&
      qmx != NULL &&
      szptr != NULL &&
      remptr != NULL) {

    s_spinlock(q);
    {
      if (q->m_is_operational == true) {
        *szptr = q->m_n_elements;
        *remptr = q->m_n_max_elements - q->m_n_elements;

        ret = 0;
        /*
         * Note that a case; (*szptr == 0 && *remptr == 0) never happen.
         */
        if (NEED_WAIT_READABLE(type) == true && *szptr == 0) {
          /*
           * Current queue size equals to zero so we need to wait for
           * readable.
           */
          ret = (gallus_result_t)GALLUS_QMUXER_POLL_READABLE;
        } else if (NEED_WAIT_WRITABLE(type) == true && *remptr == 0) {
          /*
           * Current remaining capacity is zero so we need to wait for
           * writable.
           */
          ret = (gallus_result_t)GALLUS_QMUXER_POLL_WRITABLE;
        }

        if (is_pre == true && ret > 0) {
          q->m_qmuxer = qmx;
          q->m_type = ret;
        } else {
          /*
           * We need this since the qmx could be not available when the
           * next event occurs.
           */
          q->m_qmuxer = NULL;
          q->m_type = 0;
        }

      } else {
        ret = GALLUS_RESULT_NOT_OPERATIONAL;
      }
    }
    s_spinunlock(q);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
#endif
