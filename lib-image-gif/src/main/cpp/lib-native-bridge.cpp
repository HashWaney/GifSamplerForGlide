#include <jni.h>
#include "utils/log.h"
#include "GifDecoder.h"
#include "stream/Stream.h"

////////////////////////////////////////////////////////////////////////////////
// JNILoader
////////////////////////////////////////////////////////////////////////////////

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    if (JavaStream_OnLoad(env)) {
        ALOGE("Failed to load JavaStream");
        return -1;
    }
    if (GifDecoder_OnLoad(env)) {
        ALOGE("Failed to load GifDecoder");
        return -1;
    }
    return JNI_VERSION_1_6;
}