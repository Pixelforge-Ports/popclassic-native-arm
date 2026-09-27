#pragma once
#include "jni_internals.h"

bool pop_audio_init();
bool pop_audio_probe(const char *);
void pop_audio_shutdown();
void pop_audio_play_music(JNIEnv*,jclass,jstring,jboolean);
void pop_audio_stop_music(JNIEnv*,jclass);
void pop_audio_pause_music(JNIEnv*,jclass);
void pop_audio_resume_music(JNIEnv*,jclass);
void pop_audio_rewind_music(JNIEnv*,jclass);
jboolean pop_audio_music_playing(JNIEnv*,jclass);
jfloat pop_audio_music_volume(JNIEnv*,jclass);
void pop_audio_set_music_volume(JNIEnv*,jclass,jfloat);
void pop_audio_preload_music(JNIEnv*,jclass,jstring);
jint pop_audio_play_effect(JNIEnv*,jclass,jstring,jboolean);
void pop_audio_stop_effect(JNIEnv*,jclass,jint);
void pop_audio_stop_effects(JNIEnv*,jclass);
void pop_audio_pause_effect(JNIEnv*,jclass,jint);
void pop_audio_resume_effect(JNIEnv*,jclass,jint);
void pop_audio_pause_effects(JNIEnv*,jclass);
void pop_audio_resume_effects(JNIEnv*,jclass);
void pop_audio_preload_effect(JNIEnv*,jclass,jstring);
void pop_audio_unload_effect(JNIEnv*,jclass,jstring);
jfloat pop_audio_effects_volume(JNIEnv*,jclass);
void pop_audio_set_effects_volume(JNIEnv*,jclass,jfloat);
