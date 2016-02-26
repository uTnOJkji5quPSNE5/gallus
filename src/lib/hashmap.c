#include "gallus_apis.h"





#include "hash.h"
#include "hash.c"





typedef struct gallus_hashmap_record {
  gallus_hashmap_type_t m_type;
  gallus_rwlock_t m_lock;
  HashTable m_hashtable;
  gallus_hashmap_value_freeup_proc_t m_del_proc;
  ssize_t m_n_entries;
  bool m_is_operational;
} gallus_hashmap_record;





static inline void
s_read_lock(gallus_hashmap_t hm, int *ostateptr) {
  if (hm != NULL && ostateptr != NULL) {
    (void)gallus_rwlock_reader_enter_critical(&(hm->m_lock), ostateptr);
  }
}


static inline void
s_write_lock(gallus_hashmap_t hm, int *ostateptr) {
  if (hm != NULL && ostateptr != NULL) {
    (void)gallus_rwlock_writer_enter_critical(&(hm->m_lock), ostateptr);
  }
}


static inline void
s_unlock(gallus_hashmap_t hm, int ostate) {
  if (hm != NULL) {
    (void)gallus_rwlock_leave_critical(&(hm->m_lock), ostate);
  }
}


static inline bool
s_do_iterate(gallus_hashmap_t hm,
             gallus_hashmap_iteration_proc_t proc, void *arg) {
  bool ret = false;
  if (hm != NULL && proc != NULL) {
    HashSearch s;
    gallus_hashentry_t he;

    for (he = FirstHashEntry(&(hm->m_hashtable), &s);
         he != NULL;
         he = NextHashEntry(&s)) {
      if ((ret = proc(GetHashKey(&(hm->m_hashtable), he),
                      GetHashValue(he),
                      he,
                      arg)) == false) {
        break;
      }
    }
  }
  return ret;
}


static inline gallus_hashentry_t
s_find_entry(gallus_hashmap_t hm, void *key) {
  gallus_hashentry_t ret = NULL;

  if (hm != NULL) {
    ret = FindHashEntry(&(hm->m_hashtable), key);
  }

  return ret;
}


static inline gallus_hashentry_t
s_create_entry(gallus_hashmap_t hm, void *key) {
  gallus_hashentry_t ret = NULL;

  if (hm != NULL) {
    int is_new;
    ret = CreateHashEntry(&(hm->m_hashtable), key, &is_new);
  }

  return ret;
}


static bool
s_freeup_proc(void *key, void *val, gallus_hashentry_t he, void *arg) {
  bool ret = false;
  (void)key;
  (void)he;

  if (arg != NULL) {
    gallus_hashmap_t hm = (gallus_hashmap_t)arg;
    if (hm->m_del_proc != NULL) {
      if (val != NULL) {
        hm->m_del_proc(val);
      }
      ret = true;
    }
  }

  return ret;
}


static inline void
s_freeup_all_values(gallus_hashmap_t hm) {
  s_do_iterate(hm, s_freeup_proc, (void *)hm);
}


static inline void
s_clean(gallus_hashmap_t hm, bool free_values) {
  if (free_values == true) {
    s_freeup_all_values(hm);
  }
  DeleteHashTable(&(hm->m_hashtable));
  (void)memset(&(hm->m_hashtable), 0, sizeof(HashTable));
  hm->m_n_entries = 0;
}


static inline void
s_reinit(gallus_hashmap_t hm, bool free_values) {
  s_clean(hm, free_values);
  InitHashTable(&(hm->m_hashtable), (unsigned int)hm->m_type);
}





void
gallus_hashmap_set_value(gallus_hashentry_t he, void *val) {
  if (he != NULL) {
    SetHashValue(he, val);
  }
}


