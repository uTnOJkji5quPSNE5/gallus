#ifndef __GALLUS_LOCK_H__
#define __GALLUS_LOCK_H__





/**
 *	@file	gallus_lock.h
 */





typedef enum {
  GALLUS_MUTEX_TYPE_UNKNOWN = 0,
  GALLUS_MUTEX_TYPE_DEFAULT,
  GALLUS_MUTEX_TYPE_RECURSIVE
} gallus_mutex_type_t;


typedef struct gallus_mutex_record 	*gallus_mutex_t;
typedef struct gallus_rwlock_record 	*gallus_rwlock_t;

typedef struct gallus_cond_record 	*gallus_cond_t;

typedef struct gallus_barrier_record 	*gallus_barrier_t;





__BEGIN_DECLS


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


__END_DECLS


#endif /* ! __GALLUS_LOCK_H__ */
