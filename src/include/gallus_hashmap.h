#ifndef __GALLUS_HASHMAP_H__
#define __GALLUS_HASHMAP_H__





/**
 * @file	gallus_hashmap.h
 */





#define GALLUS_HASHMAP_TYPE_STRING	0U
#define GALLUS_HASHMAP_TYPE_ONE_WORD \
  MACRO_CONSTANTIFY_UNSIGNED(SIZEOF_VOID_P)





__BEGIN_DECLS





typedef unsigned int gallus_hashmap_type_t;


typedef struct HashEntry 	*gallus_hashentry_t;
typedef struct gallus_hashmap_record 	*gallus_hashmap_t;


/**
 * @details The signature of hash map iteration functions. Returning
 * \b false stops the iteration.
 */
typedef bool
(*gallus_hashmap_iteration_proc_t)(void *key, void *val,
                                    gallus_hashentry_t he,
                                    void *arg);


/**
 * @details The signature of value free up functions called when
 * destroying hash maps.
 */
typedef void
(*gallus_hashmap_value_freeup_proc_t)(void *value);





/**
 * Set a value to an entry.
 *
 *	@param[in]	he	A hash entry.
 *	@param[in]	val	A value.
 *
 *	@details Using this function is recommended only in iteration
 *	functions invoked via the gallus_hashmap_iterate().
 */
void
gallus_hashmap_set_value(gallus_hashentry_t he,
                          void *val);


/**
 * Create a hash map.
 *
 *	@param[out]	retptr	A pointer to a hash map to be created.
 *	@param[in]	t	The type of key
 *	(\b GALLUS_HASHMAP_TYPE_STRING: string key,
 *	\b GALLUS_HASHMAP_TYPE_ONE_WORD: 64/32 bits unsigned integer key)
 *	@param[in]	proc	A value free up function (\b NULL allowed).
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details The \b proc is called when gallus_hashmap_destroy()
 *	is called, for each stored value in the hash map to free it
 *	up, if both the \b proc and the value are not \b NULL.
 *
 *	@details And note that you can use arbitrary key, by
 *	specifying \b t as an integer, in \b sizeof(int). And in this
 *	case, the key itself must be passed as a pointer for the key.
 *
 *	@details So if you want to use 64 bits key on 32 bits
 *	architecture, you must pass the \b t as
 *	\b (gallus_hashmap_type_t)(sizeof(int64_t) / sizeof(int))
 */
gallus_result_t
gallus_hashmap_create(gallus_hashmap_t *retptr,
                       gallus_hashmap_type_t t,
                       gallus_hashmap_value_freeup_proc_t proc);


/**
 * Shutdown a hash map.
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	free_values	If \b true, all the values
 *	remaining in the hash map are freed if the value free up
 *	function given by the calling of the gallus_hashmap_create()
 *	is not \b NULL.
 *
 *	@details Call this function for the graceful hash map deletion
 *	of multi-threaded applications. Call this before calling the
 *	gallus_hashmap_destroy() makes the hash map not operational
 *	anymore so that any other threads could notice that the hash
 *	map is not usable.
 */
void
gallus_hashmap_shutdown(gallus_hashmap_t *hmptr,
                         bool free_values);


/**
 * Destroy a hash map.
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	free_values	If \b true, all the values
 *	remaining in the hash map are freed if the value free up
 *	function given by the calling of the gallus_hashmap_create()
 *	is not \b NULL.
 *
 *	@details Calling this function would imply invocation of the
 *	gallus_hashmap_shutdown() if needed.
 */
void
gallus_hashmap_destroy(gallus_hashmap_t *hmptr,
                        bool free_values);


/**
 * Clear a hash map.
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	free_values	If \b true, all the values
 *	remaining in the hash map are freed if the value free up
 *	function given by the calling of the gallus_hashmap_create()
 *	is not \b NULL.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_hashmap_clear(gallus_hashmap_t *hmptr,
                      bool free_values);


/**
 * Clear a hash map (no lock).
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	free_values	If \b true, all the values
 *	remaining in the hash map are freed if the value free up
 *	function given by the calling of the gallus_hashmap_create()
 *	is not \b NULL.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_hashmap_clear_no_lock(gallus_hashmap_t *hmptr,
                              bool free_values);


/**
 * Find a value corresponding to a given key from a hash map.
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	key		A key.
 *	@param[out]	valptr		A pointer to a value to be found.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded, found the value.
 *	@retval GALLUS_RESULT_NOT_FOUND	Failed, the value not found.
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_hashmap_find(gallus_hashmap_t *hmptr,
                     void *key,
                     void **valptr);


/**
 * Find a value corresponding to a given key from a hash map (no lock).
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	key		A key.
 *	@param[out]	valptr		A pointer to a value to be found.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded, found the value.
 *	@retval GALLUS_RESULT_NOT_FOUND	Failed, the value not found.
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details This version of the find API doesn't take a reader lock.
 */
