/*	$OpenBSD: svr4_syscallargs.h,v 1.34 2002/07/06 19:22:43 nordin Exp $	*/

/*
 * System call argument lists.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	OpenBSD: syscalls.master,v 1.32 2002/07/06 19:14:20 nordin Exp 
 */

#ifdef	syscallarg
#undef	syscallarg
#endif

#define	syscallarg(x)							\
	union {								\
		register_t pad;						\
		struct { x datum; } le;					\
		struct {						\
			int8_t pad[ (sizeof (register_t) < sizeof (x))	\
				? 0					\
				: sizeof (register_t) - sizeof (x)];	\
			x datum;					\
		} be;							\
	}

struct svr4_sys_open_args {
	syscallarg(char *) path;
	syscallarg(int) flags;
	syscallarg(int) mode;
};

struct svr4_sys_wait_args {
	syscallarg(int *) status;
};

struct svr4_sys_creat_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
};

struct svr4_sys_execv_args {
	syscallarg(char *) path;
	syscallarg(char **) argp;
};

struct svr4_sys_time_args {
	syscallarg(svr4_time_t *) t;
};

struct svr4_sys_mknod_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
	syscallarg(int) dev;
};

struct svr4_sys_break_args {
	syscallarg(caddr_t) nsize;
};

struct svr4_sys_stat_args {
	syscallarg(char *) path;
	syscallarg(struct svr4_stat *) ub;
};

struct svr4_sys_alarm_args {
	syscallarg(unsigned) sec;
};

struct svr4_sys_fstat_args {
	syscallarg(int) fd;
	syscallarg(struct svr4_stat *) sb;
};

struct svr4_sys_utime_args {
	syscallarg(char *) path;
	syscallarg(struct svr4_utimbuf *) ubuf;
};

struct svr4_sys_access_args {
	syscallarg(char *) path;
	syscallarg(int) flags;
};

struct svr4_sys_nice_args {
	syscallarg(int) prio;
};

struct svr4_sys_kill_args {
	syscallarg(int) pid;
	syscallarg(int) signum;
};

struct svr4_sys_pgrpsys_args {
	syscallarg(int) cmd;
	syscallarg(int) pid;
	syscallarg(int) pgid;
};

struct svr4_sys_times_args {
	syscallarg(struct tms *) tp;
};

struct svr4_sys_signal_args {
	syscallarg(int) signum;
	syscallarg(svr4_sig_t) handler;
};

struct svr4_sys_msgsys_args {
	syscallarg(int) what;
	syscallarg(int) a2;
	syscallarg(int) a3;
	syscallarg(int) a4;
	syscallarg(int) a5;
};

struct svr4_sys_sysarch_args {
	syscallarg(int) op;
	syscallarg(void *) a1;
};

struct svr4_sys_shmsys_args {
	syscallarg(int) what;
	syscallarg(int) a2;
	syscallarg(int) a3;
	syscallarg(int) a4;
};

struct svr4_sys_semsys_args {
	syscallarg(int) what;
	syscallarg(int) a2;
	syscallarg(int) a3;
	syscallarg(int) a4;
	syscallarg(int) a5;
};

struct svr4_sys_ioctl_args {
	syscallarg(int) fd;
	syscallarg(u_long) com;
	syscallarg(caddr_t) data;
};

struct svr4_sys_utssys_args {
	syscallarg(void *) a1;
	syscallarg(void *) a2;
	syscallarg(int) sel;
	syscallarg(void *) a3;
};

struct svr4_sys_execve_args {
	syscallarg(char *) path;
	syscallarg(char **) argp;
	syscallarg(char **) envp;
};

struct svr4_sys_fcntl_args {
	syscallarg(int) fd;
	syscallarg(int) cmd;
	syscallarg(char *) arg;
};

struct svr4_sys_ulimit_args {
	syscallarg(int) cmd;
	syscallarg(long) newlimit;
};

struct svr4_sys_getdents_args {
	syscallarg(int) fd;
	syscallarg(char *) buf;
	syscallarg(int) nbytes;
};

struct svr4_sys_getmsg_args {
	syscallarg(int) fd;
	syscallarg(struct svr4_strbuf *) ctl;
	syscallarg(struct svr4_strbuf *) dat;
	syscallarg(int *) flags;
};

