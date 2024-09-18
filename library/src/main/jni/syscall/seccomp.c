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

#include "build.h"
#include "arch.h"
#define HAVE_SECCOMP_FILTER

#if defined(HAVE_SECCOMP_FILTER)

#include <sys/prctl.h>     /* prctl(2), PR_* */
#include <linux/filter.h>  /* struct sock_*, */
#include <linux/seccomp.h> /* SECCOMP_MODE_FILTER, */
#include <linux/filter.h>  /* struct sock_*, */
#include <linux/audit.h>   /* AUDIT_, */
#include <sys/queue.h>     /* LIST_FOREACH, */
#include <sys/types.h>     /* size_t, */
//#include <talloc.h>        /* talloc_*, */
#include <errno.h>         /* E*, */
#include <string.h>        /* memcpy(3), */
#include <stddef.h>        /* offsetof(3), */
#include <stdint.h>        /* uint*_t, UINT*_MAX, */
#include <assert.h>        /* assert(3), */
#include <stdlib.h>

#include "syscall/seccomp.h"
#include "tracee/tracee.h"
#include "syscall/syscall.h"
#include "syscall/sysnum.h"
//#include "extension/extension.h"
//#include "cli/note.h"
#include "cmn/cmn_vlarray.h"

#include "compat.h"
#include "attribute.h"
#include "constant.h"

#define DEBUG_FILTER(...) /* fprintf(stderr, __VA_ARGS__) */

//libc的地址区间
static uintptr_t libc_start = -1;
static uintptr_t libc_end = -1;
static bool s_exclude_libc = true;
/**
 * Allocate an empty @program->filter.  This function returns -errno
 * if an error occurred, otherwise 0.
 */
static int new_program_filter(struct sock_fprog *program)
{
	//分配一个长度为0的数组
//	program->filter = talloc_array(NULL, struct sock_filter, 0);
	program->filter = vl_new_array(sizeof(struct  sock_filter),0);
	if (program->filter == NULL)
		return -ENOMEM;

	program->len = 0;
	return 0;
}

/**
 * Append to @program->filter the given @statements (@nb_statements
 * items).  This function returns -errno if an error occurred,
 * otherwise 0.
 */
static int add_statements(struct sock_fprog *program, size_t nb_statements,
			struct sock_filter *statements)
{
	size_t length;
	void *tmp;
	size_t i;
	//获取program->filter数组的当前长度
//	length = talloc_array_length(program->filter);
	//改变program->filter数组的长度为length + nb_statements,返回新的数组为tmp
//	tmp  = talloc_realloc(NULL, program->filter, struct sock_filter, length + nb_statements);

	length = vl_array_length(program->filter);
	tmp = vl_array_realloc(program->filter, sizeof(struct sock_filter),length + nb_statements);
	if (tmp == NULL)
		return -ENOMEM;
	program->filter = tmp;

	for (i = 0; i < nb_statements; i++, length++)
		memcpy(&program->filter[length], &statements[i], sizeof(struct sock_filter));

	return 0;
}

/**
 * Append to @program->filter the statements required to notify PRoot
 * about the given @syscall made by a tracee, with the given @flag.
 * This function returns -errno if an error occurred, otherwise 0.
 */
static int add_trace_syscall(struct sock_fprog *program, word_t syscall, int flag)
{
	int status;

	/* Sanity check.  */
	if (syscall > UINT32_MAX)
		return -ERANGE;

	#define LENGTH_TRACE_SYSCALL 2
	struct sock_filter statements[LENGTH_TRACE_SYSCALL] = {
		/* Compare the accumulator with the expected syscall:
		 * skip the next statement if not equal.  */
		BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, syscall, 0, 1),

		/* Notify the tracer.  */
		BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_TRACE + flag)
	};

	DEBUG_FILTER("FILTER:     trace if syscall == %ld\n", syscall);

	status = add_statements(program, LENGTH_TRACE_SYSCALL, statements);
	if (status < 0)
		return status;

	return 0;
}

/**
 * Append to @program->filter the statements that allow anything (if
 * unfiltered).  Note that @nb_traced_syscalls is used to make a
 * sanity check.  This function returns -errno if an error occurred,
 * otherwise 0.
 */
