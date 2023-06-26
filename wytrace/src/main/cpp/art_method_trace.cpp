#include <string>
#include "jni.h"
#include "shadowhook.h"
#include <stack>
#include <array>
#include <unistd.h>
#include <android/log.h>


#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"wytrace",__VA_ARGS__)

class ArtMethod {
public:
    typedef std::string(*PrettyMethodType)(ArtMethod *thiz, bool with_signature);

    inline std::string PrettyMethod(ArtMethod *thiz, bool with_signature) {
        if (thiz == nullptr)
            return "null";
        else if (PrettyMethodSym)
            return PrettyMethodSym(thiz, with_signature);
        else
            return "null sym";
    }

    typedef const char *(*GetMethodShortyType)(JNIEnv *env, jmethodID mid);

    const char *(*GetMethodShortySym)(JNIEnv *env, jmethodID mid);

    const char *GetMethodShorty(JNIEnv *env, jmethodID mid) {
        if (GetMethodShortySym)
            return GetMethodShortySym(env, mid);
        return nullptr;
    }

    static bool Init(void *handler) {
        return !(ArtMethod::PrettyMethodSym = reinterpret_cast<ArtMethod::PrettyMethodType>(shadowhook_dlsym(handler, "_ZN3art9ArtMethod12PrettyMethodEPS0_b"))) &&
               !(ArtMethod::PrettyMethodSym = reinterpret_cast<ArtMethod::PrettyMethodType>(shadowhook_dlsym(handler, "_ZN3art12PrettyMethodEPNS_9ArtMethodEb"))) &&
               !(ArtMethod::PrettyMethodSym = reinterpret_cast<ArtMethod::PrettyMethodType>(shadowhook_dlsym(handler, "_ZN3art12PrettyMethodEPNS_6mirror9ArtMethodEb")));
    }

private:
    static std::string (*PrettyMethodSym)(ArtMethod *thiz, bool with_signature);

};

ArtMethod::PrettyMethodType ArtMethod::PrettyMethodSym = nullptr;

void (*SetJavaDebuggable)(bool debug);

void *(*ATrace_beginSection)(const char *sectionName);

void *(*ATrace_endSection)(void);


void method_before(ArtMethod *artMethod, bool &trace, long &start, std::string &method, timeval &tv);

void method_after(bool trace, long start, const std::string &method, timeval &tv);

typedef void *(*fp_ATrace_beginSection)(const char *sectionName);

typedef void *(*fp_ATrace_endSection)(void);

typedef void (*executeSwitchImplAsm_type)(void *, void *, void *);

struct shadowframet {
    void *link_;
    void *method_;
};

struct SwitchImplContext {
    void *self;
    void *accessor;
    shadowframet *shadow_frame;
};
void *executeSwitchImplAsm_orig = NULL;
void *executeSwitchImplAsm_stub = NULL;

void *executeMterpImpl_orig = NULL;
void *executeMterpImpl_stub = NULL;


thread_local std::stack<std::string> words;
thread_local std::stack<bool> key;
static std::string filter_key = "com.wy";
static int filter_tid = -1;
static int filter_depth = 10;
static bool filter_debug = false;


void method_before(ArtMethod *artMethod, bool &trace, long &start, std::string &method, timeval &tv) {
    trace = false;
    start = 0l;
    method = "";//    0 == gettimeofday(&tv, nullptr) && (tv.tv_sec * 1000 + tv.tv_usec / 1000) % 100 >= 0
    if (filter_tid < 0 || gettid() == filter_tid) {
        method = artMethod->PrettyMethod(artMethod, false);
        if (key.empty() && method.find(filter_key) != std::string::npos) {
            key.push(true);
        }
        trace = (words.size() < filter_depth) && !key.empty();
        if (trace) {
            words.push(method.c_str());
            ATrace_beginSection(method.c_str());
            if (filter_debug && 0 == gettimeofday(&tv, nullptr)) {
                start = tv.tv_sec * 1000 + tv.tv_usec / 1000;
            }
        }
    }
}

void method_after(bool trace, long start, const std::string &method, timeval &tv) {
    if (trace) {
        long word_size = words.size();
        if (word_size == 1 && !key.empty()) {
            key.pop();
        }
        if (filter_debug && 0 == gettimeofday(&tv, nullptr)) {
            long cost = tv.tv_sec * 1000 + tv.tv_usec / 1000 - start;
            LOGE("%lu, %s %lu ms", word_size, method.c_str(), cost);

        }
        words.pop();
        ATrace_endSection();
    }
}


