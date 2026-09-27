// Copyright (c) 2026 Pixelforge Ports contributors
#include "jni_internals.h"
#include "app_exit.h"
#include "fix_path.h"
#include "so_util.h"
#include "audio.h"
#include "native_bindings.h"
#include <zlib.h>
#include <cstdio>
#include <cstring>

extern Class activity_class,renderer_class;
so_module *port_guest_module();
so_module *port_cocos_module();

static jstring language(JNIEnv *env,jclass) {return env->NewStringUTF("en");}
static jstring device(JNIEnv *env,jclass) {return env->NewStringUTF("R800i");}
static void message(JNIEnv *env,jclass,jstring title,jstring text) {
    const char *a=title?env->GetStringUTFChars(title,nullptr):"";
    const char *b=text?env->GetStringUTFChars(text,nullptr):"";
    fprintf(stderr,"Prince of Persia message: %s: %s\n",a,b);
    if(title)env->ReleaseStringUTFChars(title,a);
    if(text)env->ReleaseStringUTFChars(text,b);
}
static void idle(JNIEnv*,jclass) {}
static jint zero(JNIEnv*,jclass) {return 0;}
static void animation(JNIEnv*,jclass,jdouble) {}
static bool video_pending=false;
static void video(JNIEnv *env,jclass,jstring path,jboolean,jboolean) {
    const char *name=path?env->GetStringUTFChars(path,nullptr):"";
    fprintf(stderr,"Prince of Persia: skipping video %s\n",name);
    if(path)env->ReleaseStringUTFChars(path,name);
    video_pending=true;
}
void pop_video_tick(JNIEnv *env) {
    if(!video_pending)return;
    video_pending=false;
    for(auto *module:{port_guest_module(),port_cocos_module()}) {
        auto address=so_symbol(module,donor::Cocos2dxVideo_onVideoCompleted_23_symbol);
        if(address) {
            reinterpret_cast<donor::Cocos2dxVideo_onVideoCompleted_23>(address)(env,nullptr);
            return;
        }
    }
    fprintf(stderr,"Prince of Persia: video completion callback missing\n");
}
void pop_text_bitmap(JNIEnv*,jclass,jstring,jstring,jint,jint,jint,jint);

