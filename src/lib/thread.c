#include "gallus_apis.h"
#include "gallus_thread_internal.h"





#define MAX_CPUS	1024	/* Yeah, we mean it. */
#define DEFAULT_THREAD_ALLOC_SZ	(sizeof(gallus_thread_record))





static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static bool s_is_inited = false;
static pthread_attr_t s_attr;
static gallus_hashmap_t s_thd_tbl;
static gallus_hashmap_t s_alloc_tbl;
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
static size_t s_cpu_set_sz;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
static void s_ctors(void) __attr_constructor__(106);
static void s_dtors(void) __attr_destructor__(106);





static void
s_child_at_fork(void) {
  gallus_hashmap_atfork_child(&s_thd_tbl);
  gallus_hashmap_atfork_child(&s_alloc_tbl);
}


static void
s_once_proc(void) {
  int st;
  gallus_result_t r;

  if ((st = pthread_attr_init(&s_attr)) == 0) {
    if ((st = pthread_attr_setdetachstate(&s_attr,
                                          PTHREAD_CREATE_DETACHED)) != 0) {
      errno = st;
      perror("pthread_attr_setdetachstate");
      gallus_exit_fatal("can't initialize detached thread attr.\n");
    }
  } else {
    errno = st;
    perror("pthread_attr_init");
    gallus_exit_fatal("can't initialize thread attr.\n");
  }

#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  s_cpu_set_sz = CPU_ALLOC_SIZE(MAX_CPUS);
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */

  if ((r = gallus_hashmap_create(&s_thd_tbl, GALLUS_HASHMAP_TYPE_ONE_WORD,
                                  NULL)) != GALLUS_RESULT_OK) {
    gallus_perror(r);
    gallus_exit_fatal("can't initialize the thread table.\n");
  }
  if ((r = gallus_hashmap_create(&s_alloc_tbl, GALLUS_HASHMAP_TYPE_ONE_WORD,
                                  NULL)) != GALLUS_RESULT_OK) {
    gallus_perror(r);
    gallus_exit_fatal("can't initialize the thread allocation table.\n");
  }

  (void)pthread_atfork(NULL, NULL, s_child_at_fork);

  s_is_inited = true;
}


static inline void
s_init(void) {
  gallus_heapcheck_module_initialize();
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  gallus_msg_debug(10, "The thread module is initialized.\n");
}


static void
s_final(void) {
  gallus_hashmap_destroy(&s_thd_tbl, true);
  gallus_hashmap_destroy(&s_alloc_tbl, true);
}


static void
s_dtors(void) {
  if (s_is_inited == true) {
    if (gallus_module_is_unloading() &&
        gallus_module_is_finalized_cleanly()) {
      s_final();

      gallus_msg_debug(10, "The thread module is finalized.\n");
    } else {
      gallus_msg_debug(10, "The thread module is not finalized "
                    "because of module finalization problem.\n");
    }
  }
}





static inline void
s_add_thd(const gallus_thread_t thd) {
  void *val = (void *)true;
  (void)gallus_hashmap_add(&s_thd_tbl, (void *)thd, &val, true);
}


static inline void
s_alloc_mark_thd(const gallus_thread_t thd) {
  void *val = (void *)true;
  (void)gallus_hashmap_add(&s_alloc_tbl, (void *)thd, &val, true);
}


static inline void
s_delete_thd(const gallus_thread_t thd) {
  (void)gallus_hashmap_delete(&s_thd_tbl, (void *)thd, NULL, true);
  (void)gallus_hashmap_delete(&s_alloc_tbl, (void *)thd, NULL, true);
}


static inline bool
s_is_thd(const gallus_thread_t thd) {
  void *val;
  gallus_result_t r = gallus_hashmap_find(&s_thd_tbl, (void *)thd, &val);
  return (r == GALLUS_RESULT_OK && (bool)val == true) ?
         true : false;
}


static inline bool
s_is_alloc_marked(const gallus_thread_t thd) {
  void *val;
  gallus_result_t r = gallus_hashmap_find(&s_alloc_tbl, (void *)thd, &val);
  return (r == GALLUS_RESULT_OK && (bool)val == true) ?
         true : false;
}