void executeSwitchImplAsm_proxy(SwitchImplContext *ctx, void *a, void *jvalue) {
    // do something
    ArtMethod *artMethod = reinterpret_cast<ArtMethod *>(ctx->shadow_frame->method_);
    bool trace;
    long start;
    std::string method;
    struct timeval tv;
    method_before(artMethod, trace, start, method, tv);
    ((executeSwitchImplAsm_type) executeSwitchImplAsm_orig)(ctx, a, jvalue);
    method_after(trace, start, method, tv);

}


typedef bool (*type_t2)(void *, void *, void *a, void *b);

bool executeMterpImpl_proxy(void *thread, void *shadowframe, void *a, void *b) {
    struct shadowframet *shadowframet1 = (shadowframet *) (a);
    ArtMethod *artMethod = reinterpret_cast<ArtMethod *>(shadowframet1->method_);
//    LOGE("%s", artMethod->PrettyMethod(artMethod,false).c_str());
    bool trace;
    long start;
    std::string method;
    struct timeval tv;
    method_before(artMethod, trace, start, method, tv);
    bool res = ((type_t2) executeMterpImpl_orig)(thread, shadowframe, a, b);
    method_after(trace, start, method, tv);
    return res;


}

bool hook_success;

void do_hook() {
    void *handler = shadowhook_dlopen("libart.so");
    if(handler == nullptr){
        LOGE("hook error  dlopen libart failed");
        return;
    }
    ArtMethod::Init(handler);
    shadowhook_dlclose(handler);
    void *libandroid = shadowhook_dlopen("libandroid.so");
    if(libandroid == nullptr){
        LOGE("hook error  dlopen libandroid failed");
        return;
    }
    ATrace_beginSection = reinterpret_cast<fp_ATrace_beginSection>(
            shadowhook_dlsym(libandroid, "ATrace_beginSection"));
    ATrace_endSection = reinterpret_cast<fp_ATrace_endSection>(
            shadowhook_dlsym(libandroid, "ATrace_endSection"));

    shadowhook_dlclose(libandroid);
    executeSwitchImplAsm_stub = shadowhook_hook_sym_name(
            "libart.so",
            "ExecuteSwitchImplAsm",
            (void *) executeSwitchImplAsm_proxy,
            (void **) &executeSwitchImplAsm_orig);
    executeMterpImpl_stub = shadowhook_hook_sym_name(
            "libart.so",
            "ExecuteMterpImpl",
            (void *) executeMterpImpl_proxy,
            (void **) &executeMterpImpl_orig);
    if (executeSwitchImplAsm_stub == NULL || executeMterpImpl_stub == NULL) {
        int err_num = shadowhook_get_errno();
        const char *err_msg = shadowhook_to_errmsg(err_num);
        LOGE("hook error %d - %s", err_num, err_msg);
    } else {
        LOGE("hook success10");
        hook_success = true;
    }

}

extern "C" {


JNIEXPORT  void JNICALL
Java_com_wy_lib_wytrace_ArtMethodTrace_methodHook(JNIEnv *env,
                                                                jclass clazz, jstring methodName, jint tid, jint depth, jboolean debug) {
    filter_tid = tid;
    filter_depth = depth;
    filter_debug = debug;
    const char *c_method_name = env->GetStringUTFChars(methodName, nullptr);
    filter_key = std::string(c_method_name);
    do_hook();
    (env)->ReleaseStringUTFChars(methodName, c_method_name);

}


JNIEXPORT  void JNICALL
Java_com_wy_lib_wytrace_ArtMethodTrace_methodUnHook(JNIEnv *env,jclass clazz) {
    if (!hook_success) {
        return;
    }
    if (executeMterpImpl_stub != NULL) {
        int err1 = shadowhook_unhook(executeMterpImpl_stub);
        LOGE("unhook executeMterpImpl_stub %d ", err1);

    }
    if (executeSwitchImplAsm_stub != NULL) {
        int err2 = shadowhook_unhook(executeSwitchImplAsm_stub);
        LOGE("unhook executeMterpImpl_stub %d ", err2);
        hook_success = false;
    }

}


}
















