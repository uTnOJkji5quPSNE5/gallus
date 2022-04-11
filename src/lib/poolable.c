#include "gallus_apis.h"
#include "gallus_poolable_internal.h"





gallus_result_t
gallus_poolable_create_with_size(gallus_poolable_t *pptr,
                              bool is_executor,
                              gallus_poolable_methods_t m,
                              void *carg, size_t sz) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  size_t alloc_sz = sz;
  gallus_poolable_t p = NULL;

  if (likely(pptr != NULL)) {
    if (*pptr == NULL) {
      if (alloc_sz < sizeof(gallus_poolable_record)) {
        alloc_sz = sizeof(gallus_poolable_record);
      }
      if (likely((p = (gallus_poolable_t)malloc(alloc_sz)) != NULL)) {
        *pptr = p;
      } else {
        ret = GALLUS_RESULT_NO_MEMORY;
        gallus_perror(ret);
        gallus_msg_error("can't allocate a poolable object.\n");
        goto done;
      }
    } else {
      p = *pptr;
    }

    if (unlikely((ret = gallus_mutex_create(&p->m_lck)) != GALLUS_RESULT_OK)) {
      gallus_perror(ret);
      gallus_msg_error("can't initialize a mutex for a poolable object.\n");
      goto done;
    }
    if (unlikely((ret = gallus_cond_create(&p->m_cnd)) != GALLUS_RESULT_OK)) {
      gallus_perror(ret);
      gallus_msg_error("can't initialize a cond for a poolable object.\n");
      goto done;
    }
    p->m_pool = NULL;
    p->m_is_executor = is_executor;
    p->m_obj_idx = 0;
    p->m_is_used = false;
    p->m_is_cancelled = false;
    p->m_is_torndown = false;
    p->m_is_wait_done = false;
    p->m_total_obj_size = alloc_sz;
    p->m_st = GALLUS_POOLABLE_STATE_UNKNOWN;
    p->m_last_result = GALLUS_RESULT_ANY_FAILURES;
    p->m_shutdown_lvl = SHUTDOWN_UNKNOWN;

#define set_method(mem)                                         \
    p->mem = ((m != NULL && m->mem != NULL) ? m->mem : NULL)

    set_method(m_construct);
    set_method(m_setup);
    set_method(m_shutdown);
    set_method(m_cancel);
    set_method(m_teardown);
    set_method(m_wait);
    set_method(m_destruct);
#undef set_method

    if (is_executor == true) {
      if (unlikely((p->m_construct == NULL && p->m_setup == NULL) ||
                   p->m_shutdown == NULL ||
                   p->m_cancel == NULL ||
                   p->m_teardown == NULL ||
                   p->m_wait == NULL ||
                   p->m_destruct == NULL)) {
        ret = GALLUS_RESULT_INVALID_ARGS;
        gallus_perror(ret);
        gallus_msg_error("insufficient methods to creating an executor type "
                      "poolable object.\n");
        goto done;
      }
    } else {
      if (unlikely((p->m_construct == NULL && p->m_setup == NULL) ||
                   p->m_destruct == NULL)) {
        ret = GALLUS_RESULT_INVALID_ARGS;
        gallus_perror(ret);
        gallus_msg_error("insufficient methods to creating a "
                      "poolable object.\n");
        goto done;
      }
    }

    if (p->m_construct != NULL) {
      ret = p->m_construct(pptr, carg);
    } else {
      ret = GALLUS_RESULT_OK;
    }

    if (likely(ret == GALLUS_RESULT_OK)) {
      p->m_st = GALLUS_POOLABLE_STATE_CONSTRUCTED;
    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

done:
  if (unlikely(ret != GALLUS_RESULT_OK)) {
    if (*pptr != NULL && alloc_sz != 0) {
      free(*pptr);
      *pptr = NULL;
    }
  }

  return ret;
}


gallus_result_t
gallus_poolable_setup(gallus_poolable_t *pptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(pptr != NULL && *pptr != NULL)) {

    (void)gallus_mutex_lock(&(*pptr)->m_lck);
    {

      if (likely((*pptr)->m_st == GALLUS_POOLABLE_STATE_CONSTRUCTED)) {
        if ((*pptr)->m_setup != NULL) {
          ret = (*pptr)->m_setup(pptr);
        } else {
          ret = GALLUS_RESULT_OK;
        }
        if (likely(ret == GALLUS_RESULT_OK)) {
          (*pptr)->m_st = GALLUS_POOLABLE_STATE_OPERATIONAL;
        }
      } else {
        ret = GALLUS_RESULT_INVALID_STATE_TRANSITION;
      }
      (*pptr)->m_last_result = ret;

    }
    (void)gallus_mutex_unlock(&(*pptr)->m_lck);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_poolable_shutdown(gallus_poolable_t *pptr, shutdown_grace_level_t lvl) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(pptr != NULL && *pptr != NULL)) {

    gallus_msg_debug(5, "called.\n");

    (void)gallus_mutex_lock(&(*pptr)->m_lck);
    {

      if ((*pptr)->m_st == GALLUS_POOLABLE_STATE_NOT_OPERATIONAL &&
	  (*pptr)->m_is_torndown == true) {
        ret = GALLUS_RESULT_OK;
      } else if ((*pptr)->m_st == GALLUS_POOLABLE_STATE_OPERATIONAL) {
        (*pptr)->m_shutdown_lvl = lvl;
        if ((*pptr)->m_shutdown != NULL) {
          ret = (*pptr)->m_shutdown(pptr, lvl);
        } else {
          ret = GALLUS_RESULT_OK;
        }

        if (likely(ret == GALLUS_RESULT_OK)) {
          (*pptr)->m_st = GALLUS_POOLABLE_STATE_SHUTDOWNING;
          if ((*pptr)->m_is_executor == false) {
            if ((*pptr)->m_teardown != NULL) {
              ret = (*pptr)->m_teardown(pptr, false);
            } else {
              ret = GALLUS_RESULT_OK;
            }
            if (likely(ret == GALLUS_RESULT_OK)) {
              (*pptr)->m_st = GALLUS_POOLABLE_STATE_NOT_OPERATIONAL;
            }
          }
        }
        
      } else {
        ret = GALLUS_RESULT_INVALID_STATE_TRANSITION;
      }
      (*pptr)->m_last_result = ret;

    }
    (void)gallus_mutex_unlock(&(*pptr)->m_lck);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_poolable_cancel(gallus_poolable_t *pptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(pptr != NULL && *pptr != NULL)) {

    (void)gallus_mutex_lock(&(*pptr)->m_lck);
    {

      if ((*pptr)->m_st == GALLUS_POOLABLE_STATE_NOT_OPERATIONAL &&
	  (*pptr)->m_is_torndown == true) {
        ret = GALLUS_RESULT_OK;
      } else if ((*pptr)->m_st == GALLUS_POOLABLE_STATE_OPERATIONAL ||
                 ((*pptr)->m_st == GALLUS_POOLABLE_STATE_SHUTDOWNING &&
                  (*pptr)->m_shutdown_lvl == SHUTDOWN_RIGHT_NOW)) {
        if ((*pptr)->m_cancel != NULL) {
          ret = (*pptr)->m_cancel(pptr);
        } else {
          ret = GALLUS_RESULT_OK;
        }
        if (likely(ret == GALLUS_RESULT_OK)) {
          (*pptr)->m_st = GALLUS_POOLABLE_STATE_CANCELLING;
          if ((*pptr)->m_is_executor == false) {
            (*pptr)->m_is_cancelled = true;
            if ((*pptr)->m_teardown != NULL) {
              ret = (*pptr)->m_teardown(pptr, true);
            } else {
              ret = GALLUS_RESULT_OK;
            }
            if (likely(ret == GALLUS_RESULT_OK)) {
              (*pptr)->m_st = GALLUS_POOLABLE_STATE_NOT_OPERATIONAL;
            }
          }
        }
      } else {
        ret = GALLUS_RESULT_INVALID_STATE_TRANSITION;
      }
      (*pptr)->m_last_result = ret;

    }
    (void)gallus_mutex_unlock(&(*pptr)->m_lck);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


/*
 * NOTE:
 *
 * When executor poolables are finalized by thread finalized
 * procedures, this poolable teardown proc must be called from
 * gallus_threads' final proc. In order to do this, every executor
 * poolable must have a reverse reference to its parent poolable
 * instance.
 */
gallus_result_t
gallus_poolable_teardown(gallus_poolable_t *pptr, bool is_cancelled) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  gallus_msg_debug(5, "called, cancelled %s.\n",
                (is_cancelled == true) ? "yes" : "no");

  if (likely(pptr != NULL && *pptr != NULL)) {

    bool do_call_teardown = false;
    bool is_torndown = false;

    if (is_cancelled == true) {
      (void)gallus_mutex_unlock(&(*pptr)->m_lck);
    }

    (void)gallus_mutex_lock(&(*pptr)->m_lck);
    {

      if ((*pptr)->m_is_torndown == true) {
        is_torndown = true;
        ret = GALLUS_RESULT_OK;
      } else {
        if ((*pptr)->m_teardown != NULL) {
          do_call_teardown = true;
        } else {
          is_torndown = true;          
          ret = GALLUS_RESULT_OK;
        }
      }

    }
    (void)gallus_mutex_unlock(&(*pptr)->m_lck);

    if (is_torndown != true) {
      if (do_call_teardown == true) {
        ret = (*pptr)->m_teardown(pptr, is_cancelled);
      }
      
      (void)gallus_mutex_lock(&(*pptr)->m_lck);
      {
    
        (*pptr)->m_last_result = ret;
        (*pptr)->m_is_cancelled = is_cancelled;
        (*pptr)->m_st = GALLUS_POOLABLE_STATE_NOT_OPERATIONAL;
        (*pptr)->m_is_torndown = true;
        (*pptr)->m_is_wait_done = true;

        ret = gallus_cond_notify(&(*pptr)->m_cnd, true);
    
      }
      (void)gallus_mutex_unlock(&(*pptr)->m_lck);

    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_poolable_wait(gallus_poolable_t *pptr, gallus_chrono_t nsec) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  gallus_msg_debug(5, "called.\n");
  
  if (likely(pptr != NULL && *pptr != NULL)) {

    bool do_call_wait = false;
    
    (void)gallus_mutex_lock(&(*pptr)->m_lck);
    {

      do {

        if (likely((*pptr)->m_is_torndown == true &&
                   (*pptr)->m_is_wait_done == true)) {
          ret = GALLUS_RESULT_OK;
          break;
        } else {
          ret = gallus_cond_wait(&(*pptr)->m_cnd, &(*pptr)->m_lck, nsec);
          if (likely(ret == GALLUS_RESULT_OK)) {
            if ((*pptr)->m_wait != NULL) {
              do_call_wait = true;
            }
          } else {
            break;
          }
        }

      } while (true);

    }
    (void)gallus_mutex_unlock(&(*pptr)->m_lck);

    if (do_call_wait == true) {
      ret = (*pptr)->m_wait(pptr, nsec);
    }
    
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
gallus_poolable_destroy(gallus_poolable_t *pptr) {
  if (likely(pptr != NULL && *pptr != NULL)) {
    gallus_result_t r1 = GALLUS_RESULT_ANY_FAILURES;
    gallus_result_t r2 = GALLUS_RESULT_ANY_FAILURES;

    gallus_msg_debug(5, "called.\n");

    r1 = gallus_poolable_shutdown(pptr, SHUTDOWN_RIGHT_NOW);
    r2 = gallus_poolable_wait(pptr, -1LL);

    if (r1 == GALLUS_RESULT_OK && r2 == GALLUS_RESULT_OK &&
        (*pptr)->m_is_cancelled == false) {
      if ((*pptr)->m_destruct != NULL) {
        (*pptr)->m_destruct(pptr);
      }
      (void)gallus_mutex_destroy(&(*pptr)->m_lck);
      (void)gallus_cond_destroy(&(*pptr)->m_cnd);
      if ((*pptr)->m_total_obj_size != 0) {
        free(*pptr);
        *pptr = NULL;
      }
    }
  }
}


gallus_result_t
gallus_poolable_is_operational(gallus_poolable_t *pptr, bool *bptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(pptr != NULL && *pptr != NULL && bptr != NULL)) {

    (void)gallus_mutex_lock(&(*pptr)->m_lck);
    {
      *bptr = ((*pptr)->m_st == GALLUS_POOLABLE_STATE_OPERATIONAL) ? true : false;
    }
    (void)gallus_mutex_unlock(&(*pptr)->m_lck);

    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}
  

gallus_result_t
gallus_poolable_get_status(gallus_poolable_t *pptr, gallus_poolable_state_t *st) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(pptr != NULL && *pptr != NULL && st != NULL)) {

    (void)gallus_mutex_lock(&(*pptr)->m_lck);
    {
      *st = (*pptr)->m_st;
    }
    (void)gallus_mutex_unlock(&(*pptr)->m_lck);

    ret = GALLUS_RESULT_OK;

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

