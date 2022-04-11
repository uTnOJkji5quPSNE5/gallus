#pragma once





typedef struct gallus_task_runner_thread_record {
  gallus_thread_record m_thd;

  gallus_poolable_t m_pobj;

  gallus_mutex_t m_lck;
  gallus_cond_t m_cnd;
  volatile gallus_task_runner_state_t m_state;
  volatile bool m_is_shutdown_requested;
  volatile bool m_is_got_a_task;
  shutdown_grace_level_t m_shutdown_lvl;
  bool m_is_finished;
  bool m_is_cancelled;

  gallus_task_t m_tsk;
  bool m_is_auto_delete;
} gallus_task_runner_thread_record;


typedef struct gallus_pooled_thread_record {
  gallus_poolable_record m_poolable;
  gallus_task_runner_thread_record m_trthd;
} gallus_pooled_thread_record;


/*
 * wraping a gallus_pool_record into a gallus_thread_pool_record to get a
 * warning on purpose when implicit type conversion.
 */
typedef struct gallus_thread_pool_record {
  gallus_pool_record m_pool;
  void *m_dummy;
} gallus_thread_pool_record;
