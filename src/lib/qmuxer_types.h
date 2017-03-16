#ifndef __QMUXER_TYPES_H__
#define __QMUXER_TYPES_H__





typedef struct gallus_qmuxer_poll_record {
  gallus_bbq_t m_bbq;
  gallus_qmuxer_poll_event_t m_type;
  ssize_t m_q_size;
  ssize_t m_q_rem_capacity;
} gallus_qmuxer_poll_record;


typedef struct gallus_qmuxer_record {
  gallus_mutex_t m_lock;
  gallus_cond_t m_cond;
} gallus_qmuxer_record;





#endif /* __QMUXER_TYPES_H__ */