static int end_arch_section(struct sock_fprog *program, size_t nb_traced_syscalls)
{
    LOGSECOMP("end_arch_section")
	int status;

	#define LENGTH_END_SECTION 1
	struct sock_filter statements[LENGTH_END_SECTION] = {
		BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW)
	};

	DEBUG_FILTER("FILTER:     allow\n");

	status = add_statements(program, LENGTH_END_SECTION, statements);
	if (status < 0)
		return status;

	/* Sanity check, see start_arch_section().  */
//	if (   talloc_array_length(program->filter) - program->len
//	    != LENGTH_END_SECTION + nb_traced_syscalls * LENGTH_TRACE_SYSCALL)
//		return -ERANGE;
	if (   vl_array_length(program->filter) - program->len
		   != LENGTH_END_SECTION + nb_traced_syscalls * LENGTH_TRACE_SYSCALL)
		return -ERANGE;

	return 0;
}

/**
 * Append to @program->filter the statements that check the current
 * @architecture.  Note that @nb_traced_syscalls is used to make a
 * sanity check.  This function returns -errno if an error occurred,
 * otherwise 0.
 */
static int start_arch_section(struct sock_fprog *program, uint32_t arch, size_t nb_traced_syscalls)
{
	const size_t arch_offset    = offsetof(struct seccomp_data, arch);
	const size_t syscall_offset = offsetof(struct seccomp_data, nr);
	const size_t section_length = LENGTH_END_SECTION +
					nb_traced_syscalls * LENGTH_TRACE_SYSCALL;
	int status;

	/* Sanity checks.  */
	if (   arch_offset    > UINT32_MAX
	    || syscall_offset > UINT32_MAX
	    || section_length > UINT32_MAX - 1)
		return -ERANGE;

    if (s_exclude_libc){
        LOGSECOMP("seccomp: exclude libc----------")
        #define LENGTH_START_SECTION 9
        struct sock_filter exclude_statements[LENGTH_START_SECTION] = {
				/* Load the current architecture into the
 				 * accumulator.  */
				BPF_STMT(BPF_LD + BPF_W + BPF_ABS, arch_offset),

				/* Compare the accumulator with the expected
                 * architecture: skip the following statement if
                 * equal.  */
				BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, arch, 1, 0),

				/* This is not the expected architecture, so jump
                 * unconditionally to the end of this section.  */
//				BPF_STMT(BPF_JMP + BPF_JA + BPF_K, section_length + 4),
                BPF_STMT(BPF_JMP + BPF_JA + BPF_K, 4),

				/* This is the expected architecture, so load the
                 * current syscall into the accumulator.  */
//				BPF_STMT(BPF_LD + BPF_W + BPF_ABS, syscall_offset)
				//筛选 libc之外的通知
                BPF_STMT(BPF_LD + BPF_W + BPF_ABS, offsetof(struct seccomp_data, instruction_pointer)),
                BPF_JUMP(BPF_JMP + BPF_JGT + BPF_K, libc_end, 0, 1), //>= libc_end
				BPF_STMT(BPF_JMP + BPF_JA + BPF_K, 2), //往下跳几行，是我们关心的
                BPF_JUMP(BPF_JMP + BPF_JGE + BPF_K, libc_start, 0, 1), // <= libc_end
				BPF_STMT(BPF_JMP + BPF_JA + BPF_K, section_length), //我们不关心的
				BPF_STMT(BPF_LD + BPF_W + BPF_ABS, syscall_offset) //我们关心的分支
        };
        status = add_statements(program, LENGTH_START_SECTION, exclude_statements);
        if (status < 0)
            return status;
    }else{
        #undef LENGTH_START_SECTION
		#define LENGTH_START_SECTION 4
		struct sock_filter statements[LENGTH_START_SECTION] = {
				/* Load the current architecture into the
                 * accumulator.  */
				BPF_STMT(BPF_LD + BPF_W + BPF_ABS, arch_offset),

				/* Compare the accumulator with the expected
                 * architecture: skip the following statement if
                 * equal.  */
				BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, arch, 1, 0),

				/* This is not the expected architecture, so jump
                 * unconditionally to the end of this section.  */
				BPF_STMT(BPF_JMP + BPF_JA + BPF_K, section_length + 1),

				/* This is the expected architecture, so load the
                 * current syscall into the accumulator.  */
				BPF_STMT(BPF_LD + BPF_W + BPF_ABS, syscall_offset)
		};

		DEBUG_FILTER("FILTER: if arch == %ld, up to %zdth statement\n",
					 arch, nb_traced_syscalls);

		status = add_statements(program, LENGTH_START_SECTION, statements);
		if (status < 0)
			return status;
	}

	/* See the sanity check in end_arch_section().  */
