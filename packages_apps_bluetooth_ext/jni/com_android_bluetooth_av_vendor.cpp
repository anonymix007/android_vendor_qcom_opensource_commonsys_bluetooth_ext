/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "BluetoothVendorA2dpSinkJni"

#include "com_android_bluetooth.h"
#include <hardware/bt_av_vendor.h>
#include "utils/Log.h"
#include "android_runtime/AndroidRuntime.h"

namespace android {
static btav_sink_vendor_interface_t *sBluetoothVendorA2dpSinkInterface = NULL;
static jobject mCallbacksObj = NULL;
static jmethodID method_onStartIndCallback;
static jmethodID method_onSuspendIndCallback;
static jmethodID method_onIsSuspendNeededCallback;

static jbyteArray marshall_bda(const RawAddress* bd_addr) {
  CallbackEnv sCallbackEnv(__func__);
  if (!sCallbackEnv.valid()) return nullptr;

  jbyteArray addr = sCallbackEnv->NewByteArray(sizeof(RawAddress));
  if (!addr) {
    ALOGE("Fail to new jbyteArray bd addr");
    return nullptr;
  }
  sCallbackEnv->SetByteArrayRegion(addr, 0, sizeof(RawAddress),
                                   (jbyte*)bd_addr);
  return addr;
}

static void StartIndCallback(const RawAddress* bd_addr) {

    ALOGI("%s", __FUNCTION__);
    CallbackEnv sCallbackEnv(__func__);
    if (!sCallbackEnv.valid()) return;

    ScopedLocalRef<jbyteArray> addr(sCallbackEnv.get(), marshall_bda(bd_addr));
    if (addr.get() == nullptr) return;
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onStartIndCallback,
                                 addr.get());
}

static void SuspendIndCallback(const RawAddress* bd_addr) {

    ALOGI("%s", __FUNCTION__);
    CallbackEnv sCallbackEnv(__func__);
    if (!sCallbackEnv.valid()) return;

    ScopedLocalRef<jbyteArray> addr(sCallbackEnv.get(), marshall_bda(bd_addr));
    if (addr.get() == nullptr) return;
    sCallbackEnv->CallVoidMethod(mCallbacksObj, method_onSuspendIndCallback,
                                 addr.get());
}

static bool IsSuspendNeededCallback(const RawAddress* bd_addr) {
    ALOGI("%s", __FUNCTION__);
    CallbackEnv sCallbackEnv(__func__);
    if (!sCallbackEnv.valid()) return false;

    ScopedLocalRef<jbyteArray> addr(sCallbackEnv.get(), marshall_bda(bd_addr));
    if (addr.get() == nullptr) return false;
    return sCallbackEnv->CallBooleanMethod(mCallbacksObj,
                         method_onIsSuspendNeededCallback, addr.get());
}

static btav_sink_vendor_callbacks_t sBluetoothVendorA2dpSinkCallbacks = {
    sizeof(sBluetoothVendorA2dpSinkCallbacks),
    StartIndCallback,
    SuspendIndCallback,
    IsSuspendNeededCallback
};

static void classInitNative(JNIEnv* env, jclass clazz) {

    method_onStartIndCallback = env->GetMethodID(clazz, "onStartIndCallback", "([B)V");
    method_onSuspendIndCallback =
                  env->GetMethodID(clazz, "onSuspendIndCallback", "([B)V");
    method_onIsSuspendNeededCallback =
                  env->GetMethodID(clazz, "onIsSuspendNeededCallback", "([B)Z");
    ALOGI("%s: succeeds", __FUNCTION__);
}

static void initNative(JNIEnv *env, jobject object) {
    const bt_interface_t* btInf;
    bt_status_t status;

    if ( (btInf = getBluetoothInterface()) == NULL) {
        ALOGE("Bluetooth module is not loaded");
        return;
    }

    if (mCallbacksObj != NULL) {
        ALOGW("Cleaning up Bluetooth Vendor callback object");
        env->DeleteGlobalRef(mCallbacksObj);
        mCallbacksObj = NULL;
    }

    if ( (sBluetoothVendorA2dpSinkInterface = (btav_sink_vendor_interface_t *)
          btInf->get_profile_interface(BT_PROFILE_A2DP_SINK_VENDOR_ID)) == NULL) {
        ALOGE("Failed to get Bluetooth Vendor Interface");
        return;
    }

    if ( (status = sBluetoothVendorA2dpSinkInterface->init(&sBluetoothVendorA2dpSinkCallbacks))
                 != BT_STATUS_SUCCESS) {
        ALOGE("Failed to initialize Bluetooth Vendor, status: %d", status);
        sBluetoothVendorA2dpSinkInterface = NULL;
        return;
    }
    mCallbacksObj = env->NewGlobalRef(object);
}

