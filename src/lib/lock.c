#include "gallus_apis.h"
#include "gallus_config.h"





struct gallus_mutex_record {
  pthread_mutex_t m_mtx;
  pid_t m_creator_pid;
  gallus_mutex_type_t m_type;
};


struct gallus_rwlock_record {
  pthread_rwlock_t m_rwl;
  pid_t m_creator_pid;
};


struct gallus_cond_record {
  pthread_cond_t m_cond;
  pid_t m_creator_pid;
};


struct gallus_barrier_record {
  pthread_barrier_t m_barrier;
  pid_t m_creator_pid;
};


typedef int (*notify_proc_t)(pthread_cond_t *cnd);





static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static bool s_is_inited = false;

static notify_proc_t s_notify_single_proc = pthread_cond_signal;
static notify_proc_t s_notify_all_proc = pthread_cond_broadcast;

static pthread_mutexattr_t s_recur_attr;





static void s_ctors(void) __attr_constructor__(102);
static void s_dtors(void) __attr_destructor__(102);





static void
s_once_proc(void) {

#ifdef GALLUS_OS_LINUX
#define RECURSIVE_MUTEX_ATTR PTHREAD_MUTEX_RECURSIVE_NP
#else
#define RECURSIVE_MUTEX_ATTR PTHREAD_MUTEX_RECURSIVE
#endif /* GALLUS_OS_LINUX */

  (void)pthread_mutexattr_init(&s_recur_attr);
  if (pthread_mutexattr_settype(&s_recur_attr, RECURSIVE_MUTEX_ATTR) != 0) {
    gallus_exit_fatal("can't initialize a recursive mutex attribute.\n");
  }
#undef RECURSIVE_MUTEX_ATTR
  s_is_inited = true;
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  gallus_msg_debug(10, "The mutex/lock APIs are initialized.\n");
}


static inline void
s_final(void) {
  (void)pthread_mutexattr_destroy(&s_recur_attr);
}


static void
s_dtors(void) {
  if (s_is_inited == true) {
    if (gallus_module_is_unloading() &&
        gallus_module_is_finalized_cleanly()) {
      s_final();

      gallus_msg_debug(10, "The mutex/lock APIs are finalized.\n");
    } else {
      gallus_msg_debug(10, "The mutex/lock APIs is not finalized "
                    "because of module finalization problem.\n");
    }
  }
}






