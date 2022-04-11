#pragma once





typedef struct gallus_poolable_record {
  gallus_mutex_t m_lck;
  gallus_cond_t m_cnd;

  gallus_pool_t m_pool;
  bool m_is_executor;
  uint64_t m_obj_idx;

  volatile bool m_is_used;
  volatile bool m_is_cancelled;
  volatile bool m_is_torndown;
  volatile bool m_is_wait_done;

  size_t m_total_obj_size;

  gallus_poolable_state_t m_st;

  gallus_result_t m_last_result;

  shutdown_grace_level_t m_shutdown_lvl;

  gallus_poolable_construct_proc_t m_construct;
  gallus_poolable_setup_proc_t m_setup;
  gallus_poolable_shutdown_proc_t m_shutdown;
  gallus_poolable_cancel_proc_t m_cancel;
  gallus_poolable_teardown_proc_t m_teardown;
  gallus_poolable_wait_proc_t m_wait;
  gallus_poolable_destruct_proc_t m_destruct;

} gallus_poolable_record;