static void cleanupNative(JNIEnv *env, jobject object) {
    const bt_interface_t* btInf;

    if ( (btInf = getBluetoothInterface()) == NULL) {
        ALOGE("Bluetooth module is not loaded");
        return;
    }

    if (sBluetoothVendorA2dpSinkInterface !=NULL) {
        ALOGW("Cleaning up Bluetooth Vendor Interface...");
        sBluetoothVendorA2dpSinkInterface->cleanup();
        sBluetoothVendorA2dpSinkInterface = NULL;
    }

    if (mCallbacksObj != NULL) {
        ALOGW("Cleaning up Bluetooth Vendor callback object");
        env->DeleteGlobalRef(mCallbacksObj);
        mCallbacksObj = NULL;
    }

}

/* native interface */
static jint StartIndRspNative(JNIEnv* env, jobject thiz, jbyteArray address,
                            jboolean accepted)
{
   jbyte* addr = env->GetByteArrayElements(address, NULL);
   if (!addr) {
     jniThrowIOException(env, EINVAL);
     return JNI_FALSE;
   }

   RawAddress bd_addr;
   bd_addr.FromOctets(reinterpret_cast<const uint8_t*>(addr));
    if (sBluetoothVendorA2dpSinkInterface == NULL) {
        ALOGE("No Interface initialized");
        return JNI_FALSE;
    }

    int ret = sBluetoothVendorA2dpSinkInterface->start_ind_rsp(bd_addr, accepted);

    if (ret != 0) {
        ALOGE("%s: Failure", __func__);
        return JNI_FALSE;
    } else {
        ALOGV("%s: Success :%d", __func__, accepted);
    }

    return JNI_TRUE;
}

/* native interface */
static jint SuspendIndRspNative(JNIEnv* env, jobject thiz, jbyteArray address,
                               jboolean accepted)
{
   jbyte* addr = env->GetByteArrayElements(address, NULL);
   if (!addr) {
     jniThrowIOException(env, EINVAL);
     return JNI_FALSE;
   }

   RawAddress bd_addr;
   bd_addr.FromOctets(reinterpret_cast<const uint8_t*>(addr));
    if (sBluetoothVendorA2dpSinkInterface == NULL) {
        ALOGE("No Interface initialized");
        return JNI_FALSE;
    }

    int ret = sBluetoothVendorA2dpSinkInterface->suspend_ind_rsp(bd_addr, accepted);

    if (ret != 0) {
        ALOGE("%s: Failure", __func__);
        return JNI_FALSE;
    } else {
        ALOGV("%s: Success :%d", __func__, accepted);
    }

    return JNI_TRUE;
}

static JNINativeMethod sMethods[] = {
    {"classInitNative", "()V", (void *) classInitNative},
    {"initNative", "()V", (void *) initNative},
    {"cleanupNative", "()V", (void *) cleanupNative},
    { "StartIndRspNative", "([BZ)I", (void*)StartIndRspNative},
    { "SuspendIndRspNative", "([BZ)I", (void*)SuspendIndRspNative},
};

static bool cMethods[NELEM(sMethods)];

extern int jniRegisterNativeMethodsSafe(JNIEnv* env, const char *className, JNINativeMethod *methods, bool *cursedMethods, size_t count);

int register_com_android_bluetooth_a2dp_sink_vendor_service(JNIEnv* env)
{
    ALOGE("%s:",__FUNCTION__);
    return jniRegisterNativeMethodsSafe(env, "com/android/bluetooth/a2dpsink/A2dpSinkVendorService",
                                    sMethods, cMethods, NELEM(sMethods));
}

} /* namespace android */