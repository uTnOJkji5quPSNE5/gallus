typedef struct callout_stage_record {
  gallus_pipeline_stage_record m_stg;

  bool m_is_initialized;
  size_t m_n_workers;
  gallus_bbq_t *m_qs;
  uint64_t m_last_q;
} callout_stage_record;
typedef callout_stage_record *callout_stage_t;





static callout_stage_record s_cs;





static gallus_result_t
s_callout_worker_sched(const gallus_pipeline_stage_t *sptr,
                       void *evbuf,
                       size_t n_evs,
                       void *hint) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    callout_stage_t cs = (callout_stage_t)*sptr;
    size_t idx = (size_t)hint;
    gallus_callout_task_t *tasks = (gallus_callout_task_t *)evbuf;

    if (likely(idx < cs->m_n_workers)) {
      size_t n_puts = 0;
      size_t n_total_puts = 0;

      while (n_total_puts < n_evs) {
        ret = gallus_bbq_put_n(&(cs->m_qs[idx]),
                                (void **)(tasks + n_total_puts),
                                n_evs - n_total_puts,
                                gallus_callout_task_t,
                                -1LL, &n_puts);
        if (likely(ret >= 0)) {
          n_total_puts += (size_t)ret;
        } else {
          if (likely(ret != GALLUS_RESULT_WAKEUP_REQUESTED)) {
            break;
          } else {
            n_total_puts += n_puts;
          }
        }
      }

      if (likely(ret > 0)) {
        ret = (gallus_result_t)n_total_puts;
        gallus_msg_debug(4, "submit " PFSZ(u) " tasks.\n", ret);
      }
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }
          
  return ret;
}


static gallus_result_t
s_callout_worker_fetch(const gallus_pipeline_stage_t *sptr,
                       size_t idx,
                       void *evbuf,
                       size_t max_n_evs) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    callout_stage_t cs = (callout_stage_t)*sptr;
    size_t n_puts = 0;

    ret = gallus_bbq_get_n(&(cs->m_qs[idx]),
                            (void **)evbuf,
                            max_n_evs, 1,
                            gallus_callout_task_t,
                            1000LL * 1000LL * 1000LL, /* 1 sec. */
                            &n_puts);
    switch (ret) {
      case GALLUS_RESULT_WAKEUP_REQUESTED: {
        ret = (gallus_result_t)n_puts;
        break;
      }
      case GALLUS_RESULT_TIMEDOUT: {
        /*
         * Returning a negative value stops pipeline. So return zero.
         */
        ret = 0;
        break;
      }
      case GALLUS_RESULT_OK: {
        ret = 0;
        break;
      }
      default: {
        if (likely(ret > 0)) {
          gallus_msg_debug(4, "fetched " PFSZ(u) " tasks.\n", ret);
        } else {
          gallus_perror(ret);
          gallus_msg_warning("task fetch failed.\n");
        }
        break;
      }
    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static gallus_result_t
s_callout_worker_main(const gallus_pipeline_stage_t *sptr,
                      size_t idx, void *buf, size_t n) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  (void)idx;

  if (likely(sptr != NULL && *sptr != NULL)) {
    size_t i;
    gallus_callout_task_t t;
    gallus_callout_task_t *tasks = (gallus_callout_task_t *)buf;

    for (i = 0; i < n; i++) {
      t = tasks[i];
      (void)s_exec_task(t);
    }
    ret = (gallus_result_t)n;

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static void
s_callout_worker_final(const gallus_pipeline_stage_t *sptr,
                       bool is_cancelled) {
  if (likely(sptr != NULL)) {
    callout_stage_t cs = (callout_stage_t)*sptr;
    if (unlikely(is_cancelled == true)) {
      size_t i;
      for (i = 0; i < cs->m_n_workers; i++) {
        (void)gallus_bbq_cancel_janitor(&(cs->m_qs[i]));
      }
      gallus_pipeline_stage_cancel_janitor(sptr);
    }
  }
}


static void
s_callout_worker_free(const gallus_pipeline_stage_t *sptr) {
  if (likely(sptr != NULL && *sptr != NULL)) {
    callout_stage_t cs = (callout_stage_t)*sptr;

    if (likely(cs == &s_cs)) {
      size_t i;

      for (i = 0; i < cs->m_n_workers; i++) {
        gallus_bbq_destroy(&(cs->m_qs[i]), true);
      }

      free((void *)(cs->m_qs));
      cs->m_n_workers = 0;
      cs->m_qs = NULL;

    } else {
      gallus_exit_fatal("Too bad address for a callout stage.\n");
    }
  }
}





static inline gallus_result_t
s_create_callout_stage(size_t n_workers) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(n_workers > 0)) {
    size_t i;
    gallus_pipeline_stage_t s = (gallus_pipeline_stage_t)&s_cs;
    gallus_bbq_t *qs = (gallus_bbq_t *)malloc(sizeof(gallus_bbq_t) * 
                                                n_workers);

    if (likely(qs != NULL)) {
      (void)memset((void *)qs, 0,
                   sizeof(sizeof(gallus_bbq_t) * n_workers));

      for (i = 0; i < n_workers; i++) {
        ret = gallus_bbq_create(&(qs[i]), gallus_callout_task_t,
                                 CALLOUT_TASK_MAX, s_task_freeup);
        if (unlikely(ret != GALLUS_RESULT_OK)) {
          break;
        }
      }

      if (likely(ret == GALLUS_RESULT_OK)) {
        ret = gallus_pipeline_stage_create(
            &s, 0, "c.o.worker", n_workers,
            sizeof(gallus_callout_task_t), CALLOUT_TASK_MAX,
            NULL,			/* pre_pause */
            s_callout_worker_sched,	/* sched */
            NULL,			/* setup */
            s_callout_worker_fetch,	/* fetch */
            s_callout_worker_main,	/* main */
            NULL,			/* throw */
            NULL,			/* shutdown */
            s_callout_worker_final,	/* final */
            s_callout_worker_free	/* freeup */
                                            );
        if (likely(ret == GALLUS_RESULT_OK)) {
          s_cs.m_is_initialized = true;
          s_cs.m_n_workers = n_workers;
          s_cs.m_qs = qs;
        }
      }
    }

    if (unlikely(ret != GALLUS_RESULT_OK)) {
      for (i = 0; i < n_workers; i++) {
        gallus_bbq_destroy(&(qs[i]), false);
      }
      free((void *)qs);
    }

  }
  return ret;
}


