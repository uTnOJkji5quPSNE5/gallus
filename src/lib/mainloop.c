#include "gallus_apis.h"





#define ONE_SEC	1000LL * 1000LL * 1000LL

#define MAINLOOP_IDLE_PROC_INTERVAL	ONE_SEC
#define MAINLOOP_SHUTDOWN_TIMEOUT	5LL * ONE_SEC

#define DEFAULT_PIDFILE_DIR	"/var/run/"

#if defined(GALLUS_OS_LINUX) && defined(HAVE_SYS_PRCTL_H)
#define USE_PRCTL
#endif /* defined(GALLUS_OS_LINUX) && defined(HAVE_SYS_PRCTL_H) */


#define PARENT_EXIT_WAIT_TIMEOUT	ONE_SEC





typedef gallus_result_t (*mainloop_proc_t)(
    int argc, const char * const argv[],
    gallus_mainloop_startup_hook_proc_t pre_hook,
    gallus_mainloop_startup_hook_proc_t post_hook,
    int ipcfd);


typedef struct {
  mainloop_proc_t m_proc;
  int m_argc;
  const char * const * m_argv;
  gallus_mainloop_startup_hook_proc_t m_pre_hook;
  gallus_mainloop_startup_hook_proc_t m_post_hook;
  int m_ipcfd;
} mainloop_args_t;





static shutdown_grace_level_t s_gl = SHUTDOWN_UNKNOWN;

static char s_pidfile_buf[PATH_MAX];
static const char *s_pidfile;
static volatile bool s_do_pidfile = false;

static volatile bool s_do_fork = false;
static volatile bool s_do_abort = false;

static size_t s_n_callout_workers = 1;

static gallus_chrono_t s_idle_proc_interval = 
    MAINLOOP_IDLE_PROC_INTERVAL;
static gallus_chrono_t s_shutdown_timeout = 
    MAINLOOP_SHUTDOWN_TIMEOUT;


static gallus_thread_t s_thd = NULL;
static mainloop_args_t s_args = { NULL };

static bool s_org_multi_proc_mode = false;

#ifdef USE_PRCTL

static gallus_mutex_t s_parent_sync_lck = NULL;
static gallus_cond_t s_parent_sync_cnd = NULL;
static volatile bool s_is_parent_exit = false;
static bool s_is_inited = false;
static pthread_once_t s_once = PTHREAD_ONCE_INIT;

static void s_ctors(void) __attr_constructor__(114);
static void s_dtors(void) __attr_destructor__(114);

static void
s_once_proc(void) {
  gallus_result_t r;

  if ((r = gallus_mutex_create(&s_parent_sync_lck)) != GALLUS_RESULT_OK) {
    gallus_perror(r);
    gallus_exit_fatal("can't initialize the parent sync mutex.\n");
  }

  if ((r = gallus_cond_create(&s_parent_sync_cnd)) != GALLUS_RESULT_OK) {
    gallus_perror(r);
    gallus_exit_fatal("can't initialize the parent sync cond.\n");
  }

  s_is_inited = true;
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
  if (s_parent_sync_cnd != NULL) {
    (void)gallus_cond_destroy(&s_parent_sync_cnd);
  }
  if (s_parent_sync_lck != NULL) {
    (void)gallus_mutex_destroy(&s_parent_sync_lck);
  }
}


static void
s_dtors(void) {
  if (s_is_inited == true) {
    if (gallus_module_is_unloading() &&
        gallus_module_is_finalized_cleanly()) {
      s_final();
    }
  }
}


static void
s_temp_USR1_handler(int sig) {
  if (sig == SIGUSR1) {
    (void)gallus_mutex_lock(&s_parent_sync_lck);
    {
      s_is_parent_exit = true;
      (void)gallus_cond_notify(&s_parent_sync_cnd, true);
    }
    (void)gallus_mutex_unlock(&s_parent_sync_lck);
  }
}


