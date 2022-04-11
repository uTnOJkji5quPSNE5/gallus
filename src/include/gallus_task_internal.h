#pragma once





typedef struct gallus_task_record {
  gallus_task_state_t m_state;
  char *m_name;

  gallus_mutex_t m_lck;
  gallus_cond_t m_cnd;
  
  size_t m_total_obj_size;
  gallus_result_t m_exit_code;

  volatile bool m_is_started;
  volatile bool m_is_clean_finished;
  volatile bool m_is_runner_shutdown;
  volatile bool m_is_cancelled;

  volatile bool m_is_wait_done;

  gallus_task_main_proc_t m_main;
  gallus_task_finalize_proc_t m_finalize;
  gallus_task_freeup_proc_t m_freeup;

  int m_flag;

  gallus_thread_t m_tmp_thd;
} gallus_task_record;
