#include "gallus_apis.h"

#include "gallus_poolable_internal.h"
#include "gallus_thread_internal.h"
#include "gallus_pool_internal.h"
#include "gallus_pooled_thread_internal.h"
#include "gallus_task_internal.h"





/*
 * Thread methods
 */


static gallus_result_t
s_task_runner_main(const gallus_thread_t *tptr, void *arg) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_task_runner_thread_t tr = NULL;
  gallus_poolable_t pobj = NULL;
  char org_thd_name[16];
  char *tmp_name = NULL;

  (void)arg;

  if (likely(tptr != NULL && *tptr != NULL &&
             (tr = (gallus_task_runner_thread_t)*tptr) != NULL &&
             (pobj = tr->m_pobj) != NULL)) {

    bool do_autodelete = false;
    bool do_autorelease = false;
    bool do_shutdown = false;
    bool do_name = false;
    gallus_task_t t = NULL;
    global_state_t gst = GLOBAL_STATE_UNKNOWN;
    shutdown_grace_level_t lvl = GLOBAL_STATE_UNKNOWN;
    
    /*
     * Wait for the gala opening.
     */
    ret = global_state_wait_for(GLOBAL_STATE_STARTED, &gst, &lvl, -1LL);
    if (ret != GALLUS_RESULT_OK || gst != GLOBAL_STATE_STARTED) {
      gallus_msg_error("failed to waiting the gala opening.\n");
      gallus_perror(ret);
      goto done;
    }

    while (true) {

      gallus_msg_debug(5, "task loop start.\n");

      do_autodelete = false;
      do_autorelease = false;
      do_shutdown = false;
      do_name = false;
      t = NULL;

      (void)gallus_mutex_lock(&tr->m_lck);
      {

        /*
         * Wait for a task assignment.
         */
        do {

          do_shutdown = tr->m_is_shutdown_requested;

          if (likely(tr->m_is_got_a_task == true &&
                     (t = tr->m_tsk) != NULL)) {
            ret = GALLUS_RESULT_OK;
            tr->m_state = GALLUS_TASK_RUNNER_STATE_ASSIGNED;
            break;
          } else if (do_shutdown == true) {
            break;
          } else {
            ret = gallus_cond_wait(&tr->m_cnd, &tr->m_lck, -1LL);
            if (likely(ret == GALLUS_RESULT_OK)) {
              continue;
            } else {
              break;
            }
          }

        } while (true);

      }
      (void)gallus_mutex_unlock(&tr->m_lck);

      if (likely(ret == GALLUS_RESULT_OK && t != NULL &&
                 tr->m_state == GALLUS_TASK_RUNNER_STATE_ASSIGNED &&
                 do_shutdown == false &&
                 pobj->m_is_used == true)) {

        (void)gallus_mutex_lock(&tr->m_lck);
        {

          tr->m_state = GALLUS_TASK_RUNNER_STATE_RUNNING;

          (void)gallus_mutex_lock(&t->m_lck);
          {

            t->m_is_started = true;
            t->m_state = GALLUS_TASK_STATE_RUNNING;
	    do_autodelete =
                ((t->m_flag & GALLUS_TASK_DELETE_CONTEXT_AFTER_EXEC) != 0) ?
                true : false;
            do_autorelease =
                ((t->m_flag & GALLUS_TASK_RELEASE_THREAD_AFTER_EXEC) != 0) ?
                true : false;
            do_name = (IS_VALID_STRING(t->m_name) == true) ? true : false;

          }
          (void)gallus_mutex_unlock(&t->m_lck);

        }
        (void)gallus_mutex_unlock(&tr->m_lck);

        /*
         * Execute the task.
         */
        if (do_name == true) {
          (void)gallus_thread_get_name(tptr, org_thd_name, sizeof(org_thd_name));
          (void)gallus_thread_set_name(tptr, t->m_name);
          tmp_name = t->m_name;
        } else {
          tmp_name = (char *)"???";
        }

        gallus_msg_debug(5, "task \"%s\" start...\n", tmp_name);
        ret = t->m_main(&t);
        gallus_msg_debug(5, "task \"%s\" done.\n", tmp_name);
        
        if (do_name == true) {
          (void)gallus_thread_set_name(tptr, org_thd_name);
        }

        /*
         * Execution done.
         */
        gallus_msg_debug(5, "update \"%s\" status, return code %ld.\n",
                      tmp_name, ret);
        (void)gallus_mutex_lock(&tr->m_lck);
        {

          tr->m_is_got_a_task = false;
          tr->m_tsk = NULL;
          tr->m_state = GALLUS_TASK_RUNNER_STATE_NOT_ASSIGNED;

          (void)gallus_mutex_lock(&t->m_lck);
          {

            t->m_exit_code = ret;
            t->m_is_clean_finished = true;
            t->m_state = GALLUS_TASK_STATE_CLEAN_FINISHED;

          }
          (void)gallus_mutex_unlock(&t->m_lck);

        }
        (void)gallus_mutex_unlock(&tr->m_lck);

        gallus_task_finalize(&t, false);

        if (do_autodelete == true) {
          gallus_task_destroy(&t);
        }
        if (do_autorelease == true) {
          gallus_pool_release_poolable(&pobj);
        }
        
      } else {
        if (do_shutdown == true) {

          if (t != NULL) {

            (void)gallus_mutex_lock(&t->m_lck);
            {

              t->m_exit_code = GALLUS_RESULT_NOT_STARTED;
              t->m_is_runner_shutdown = true;
              t->m_state = GALLUS_TASK_STATE_RUNNER_SHUTDOWN;

            }
            (void)gallus_mutex_unlock(&t->m_lck);

          }
          
          ret = GALLUS_RESULT_OK;

          break;
        } else if (pobj->m_is_used == false) {
          gallus_msg_fatal("poolable obj is used without acquisition??\n");
          ret = GALLUS_RESULT_INVALID_STATE_TRANSITION;
          break;
        }

      }

      /*
       * end of main loop.
       */
    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

done:

  gallus_msg_debug(5, "task runner thread main loop exited.\n");

  return ret;
}


static void
s_task_runner_finalize(const gallus_thread_t *tptr, bool is_cancelled,
                       void *arg) {
  gallus_poolable_t pobj = NULL;
  gallus_task_runner_thread_t tr = NULL;
  gallus_task_t t = NULL;
  
  (void)arg;

  gallus_msg_debug(5, "called, cancelled %s.\n",
                (is_cancelled == true) ? "yes" : "no");

  if (likely(tptr != NULL && *tptr != NULL &&
             (tr = (gallus_task_runner_thread_t)*tptr) != NULL &&
             (pobj = tr->m_pobj) != NULL)) {

    if (is_cancelled == true) {
      (void)gallus_mutex_unlock(&tr->m_lck);
    }

    if ((t = tr->m_tsk) != NULL) {

      if (is_cancelled == true) {
        (void)gallus_mutex_unlock(&t->m_lck);
      }

      (void)gallus_mutex_lock(&t->m_lck);
      {

        t->m_is_cancelled = is_cancelled;
        if (is_cancelled == true) {
          t->m_state = GALLUS_TASK_STATE_CANCELLED;
        } else {
          t->m_state = GALLUS_TASK_STATE_HALTED;
        }
        
      }
      (void)gallus_mutex_unlock(&t->m_lck);

      gallus_task_finalize(&t, is_cancelled);
    }

    /*
     * finally call the poolable finalizer. This triggers invokation
     * of this poolable obj's teardown method
     * s_pooled_thread_teardown().
     */
    pobj->m_last_result = gallus_poolable_teardown(&pobj, is_cancelled);

  }

}


static void
s_task_runner_freeup(const gallus_thread_t *tptr, void *arg) {
  gallus_task_runner_thread_t tr = NULL;

  (void)arg;

  gallus_msg_debug(5, "called.\n");

  if (tptr != NULL && (tr = (gallus_task_runner_thread_t)*tptr) != NULL) {
    if (tr->m_lck != NULL) {
      gallus_mutex_destroy(&tr->m_lck);
      tr->m_lck = NULL;
    }
    if (tr->m_cnd != NULL) {
      gallus_cond_destroy(&tr->m_cnd);
      tr->m_cnd = NULL;
    }
  }
}





/*
 * poolable methods
 */


static gallus_result_t
s_pooled_thread_construct(gallus_poolable_t *pptr, void *args) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_pooled_thread_t ptptr = NULL;
  gallus_task_runner_thread_t tr = NULL;
  bool is_thread_created = false;
  
  (void)args;

  if (likely(pptr != NULL && (ptptr = (gallus_pooled_thread_t)*pptr) != NULL &&
             (tr = &ptptr->m_trthd) != NULL)) {
    /* thread creation */
    ret = gallus_thread_create_with_size((gallus_thread_t *)&tr,
                                      0,
                                      s_task_runner_main,
                                      s_task_runner_finalize,
                                      s_task_runner_freeup,
                                      "pooled_thread",
                                      NULL);
    if (likely(ret == GALLUS_RESULT_OK)) {

      is_thread_created = true;

      tr->m_pobj = *pptr;
      if (unlikely((ret = gallus_mutex_create(&tr->m_lck)) != GALLUS_RESULT_OK)) {
        gallus_perror(ret);
        gallus_msg_error("can't initialize a mutex for a pooled thread.\n");
        goto done;
      }
      if (unlikely((ret = gallus_cond_create(&tr->m_cnd)) != GALLUS_RESULT_OK)) {
        gallus_perror(ret);
        gallus_msg_error("can't initialize a cond for a pooled thread.\n");
        goto done;
      }
      tr->m_state = GALLUS_TASK_RUNNER_STATE_UNKNOWN;
      tr->m_is_shutdown_requested = false;
      tr->m_is_got_a_task = false;
      tr->m_shutdown_lvl = SHUTDOWN_UNKNOWN;
      tr->m_is_finished = false;
      tr->m_is_cancelled = false;
      tr->m_tsk = NULL;
      tr->m_is_auto_delete = false;

      tr->m_state = GALLUS_TASK_RUNNER_STATE_CREATED;

    } else {
      gallus_perror(ret);
      gallus_msg_error("can't create a pooled thread.\n");
      goto done;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

done:
  if (unlikely(tr != NULL && ret != GALLUS_RESULT_OK &&
               is_thread_created == true)) {
    gallus_thread_destroy((gallus_thread_t *)&tr);
  }

  return ret;
}


static gallus_result_t
s_pooled_thread_setup(gallus_poolable_t *pptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_pooled_thread_t ptptr = NULL;
  gallus_task_runner_thread_t tr = NULL;

  if (likely(pptr != NULL && (ptptr = (gallus_pooled_thread_t)*pptr) != NULL &&
             (tr = &ptptr->m_trthd) != NULL &&
             tr->m_state == GALLUS_TASK_RUNNER_STATE_CREATED)) {
    ret = gallus_thread_start((gallus_thread_t *)&tr, false);
    if (likely(ret == GALLUS_RESULT_OK)) {
      tr->m_state = GALLUS_TASK_RUNNER_STATE_NOT_ASSIGNED;
    }
  } else {
    if (tr != NULL && tr->m_state != GALLUS_TASK_RUNNER_STATE_CREATED) {
      ret = GALLUS_RESULT_INVALID_OBJECT;
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  }

  return ret;
}


static gallus_result_t
s_pooled_thread_shutdown(gallus_poolable_t *pptr, shutdown_grace_level_t lvl) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_pooled_thread_t ptptr = NULL;
  gallus_task_runner_thread_t tr = NULL;

  gallus_msg_debug(5, "called.\n");

  if (likely(pptr != NULL && (ptptr = (gallus_pooled_thread_t)*pptr) != NULL &&
             (tr = &ptptr->m_trthd) != NULL)) {

    (void)gallus_mutex_lock(&tr->m_lck);
    {

      if (tr->m_state != GALLUS_TASK_RUNNER_STATE_CREATED &&
          tr->m_state != GALLUS_TASK_RUNNER_STATE_FINISH) {
        tr->m_is_shutdown_requested = true;
        tr->m_shutdown_lvl = lvl;

        ret = gallus_cond_notify(&tr->m_cnd, &tr->m_lck);
      } else {
        ret = GALLUS_RESULT_OK;
      }

    }
    (void)gallus_mutex_unlock(&tr->m_lck);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static gallus_result_t
s_pooled_thread_cancel(gallus_poolable_t *pptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_pooled_thread_t ptptr = NULL;
  gallus_task_runner_thread_t tr = NULL;

  if (likely(pptr != NULL && (ptptr = (gallus_pooled_thread_t)*pptr) != NULL &&
             (tr = &ptptr->m_trthd) != NULL)) {

    (void)gallus_mutex_lock(&tr->m_lck);
    {

      if (tr->m_state != GALLUS_TASK_RUNNER_STATE_CREATED &&
          tr->m_state != GALLUS_TASK_RUNNER_STATE_FINISH) {
        ret = gallus_thread_cancel((gallus_thread_t *)&tr);
      } else {
        ret = GALLUS_RESULT_OK;
      }

    }
    (void)gallus_mutex_unlock(&tr->m_lck);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static gallus_result_t
s_pooled_thread_teardown(gallus_poolable_t *pptr, bool is_cancelled) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_pooled_thread_t ptptr = NULL;
  gallus_task_runner_thread_t tr = NULL;

  gallus_msg_debug(5, "called, cancelled %s.\n",
                (is_cancelled == true) ? "yes" : "no");

  if (likely(pptr != NULL && (ptptr = (gallus_pooled_thread_t)*pptr) != NULL &&
             (tr = &ptptr->m_trthd) != NULL)) {
    if (is_cancelled == true) {
      gallus_mutex_unlock(&tr->m_lck);
    }

    ret = GALLUS_RESULT_OK;

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static gallus_result_t
s_pooled_thread_wait(gallus_poolable_t *pptr, gallus_chrono_t nsec) {
  (void)pptr;
  (void)nsec;

  gallus_msg_debug(5, "called.\n");

  return GALLUS_RESULT_OK;
}


static void
s_pooled_thread_destruct(gallus_poolable_t *pptr) {
  gallus_pooled_thread_t ptptr = NULL;
  gallus_task_runner_thread_t tr = NULL;

  if (likely(pptr != NULL && (ptptr = (gallus_pooled_thread_t)*pptr) != NULL &&
             (tr = &ptptr->m_trthd) != NULL)) {

    if (tr->m_lck != NULL) {
      gallus_mutex_destroy(&tr->m_lck);
      tr->m_lck = NULL;
    }
    if (tr->m_cnd != NULL) {
      gallus_cond_destroy(&tr->m_cnd);
      tr->m_cnd = NULL;
    }

    (void)gallus_thread_destroy((gallus_thread_t *)&tr);

  }
}





/*
 * pool wrapper
 */


static gallus_poolable_methods_record s_methods = {
  s_pooled_thread_construct,
  s_pooled_thread_setup,
  s_pooled_thread_shutdown,
  s_pooled_thread_cancel,
  s_pooled_thread_teardown,
  s_pooled_thread_wait,
  s_pooled_thread_destruct,
};


gallus_result_t
gallus_thread_pool_create(gallus_thread_pool_t *pptr, const char *name, size_t n) {
  return gallus_pool_create((gallus_pool_t *)pptr,
                         sizeof(gallus_thread_pool_record),
                         name,
                         GALLUS_POOL_TYPE_QUEUE,	/* queue type */
                         true,			/* executor */
                         n,
                         sizeof(gallus_pooled_thread_record),
			 &s_methods);
}


gallus_result_t
gallus_thread_pool_acquire_thread(gallus_thread_pool_t *pptr,
                               gallus_pooled_thread_t *ptptr,
                               gallus_chrono_t to) {
  return gallus_pool_acquire_poolable((gallus_pool_t *)pptr, to,
                                   (gallus_poolable_t *)ptptr);
}


gallus_result_t
gallus_thread_pool_release_thread(gallus_pooled_thread_t *ptptr) {
  return gallus_pool_release_poolable((gallus_poolable_t *)ptptr);
}


gallus_result_t
gallus_thread_pool_get_outstanding_thread_num(gallus_thread_pool_t *pptr) {
  return gallus_pool_get_outstanding_obj_num((gallus_pool_t *)pptr);
}


gallus_result_t
gallus_thread_pool_wakeup(gallus_thread_pool_t *pptr, gallus_chrono_t to) {
  return gallus_pool_wakeup((gallus_pool_t *)pptr, to);
}


gallus_result_t
gallus_thread_pool_shutdown_all(gallus_thread_pool_t *pptr,
			     shutdown_grace_level_t lvl,
			     gallus_chrono_t to) {
  return gallus_pool_shutdown((gallus_pool_t *)pptr, lvl, to);
}


gallus_result_t
gallus_thread_pool_cancel_all(gallus_thread_pool_t *pptr) {
  return gallus_pool_cancel((gallus_pool_t *)pptr);
}


gallus_result_t
gallus_thread_pool_wait_all(gallus_thread_pool_t *pptr, gallus_chrono_t to) {
  return gallus_pool_wait((gallus_pool_t *)pptr, to);
}


void
gallus_thread_pool_destroy(gallus_thread_pool_t *pptr) {
  gallus_pool_destroy((gallus_pool_t *)pptr);
}


gallus_result_t
gallus_thread_pool_get(const char *name, gallus_thread_pool_t *pptr) {
  return gallus_pool_get_pool(name, (gallus_pool_t *)pptr);  
}

