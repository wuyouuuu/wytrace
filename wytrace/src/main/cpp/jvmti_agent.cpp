//
// Created by 周俊佑 on 2023/7/12.
//
#include "jni.h"
#include "jvmti.h"
#include <android/log.h>
#include <cstring>


jvmtiEnv *CreateJvmtiEnv(JavaVM *vm) {
    jvmtiEnv *jvmti_env;
    jint result = vm->GetEnv((void **) &jvmti_env, JVMTI_VERSION_1_2);
    if (result != JNI_OK) {
        return nullptr;
    }

    return jvmti_env;

}

void MethodEntry(jvmtiEnv *jvmti_env,
                 JNIEnv *jni_env,
                 jthread thread,
                 jmethodID method) {

}


extern "C" {

JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM *vm, char *options,
                                      void *reserved) {

    jvmtiEnv *jvmti_env = CreateJvmtiEnv(vm);
    jvmtiCapabilities caps;
    jvmtiError jverror = jvmti_env->GetPotentialCapabilities(&caps);
    jverror = jvmti_env->AddCapabilities(&caps);
    if (jverror != JVMTI_ERROR_NONE) {
        return jverror;
    }
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.MethodEntry = &MethodEntry;
    int error = jvmti_env->SetEventCallbacks(&callbacks, sizeof(callbacks));
    error = jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, nullptr);
    __android_log_print(ANDROID_LOG_ERROR, "arthack", "Agent_OnAttach %d", error);

    return JNI_OK;


}
}