static inline void
s_destroy_callout_stage(void) {
  if (likely(s_cs.m_is_initialized == true)) {
    gallus_pipeline_stage_t s = (gallus_pipeline_stage_t)&s_cs;

    /*
     * s_cs.m_qs are freed up via the s_callout_worker_free().
     */
    gallus_pipeline_stage_destroy(&s);
  }
}


static inline gallus_result_t
s_start_callout_stage(void) {
  gallus_pipeline_stage_t s = (gallus_pipeline_stage_t)&s_cs;

  return gallus_pipeline_stage_start(&s);
}


static gallus_result_t
s_submit_callout_stage(const gallus_callout_task_t * const tasks,
                       gallus_chrono_t start_time,
                       size_t n) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(s_cs.m_is_initialized == true)) {
    if (likely(tasks != NULL && n > 0)) {
      gallus_pipeline_stage_t s = (gallus_pipeline_stage_t)&s_cs;
      size_t i;
      gallus_callout_task_t t;

      for (i = 0; i < n; i++) {
        /*
         * Change the state of tasks to TASK_STATE_DEQUEUED and set
         * the last execution (start) time as now.
         */

        t = tasks[i];

        s_lock_task(t);
        {
          if (t->m_status == TASK_STATE_ENQUEUED ||
              t->m_status == TASK_STATE_CREATED) {
            (void)s_set_task_state_in_table(t, TASK_STATE_DEQUEUED);
            t->m_status = TASK_STATE_DEQUEUED;
          }
          t->m_last_abstime = start_time;
        }
        s_unlock_task(t);

      }

      if (s_cs.m_n_workers == 1) {
        ret = gallus_pipeline_stage_submit(&s, (void *)tasks, n, (void *)0);
      } else {
        size_t tries = 0;
#if defined(GALLUS_ARCH_64_BITS)
        size_t q;
#elif defined(GALLUS_ARCH_32_BITS)
        uint32_t q;
#endif /* GALLUS_ARCH_64_BITS */
        size_t n_remains;
        size_t n_put;
        size_t n_total = 0;
        size_t stride = n / s_cs.m_n_workers;
        gallus_result_t last_err;

        if (stride == 0) {
          stride = 1;
        }

        while (n_total < n && tries++ < n) {
          n_remains = n - n_total;
          n_put = (stride < n_remains) ? stride : n_remains;
          q = (uint32_t)(s_cs.m_last_q % s_cs.m_n_workers);

          last_err = ret = 
              gallus_pipeline_stage_submit(&s,
                                            (void *)(tasks + n_total),
                                            n_put,
                                            (void *)q);
          s_cs.m_last_q++;

          if (likely(ret >= 0)) {
            n_total += (size_t)ret;
            last_err = ret = GALLUS_RESULT_OK;
          }
        }

        ret = (gallus_result_t)n_total;
        if (unlikely(last_err < 0)) {
          ret = last_err;
          gallus_perror(ret);
          gallus_msg_error("Task submission error(s). Only " PFSZ(u) " tasks "
                            "are submitted for " PFSZ(u) " tasks.\n",
                            n_total, n);
        } else if (unlikely(n_total != n)) {
          gallus_msg_warning("can't submit some task, tried " PFSZ(u)
                              ", but " PFSZ(u) ".\n",
                              n, n_total);
        }
      }
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = GALLUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


static inline void
s_wakeup_callout_workers(void) {
  gallus_pipeline_stage_t s = (gallus_pipeline_stage_t)&s_cs;
  gallus_result_t r = GALLUS_RESULT_ANY_FAILURES;
  size_t i;

  for (i = 0; i < s->m_n_workers; i++) {
    r = gallus_bbq_wakeup(&(s_cs.m_qs[i]), 1000LL * 1000LL * 100LL);
    if (r != GALLUS_RESULT_OK && r != GALLUS_RESULT_NOT_OPERATIONAL) {
      gallus_perror(r);
      gallus_msg_warning("can't wake callout worker #" PFSZ(u) " up.\n", i);
    }
  }
}


static inline gallus_result_t
s_shutdown_callout_stage(shutdown_grace_level_t lvl) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_pipeline_stage_t s = (gallus_pipeline_stage_t)&s_cs;

  /*
   * Set the stage shutdown mode first.
   */
  ret = gallus_pipeline_stage_shutdown(&s, lvl);

  /*
   * Then wake all the worker that might waiting tasks queued up.
   */
  s_wakeup_callout_workers();

  return ret;
}


static inline gallus_result_t
s_cancel_callout_stage(void) {
  gallus_pipeline_stage_t s = (gallus_pipeline_stage_t)&s_cs;

  return gallus_pipeline_stage_cancel(&s);
}


static inline gallus_result_t
s_wait_callout_stage(gallus_chrono_t nsec) {
  gallus_pipeline_stage_t s = (gallus_pipeline_stage_t)&s_cs;

  return gallus_pipeline_stage_wait(&s, nsec);
}


static inline gallus_result_t
s_finish_callout_stage(gallus_chrono_t timeout) {
  gallus_result_t ret = 
      s_shutdown_callout_stage((timeout == 0) ?
                               SHUTDOWN_RIGHT_NOW : SHUTDOWN_GRACEFULLY);

  if (likely(ret == GALLUS_RESULT_OK)) {
    ret = s_wait_callout_stage(timeout + 1000LL * 1000LL * 1000LL);
    if (ret == GALLUS_RESULT_OK) {
      s_destroy_callout_stage();
      gallus_msg_debug(4, "the callout stage shutdown cleanly.\n");
    } else if (ret == GALLUS_RESULT_TIMEDOUT) {
      ret = s_cancel_callout_stage();
      if (ret == GALLUS_RESULT_OK) {
        ret = s_wait_callout_stage(timeout + 1000LL * 1000LL * 1000LL);
        if (ret == GALLUS_RESULT_OK) {
          s_destroy_callout_stage();
          gallus_msg_debug(4, "the callout stage is cancelled.\n");
        } else {
          gallus_perror(ret);
          gallus_msg_error("can't cancel/wait the callout stage.\n");
        }
      }
    } else {
      gallus_perror(ret);
      gallus_msg_error("can't wait the callout stage.\n");
    }
  } else {
    gallus_perror(ret);
    gallus_msg_error("can't shutdown the callout stage.\n");
  }

  return ret;
}

