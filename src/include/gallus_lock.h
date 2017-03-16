#ifndef __GALLUS_LOCK_H__
#define __GALLUS_LOCK_H__





/**
 *	@file	gallus_lock.h
 */





__BEGIN_DECLS





typedef enum {
  GALLUS_MUTEX_TYPE_UNKNOWN = 0,
  GALLUS_MUTEX_TYPE_DEFAULT,
  GALLUS_MUTEX_TYPE_RECURSIVE
} gallus_mutex_type_t;


typedef struct gallus_mutex_record 	*gallus_mutex_t;
typedef struct gallus_rwlock_record 	*gallus_rwlock_t;

typedef struct gallus_cond_record 	*gallus_cond_t;

typedef struct gallus_barrier_record 	*gallus_barrier_t;





gallus_result_t
gallus_mutex_create(gallus_mutex_t *mtxptr);

gallus_result_t
gallus_mutex_create_recursive(gallus_mutex_t *mtxptr);

void
gallus_mutex_destroy(gallus_mutex_t *mtxptr);

gallus_result_t
gallus_mutex_reinitialize(gallus_mutex_t *mtxptr);

gallus_result_t
gallus_mutex_get_type(gallus_mutex_t *mtxptr, gallus_mutex_type_t *tptr);

gallus_result_t
gallus_mutex_lock(gallus_mutex_t *mtxptr);

gallus_result_t
gallus_mutex_trylock(gallus_mutex_t *mtxptr);

gallus_result_t
gallus_mutex_timedlock(gallus_mutex_t *mtxptr, gallus_chrono_t nsec);


gallus_result_t
gallus_mutex_unlock(gallus_mutex_t *mtxptr);


gallus_result_t
gallus_mutex_enter_critical(gallus_mutex_t *mtxptr, int *ostateptr);

gallus_result_t
gallus_mutex_leave_critical(gallus_mutex_t *mtxptr, int ostate);





gallus_result_t
gallus_rwlock_create(gallus_rwlock_t *rwlptr);

void
gallus_rwlock_destroy(gallus_rwlock_t *rwlptr);

gallus_result_t
gallus_rwlock_reinitialize(gallus_rwlock_t *rwlptr);


gallus_result_t
gallus_rwlock_reader_lock(gallus_rwlock_t *rwlptr);

gallus_result_t
gallus_rwlock_reader_trylock(gallus_rwlock_t *rwlptr);

gallus_result_t
gallus_rwlock_reader_timedlock(gallus_rwlock_t *rwlptr,
                                gallus_chrono_t nsec);

gallus_result_t
gallus_rwlock_writer_lock(gallus_rwlock_t *rwlptr);

gallus_result_t
gallus_rwlock_writer_trylock(gallus_rwlock_t *rwlptr);

gallus_result_t
gallus_rwlock_writer_timedlock(gallus_rwlock_t *rwlptr,
                                gallus_chrono_t nsec);

gallus_result_t
gallus_rwlock_unlock(gallus_rwlock_t *rwlptr);


gallus_result_t
gallus_rwlock_reader_enter_critical(gallus_rwlock_t *rwlptr, int *ostateptr);

gallus_result_t
gallus_rwlock_writer_enter_critical(gallus_rwlock_t *rwlptr, int *ostateptr);

gallus_result_t
gallus_rwlock_leave_critical(gallus_rwlock_t *rwlptr, int ostate);





gallus_result_t
gallus_cond_create(gallus_cond_t *cndptr);

void
gallus_cond_destroy(gallus_cond_t *cndptr);

gallus_result_t
gallus_cond_wait(gallus_cond_t *cndptr,
                  gallus_mutex_t *mtxptr,
                  gallus_chrono_t nsec);

gallus_result_t
gallus_cond_notify(gallus_cond_t *cndptr,
                    bool for_all);





gallus_result_t
gallus_barrier_create(gallus_barrier_t *bptr, size_t n);


void
gallus_barrier_destroy(gallus_barrier_t *bptr);


gallus_result_t
gallus_barrier_wait(gallus_barrier_t *bptr, bool *is_master);





typedef pthread_spinlock_t gallus_spinlock_t;


static inline gallus_result_t
gallus_spinlock_initialize(gallus_spinlock_t *l) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    int st;
    errno = 0;
    if (likely((st = pthread_spin_init(l, PTHREAD_PROCESS_PRIVATE)) == 0)) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
gallus_spinlock_lock(gallus_spinlock_t *l) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    int st;
    errno = 0;
    if (likely((st = pthread_spin_lock(l) == 0))) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
gallus_spinlock_trylock(gallus_spinlock_t *l) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    int st;
    errno = 0;
    if (likely((st = pthread_spin_trylock(l) == 0))) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
gallus_spinlock_unlock(gallus_spinlock_t *l) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    int st;
    errno = 0;
    if (likely((st = pthread_spin_unlock(l) == 0))) {
      ret = GALLUS_RESULT_OK;
    } else {
      errno = st;
      ret = GALLUS_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
gallus_spinlock_finalize(gallus_spinlock_t *l) {
  if (likely(l != NULL)) {
    (void)pthread_spin_destroy(l);
  }
}





__END_DECLS





#endif /* __GALLUS_LOCK_H__ */