gallus_result_t
gallus_hashmap_find_no_lock(gallus_hashmap_t *hmptr,
                             void *key,
                             void **valptr);


/**
 * Add a key - value pair to a hash map.
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *	@param[in]	key	A key.
 *	@param[in,out]	valptr	A pointr to a value.
 *	@param[in]	allow_overwrite When the pair already exists; \b true:
 *	overwrite, \b false: the operation is canceled.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded, the value newly
 *	added.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_ALREADY_EXISTS	Failed, the key - value pair
 *	already exists.
 *	@retval GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details No matter what the \b allow_overwrite is, if a value
 *	for the \b key doesn't exist in the hash map and the function
 *	returns \b GALLUS_RESULT_OK, the \b *valptr is set to \b NULL
 *	to indicate that no entry confliction occurs while the
 *	addition. And in this case, it is not possible to distinguish
 *	the cases between; 1) the entry doesn't exist. 2) the entry
 *	does exists and the value is \b NULL. If it is needed to
 *	distinguish these cases (e.g.: to add the \b NULL to a hash
 *	map as a value is very nature of the application), call the
 *	gallus_hashmap_find() to check the existence of the entry
 *	before the addition.
 *
 *	@details And also no matter what the \b allow_overwrite is, if
 *	an entry exists for the \b key, the returned \b *valptr is the
 *	former value (including \b NULL). And also if the functions
 *	returns \b GALLUS_RESULT_OK (note that the \b allow_overwrite
 *	is \b true for this case), it could be needed to free the
 *	value up.
 *
 *	@details If the \b allow_overwrite is \b false and the
 *	functions returns \b GALLUS_RESULT_ALREADY_EXISTS, nothing is
 *	changed in the hash map and the \b *valptr is the current
 *	value.
 *
 *	@details <em> And, NOTE THAT the \b *valptr could be modified
 *	almost always. If it is needed to keep the value referred by
 *	the \b valptr, backup/copy it before calling this function or
 *	pass the copied pointer. </em>
 */
gallus_result_t
gallus_hashmap_add(gallus_hashmap_t *hmptr,
                    void *key,
                    void **valptr,
                    bool allow_overwrite);


/**
 * Add a key - value pair to a hash map (no lock).
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *	@param[in]	key	A key.
 *	@param[in,out]	valptr	A pointr to a value.
 *	@param[in]	allow_overwrite When the pair already exists; \b true:
 *	overwrite, \b false: the operation is canceled.
 *
 *	@retval GALLUS_RESULT_OK		Succeeded, the value newly
 *	added.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_ALREADY_EXISTS	Failed, the key - value pair
 *	already exists.
 *	@retval GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details No matter what the \b allow_overwrite is, if a value
 *	for the \b key doesn't exist in the hash map and the function
 *	returns \b GALLUS_RESULT_OK, the \b *valptr is set to \b NULL
 *	to indicate that no entry confliction occurs while the
 *	addition. And in this case, it is not possible to distinguish
 *	the cases between; 1) the entry doesn't exist. 2) the entry
 *	does exists and the value is \b NULL. If it is needed to
 *	distinguish these cases (e.g.: to add the \b NULL to a hash
 *	map as a value is very nature of the application), call the
 *	gallus_hashmap_find() to check the existence of the entry
 *	before the addition.
 *
 *	@details And also no matter what the \b allow_overwrite is, if
 *	an entry exists for the \b key, the returned \b *valptr is the
 *	former value (including \b NULL). And also if the functions
 *	returns \b GALLUS_RESULT_OK (note that the \b allow_overwrite
 *	is \b true for this case), it could be needed to free the
 *	value up.
 *
 *	@details If the \b allow_overwrite is \b false and the
 *	functions returns \b GALLUS_RESULT_ALREADY_EXISTS, nothing is
 *	changed in the hash map and the \b *valptr is the current
 *	value.
 *
 *	@details <em> And, NOTE THAT the \b *valptr could be modified
 *	almost always. If it is needed to keep the value referred by
 *	the \b valptr, backup/copy it before calling this function or
 *	pass the copied pointer. </em>
 *
 *	@details This version of the add API doesn't take a writer lock.
 */
gallus_result_t
gallus_hashmap_add_no_lock(gallus_hashmap_t *hmptr,
                            void *key,
                            void **valptr,
                            bool allow_overwrite);