static void setup(Class &clazz,const char *path,const ManagedMethod *methods) {
    clazz.classpath=path;
    clazz.classname=strrchr(path,'/')+1;
    clazz.managed_methods=methods;
    clazz.instance_size=0;
    ClassRegistry::register_class(clazz);
}
#define M(c,f,n,s) ManagedMethod::RegisterStatic<f>(c,n,s)
void register_pop_services() {
    static const ManagedMethod activity[]={
        M(activity_class,language,"getCurrentLanguage","()Ljava/lang/String;"),
        M(activity_class,device,"getDeviceName","()Ljava/lang/String;"),
        M(activity_class,message,"showMessageBox","(Ljava/lang/String;Ljava/lang/String;)V"),
        M(activity_class,pop_audio_play_music,"playBackgroundMusic","(Ljava/lang/String;Z)V"),
        M(activity_class,pop_audio_stop_music,"stopBackgroundMusic","()V"),
        M(activity_class,pop_audio_pause_music,"pauseBackgroundMusic","()V"),
        M(activity_class,pop_audio_resume_music,"resumeBackgroundMusic","()V"),
        M(activity_class,pop_audio_rewind_music,"rewindBackgroundMusic","()V"),
        M(activity_class,pop_audio_music_playing,"isBackgroundMusicPlaying","()Z"),
        M(activity_class,pop_audio_music_volume,"getBackgroundMusicVolume","()F"),
        M(activity_class,pop_audio_set_music_volume,"setBackgroundMusicVolume","(F)V"),
        M(activity_class,pop_audio_preload_music,"preloadBackgroundMusic","(Ljava/lang/String;)V"),
        M(activity_class,pop_audio_play_effect,"playEffect","(Ljava/lang/String;Z)I"),
        M(activity_class,pop_audio_stop_effect,"stopEffect","(I)V"),
        M(activity_class,pop_audio_stop_effects,"stopAllEffects","()V"),
        M(activity_class,pop_audio_pause_effect,"pauseEffect","(I)V"),
        M(activity_class,pop_audio_resume_effect,"resumeEffect","(I)V"),
        M(activity_class,pop_audio_pause_effects,"pauseAllEffects","()V"),
        M(activity_class,pop_audio_resume_effects,"resumeAllEffects","()V"),
        M(activity_class,pop_audio_preload_effect,"preloadEffect","(Ljava/lang/String;)V"),
        M(activity_class,pop_audio_unload_effect,"unloadEffect","(Ljava/lang/String;)V"),
        M(activity_class,pop_audio_effects_volume,"getEffectsVolume","()F"),
        M(activity_class,pop_audio_set_effects_volume,"setEffectsVolume","(F)V"),
        {} };
    static const ManagedMethod renderer[]={
        M(renderer_class,animation,"setAnimationInterval","(D)V"),{}
    };
    static Class bitmap_class{};
    static Class share_class{};
    static Class flurry_class{},papaya_class{};
    static const ManagedMethod bitmap[]={
        M(bitmap_class,pop_text_bitmap,"createTextBitmap","(Ljava/lang/String;Ljava/lang/String;IIII)V"),{}
    };
    static const ManagedMethod share[]={
        M(share_class,device,"getDeviceName","()Ljava/lang/String;"),
        M(share_class,zero,"getRewardsCoins","()I"),
        M(share_class,video,"playVideo","(Ljava/lang/String;ZZ)V"),{}
    };
    static const ManagedMethod flurry[]={M(flurry_class,idle,"startFlurry","()V"),{}};
    static const ManagedMethod papaya[]={M(papaya_class,idle,"initializePapayaFramework","()V"),{}};
    setup(activity_class,"org/cocos2dx/lib/Cocos2dxActivity",activity);
    setup(renderer_class,"org/cocos2dx/lib/Cocos2dxRenderer",renderer);
    setup(bitmap_class,"org/cocos2dx/lib/Cocos2dxBitmap",bitmap);
    setup(share_class,"org/cocos2dx/lib/ShareUtility",share);
    setup(flurry_class,"org/cocos2dx/lib/FlurryUtility",flurry);
    setup(papaya_class,"org/cocos2dx/lib/UbiPapayaUtility",papaya);
}

extern "C" const char *port_fix_path(const char *original,char *out,size_t size) {
    if(!original) return nullptr;
    for(const char *bucket:{"DataHighRes","DataMidRes","DataLowRes"}) {
        const size_t n=strlen(original),b=strlen(bucket);
        if(n>b && !strcmp(original+n-b,bucket) && original[n-b-1]=='/') {
            snprintf(out,size,"%s/main.1.org.ubisoft.premium.POPClassic.obb",io_game_dir());
            return out;
        }
    }
    if(!strncmp(original,"/sdcard/",8)) {
        snprintf(out,size,"%s/%s",io_game_dir(),original+8);
        return out;
    }
    const char *appdir="/data/data/org.ubisoft.premium.POPClassic";
    if(!strncmp(original,appdir,strlen(appdir)) &&
       (original[strlen(appdir)]=='/' || original[strlen(appdir)]=='\0')) {
        snprintf(out,size,"%s%s",io_writable_dir(),original+strlen(appdir));
        return out;
    }
    return nullptr;
}
extern "C" const char *port_system_font() {return nullptr;}

// Cocos2d's zlib imports are broader than those used by the shared loader.
DynLibFunction symtable_nova2[]={
    {"inflateReset",(uintptr_t)&inflateReset},
    {"inflateEnd",(uintptr_t)&inflateEnd},
    {"deflate",(uintptr_t)&deflate},
    {"inflateInit2_",(uintptr_t)&inflateInit2_},
    {"inflate",(uintptr_t)&inflate},
    {"crc32",(uintptr_t)&crc32},
    {"deflateEnd",(uintptr_t)&deflateEnd},
    {"deflateReset",(uintptr_t)&deflateReset},
    {"inflateInit_",(uintptr_t)&inflateInit_},
    {"deflateInit2_",(uintptr_t)&deflateInit2_},
    {"gzread",(uintptr_t)&gzread},
    {"uncompress",(uintptr_t)&uncompress},
    {nullptr,0}
};
