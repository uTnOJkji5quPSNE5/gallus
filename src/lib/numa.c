#include "gallus_apis.h"





static bool s_is_numa = false;


static void s_ctors(void) __attr_constructor__(105);
static void s_dtors(void) __attr_destructor__(105);





#ifdef HAVE_NUMA


#define DO_NUMA_EVNE_ONE_NODE





typedef void *	(*numa_alloc_proc_t)(size_t sz, int cpu);
typedef void	(*numa_free_proc_t)(void *p);





static pthread_once_t s_once = PTHREAD_ONCE_INIT;

static void	s_init_numa_thingies(void);
static void	s_final_numa_thingies(void);

static int64_t s_n_cpus;
static unsigned int s_min_numa_node = UINT_MAX;
static unsigned int s_max_numa_node = 0;

static unsigned int *s_numa_nodes;
static gallus_hashmap_t s_tbl;

static numa_alloc_proc_t s_alloc_proc = NULL;
static numa_free_proc_t s_free_proc = NULL;

static void *	s_numa_alloc(size_t sz, int cpu);
static void	s_numa_free(void *p);

#ifndef DO_NUMA_EVNE_ONE_NODE
static void *	s_uma_alloc(size_t sz, int cpu);
static void	s_uma_free(void *p);
#endif /* ! DO_NUMA_EVNE_ONE_NODE */





static void
s_once_proc(void) {
  s_init_numa_thingies();
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();
}


static inline void
s_final(void) {
  s_final_numa_thingies();
}


static void
s_dtors(void) {
  s_final();
}





static inline void
s_init_numa_thingies(void) {
  numa_set_strict(1);

  s_n_cpus = (int64_t)sysconf(_SC_NPROCESSORS_CONF);
  if (s_n_cpus > 0) {
    s_numa_nodes = (unsigned int *)malloc(
	    sizeof(unsigned int) * (size_t)s_n_cpus);
    if (s_numa_nodes != NULL) {
      int i;
      gallus_result_t r;

      (void)memset((void *)s_numa_nodes, 0,
		   sizeof(unsigned int) * (size_t)s_n_cpus);

      for (i = 0; i < (int)s_n_cpus; i++) {
        s_numa_nodes[i] = (unsigned int)numa_node_of_cpu(i);
        if (s_min_numa_node > s_numa_nodes[i]) {
          s_min_numa_node = s_numa_nodes[i];
        }
        if (s_max_numa_node < s_numa_nodes[i]) {
          s_max_numa_node = s_numa_nodes[i];
        }
      }

#ifndef DO_NUMA_EVNE_ONE_NODE
      if (s_max_numa_node > s_min_numa_node) {
        r = gallus_hashmap_create(&s_tbl,
                                   GALLUS_HASHMAP_TYPE_ONE_WORD, NULL);
        if (r == GALLUS_RESULT_OK) {
          s_is_numa = true;

          s_alloc_proc = s_numa_alloc;
          s_free_proc = s_numa_free;
          
          gallus_msg_debug(5, "The NUMA aware memory allocator is "
                            "initialized.\n");
        } else {
          gallus_perror(r);
          gallus_exit_fatal("can't initialize the "
                             "NUMA memory allocation table.\n");
        }
      } else {
        s_alloc_proc = s_uma_alloc;
        s_free_proc = s_uma_free;

        gallus_msg_debug(5, "There is only one NUMA node on this machine. "
                          "No NUMA aware memory allocation is enabled.\n");
      }
#else
      r = gallus_hashmap_create(&s_tbl,
                                 GALLUS_HASHMAP_TYPE_ONE_WORD, NULL);
      if (r == GALLUS_RESULT_OK) {
        s_is_numa = true;

        s_alloc_proc = s_numa_alloc;
        s_free_proc = s_numa_free;
          
        gallus_msg_debug(5, "The NUMA aware memory allocator is "
                          "initialized.\n");
      } else {
        gallus_perror(r);
        gallus_exit_fatal("can't initialize the "
                           "NUMA memory allocation table.\n");
      }
#endif /* ! DO_NUMA_EVNE_ONE_NODE */
    }
  }
}


static bool
s_free_all(void *key, void *val, gallus_hashentry_t he, void *arg) {
  void *addr = key;
  size_t sz = (size_t)val;

  (void)he;
  (void)arg;

  if (likely(addr != NULL && sz > 0)) {
    numa_free(addr, sz);
  }

  return true;
}


static inline void
s_final_numa_thingies(void) {
  if (s_numa_nodes != NULL) {
    if (gallus_module_is_unloading() &&
        gallus_module_is_finalized_cleanly()) {    
      free((void *)s_numa_nodes);

      if (s_tbl != NULL) {
        (void)gallus_hashmap_iterate(&s_tbl, s_free_all, NULL);
        (void)gallus_hashmap_destroy(&s_tbl, true);
      }

      gallus_msg_debug(5, "The NUMA aware memory allocator is finalized.\n");
    } else {
      gallus_msg_debug(5, "The NUMA aware memory allocator is not finalized"
                        "because of module finalization problem.\n");
    }
  }
}





