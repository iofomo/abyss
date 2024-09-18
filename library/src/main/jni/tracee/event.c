/* -*- c-set-style: "K&R"; c-basic-offset: 8 -*-
 *
 * This file is part of PRoot.
 *
 * Copyright (C) 2015 STMicroelectronics
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#include <stdio.h>
#include <sched.h>      /* CLONE_*,  */
#include <sys/types.h>  /* pid_t, */
#include <sys/ptrace.h> /* ptrace(1), PTRACE_*, */
#include <sys/types.h>  /* waitpid(2), */
#include <sys/wait.h>   /* waitpid(2), */
#include <sys/utsname.h> /* uname(2), */
#include <unistd.h>     /* fork(2), chdir(2), getpid(2), */
#include <string.h>     /* strcmp(3), */
#include <errno.h>      /* errno(3), */
#include <stdbool.h>    /* bool, true, false, */
#include <assert.h>     /* assert(3), */
#include <stdlib.h>     /* atexit(3), getenv(3), */
//#include <talloc.h>     /* talloc_*, */
#include <inttypes.h>   /* PRI*, */
#include <linux/version.h> /* KERNEL_VERSION, */
#include <strings.h>

#include "tracee/event.h"
//#include "cli/note.h"
#include "path/path.h"
//#include "path/binding.h"
#include "syscall/syscall.h"
#include "syscall/seccomp.h"
#include "ptrace/wait.h"
//#include "extension/extension.h"
#include "execve/elf.h"

#include "attribute.h"
#include "compat.h"
#include "constant.h"
#include "cmn/cmn_back_call_stack.h"
#include "note.h"
#include "cmn/cmn_pthread_cond.h"
#include "library.h"

//方便测试
#define TEST_ONLY

#ifdef TEST_ONLY
bool waitting_for_tracee = true;
#endif


/**
 * Start @tracee->exe with the given @argv[].  This function
 * returns -errno if an error occurred, otherwise 0.
 */
//int launch_process(Tracee *tracee, char *const argv[])
//{
//	char *const default_argv[] = { "-sh", NULL };
//	long status;
//	pid_t pid;
//
//	/* Warn about open file descriptors. They won't be
//	 * translated until they are closed. */
//	list_open_fd(tracee);
//
//	pid = fork();
//	switch(pid) {
//	case -1:
//		note(tracee, ERROR, SYSTEM, "fork()");
//		return -errno;
//
//	case 0: /* child */
//		/* Declare myself as ptraceable before executing the
//		 * requested program. */
//		status = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
//		if (status < 0) {
//			note(tracee, ERROR, SYSTEM, "ptrace(TRACEME)");
//			return -errno;
//		}
//
//		/* Synchronize with the tracer's event loop.  Without
//		 * this trick the tracer only sees the "return" from
//		 * the next execve(2) so PRoot wouldn't handle the
//		 * interpreter/runner.  I also verified that strace
//		 * does the same thing. */
//		kill(getpid(), SIGSTOP);
//
//		/* Improve performance by using seccomp mode 2, unless
//		 * this support is explicitly disabled.  */
//		if (getenv("PROOT_NO_SECCOMP") == NULL)
//			(void) enable_syscall_filtering(tracee);
//
//		/* Now process is ptraced, so the current rootfs is already the
//		 * guest rootfs.  Note: Valgrind can't handle execve(2) on
//		 * "foreign" binaries (ENOEXEC) but can handle execvp(3) on such
//		 * binaries.  */
//		execvp(tracee->exe, argv[0] != NULL ? argv : default_argv);
//		return -errno;
//
//	default: /* parent */
//		/* We know the pid of the first tracee now.  */
//		tracee->pid = pid;
//		return 0;
//	}
//
//	/* Never reached.  */
//	return -ENOSYS;
//}

/* Send the KILL signal to all tracees when PRoot has received a fatal
 * signal.  */
static void kill_all_tracees2(int signum, siginfo_t *siginfo UNUSED, void *ucontext UNUSED)
{
	LOGSYSW("signal %d received from process %d",signum, siginfo->si_pid)
//    sig_handler_call_stack(signum,siginfo,ucontext);
//	kill_all_tracees();

	/* Exit immediately for system signals (segmentation fault,
	 * illegal instruction, ...), otherwise exit cleanly through
	 * the event loop.  */
	if (signum != SIGQUIT)
		_exit(EXIT_FAILURE);
}