static inline gallus_result_t
s_wait_parent_exit(void) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  pid_t ppid = getppid();
  gallus_chrono_t stop;
  gallus_chrono_t now;

  WHAT_TIME_IS_IT_NOW_IN_NSEC(stop);
  stop += PARENT_EXIT_WAIT_TIMEOUT;	/* 1 sec. */

  gallus_msg_debug(5, "Waiting parent %d exit ...\n", ppid);

  (void)gallus_mutex_lock(&s_parent_sync_lck);
  {
    errno = 0;
    while (s_is_parent_exit == false) {
      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      if (now < stop) {
        errno = 0;
        if (kill(ppid, 0) == 0) {
          ret = gallus_cond_wait(&s_parent_sync_cnd, &s_parent_sync_lck,
                              100LL * 1000LL * 1000LL /* 100ms. */);
          continue;
        } else {
          if (errno == ESRCH) {
            ret = GALLUS_RESULT_OK;
          } else {
            ret = GALLUS_RESULT_POSIX_API_ERROR;
            gallus_perror(ret);
          }
          break;
        }
      }
    }
  }
  (void)gallus_mutex_unlock(&s_parent_sync_lck);

  gallus_msg_debug(5, "Waiting parent %d exit ... done: %s.\n",
                ppid, gallus_error_get_string(ret));

  (void)gallus_signal(SIGUSR1, SIG_DFL, NULL);

  return ret;
}


static inline gallus_result_t
s_prepare_wait_parent_exit(void) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  pid_t ppid = getppid();

  if (kill(ppid, 0) == 0) {
    (void)gallus_signal(SIGUSR1, s_temp_USR1_handler, NULL);
    if (prctl(PR_SET_PDEATHSIG, SIGUSR1) == 0) {
      ret = GALLUS_RESULT_OK;
    } else {
      ret = GALLUS_RESULT_NOT_FOUND;
      gallus_perror(ret);
    }
  } else {
    ret = GALLUS_RESULT_NOT_FOUND;
  }

  return ret;
}


#else


static inline gallus_result_t
s_wait_parent_exit(void) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  pid_t ppid = getppid();
  gallus_chrono_t stop;
  gallus_chrono_t now;

  WHAT_TIME_IS_IT_NOW_IN_NSEC(stop);
  stop += PARENT_EXIT_WAIT_TIMEOUT;	/* 1 sec. */

  while (s_is_parent_exit == false) {
    WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
    if (now < stop) {
        errno = 0;
        if (kill(ppid, 0) == 0) {
          continue;
        } else {
          if (errno == ESRCH) {
            ret = GALLUS_RESULT_OK;
          } else {
            ret = GALLUS_RESULT_POSIX_API_ERROR;
            gallus_perror(ret);
          }
          break;
        }
      }
    }
  }

  return ret;
}


static inline gallus_result_t
s_prepare_wait_parent_exit(void) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  pid_t ppid = getppid();

  if (kill(ppid, 0) == 0) {
    ret = GALLUS_RESULT_OK;
  } else {
    ret = GALLUS_RESULT_NOT_FOUND;
  }

  return ret;
}


#endif /* USE_PRCTL */





static inline pid_t
s_setsid(void) {
  pid_t ret = (pid_t)-1;

#if 0
  int fd = open("/dev/tty", O_RDONLY);

  if (fd >= 0) {
#ifdef TIOCNOTTY
    int one = 1;
    (void)ioctl(fd, TIOCNOTTY, &one);
#endif /* TIOCNOTTY */
    (void)close(fd);
  }
#endif

  errno = 0;
  if (unlikely((ret = setsid()) < 0)) {
    perror("setsid");
    errno = 0;
    if (likely(setpgid(0, 0) == 0)) {
      errno = 0;
      ret = getpgrp();
    } else {
      perror("getpgrp");
      ret = (pid_t)-1;
    }
  }
  
  return ret;
}


static inline void
s_daemonize(int exclude_fd) {
  int i;

  (void)s_setsid();

  for (i = 0; i < 1024; i++) {
    if (i != exclude_fd) {
      (void)close(i);
    }
  }

  i = open("/dev/zero", O_RDONLY);
  if (i > 0) {
    (void)dup2(0, i);
    (void)close(i);
  }

  i = open("/dev/null", O_WRONLY);
  if (i > 1) {
    (void)dup2(1, i);
    (void)close(i);
  }

  i = open("/dev/null", O_WRONLY);
  if (i > 2) {
    (void)dup2(2, i);
    (void)close(i);
  }
}