/**
 * Delete a key - value pair specified by the key.
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	key		A key.
 *	@param[out]	valptr		A pointer to save the former value
 *	if it exists (\b NULL allowed).
 *	@param[in]	free_value	If \b true, the value corresponding
 *	to the key is freed up if the free up function given by the
 *	calling of the gallus_hashmap_create() is not \b NULL.
 *
 *	@retval GALLUS_RESULT_OK Succeeded, the pair deleted, or the
 *	pair doesn't exist.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b free_value is \b true, <em> the returned \b
 *	*valptr is \b NOT accessible EVEN it is not \b
 *	NULL. </em>
 */
gallus_result_t
gallus_hashmap_delete(gallus_hashmap_t *hmptr,
                       void *key,
                       void **valptr,
                       bool free_value);


/**
 * Delete a key - value pair specified by the key (no lock).
 *
 *	@param[in]	hmptr		A pointer to a hash map.
 *	@param[in]	key		A key.
 *	@param[out]	valptr		A pointer to save the former value
 *	if it exists (\b NULL allowed).
 *	@param[in]	free_value	If \b true, the value corresponding
 *	to the key is freed up if the free up function given by the
 *	calling of the gallus_hashmap_create() is not \b NULL.
 *
 *	@retval GALLUS_RESULT_OK Succeeded, the pair deleted, or the
 *	pair doesn't exist.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b free_value is \b true, <em> the returned \b
 *	*valptr is \b NOT accessible EVEN it is not \b
 *	NULL. </em>
 *
 *	@details This version of the delete API doesn't take a writer lock.
 */
gallus_result_t
gallus_hashmap_delete_no_lock(gallus_hashmap_t *hmptr,
                               void *key,
                               void **valptr,
                               bool free_value);


/**
 * Get a # of entries in a hash map.
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *
 *	@retval	>=0	A # of entries (a # of pairs) in the hash map.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_hashmap_size(gallus_hashmap_t *hmptr);


/**
 * Get a # of entries in a hash map (no lock).
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *
 *	@retval	>=0	A # of entries (a # of pairs) in the hash map.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 */
gallus_result_t
gallus_hashmap_size_no_lock(gallus_hashmap_t *hmptr);


/**
 * Apply a function to all entries in a hash map iteratively.
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *	@param[in]	proc	An iteration function.
 *	@param[in]	arg	An auxiliary argument for the \b proc
 *	(\b NULL allowed).
 *
 *	@details DO NOT add/delete any entries in any iteration
 *	functions. The modification of the values of entries by
 *	calling the gallus_hashmap_set_value() is allowed in the
 *	functions.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_ITERATION_HALTED The iteration was
 *	stopped since the \b proc returned \b false.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b proc returns false while the iteration, the
 *	iteration stops.
 */
gallus_result_t
gallus_hashmap_iterate(gallus_hashmap_t *hmptr,
                        gallus_hashmap_iteration_proc_t proc,
                        void *arg);


/**
 * Apply a function to all entries in a hash map iteratively (no lock).
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *	@param[in]	proc	An iteration function.
 *	@param[in]	arg	An auxiliary argument for the \b proc
 *	(\b NULL allowed).
 *
 *	@details DO NOT add/delete any entries in any iteration
 *	functions. The modification of the values of entries by
 *	calling the gallus_hashmap_set_value() is allowed in the
 *	functions.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_ITERATION_HALTED The iteration was
 *	stopped since the \b proc returned \b false.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b proc returns false while the iteration, the
 *	iteration stops.
 *
 *	@details This version of the iterate API doesn't take a writer lock.
 */
gallus_result_t
gallus_hashmap_iterate_no_lock(gallus_hashmap_t *hmptr,
                                gallus_hashmap_iteration_proc_t proc,
                                void *arg);


/**
 * Get statistics of a hash map.
 *
 *	@param[in]	hmptr	A pointer to a hash map.
 *	@param[out]	msgptr	A pointer to a string including statistics.
 *
 *	@retval	GALLUS_RESULT_OK		Succeeded.
 *	@retval GALLUS_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval GALLUS_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval GALLUS_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval GALLUS_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the returned \b *msgptr is not \b NULL, it must be
 *	freed up by \b free().
 */
gallus_result_t
gallus_hashmap_statistics(gallus_hashmap_t *hmptr,
                           const char **msgptr);


/**
 * Preparation for fork(2).
 *
 *	@details This function is provided for the pthread_atfork(3)'s
 *	last argument. If we have to, functions for the first and
 *	second arguments for the pthread_atfork(3) would be provided
 *	as well, in later.
 */
void
gallus_hashmap_atfork_child(gallus_hashmap_t *hmptr);





__END_DECLS





#endif /* ! __GALLUS_HASHMAP_H__ */