/**
 * Helper for print_talloc_hierarchy().
 */
//static void print_talloc_chunk(const void *ptr, int depth, int max_depth UNUSED,
//			int is_ref, void *data UNUSED)
//{
//	const char *name;
//	size_t count;
//	size_t size;
//
//	name = talloc_get_name(ptr);
//	size = talloc_get_size(ptr);
//	count = talloc_reference_count(ptr);
//
//	if (depth == 0)
//		return;
//
//	while (depth-- > 1)
//		fprintf(stderr, "\t");
//
//	fprintf(stderr, "%-16s ", name);
//
//	if (is_ref)
//		fprintf(stderr, "-> %-8p", ptr);
//	else {
//		fprintf(stderr, "%-8p  %zd bytes  %zd ref'", ptr, size, count);
//
//		if (name[0] == '$') {
//			fprintf(stderr, "\t(\"%s\")", (char *)ptr);
//		}
//		if (name[0] == '@') {
//			char **argv;
//			int i;
//
//			fprintf(stderr, "\t(");
//			for (i = 0, argv = (char **)ptr; argv[i] != NULL; i++)
//				fprintf(stderr, "\"%s\", ", argv[i]);
//			fprintf(stderr, ")");
//		}
//		else if (strcmp(name, "Tracee") == 0) {
//			fprintf(stderr, "\t(pid = %d, parent = %p)",
//				((Tracee *)ptr)->pid, ((Tracee *)ptr)->parent);
//		}
//		else if (strcmp(name, "Bindings") == 0) {
//			Tracee *tracee;
//
//			tracee = TRACEE(ptr);
//
//			if (ptr == tracee->fs->bindings.pending)
//				fprintf(stderr, "\t(pending)");
//			else if (ptr == tracee->fs->bindings.guest)
//				fprintf(stderr, "\t(guest)");
//			else if (ptr == tracee->fs->bindings.host)
//				fprintf(stderr, "\t(host)");
//		}
//		else if (strcmp(name, "Binding") == 0) {
//			Binding *binding = (Binding *)ptr;
//			fprintf(stderr, "\t(%s:%s)", binding->host.path, binding->guest.path);
//		}
//	}
//
//	fprintf(stderr, "\n");
//}

/* Print on stderr the complete talloc hierarchy.  */
static void print_talloc_hierarchy(int signum, siginfo_t *siginfo UNUSED, void *ucontext UNUSED)
{
	LOGSYSE("print_talloc_hierarchy %d",signum)
//	switch (signum) {
//	case SIGUSR1:
//		talloc_report_depth_cb(NULL, 0, 100, print_talloc_chunk, NULL);
//		break;
//
//	case SIGUSR2:
//		talloc_report_depth_file(NULL, 0, 100, stderr);
//		break;
//
//	default:
//		break;
//	}
}

static int last_exit_status = -1;

/**
 * Check if kernel >= 4.8
 */
static bool is_kernel_4_8(void)
{
	static int version_48 = -1;
	int major = 0;
	int minor = 0;

	if (version_48 != -1)
		return version_48;

	version_48 = false;

	struct utsname utsname;

	if (uname(&utsname) < 0)
		return false;

	sscanf(utsname.release, "%d.%d", &major, &minor);

	if ((major == 4 && minor >= 8) || major > 4)
		version_48 = true;

	return version_48;
}

/**
 * Wait then handle any event from any tracee.  This function returns
 * the exit status of the last terminated program.
 */