struct svr4_sys_putmsg_args {
	syscallarg(int) fd;
	syscallarg(struct svr4_strbuf *) ctl;
	syscallarg(struct svr4_strbuf *) dat;
	syscallarg(int) flags;
};

struct svr4_sys_lstat_args {
	syscallarg(char *) path;
	syscallarg(struct svr4_stat *) ub;
};

struct svr4_sys_sigprocmask_args {
	syscallarg(int) how;
	syscallarg(svr4_sigset_t *) set;
	syscallarg(svr4_sigset_t *) oset;
};

struct svr4_sys_sigsuspend_args {
	syscallarg(svr4_sigset_t *) ss;
};

struct svr4_sys_sigaltstack_args {
	syscallarg(struct svr4_sigaltstack *) nss;
	syscallarg(struct svr4_sigaltstack *) oss;
};

struct svr4_sys_sigaction_args {
	syscallarg(int) signum;
	syscallarg(struct svr4_sigaction *) nsa;
	syscallarg(struct svr4_sigaction *) osa;
};

struct svr4_sys_sigpending_args {
	syscallarg(int) what;
	syscallarg(svr4_sigset_t *) mask;
};

struct svr4_sys_context_args {
	syscallarg(int) func;
	syscallarg(struct svr4_ucontext *) uc;
};

struct svr4_sys_statvfs_args {
	syscallarg(char *) path;
	syscallarg(struct svr4_statvfs *) fs;
};

struct svr4_sys_fstatvfs_args {
	syscallarg(int) fd;
	syscallarg(struct svr4_statvfs *) fs;
};

struct svr4_sys_waitsys_args {
	syscallarg(int) grp;
	syscallarg(int) id;
	syscallarg(union svr4_siginfo *) info;
	syscallarg(int) options;
};

struct svr4_sys_hrtsys_args {
	syscallarg(int) cmd;
	syscallarg(int) fun;
	syscallarg(int) sub;
	syscallarg(void *) rv1;
	syscallarg(void *) rv2;
};

struct svr4_sys_pathconf_args {
	syscallarg(char *) path;
	syscallarg(int) name;
};

struct svr4_sys_mmap_args {
	syscallarg(svr4_caddr_t) addr;
	syscallarg(svr4_size_t) len;
	syscallarg(int) prot;
	syscallarg(int) flags;
	syscallarg(int) fd;
	syscallarg(svr4_off_t) pos;
};

struct svr4_sys_fpathconf_args {
	syscallarg(int) fd;
	syscallarg(int) name;
};

struct svr4_sys_xstat_args {
	syscallarg(int) two;
	syscallarg(char *) path;
	syscallarg(struct svr4_xstat *) ub;
};

struct svr4_sys_lxstat_args {
	syscallarg(int) two;
	syscallarg(char *) path;
	syscallarg(struct svr4_xstat *) ub;
};

struct svr4_sys_fxstat_args {
	syscallarg(int) two;
	syscallarg(int) fd;
	syscallarg(struct svr4_xstat *) sb;
};

struct svr4_sys_xmknod_args {
	syscallarg(int) two;
	syscallarg(char *) path;
	syscallarg(svr4_mode_t) mode;
	syscallarg(svr4_dev_t) dev;
};

struct svr4_sys_setrlimit_args {
	syscallarg(int) which;
	syscallarg(struct ogetrlimit *) rlp;
};

struct svr4_sys_getrlimit_args {
	syscallarg(int) which;
	syscallarg(struct ogetrlimit *) rlp;
};

struct svr4_sys_memcntl_args {
	syscallarg(svr4_caddr_t) addr;
	syscallarg(svr4_size_t) len;
	syscallarg(int) cmd;
	syscallarg(svr4_caddr_t) arg;
	syscallarg(int) attr;
	syscallarg(int) mask;
};

struct svr4_sys_uname_args {
	syscallarg(struct svr4_utsname *) name;
	syscallarg(int) dummy;
};

struct svr4_sys_setegid_args {
	syscallarg(gid_t) egid;
};

struct svr4_sys_sysconfig_args {
	syscallarg(int) name;
};

struct svr4_sys_systeminfo_args {
	syscallarg(int) what;
	syscallarg(char *) buf;
	syscallarg(long) len;
};

struct svr4_sys_fchroot_args {
	syscallarg(int) fd;
};

struct svr4_sys_utimes_args {
	syscallarg(char *) path;
	syscallarg(struct timeval *) tptr;
};