static inline void
s_gen_pidfile(void) {
  FILE *fd = NULL;

  if (IS_VALID_STRING(s_pidfile) == true) {
    snprintf(s_pidfile_buf, sizeof(s_pidfile_buf), "%s", s_pidfile);
  } else {
    snprintf(s_pidfile_buf, sizeof(s_pidfile_buf),
             DEFAULT_PIDFILE_DIR "%s.pid",
             gallus_get_command_name());
  }

  /* TODO: umask(2) */

  fd = fopen(s_pidfile_buf, "w");
  if (fd != NULL) {
    fprintf(fd, "%d\n", (int)getpid());
    fflush(fd);
    (void)fclose(fd);
  } else {
    gallus_perror(GALLUS_RESULT_POSIX_API_ERROR);
    gallus_msg_error("can't create a pidfile \"%s\".\n",
                      s_pidfile_buf);
  }
}


static inline void
s_del_pidfile(void) {
  if (IS_VALID_STRING(s_pidfile_buf) == true) {
    struct stat st;

    if (stat(s_pidfile_buf, &st) == 0) {
      if (S_ISREG(st.st_mode)) {
        (void)unlink(s_pidfile_buf);
      }
    }
  }
}





static void
s_term_handler(int sig) {
  gallus_result_t r = GALLUS_RESULT_ANY_FAILURES;
  global_state_t gs = GLOBAL_STATE_UNKNOWN;

  if ((r = global_state_get(&gs)) == GALLUS_RESULT_OK) {

    if ((int)gs == (int)GLOBAL_STATE_STARTED) {

      shutdown_grace_level_t l = SHUTDOWN_UNKNOWN;
      if (sig == SIGTERM || sig == SIGINT) {
        l = SHUTDOWN_GRACEFULLY;
      } else if (sig == SIGQUIT) {
        l = SHUTDOWN_RIGHT_NOW;
      }
      if (IS_VALID_SHUTDOWN(l) == true) {
        gallus_msg_info("About to request shutdown(%s)...\n",
                         (l == SHUTDOWN_RIGHT_NOW) ?
                         "RIGHT_NOW" : "GRACEFULLY");
        if ((r = global_state_request_shutdown(l)) == GALLUS_RESULT_OK) {
          gallus_msg_info("The shutdown request accepted.\n");
        } else {
          gallus_perror(r);
          gallus_msg_error("can't request shutdown.\n");
        }
      }

    } else if ((int)gs < (int)GLOBAL_STATE_STARTED) {

      if (sig == SIGTERM || sig == SIGINT || sig == SIGQUIT) {
        gallus_abort_before_mainloop();
      }

    } else {
      gallus_msg_debug(5, "The system is already shutting down.\n");
    }

  }

}


