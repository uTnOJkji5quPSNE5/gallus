#include "gallus_apis.h"

#include "gallus_poolable_internal.h"
#include "gallus_thread_internal.h"
#include "gallus_pool_internal.h"
#include "gallus_pooled_thread_internal.h"
#include "gallus_task_internal.h"





/*
 * sz must include a size of *arg.
 */
gallus_result_t
gallus_task_create(gallus_task_t *tptr, size_t sz, const char *name,
                gallus_task_main_proc_t main_func,
                gallus_task_finalize_proc_t finalize_func,
		gallus_task_freeup_proc_t freeup_func) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  size_t alloc_sz = sz;
  gallus_task_t t = NULL;

  if (likely(tptr != NULL && main_func != NULL)) {
    if (*tptr == NULL) {
      if (alloc_sz <= 0) {
        alloc_sz = sizeof(gallus_task_record);
      } else if (alloc_sz < sizeof(gallus_task_record)) {
        ret = GALLUS_RESULT_TOO_SMALL;
        gallus_perror(ret);
        gallus_msg_error("task size is too small.\n");
        goto done;
      }
      *tptr = t = (gallus_task_t)malloc(alloc_sz);
      if (unlikely(*tptr == NULL)) {
        ret = GALLUS_RESULT_NO_MEMORY;
        gallus_perror(ret);
        gallus_msg_error("can't allocate a task.\n");
        goto done;
      }
    } else {
      t = *tptr;
    }

    t->m_state = GALLUS_TASK_STATE_UNKNOWN;
    if (IS_VALID_STRING(name) == true) {
      t->m_name = strdup(name);
      if (unlikely(t->m_name == NULL)) {
        ret = GALLUS_RESULT_NO_MEMORY;
        gallus_perror(ret);
        gallus_msg_error("can'r allocate a task name.\n");
        goto done;
      }
    } else {
      t->m_name = NULL;
    }
    if (unlikely((ret = gallus_mutex_create(&t->m_lck)) != GALLUS_RESULT_OK)) {
      gallus_perror(ret);
      gallus_msg_error("can't create a mutex for a task.\n");
      goto done;
    }
    if (unlikely((ret = gallus_cond_create(&t->m_cnd)) != GALLUS_RESULT_OK)) {
      gallus_perror(ret);
      gallus_msg_error("can't create a cond for a task.\n");
      goto done;
    }
    t->m_total_obj_size = alloc_sz;
    t->m_exit_code = GALLUS_RESULT_NOT_STARTED;
    t->m_is_started = false;
    t->m_is_clean_finished = false;
    t->m_is_runner_shutdown = false;
    t->m_is_cancelled = false;
    t->m_is_wait_done = false;
    t->m_main = main_func;
    t->m_finalize = finalize_func;
    t->m_freeup = freeup_func;
    t->m_flag = 0;
    t->m_tmp_thd = NULL;

    t->m_state = GALLUS_TASK_STATE_CONSTRUCTED;

    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

done:
  if (ret != GALLUS_RESULT_OK && t != NULL) {
    gallus_task_destroy(&t);
  }

  return ret;
}