struct svr4_sys_gettimeofday_args {
	syscallarg(struct timeval *) tp;
};

struct svr4_sys_pread_args {
	syscallarg(int) fd;
	syscallarg(void *) buf;
	syscallarg(size_t) nbyte;
	syscallarg(svr4_off_t) off;
};

struct svr4_sys_pwrite_args {
	syscallarg(int) fd;
	syscallarg(const void *) buf;
	syscallarg(size_t) nbyte;
	syscallarg(svr4_off_t) off;
};

struct svr4_sys_llseek_args {
	syscallarg(int) fd;
	syscallarg(long) offset1;
	syscallarg(long) offset2;
	syscallarg(int) whence;
};

struct svr4_sys_acl_args {
	syscallarg(char *) path;
	syscallarg(int) cmd;
	syscallarg(int) num;
	syscallarg(struct svr4_aclent *) buf;
};

struct svr4_sys_auditsys_args {
	syscallarg(int) code;
	syscallarg(int) a1;
	syscallarg(int) a2;
	syscallarg(int) a3;
	syscallarg(int) a4;
	syscallarg(int) a5;
};

struct svr4_sys_facl_args {
	syscallarg(int) fd;
	syscallarg(int) cmd;
	syscallarg(int) num;
	syscallarg(struct svr4_aclent *) buf;
};

struct svr4_sys_getdents64_args {
	syscallarg(int) fd;
	syscallarg(struct svr4_dirent64 *) dp;
	syscallarg(int) nbytes;
};

struct svr4_sys_mmap64_args {
	syscallarg(svr4_caddr_t) addr;
	syscallarg(svr4_size_t) len;
	syscallarg(int) prot;
	syscallarg(int) flags;
	syscallarg(int) fd;
	syscallarg(svr4_off64_t) pos;
};

struct svr4_sys_stat64_args {
	syscallarg(const char *) path;
	syscallarg(struct svr4_stat64 *) sb;
};

struct svr4_sys_lstat64_args {
	syscallarg(const char *) path;
	syscallarg(struct svr4_stat64 *) sb;
};

struct svr4_sys_fstat64_args {
	syscallarg(int) fd;
	syscallarg(struct svr4_stat64 *) sb;
};

struct svr4_sys_fstatvfs64_args {
	syscallarg(int) fd;
	syscallarg(struct svr4_statvfs64 *) fs;
};

struct svr4_sys_pread64_args {
	syscallarg(int) fd;
	syscallarg(void *) buf;
	syscallarg(size_t) nbyte;
	syscallarg(svr4_off64_t) off;
};

struct svr4_sys_pwrite64_args {
	syscallarg(int) fd;
	syscallarg(const void *) buf;
	syscallarg(size_t) nbyte;
	syscallarg(svr4_off64_t) off;
};

struct svr4_sys_creat64_args {
	syscallarg(char *) path;
	syscallarg(int) mode;
};

struct svr4_sys_open64_args {
	syscallarg(char *) path;
	syscallarg(int) flags;
	syscallarg(int) mode;
};

struct svr4_sys_socket_args {
	syscallarg(int) domain;
	syscallarg(int) type;
	syscallarg(int) protocol;
};

/*
 * System call prototypes.
 */