static gallus_result_t
s_mainloop_thd_main(const gallus_thread_t *tptr, void *arg) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  (void)tptr;

  if (likely(arg != NULL)) {
    mainloop_args_t *mlarg = (mainloop_args_t *)arg;
    if (likely(mlarg->m_proc != NULL &&
               mlarg->m_argc > 0 &&
               mlarg->m_argv != NULL &&
               IS_VALID_STRING(mlarg->m_argv[0]) == true)) {
      ret = mlarg->m_proc(mlarg->m_argc,
                          mlarg->m_argv,
                          mlarg->m_pre_hook,
                          mlarg->m_post_hook,
                          mlarg->m_ipcfd);
    } else {
      ret = GALLUS_RESULT_INVALID_ARGS;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline gallus_result_t
s_start_mainloop_thd(mainloop_proc_t proc, int argc, const char * const argv[],
                     gallus_mainloop_startup_hook_proc_t pre_hook,
                     gallus_mainloop_startup_hook_proc_t post_hook,
                     int ipcfd) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(proc != NULL &&
             argc > 0 &&
             argv != NULL &&
             IS_VALID_STRING(argv[0]) == true)) {

    s_args.m_proc = proc;
    s_args.m_argc = argc;
    s_args.m_argv = argv;
    s_args.m_pre_hook = pre_hook;
    s_args.m_post_hook = post_hook;
    s_args.m_ipcfd = ipcfd;

    ret = gallus_thread_create(&s_thd,
                                s_mainloop_thd_main,
                                NULL,
                                NULL,
                                "mainlooper",
                                (void *)&s_args);
    if (likely(ret == GALLUS_RESULT_OK)) {
      ret = gallus_thread_start(&s_thd, false);
      if (likely(ret == GALLUS_RESULT_OK)) {
        global_state_t s;
        shutdown_grace_level_t l;

        ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
        if (likely(ret == GALLUS_RESULT_OK)) {
          if (unlikely(s != GLOBAL_STATE_STARTED)) {
            ret = GALLUS_RESULT_INVALID_STATE;
          }
        }
      }
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_wait_mainloop_thd(void) {
  gallus_result_t r = 
      gallus_thread_wait(&s_thd, MAINLOOP_SHUTDOWN_TIMEOUT);
  if (likely(r == GALLUS_RESULT_OK)) {
    gallus_thread_destroy(&s_thd);
  } else if (r == GALLUS_RESULT_TIMEDOUT) {
    gallus_msg_warning("the mainloop thread doesn't stop. cancel it.\n");
    r = gallus_thread_cancel(&s_thd);
    if (likely(r == GALLUS_RESULT_OK)) {
      r = gallus_thread_wait(&s_thd, MAINLOOP_SHUTDOWN_TIMEOUT);
      if (likely(r == GALLUS_RESULT_OK)) {
        gallus_thread_destroy(&s_thd);
      } else if (r == GALLUS_RESULT_TIMEDOUT) {
        gallus_msg_error("the mainloop thread still exists.\n");
      }
    } else {
      gallus_perror(r);
      gallus_msg_error("can't cancel the mainloop thread.\n");
    }
  } else {
    gallus_perror(r);
    gallus_msg_error("failed to wait the mainloop thread stop.\n");
  }
}





static inline gallus_result_t
s_prologue(int argc, const char * const argv[],
           gallus_mainloop_startup_hook_proc_t pre_hook,
           gallus_mainloop_startup_hook_proc_t post_hook,
           int ipcfd) {
  int st;
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  gallus_msg_info("%s: Initializing all the modules.\n",
                   gallus_get_command_name());

  (void)global_state_set(GLOBAL_STATE_INITIALIZING);

  ret = gallus_module_initialize_all(argc, (const char * const *)argv);
  if (likely(ret == GALLUS_RESULT_OK)) {
    gallus_msg_info("%s: All the modules are initialized.\n",
                     gallus_get_command_name());                     

    if (pre_hook != NULL) {
      ret = pre_hook(argc, argv);
      if (unlikely(ret != GALLUS_RESULT_OK)) {
        gallus_perror(ret);
        gallus_msg_error("pre-startup hook failed.\n");
        goto done;
      }
    }

    gallus_msg_info("%s: Starting all the modules.\n",
                     gallus_get_command_name());
    (void)global_state_set(GLOBAL_STATE_STARTING);

    ret = gallus_module_start_all();
    if (likely(ret == GALLUS_RESULT_OK)) {

      gallus_msg_info("%s: All the modules are started and ready to go.\n",
                       gallus_get_command_name());

      (void)global_state_set(GLOBAL_STATE_STARTED);

      if (post_hook != NULL) {
        ret = post_hook(argc, argv);
        if (unlikely(ret != GALLUS_RESULT_OK)) {
          gallus_perror(ret);
          gallus_msg_error("post-startup hook failed.\n");
          goto done;
        }
      }
    }
  }

done:
  if (likely(ret == GALLUS_RESULT_OK)) {
    if (s_do_pidfile == true) {
      s_gen_pidfile();
    }
    st = 0;
  } else {
    st = 1;
  }
  if (s_do_fork == true && ipcfd >= 0) {
    (void)s_prepare_wait_parent_exit();
    (void)write(ipcfd, &st, sizeof(int));
    (void)close(ipcfd);
    ret = s_wait_parent_exit();
    if (ret == GALLUS_RESULT_OK) {
      (void)gallus_log_set_multi_process(s_org_multi_proc_mode);
    } else {
      gallus_msg_warning("Wainting exit of parent process failed.\n");
      ret = GALLUS_RESULT_OK;
    }
  }

  return ret;
}


static inline gallus_result_t
s_epilogue(shutdown_grace_level_t l, gallus_chrono_t to) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  (void)global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);

  gallus_msg_info("%s: About to shutdown all the modules...\n",
                   gallus_get_command_name());

  ret = gallus_module_shutdown_all(l);
  if (likely(ret == GALLUS_RESULT_OK)) {
    ret = gallus_module_wait_all(to);
    if (likely(ret == GALLUS_RESULT_OK)) {
      gallus_msg_info("%s: Shutdown all the modules succeeded.\n",
                       gallus_get_command_name());
    } else if (ret == GALLUS_RESULT_TIMEDOUT) {
   do_cancel:
      gallus_msg_warning("%s: Shutdown failed. Trying to stop al the module "
                          "forcibly...\n", gallus_get_command_name());
      ret = gallus_module_stop_all();
      if (likely(ret == GALLUS_RESULT_OK)) {
        ret = gallus_module_wait_all(to);
        if (likely(ret == GALLUS_RESULT_OK)) {
          gallus_msg_warning("%s: All the modules are stopped forcibly.\n",
                              gallus_get_command_name());
        } else {
          gallus_perror(ret);
          gallus_msg_error("%s: can't stop all the modules.\n",
                            gallus_get_command_name());
        }
      }
    }
  } else if (ret == GALLUS_RESULT_TIMEDOUT) {
    goto do_cancel;
  }

  gallus_module_finalize_all();

  if (s_do_pidfile == true) {
    s_del_pidfile();
  }

  return ret;
}





static inline gallus_result_t
s_default_idle_proc(void) {
  return global_state_wait_for_shutdown_request(&s_gl,
                                                s_idle_proc_interval);
}


static inline gallus_result_t
s_default_mainloop(int argc, const char * const argv[],
                   gallus_mainloop_startup_hook_proc_t pre_hook,
                   gallus_mainloop_startup_hook_proc_t post_hook,
                   int ipcfd) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(s_do_abort == false)) {
    ret = s_prologue(argc, argv, pre_hook, post_hook, ipcfd);
    if (likely(ret == GALLUS_RESULT_OK &&
               s_do_abort == false)) {

      gallus_msg_info("%s is go and entering the main loop.\n",
                       gallus_get_command_name());

      while ((ret = s_default_idle_proc()) == GALLUS_RESULT_TIMEDOUT) {
        gallus_msg_debug(10, "%s: Wainitg for the shutdown request...\n",
                          gallus_get_command_name());
      }
      if (likely(ret == GALLUS_RESULT_OK)) {
        (void)global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);
      }

      ret = s_epilogue(s_gl, s_shutdown_timeout);
    }
  }

  return ret;
}