static inline gallus_result_t
s_create(gallus_mutex_t *mtxptr, gallus_mutex_type_t type) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_mutex_t mtx = NULL;

  if (mtxptr != NULL) {
    *mtxptr = NULL;
    mtx = (gallus_mutex_t)malloc(sizeof(*mtx));
    if (mtx != NULL) {
      int st;
      pthread_mutexattr_t *attr = NULL;

      switch (type) {
        case GALLUS_MUTEX_TYPE_DEFAULT: {
          break;
        }
        case GALLUS_MUTEX_TYPE_RECURSIVE: {
          attr = &s_recur_attr;
          break;
        }
        default: {
          gallus_exit_fatal("Invalid gallus mutex type (%d).\n",
                             (int)type);
          /* not reached. */
          break;
        }
      }

      errno = 0;
      if ((st = pthread_mutex_init(&(mtx->m_mtx), attr)) == 0) {
        mtx->m_type = type;
        mtx->m_creator_pid = getpid();
        *mtxptr = mtx;
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = GALLUS_RESULT_NO_MEMORY;
    }
    if (ret != GALLUS_RESULT_OK) {
      free((void *)mtx);
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





gallus_result_t
gallus_mutex_create(gallus_mutex_t *mtxptr) {
  return s_create(mtxptr, GALLUS_MUTEX_TYPE_DEFAULT);
}


gallus_result_t
gallus_mutex_create_recursive(gallus_mutex_t *mtxptr) {
  return s_create(mtxptr, GALLUS_MUTEX_TYPE_RECURSIVE);
}


void
gallus_mutex_destroy(gallus_mutex_t *mtxptr) {
  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    if ((*mtxptr)->m_creator_pid == getpid()) {
      (void)pthread_mutex_destroy(&((*mtxptr)->m_mtx));
    }
    free((void *)*mtxptr);
    *mtxptr = NULL;
  }
}


gallus_result_t
gallus_mutex_reinitialize(gallus_mutex_t *mtxptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    pthread_mutexattr_t *attr = NULL;

    switch ((*mtxptr)->m_type) {
      case GALLUS_MUTEX_TYPE_DEFAULT: {
        break;
      }
      case GALLUS_MUTEX_TYPE_RECURSIVE: {
        attr = &s_recur_attr;
        break;
      }
      default: {
        gallus_exit_fatal("Invalid gallus mutex type (%d).\n",
                           (*mtxptr)->m_type);
        /* not reached. */
        break;
      }
    }

    errno = 0;
    if ((st = pthread_mutex_init(&((*mtxptr)->m_mtx), attr)) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_mutex_get_type(gallus_mutex_t *mtxptr, gallus_mutex_type_t *tptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL && *mtxptr != NULL && tptr != NULL) {
    *tptr = (*mtxptr)->m_type;
    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_mutex_lock(gallus_mutex_t *mtxptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    if ((st = pthread_mutex_lock(&((*mtxptr)->m_mtx))) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_mutex_trylock(gallus_mutex_t *mtxptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;
    if ((st = pthread_mutex_trylock(&((*mtxptr)->m_mtx))) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      if (st == EBUSY) {
        ret = GALLUS_RESULT_BUSY;
      } else {
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

#ifdef HAVE_PTHREAD_MUTEX_TIMEDLOCK
gallus_result_t
gallus_mutex_timedlock(gallus_mutex_t *mtxptr,
                        gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;

    if (nsec < 0) {
      if ((st = pthread_mutex_lock(&((*mtxptr)->m_mtx))) == 0) {
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      struct timespec ts;
      gallus_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      now += nsec;
      NSEC_TO_TS(now, ts);

      if ((st = pthread_mutex_timedlock(&((*mtxptr)->m_mtx),
                                        &ts)) == 0) {
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        if (st == ETIMEDOUT) {
          ret = GALLUS_RESULT_TIMEDOUT;
        } else {
          ret = GALLUS_RESULT_POSIX_API_ERROR;
        }
      }
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
#endif /* HAVE_PTHREAD_MUTEX_TIMEDLOCK */


gallus_result_t
gallus_mutex_unlock(gallus_mutex_t *mtxptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL) {
    int st;

    /*
     * The caller must have this mutex locked.
     */
    if ((st = pthread_mutex_unlock(&((*mtxptr)->m_mtx))) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_mutex_enter_critical(gallus_mutex_t *mtxptr, int *ostateptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL && *mtxptr != NULL &&
      ostateptr != NULL) {
    int st;

    if ((st = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                     ostateptr)) == 0) {
      if ((st = pthread_mutex_lock(&((*mtxptr)->m_mtx))) == 0) {
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_mutex_leave_critical(gallus_mutex_t *mtxptr, int ostate) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL && *mtxptr != NULL &&
      (ostate == PTHREAD_CANCEL_ENABLE ||
       ostate == PTHREAD_CANCEL_DISABLE)) {
    int st;

    /*
     * The caller must have this mutex locked.
     */
    if ((st = pthread_mutex_unlock(&((*mtxptr)->m_mtx))) == 0) {
      if ((st = pthread_setcancelstate(ostate, NULL)) == 0) {
          ret = GALLUS_RESULT_OK;
          if (ostate == PTHREAD_CANCEL_ENABLE) {
            pthread_testcancel();
          }
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





gallus_result_t
gallus_rwlock_create(gallus_rwlock_t *rwlptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_rwlock_t rwl = NULL;

  if (rwlptr != NULL) {
    *rwlptr = NULL;
    rwl = (gallus_rwlock_t)malloc(sizeof(*rwl));
    if (rwl != NULL) {
      int st;
      errno = 0;
      if ((st = pthread_rwlock_init(&(rwl->m_rwl), NULL)) == 0) {
        rwl->m_creator_pid = getpid();
        *rwlptr = rwl;
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = GALLUS_RESULT_NO_MEMORY;
    }
    if (ret != GALLUS_RESULT_OK) {
      free((void *)rwl);
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
gallus_rwlock_destroy(gallus_rwlock_t *rwlptr) {
  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    if ((*rwlptr)->m_creator_pid == getpid()) {
      (void)pthread_rwlock_destroy(&((*rwlptr)->m_rwl));
    }
    free((void *)*rwlptr);
    *rwlptr = NULL;
  }
}


gallus_result_t
gallus_rwlock_reinitialize(gallus_rwlock_t *rwlptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;

    errno = 0;
    if ((st = pthread_rwlock_init(&((*rwlptr)->m_rwl), NULL)) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_rwlock_reader_lock(gallus_rwlock_t *rwlptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    if ((st = pthread_rwlock_rdlock(&((*rwlptr)->m_rwl))) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_rwlock_reader_trylock(gallus_rwlock_t *rwlptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    if ((st = pthread_rwlock_tryrdlock(&((*rwlptr)->m_rwl))) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      if (st == EBUSY) {
        ret = GALLUS_RESULT_BUSY;
      } else {
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_rwlock_reader_timedlock(gallus_rwlock_t *rwlptr,
                                gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;

    if (nsec < 0) {
      if ((st = pthread_rwlock_rdlock(&((*rwlptr)->m_rwl))) == 0) {
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      struct timespec ts;
      gallus_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      now += nsec;
      NSEC_TO_TS(now, ts);

      if ((st = pthread_rwlock_timedrdlock(&((*rwlptr)->m_rwl),
                                           &ts)) == 0) {
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        if (st == ETIMEDOUT) {
          ret = GALLUS_RESULT_TIMEDOUT;
        } else {
          ret = GALLUS_RESULT_POSIX_API_ERROR;
        }
      }
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_rwlock_writer_lock(gallus_rwlock_t *rwlptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    if ((st = pthread_rwlock_wrlock(&((*rwlptr)->m_rwl))) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_rwlock_writer_trylock(gallus_rwlock_t *rwlptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;
    if ((st = pthread_rwlock_trywrlock(&((*rwlptr)->m_rwl))) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      if (st == EBUSY) {
        ret = GALLUS_RESULT_BUSY;
      } else {
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_rwlock_writer_timedlock(gallus_rwlock_t *rwlptr,
                                gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;

    if (nsec < 0) {
      if ((st = pthread_rwlock_wrlock(&((*rwlptr)->m_rwl))) == 0) {
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      struct timespec ts;
      gallus_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      now += nsec;
      NSEC_TO_TS(now, ts);

      if ((st = pthread_rwlock_timedwrlock(&((*rwlptr)->m_rwl),
                                           &ts)) == 0) {
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        if (st == ETIMEDOUT) {
          ret = GALLUS_RESULT_TIMEDOUT;
        } else {
          ret = GALLUS_RESULT_POSIX_API_ERROR;
        }
      }
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_rwlock_unlock(gallus_rwlock_t *rwlptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL &&
      *rwlptr != NULL) {
    int st;

    /*
     * The caller must have this rwlock locked.
     */
    if ((st = pthread_rwlock_unlock(&((*rwlptr)->m_rwl))) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_rwlock_reader_enter_critical(gallus_rwlock_t *rwlptr,
                                     int *ostateptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL && *rwlptr != NULL &&
      ostateptr != NULL) {
    int st;

    if ((st = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                     ostateptr)) == 0) {
      if ((st = pthread_rwlock_rdlock(&((*rwlptr)->m_rwl))) == 0) {
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_rwlock_writer_enter_critical(gallus_rwlock_t *rwlptr,
                                     int *ostateptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL && *rwlptr != NULL &&
      ostateptr != NULL) {
    int st;

    if ((st = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                     ostateptr)) == 0) {
      if ((st = pthread_rwlock_wrlock(&((*rwlptr)->m_rwl))) == 0) {
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_rwlock_leave_critical(gallus_rwlock_t *rwlptr, int ostate) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (rwlptr != NULL && *rwlptr != NULL &&
      (ostate == PTHREAD_CANCEL_ENABLE ||
       ostate == PTHREAD_CANCEL_DISABLE)) {
    int st;

    /*
     * The caller must have this rwlock locked.
     */
    if ((st = pthread_rwlock_unlock(&((*rwlptr)->m_rwl))) == 0) {
      if ((st = pthread_setcancelstate(ostate, NULL)) == 0) {
        ret = GALLUS_RESULT_OK;
        if (ostate == PTHREAD_CANCEL_ENABLE) {
          pthread_testcancel();
        }
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





gallus_result_t
gallus_cond_create(gallus_cond_t *cndptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_cond_t cnd = NULL;

  if (cndptr != NULL) {
    *cndptr = NULL;
    cnd = (gallus_cond_t)malloc(sizeof(*cnd));
    if (cnd != NULL) {
      int st;
      errno = 0;
      if ((st = pthread_cond_init(&(cnd->m_cond), NULL)) == 0) {
        cnd->m_creator_pid = getpid();
        *cndptr = cnd;
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = GALLUS_RESULT_NO_MEMORY;
    }
    if (ret != GALLUS_RESULT_OK) {
      free((void *)cnd);
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
gallus_cond_destroy(gallus_cond_t *cndptr) {
  if (cndptr != NULL &&
      *cndptr != NULL) {
    if ((*cndptr)->m_creator_pid == getpid()) {
      (void)pthread_cond_destroy(&((*cndptr)->m_cond));
    }
    free((void *)*cndptr);
    *cndptr = NULL;
  }
}


gallus_result_t
gallus_cond_wait(gallus_cond_t *cndptr,
                  gallus_mutex_t *mtxptr,
                  gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mtxptr != NULL &&
      *mtxptr != NULL &&
      cndptr != NULL &&
      *cndptr != NULL) {
    /*
     * It's kinda risky but we allow to cond-wait with recursive lock.
     *
     * So this condition is not checked:
     * (*mtxptr)->m_type != GALLUS_MUTEX_TYPE_RECURSIVE)
     */
    int st;

    errno = 0;
    if (nsec < 0) {
      if ((st = pthread_cond_wait(&((*cndptr)->m_cond),
                                  &((*mtxptr)->m_mtx))) == 0) {
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      struct timespec ts;
      gallus_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      now += nsec;
      NSEC_TO_TS(now, ts);
    retry:
      errno = 0;
      if ((st = pthread_cond_timedwait(&((*cndptr)->m_cond),
                                       &((*mtxptr)->m_mtx),
                                       &ts)) == 0) {
        ret = GALLUS_RESULT_OK;
      } else {
        if (st == EINTR) {
          goto retry;
        } else if (st == ETIMEDOUT) {
          ret = GALLUS_RESULT_TIMEDOUT;
        } else {
          errno = st;
          ret = GALLUS_RESULT_POSIX_API_ERROR;
        }
      }
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_cond_notify(gallus_cond_t *cndptr,
                    bool for_all) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (cndptr != NULL &&
      *cndptr != NULL) {
    int st;

    /*
     * I know I don't need this but:
     */
    mbar();

    errno = 0;
    if ((st = ((for_all == true) ? s_notify_all_proc : s_notify_single_proc)(
                &((*cndptr)->m_cond))) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





gallus_result_t
gallus_barrier_create(gallus_barrier_t *bptr, size_t n) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_barrier_t b = NULL;

  if (bptr != NULL && n > 0) {
    *bptr = NULL;
    b = (gallus_barrier_t)malloc(sizeof(*b));
    if (b != NULL) {
      int st;
      errno = 0;
      if ((st = pthread_barrier_init(&(b->m_barrier), NULL,
                                     (unsigned)n)) == 0) {
        b->m_creator_pid = getpid();
        *bptr = b;
        ret = GALLUS_RESULT_OK;
      } else {
        errno = st;
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = GALLUS_RESULT_NO_MEMORY;
    }
    if (ret != GALLUS_RESULT_OK) {
      free((void *)b);
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
gallus_barrier_destroy(gallus_barrier_t *bptr) {
  if (bptr != NULL &&
      *bptr != NULL) {
    if ((*bptr)->m_creator_pid == getpid()) {
      (void)pthread_barrier_destroy(&((*bptr)->m_barrier));
    }
    free((void *)*bptr);
    *bptr = NULL;
  }
}


gallus_result_t
gallus_barrier_wait(gallus_barrier_t *bptr, bool *is_master) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (bptr != NULL &&
      *bptr != NULL) {
    gallus_barrier_t b = *bptr;
    int st;

    errno = 0;
    st = pthread_barrier_wait(&(b->m_barrier));
    if (st == 0 || st == PTHREAD_BARRIER_SERIAL_THREAD) {
      ret = GALLUS_RESULT_OK;
      if (st == PTHREAD_BARRIER_SERIAL_THREAD && is_master != NULL) {
        *is_master = true;
      }
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