gallus_result_t
gallus_hashmap_create(gallus_hashmap_t *retptr,
                       gallus_hashmap_type_t t,
                       gallus_hashmap_value_freeup_proc_t proc) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_hashmap_t hm;

  if (retptr != NULL) {
    *retptr = NULL;
    hm = (gallus_hashmap_t)malloc(sizeof(*hm));
    if (hm != NULL) {
      if ((ret = gallus_rwlock_create(&(hm->m_lock))) ==
          GALLUS_RESULT_OK) {
        hm->m_type = t;
        InitHashTable(&(hm->m_hashtable), (unsigned int)t);
        hm->m_del_proc = proc;
        hm->m_n_entries = 0;
        hm->m_is_operational = true;
        *retptr = hm;
        ret = GALLUS_RESULT_OK;
      } else {
        free((void *)hm);
      }
    } else {
      ret = GALLUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
gallus_hashmap_shutdown(gallus_hashmap_t *hmptr, bool free_values) {
  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      if ((*hmptr)->m_is_operational == true) {
        (*hmptr)->m_is_operational = false;
        s_clean(*hmptr, free_values);
      }
    }
    s_unlock(*hmptr, cstate);

  }
}


void
gallus_hashmap_destroy(gallus_hashmap_t *hmptr, bool free_values) {
  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      if ((*hmptr)->m_is_operational == true) {
        (*hmptr)->m_is_operational = false;
        s_clean(*hmptr, free_values);
      }
    }
    s_unlock(*hmptr, cstate);

    gallus_rwlock_destroy(&((*hmptr)->m_lock));
    free((void *)*hmptr);
    *hmptr = NULL;
  }
}