static gallus_result_t
s_callout_idle_proc(void *arg) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;
  shutdown_grace_level_t *glptr = (shutdown_grace_level_t *)arg;
  shutdown_grace_level_t gl = SHUTDOWN_UNKNOWN;

  gallus_msg_debug(10, "Checking the shutdown request...\n");

  ret = global_state_wait_for_shutdown_request(&gl, 0LL);
  if (unlikely(ret == GALLUS_RESULT_OK)) {
    gallus_msg_debug(1, "Got the shutdown request.\n");

    if (glptr != NULL) {
      *glptr = gl;
    }
    /*
     * Got a shutdown request. Accept it anyway.
     */
    (void)global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);
    /*
     * And return <0 to stop the callout main loop.
     */
    ret = GALLUS_RESULT_ANY_FAILURES;
  } else {
    ret = GALLUS_RESULT_OK;
  }

  return ret;
}


static inline gallus_result_t
s_callout_mainloop(int argc, const char * const argv[],
                   gallus_mainloop_startup_hook_proc_t pre_hook,
                   gallus_mainloop_startup_hook_proc_t post_hook,
                   int ipcfd) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(s_do_abort == false)) {
    bool handler_inited = false;

    /*
     * Initialize the callout handler.
     */
    ret = gallus_callout_initialize_handler(s_n_callout_workers,
                                             s_callout_idle_proc,
                                             (void *)&s_gl,
                                             s_idle_proc_interval,
                                             NULL);
    if (likely(ret == GALLUS_RESULT_OK)) {
      handler_inited = true;
      if (s_do_abort == false) {
        ret = s_prologue(argc, argv, pre_hook, post_hook, ipcfd);
        if (likely(ret == GALLUS_RESULT_OK &&
                   s_do_abort == false)) {
          gallus_result_t r1 = GALLUS_RESULT_ANY_FAILURES;
          gallus_result_t r2 = GALLUS_RESULT_ANY_FAILURES;


          gallus_msg_info("%s is go and entering the main loop.\n",
                       gallus_get_command_name());

          /*
           * Start the callout handler main loop.
           */
          ret = gallus_callout_start_main_loop();

          /*
           * No matter the ret is, here we have the main loop stopped. To
           * make it sure that the loop stopping call the stop API in case.
           */
          r1 = gallus_callout_stop_main_loop();

          r2 = s_epilogue(s_gl, s_shutdown_timeout);

          if (ret == GALLUS_RESULT_OK) {
            ret = r1;
          }
          if (ret == GALLUS_RESULT_OK) {
            ret = r2;
          }
        }
      }
      
      if (handler_inited == true) {
        gallus_callout_finalize_handler();
      }
    }
  }

  return ret;
}