static inline gallus_result_t
s_add_addr(void *addr, size_t sz) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(s_tbl != NULL)) {
    if (likely(addr != NULL)) {
      void *val;
#ifdef GALLUS_ARCH_32_BITS
      val = (void *)((uint32_t)sz);
#else
      val = (void *)sz;
#endif /* GALLUS_ARCH_32_BITS */

      ret = gallus_hashmap_add(&s_tbl, addr, &val, false);
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = GALLUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


static inline gallus_result_t
s_find_addr(void *addr, size_t *sptr) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(s_tbl != NULL)) {
    if (likely(addr != NULL && sptr != NULL)) {
      void *val;

      *sptr = 0;
      ret = gallus_hashmap_find(&s_tbl, addr, &val);
      if (likely(ret == GALLUS_RESULT_OK)) {
        *sptr = (size_t)val;
      }
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = GALLUS_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


static inline void
s_delete_addr(void *addr) {
  if (likely(addr != NULL)) {
    (void)gallus_hashmap_delete(&s_tbl, addr, NULL, true);
  }
}





static void *
s_numa_alloc(size_t sz, int cpu) {
  void *ret = NULL;

  if (likely(sz > 0)) {
    if (likely(cpu >= 0)) {
      if (likely(s_numa_nodes != NULL && s_n_cpus > 0)) {
        unsigned int node = s_numa_nodes[cpu];
        unsigned int allocd_node = UINT_MAX;
        struct bitmask *bmp;
        int r;
  
        bmp = numa_allocate_nodemask();
        numa_bitmask_setbit(bmp, node);

        errno = 0;
        r = (int)set_mempolicy(MPOL_BIND, bmp->maskp, bmp->size + 1);
        if (likely(r == 0)) {
          errno = 0;
          ret = numa_alloc_onnode(sz, (int)node);
          if (likely(ret != NULL)) {
            gallus_result_t rl;

            /*
             * We need this "first touch" even using the
             * numa_alloc_onnode().
             */
            (void)memset(ret, 0, sz);

            errno = 0;
            r = (int)get_mempolicy((int *)&allocd_node, NULL, 0, ret,
                                   MPOL_F_NODE|MPOL_F_ADDR);
            if (likely(r == 0)) {
              if (unlikely(node != allocd_node)) {
                /*
                 * The memory is not allocated on the node, but it is
                 * still usable. Just return it.
                 */
                gallus_msg_warning("can't allocate " PFSZ(u) " bytes memory "
                                    "for CPU %d (NUMA node %d).\n",
                                    sz, cpu, node);
              }
            } else {
              gallus_perror(GALLUS_RESULT_POSIX_API_ERROR);
              gallus_msg_error("get_mempolicy() returned %d.\n", r);
            }

            rl = s_add_addr(ret, sz);
            if (unlikely(rl != GALLUS_RESULT_OK)) {
              gallus_perror(rl);
              gallus_msg_error("can't register the allocated address.\n");
              numa_free(ret, sz);
              ret = NULL;
            }
          }

        } else {	/* r == 0 */
          gallus_perror(GALLUS_RESULT_POSIX_API_ERROR);
          gallus_msg_error("set_mempolicy() returned %d.\n", r);
        }

        numa_free_nodemask(bmp);
        set_mempolicy(MPOL_DEFAULT, NULL, 0);

      } else {	/* s_numa_nodes != NULL && s_n_cpus > 0 */
        /*
         * Not initialized or initialization failure.
         */
        gallus_msg_warning("The NUMA related information is not initialized. "
                            "Use malloc(3) instead.\n");
        ret = malloc(sz);
      }

    } else {	/* cpu >= 0 */
      /*
       * Use pure malloc(3).
       */
      ret = malloc(sz);
    }

  }

  return ret;
}


static void
s_numa_free(void *p) {
  if (likely(p != NULL)) {
    size_t sz = 0;
    gallus_result_t r = s_find_addr(p, &sz);

    if (likely(r == GALLUS_RESULT_OK)) {
      numa_free(p, sz);
      s_delete_addr(p);
    } else {
      free(p);
    }
  }
}





#ifndef DO_NUMA_EVNE_ONE_NODE

static void *
s_uma_alloc(size_t sz, int cpu) {
  (void)cpu;

  return malloc(sz);
}


static void
s_uma_free(void *p) {
  free(p);
}

#endif /* ! DO_NUMA_EVNE_ONE_NODE */





void *
gallus_malloc_on_cpu(size_t sz, int cpu) {
  if (likely(s_alloc_proc != NULL)) {
    return s_alloc_proc(sz, cpu);
  } else {
    gallus_exit_fatal("The NUMA aware allocator not initialized??\n");
  }
}


void
gallus_free_on_cpu(void *p) {
  if (likely(s_free_proc != NULL)) {
    return s_free_proc(p);
  } else {
    gallus_exit_fatal("The NUMA aware allocator not initialized??\n");
  }
}





#else





static void
s_ctors(void) {
  gallus_msg_debug(5, "The NUMA is not supported.\n");
}


static void
s_dtors(void) {
  gallus_msg_debug(5, "The NUMA is not supported.\n");
}





void *
gallus_malloc_on_cpu(size_t sz, int cpu) {
  (void)cpu;

  return malloc(sz);
}


void
gallus_free_on_cpu(void *p) {
  free(p);
}





#endif /* HAVE_NUMA */


bool
gallus_is_numa_enabled(void) {
  return s_is_numa;
}