static inline gallus_result_t
s_clear(gallus_hashmap_t *hmptr, bool free_values) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if ((*hmptr)->m_is_operational == true) {
    (*hmptr)->m_is_operational = false;
    s_reinit(*hmptr, free_values);
    (*hmptr)->m_is_operational = true;
    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


gallus_result_t
gallus_hashmap_clear(gallus_hashmap_t *hmptr, bool free_values) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      ret = s_clear(hmptr, free_values);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_hashmap_clear_no_lock(gallus_hashmap_t *hmptr, bool free_values) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {
    ret = s_clear(hmptr, free_values);
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline gallus_result_t
s_find(gallus_hashmap_t *hmptr,
       void *key, void **valptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  gallus_hashentry_t he;

  *valptr = NULL;

  if ((*hmptr)->m_is_operational == true) {
    if ((he = s_find_entry(*hmptr, key)) != NULL) {
      *valptr = GetHashValue(he);
      ret = GALLUS_RESULT_OK;
    } else {
      ret = GALLUS_RESULT_NOT_FOUND;
    }
  } else {
    ret = GALLUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


gallus_result_t
gallus_hashmap_find_no_lock(gallus_hashmap_t *hmptr,
                             void *key, void **valptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {

    ret = s_find(hmptr, key, valptr);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_hashmap_find(gallus_hashmap_t *hmptr, void *key, void **valptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {
    int cstate;

    s_read_lock(*hmptr, &cstate);
    {
      ret = s_find(hmptr, key, valptr);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline gallus_result_t
s_add(gallus_hashmap_t *hmptr,
      void *key, void **valptr,
      bool allow_overwrite) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  void *oldval = NULL;
  gallus_hashentry_t he;

  if ((*hmptr)->m_is_operational == true) {
    if ((he = s_find_entry(*hmptr, key)) != NULL) {
      oldval = GetHashValue(he);
      if (allow_overwrite == true) {
        SetHashValue(he, *valptr);
        ret = GALLUS_RESULT_OK;
      } else {
        ret = GALLUS_RESULT_ALREADY_EXISTS;
      }
    } else {
      he = s_create_entry(*hmptr, key);
      if (he != NULL) {
        SetHashValue(he, *valptr);
        (*hmptr)->m_n_entries++;
        ret = GALLUS_RESULT_OK;
      } else {
        ret = GALLUS_RESULT_NO_MEMORY;
      }
    }
    *valptr = oldval;
  } else {
    ret = GALLUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


gallus_result_t
gallus_hashmap_add(gallus_hashmap_t *hmptr,
                    void *key, void **valptr,
                    bool allow_overwrite) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      ret = s_add(hmptr, key, valptr, allow_overwrite);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_hashmap_add_no_lock(gallus_hashmap_t *hmptr,
                            void *key, void **valptr,
                            bool allow_overwrite) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {

    ret = s_add(hmptr, key, valptr, allow_overwrite);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline gallus_result_t
s_delete(gallus_hashmap_t *hmptr,
         void *key, void **valptr,
         bool free_value) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  void *val = NULL;
  gallus_hashentry_t he;

  if ((*hmptr)->m_is_operational == true) {
    if ((he = s_find_entry(*hmptr, key)) != NULL) {
      val = GetHashValue(he);
      if (val != NULL &&
          free_value == true &&
          (*hmptr)->m_del_proc != NULL) {
        (*hmptr)->m_del_proc(val);
      }
      DeleteHashEntry(he);
      (*hmptr)->m_n_entries--;
    }
    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_NOT_OPERATIONAL;
  }

  if (valptr != NULL) {
    *valptr = val;
  }

  return ret;
}


gallus_result_t
gallus_hashmap_delete(gallus_hashmap_t *hmptr,
                       void *key, void **valptr,
                       bool free_value) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      ret = s_delete(hmptr, key, valptr, free_value);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_hashmap_delete_no_lock(gallus_hashmap_t *hmptr,
                               void *key, void **valptr,
                               bool free_value) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {

    ret = s_delete(hmptr, key, valptr, free_value);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline gallus_result_t
s_iterate(gallus_hashmap_t *hmptr,
          gallus_hashmap_iteration_proc_t proc,
          void *arg) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if ((*hmptr)->m_is_operational == true) {
    if (s_do_iterate(*hmptr, proc, arg) == true) {
      ret = GALLUS_RESULT_OK;
    } else {
      ret = GALLUS_RESULT_ITERATION_HALTED;
    }
  } else {
    ret = GALLUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


gallus_result_t
gallus_hashmap_iterate(gallus_hashmap_t *hmptr,
                        gallus_hashmap_iteration_proc_t proc,
                        void *arg) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      proc != NULL) {
    int cstate;

    /*
     * The proc could modify hash values so we use write lock.
     */
    s_write_lock(*hmptr, &cstate);
    {
      ret = s_iterate(hmptr, proc, arg);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_hashmap_iterate_no_lock(gallus_hashmap_t *hmptr,
                                gallus_hashmap_iteration_proc_t proc,
                                void *arg) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      proc != NULL) {

    ret = s_iterate(hmptr, proc, arg);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





gallus_result_t
gallus_hashmap_size(gallus_hashmap_t *hmptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_read_lock(*hmptr, &cstate);
    {
      if ((*hmptr)->m_is_operational == true) {
        ret = (*hmptr)->m_n_entries;
      } else {
        ret = GALLUS_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_hashmap_size_no_lock(gallus_hashmap_t *hmptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {

    if ((*hmptr)->m_is_operational == true) {
      ret = (*hmptr)->m_n_entries;
    } else {
      ret = GALLUS_RESULT_NOT_OPERATIONAL;
    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


gallus_result_t
gallus_hashmap_statistics(gallus_hashmap_t *hmptr, const char **msgptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      msgptr != NULL) {
    int cstate;

    *msgptr = NULL;

    s_read_lock(*hmptr, &cstate);
    {
      if ((*hmptr)->m_is_operational == true) {
        *msgptr = (const char *)HashStats(&((*hmptr)->m_hashtable));
        if (*msgptr != NULL) {
          ret = GALLUS_RESULT_OK;
        } else {
          ret = GALLUS_RESULT_NO_MEMORY;
        }
      } else {
        ret = GALLUS_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
gallus_hashmap_atfork_child(gallus_hashmap_t *hmptr) {
  if (hmptr != NULL &&
      *hmptr != NULL) {
    gallus_rwlock_reinitialize(&((*hmptr)->m_lock));
  }
}