//	program->len = talloc_array_length(program->filter);
	program->len = vl_array_length(program->filter);
	return 0;
}

/**
 * Append to @program->filter the statements that forbid anything (if
 * unfiltered) and update @program->len.  This function returns -errno
 * if an error occurred, otherwise 0.
 */
static int finalize_program_filter(struct sock_fprog *program)
{
	int status;

	#define LENGTH_FINALIZE 1
	struct sock_filter statements[LENGTH_FINALIZE] = {
		BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_KILL)
	};

	DEBUG_FILTER("FILTER: kill\n");

	status = add_statements(program, LENGTH_FINALIZE, statements);
	if (status < 0)
		return status;

//	program->len = talloc_array_length(program->filter);
	program->len = vl_array_length(program->filter);
	return 0;
}

/**
 * Free @program->filter and set @program->len to 0.
 */
static void free_program_filter(struct sock_fprog *program)
{
//	TALLOC_FREE(program->filter);
	vl_array_free(program->filter);
	program->len = 0;
}

/**
 * Convert the given @sysnums into BPF filters according to the
 * following pseudo-code, then enabled them for the given @tracee and
 * all of its future children:
 *
 *     for each handled architectures
 *         for each filtered syscall
 *             trace
 *         allow
 *     kill
 *
 * This function returns -errno if an error occurred, otherwise 0.
 */
static int set_seccomp_filters(const FilteredSysnum *sysnums)
{
	SeccompArch seccomp_archs[] = SECCOMP_ARCHS;
	size_t nb_archs = sizeof(seccomp_archs) / sizeof(SeccompArch);

	struct sock_fprog program = { .len = 0, .filter = NULL };
	size_t nb_traced_syscalls;
	size_t i, j, k;
	int status;

	status = new_program_filter(&program);
	if (status < 0)
		goto end;

	/* For each handled architectures */
	for (i = 0; i < nb_archs; i++) {
		word_t syscall;

		nb_traced_syscalls = 0;

		/* Pre-compute the number of traced syscalls for this architecture.  */
		for (j = 0; j < seccomp_archs[i].nb_abis; j++) {
			for (k = 0; sysnums[k].value != PR_void; k++) {
				syscall = detranslate_sysnum(seccomp_archs[i].abis[j], sysnums[k].value);
				if (syscall != SYSCALL_AVOIDER)
					nb_traced_syscalls++;
			}
		}

		/* Filter: if handled architecture */
		status = start_arch_section(&program, seccomp_archs[i].value, nb_traced_syscalls);
		if (status < 0)
			goto end;

		for (j = 0; j < seccomp_archs[i].nb_abis; j++) {
			LOGSECOMP("secomp: set_seccomp_filters abi:%d",seccomp_archs[i].abis[j])
			for (k = 0; sysnums[k].value != PR_void; k++) {
				/* Get the architecture specific syscall number.  */
				syscall = detranslate_sysnum(seccomp_archs[i].abis[j], sysnums[k].value);
				if (syscall == SYSCALL_AVOIDER)
					continue;

				/* Filter: trace if handled syscall */
				LOGSECOMP("secomp: set_seccomp_filters 2 abi:%d,syscall:%lu",seccomp_archs[i].abis[j],syscall)
				status = add_trace_syscall(&program, syscall, sysnums[k].flags);
				if (status < 0)
					goto end;
			}
		}

		/* Filter: allow untraced syscalls for this architecture */
		status = end_arch_section(&program, nb_traced_syscalls);
		if (status < 0)
			goto end;
	}

	status = finalize_program_filter(&program);
	if (status < 0)
		goto end;

	status = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
	if (status < 0)
		goto end;

	/* To output this BPF program for debug purpose:
	 *
	 *     write(2, program.filter, program.len * sizeof(struct sock_filter));
	 */
    LOGSECOMP("secomp: before set---")
	status = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program);
	if (status < 0)
		goto end;

	status = 0;
