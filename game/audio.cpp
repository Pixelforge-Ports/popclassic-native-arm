// Copyright (c) 2026 Pixelforge Ports contributors
#include "audio.h"
#include "fix_path.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

#define MINIMP3_ONLY_MP3
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_EXT_IMPLEMENTATION
#include "../third_party/minimp3/minimp3_ex.h"
#include "../third_party/minimp4/minimp4.h"
#include "../third_party/faad2/include/neaacdec.h"

struct Clip {std::vector<Uint8> pcm;};
struct Voice {
    int id=0;
    std::shared_ptr<Clip> clip;
    size_t offset=0;
    bool loop=false;
    bool paused=false;
};
static SDL_AudioDeviceID device_id=0;
static SDL_AudioSpec output_spec=[] {
    SDL_AudioSpec spec{};
    spec.freq=44100;spec.format=AUDIO_S16SYS;spec.channels=2;
    return spec;
}();
static std::map<std::string,std::shared_ptr<Clip>> cache;
static Voice music;
static std::vector<Voice> effects;
static float music_volume=1.f,effects_volume=1.f;
static int next_effect_id=1;

static int sdl_volume(float value) {
    return std::max(0,std::min(SDL_MIX_MAXVOLUME,int(std::lround(value*SDL_MIX_MAXVOLUME))));
}
static void mix_voice(Voice &voice,Uint8 *stream,int length,int volume) {
    if(!voice.clip || voice.paused || volume<=0)return;
    int done=0;
    while(done<length) {
        const auto &pcm=voice.clip->pcm;
        if(voice.offset>=pcm.size()) {
            if(voice.loop && !pcm.empty())voice.offset=0;
            else {voice.clip.reset();break;}
        }
        if(!voice.clip)break;
        size_t count=std::min(size_t(length-done),pcm.size()-voice.offset);
        SDL_MixAudioFormat(stream+done,pcm.data()+voice.offset,AUDIO_S16SYS,Uint32(count),volume);
        voice.offset+=count;
        done+=int(count);
    }
}
static void callback(void*,Uint8 *stream,int length) {
    memset(stream,0,size_t(length));
    mix_voice(music,stream,length,sdl_volume(music_volume));
    for(auto &voice:effects)mix_voice(voice,stream,length,sdl_volume(effects_volume));
    effects.erase(std::remove_if(effects.begin(),effects.end(),
                                 [](const Voice &v){return !v.clip;}),effects.end());
}
static std::string owned_audio_path(const char *requested) {
    if(!requested)return {};
    std::string source=requested;
    std::replace(source.begin(),source.end(),'\\','/');
    const auto at=source.find("Extra/Audio/");
    if(at==std::string::npos)return {};
    std::string relative=source.substr(at);
    if(relative.find("..")!=std::string::npos || relative.find(':')!=std::string::npos)return {};
    return std::string(io_game_dir())+"/apk-assets/"+relative;
}
static int mp4_read(int64_t offset,void *buffer,size_t size,void *token) {
    FILE *file=static_cast<FILE*>(token);
    if(offset<0 || offset>LONG_MAX || fseek(file,long(offset),SEEK_SET))return 1;
    return fread(buffer,1,size,file)==size?0:1;
}
static bool load_aac_mp4(FILE *file,long file_size,std::vector<Uint8> &pcm,int &hz,int &channels) {
    MP4D_demux_t mp4{};
    if(!MP4D_open(&mp4,mp4_read,file,file_size))return false;
    bool success=false;
    for(unsigned track=0;track<mp4.track_count && !success;track++) {
        auto &part=mp4.track[track];
        if(part.handler_type!=MP4D_HANDLER_TYPE_SOUN || !part.dsi || !part.dsi_bytes ||
           part.sample_count>100000 || part.object_type_indication!=0x40)continue;
        NeAACDecHandle decoder=NeAACDecOpen();
        if(!decoder)continue;
        auto config=NeAACDecGetCurrentConfiguration(decoder);
        config->outputFormat=FAAD_FMT_16BIT;
        NeAACDecSetConfiguration(decoder,config);
        unsigned long rate=0;
        unsigned char count=0;
        if(NeAACDecInit2(decoder,part.dsi,part.dsi_bytes,&rate,&count)==0 &&
           rate>0 && (count==1 || count==2)) {
            hz=int(rate);channels=int(count);success=true;
            for(unsigned sample=0;sample<part.sample_count;sample++) {
                unsigned bytes=0;
                MP4D_file_offset_t offset=MP4D_frame_offset(&mp4,track,sample,&bytes,nullptr,nullptr);
                if(bytes==0 || bytes>1024*1024 ||
                   uint64_t(offset)+bytes>uint64_t(file_size)) {success=false;break;}
                std::vector<Uint8> encoded(bytes);
                if(mp4_read(offset,encoded.data(),encoded.size(),file)) {success=false;break;}
                NeAACDecFrameInfo frame{};
                void *decoded=NeAACDecDecode(decoder,&frame,encoded.data(),encoded.size());
                if(frame.error || !decoded || frame.channels!=count || frame.samplerate!=rate ||
                   pcm.size()+size_t(frame.samples)*2>64*1024*1024) {success=false;break;}
                const auto *data=static_cast<const Uint8*>(decoded);
                pcm.insert(pcm.end(),data,data+size_t(frame.samples)*2);
            }
            if(pcm.empty())success=false;
        }
        NeAACDecClose(decoder);
    }
    MP4D_close(&mp4);
    if(!success)pcm.clear();
    return success;
}
static std::shared_ptr<Clip> load_clip(const char *requested) {
    const std::string path=owned_audio_path(requested);
    if(path.empty()) {fprintf(stderr,"Prince of Persia audio: invalid asset path %s\n",requested?requested:"(null)");return {};}
    auto cached=cache.find(path);
    if(cached!=cache.end())return cached->second;
    FILE *file=fopen(path.c_str(),"rb");
    if(!file || fseek(file,0,SEEK_END)) {
        fprintf(stderr,"Prince of Persia audio: cannot open %s\n",path.c_str());
        if(file)fclose(file);
        return {};
    }
    long size=ftell(file);
    if(size<=0 || size>16*1024*1024 || fseek(file,0,SEEK_SET)) {fclose(file);return {};}
    std::vector<Uint8> raw;
    int source_hz=0,source_channels=0;
    if(path.size()>=4 && path.substr(path.size()-4)==".mp3") {
        std::vector<Uint8> encoded(size_t(size),0);
        size_t received=fread(encoded.data(),1,encoded.size(),file);
        fclose(file);
        if(received!=encoded.size())return {};
        mp3dec_t decoder{};
        mp3dec_file_info_t info{};
        int result=mp3dec_load_buf(&decoder,encoded.data(),encoded.size(),&info,nullptr,nullptr);
        if(result || !info.buffer || !info.samples || info.hz<=0 ||
           (info.channels!=1 && info.channels!=2)) {
            fprintf(stderr,"Prince of Persia audio: MP3 decode failed (%d): %s\n",result,path.c_str());
            free(info.buffer);return {};
        }
        source_hz=info.hz;source_channels=info.channels;
        const auto *data=reinterpret_cast<const Uint8*>(info.buffer);
        raw.assign(data,data+info.samples*sizeof(mp3d_sample_t));
        free(info.buffer);
    } else if(path.size()>=4 && (path.substr(path.size()-4)==".mp4" ||
               path.substr(path.size()-4)==".m4a")) {
        bool ok=load_aac_mp4(file,size,raw,source_hz,source_channels);
        fclose(file);
        if(!ok) {fprintf(stderr,"Prince of Persia audio: AAC decode failed: %s\n",path.c_str());return {};}
    } else {fclose(file);return {};}
    const size_t source_bytes=raw.size();
    SDL_AudioCVT cvt{};
    if(source_bytes>64*1024*1024 || SDL_BuildAudioCVT(&cvt,AUDIO_S16SYS,Uint8(source_channels),
         source_hz,AUDIO_S16SYS,2,output_spec.freq)<0) {
        fprintf(stderr,"Prince of Persia audio: unsupported PCM conversion: %s\n",path.c_str());
        return {};
    }
    auto clip=std::make_shared<Clip>();
    clip->pcm.resize(source_bytes*size_t(std::max(1,cvt.len_mult)));
    memcpy(clip->pcm.data(),raw.data(),source_bytes);
    if(cvt.needed) {
        cvt.buf=clip->pcm.data();
        cvt.len=int(source_bytes);
        if(SDL_ConvertAudio(&cvt)<0) {
            fprintf(stderr,"Prince of Persia audio: SDL conversion failed: %s\n",SDL_GetError());
            return {};
        }
        clip->pcm.resize(size_t(cvt.len_cvt));
    } else clip->pcm.resize(source_bytes);
    if(clip->pcm.empty())return {};
    fprintf(stderr,"Prince of Persia audio: decoded %s (%zu PCM bytes, %d Hz)\n",
            path.c_str(),clip->pcm.size(),output_spec.freq);
    cache.emplace(path,clip);
    return clip;
}
bool pop_audio_probe(const char *path) {return bool(load_clip(path));}
static std::shared_ptr<Clip> from_jstring(JNIEnv *env,jstring value) {
    if(!value)return {};
    const char *text=env->GetStringUTFChars(value,nullptr);
    auto clip=load_clip(text);
    env->ReleaseStringUTFChars(value,text);
    return clip;
}
static std::string key_from_jstring(JNIEnv *env,jstring value) {
    if(!value)return {};
    const char *text=env->GetStringUTFChars(value,nullptr);
    std::string path=owned_audio_path(text);
    env->ReleaseStringUTFChars(value,text);
    return path;
}
bool pop_audio_init() {
    SDL_AudioSpec want{};
    want.freq=44100;want.format=AUDIO_S16SYS;want.channels=2;want.samples=1024;
    want.callback=callback;
    const char *name=getenv("POPCLASSIC_AUDIO_DEVICE");
    if(name && !*name)name=nullptr;
    SDL_AudioSpec obtained{};
    device_id=SDL_OpenAudioDevice(name,0,&want,&obtained,SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if(!device_id || obtained.format!=AUDIO_S16SYS || obtained.channels!=2) {
        fprintf(stderr,"Prince of Persia audio: %s unavailable: %s\n",
                name?name:"default device",SDL_GetError());
        if(device_id)SDL_CloseAudioDevice(device_id);
        device_id=0;
        return false;
    }
    output_spec=obtained;
    fprintf(stderr,"Prince of Persia audio: opened %s\n",name?name:"default device");
    SDL_PauseAudioDevice(device_id,0);
    fprintf(stderr,"Prince of Persia audio: %s, %d Hz stereo\n",
            SDL_GetCurrentAudioDriver(),output_spec.freq);
    return true;
}
void pop_audio_shutdown() {
    if(device_id) {
        SDL_LockAudioDevice(device_id);
        music={};effects.clear();cache.clear();
        SDL_UnlockAudioDevice(device_id);
        SDL_CloseAudioDevice(device_id);
        device_id=0;
    }
}
void pop_audio_play_music(JNIEnv *env,jclass,jstring path,jboolean loop) {
    if(!device_id)return;
    auto clip=from_jstring(env,path);
    SDL_LockAudioDevice(device_id);
    music={};music.clip=clip;music.loop=loop;
    SDL_UnlockAudioDevice(device_id);
}
void pop_audio_stop_music(JNIEnv*,jclass) {if(device_id){SDL_LockAudioDevice(device_id);music={};SDL_UnlockAudioDevice(device_id);}}
void pop_audio_pause_music(JNIEnv*,jclass) {if(device_id){SDL_LockAudioDevice(device_id);music.paused=true;SDL_UnlockAudioDevice(device_id);}}
void pop_audio_resume_music(JNIEnv*,jclass) {if(device_id){SDL_LockAudioDevice(device_id);music.paused=false;SDL_UnlockAudioDevice(device_id);}}
void pop_audio_rewind_music(JNIEnv*,jclass) {if(device_id){SDL_LockAudioDevice(device_id);music.offset=0;SDL_UnlockAudioDevice(device_id);}}
jboolean pop_audio_music_playing(JNIEnv*,jclass) {if(!device_id)return JNI_FALSE;SDL_LockAudioDevice(device_id);bool playing=music.clip && !music.paused;SDL_UnlockAudioDevice(device_id);return playing?JNI_TRUE:JNI_FALSE;}
jfloat pop_audio_music_volume(JNIEnv*,jclass) {return music_volume;}
void pop_audio_set_music_volume(JNIEnv*,jclass,jfloat value) {if(device_id)SDL_LockAudioDevice(device_id);music_volume=std::clamp(float(value),0.f,1.f);if(device_id)SDL_UnlockAudioDevice(device_id);}
void pop_audio_preload_music(JNIEnv *env,jclass,jstring path) {from_jstring(env,path);}
jint pop_audio_play_effect(JNIEnv *env,jclass,jstring path,jboolean loop) {
    if(!device_id)return 0;
    auto clip=from_jstring(env,path);
    if(!clip)return 0;
    SDL_LockAudioDevice(device_id);
    int id=next_effect_id++;
    effects.push_back({id,clip,0,bool(loop),false});
    SDL_UnlockAudioDevice(device_id);
    return id;
}
void pop_audio_stop_effect(JNIEnv*,jclass,jint id) {if(device_id){SDL_LockAudioDevice(device_id);effects.erase(std::remove_if(effects.begin(),effects.end(),[id](const Voice &v){return v.id==id;}),effects.end());SDL_UnlockAudioDevice(device_id);}}
void pop_audio_stop_effects(JNIEnv*,jclass) {if(device_id){SDL_LockAudioDevice(device_id);effects.clear();SDL_UnlockAudioDevice(device_id);}}
void pop_audio_pause_effect(JNIEnv*,jclass,jint id) {if(device_id){SDL_LockAudioDevice(device_id);for(auto &v:effects)if(v.id==id)v.paused=true;SDL_UnlockAudioDevice(device_id);}}
void pop_audio_resume_effect(JNIEnv*,jclass,jint id) {if(device_id){SDL_LockAudioDevice(device_id);for(auto &v:effects)if(v.id==id)v.paused=false;SDL_UnlockAudioDevice(device_id);}}
void pop_audio_pause_effects(JNIEnv*,jclass) {if(device_id){SDL_LockAudioDevice(device_id);for(auto &v:effects)v.paused=true;SDL_UnlockAudioDevice(device_id);}}
void pop_audio_resume_effects(JNIEnv*,jclass) {if(device_id){SDL_LockAudioDevice(device_id);for(auto &v:effects)v.paused=false;SDL_UnlockAudioDevice(device_id);}}
void pop_audio_preload_effect(JNIEnv *env,jclass,jstring path) {from_jstring(env,path);}
void pop_audio_unload_effect(JNIEnv *env,jclass,jstring path) {cache.erase(key_from_jstring(env,path));}
jfloat pop_audio_effects_volume(JNIEnv*,jclass) {return effects_volume;}
void pop_audio_set_effects_volume(JNIEnv*,jclass,jfloat value) {if(device_id)SDL_LockAudioDevice(device_id);effects_volume=std::clamp(float(value),0.f,1.f);if(device_id)SDL_UnlockAudioDevice(device_id);}
