#pragma once
#include "jni.h"
#include <cstdint>
// Copyright (c) 2026 Pixelforge Ports contributors
// Exact declarations read from the supplied APK. No Android code included.
#if defined(__arm__)
#define GUEST_ABI __attribute__((pcs("aapcs")))
#else
#define GUEST_ABI
#endif
namespace donor {
// Lorg/cocos2dx/lib/Cocos2dxAccelerometer; onSensorChanged(FFFJ)V
using Cocos2dxAccelerometer_onSensorChanged_0 = void (GUEST_ABI *)(JNIEnv *, jclass ,jfloat, jfloat, jfloat, jlong);
inline constexpr const char *Cocos2dxAccelerometer_onSensorChanged_0_symbol = "Java_org_cocos2dx_lib_Cocos2dxAccelerometer_onSensorChanged";
// Lorg/cocos2dx/lib/Cocos2dxActivity; GetConfig(Ljava/lang/String;Ljava/lang/String;)Z
using Cocos2dxActivity_GetConfig_1 = jboolean (GUEST_ABI *)(JNIEnv *, jclass ,jstring, jstring);
inline constexpr const char *Cocos2dxActivity_GetConfig_1_symbol = "Java_org_cocos2dx_lib_Cocos2dxActivity_GetConfig";
// Lorg/cocos2dx/lib/Cocos2dxActivity; SetControlInVisible()V
using Cocos2dxActivity_SetControlInVisible_2 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *Cocos2dxActivity_SetControlInVisible_2_symbol = "Java_org_cocos2dx_lib_Cocos2dxActivity_SetControlInVisible";
// Lorg/cocos2dx/lib/Cocos2dxActivity; SetControlVisible()V
using Cocos2dxActivity_SetControlVisible_3 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *Cocos2dxActivity_SetControlVisible_3_symbol = "Java_org_cocos2dx_lib_Cocos2dxActivity_SetControlVisible";
// Lorg/cocos2dx/lib/Cocos2dxActivity; nativeSetDensityScaleValue(F)V
using Cocos2dxActivity_nativeSetDensityScaleValue_4 = void (GUEST_ABI *)(JNIEnv *, jclass ,jfloat);
inline constexpr const char *Cocos2dxActivity_nativeSetDensityScaleValue_4_symbol = "Java_org_cocos2dx_lib_Cocos2dxActivity_nativeSetDensityScaleValue";
// Lorg/cocos2dx/lib/Cocos2dxActivity; nativeSetIsGoogleLauncherBuild(Z)V
using Cocos2dxActivity_nativeSetIsGoogleLauncherBuild_5 = void (GUEST_ABI *)(JNIEnv *, jclass ,jboolean);
inline constexpr const char *Cocos2dxActivity_nativeSetIsGoogleLauncherBuild_5_symbol = "Java_org_cocos2dx_lib_Cocos2dxActivity_nativeSetIsGoogleLauncherBuild";
// Lorg/cocos2dx/lib/Cocos2dxActivity; nativeSetNumOfCPUCores(I)V
using Cocos2dxActivity_nativeSetNumOfCPUCores_6 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint);
inline constexpr const char *Cocos2dxActivity_nativeSetNumOfCPUCores_6_symbol = "Java_org_cocos2dx_lib_Cocos2dxActivity_nativeSetNumOfCPUCores";
// Lorg/cocos2dx/lib/Cocos2dxActivity; nativeSetPackageName(Ljava/lang/String;)V
using Cocos2dxActivity_nativeSetPackageName_7 = void (GUEST_ABI *)(JNIEnv *, jclass ,jstring);
inline constexpr const char *Cocos2dxActivity_nativeSetPackageName_7_symbol = "Java_org_cocos2dx_lib_Cocos2dxActivity_nativeSetPackageName";
// Lorg/cocos2dx/lib/Cocos2dxActivity; nativeSetPaths(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
using Cocos2dxActivity_nativeSetPaths_8 = void (GUEST_ABI *)(JNIEnv *, jclass ,jstring, jstring, jstring);
inline constexpr const char *Cocos2dxActivity_nativeSetPaths_8_symbol = "Java_org_cocos2dx_lib_Cocos2dxActivity_nativeSetPaths";
// Lorg/cocos2dx/lib/Cocos2dxBitmap; nativeInitBitmapDC(II[B)V
using Cocos2dxBitmap_nativeInitBitmapDC_9 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint, jint, jbyteArray);
inline constexpr const char *Cocos2dxBitmap_nativeInitBitmapDC_9_symbol = "Java_org_cocos2dx_lib_Cocos2dxBitmap_nativeInitBitmapDC";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeDeleteBackward()V
using Cocos2dxRenderer_nativeDeleteBackward_10 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *Cocos2dxRenderer_nativeDeleteBackward_10_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeDeleteBackward";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeGetContentText()Ljava/lang/String;
using Cocos2dxRenderer_nativeGetContentText_11 = jstring (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *Cocos2dxRenderer_nativeGetContentText_11_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeGetContentText";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeInit(II)V
using Cocos2dxRenderer_nativeInit_12 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint, jint);
inline constexpr const char *Cocos2dxRenderer_nativeInit_12_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeInsertText(Ljava/lang/String;)V
using Cocos2dxRenderer_nativeInsertText_13 = void (GUEST_ABI *)(JNIEnv *, jclass ,jstring);
inline constexpr const char *Cocos2dxRenderer_nativeInsertText_13_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInsertText";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeKeyDown(I)Z
using Cocos2dxRenderer_nativeKeyDown_14 = jboolean (GUEST_ABI *)(JNIEnv *, jclass ,jint);
inline constexpr const char *Cocos2dxRenderer_nativeKeyDown_14_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeKeyDown";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeKeyUp(I)Z
using Cocos2dxRenderer_nativeKeyUp_15 = jboolean (GUEST_ABI *)(JNIEnv *, jclass ,jint);
inline constexpr const char *Cocos2dxRenderer_nativeKeyUp_15_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeKeyUp";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeOnPause()V
using Cocos2dxRenderer_nativeOnPause_16 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *Cocos2dxRenderer_nativeOnPause_16_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnPause";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeOnResume()V
using Cocos2dxRenderer_nativeOnResume_17 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *Cocos2dxRenderer_nativeOnResume_17_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnResume";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeRender()V
using Cocos2dxRenderer_nativeRender_18 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *Cocos2dxRenderer_nativeRender_18_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeRender";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeTouchesBegin(IFF)V
using Cocos2dxRenderer_nativeTouchesBegin_19 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint, jfloat, jfloat);
inline constexpr const char *Cocos2dxRenderer_nativeTouchesBegin_19_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesBegin";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeTouchesCancel([I[F[F)V
using Cocos2dxRenderer_nativeTouchesCancel_20 = void (GUEST_ABI *)(JNIEnv *, jclass ,jintArray, jfloatArray, jfloatArray);
inline constexpr const char *Cocos2dxRenderer_nativeTouchesCancel_20_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesCancel";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeTouchesEnd(IFF)V
using Cocos2dxRenderer_nativeTouchesEnd_21 = void (GUEST_ABI *)(JNIEnv *, jclass ,jint, jfloat, jfloat);
inline constexpr const char *Cocos2dxRenderer_nativeTouchesEnd_21_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesEnd";
// Lorg/cocos2dx/lib/Cocos2dxRenderer; nativeTouchesMove([I[F[F)V
using Cocos2dxRenderer_nativeTouchesMove_22 = void (GUEST_ABI *)(JNIEnv *, jclass ,jintArray, jfloatArray, jfloatArray);
inline constexpr const char *Cocos2dxRenderer_nativeTouchesMove_22_symbol = "Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeTouchesMove";
// Lorg/cocos2dx/lib/Cocos2dxVideo; onVideoCompleted()V
using Cocos2dxVideo_onVideoCompleted_23 = void (GUEST_ABI *)(JNIEnv *, jclass);
inline constexpr const char *Cocos2dxVideo_onVideoCompleted_23_symbol = "Java_org_cocos2dx_lib_Cocos2dxVideo_onVideoCompleted";
// Lorg/ubisoft/InApp/InAppHandler; purchaseSuccessful(Ljava/lang/String;)I
using InAppHandler_purchaseSuccessful_24 = jint (GUEST_ABI *)(JNIEnv *, jobject ,jstring);
inline constexpr const char *InAppHandler_purchaseSuccessful_24_symbol = "Java_org_ubisoft_InApp_InAppHandler_purchaseSuccessful";
}
#undef GUEST_ABI
