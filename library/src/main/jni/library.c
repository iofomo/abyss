#include <jni.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <linux/prctl.h>
#include <sys/prctl.h>

#include "library.h"
#include "constant.h"
#include "tracee/tracee.h"
#include "syscall/seccomp.h"
#include "tracee/event.h"
#include "cmn/cmn_pthread_cond.h"
#include "note.h"
#include "cmn/cmn_back_call_stack.h"
#include "syscall/syscall.h"
#include "mem.h"

#define FILTERED_FUNC_OPEN      0x1
#define FILTERED_FUNC_CLOSE     0x2

/**
 * 增加自定义要拦截的系统调用
 */
static FilteredSysnum g_add_filtered_sysnums[3] = {};

pthread_t work_tid = 0L;
pid_t snew_attach_pid = -1;
struct PCond_Context spctx;
on_sys_event_t  global_on_sysenter = NULL;
on_sys_event_t  global_on_sysexit = NULL;


static void fill_filtered_sysnum(int cur_index, int sysnum){
    g_add_filtered_sysnums[cur_index].value = sysnum;
    g_add_filtered_sysnums[cur_index].flags = 0;
}

static void mark_filtered_sysnum_end(int cur_index){
    g_add_filtered_sysnums[cur_index].value = PR_void;
    g_add_filtered_sysnums[cur_index].flags = 0;
}

/**
 * -------------- tracee start  ---------------------
 */


void test_native(){
    errno = 0;
   int fd =  open("/proc/self/maps",O_RDONLY);
    LOGI("tracee open fd,%d,err:%d,%s",fd,errno, strerror(errno));
    close(fd);
}

//
JNIEXPORT jboolean JNICALL
Java_com_iofomo_opensrc_abyss_sdk_Nativee_before_1attach(JNIEnv *env, jclass clazz) {
    if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1){
        LOGD("set dumpable error,%d,%s",errno, strerror(errno))
        return false;
    }
    return true;
}

JNIEXPORT jboolean JNICALL Java_com_iofomo_opensrc_abyss_sdk_Nativee_tracee_1init_1native
        (JNIEnv * env, jclass clazz, jint flags){
    int add_index = 0;
    mark_filtered_sysnum_end(add_index);
    if (0 != (flags & FILTERED_FUNC_OPEN)) {
        fill_filtered_sysnum(add_index,PR_openat);
        add_index++;
        mark_filtered_sysnum_end(add_index);
    }
    if (0 != (flags & FILTERED_FUNC_CLOSE)) {
        fill_filtered_sysnum(add_index,PR_close);
        add_index++;
        mark_filtered_sysnum_end(add_index);
    }
    return tracee_init(g_add_filtered_sysnums,false);
}

bool tracee_init(FilteredSysnum* add_filtered_sysnums,bool exclude_libc) {
    //Synchronize with the tracer's event loop.
    pid_t my_pid = getpid();
    Tracee* tracee = get_tracee_for_ee(my_pid);
    if (!tracee){
        LOGE("new tracee error");
        return false;
    }
    //关键操作、seccomp mode 2
    /* Improve performance by using seccomp mode 2, unless
 * this support is explicitly disabled.  */
    (void) enable_syscall_filtering(tracee,add_filtered_sysnums,exclude_libc);
    free(tracee);
    LOGI("tracee init finished")
//    test_native();
//    register_crash_handler();
    return true;
}

/**
 * -------------- tracee end  ---------------------
 */


/**
 * -------------- tracer start  ---------------------
 */
static struct PCond_Context s_work_launch_sync;
static bool s_work_thread_doing = false; //工作线程是否正在工作
struct trace_init_data{
    pid_t pid;
    struct PCond_Context* pctx;
};

static bool need_signal_launch(){
    s_work_thread_doing = true;
    return true;
}

static bool need_wait_work(){
    return !s_work_thread_doing;
}

void* work_main(void* data){
    LOGI("work_main start,cur_pid:%d",getpid())
    LOGD("handle_event called");
    cmn_pt_signal2(&s_work_launch_sync,need_signal_launch);
    LOGD("before event loop")
    int ret = event_loop();
    LOGE("event loop exit,ret:%d",ret);
    return NULL;
}

void tracer_init() {
    int code;
    cmn_pt_context_init(&s_work_launch_sync);
    cmn_pt_context_init(&spctx);
    if ((code = pthread_create(&work_tid,NULL, work_main,NULL)) != 0){
        LOGE("start work_main error,code:%d,%d,%s",code,errno,strerror(errno));
        return;
    }
}