static inline void
s_set_failsafe_handler(int sig) {
  sighandler_t h = NULL;

  if (gallus_signal(sig, SIG_CUR, &h) == GALLUS_RESULT_OK) {
    if (h == NULL || h == SIG_DFL || h == SIG_IGN) {
      gallus_result_t r = gallus_signal(sig, s_term_handler, NULL);
      if (r == GALLUS_RESULT_OK) {
        gallus_msg_warning("The signal %d seems not to be properly handled. "
                            "Set a decent handler.\n", sig);
      } else {
        gallus_perror(r);
          gallus_msg_warning("Can't set failsefa signal handler to "
                              "signal %d.\n", sig);
      }
    }
  }
}
  

static inline gallus_result_t
s_do_mainloop(mainloop_proc_t mainloopproc,
              int argc, const char * const argv[],
              gallus_mainloop_startup_hook_proc_t pre_hook,
              gallus_mainloop_startup_hook_proc_t post_hook,
              bool do_fork, bool do_pidfile, bool do_thread) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (likely(mainloopproc != NULL)) {

    s_do_pidfile = do_pidfile;
    s_do_fork = do_fork;

    /*
     * failsafe.
     */
    if (do_fork == false) {
      s_set_failsafe_handler(SIGINT);
    }
    s_set_failsafe_handler(SIGTERM);
    s_set_failsafe_handler(SIGQUIT);

    if (do_fork == true) {
      int ipcfds[2];
      int st;

      errno = 0;
      st = pipe(ipcfds);
      if (likely(st == 0)) {
        pid_t pid;

        /*
         * Set logger to lock file for parent/child simultaneous
         * logging forcibly fo now, then reset it to original mode
         * after the parent exits.
         */
        (void)gallus_log_get_multi_process(&s_org_multi_proc_mode);
        (void)gallus_log_set_multi_process(true);
        
        errno = 0;
        pid = fork();
        if (pid > 0) {
          /*
           * Parent.
           */
          int child_st = -1;
          ssize_t ssz;

          (void)close(ipcfds[1]);

          errno = 0;
          ssz = read(ipcfds[0], &child_st, sizeof(int));
          if (likely(ssz == sizeof(int))) {
            ret = (child_st == 0) ?
                GALLUS_RESULT_OK : GALLUS_RESULT_ANY_FAILURES;
          } else {
            ret = GALLUS_RESULT_POSIX_API_ERROR;
          }
          (void)close(ipcfds[0]);
        } else if (pid == 0) {
          /*
           * Child.
           */
          (void)close(ipcfds[0]);
          s_daemonize(ipcfds[1]);
          if (gallus_log_get_destination(NULL) == GALLUS_LOG_EMIT_TO_FILE) {
            (void)gallus_log_reinitialize();
          }
          if (do_thread == false) {
            ret = mainloopproc(argc, argv, pre_hook, post_hook, ipcfds[1]);
          } else {
            ret = s_start_mainloop_thd(mainloopproc, argc, argv,
                                       pre_hook, post_hook, ipcfds[1]);
          }
        } else {
          ret = GALLUS_RESULT_POSIX_API_ERROR;
        }          

      } else {
        ret = GALLUS_RESULT_POSIX_API_ERROR;
      }

    } else {	/* do_fork == true */

      if (do_thread == false) {
        ret = mainloopproc(argc, argv, pre_hook, post_hook, -1);
      } else {
        ret = s_start_mainloop_thd(mainloopproc, argc, argv,
                                   pre_hook, post_hook, -1);
      }

    }

  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}





