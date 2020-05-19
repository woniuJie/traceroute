#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <malloc.h>
#include "traceroute.h"
#include <pthread.h>

#define OUTPUT_LENGTH  10000

#define TAG "traceroute-jni" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型

//当前的JVM
JavaVM *g_jvm;
// java调用此jni的类
static jobject j_java_cls;
// java传递过来的参数数组
static jobjectArray j_argvs;
// 用-3标识自定义异常
int exec_status = -3;
// 执行traceroute的线程
pthread_t trace_thread;


// 动态链接库时，jvm加载so是第一个调用的方法，可在此方法动态注解jni接口
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    // 缓存JavaVM指针
    g_jvm = vm;
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6)) {
        LOGE("Could not get JNIEnv*");
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}

// 抛出异常工具方法
void JNU_ThrowByName(JNIEnv *env, const char *name, const char *msg) {
    // 查找异常类
    jclass cls = (*env)->FindClass(env, name);
    /* 如果这个异常类没有找到，VM会抛出一个NowClassDefFoundError异常 */
    if (cls != NULL) {
        (*env)->ThrowNew(env, cls, msg);  // 抛出指定名字的异常
    }
    /* 释放局部引用 */
    (*env)->DeleteLocalRef(env, cls);
}

// 获取JNIEnv*的工具方法
JNIEnv *JNU_GetEnv() {
    JNIEnv *env;
    (*g_jvm)->GetEnv(g_jvm, (void **) &env, JNI_VERSION_1_6);
    return env;
}

// 调用java的clearResult()方法
void call_java_clear_result() {
    JNIEnv *env = JNU_GetEnv();
    jclass j_cls = (*env)->GetObjectClass(env, j_java_cls);
    jmethodID j_clear_result = (*env)->GetMethodID(env, j_cls, "clearResult", "()V");
    (*env)->CallVoidMethod(env, j_java_cls, j_clear_result);
}

// 调用java的appendResult方法, 回调traceroute结果
void call_java_append_result(const char *text) {
    JNIEnv *jniEnv = JNU_GetEnv();
    jclass j_cls = (*jniEnv)->GetObjectClass(jniEnv, j_java_cls);
    jmethodID j_append_result = (*jniEnv)->GetMethodID(jniEnv, j_cls, "appendResult",
                                                       "(Ljava/lang/String;)V");
    jstring message = (*jniEnv)->NewStringUTF(jniEnv, text);
    (*jniEnv)->CallVoidMethod(jniEnv, j_java_cls, j_append_result, message);
    (*jniEnv)->DeleteLocalRef(jniEnv, message);
}

// 重载printf函数
int printf(const char *fmt, ...) {
    va_list argptr;
    int cnt;
    va_start(argptr, fmt);
    char *buffer = (char *) malloc(OUTPUT_LENGTH);
    memset(buffer, OUTPUT_LENGTH, 0);
    cnt = vsnprintf(buffer, OUTPUT_LENGTH, fmt, argptr);
    buffer[cnt] = '\0';
    call_java_append_result(buffer);
    free(buffer);
    va_end(argptr);
    return 1;
}

// 重载fprintf打印错误日志
int fprintf(FILE *fp, const char *fmt, ...) {
    va_list argptr;
    int cnt;
    va_start(argptr, fmt);
    char *buffer = (char *) malloc(OUTPUT_LENGTH);
    memset(buffer, OUTPUT_LENGTH, 0);
    cnt = vsnprintf(buffer, OUTPUT_LENGTH, fmt, argptr);
    buffer[cnt] = '\0';
    LOGE("traceroute error message(fprintf): %s", buffer);
    free(buffer);
    va_end(argptr);
    return 1;
}

// 重载vfprintf打印错误日志
int vfprintf(FILE *fp, const char *fmt, va_list args) {
    int cnt;
    char *buffer = (char *) malloc(OUTPUT_LENGTH);
    memset(buffer, OUTPUT_LENGTH, 0);
    cnt = vsnprintf(buffer, OUTPUT_LENGTH, fmt, args);
    buffer[cnt] = '\0';
    LOGE("traceroute error message(vfprintf): %s", buffer);
    free(buffer);
    return 1;
}

// 重载perror打印错误日志
void perror(const char *msg) {
    LOGE("traceroute error message(perror): %s", msg);
}

// 重载exit，不让进程退出，进而不让jvm退出
void exit(int status) {
    // 在vivo x7上断网执行的时候，exec执行会导致中断，走到此方法，导致没有DetachCurrentThread发生崩溃
    (*g_jvm)->DetachCurrentThread(g_jvm);
    // 异常时重置状态
    exec_status = -3;
    LOGE("traceroute error to exit program, status:%d", status);
    pthread_exit(0);
}

// 执行traceroute的实际方法，在c子线程中执行
void * do_execute(void * args) {
    // 获取JNIEnv，JNIEnv指针仅在创建它的线程有效。
    JNIEnv *jniEnv;
    jint status = (*g_jvm)->AttachCurrentThread(g_jvm, (void **) &jniEnv, NULL);
    if (status != 0) {
        LOGE("AttachCurrentThread failed");
        return NULL;
    }
    jniEnv = JNU_GetEnv();

    // 得到参数数组长度
    jint size = (*jniEnv)->GetArrayLength(jniEnv, j_argvs);
    LOGD("command size:%d", size);

    // 将jobjectArray转换为char *argv[]，转换参数
    char *argv[size];
    int i;
    for (i = 0; i < size; i++) {
        jstring command = (jstring) ((*jniEnv)->GetObjectArrayElement(jniEnv, j_argvs, i));
        argv[i] = (char *) ((*jniEnv)->GetStringUTFChars(jniEnv, command, 0));
        LOGD("command %d = %s", i, argv[i]);
    }
    // 清除单例缓存的回调结果
    call_java_clear_result();

    // 执行traceroute
    exec_status = exec(size, argv);
    LOGD("execute command result:%d", exec_status);
    (*g_jvm)->DetachCurrentThread(g_jvm);
    return NULL;
}

JNIEXPORT jint JNICALL Java_com_chuchujie_core_traceroute_TraceRoute_execute
        (JNIEnv *env, jobject jthis, jobjectArray jarray) {

//    异常处理，流程：检查->处理->清除
//    jthrowable throwable = NULL;
//    throwable = (*env)->ExceptionOccurred(env);
//    if (throwable) {
//        (*env)->ExceptionDescribe(env);
//        (*env)->ExceptionClear(env);
//    }

    LOGD("start traceroute");
    // 保存为全局变量
    j_java_cls = (*env)->NewGlobalRef(env, jthis);
    j_argvs = (*env)->NewGlobalRef(env, jarray);

    /*
     * https://blog.csdn.net/hmxz2nn/article/details/80041476
     *
     * 1.子线程使用return退出，主线程中使用pthread_join回收线程。
     * 2.子线程使用pthread_exit退出，主线程中使用pthread_join接收pthread_exit的返回值，并回收线程。
     * 3.主线程中调用pthread_cancel,然后调用pthread_join回收线程。
     */

    // 创建c线程执行do_execute方法
    pthread_create(&trace_thread, NULL, do_execute, NULL);
    // 等待trace_thread线程执行完成
    pthread_join(trace_thread, NULL);

    // 删除全局变量
    (*env)->DeleteGlobalRef(env, j_java_cls);
    (*env)->DeleteGlobalRef(env, j_argvs);

    LOGD("finish traceroute, status:%d", exec_status);
    return exec_status;
}
