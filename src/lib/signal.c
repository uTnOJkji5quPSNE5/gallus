#include "gallus_apis.h"
#include "gallus_thread_internal.h"





#ifndef NSIG
#ifdef _NSIG
#define NSIG _NSIG
#else
#define NSIG	64	/* Super wild guess. */
#endif /* _NSIG */
#endif /* ! NSIG */





typedef struct signal_thread_record {
  struct gallus_thread_record m_thd;
  gallus_mutex_t m_lock;
  sighandler_t m_sigprocs[NSIG];
  sigset_t m_sigset;
  bool m_do_loop;
} signal_thread_record;
typedef signal_thread_record 	*signal_thread_t;





static void s_ctors(void) __attr_constructor__(108);
static void s_dtors(void) __attr_destructor__(108);
static void s_child_at_fork(void);

static pthread_once_t s_once_init = PTHREAD_ONCE_INIT;
static pthread_once_t s_once_final = PTHREAD_ONCE_INIT;
static bool s_is_first = true;
static bool s_is_last = false;
static sigset_t old_sigset;
static sighandler_t old_sigprocs[NSIG];
static signal_thread_record s_sigthd_rec;
static signal_thread_t s_sigthd = &s_sigthd_rec;

static sigset_t org_proc_sigset;





static inline void
s_sigfillset(sigset_t *ssptr) {
  (void)sigfillset(ssptr);
#if defined(SIGRTMIN) && defined(SIGRTMAX)
  /*
   * Exclude realtime signals (also for Linux's thread
   * cancellation).
   */
  {
    int i;
    int sMin = SIGRTMIN;
    int sMax = SIGRTMAX;
    for (i = sMin; i < sMax; i++) {
      (void)sigdelset(ssptr, i);
    }
  }
#endif /* SIGRTMIN && SIGRTMAX */
  /*
   * And exclude everything that seems for thread-related.
   */
#ifdef SIGWAITING
  (void)sigdelset(ssptr, SIGWAITING);
#endif /* SIGWAITING */
#ifdef SIGLWP
  (void)sigdelset(ssptr, SIGLWP);
#endif /* SIGLWP */
#ifdef SIGFREEZE
  (void)sigdelset(ssptr, SIGFREEZE);
#endif /* SIGFREEZE */
#ifdef SIGCANCEL
  (void)sigdelset(ssptr, SIGCANCEL);
#endif /* SIGCANCEL */
}


static inline void
s_block_all_signals(void) {
  sigset_t ss;
  s_sigfillset(&ss);
  (void)pthread_sigmask(SIG_SETMASK, &ss, NULL);
}


static inline void
s_unblock_all_signals(void) {
  sigset_t ss;
  (void)sigemptyset(&ss);
  (void)pthread_sigmask(SIG_SETMASK, &ss, NULL);
}





static inline void
s_reinitialize(signal_thread_t st) {
  if (st != NULL) {
    (void)gallus_mutex_reinitialize(&(st->m_lock));
  }
}


static inline void
s_lock(signal_thread_t st) {
  if (st != NULL) {
    (void)gallus_mutex_lock(&(st->m_lock));
  }
}


static inline void
s_unlock(signal_thread_t st) {
  if (st != NULL) {
    (void)gallus_mutex_unlock(&(st->m_lock));
  }
}


static inline sigset_t
s_get_sigset(signal_thread_t st) {
  sigset_t ret;

  if (st != NULL) {
    ret = st->m_sigset;
  } else {
    (void)sigemptyset(&ret);
  }

  return ret;
}


static inline void
s_save_sigprocs(signal_thread_t st, sighandler_t *procs) {
  if (st != NULL) {
    (void)memcpy((void *)procs,
                 (void *)(st->m_sigprocs),
                 sizeof((st->m_sigprocs)));
  } else {
    (void)memset((void *)procs, 0, sizeof((st->m_sigprocs)));
  }
}


static inline void
s_stop(signal_thread_t st) {
  if (st != NULL) {
    st->m_do_loop = false;
  }
}