int	sys_nosys(struct proc *, void *, register_t *);
int	sys_exit(struct proc *, void *, register_t *);
int	sys_fork(struct proc *, void *, register_t *);
int	sys_read(struct proc *, void *, register_t *);
int	sys_write(struct proc *, void *, register_t *);
int	svr4_sys_open(struct proc *, void *, register_t *);
int	sys_close(struct proc *, void *, register_t *);
int	svr4_sys_wait(struct proc *, void *, register_t *);
int	svr4_sys_creat(struct proc *, void *, register_t *);
int	sys_link(struct proc *, void *, register_t *);
int	sys_unlink(struct proc *, void *, register_t *);
int	svr4_sys_execv(struct proc *, void *, register_t *);
int	sys_chdir(struct proc *, void *, register_t *);
int	svr4_sys_time(struct proc *, void *, register_t *);
int	svr4_sys_mknod(struct proc *, void *, register_t *);
int	sys_chmod(struct proc *, void *, register_t *);
int	sys_chown(struct proc *, void *, register_t *);
int	svr4_sys_break(struct proc *, void *, register_t *);
int	svr4_sys_stat(struct proc *, void *, register_t *);
int	compat_43_sys_lseek(struct proc *, void *, register_t *);
int	sys_getpid(struct proc *, void *, register_t *);
int	sys_setuid(struct proc *, void *, register_t *);
int	sys_getuid(struct proc *, void *, register_t *);
int	svr4_sys_alarm(struct proc *, void *, register_t *);
int	svr4_sys_fstat(struct proc *, void *, register_t *);
int	svr4_sys_pause(struct proc *, void *, register_t *);
int	svr4_sys_utime(struct proc *, void *, register_t *);
int	svr4_sys_access(struct proc *, void *, register_t *);
int	svr4_sys_nice(struct proc *, void *, register_t *);
int	sys_sync(struct proc *, void *, register_t *);
int	svr4_sys_kill(struct proc *, void *, register_t *);
int	svr4_sys_pgrpsys(struct proc *, void *, register_t *);
int	sys_dup(struct proc *, void *, register_t *);
int	sys_opipe(struct proc *, void *, register_t *);
int	svr4_sys_times(struct proc *, void *, register_t *);
int	sys_setgid(struct proc *, void *, register_t *);
int	sys_getgid(struct proc *, void *, register_t *);
int	svr4_sys_signal(struct proc *, void *, register_t *);
#ifdef SYSVMSG
int	svr4_sys_msgsys(struct proc *, void *, register_t *);
#else
#endif
int	svr4_sys_sysarch(struct proc *, void *, register_t *);
#ifdef SYSVSHM
int	svr4_sys_shmsys(struct proc *, void *, register_t *);
#else
#endif
#ifdef SYSVSEM
int	svr4_sys_semsys(struct proc *, void *, register_t *);
#else
#endif
int	svr4_sys_ioctl(struct proc *, void *, register_t *);
int	svr4_sys_utssys(struct proc *, void *, register_t *);
int	sys_fsync(struct proc *, void *, register_t *);
int	svr4_sys_execve(struct proc *, void *, register_t *);
int	sys_umask(struct proc *, void *, register_t *);
int	sys_chroot(struct proc *, void *, register_t *);
int	svr4_sys_fcntl(struct proc *, void *, register_t *);
int	svr4_sys_ulimit(struct proc *, void *, register_t *);
int	svr4_sys_rdebug(struct proc *, void *, register_t *);
int	sys_rmdir(struct proc *, void *, register_t *);
int	sys_mkdir(struct proc *, void *, register_t *);
int	svr4_sys_getdents(struct proc *, void *, register_t *);
int	svr4_sys_getmsg(struct proc *, void *, register_t *);
int	svr4_sys_putmsg(struct proc *, void *, register_t *);
int	sys_poll(struct proc *, void *, register_t *);
int	svr4_sys_lstat(struct proc *, void *, register_t *);
int	sys_symlink(struct proc *, void *, register_t *);
int	sys_readlink(struct proc *, void *, register_t *);
int	sys_getgroups(struct proc *, void *, register_t *);
int	sys_setgroups(struct proc *, void *, register_t *);
int	sys_fchmod(struct proc *, void *, register_t *);
int	sys_fchown(struct proc *, void *, register_t *);
int	svr4_sys_sigprocmask(struct proc *, void *, register_t *);
int	svr4_sys_sigsuspend(struct proc *, void *, register_t *);
int	svr4_sys_sigaltstack(struct proc *, void *, register_t *);
int	svr4_sys_sigaction(struct proc *, void *, register_t *);
int	svr4_sys_sigpending(struct proc *, void *, register_t *);
int	svr4_sys_context(struct proc *, void *, register_t *);
int	svr4_sys_statvfs(struct proc *, void *, register_t *);
int	svr4_sys_fstatvfs(struct proc *, void *, register_t *);
int	svr4_sys_waitsys(struct proc *, void *, register_t *);
int	svr4_sys_hrtsys(struct proc *, void *, register_t *);
int	svr4_sys_pathconf(struct proc *, void *, register_t *);
int	sys_mincore(struct proc *, void *, register_t *);
int	svr4_sys_mmap(struct proc *, void *, register_t *);
int	sys_mprotect(struct proc *, void *, register_t *);
int	sys_munmap(struct proc *, void *, register_t *);
int	svr4_sys_fpathconf(struct proc *, void *, register_t *);
int	sys_vfork(struct proc *, void *, register_t *);
int	sys_fchdir(struct proc *, void *, register_t *);
int	sys_readv(struct proc *, void *, register_t *);
int	sys_writev(struct proc *, void *, register_t *);
int	svr4_sys_xstat(struct proc *, void *, register_t *);
int	svr4_sys_lxstat(struct proc *, void *, register_t *);
int	svr4_sys_fxstat(struct proc *, void *, register_t *);
int	svr4_sys_xmknod(struct proc *, void *, register_t *);
int	svr4_sys_setrlimit(struct proc *, void *, register_t *);
int	svr4_sys_getrlimit(struct proc *, void *, register_t *);
int	sys_lchown(struct proc *, void *, register_t *);
int	svr4_sys_memcntl(struct proc *, void *, register_t *);
int	sys_rename(struct proc *, void *, register_t *);
int	svr4_sys_uname(struct proc *, void *, register_t *);
int	svr4_sys_setegid(struct proc *, void *, register_t *);
int	svr4_sys_sysconfig(struct proc *, void *, register_t *);
int	sys_adjtime(struct proc *, void *, register_t *);
int	svr4_sys_systeminfo(struct proc *, void *, register_t *);
int	sys_seteuid(struct proc *, void *, register_t *);
int	svr4_sys_fchroot(struct proc *, void *, register_t *);
int	svr4_sys_utimes(struct proc *, void *, register_t *);
int	svr4_sys_vhangup(struct proc *, void *, register_t *);
int	svr4_sys_gettimeofday(struct proc *, void *, register_t *);
int	sys_getitimer(struct proc *, void *, register_t *);
int	sys_setitimer(struct proc *, void *, register_t *);
int	svr4_sys_pread(struct proc *, void *, register_t *);
int	svr4_sys_pwrite(struct proc *, void *, register_t *);
int	svr4_sys_llseek(struct proc *, void *, register_t *);
int	svr4_sys_acl(struct proc *, void *, register_t *);
int	svr4_sys_auditsys(struct proc *, void *, register_t *);
int	sys_clock_gettime(struct proc *, void *, register_t *);
int	sys_clock_settime(struct proc *, void *, register_t *);
int	sys_clock_getres(struct proc *, void *, register_t *);
int	sys_nanosleep(struct proc *, void *, register_t *);
int	svr4_sys_facl(struct proc *, void *, register_t *);
int	compat_43_sys_setreuid(struct proc *, void *, register_t *);
int	compat_43_sys_setregid(struct proc *, void *, register_t *);
int	svr4_sys_getdents64(struct proc *, void *, register_t *);
int	svr4_sys_mmap64(struct proc *, void *, register_t *);
int	svr4_sys_stat64(struct proc *, void *, register_t *);
int	svr4_sys_lstat64(struct proc *, void *, register_t *);
int	svr4_sys_fstat64(struct proc *, void *, register_t *);
int	svr4_sys_fstatvfs64(struct proc *, void *, register_t *);
int	svr4_sys_pread64(struct proc *, void *, register_t *);
int	svr4_sys_pwrite64(struct proc *, void *, register_t *);
int	svr4_sys_creat64(struct proc *, void *, register_t *);
int	svr4_sys_open64(struct proc *, void *, register_t *);
int	svr4_sys_socket(struct proc *, void *, register_t *);
int	sys_socketpair(struct proc *, void *, register_t *);
int	sys_bind(struct proc *, void *, register_t *);
int	sys_listen(struct proc *, void *, register_t *);
int	compat_43_sys_accept(struct proc *, void *, register_t *);
int	sys_connect(struct proc *, void *, register_t *);
int	sys_shutdown(struct proc *, void *, register_t *);
int	compat_43_sys_recv(struct proc *, void *, register_t *);
int	compat_43_sys_recvfrom(struct proc *, void *, register_t *);
int	compat_43_sys_recvmsg(struct proc *, void *, register_t *);
int	compat_43_sys_send(struct proc *, void *, register_t *);
int	compat_43_sys_sendmsg(struct proc *, void *, register_t *);
int	sys_sendto(struct proc *, void *, register_t *);
int	compat_43_sys_getpeername(struct proc *, void *, register_t *);
int	compat_43_sys_getsockname(struct proc *, void *, register_t *);
int	sys_getsockopt(struct proc *, void *, register_t *);
int	sys_setsockopt(struct proc *, void *, register_t *);