int event_loop()
{
	struct sigaction signal_action;
	long status;
    long trace_new_status;
	int signum;
    Tracee *new_tracee;

	/* Kill all tracees when exiting.  */
	status = atexit(kill_all_tracees);
	if (status != 0)
		LOGSYSW("atexit() failed")

	/* All signals are blocked when the signal handler is called.
	 * SIGINFO is used to know which process has signaled us and
	 * RESTART is used to restart waitpid(2) seamlessly.  */
	bzero(&signal_action, sizeof(signal_action));
	signal_action.sa_flags = SA_SIGINFO | SA_RESTART;
	status = sigfillset(&signal_action.sa_mask);
	if (status < 0)
		LOGSYSW("sigfillset()")

	/* Handle all signals.  */
	for (signum = 0; signum < SIGRTMAX; signum++) {
		switch (signum) {
		case SIGQUIT:
		case SIGILL:
		case SIGABRT:
		case SIGFPE:
		case SIGSEGV:
			/* Kill all tracees on abnormal termination
			 * signals.  This ensures no process is left
			 * untraced.  */
			signal_action.sa_sigaction = kill_all_tracees2;
			break;

		case SIGUSR1:
		case SIGUSR2:
			/* Print on stderr the complete talloc
			 * hierarchy, useful for debug purpose.  */
			signal_action.sa_sigaction = print_talloc_hierarchy;
			break;

		case SIGCHLD:
		case SIGCONT:
		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/* The default action is OK for these signals,
			 * they are related to tty and job control.  */
			continue;

		default:
			/* Ignore all other signals, including
			 * terminating ones (^C for instance). */
			signal_action.sa_sigaction = (void *)SIG_IGN;
			break;
		}

		status = sigaction(signum, &signal_action, NULL);
		if (status < 0 && errno != EINVAL)
			note(NULL, WARNING, SYSTEM, "sigaction(%d)", signum);
	}
	LOGD("tracer waitpid start,cur_pid:%d",getpid())
	while (1) {
		int tracee_status;
		Tracee *tracee;
		int signal;
		pid_t pid;

		/* This is the only safe place to free tracees.  */
		free_terminated_tracees();

		/* Wait for the next tracee's stop. */
//		LOGD("tracer waitpid,cur_pid:%d",getpid())
		pid = waitpid(-1, &tracee_status, __WALL);
		if (pid < 0) {
//			LOGD("resume pid error:%d,cur_pid:%d,%d,%s",pid,getpid(),errno, strerror(errno))
			if (errno != ECHILD) {
				LOGSYSE("waitpid()")
				return EXIT_FAILURE;
			}
#ifdef TEST_ONLY
            if (!waitting_for_tracee){
                LOGSYS("no more tracee,exit---------------");
                break;
            }
#endif
			continue;
		}

//        LOGD("resume pid:%d,snew_attach_pid:%d,tracee_status:%d,%d,%d",pid,snew_attach_pid,tracee_status,WIFEXITED(tracee_status),WEXITSTATUS(tracee_status))
		if (snew_attach_pid != -1 && WIFEXITED(tracee_status) == 1 &&  255 == WEXITSTATUS(tracee_status) && get_tracee(NULL, pid, false) == NULL){//会有并发问题吗?? 需要更严格校验不???
			do {
				LOGSYS("handle helper event,snew_attach_pid:%d",snew_attach_pid)
				trace_new_status = ptrace(PTRACE_ATTACH,snew_attach_pid,NULL,NULL);
				if (trace_new_status < 0){
					LOGE("ptrace %d error,%d,%s",snew_attach_pid,errno, strerror(errno))
					break;
				}
#ifdef TEST_ONLY
                waitting_for_tracee = false; //只要存在过tracee,就置为false
                LOGSYS("test only enabled,waitting_for_tracee false")
#endif
				new_tracee = get_tracee(NULL, 0, true);
				if (!new_tracee){
					LOGE("new tracee error,%d",snew_attach_pid)
					break;
				}
				new_tracee->verbose = 6; // -v 6 调试用
				new_tracee->pid = snew_attach_pid;
				new_tracee->is_root = true;
				new_tracee->deliver_sigtrap = false;
				LOGD("notify start trace 进程")
				cmn_pt_signal(&spctx);
				snew_attach_pid = -1;
				LOGD("new trace finished，cur_pid:%d",getpid())
			} while (0);
			continue;
		}

		/* Get information about this tracee. */
		tracee = get_tracee(NULL, pid, true);
		assert(tracee != NULL);
//		LOGD("vpid %" PRIu64 ",resume pid:%d,cur_pid:%d",tracee->vpid,pid,getpid())
		tracee->running = false;

//		VERBOSE(tracee, 6, "vpid %" PRIu64 ": got event %x",
//			tracee->vpid, tracee_status);
		LOGSYS("vpid %" PRIu64 ": got event %x",tracee->vpid, tracee_status)

//		status = notify_extensions(tracee, NEW_STATUS, tracee_status, 0);
//		if (status != 0)
//			continue;

		if (tracee->as_ptracee.ptracer != NULL) {
			bool keep_stopped = handle_ptracee_event(tracee, tracee_status);
			if (keep_stopped)
				continue;
		}

		signal = handle_tracee_event(tracee, tracee_status);
#if 0
        //test only
        if (signal == SIGSEGV){
			//fetch寄存器
			int  fstatus = fetch_regs(tracee);
			if (fstatus < 0){
				VERBOSE(tracee, 6, "vpid %" PRIu64 ": fetch regs fatal error, signal %d, tracee pid %d",
						tracee->vpid, signal,tracee->pid);
			}else{
				#if defined(__aarch64__)
					LOGSYS("vpid %" PRIu64 ": fatal error signal %d, pid %d, pc %lx",
		   			tracee->vpid, signal,tracee->pid, tracee->_regs[CURRENT].pc)
                print_remote_call_stack_arm64(tracee);
                #endif
			}
            exit(0);
        }
#endif
		(void) restart_tracee(tracee, signal);
	}

	return last_exit_status;
}