static gallus_result_t
s_main(const gallus_thread_t *tptr, void *arg) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  (void)arg;

  if (tptr != NULL) {
    signal_thread_t st = (signal_thread_t)*tptr;

    if (st != NULL) {
      int sig;

      /*
       * Don't initialize stptr->m_sigset since it would be set before
       * the start.
       */
      while (st->m_do_loop == true) {

        s_lock(st);
        {
          s_block_all_signals();
          /*
           * Use ths SIGUSR2 as a interruption.
           */
          (void)sigaddset(&(st->m_sigset), SIGUSR2);
          (void)pthread_sigmask(SIG_SETMASK, &(st->m_sigset), NULL);
        }
        s_unlock(st);

        errno = 0;
        if (sigwait(&(st->m_sigset), &sig) == -1) {
          if (errno != EAGAIN) {
            perror("sigwait");
          }
          continue;
        }

        if (sig > 0 && sig < NSIG) {
          gallus_msg_debug(5, "Got a valid signal %d\n", sig);
          if (st->m_sigprocs[sig] != NULL) {
            (st->m_sigprocs[sig])(sig);
            gallus_msg_debug(5, "signal %d was handled.\n", sig);
          } else {
            if (sig == SIGUSR2) {
              gallus_msg_debug(5, "seems only notification.\n");
            } else {
              gallus_msg_debug(5, "signal %d was ignored.\n", sig);
            }
          }
        } else {
          gallus_msg_debug(5, "Got an in valid signal %d\n", sig);
        }
      }

      ret = GALLUS_RESULT_OK;
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static void
s_finalize(const gallus_thread_t *tptr, bool is_canceled,
           void *arg) {
  (void)tptr;
  (void)arg;

  gallus_msg_debug(5, "called. %s.\n",
                    (is_canceled == false) ? "finished" : "canceled");
}


static void
s_destroy(const gallus_thread_t *tptr, void *arg) {
  (void)arg;

  gallus_msg_debug(5, "called.\n");

  if (tptr != NULL) {
    bool isvalid = false;
    if (gallus_thread_is_valid(tptr, &isvalid) == GALLUS_RESULT_OK) {
      if (isvalid == true) {
        signal_thread_t st = (signal_thread_t)*tptr;
        if (st != NULL) {
          if (st->m_lock != NULL) {
            (void)gallus_mutex_destroy(&(st->m_lock));
          }
        }
      }
    }
  }
}


static inline gallus_result_t
s_create(signal_thread_t *stptr, sigset_t *ssptr, sighandler_t *procs) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (stptr != NULL) {
    if ((ret = gallus_thread_create((gallus_thread_t *)stptr,
                                     s_main,
                                     s_finalize,
                                     s_destroy,
                                     "signal thread",
                                     NULL)) == GALLUS_RESULT_OK) {
      if ((ret = gallus_mutex_create(&((*stptr)->m_lock))) ==
          GALLUS_RESULT_OK) {
        if (ssptr == NULL) {
          (void)sigemptyset(&((*stptr)->m_sigset));
        } else {
          (*stptr)->m_sigset = *ssptr;
        }
        if (procs == NULL) {
          (void)memset((void *)((*stptr)->m_sigprocs), 0,
                       sizeof((*stptr)->m_sigprocs));
        } else {
          (void)memcpy((void *)((*stptr)->m_sigprocs),
                       (void *)procs,
                       sizeof((*stptr)->m_sigprocs));
        }
        (*stptr)->m_do_loop = true;
      }
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline void
s_create_and_start(signal_thread_t *stptr, sigset_t *ssptr,
                   sighandler_t *procs) {
  gallus_result_t r = GALLUS_RESULT_ANY_FAILURES;

  s_block_all_signals();

  if ((r = s_create(stptr, ssptr, procs)) == GALLUS_RESULT_OK) {
    if ((r = gallus_thread_start((gallus_thread_t *)stptr, false)) !=
        GALLUS_RESULT_OK) {
      goto an_error;
    }
  } else {
  an_error:
    gallus_perror(r);
    gallus_exit_fatal("can't create a signal thread.\n");
    /* not reached. */
  }
}


static void
s_os_SIGUSR2_handler(int sig) {
  (void)sig;
  gallus_exit_fatal("OS SIGUSR2 handler called, must not happen.\n");
}


static void
s_once_init_proc(void) {
  s_sigthd = &s_sigthd_rec;

  (void)pthread_atfork(NULL, NULL, s_child_at_fork);

  (void)signal(SIGUSR2, s_os_SIGUSR2_handler);

  if (s_is_first == true) {
    s_is_first = false;

    (void)sigprocmask(SIG_UNBLOCK, NULL, &org_proc_sigset);
    s_create_and_start(&s_sigthd, NULL, NULL);
    /*
     * failsafe.
     */
    s_lock(s_sigthd);
    {
      old_sigset = s_get_sigset(s_sigthd);
      s_save_sigprocs(s_sigthd, old_sigprocs);
    }
    s_unlock(s_sigthd);
  } else {
    s_create_and_start(&s_sigthd, &old_sigset, old_sigprocs);
  }
}


static void
s_child_at_fork(void) {
  if (s_is_last == false) {
    s_reinitialize(s_sigthd);

    s_lock(s_sigthd);
    {
      old_sigset = s_get_sigset(s_sigthd);
      s_save_sigprocs(s_sigthd, old_sigprocs);
    }
    s_unlock(s_sigthd);

    gallus_thread_atfork_child((const gallus_thread_t *)&s_sigthd);
    (void)gallus_thread_destroy((gallus_thread_t *)&s_sigthd);

    s_once_init_proc();

    gallus_msg_debug(10, "signal thread re-initialized after fork(2).\n");
  }
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once_init, s_once_init_proc);
}


static void
s_ctors(void) {
  s_init();

  gallus_msg_debug(10, "signal thread initialized.\n");
}


static void
s_once_final_proc(void) {
  if (s_sigthd != NULL) {
    bool is_valid = false;
    gallus_result_t r =
      gallus_thread_is_valid((const gallus_thread_t *)&s_sigthd,
                              &is_valid);
    if (r == GALLUS_RESULT_OK && is_valid == true) {
      pthread_t tid;

      r = gallus_thread_get_pthread_id((const gallus_thread_t *)&s_sigthd,
                                        &tid);
      if (r == GALLUS_RESULT_OK) {
        pthread_t me;

        s_block_all_signals();
        (void)signal(SIGUSR2, SIG_IGN);

        s_lock(s_sigthd);
        {
          s_stop(s_sigthd);
        }
        s_unlock(s_sigthd);

        (void)kill(getpid(), SIGUSR2);

        me = pthread_self();
        if (me != tid) {
          (void)gallus_thread_wait((gallus_thread_t *)&s_sigthd, -1LL);
          (void)gallus_thread_destroy((gallus_thread_t *)&s_sigthd);
        }

        s_is_last = true;
      }
    }
  }
}


static inline void
s_final(void) {
  (void)pthread_once(&s_once_final, s_once_final_proc);
}


static void
s_dtors(void) {
  s_final();

  gallus_msg_debug(10, "signal thread finalized.\n");
}





gallus_result_t
gallus_signal(int signum, sighandler_t new, sighandler_t *oldptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (s_sigthd != NULL) {
    if (signum > 0 && signum < NSIG) {

      sigset_t cur_sigset;

      s_lock(s_sigthd);
      {
        cur_sigset = s_sigthd->m_sigset;

        if (oldptr != NULL) {
          int st;
          sighandler_t h = s_sigthd->m_sigprocs[signum];

          errno = 0;
          if ((st = sigismember(&cur_sigset, signum)) == 1) {
            if (h == NULL) {
              /*
               * We are waiting for the signum and no handler set,
               * menas we are ignoring the signum. So return SIG_IGN.
               */
              *oldptr = SIG_IGN;
            } else {
              /*
               * We are waiting for the signum and having a handler
               * for it. So return the handler.
               */
              *oldptr = h;
            }
          } else if (st == 0) {
            /*
             * We aren't waiting for the signum, means the OS default
             * handler is activated. So return SIG_DFL.
             */
            *oldptr = SIG_DFL;
          } else {
            ret = GALLUS_RESULT_POSIX_API_ERROR;
            goto unlock;
          }
        }

        switch ((uint64_t)new) {
          case (uint64_t)SIG_DFL: {
            if (signum != SIGUSR2) {
              (void)sigdelset(&cur_sigset, signum);
            }
            s_sigthd->m_sigprocs[signum] = NULL;
            break;
          }
          case (uint64_t)SIG_IGN: {
            (void)sigaddset(&cur_sigset, signum);
            s_sigthd->m_sigprocs[signum] = NULL;
            break;
          }
          default: {
            if (new != SIG_CUR) {
              (void)sigaddset(&cur_sigset, signum);
              s_sigthd->m_sigprocs[signum] = new;
            }
            break;
          }
        }

        if (new != SIG_CUR) {
          s_sigthd->m_sigset = cur_sigset;

          /*
           * Send myself SIGUSR2 for notification.
           */
          (void)kill(getpid(), SIGUSR2);

          gallus_msg_debug(5, "Notify sigmask change.\n");
        }

        ret = GALLUS_RESULT_OK;
      }
    unlock:
      s_unlock(s_sigthd);

    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = GALLUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


void
gallus_signal_old_school_semantics(void) {
  s_final();
  (void)sigprocmask(SIG_SETMASK, &org_proc_sigset, NULL);
  (void)signal(SIGUSR2, SIG_DFL);
}





void
__s_I_g_C_u_R__(int sig) {
  (void)sig;
}

