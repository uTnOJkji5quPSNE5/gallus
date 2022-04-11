#pragma once





typedef enum {
  GALLUS_POOL_STATE_UNKNOWN = 0,
  GALLUS_POOL_STATE_CONSTRUCTED = 1,
  GALLUS_POOL_STATE_OPERATIONAL = 2,
  GALLUS_POOL_STATE_SHUTTINGDOWN = 3,
  GALLUS_POOL_STATE_NOT_OPERATIONAL = 4,
} gallus_pool_state_t;


typedef struct gallus_pool_record {
  gallus_pool_type_t m_type;
  bool m_is_executor;
  gallus_pool_state_t m_state;
  uint64_t m_total_obj_size;

  char *m_name;

  gallus_mutex_t m_lck;
  gallus_cond_t m_cnd;
  gallus_cond_t m_awakened_cnd;
  volatile bool m_is_awakened;
  volatile bool m_is_cancelled;
	
  size_t m_n_waiters;
  
  size_t m_obj_idx;

  gallus_poolable_methods_record m_m;
  size_t m_pobj_size;

  size_t m_n_max;
  size_t m_n_cur;
  size_t m_n_free;
	
  gallus_poolable_t *m_objs;	/* size: m_n_max */

  /* For GALLUS_POOL_TYPE_QUEUE */
  gallus_bbq_t m_free_q;

} gallus_pool_record;