void
gallus_abort_before_mainloop(void) {
  s_do_abort = true;
  mbar();
}


bool
gallus_is_abort_before_mainloop(void) {
  return s_do_abort;
}


gallus_result_t
gallus_set_pidfile(const char *file) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(file) == true) {
    const char *newfile = strdup(file);
    if (IS_VALID_STRING(newfile) == true) {
      if (IS_VALID_STRING(s_pidfile) == true) {
        free((void *)s_pidfile);
      }
      s_pidfile = newfile;
      ret = GALLUS_RESULT_OK;
    } else {
      /*
       * Note: scan-build warns a potentialy memory leak here and yes
       * it is intended to be.
       */
      ret = GALLUS_RESULT_NO_MEMORY;
    }
  } else {
    ret = GALLUS_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
gallus_create_pidfile(void) {
  s_gen_pidfile();
}


void
gallus_remove_pidfile(void) {
  s_del_pidfile();
}


gallus_result_t
gallus_mainloop_set_callout_workers_number(size_t n) {
  gallus_result_t ret = GALLUS_RESULT_ANY_FAILURES;

  if (n > 4) {
    ret = GALLUS_RESULT_TOO_LARGE;
  } else {
    s_n_callout_workers = n;
    ret = GALLUS_RESULT_OK;
  }

  return ret;
}


gallus_result_t
gallus_mainloop_set_shutdown_check_interval(gallus_chrono_t nsec) {
  if (nsec < ONE_SEC) {
    return GALLUS_RESULT_TOO_SMALL;
  } else if (nsec >= (10LL * ONE_SEC)) {
    return GALLUS_RESULT_TOO_LARGE;
  }

  s_idle_proc_interval = nsec;
  return GALLUS_RESULT_OK;
}


gallus_result_t
gallus_mainloop_set_shutdown_timeout(gallus_chrono_t nsec) {
  if (nsec < ONE_SEC) {
    return GALLUS_RESULT_TOO_SMALL;
  } else if (nsec >= (30LL * ONE_SEC)) {
    return GALLUS_RESULT_TOO_LARGE;
  }

  s_shutdown_timeout = nsec;
  return GALLUS_RESULT_OK;
}
  




gallus_result_t
gallus_mainloop(int argc, const char * const argv[],
                 gallus_mainloop_startup_hook_proc_t pre_hook,
                 gallus_mainloop_startup_hook_proc_t post_hook,
                 bool do_fork, bool do_pidfile, bool do_thread) {
  return s_do_mainloop(s_default_mainloop, argc, argv,
                       pre_hook, post_hook,
                       do_fork, do_pidfile, do_thread);
}


gallus_result_t
gallus_mainloop_with_callout(int argc, const char * const argv[],
                              gallus_mainloop_startup_hook_proc_t pre_hook,
                              gallus_mainloop_startup_hook_proc_t post_hook,
                              bool do_fork, bool do_pidfile,
                              bool do_thread) {
  return s_do_mainloop(s_callout_mainloop, argc, argv,
                       pre_hook, post_hook,
                       do_fork, do_pidfile, do_thread);
}


void
gallus_mainloop_wait_thread(void) {
  if (s_thd != NULL) {
    s_wait_mainloop_thd();
  }
}


gallus_result_t
gallus_mainloop_prologue(int argc, const char * const argv[],
                          gallus_mainloop_startup_hook_proc_t pre_hook,
                          gallus_mainloop_startup_hook_proc_t post_hook) {
  return s_prologue(argc, argv, pre_hook, post_hook, -1);
}


gallus_result_t
gallus_mainloop_epilogue(shutdown_grace_level_t l, gallus_chrono_t to) {
  return s_epilogue(l, to);
}