static inline void
s_op_lock(const gallus_thread_t thd) {
  if (thd != NULL) {
    (void)gallus_mutex_lock(&(thd->m_op_lock));
  }
}


static inline void
s_op_unlock(const gallus_thread_t thd) {
  if (thd != NULL) {
    (void)gallus_mutex_unlock(&(thd->m_op_lock));
  }
}


static inline void
s_wait_lock(const gallus_thread_t thd) {
  if (thd != NULL) {
    (void)gallus_mutex_lock(&(thd->m_wait_lock));
  }
}


static inline void
s_wait_unlock(const gallus_thread_t thd) {
  if (thd != NULL) {
    (void)gallus_mutex_unlock(&(thd->m_wait_lock));
  }
}


static inline void
s_cancel_lock(const gallus_thread_t thd) {
  if (thd != NULL) {
    (void)gallus_mutex_lock(&(thd->m_cancel_lock));
  }
}


static inline void
s_cancel_unlock(const gallus_thread_t thd) {
  if (thd != NULL) {
    (void)gallus_mutex_unlock(&(thd->m_cancel_lock));
  }
}





static inline gallus_result_t
s_initialize(gallus_thread_t thd,
	     bool is_allocd,
             gallus_thread_main_proc_t mainproc,
             gallus_thread_finalize_proc_t finalproc,
             gallus_thread_freeup_proc_t freeproc,
             const char *name,
             void *arg) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thd != NULL) {
    (void)memset((void *)thd, 0, sizeof(*thd));
    s_add_thd(thd);
    if (((ret = gallus_mutex_create(&(thd->m_wait_lock))) ==
         GALLUS_RESULT_OK) &&
        ((ret = gallus_mutex_create(&(thd->m_op_lock))) ==
         GALLUS_RESULT_OK) &&
        ((ret = gallus_mutex_create(&(thd->m_cancel_lock))) ==
         GALLUS_RESULT_OK) &&
        ((ret = gallus_mutex_create(&(thd->m_finalize_lock))) ==
         GALLUS_RESULT_OK) &&
        ((ret = gallus_cond_create(&(thd->m_wait_cond))) ==
         GALLUS_RESULT_OK) &&
        ((ret = gallus_cond_create(&(thd->m_startup_cond))) ==
         GALLUS_RESULT_OK) &&
        ((ret = gallus_cond_create(&(thd->m_finalize_cond))) ==
         GALLUS_RESULT_OK)) {
      thd->m_arg = arg;
      thd->m_is_allocd = is_allocd;
      if (IS_VALID_STRING(name) == true) {
        snprintf(thd->m_name, sizeof(thd->m_name), "%s", name);
      }
      thd->m_creator_pid = getpid();
      thd->m_pthd = GALLUS_INVALID_THREAD;

      thd->m_main_proc = mainproc;
      thd->m_final_proc = finalproc;
      thd->m_freeup_proc = freeproc;
      thd->m_result_code = GALLUS_RESULT_NOT_STARTED;

      thd->m_is_started = false;
      thd->m_is_activated = false;
      thd->m_is_canceled = false;
      thd->m_is_finalized = false;
      thd->m_is_destroying = false;

      thd->m_do_autodelete = false;
      thd->m_startup_sync_done = false;
      thd->m_n_finalized_count = 0LL;

      ret = GALLUS_RESULT_OK;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_finalize(gallus_thread_t thd, bool is_canceled,
           gallus_result_t rcode) {
  if (likely(thd != NULL)) {
    gallus_thread_t cthd = thd;
    bool is_valid = false;
    gallus_result_t r = gallus_thread_is_valid(&cthd, &is_valid);

    if (likely(r == GALLUS_RESULT_OK && is_valid == true)) {
      bool do_autodelete = false;
      int o_cancel_state;

      (void)gallus_mutex_enter_critical(&(thd->m_finalize_lock),
                                         &o_cancel_state);
      {

        gallus_msg_debug(10, "enter: %s\n",
                          (is_canceled == true) ? "canceled" : "exit");

        s_cancel_lock(thd);
        {
          thd->m_is_canceled = is_canceled;
        }
        s_cancel_unlock(thd);

        s_op_lock(thd);
        {
          thd->m_result_code = rcode;
        }
        s_op_unlock(thd);

        s_wait_lock(thd);
        {
          if (thd->m_final_proc != NULL) {
            if (likely(thd->m_n_finalized_count == 0)) {
              thd->m_final_proc(&thd, is_canceled, thd->m_arg);
            } else {
              gallus_msg_warning("the thread is already %s, count "
                                  PFSZ(u) " (now %s)\n",
                                  (thd->m_is_canceled == false) ?
                                  "exit" : "canceled",
                                  thd->m_n_finalized_count,
                                  (is_canceled == false) ?
                                  "exit" : "canceled");
            }
          }
          thd->m_pthd = GALLUS_INVALID_THREAD;
          do_autodelete = thd->m_do_autodelete;
          thd->m_is_finalized = true;
          thd->m_is_activated = false;
          thd->m_n_finalized_count++;
          (void)gallus_cond_notify(&(thd->m_wait_cond), true);
        }
        s_wait_unlock(thd);

        (void)gallus_cond_notify(&(thd->m_finalize_cond), true);

      }
      (void)gallus_mutex_leave_critical(&(thd->m_finalize_lock),
                                         o_cancel_state);

      if (do_autodelete == true) {
        gallus_thread_destroy(&thd);
      }

      /*
       * NOTE:
       *
       *	Don't call pthread_exit() at here because now it
       *	triggers the cancel handler as of current cancellation
       *	handling.
       */
    }
  }
}


static inline void
s_delete(gallus_thread_t thd) {
  if (thd != NULL) {
    if (s_is_thd(thd) == true) {
      if (thd->m_is_allocd == true ||
          s_is_alloc_marked(thd) == true) {
        gallus_msg_debug(10, "free %p\n", (void *)thd);
        free((void *)thd);
      } else {
        if (gallus_heapcheck_is_in_heap((const void *)thd) == true) {
          gallus_msg_debug(10, "%p is a thread object NOT allocated by the "
                            "gallus_thread_create(). If you want to free(3) "
                            "this by calling the gallus_thread_destroy(), "
                            "call gallus_thread_free_when_destroy(%p).\n",
                            (void *)thd, (void *)thd);
        } else {
          gallus_msg_debug(10, "A thread was created in an address %p and it "
                            "seems not in the heap area.\n", (void *)thd);
        }
      }
      s_delete_thd(thd);
    }
  }
}


static inline void
s_destroy(gallus_thread_t *thdptr, bool is_clean_finish) {
  if (thdptr != NULL && *thdptr != NULL) {

    s_wait_lock(*thdptr);
    {

      if (is_clean_finish == true) {

        if ((*thdptr)->m_is_destroying == false) {
          (*thdptr)->m_is_destroying = true;

          if ((*thdptr)->m_freeup_proc != NULL) {
            (*thdptr)->m_freeup_proc(thdptr, (*thdptr)->m_arg);
          }
        }

      }

      if ((*thdptr)->m_cpusetptr != NULL) {
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
        CPU_FREE((*thdptr)->m_cpusetptr);
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
        (*thdptr)->m_cpusetptr = NULL;
      }

      if ((*thdptr)->m_op_lock != NULL) {
        (void)gallus_mutex_destroy(&((*thdptr)->m_op_lock));
        (*thdptr)->m_op_lock = NULL;
      }
      if ((*thdptr)->m_cancel_lock != NULL) {
        (void)gallus_mutex_destroy(&((*thdptr)->m_cancel_lock));
        (*thdptr)->m_cancel_lock = NULL;
      }
      if ((*thdptr)->m_finalize_lock != NULL) {
        (void)gallus_mutex_destroy(&((*thdptr)->m_finalize_lock));
        (*thdptr)->m_finalize_lock = NULL;
      }
      if ((*thdptr)->m_wait_cond != NULL) {
        (void)gallus_cond_destroy(&((*thdptr)->m_wait_cond));
        (*thdptr)->m_wait_cond = NULL;
      }
      if ((*thdptr)->m_startup_cond != NULL) {
        (void)gallus_cond_destroy(&((*thdptr)->m_startup_cond));
        (*thdptr)->m_startup_cond = NULL;
      }
      if ((*thdptr)->m_finalize_cond != NULL) {
        (void)gallus_cond_destroy(&((*thdptr)->m_finalize_cond));
        (*thdptr)->m_finalize_cond = NULL;
      }

    }
    s_wait_unlock(*thdptr);

    if ((*thdptr)->m_wait_lock != NULL) {
      (void)gallus_mutex_destroy(&((*thdptr)->m_wait_lock));
    }

    s_delete(*thdptr);
  }
}





static void
s_pthd_cancel_handler(void *ptr) {
  if (ptr != NULL) {
    gallus_thread_t thd = (gallus_thread_t)ptr;
    s_finalize(thd, true, GALLUS_RESULT_INTERRUPTED);
  }
}


static void *
s_pthd_entry_point(void *ptr) {
  if (likely(ptr != NULL)) {

    pthread_cleanup_push(s_pthd_cancel_handler, ptr);
    {
      int o_cancel_state;
      gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
      gallus_thread_t thd = (gallus_thread_t)ptr;
      const gallus_thread_t *tptr =
        (const gallus_thread_t *)&thd;

      (void)gallus_mutex_enter_critical(&(thd->m_wait_lock), &o_cancel_state);
      {

        s_cancel_lock(thd);
        {
          thd->m_is_canceled = false;
        }
        s_cancel_unlock(thd);

        thd->m_is_finalized = false;
        thd->m_is_activated = true;
        thd->m_is_started = true;
        (void)gallus_cond_notify(&(thd->m_startup_cond), true);

        /*
         * The notifiction is done and then the parent thread send us
         * "an ACK". Wait for it.
         */
     sync_done_check:
        mbar();
        if (thd->m_startup_sync_done == false) {
          ret = gallus_cond_wait(&(thd->m_startup_cond),
                                  &(thd->m_wait_lock),
                                  -1);

          if (ret == GALLUS_RESULT_OK) {
            goto sync_done_check;
          } else {
            gallus_perror(ret);
            gallus_msg_error("startup synchronization failed with the"
                              "parent thread.\n");
          }
        }

      }
      (void)gallus_mutex_leave_critical(&(thd->m_wait_lock), o_cancel_state);

      ret = thd->m_main_proc(tptr, thd->m_arg);

      /*
       * NOTE:
       *
       *	This very moment this thread could be cancelled and no
       *	correct result is set. Even it is impossible to
       *	prevent it completely, try to prevent it anyway.
       */
      (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &o_cancel_state);

      /*
       * We got through anyhow.
       */
      s_finalize(thd, false, ret);

      /*
       * NOTE
       *
       *	Don't call pthread_exit() at here or the cancellation
       *	handler will be called.
       */
    }
    pthread_cleanup_pop(0);

  }

  return NULL;
}





gallus_result_t
gallus_thread_create_with_size(gallus_thread_t *thdptr,
			    size_t alloc_size,
			    gallus_thread_main_proc_t mainproc,
			    gallus_thread_finalize_proc_t finalproc,
			    gallus_thread_freeup_proc_t freeproc,
			    const char *name,
			    void *arg) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      mainproc != NULL) {
    gallus_thread_t thd;
    size_t sz = alloc_size;
    bool is_allocd = false;

    if (*thdptr == NULL) {
      if (sz < DEFAULT_THREAD_ALLOC_SZ) {
        sz = DEFAULT_THREAD_ALLOC_SZ;
      }
      thd = (gallus_thread_t)malloc(sz);
      if (thd != NULL) {
        (void)memset(thd, 0, sz);
        *thdptr = thd;
        s_alloc_mark_thd(thd);
      } else {
        *thdptr = NULL;
        ret = GALLUS_RESULT_NO_MEMORY;
        goto done;
      }
      is_allocd = true;
    } else {
      thd = *thdptr;
    }

    ret = s_initialize(thd, is_allocd,
		       mainproc, finalproc, freeproc, name, arg);
    if (ret != GALLUS_RESULT_OK) {
      s_destroy(&thd, false);
    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


gallus_result_t
gallus_thread_create(gallus_thread_t *thdptr,
                      gallus_thread_main_proc_t mainproc,
                      gallus_thread_finalize_proc_t finalproc,
                      gallus_thread_freeup_proc_t freeproc,
                      const char *name,
                      void *arg) {
  return gallus_thread_create_with_size(thdptr, DEFAULT_THREAD_ALLOC_SZ,
                                     mainproc, finalproc, freeproc,
                                     name, arg);
}


gallus_result_t
gallus_thread_start(const gallus_thread_t *thdptr,
                     bool autodelete) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {

      int st;

      s_wait_lock(*thdptr);
      {
        if ((*thdptr)->m_is_activated == false) {
          (*thdptr)->m_do_autodelete = autodelete;
          errno = 0;
          (*thdptr)->m_pthd = GALLUS_INVALID_THREAD;
          if ((st = pthread_create((pthread_t *)&((*thdptr)->m_pthd),
                                   &s_attr,
                                   s_pthd_entry_point,
                                   (void *)*thdptr)) == 0) {
#ifdef HAVE_PTHREAD_SETNAME_NP
            if (IS_VALID_STRING((*thdptr)->m_name) == true) {
              (void)pthread_setname_np((*thdptr)->m_pthd, (*thdptr)->m_name);
            }
#endif /* HAVE_PTHREAD_SETNAME_NP */
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
            if ((*thdptr)->m_cpusetptr != NULL &&
                (st =
                   pthread_setaffinity_np((*thdptr)->m_pthd,
                                          s_cpu_set_sz,
                                          (*thdptr)->m_cpusetptr)) != 0) {
              int s_errno = errno;
              errno = st;
              gallus_perror(GALLUS_RESULT_POSIX_API_ERROR);
              gallus_msg_warning("Can't set cpu affinity.\n");
              errno = s_errno;
            }
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
            (*thdptr)->m_is_activated = false;
            (*thdptr)->m_is_finalized = false;
            (*thdptr)->m_is_destroying = false;
            (*thdptr)->m_is_canceled = false;
            (*thdptr)->m_is_started = false;
            (*thdptr)->m_startup_sync_done = false;
            (*thdptr)->m_n_finalized_count = 0LL;
            mbar();

            /*
             * Wait the spawned thread starts.
             */

          startcheck:
            mbar();
            if ((*thdptr)->m_is_started == false) {

              /*
               * Note that very here, very this moment the spawned
               * thread starts to run since this thread sleeps via
               * calling of the pthread_cond_wait() that implies
               * release of the lock.
               */

              ret = gallus_cond_wait(&((*thdptr)->m_startup_cond),
                                      &((*thdptr)->m_wait_lock),
                                      -1);

              if (ret == GALLUS_RESULT_OK) {
                goto startcheck;
              } else {
                goto unlock;
              }
            }

            /*
             * The newly created thread notified its start. Then "send
             * an ACK" to the thread to notify that the thread can do
             * anything, including its deletion.
             */
            (*thdptr)->m_startup_sync_done = true;
            (void)gallus_cond_notify(&((*thdptr)->m_startup_cond), true);

            ret = GALLUS_RESULT_OK;

          } else {
            errno = st;
            ret = GALLUS_RESULT_POSIX_API_ERROR;
          }
        } else {
          ret = GALLUS_RESULT_ALREADY_EXISTS;
        }
      }
    unlock:
      s_wait_unlock(*thdptr);

    } else {
      ret = GALLUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_thread_cancel(const gallus_thread_t *thdptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {
    if (getpid() == (*thdptr)->m_creator_pid) {
      if (s_is_thd(*thdptr) == true) {

        s_wait_lock(*thdptr);
        {
          if ((*thdptr)->m_is_activated == true) {

            s_cancel_lock(*thdptr);
            {
              if ((*thdptr)->m_is_canceled == false &&
                  (*thdptr)->m_pthd != GALLUS_INVALID_THREAD) {
                int st;

                errno = 0;
                if ((st = pthread_cancel((*thdptr)->m_pthd)) == 0) {
                  ret = GALLUS_RESULT_OK;
                } else {
                  errno = st;
                  ret = GALLUS_RESULT_POSIX_API_ERROR;
                }
              } else {
                ret = GALLUS_RESULT_OK;
              }
            }
            s_cancel_unlock(*thdptr);

          } else {
            ret = GALLUS_RESULT_OK;
          }
        }
        s_wait_unlock(*thdptr);

        /*
         * Very here, very this moment the s_finalize() would start to
         * run.
         */

      } else {
        ret = GALLUS_RESULT_INVALID_OBJECT;
      }
    } else {
      ret = GALLUS_RESULT_NOT_OWNER;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_thread_wait(const gallus_thread_t *thdptr,
                    gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {
    if (getpid() == (*thdptr)->m_creator_pid) {
      if (s_is_thd(*thdptr) == true) {
        if ((*thdptr)->m_do_autodelete == false) {

          int o_cancel_state;

          (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                       &o_cancel_state);
          {

            s_wait_lock(*thdptr);
            {
           waitcheck:
              mbar();
              if ((*thdptr)->m_is_activated == true) {
                ret = gallus_cond_wait(&((*thdptr)->m_wait_cond),
                                        &((*thdptr)->m_wait_lock),
                                        nsec);
                if (ret == GALLUS_RESULT_OK) {
                  goto waitcheck;
                }
              } else {
                ret = GALLUS_RESULT_OK;
              }
            }
            s_wait_unlock(*thdptr);

            if ((*thdptr)->m_is_started == true) {

              (void)gallus_mutex_lock(&((*thdptr)->m_finalize_lock));
              {
                mbar();
             finalcheck:
                if ((*thdptr)->m_n_finalized_count == 0) {
                  ret = gallus_cond_wait(&((*thdptr)->m_finalize_cond),
                                          &((*thdptr)->m_finalize_lock),
                                          nsec);
                  if (ret == GALLUS_RESULT_OK) {
                    goto finalcheck;
                  }
                }
              }
              (void)gallus_mutex_unlock(&((*thdptr)->m_finalize_lock));

            }

          }
          (void)pthread_setcancelstate(o_cancel_state, NULL);

        } else {
          ret = GALLUS_RESULT_NOT_OPERATIONAL;
        }
      } else {
        ret = GALLUS_RESULT_INVALID_OBJECT;
      }
    } else {
      ret = GALLUS_RESULT_NOT_OWNER;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
gallus_thread_destroy(gallus_thread_t *thdptr) {
  if (thdptr != NULL &&
      *thdptr != NULL &&
      s_is_thd(*thdptr) == true) {
    (void)gallus_thread_cancel(thdptr);
    (void)gallus_thread_wait(thdptr, -1);

    s_destroy(thdptr, true);
    *thdptr = NULL;
  }
}


static inline gallus_result_t
s_get_pthdid(const gallus_thread_t *thdptr,
             pthread_t *tidptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if ((*thdptr)->m_is_started == true) {
    *tidptr = (*thdptr)->m_pthd;
    if ((*thdptr)->m_pthd != GALLUS_INVALID_THREAD) {
      ret = GALLUS_RESULT_OK;
    } else {
      ret = GALLUS_RESULT_ALREADY_HALTED;
    }
  } else {
    ret = GALLUS_RESULT_NOT_STARTED;
  }

  return ret;
}


gallus_result_t
gallus_thread_get_pthread_id(const gallus_thread_t *thdptr,
                              pthread_t *tidptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      tidptr != NULL) {
    if (s_is_thd(*thdptr) == true) {
      int o_cancel_state;
      *tidptr = GALLUS_INVALID_THREAD;

      (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &o_cancel_state);
      {

        s_wait_lock(*thdptr);
        {

          ret = s_get_pthdid(thdptr, tidptr);

        }
        s_wait_unlock(*thdptr);

      }
      (void)pthread_setcancelstate(o_cancel_state, NULL);

    } else {
      ret = GALLUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_thread_set_cpu_affinity(const gallus_thread_t *thdptr,
                                int cpu) {
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL && *thdptr != NULL) {
    if (s_is_thd(*thdptr) == true) {
      pthread_t tid;

      s_wait_lock(*thdptr);
      {
        if ((*thdptr)->m_cpusetptr == NULL) {
          (*thdptr)->m_cpusetptr = CPU_ALLOC(MAX_CPUS);
          if ((*thdptr)->m_cpusetptr == NULL) {
            ret = GALLUS_RESULT_NO_MEMORY;
            goto done;
          }
          CPU_ZERO_S(s_cpu_set_sz, (*thdptr)->m_cpusetptr);
        }

        if (cpu >= 0) {
          CPU_SET_S((size_t)cpu, s_cpu_set_sz, (*thdptr)->m_cpusetptr);
        } else {
          CPU_ZERO_S(s_cpu_set_sz, (*thdptr)->m_cpusetptr);
        }

        ret = s_get_pthdid(thdptr, &tid);
        if (ret == GALLUS_RESULT_OK) {
          int st;
          if ((st = pthread_setaffinity_np(tid,
                                           s_cpu_set_sz,
                                           (*thdptr)->m_cpusetptr)) == 0) {
            ret = GALLUS_RESULT_OK;
          } else {
            errno = st;
            ret = GALLUS_RESULT_POSIX_API_ERROR;
          }
        } else {
          if (ret == GALLUS_RESULT_NOT_STARTED) {
            ret = GALLUS_RESULT_OK;
          }
        }

      }
    done:
      s_wait_unlock(*thdptr);

    } else {
      ret = GALLUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
#else
  (void)thdptr;
  (void)cpu;
  return GALLUS_RESULT_OK;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
}


gallus_result_t
gallus_thread_get_cpu_affinity(const gallus_thread_t *thdptr) {
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL && *thdptr != NULL) {
    if (s_is_thd(*thdptr) == true) {
      pthread_t tid;

      s_wait_lock(*thdptr);
      {

        if ((ret = s_get_pthdid(thdptr, &tid)) == GALLUS_RESULT_OK) {
          int st;
          cpu_set_t *cur_set = CPU_ALLOC(MAX_CPUS);

          if (cur_set == NULL) {
            ret = GALLUS_RESULT_NO_MEMORY;
            goto done;
          }
          CPU_ZERO_S(s_cpu_set_sz, (*thdptr)->m_cpusetptr);

          if ((st = pthread_getaffinity_np(tid, s_cpu_set_sz, cur_set)) == 0) {
            int i;
            int cpu = -INT_MAX;

            for (i = 0; i < MAX_CPUS; i++) {
              if (CPU_ISSET_S((unsigned)i, s_cpu_set_sz, cur_set)) {
                cpu = (gallus_result_t)i;
                break;
              }
            }

            if (cpu != -INT_MAX) {

#if SIZEOF_PTHREAD_T == SIZEOF_INT64_T
#define TIDFMT "0x" GALLUSIDS(016, x)
#elif SIZEOF_PTHREAD_T == SIZEOF_INT
#define TIDFMT "0x" GALLUSIDS(08, x)
#endif /* SIZEOF_PTHREAD_T == SIZEOF_INT64_T ... */

              if ((*thdptr)->m_cpusetptr != NULL &&
                  (!(CPU_ISSET_S((unsigned)cpu, s_cpu_set_sz,
                                 (*thdptr)->m_cpusetptr)))) {
                const char *name =
                    (IS_VALID_STRING((*thdptr)->m_name) == true) ?
                    (*thdptr)->m_name : "???";
              

                gallus_msg_warning("Thread " TIDFMT " \"%s\" is running on "
                                    "CPU %d, but is not specified to run "
                                    "on it.\n",
                                    tid, name, cpu);
              }
                  
              ret = (gallus_result_t)cpu;

            } else {
              gallus_msg_error("Thread " TIDFMT " is running on unknown "
                                "CPU??\n", tid);
              ret = GALLUS_RESULT_POSIX_API_ERROR;
            }

#undef TIDFMT

          } else {
            errno = st;
            ret = GALLUS_RESULT_POSIX_API_ERROR;
          }

          CPU_FREE(cur_set);

        } else {	/* (ret = s_get_pthdid(thdptr, &tid)) ... */

          if (ret == GALLUS_RESULT_NOT_STARTED) {
            if ((*thdptr)->m_cpusetptr != NULL) {
              int i;
              int cpu = -INT_MAX;

              for (i = 0; i < MAX_CPUS; i++) {
                if (CPU_ISSET_S((unsigned)i, s_cpu_set_sz,
                                (*thdptr)->m_cpusetptr)) {
                  cpu = (gallus_result_t)i;
                  break;
                }
              }

              if (cpu != -INT_MAX) {
                ret = (gallus_result_t)cpu;
              } else {
                ret = GALLUS_RESULT_NOT_DEFINED;
              }
            } else {
              ret = GALLUS_RESULT_NOT_DEFINED;
            }
          }

        }

      }
    done:
      s_wait_unlock(*thdptr);

    } else {
      ret = GALLUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
#else
  (void)thdptr;
  return 0;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
}


gallus_result_t
gallus_thread_set_result_code(const gallus_thread_t *thdptr,
                               gallus_result_t code) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {

      s_op_lock(*thdptr);
      {
        (*thdptr)->m_result_code = code;
        ret = GALLUS_RESULT_OK;
      }
      s_op_unlock(*thdptr);

    } else {
      ret = GALLUS_RESULT_INVALID_OBJECT;
    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_thread_get_result_code(const gallus_thread_t *thdptr,
                               gallus_result_t *codeptr,
                               gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      codeptr != NULL) {

    *codeptr = GALLUS_RESULT_ANY_FAILURES;

    if (s_is_thd(*thdptr) == true) {
      if ((ret = gallus_thread_wait(thdptr, nsec)) == GALLUS_RESULT_OK) {

        s_op_lock(*thdptr);
        {
          *codeptr = (*thdptr)->m_result_code;
          ret = GALLUS_RESULT_OK;
        }
        s_op_unlock(*thdptr);

      }
    } else {
      ret = GALLUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_thread_is_canceled(const gallus_thread_t *thdptr,
                           bool *retptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      retptr != NULL) {

    *retptr = false;

    if (s_is_thd(*thdptr) == true) {

      s_cancel_lock(*thdptr);
      {
        *retptr = (*thdptr)->m_is_canceled;
        ret = GALLUS_RESULT_OK;
      }
      s_cancel_unlock(*thdptr);

    } else {
      ret = GALLUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_thread_free_when_destroy(gallus_thread_t *thdptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {
      s_alloc_mark_thd(*thdptr);
      ret = GALLUS_RESULT_OK;
    } else {
      ret = GALLUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_thread_is_valid(const gallus_thread_t *thdptr,
                        bool *retptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      retptr != NULL) {

    if (s_is_thd(*thdptr) == true) {
      *retptr = true;
      ret = GALLUS_RESULT_OK;
    } else {
      *retptr = false;
      ret = GALLUS_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_thread_get_name(const gallus_thread_t *thdptr, char *buf, size_t n) {
  if (likely(thdptr != NULL && *thdptr != NULL && buf != NULL && n > 0 &&
             IS_VALID_STRING((*thdptr)->m_name) == true)) {

    s_op_lock(*thdptr);
    {

      (void)snprintf(buf, n, "%s", (*thdptr)->m_name);

    }
    s_op_unlock(*thdptr);
    
    return GALLUS_RESULT_OK;
  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}


gallus_result_t
gallus_thread_set_name(const gallus_thread_t *thdptr, const char *name) {
  if (likely(thdptr != NULL && *thdptr != NULL && 
             IS_VALID_STRING(name) == true)) {

    s_op_lock(*thdptr);
    {

      (void)snprintf((*thdptr)->m_name, sizeof((*thdptr)->m_name), "%s", name);
#ifdef HAVE_PTHREAD_SETNAME_NP
      if ((*thdptr)->m_pthd != GALLUS_INVALID_THREAD) {
        (void)pthread_setname_np((*thdptr)->m_pthd, (*thdptr)->m_name);
      }
#endif /* HAVE_PTHREAD_SETNAME_NP */

    }
    s_op_unlock(*thdptr);

    return GALLUS_RESULT_OK;
  } else {
    return GALLUS_RESULT_INVALID_ARGS;
  }
}


void
gallus_thread_atfork_child(const gallus_thread_t *thdptr) {
  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {
      (void)gallus_mutex_reinitialize(&((*thdptr)->m_op_lock));
      (void)gallus_mutex_reinitialize(&((*thdptr)->m_wait_lock));
      (void)gallus_mutex_reinitialize(&((*thdptr)->m_cancel_lock));
      (void)gallus_mutex_reinitialize(&((*thdptr)->m_finalize_lock));
    }
  }
}


void
gallus_thread_module_initialize(void) {
  s_init();
}


void
gallus_thread_module_finalize(void) {
  s_final();
}