void* trace_new_pid_internal(void* data){
    LOGD("trace_new_pid_internal,%d",getpid())
    pid_t child = fork();
    if (child == 0){ //要在work_main之后,否则这个child不能被waitpid到
        //child -- helper进程
        LOGD("enter helper进程--,%d",getpid())
       // LOGD("helper after stop")
        LOGD("exit helper进程--")
        _exit(255); //[0,255]
    }else if (child > 0){
        //parent
        LOGD("helper wait for trace pid:%d",snew_attach_pid)
        cmn_pt_wait(&spctx);
        LOGD("helper finish trace pid:%d",snew_attach_pid)
    }else{
        //error
        LOGE("trace error,%d",snew_attach_pid);
    }
    snew_attach_pid = -1;
//    int ret = 100;
    //TODO 定制返回值
    return NULL;
}


int trace_new_pid(int pid) {
    LOGD("start trace comp pid----------------%d", pid)
    if (!s_work_thread_doing){
        LOGD("wait for work thread") //!!!!
        cmn_pt_wait2(&s_work_launch_sync,need_wait_work);
        LOGD("after work thread working")
    }
    snew_attach_pid = pid;
    pthread_t tid;
    pthread_create(&tid,NULL,trace_new_pid_internal,NULL);
    if (pthread_join(tid,NULL) != 0){
        printf("failed to join,cur_pid:%d\n",getpid());
    }
    LOGD("end trace comp pid----------------%d", pid)
    return 0;
}



void JNICALL
Java_com_iofomo_opensrc_abyss_sdk_Native_init(JNIEnv *env, jclass clazz) {
    tracer_init();
}

jint JNICALL
Java_com_iofomo_opensrc_abyss_sdk_Native_trace_1pid(JNIEnv *env, jclass clazz, jint pid) {
    return trace_new_pid(pid);
}

void set_syscall_event_callback(void (*on_sysenter)(syscall_data* data),void (*on_sysexit)(syscall_data* data)){
    global_on_sysenter = on_sysenter;
    global_on_sysexit = on_sysexit;
}

word_t syscall_peek_reg(const syscall_data* sysdata,Reg reg){
    return peek_reg(sysdata->_internal,CURRENT,reg);
}

void syscall_poke_reg(const syscall_data* sysdata, Reg reg, word_t value){
    poke_reg(sysdata->_internal,reg,value);
}

int syscall_set_sysarg_data(const syscall_data* sysdata,const void *tracer_ptr, word_t data_size, Reg reg){
    return set_sysarg_data(sysdata->_internal,tracer_ptr,data_size,reg);
}


int syscall_set_sysarg_str(const syscall_data* sysdata,const char* tracer_ptr, Reg reg){
    return set_sysarg_data(sysdata->_internal, tracer_ptr, strlen(tracer_ptr) + 1, reg);
}

int syscall_get_sysarg_data(const syscall_data* sysdata, char* dest_tracer,word_t max_size, Reg reg){
    int size;
    word_t src;
    const Tracee *tracee = sysdata->_internal;
    src = peek_reg(tracee, CURRENT, reg);
    if (src == 0) {
        return 0;
    }
    /* Get the str from the tracee's memory space. */
    size = read_data(tracee,dest_tracer,src,max_size);
    return size;
}

int syscall_get_sysarg_str(const syscall_data* sysdata, char* dest_tracer,word_t max_size, Reg reg){
    int size;
    word_t src;
    const Tracee *tracee = sysdata->_internal;
    src = peek_reg(tracee, CURRENT, reg);
    if (src == 0) {
        dest_tracer[0] = '\0';
        return 0;
    }
    /* Get the str from the tracee's memory space. */
    size = read_string(tracee,dest_tracer,src,max_size);
    if (size < 0)
        return size;
    dest_tracer[size] = '\0';
    return size;
}

int syscall_write_data(const syscall_data* sysdata, word_t dest_tracee, const void *src_tracer, word_t size){
   return write_data(sysdata->_internal,dest_tracee,src_tracer,size);
}

//int syscall_syscall_read_data(const syscall_data sysdata, void *dest_tracer, word_t src_tracee, word_t size){
//    return read_data(sysdata._internal,dest_tracer,src_tracee,size);
//}

//int syscall_read_string(const syscall_data sysdata, char *dest_tracer, word_t src_tracee, word_t max_size){
//    return read_string(sysdata._internal,dest_tracer,src_tracee,max_size);
//}



/**
 * -------------- tracer end  ---------------------
 */