/**
 * For kernels >= 4.8.0
 * Handle the current event (@tracee_status) of the given @tracee.
 * This function returns the "computed" signal that should be used to
 * restart the given @tracee.
 */
int handle_tracee_event(Tracee *tracee, int tracee_status)
{
//	LOGD("handle_tracee_event_kernel_4_8")
	//TODO 每个进程树放到一个队列里，然后这个队列都维护一个global信息存储seccomp_detected、seccomp_enabled信息
//	static bool seccomp_detected = false;
//	static bool seccomp_enabled = false; /* added for 4.8.0 */
	bool seccomp_detected = tracee->seccomp_detected;
	bool seccomp_enabled = tracee->seccomp_enabled;
	long status;
	int signal;

//	VERBOSE(tracee, 6, "vpid %" PRIu64 ": handle_tracee_event_kernel_4_8  restart_how: %d, seccomp: %d, sysexit_pending: %d",
//			tracee->vpid, tracee->restart_how, tracee->seccomp,tracee->sysexit_pending);
	/* Don't overwrite restart_how if it is explicitly set
	 * elsewhere, i.e in the ptrace emulation when single
	 * stepping.  */
	if (tracee->restart_how == 0) {
		/* When seccomp is enabled, all events are restarted in
		 * non-stop mode, but this default choice could be overwritten
		 * later if necessary.  The check against "sysexit_pending"
		 * ensures PTRACE_SYSCALL (used to hit the exit stage under
		 * seccomp) is not cleared due to an event that would happen
		 * before the exit stage, eg. PTRACE_EVENT_EXEC for the exit
		 * stage of execve(2).  */
		if (tracee->seccomp == ENABLED && !tracee->sysexit_pending)
			tracee->restart_how = PTRACE_CONT;
		else
			tracee->restart_how = PTRACE_SYSCALL;
	}

	/* Not a signal-stop by default.  */
	signal = 0;

	if (WIFEXITED(tracee_status)) {
		last_exit_status = WEXITSTATUS(tracee_status);
//		VERBOSE(tracee, 1,
//			"vpid %" PRIu64 ": exited with status %d",
//			tracee->vpid, last_exit_status);
		LOGSYS("vpid %" PRIu64 ": exited with status %d",
			tracee->vpid, last_exit_status);
		terminate_tracee(tracee);
	}
	else if (WIFSIGNALED(tracee_status)) {
//		check_architecture(tracee);
//		VERBOSE(tracee, 1,
//			"vpid %" PRIu64 ": terminated with signal %d",
//			tracee->vpid, WTERMSIG(tracee_status));
		LOGSYS("vpid %" PRIu64 ": terminated with signal %d",
				tracee->vpid, WTERMSIG(tracee_status));
		terminate_tracee(tracee);
	}
	else if (WIFSTOPPED(tracee_status)) {
		/* Don't use WSTOPSIG() to extract the signal
		 * since it clears the PTRACE_EVENT_* bits. */
		signal = (tracee_status & 0xfff00) >> 8;

		switch (signal) {
//			static bool deliver_sigtrap = false;

		case SIGTRAP: {
			const unsigned long default_ptrace_options = (
				PTRACE_O_TRACESYSGOOD	|
				PTRACE_O_TRACEFORK	|
				PTRACE_O_TRACEVFORK	|
				PTRACE_O_TRACEVFORKDONE	|
				PTRACE_O_TRACEEXEC	|
				PTRACE_O_TRACECLONE	|
				PTRACE_O_TRACEEXIT);


			LOGD("vpid %" PRIu64 ",is_root %d,deliver_sigtrap %d",tracee->vpid,tracee->is_root,tracee->deliver_sigtrap)
			/* Distinguish some events from others and
			 * automatically trace each new process with
			 * the same options.
			 *
			 * Note that only the first bare SIGTRAP is
			 * related to the tracing loop, others SIGTRAP
			 * carry tracing information because of
			 * TRACE*FORK/CLONE/EXEC.  */
//			if (deliver_sigtrap)
//				break;  /* Deliver this signal as-is.  */
//
//			deliver_sigtrap = true;
			if (!tracee->is_root || tracee->deliver_sigtrap){
				break;
			}
			//是进程树root && 它的deliver_sigtrap为false
			tracee->deliver_sigtrap = true;

			/* Try to enable seccomp mode 2...  */
			errno = 0;
			status = ptrace(PTRACE_SETOPTIONS, tracee->pid, NULL,
					default_ptrace_options | PTRACE_O_TRACESECCOMP);
			if (status < 0) {
				LOGD("vpid %" PRIu64 ",PTRACE_SETOPTIONS status %ld,%d,%s",tracee->vpid,status,errno,
					 strerror(errno))
				tracee->seccomp_enabled = false;
				/* ... otherwise use default options only.  */
				status = ptrace(PTRACE_SETOPTIONS, tracee->pid, NULL,
						default_ptrace_options);
				if (status < 0) {
//					note(tracee, ERROR, SYSTEM, "ptrace(PTRACE_SETOPTIONS)");
					LOGSYSE("ptrace(PTRACE_SETOPTIONS)")
					exit(EXIT_FAILURE);
				}
			}
			else {
                tracee->seccomp_enabled = true;
			}
			LOGD("vpid %" PRIu64 ",secomp_enabled %d",tracee->vpid,tracee->seccomp_enabled)
		}
			/* Fall through. */
		case SIGTRAP | PTRACE_EVENT_SECCOMP2 << 8:
		case SIGTRAP | PTRACE_EVENT_SECCOMP << 8:
			LOGD("vpid %" PRIu64 ",secomp_enabled %d,",tracee->vpid,tracee->seccomp_enabled)
			if (!tracee->seccomp_detected && tracee->seccomp_enabled) {
				VERBOSE(tracee, 1, "ptrace acceleration (seccomp mode 2) enabled"); //第二个进程没走这里?
//				LOGSYS("ptrace acceleration (seccomp mode 2) enabled")
				tracee->seccomp = ENABLED;
				tracee->seccomp_detected = true;
			}

			if (signal == (SIGTRAP | PTRACE_EVENT_SECCOMP2 << 8) ||
			    signal == (SIGTRAP | PTRACE_EVENT_SECCOMP << 8)) {

				unsigned long flags = 0;
				signal = 0;

				/* Use the common ptrace flow if seccomp was
				 * explicitly disabled for this tracee.  */
				if (tracee->seccomp != ENABLED)
					break;

				status = ptrace(PTRACE_GETEVENTMSG, tracee->pid, NULL, &flags);
				if (status < 0)
					break;

				if ((flags & FILTER_SYSEXIT) == 0) {
					tracee->restart_how = PTRACE_CONT;
					translate_syscall(tracee);

					if (tracee->seccomp == DISABLING)
						tracee->restart_how = PTRACE_SYSCALL;
					break;
				}
			}

			/* Fall through. */
		case SIGTRAP | 0x80:

			signal = 0;

			/* This tracee got signaled then freed during the
			   sysenter stage but the kernel reports the sysexit
			   stage; just discard this spurious tracee/event.  */

//			if (tracee->exe == NULL) {
//				tracee->restart_how = PTRACE_CONT; /* SYSCALL OR CONT */
//				return 0;
//			}

			switch (tracee->seccomp) {
			case ENABLED:
				if (IS_IN_SYSENTER(tracee)) {
					/* sysenter: ensure the sysexit
					 * stage will be hit under seccomp.  */
					tracee->restart_how = PTRACE_SYSCALL;
					tracee->sysexit_pending = true;
				}
				else {
					/* sysexit: the next sysenter
					 * will be notified by seccomp.  */
					tracee->restart_how = PTRACE_CONT;
					tracee->sysexit_pending = false;
				}
				/* Fall through.  */
			case DISABLED:
				translate_syscall(tracee);

				/* This syscall has disabled seccomp.  */
				if (tracee->seccomp == DISABLING) {
					tracee->restart_how = PTRACE_SYSCALL;
					tracee->seccomp = DISABLED;
				}

				break;

			case DISABLING:
				/* Seccomp was disabled by the
				 * previous syscall, but its sysenter
				 * stage was already handled.  */
				tracee->seccomp = DISABLED;
				if (IS_IN_SYSENTER(tracee))
					tracee->status = 1;
				break;
			}
			break;

		case SIGTRAP | PTRACE_EVENT_VFORK << 8:
			signal = 0;
			(void) new_child(tracee, CLONE_VFORK);
			break;

		case SIGTRAP | PTRACE_EVENT_FORK  << 8:
		case SIGTRAP | PTRACE_EVENT_CLONE << 8:
			signal = 0;
			(void) new_child(tracee, 0);
			break;

		case SIGTRAP | PTRACE_EVENT_VFORK_DONE << 8:
		case SIGTRAP | PTRACE_EVENT_EXEC  << 8:
		case SIGTRAP | PTRACE_EVENT_EXIT  << 8:
			signal = 0;
			break;

		case SIGSTOP:
			/* Stop this tracee until PRoot has received
			 * the EVENT_*FORK|CLONE notification.  */
			//exe为空不是问题
//			if (tracee->exe == NULL) {
//				tracee->sigstop = SIGSTOP_PENDING;
//				signal = -1;
//			}

			/* For each tracee, the first SIGSTOP
			 * is only used to notify the tracer.  */
			if (tracee->sigstop == SIGSTOP_IGNORED) {
				LOGD("in first stop----------------------,tracee pid:%d",tracee->pid)
				tracee->sigstop = SIGSTOP_ALLOWED;
				signal = 0;
			}
			break;

		default:
			/* Deliver this signal as-is.  */
			break;
		}
	}

	/* Clear the pending event, if any.  */
	tracee->as_ptracee.event4.proot.pending = false;

	return signal;
}