end:
	free_program_filter(&program);
	return status;
}

/* List of sysnums handled by PRoot.  */
static FilteredSysnum proot_sysnums[] = {
	{ PR_accept,		FILTER_SYSEXIT },
	{ PR_accept4,		FILTER_SYSEXIT },
	{ PR_access,		0 },
	{ PR_acct,		0 },
	{ PR_bind,		0 },
	{ PR_brk,		FILTER_SYSEXIT },
	{ PR_chdir,		FILTER_SYSEXIT },
	{ PR_chmod,		0 },
	{ PR_chown,		0 },
	{ PR_chown32,		0 },
	{ PR_chroot,		0 },
	{ PR_connect,		0 },
	{ PR_creat,		0 },
	{ PR_execve,		FILTER_SYSEXIT },
	{ PR_faccessat,		0 },
	{ PR_fchdir,		FILTER_SYSEXIT },
	{ PR_fchmodat,		0 },
	{ PR_fchownat,		0 },
	{ PR_fstatat64,		0 },
	{ PR_futimesat,		0 },
	{ PR_getcwd,		FILTER_SYSEXIT },
	{ PR_getpeername,	FILTER_SYSEXIT },
	{ PR_getsockname,	FILTER_SYSEXIT },
	{ PR_getxattr,		0 },
	{ PR_inotify_add_watch,	0 },
	{ PR_lchown,		0 },
	{ PR_lchown32,		0 },
	{ PR_lgetxattr,		0 },
	{ PR_link,		0 },
	{ PR_linkat,		0 },
	{ PR_listxattr,		0 },
	{ PR_llistxattr,	0 },
	{ PR_lremovexattr,	0 },
	{ PR_lsetxattr,		0 },
	{ PR_lstat,		0 },
	{ PR_lstat64,		0 },
	{ PR_mkdir,		0 },
	{ PR_mkdirat,		0 },
	{ PR_mknod,		0 },
	{ PR_mknodat,		0 },
	{ PR_mount,		0 },
	{ PR_name_to_handle_at,	0 },
	{ PR_newfstatat,	0 },
	{ PR_oldlstat,		0 },
	{ PR_oldstat,		0 },
	{ PR_open,		0 },
	{ PR_openat,		0 },
	{ PR_close,		0 }, //添加针对close系统调用的监听
	{ PR_pivot_root,	0 },
	{ PR_prctl, 		0 },
	{ PR_prlimit64,		FILTER_SYSEXIT },
	{ PR_ptrace,		FILTER_SYSEXIT },
	{ PR_readlink,		FILTER_SYSEXIT },
	{ PR_readlinkat,	FILTER_SYSEXIT },
	{ PR_removexattr,	0 },
	{ PR_rename,		FILTER_SYSEXIT },
	{ PR_renameat,		FILTER_SYSEXIT },
	{ PR_renameat2,		FILTER_SYSEXIT },
	{ PR_rmdir,		0 },
	{ PR_setrlimit,		FILTER_SYSEXIT },
	{ PR_setxattr,		0 },
	{ PR_socketcall,	FILTER_SYSEXIT },
	{ PR_stat,		0 },
	{ PR_statx,		0 },
	{ PR_faccessat2,	0 },
	{ PR_stat64,		0 },
	{ PR_statfs,		0 },
	{ PR_statfs64,		0 },
	{ PR_swapoff,		0 },
	{ PR_swapon,		0 },
	{ PR_symlink,		0 },
	{ PR_symlinkat,		0 },
	{ PR_truncate,		0 },
	{ PR_truncate64,	0 },
	{ PR_umount,		0 },
	{ PR_umount2,		0 },
	{ PR_uname,		FILTER_SYSEXIT },
	{ PR_unlink,		0 },
	{ PR_unlinkat,		0 },
	{ PR_uselib,		0 },
	{ PR_utime,		0 },
	{ PR_utimensat,		0 },
	{ PR_utimes,		0 },
	{ PR_wait4,		FILTER_SYSEXIT },
	{ PR_waitpid,		FILTER_SYSEXIT },
	FILTERED_SYSNUM_END,
};

