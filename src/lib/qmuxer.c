#include "gallus_apis.h"
#include "qmuxer_types.h"
#include "qmuxer_internal.h"





static inline void
s_lock(gallus_qmuxer_t qmx) {
  if (qmx != NULL) {
    (void)gallus_mutex_lock(&(qmx->m_lock));
  }
}


static inline void
s_unlock(gallus_qmuxer_t qmx) {
  if (qmx != NULL) {
    (void)gallus_mutex_unlock(&(qmx->m_lock));
  }
}


static inline gallus_result_t
s_qmx_initialize(gallus_qmuxer_t qmx) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qmx != NULL) {
    (void)memset((void *)qmx, 0, sizeof(*qmx));
    if (((ret = gallus_mutex_create(&(qmx->m_lock))) ==
         GALLUS_RESULT_OK) &&
        ((ret = gallus_cond_create(&(qmx->m_cond))) ==
         GALLUS_RESULT_OK)) {
      ;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_qmx_destroy(gallus_qmuxer_t qmx) {
  if (qmx != NULL) {

    if (qmx->m_lock != NULL) {

      s_lock(qmx);
      {
        if (qmx->m_cond != NULL) {
          (void)gallus_cond_destroy(&(qmx->m_cond));
        }
      }
      s_unlock(qmx);

      (void)gallus_mutex_destroy(&(qmx->m_lock));
    }

    free((void *)qmx);
  }
}


static inline gallus_result_t
s_qmx_setup_for_poll(gallus_qmuxer_t *qmxptr,
                     gallus_qmuxer_poll_t const polls[],
                     size_t npolls,
                     bool is_pre) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qmxptr != NULL &&
      *qmxptr != NULL &&
      polls != NULL &&
      npolls > 0) {
    gallus_result_t st;
    const gallus_qmuxer_poll_t *mpptr;
    size_t i;
    size_t n_nulls = 0;
    ssize_t nev = 0;

    for (i = 0; i < npolls; i++) {
      mpptr = &(polls[i]);

      if (mpptr != NULL &&
          *mpptr != NULL) {
        if ((*mpptr)->m_bbq != NULL) {
          st = cbuffer_setup_for_qmuxer((*mpptr)->m_bbq,
                                        *qmxptr,
                                        &((*mpptr)->m_q_size),
                                        &((*mpptr)->m_q_rem_capacity),
                                        (*mpptr)->m_type,
                                        is_pre);
          /*
           * Note:
           *	st < 0:		Any gallus API error(s).
           *	st == 0:	The queue is readable/writable.
           *	st == 1:	Need to wait for reaable.
           *	st == 2:	Need to wait for writable.
           *	st == 3:	Need to wait for both readable and writable.
           *			So this must not happen and the
           *			function guarantees it.
           */
          if (st == 0) {
            /*
             * Count up # of poll objects having events at this moment.
             */
            nev++;
          }
        } else {
          /*
           * Count up # of poll objects having a NULL queue.
           */
          n_nulls++;
          /*
           * And compensation for not calling the
           * cbuffer_setup_for_qmuxer().
           */
          st = 0;
          (*mpptr)->m_q_size = 0;
          (*mpptr)->m_q_rem_capacity = 0;
        }
        if (is_pre == true && st < 0) {
          ret = st;
          goto done;
        }
      } else {
        ret = GALLUS_RESULT_INVALID_ARGS;
        if (is_pre == true) {
          goto done;
        }
      }
    }

    if (n_nulls == npolls) {
      /*
       * All the poll objects are havinig a NULL bbq. What a shame.
       */
      ret = GALLUS_RESULT_INVALID_ARGS;
    } else {
      ret = nev;
    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}





gallus_result_t
gallus_qmuxer_create(gallus_qmuxer_t *qmxptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qmxptr != NULL) {
    *qmxptr = (gallus_qmuxer_t)malloc(sizeof(**qmxptr));
    if (*qmxptr == NULL) {
      ret = GALLUS_RESULT_NO_MEMORY;
      goto done;
    }
    ret = s_qmx_initialize(*qmxptr);
    if (ret != GALLUS_RESULT_OK) {
      s_qmx_destroy(*qmxptr);
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


void
gallus_qmuxer_destroy(gallus_qmuxer_t *qmxptr) {
  if (qmxptr != NULL) {
    s_qmx_destroy(*qmxptr);
  }
}





gallus_result_t
gallus_qmuxer_poll(gallus_qmuxer_t *qmxptr,
                    gallus_qmuxer_poll_t const polls[],
                    size_t npolls,
                    gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (qmxptr != NULL &&
      *qmxptr != NULL) {

    if (polls != NULL &&
        npolls > 0) {

    recheck:
      /*
       * Setup the polls for pre-wait.
       */
      ret = s_qmx_setup_for_poll(qmxptr, polls, npolls, true);
      if (ret == 0) {

        /*
         * Need to wait.
         *
         * Note:
         *	As you see, locking the muxer itself is almost
         *	meaningless. If we don't need to concern about the
         *	performance, we want to lock the qmuxer just like
         *	using a giant lock among all the queues in the
         *	polls. Only reason to lock the *qmxptr is because
         *	pthread_cond_wait() needs it, at least at this
         *	moment. We would change this if the qmuxer mechanism
         *	seems doropping events regular basis and if we realize
         *	that catching events is more important than the
         *	overall performance.
         */
        s_lock(*qmxptr);
        {
          ret = gallus_cond_wait(&((*qmxptr)->m_cond),
                                  &((*qmxptr)->m_lock),
                                  nsec);
        }
        s_unlock(*qmxptr);

        if (ret == GALLUS_RESULT_OK) {
          /*
           * Check if any events occur on all bbq/polls.
           */
          ret = s_qmx_setup_for_poll(qmxptr, polls, npolls, false);
          if (ret != 0) {
            /*
             * ret > 0 ... Having at least a queue that is ready for
             *		   play with.
             * ret < 0 ... An error.
             *
             *	no matter what the ret is we have to return anyway.
             */
            goto done;
          } else {
            /*
             * What happens here:
             *
             *	We are just awakened even the queues have no
             *	events (means ret == 0). Why ?:
             *
             *		1) We are running on a operating system other
             *		than the Linux since the Linux guarantees that
             *		the sleepers in pthread_cond_wait(3) are
             *		awakened ONLY by calling the
             *		pthread_cond_broadcast(3)/pthread_cond_signal(3),
             *		no system dependent events wakes the sleepers.
             *
             *		2) Most likely, any other threads just steal
             *		the events already. If this occured, maybe you
             *		better think why and rewrite your code.
             *
             *	Anyway, we have to start this all over again.
             */
            goto recheck;
          }
        }
        /*
         * else {
         *	Any errors occur. Just return.
         * }
         */
      }
      /*
       * else {
       *	We already have at least a queue having events, or, an
       *	error occurs. Either ways Just return.
       * }
       */
    } else {
      if (nsec > 0) {
        /*
         * No queues to poll are specified but a timeout is
         * specified. Just wait.
         */
        s_lock(*qmxptr);
        {
          ret = gallus_cond_wait(&((*qmxptr)->m_cond),
                                  &((*qmxptr)->m_lock),
                                  nsec);
        }
        s_unlock(*qmxptr);
      } else {
        ret = GALLUS_RESULT_INVALID_ARGS;
      }
    }
  } else {
    if (nsec > 0) {
      /*
       * No valid muxer is specified. just sleep.
       */
      ret = gallus_chrono_nanosleep(nsec, NULL);
      if (ret == GALLUS_RESULT_OK) {
        ret = GALLUS_RESULT_TIMEDOUT;
      }
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  }

done:
  return ret;
}





void
qmuxer_notify(gallus_qmuxer_t qmx) {
  if (qmx != NULL) {

    s_lock(qmx);
    {
      (void)gallus_cond_notify(&(qmx->m_cond), true);
    }
    s_unlock(qmx);

  }
}


void
gallus_qmuxer_cancel_janitor(gallus_qmuxer_t *qmxptr) {
  if (qmxptr != NULL &&
      *qmxptr != NULL) {
    s_unlock(*qmxptr);
  }
}