/**
 * Restart the given @tracee with the specified @signal.  This
 * function returns false if the tracee was not restarted (error or
 * put in the "waiting for ptracee" state), otherwise true.
 */
bool restart_tracee(Tracee *tracee, int signal)
{
//	LOGSYS("restart_tracee %p,signal:%d,wait_pid:%d,how:%d,trace_pid:%d",tracee,signal,tracee->as_ptracer.wait_pid,tracee->restart_how,tracee->pid)
	int status;

	/* Put in the "stopped"/"waiting for ptracee" state?.  */
	if (tracee->as_ptracer.wait_pid != 0 || signal == -1)
		return false;

	/* Restart the tracee and stop it at the next instruction, or
	 * at the next entry or exit of a system call. */
	status = ptrace(tracee->restart_how, tracee->pid, NULL, signal);
	if (status < 0){
		LOGE("restart_tracee error,%d,%d,%d,err:%d,%s",tracee->restart_how,tracee->pid,signal,errno,
			 strerror(errno))
		return false; /* The process likely died in a syscall.  */
	}

	VERBOSE(tracee, 6, "vpid %" PRIu64 ": restarted using %d, signal %d, tracee pid %d,app_pid %d",
		tracee->vpid, tracee->restart_how, signal,tracee->pid,(tracee->parent != NULL ? tracee->parent->pid : tracee->pid));

	tracee->restart_how = 0;
	tracee->running = true;

	return true;
}