/**
 * Add the @new_sysnums to the list of filtered @sysnums, using the
 * given Talloc @context.  This function returns -errno if an error
 * occurred, otherwise 0.
 */
//static int merge_filtered_sysnums(void *context, FilteredSysnum **sysnums,
//				const FilteredSysnum *new_sysnums)
static int merge_filtered_sysnums(FilteredSysnum **sysnums,
								  const FilteredSysnum *new_sysnums)
{
	size_t i, j;

	assert(sysnums != NULL);

	if (*sysnums == NULL) {
		/* Start with no sysnums but the terminator.  */
//		*sysnums = talloc_array(context, FilteredSysnum, 1);
		LOGSECOMP("secomp: merge_filtered_sysnums 1")
		*sysnums = vl_new_array(sizeof(FilteredSysnum), 1);
		if (*sysnums == NULL)
			return -ENOMEM;

		(*sysnums)[0].value = PR_void;
	}

	for (i = 0; new_sysnums[i].value != PR_void; i++) {
		LOGSECOMP("secomp: merge_filtered_sysnums 2,val:%d",new_sysnums[i].value)
		/* Search for the given sysnum.  */
		for (j = 0; (*sysnums)[j].value != PR_void
			 && (*sysnums)[j].value != new_sysnums[i].value; j++)
			;

		if ((*sysnums)[j].value == PR_void) {
			/* No such sysnum, allocate a new entry.  */
//			(*sysnums) = talloc_realloc(context, (*sysnums), FilteredSysnum, j + 2);
			(*sysnums) = vl_array_realloc((*sysnums), sizeof(FilteredSysnum), j + 2);
			if ((*sysnums) == NULL)
				return -ENOMEM;

			(*sysnums)[j] = new_sysnums[i];

			/* The last item is the terminator.  */
			(*sysnums)[j + 1].value = PR_void;
		}
		else
			/* The sysnum is already filtered, merge the
			 * flags.  */
			(*sysnums)[j].flags |= new_sysnums[i].flags;
	}

	return 0;
}

static void find_libc_exec_maps(){
	LOGD("find_libc_exec_maps-----")
	FILE *fp;
    if(NULL == (fp = fopen("/proc/self/maps", "r")))
	{
		LOGE("fopen /proc/self/maps failed")
		return;
	}
	char line[2048];
	unsigned long offset = 0;
	while (fgets(line, sizeof(line), fp) != NULL) {
		unsigned long base, end, inode;
		base = end = inode = 0;
		int length = 0;

		char perms[5] = { 0, };
		if (sscanf (line,
					"%lx-%lx "
					"%4c "
					"%lx %*s %ld"
					"%n",
					&base,  &end,
					perms,
					&offset, &inode,
					&length) != 5) continue;

		if (perms[2] != 'x') continue;
		if (inode != 0) {
			char *p = strchr (line + length, '/');
			if (p != NULL) {
				*strchr(p, '\n') = '\0';
//				LOGD("zzz p:%s",p)
				if (strstr(p, "/libc.so") != NULL){
                    libc_start = base;
                    libc_end = end;
                    break;
                }
			}
		}
	}
    LOGI("find_libc_exec_maps libc start:%lx,end:%lx",libc_start,libc_end);
}

/**
 * Tell the kernel to trace only syscalls handled by PRoot and its
 * extensions.  This filter will be enabled for the given @tracee and
 * all of its future children.  This function returns -errno if an
 * error occurred, otherwise 0.
 */
