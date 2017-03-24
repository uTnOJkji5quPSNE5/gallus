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
  uint64_t m_seq;
  size_t m_n_amount;
} memcpy_result_t;


typedef struct gallus_bbq2_record {
  unsigned int m_numa_node;

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

  volatile uint64_t m_cur_put_seq;
  volatile uint64_t m_done_put_seq;

  volatile uint64_t m_cur_get_seq;
  volatile uint64_t m_done_get_seq;

  memcpy_result_t *m_put_results;
  memcpy_result_t *m_get_results;
  
  volatile size_t m_n_get_waiters;
  volatile size_t m_n_put_waiters;

  gallus_bbq2_value_freeup_proc_t m_del_proc;

  size_t m_element_size;

  size_t m_n_max_elements;

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
    size_t i;
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
s_awake_all(gallus_bbq2_t q) {
  if (likely(q != NULL)) {
    (void)gallus_mutex_lock(&(q->m_put_lock));
    (void)gallus_mutex_lock(&(q->m_get_lock));
    (void)gallus_mutex_lock(&(q->m_muxlock));
    {
      (void)gallus_cond_notify(&(q->m_put_cond), true);
      (void)gallus_cond_notify(&(q->m_get_cond), true);
      q->m_is_awakened = false;
      (void)gallus_cond_notify(&(q->m_awaken_cond), true);
    }
    (void)gallus_mutex_unlock(&(q->m_muxlock));
    (void)gallus_mutex_unlock(&(q->m_get_lock));
    (void)gallus_mutex_unlock(&(q->m_put_lock));
  }
}


static inline void
s_awake_putter(gallus_bbq2_t q) {
  if (likely(q != NULL)) {
    (void)gallus_mutex_lock(&(q->m_put_lock));
    {
      (void)gallus_cond_notify(&(q->m_put_cond), true);      
    }
    (void)gallus_mutex_unlock(&(q->m_put_lock));
  }
}


static inline void
s_clean(gallus_bbq2_t q, bool free_values, bool do_wake) {
  if (likely(q != NULL)) {
    if (free_values == true) {
      s_freeup_all_values(q);
    }
    q->m_r_idx = 0;
    q->m_w_idx = 0;
    (void)memset((void *)(q->m_data), 0,
                 q->m_element_size * q->m_n_max_elements);
    (void)memset((void *)(q->m_put_results), 0,
                 sizeof(memcpy_result_t) * q->m_n_max_elements);
    (void)memset((void *)(q->m_get_results), 0,
                 sizeof(memcpy_result_t) * q->m_n_max_elements);
    q->m_n_elements = 0;
    q->m_n_elements_temp = 0;
    q->m_cur_put_seq = 0;
    q->m_done_put_seq = 0;
    q->m_cur_get_seq = 0;
    q->m_done_get_seq = 0;
    q->m_n_get_waiters = 0;
    q->m_n_put_waiters = 0;
    q->m_is_operational = true;

    if (do_wake == true) {
      s_awake_all(q);
    }
  }
}


static inline void
s_shutdown(gallus_bbq2_t q, bool free_values) {
  if (likely(q != NULL &&
             q->m_is_operational == true)) {
    s_clean(q, free_values, true);
    q->m_is_operational = false;
  }
}





static inline void
s_increment_amount(gallus_bbq2_t q, size_t amount) {
  q->m_n_elements += amount;
}


static inline void
s_decrement_amount(gallus_bbq2_t q, size_t amount) {
  q->m_n_elements -= amount;
}


static inline void
s_increment_temp_amount(gallus_bbq2_t q, size_t amount) {
  q->m_n_elements_temp += amount;
}


static inline void
s_decrement_temp_amount(gallus_bbq2_t q, size_t amount) {
  q->m_n_elements_temp -= amount;
}





static inline uint64_t
s_update_put_index(gallus_bbq2_t q, size_t max_n) {
  uint64_t ret = q->m_w_idx;

  q->m_w_idx += max_n;

  return ret;
}


static inline uint64_t
s_update_put_sequence(gallus_bbq2_t q) {
  uint64_t ret = q->m_cur_put_seq;

  q->m_cur_put_seq++;

  return ret;
}


static inline void
s_accumulate_put_result(gallus_bbq2_t q, uint64_t idx) {
  q->m_done_put_seq = q->m_put_results[idx].m_seq;
  s_increment_amount(q, q->m_put_results[idx].m_n_amount);
}


static inline size_t
s_do_copyin(gallus_bbq2_t q, uint64_t my_w_idx, void *buf, size_t max_n,
            uint64_t my_put_seq) {
  uint64_t idx = my_w_idx % q->m_n_max_elements;
  char *dst = q->m_data + idx * q->m_element_size;

  /*
   * Acquire a "slot" to store following memcpy()s' result/side effect
   * and store them into the slot.
   */
  uint64_t seq_idx = my_put_seq % q->m_n_max_elements;

  q->m_put_results[seq_idx].m_done = false;
  q->m_put_results[seq_idx].m_seq = my_put_seq;
  q->m_put_results[seq_idx].m_n_amount = max_n;

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

  /*
   * Update the memcpy() state.
   */
  q->m_put_results[seq_idx].m_done = true;

  return seq_idx;
}


static inline size_t
s_acquire_put_remains(gallus_bbq2_t q, uint64_t my_put_seq) {
  return my_put_seq - q->m_done_put_seq;
}


static inline uint64_t
s_acquire_put_index(gallus_bbq2_t q, uint64_t my_seq_idx, uint64_t relidx) {
  return (my_seq_idx + q->m_n_max_elements - relidx) % q->m_n_max_elements;
}


static inline bool
s_is_put_done(gallus_bbq2_t q, uint64_t idx) {
  return q->m_put_results[idx].m_done;
}

    
static inline size_t
s_copyin(gallus_bbq2_t q, void *buf, size_t n) {
  size_t max_rooms;
  size_t max_n;
  uint64_t my_w_idx;
  uint64_t my_put_seq;
  size_t copy_remains;
  int64_t i;
  uint64_t seq_idx;
  uint64_t idx;
  int do_flush;

  s_spinlock(q);
  {

    /*
     * First calculate copiable amount.
     */
    max_rooms = q->m_n_max_elements - q->m_n_elements_temp;
    max_n = (max_rooms < n) ? max_rooms : n;

    if (likely(max_n > 0)) {
      /*
       * And if it is greater than zero:
       *	1) decide write index for this thread.
       *	2) update the index by the amount.
       *	3) acquire a put ticket (my_put_seq) for this thread.
       *	4) update the global put ticket for this buffer.
       *	5) update temp. sum in this buffer.
       */
      my_w_idx = s_update_put_index(q, max_n);	/* 1), 2) */
      my_put_seq = s_update_put_sequence(q);	/* 3), 4) */
      s_increment_temp_amount(q, max_n);	/* 5) */

      mbar();
    }

  }
  s_spinunlock(q);
    
  if (likely(max_n > 0)) {
    /*
     * Copy the buf into the queue.
     */
    seq_idx = s_do_copyin(q, my_w_idx, buf, max_n, my_put_seq);

    s_spinlock(q);
    {

      /*
       * Determine how many memcpy() not yet be checked, as maximum.
       */
      copy_remains = s_acquire_put_remains(q, my_put_seq);

      /*
       * Scan the memcpy() results in chronological order of the
       * sequence #.
       */
      for (do_flush = 0, i = (int64_t)copy_remains; i >= 0; i--) {
        idx = s_acquire_put_index(q, seq_idx, (uint64_t)i);
        if (s_is_put_done(q, idx) == true) {
          /*
           * If the memcpy() which sequence # is younger than this is
           * done then:
           *	1) update the maximum sequence # which is done.
           *	2) accumulate amount which copied by the memcpy(),
           *	   into the global sum.
           */
          s_accumulate_put_result(q, idx);	/* 1), 2) */
          do_flush++;
        } else {
          /*
           * memcpy that sequence # is younger than this still
           * running. Just leave it alone for now.
           */
          break;
        }
      }

      if (do_flush > 0) {
        mbar();
      }

    }
    s_spinunlock(q);

    /*
     * And wake all the getters.
     */
    (void)gallus_cond_notify(&(q->m_get_cond), true);
  }

  return max_n;
}





static inline uint64_t
s_update_get_index(gallus_bbq2_t q, size_t max_n) {
  uint64_t ret = q->m_r_idx;

  q->m_r_idx += max_n;

  return ret;
}


static inline uint64_t
s_update_get_sequence(gallus_bbq2_t q) {
  uint64_t ret = q->m_cur_get_seq;

  q->m_cur_get_seq++;

  return ret;
}


static inline void
s_accumulate_get_result(gallus_bbq2_t q, uint64_t idx) {
  q->m_done_get_seq = q->m_get_results[idx].m_seq;
  s_decrement_temp_amount(q, q->m_get_results[idx].m_n_amount);
}


static inline void
s_do_copyout_pure(gallus_bbq2_t q, uint64_t my_r_idx,
                  void *buf, size_t max_n) {
  uint64_t idx =  my_r_idx % q->m_n_max_elements;
  char *src = q->m_data + idx * q->m_element_size;

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
}


static inline size_t
s_do_copyout(gallus_bbq2_t q, uint64_t my_r_idx, void *buf, size_t max_n,
             uint64_t my_get_seq) {
  /*
   * Acquire a "slot" to store following memcpy()s' result/side effect
   * and store them into the slot.
   */
  uint64_t seq_idx = my_get_seq % q->m_n_max_elements;
  q->m_get_results[seq_idx].m_done = false;
  q->m_get_results[seq_idx].m_seq = my_get_seq;
  q->m_get_results[seq_idx].m_n_amount = max_n;

  s_do_copyout_pure(q, my_r_idx, buf, max_n);

  /*
   * Update the memcpy() state.
   */
  q->m_get_results[seq_idx].m_done = true;

  return seq_idx;
}


static inline size_t
s_acquire_get_remains(gallus_bbq2_t q, uint64_t my_get_seq) {
  return my_get_seq - q->m_done_get_seq;
}


static inline uint64_t
s_acquire_get_index(gallus_bbq2_t q, uint64_t my_seq_idx, uint64_t relidx) {
  return (my_seq_idx + q->m_n_max_elements - relidx) % q->m_n_max_elements;
}


static inline bool
s_is_get_done(gallus_bbq2_t q, uint64_t idx) {
  return q->m_get_results[idx].m_done;
}


static inline size_t
s_copyout(gallus_bbq2_t q, void *buf, size_t n, bool do_incr) {
  size_t max_elem;
  size_t max_n;
  uint64_t my_r_idx;
  uint64_t my_get_seq;
  size_t copy_remains;
  int64_t i;
  uint64_t seq_idx;
  uint64_t idx;
  int do_flush;

  if (likely(do_incr == true)) {

    s_spinlock(q);
    {

      /*
       * First calculate copiable amount.
       */
      max_elem = q->m_n_elements;
      max_n = (n < max_elem) ? n : max_elem;

      if (likely(max_n > 0)) {
        /*
         * And ic it is greater than zero:
         *	1) decide read index for this thread.
         *	2) update the index by the amount.
         *	3) acquire a get ticket (my_get_seq) for this thread.
         *	4) update the global get ticket for this buffer.
         *	5) update global sum in this buffer.
         */
        my_r_idx = s_update_get_index(q, max_n);	/* 1), 2) */
        my_get_seq = s_update_get_sequence(q);		/* 3), 4) */
        s_decrement_amount(q, max_n);			/* 5) */

        mbar();
      }

    }
    s_spinunlock(q);

    if (likely(max_n > 0)) {    
      /*
       * Copy the queue contents to the buf.
       */
      seq_idx = s_do_copyout(q, my_r_idx, buf, max_n, my_get_seq);

      s_spinlock(q);
      {

        /*
         * Determine how many memcpy() not yet be checked, as maximum.
         */
        copy_remains = s_acquire_get_remains(q, my_get_seq);

        /*
         * Scan the memcpy() results in chronological order of the
         * sequence #.
         */
        for (do_flush = 0, i = (int64_t)copy_remains; i >= 0; i--) {
          idx = s_acquire_put_index(q, seq_idx, (uint64_t)i);
          if (s_is_get_done(q, idx) == true) {
            /*
             * If the memcpy() which sequence # is younger than this is
             * done then:
             *	1) update the maximum sequence # which is done.
             *	2) accumulate amount which copied by the memcpy(),
             *	   into the temp. sum.
             */
            s_accumulate_get_result(q, idx);	/* 1), 2) */
            do_flush++;
          } else {
            /*
             * memcpy that sequence # is younger than this still
             * running. Just leave it alone for now.
             */
            break;
          }
        }

        if (do_flush > 0) {
          mbar();
        }

      }
      s_spinunlock(q);

      /*
       * And wake all the putters.
       */
      (void)gallus_cond_notify(&(q->m_put_cond), true);

    }

  } else {

    s_spinlock(q);
    {

      /*
       * First calculate copiable amount.
       */
      max_elem = q->m_n_elements;
      max_n = (n < max_elem) ? n : max_elem;

      my_r_idx = q->m_r_idx;

    }
    s_spinunlock(q);

    if (likely(max_n > 0)) {
      s_do_copyout_pure(q, my_r_idx, buf, max_n);
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
             dir != wait_unknown)) {
    if (nsec != 0LL) {

      gallus_msg_debug(5, "wait " PF64(d) " nsec.\n",
                        nsec);

      if (dir == wait_put) {
        (void)gallus_mutex_lock(&(q->m_put_lock));
        {
          (void)__sync_fetch_and_add(&(q->m_n_put_waiters), 1);
          ret = gallus_cond_wait(&(q->m_put_cond), &(q->m_put_lock), nsec);
          (void)__sync_sub_and_fetch(&(q->m_n_put_waiters), 1);
        }
        (void)gallus_mutex_unlock(&(q->m_put_lock));
      } else if (dir == wait_get) {
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
          n_waiters = q->m_n_put_waiters + q->m_n_get_waiters;
          if (n_waiters == 0) {

            /*
             * All the waiters are gone. Wake the waker up.
             */

            gallus_msg_debug(5, "sync wakeup who woke me up.\n");

            (void)gallus_mutex_lock(&(q->m_muxlock));
            {
              q->m_is_awakened = false;
              (void)gallus_cond_notify(&(q->m_awaken_cond), true);
            }
            (void)gallus_mutex_unlock(&(q->m_muxlock));
          }

          ret = GALLUS_RESULT_WAKEUP_REQUESTED;

        } else {
          (void)gallus_mutex_lock(&(q->m_muxlock));
          {
            q->m_is_awakened = false;
            (void)gallus_cond_notify(&(q->m_awaken_cond), true);
          }
          (void)gallus_mutex_unlock(&(q->m_muxlock));

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
            ret = (gallus_result_t)n_copyout;
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





static inline gallus_result_t
s_bbq2_create(gallus_bbq2_t *qptr,
              unsigned int numa_node,
              size_t elemsize,
              size_t maxelems,
              gallus_bbq2_value_freeup_proc_t proc) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(qptr != NULL &&
             elemsize > 0 &&
             maxelems > 0)) {
    gallus_bbq2_t q =
        (gallus_bbq2_t)gallus_malloc_on_numanode(
            sizeof(*q) + elemsize * maxelems,
            numa_node);
    memcpy_result_t *put_res =
        (memcpy_result_t *)gallus_malloc_on_numanode(
            sizeof(memcpy_result_t) * maxelems,
            numa_node);
    memcpy_result_t *get_res =
        (memcpy_result_t *)gallus_malloc_on_numanode(
            sizeof(memcpy_result_t) * maxelems,
            numa_node);

    *qptr = NULL;

    if (likely(q != NULL &&
               put_res != NULL &&
               get_res != NULL)) {
      if (likely(((ret = gallus_mutex_create(&(q->m_muxlock))) ==
                  GALLUS_RESULT_OK) &&
                 ((ret = gallus_mutex_create(&(q->m_put_lock))) ==
                  GALLUS_RESULT_OK) &&
                 ((ret = gallus_mutex_create(&(q->m_get_lock))) ==
                  GALLUS_RESULT_OK) &&
                 ((ret = gallus_cond_create(&(q->m_put_cond))) ==
                  GALLUS_RESULT_OK) &&
                 ((ret = gallus_cond_create(&(q->m_get_cond))) ==
                  GALLUS_RESULT_OK) &&
                 ((ret = gallus_cond_create(&(q->m_awaken_cond))) ==
                  GALLUS_RESULT_OK) &&
                 ((ret = gallus_spinlock_initialize(&(q->m_spinlock))) ==
                  GALLUS_RESULT_OK))) {
        q->m_numa_node = numa_node;
        q->m_n_max_elements = maxelems;
        q->m_element_size = elemsize;
        q->m_del_proc = proc;
        q->m_put_results = put_res;
        q->m_get_results = get_res;

        s_clean(q, true, false);

        *qptr = q;

        ret = GALLUS_RESULT_OK;

      } else {
        gallus_free_on_numanode((void *)q);
      }
    } else {
      ret = GALLUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_bbq2_shutdown(gallus_bbq2_t *qptr,
                bool free_values) {
  if (likely(qptr != NULL &&
             *qptr != NULL)) {

    s_spinlock(*qptr);
    {
      s_shutdown(*qptr, free_values);
    }
    s_spinunlock(*qptr);

  }
}


static inline void
s_bbq2_destroy(gallus_bbq2_t *qptr,
               bool free_values) {
  if (likely(qptr != NULL &&
             *qptr != NULL)) {

    s_spinlock(*qptr);
    {
      s_shutdown(*qptr, free_values);
      gallus_free_on_numanode((*qptr)->m_put_results);
      gallus_free_on_numanode((*qptr)->m_get_results);
      gallus_cond_destroy(&((*qptr)->m_put_cond));
      gallus_cond_destroy(&((*qptr)->m_get_cond));
      gallus_cond_destroy(&((*qptr)->m_awaken_cond));
      gallus_mutex_destroy(&((*qptr)->m_muxlock));
      gallus_mutex_destroy(&((*qptr)->m_put_lock));
      gallus_mutex_destroy(&((*qptr)->m_get_lock));
    }
    s_spinunlock(*qptr);

    gallus_spinlock_finalize(&((*qptr)->m_spinlock));

    /*
     * FIXME:
     *	Don't free the object while it is used.
     */
    gallus_free_on_numanode((void *)*qptr);

    *qptr = NULL;
  }
}


static inline gallus_result_t
s_bbq2_clear(gallus_bbq2_t *qptr,
                      bool free_values) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(qptr != NULL &&
             *qptr != NULL)) {

    s_spinlock(*qptr);
    {
      s_clean(*qptr, free_values, false);
      s_awake_putter(*qptr);
    }
    s_spinunlock(*qptr);

    ret = GALLUS_RESULT_OK;

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_bbq2_wakeup(gallus_bbq2_t *qptr, gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(qptr != NULL &&
             *qptr != NULL)) {
    size_t n_waiters;

    n_waiters = (*qptr)->m_n_put_waiters + (*qptr)->m_n_get_waiters;
    if (n_waiters > 0) {

      (void)gallus_mutex_lock(&((*qptr)->m_muxlock));
      {

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

                  if ((ret = gallus_cond_wait(&((*qptr)->m_awaken_cond),
                                               &((*qptr)->m_muxlock), nsec)) ==
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

      }
      (void)gallus_mutex_unlock(&((*qptr)->m_muxlock));

    } else {	/* n_waiters > 0 */
      ret = GALLUS_RESULT_OK;
    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_bbq2_wait_gettable(gallus_bbq2_t *qptr,
                     gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  
  if (likely(qptr != NULL && *qptr != NULL)) {
    size_t n_elem = (*qptr)->m_n_elements;

    if (n_elem > 0) {
      ret = (gallus_result_t)n_elem;
    } else {
      ret = s_wait_gettable(*qptr, nsec);
      if (ret == GALLUS_RESULT_OK) {
        n_elem = (*qptr)->m_n_elements;
        ret = (gallus_result_t)n_elem; 
      }
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_bbq2_wait_puttable(gallus_bbq2_t *qptr,
                     gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(qptr != NULL && *qptr != NULL)) {
    size_t remains = (*qptr)->m_n_max_elements - (*qptr)->m_n_elements;
    if (remains > 0) {
      ret = (gallus_result_t)remains;
    } else {
      ret = s_wait_puttable(*qptr, nsec);
      if (ret == GALLUS_RESULT_OK) {
        remains = (*qptr)->m_n_max_elements - (*qptr)->m_n_elements;
        ret = (gallus_result_t)remains;
      }
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline gallus_result_t
s_bbq2_put(gallus_bbq2_t *qptr,
           void **valptr,
           size_t valsz,
           gallus_chrono_t nsec) {
  gallus_result_t n = s_put_n(qptr, (void *)valptr, 1LL, valsz, nsec, NULL);
  return (n == 1LL) ? GALLUS_RESULT_OK : n;
}


static inline gallus_result_t
s_bbq2_put_n(gallus_bbq2_t *qptr,
             void **valptr,
             size_t n_vals,
             size_t valsz,
             gallus_chrono_t nsec,
             size_t *n_actual_put) {
  return s_put_n(qptr, (void *)valptr, n_vals, valsz, nsec, n_actual_put);
}


static inline gallus_result_t
s_bbq2_get(gallus_bbq2_t *qptr,
           void **valptr,
           size_t valsz,
           gallus_chrono_t nsec) {
  gallus_result_t n = s_get_n(qptr, (void *)valptr, 1LL, 1LL,
                              valsz, nsec, NULL, true);
  return (n == 1LL) ? GALLUS_RESULT_OK : n;
}


static inline gallus_result_t
s_bbq2_get_n(gallus_bbq2_t *qptr,
             void **valptr,
             size_t n_vals_max,
             size_t n_at_least,
             size_t valsz,
             gallus_chrono_t nsec,
             size_t *n_actual_get) {
  return s_get_n(qptr, (void *)valptr, n_vals_max, n_at_least,
                 valsz, nsec, n_actual_get, true);
}


static inline gallus_result_t
s_bbq2_peek(gallus_bbq2_t *qptr,
            void **valptr,
            size_t valsz,
            gallus_chrono_t nsec) {
  gallus_result_t n = s_get_n(qptr, (void *)valptr, 1LL, 1LL,
                              valsz, nsec, NULL, false);
  return (n == 1LL) ? GALLUS_RESULT_OK : n;
}


static inline gallus_result_t
gallus_bbq2_peek_n(gallus_bbq2_t *qptr,
                   void **valptr,
                   size_t n_vals_max,
                   size_t n_at_least,
                   size_t valsz,
                   gallus_chrono_t nsec,
                   size_t *n_actual_get) {
  return s_get_n(qptr, (void *)valptr, n_vals_max, n_at_least,
                 valsz, nsec, n_actual_get, false);
}





static inline gallus_result_t
s_bbq2_size(gallus_bbq2_t *qptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(qptr != NULL &&
             *qptr != NULL)) {
    if (likely((*qptr)->m_is_operational == true)) {
      ret = (gallus_result_t)(*qptr)->m_n_elements;
    } else {
      ret = GALLUS_RESULT_NOT_OPERATIONAL;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_bbq2_remaining_capacity(gallus_bbq2_t *qptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(qptr != NULL &&
             *qptr != NULL)) {
    if (likely((*qptr)->m_is_operational == true)) {
      ret = (gallus_result_t)
            ((*qptr)->m_n_max_elements - (*qptr)->m_n_elements);
    } else {
      ret = GALLUS_RESULT_NOT_OPERATIONAL;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_bbq2_max_capacity(gallus_bbq2_t *qptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(qptr != NULL &&
             *qptr != NULL)) {
    if ((*qptr)->m_is_operational == true) {
      ret = (gallus_result_t)((*qptr)->m_n_max_elements);
    } else {
      ret = GALLUS_RESULT_NOT_OPERATIONAL;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_bbq2_is_full(gallus_bbq2_t *qptr, bool *retptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(qptr != NULL &&
             *qptr != NULL &&
             retptr != NULL)) {
    *retptr = false;
    if (likely((*qptr)->m_is_operational == true)) {
      *retptr = ((*qptr)->m_n_elements >= (*qptr)->m_n_max_elements)
                ? true : false;
      ret = GALLUS_RESULT_OK;
    } else {
      ret = GALLUS_RESULT_NOT_OPERATIONAL;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_bbq2_is_empty(gallus_bbq2_t *qptr, bool *retptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(qptr != NULL &&
             *qptr != NULL &&
             retptr != NULL)) {
    *retptr = false;
    if (likely((*qptr)->m_is_operational == true)) {
      *retptr = ((*qptr)->m_n_elements == 0) ? true : false;
      ret = GALLUS_RESULT_OK;
    } else {
      ret = GALLUS_RESULT_NOT_OPERATIONAL;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_bbq2_is_operational(gallus_bbq2_t *qptr, bool *retptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(qptr != NULL &&
             *qptr != NULL &&
             retptr != NULL)) {
    *retptr = (*qptr)->m_is_operational;
    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_bbq2_cancel_janitor(gallus_bbq2_t *qptr) {
  if (likely(qptr != NULL &&
             *qptr != NULL)) {
    s_spinunlock(*qptr);
    (void)gallus_mutex_unlock(&((*qptr)->m_muxlock));
    (void)gallus_mutex_unlock(&((*qptr)->m_get_lock));
    (void)gallus_mutex_unlock(&((*qptr)->m_put_lock));
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
