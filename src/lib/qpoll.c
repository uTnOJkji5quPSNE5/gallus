#include "gallus_apis.h"
#include "qmuxer_types.h"
#include "qmuxer_internal.h"





static inline gallus_result_t
s_poll_initialize(gallus_qmuxer_poll_t mp,
                  gallus_bbq_t bbq,
                  gallus_qmuxer_poll_event_t type) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mp != NULL &&
      bbq != NULL &&
      IS_VALID_POLL_TYPE(type) == true) {
    (void)memset((void *)mp, 0, sizeof(*mp));
    mp->m_bbq = bbq;
    mp->m_type = type;
    mp->m_q_size = 0;
    mp->m_q_rem_capacity = 0;

    ret = GALLUS_RESULT_OK;

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_poll_destroy(gallus_qmuxer_poll_t mp) {
  free((void *)mp);
}





gallus_result_t
gallus_qmuxer_poll_create(gallus_qmuxer_poll_t *mpptr,
                           gallus_bbq_t bbq,
                           gallus_qmuxer_poll_event_t type) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL) {
    *mpptr = (gallus_qmuxer_poll_t)malloc(sizeof(**mpptr));
    if (*mpptr == NULL) {
      ret = GALLUS_RESULT_NO_MEMORY;
      goto done;
    }
    ret = s_poll_initialize(*mpptr, bbq, type);
    if (ret != GALLUS_RESULT_OK) {
      s_poll_destroy(*mpptr);
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


void
gallus_qmuxer_poll_destroy(gallus_qmuxer_poll_t *mpptr) {
  if (mpptr != NULL) {
    s_poll_destroy(*mpptr);
  }
}


gallus_result_t
gallus_qmuxer_poll_reset(gallus_qmuxer_poll_t *mpptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    (*mpptr)->m_q_size = 0;
    (*mpptr)->m_q_rem_capacity = 0;

    ret = GALLUS_RESULT_OK;

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_qmuxer_poll_set_queue(gallus_qmuxer_poll_t *mpptr,
                              gallus_bbq_t bbq) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    (*mpptr)->m_bbq = bbq;
    if (bbq != NULL) {
      bool tmp = false;
      ret = gallus_bbq_is_operational(&bbq, &tmp);
      if (ret == GALLUS_RESULT_OK && tmp == true) {
        ret = GALLUS_RESULT_OK;
      } else {
        /*
         * rather GALLUS_RESULT_INVALID_ARGS ?
         */
        ret = GALLUS_RESULT_NOT_OPERATIONAL;
      }
    } else {
      ret = GALLUS_RESULT_OK;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_qmuxer_poll_get_queue(gallus_qmuxer_poll_t *mpptr,
                              gallus_bbq_t *bbqptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL &&
      bbqptr != NULL) {
    *bbqptr = (*mpptr)->m_bbq;
    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_qmuxer_poll_set_type(gallus_qmuxer_poll_t *mpptr,
                             gallus_qmuxer_poll_event_t type) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL &&
      IS_VALID_POLL_TYPE(type) == true) {

    if ((*mpptr)->m_bbq != NULL) {
      (*mpptr)->m_type = type;
    } else {
      (*mpptr)->m_type = GALLUS_QMUXER_POLL_UNKNOWN;
    }
    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_qmuxer_poll_size(gallus_qmuxer_poll_t *mpptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    if ((*mpptr)->m_bbq != NULL) {
      ret = (*mpptr)->m_q_size;
    } else {
      ret = 0;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_qmuxer_poll_remaining_capacity(gallus_qmuxer_poll_t *mpptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    if ((*mpptr)->m_bbq != NULL) {
      ret = (*mpptr)->m_q_rem_capacity;
    } else {
      ret = 0;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}