int enable_syscall_filtering(const Tracee *tracee,FilteredSysnum* add_filtered_sysnums,bool exclude_libc)
{
    s_exclude_libc = exclude_libc;
	LOGD("enable_syscall_filtering,%d",s_exclude_libc);
	if (s_exclude_libc){
		find_libc_exec_maps();
	}
	FilteredSysnum *filtered_sysnums = NULL;
//	Extension *extension;
	int status;

//	assert(tracee != NULL && tracee->ctx != NULL);
	assert(tracee != NULL);
    LOGSECOMP("secomp: enable_syscall_filtering add print:")
    for (int i = 0; add_filtered_sysnums[i].value != PR_void; i++) {
        LOGSECOMP("secomp: enable_syscall_filtering add,%d,%d",i,add_filtered_sysnums[i].value)
    }
    LOGSECOMP("secomp: enable_syscall_filtering add print end")

    bool sysnum_map[512] = {0};
    int count = 0;
    for (int i = 0; add_filtered_sysnums[i].value != PR_void; i++) {
        sysnum_map[add_filtered_sysnums[i].value] = 1;
        count++;
    }
    FilteredSysnum internal_sysnums[] = {
            { PR_ptrace,		FILTER_SYSEXIT },
            { PR_wait4,		FILTER_SYSEXIT },
            { PR_waitpid,		FILTER_SYSEXIT },
            { PR_execve,		FILTER_SYSEXIT },
            { PR_execveat,		FILTER_SYSEXIT },
			{PR_readlinkat,FILTER_SYSEXIT},
    };

    int max_size = count + sizeof(internal_sysnums)/
                           sizeof(FilteredSysnum) + 1;
    FilteredSysnum * filtered_add_internal_sysnums =  vl_new_array(sizeof(FilteredSysnum),max_size); //include PR_void
    for (int i = 0; i < count; ++i) {
        filtered_add_internal_sysnums[i].value = add_filtered_sysnums[i].value;
        filtered_add_internal_sysnums[i].flags = add_filtered_sysnums[i].flags;
    }
    LOGSECOMP("secomp: count:%d",count)
    for (int i = 0; i < sizeof(internal_sysnums)/sizeof(FilteredSysnum); ++i) {
        if (!sysnum_map[internal_sysnums[i].value]){
            filtered_add_internal_sysnums[count].value = internal_sysnums[i].value;
            filtered_add_internal_sysnums[count].flags = internal_sysnums[i].flags;
            LOGSECOMP("secomp: count:%d,i:%d,val:%d,flag:%lu",count,i,internal_sysnums[i].value,internal_sysnums[i].flags)
            count++;
        }
    }
    filtered_add_internal_sysnums[count].value = PR_void;
    filtered_add_internal_sysnums[count].value = 0;
	/* Add the sysnums required by PRoot to the list of filtered
	 * sysnums.  TODO: only if path translation is required.  */
//	status = merge_filtered_sysnums(tracee->ctx, &filtered_sysnums, proot_sysnums);
    LOGSECOMP("secomp: enable_syscall_filtering add2 print:")
    for (int i = 0; filtered_add_internal_sysnums[i].value != PR_void; i++) {
        LOGSECOMP("secomp: enable_syscall_filtering add2,%d,%d",i,filtered_add_internal_sysnums[i].value)
    }
    LOGSECOMP("secomp: enable_syscall_filtering add2 print end")
	status = merge_filtered_sysnums(&filtered_sysnums, filtered_add_internal_sysnums);
	if (status < 0){
        vl_array_free(filtered_add_internal_sysnums);
        LOGE("secomp: enable_syscall_filtering fail 1,%d",status)
		return status;
	}


	/* Merge the sysnums required by the extensions to the list
	 * of filtered sysnums.  */
//	if (tracee->extensions != NULL) {
//		LIST_FOREACH(extension, tracee->extensions, link) {
//			if (extension->filtered_sysnums == NULL)
//				continue;
//
//			status = merge_filtered_sysnums(tracee->ctx, &filtered_sysnums,
//							extension->filtered_sysnums);
//			if (status < 0)
//				return status;
//		}
//	}

	status = set_seccomp_filters(filtered_sysnums);
    vl_array_free(filtered_add_internal_sysnums);
    if (status < 0){
		LOGE("secomp: enable_syscall_filtering fail 2,%d",status)
		return status;
	}

	LOGD("secomp: enable_syscall_filtering success")
	return 0;
}

#else

#include "tracee/tracee.h"
#include "attribute.h"

int enable_syscall_filtering(const Tracee *tracee UNUSED)
{
	return 0;
}

#endif /* defined(HAVE_SECCOMP_FILTER) */