gallus_result_t
gallus_task_run(gallus_task_t *tptr, gallus_pooled_thread_t *ptptr, int flag) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_task_t t = NULL;

  if (likely(tptr != NULL && (t = *tptr) != NULL && t->m_main != NULL)) {
    gallus_pooled_thread_t pt = NULL;
    gallus_poolable_t pobj = NULL;
    gallus_task_runner_thread_t tr = NULL;

    if (likely(ptptr != NULL && (pt = *ptptr) != NULL &&
               (pobj = (gallus_poolable_t)pt) != NULL &&
               pobj->m_is_used == true &&
               (tr = &pt->m_trthd) != NULL)) {

      (void)gallus_mutex_lock(&tr->m_lck);
      {

        (void)gallus_mutex_lock(&t->m_lck);
        {

          t->m_state = GALLUS_TASK_STATE_ATTACHED;
          t->m_flag = flag;

        }
        (void)gallus_mutex_unlock(&t->m_lck);

        tr->m_tsk = t;
        tr->m_is_got_a_task = true;

        /*
         * Signal task runner to exec this task.
         */
        ret = gallus_cond_notify(&tr->m_cnd, true);

      }
      (void)gallus_mutex_unlock(&tr->m_lck);

      if (likely(ret == GALLUS_RESULT_OK)) {
        if ((t->m_flag & GALLUS_TASK_DELETE_CONTEXT_AFTER_EXEC) != 0) {
          /*
           * no one can touch this task anymore.
           */
          *tptr = NULL;
        }
      }

    } else if (pt == NULL) {
      /*
       * TODO:
       *	Launch temp thread to run the task.
       */
      ret = GALLUS_RESULT_NOT_OPERATIONAL;
    } else {
      ret = GALLUS_RESULT_INVALID_OBJECT;
    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
gallus_task_finalize(gallus_task_t *tptr, bool is_cancelled) {
  gallus_task_t t = NULL;

  gallus_msg_debug(5, "called, cancel %s.\n",
                (is_cancelled == true) ? "yes" : "no");
  
  if (likely(tptr != NULL && (t = *tptr) != NULL)) {

    if (t->m_finalize != NULL) {
      t->m_finalize(tptr, is_cancelled);
    }

    if (unlikely(is_cancelled == true)) {
      (void)gallus_mutex_unlock(&t->m_lck);
    }

    gallus_msg_debug(5, "about to wake task \"%s\" waiters up...\n", t->m_name);

    (void)gallus_mutex_lock(&t->m_lck);
    {

      t->m_is_wait_done = true;

      (void)gallus_cond_notify(&t->m_cnd, true);

      gallus_msg_debug(5, "woke task \"%s\" waiters up.\n", t->m_name);
    }
    (void)gallus_mutex_unlock(&t->m_lck);
  }
}


gallus_result_t
gallus_task_wait(gallus_task_t *tptr, gallus_chrono_t to) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_task_t t = NULL;

  if (likely(tptr != NULL && (t = *tptr) != NULL)) {

    gallus_msg_debug(5, "wait for task \"%s\" done...\n", t->m_name);

                  (void)gallus_mutex_lock(&t->m_lck);
    {

      do {
        if (likely(t->m_is_wait_done == true)) {
          ret = GALLUS_RESULT_OK;
          break;
        } else {
          ret = gallus_cond_wait(&t->m_cnd, &t->m_lck, to);
          if (unlikely(ret != GALLUS_RESULT_OK)) {
            break;
          }
        }
      } while (true);

    }
    (void)gallus_mutex_unlock(&t->m_lck);

    gallus_msg_debug(5, "task \"%s\" done, returns %ld.\n",
                  t->m_name, t->m_exit_code);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_task_get_exit_code(gallus_task_t *tptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_task_t t = NULL;

  if (likely(tptr != NULL && (t = *tptr) != NULL)) {

    (void)gallus_mutex_lock(&t->m_lck);
    {

      ret = t->m_exit_code;

    }
    (void)gallus_mutex_unlock(&t->m_lck);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_task_get_state(gallus_task_t *tptr, gallus_task_state_t *stptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_task_t t = NULL;

  if (likely(tptr != NULL && (t = *tptr) != NULL && stptr != NULL)) {

    (void)gallus_mutex_lock(&t->m_lck);
    {

      *stptr = t->m_state;

    }
    (void)gallus_mutex_unlock(&t->m_lck);

    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
gallus_task_destroy(gallus_task_t *tptr) {
  gallus_task_t t = NULL;
  if (tptr != NULL && (t = *tptr) != NULL &&
      t->m_state != GALLUS_TASK_STATE_RUNNING &&
      t->m_state != GALLUS_TASK_STATE_CANCELLED) {

    if (t->m_freeup != NULL) {
      t->m_freeup(tptr);
    }

    if (t->m_cnd != NULL) {

      if (t->m_lck != NULL) {
        (void)gallus_mutex_lock(&t->m_lck);
      }
      {

        gallus_cond_notify(&t->m_cnd, true);

      }
      if (t->m_lck != NULL) {
        (void)gallus_mutex_unlock(&t->m_lck);
      }
      gallus_cond_destroy(&t->m_cnd);

    }

    if (t->m_lck != NULL) {
      gallus_mutex_destroy(&t->m_lck);
    }

    free(t->m_name);

    if (t->m_total_obj_size != 0) {
      free(t);
      *tptr = NULL;
    }
  }
}


