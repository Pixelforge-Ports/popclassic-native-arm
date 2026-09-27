// Copyright (c) 2026 Pixelforge Ports contributors
#include "native_bindings.h"
#include "so_util.h"
#include "jni_internals.h"
#include "display_config.h"
#include "khronos/gles2.h"
#include "app_exit.h"
#include "crash.h"
#include "fb_probe.h"
#include "cursor_draw.h"
#include "fix_path.h"
#include "audio.h"
#include "input_bridge.h"
#include <SDL2/SDL.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static so_module *cocos_module=nullptr;
static so_module *game_module=nullptr;
static so_module *sound_module=nullptr;
static JNIEnv *env=nullptr;
Class activity_class{},renderer_class{};
static std::string donor_root;
static int width=720,height=480;
extern "C" void viewport_scale_init(int,int);

static int gl_probe_load(const char *path) {
    if(!path || !*path) {
        fprintf(stderr,"usage: popclassic --gl-probe <library>\n");
        return 2;
    }
    dlerror();
    void *handle=dlopen(path,RTLD_NOW|RTLD_LOCAL);
    if(!handle) {
        const char *error=dlerror();
        fprintf(stderr,"GL probe: %s failed to load: %s\n",path,error?error:"unknown loader error");
        return 3;
    }
    dlclose(handle);
    return 0;
}

static int gl_probe_init() {
    if(SDL_Init(SDL_INIT_VIDEO)!=0) {
        fprintf(stderr,"GL probe: SDL video init failed: %s\n",SDL_GetError());
        return 3;
    }
    SDL_Window *window=nullptr;
    SDL_GLContext context=nullptr;
    const int profiles[]={SDL_GL_CONTEXT_PROFILE_ES,SDL_GL_CONTEXT_PROFILE_COMPATIBILITY};
    const int majors[]={1,2};
    const int minors[]={1,1};
    for(int attempt=0;attempt<2 && !context;attempt++) {
        if(window) {SDL_DestroyWindow(window);window=nullptr;}
        SDL_GL_ResetAttributes();
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,profiles[attempt]);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,majors[attempt]);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,minors[attempt]);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
        window=SDL_CreateWindow("Prince of Persia Classic GL check",0,0,64,64,
                                SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
        if(window)context=SDL_GL_CreateContext(window);
    }
    if(!context) {
        fprintf(stderr,"GL probe: no usable GLES1 or compatibility context: %s\n",SDL_GetError());
        if(window)SDL_DestroyWindow(window);
        SDL_Quit();
        return 3;
    }
    const char *driver=SDL_GetCurrentVideoDriver();
    fprintf(stderr,"GL probe: video=%s; context created\n",driver?driver:"unknown");
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

static void flush_saves() {
    fflush(nullptr);
    const std::string directory=io_writable_dir();
    DIR *dir=opendir(directory.c_str());
    if(!dir)return;
    while(dirent *entry=readdir(dir)) {
        if(entry->d_name[0]=='.')continue;
        std::string path=directory+"/"+entry->d_name;
        int fd=open(path.c_str(),O_RDONLY|O_CLOEXEC);
        if(fd<0)continue;
        struct stat statbuf{};
        if(fstat(fd,&statbuf)==0 && S_ISREG(statbuf.st_mode))fsync(fd);
        close(fd);
    }
    closedir(dir);
    int fd=open(directory.c_str(),O_RDONLY|O_CLOEXEC);
    if(fd>=0) {fsync(fd);close(fd);}
}

static void capture_frame(int frame,int w,int h) {
    const char *target=getenv("POPCLASSIC_CAPTURE_FRAME");
    const char *many=getenv("POPCLASSIC_CAPTURE_FRAMES");
    bool selected=target && frame==atoi(target);
    if(many) {
        const char *at=many;
        while(*at) {
            if(frame==atoi(at))selected=true;
            const char *next=strchr(at,',');
            if(!next)break;
            at=next+1;
        }
    }
    if(!selected || w<=0 || h<=0)return;
    const char *path=getenv("POPCLASSIC_CAPTURE_PATH");
    std::string output;
    if(many) {
        const char *directory=getenv("POPCLASSIC_CAPTURE_DIR");
        if(!directory || !*directory)return;
        output=std::string(directory)+"/frame"+std::to_string(frame)+".bmp";
        path=output.c_str();
    }
    if(!path || !*path)return;
    using ReadPixels=void (*)(int,int,int,int,unsigned,unsigned,void*);
    auto read=reinterpret_cast<ReadPixels>(SDL_GL_GetProcAddress("glReadPixels"));
    if(!read)return;
    std::vector<unsigned char> rgba(size_t(w)*h*4),flipped(rgba.size());
    read(0,0,w,h,0x1908,0x1401,rgba.data());
    for(int y=0;y<h;y++)memcpy(flipped.data()+size_t(y)*w*4,
                                  rgba.data()+size_t(h-1-y)*w*4,size_t(w)*4);
    SDL_Surface *surface=SDL_CreateRGBSurfaceFrom(flipped.data(),w,h,32,w*4,
                                                   0x000000ff,0x0000ff00,0x00ff0000,0xff000000);
    if(surface) {SDL_SaveBMP(surface,path);SDL_FreeSurface(surface);}
}

static void test_tap(int frame) {
    const char *script=getenv("POPCLASSIC_TEST_TAPS");
    if(!script)return;
    int at=0,x=0,y=0;
    while(*script) {
        if(sscanf(script,"%d:%d:%d",&at,&x,&y)==3 && (frame==at || frame==at+1)) {
            if(frame==at)android_input_cursor_set(float(x),float(y));
            android_input_cursor_press(frame==at);
        }
        const char *next=strchr(script,';');
        if(!next)break;
        script=next+1;
    }
}
static void test_control(int frame) {
    const char *script=getenv("POPCLASSIC_TEST_CONTROLS");
    if(!script)return;
    int at=0,down=0;
    char name[32]{};
    while(*script) {
        if(sscanf(script,"%d:%31[^:]:%d",&at,name,&down)==3 && frame==at)
            android_input_inject_control(name,down!=0);
        const char *next=strchr(script,';');
        if(!next)break;
        script=next+1;
    }
}

void register_pop_services();
void pop_video_tick(JNIEnv *);
void pop_input_event(const SDL_Event &);
void pop_input_frame(Uint32);
void pop_input_init(so_module *,JNIEnv *,Class *,int,int);
void pop_input_hide_touch_controls();

so_module *port_guest_module() { return game_module ? game_module : cocos_module; }
so_module *port_cocos_module() { return cocos_module; }
extern "C" int so_after_relocate(so_module *module) {
    if(module->soname && strstr(module->soname,"libcocos2d.so")) cocos_module=module;
    if(module->soname && strstr(module->soname,"libgame_logic.so")) game_module=module;
    if(module->soname && strstr(module->soname,"libcocosdenshion.so")) sound_module=module;
    return 0;
}

template<typename T> static T required(so_module *module,const char *symbol) {
    auto address=module ? so_symbol(module,symbol) : 0;
    if(!address) {fprintf(stderr,"Prince of Persia: missing native entry %s\n",symbol);exit(1);}
    return reinterpret_cast<T>(address);
}

int main(int argc,char **argv) {
    if(argc>1 && !strcmp(argv[1],"--version")) {puts("popclassic-development-0.1");return 0;}
    if(argc>1 && !strcmp(argv[1],"--gl-probe")) return gl_probe_load(argc>2?argv[2]:nullptr);
    if(argc>1 && !strcmp(argv[1],"--gl-probe-init")) return gl_probe_init();
    const char *root=argc>1?argv[1]:"donor";
    char absolute[4096];
    if(!realpath(root,absolute)) {perror("Game data directory");return 1;}
    donor_root=absolute;
    const std::string libdir=donor_root+"/lib/armeabi";
    const std::string apk=donor_root+"/original.apk";
    if(access(apk.c_str(),R_OK) || access((libdir+"/libgame_logic.so").c_str(),R_OK)) {
        fprintf(stderr,"Prince of Persia: original.apk and all three donor libraries are required in %s\n",donor_root.c_str());
        return 1;
    }
    io_set_game_dir(donor_root.c_str());
    if(chdir(donor_root.c_str())) return 1;
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)) {
        fprintf(stderr,"SDL_Init: %s\n",SDL_GetError());return 1;
    }
    if(SDL_InitSubSystem(SDL_INIT_AUDIO))
        fprintf(stderr,"Prince of Persia audio: SDL subsystem unavailable: %s\n",SDL_GetError());
    else if(!pop_audio_init())
        fprintf(stderr,"Prince of Persia audio: no output device; continuing without sound\n");
    if(const char *asset=getenv("POPCLASSIC_TEST_AUDIO")) {
        bool ok=pop_audio_probe(asset);
        pop_audio_shutdown();SDL_Quit();
        return ok?0:1;
    }
    if(!display_config::detect("POPCLASSIC",width,height,false)) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
    SDL_Window *window=SDL_CreateWindow("Prince of Persia Classic",0,0,width,height,
                                       SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN);
    SDL_GLContext gl=window?SDL_GL_CreateContext(window):nullptr;
    if(!gl) {
        fprintf(stderr,"GLES1 context unavailable (%s); trying desktop compatibility GL\n",SDL_GetError());
        if(window)SDL_DestroyWindow(window);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
        window=SDL_CreateWindow("Prince of Persia Classic",0,0,width,height,
                                SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN);
        gl=window?SDL_GL_CreateContext(window):nullptr;
    }
    if(!gl) {fprintf(stderr,"GLES context: %s\n",SDL_GetError());return 1;}
    int target_fps=60;
    if(const char *limit=getenv("POPCLASSIC_FPS")) {
        const int requested=atoi(limit);
        if(requested>=30 && requested<=120)target_fps=requested;
    }
    int swap_interval=0;
    const char *frame_pacing="timer";
    if(SDL_GL_SetSwapInterval(-1)==0) {
        swap_interval=-1;
        frame_pacing="adaptive-vsync";
    } else if(SDL_GL_SetSwapInterval(1)==0) {
        swap_interval=1;
        frame_pacing="vsync";
    } else {
        SDL_GL_SetSwapInterval(0);
    }
    const bool display_sync=swap_interval!=0;
    const Uint64 performance_frequency=SDL_GetPerformanceFrequency();
    const Uint64 frame_ticks=performance_frequency
        ? (performance_frequency+static_cast<Uint64>(target_fps/2))/static_cast<Uint64>(target_fps)
        : 0;
    fprintf(stderr,"Prince of Persia: frame pacing=%s target=%d fps\n",
            frame_pacing,target_fps);
    int draw_w=0,draw_h=0;SDL_GL_GetDrawableSize(window,&draw_w,&draw_h);
    if(!display_config::drawable("POPCLASSIC",draw_w,draw_h,width,height,false)) return 1;
    load_gles1_funcs();load_gles2_funcs();viewport_scale_init(draw_w,draw_h);
    register_pop_services();
    JavaVM *vm=nullptr;
    if(JNI_CreateJavaVM(&vm,&env,nullptr)!=JNI_OK) return 1;
    so_set_options(nullptr,libdir.c_str());
    game_module=so_load_module("libgame_logic.so",nullptr,nullptr);
    if(!game_module || !cocos_module || !sound_module) {
        fprintf(stderr,"Prince of Persia: missing game, Cocos2d or CocosDenshion module\n");
        return 1;
    }
    using OnLoad=jint (ABI_ATTR *)(JavaVM*,void*);
    for(auto *module:{sound_module,cocos_module,game_module}) {
        auto onload=reinterpret_cast<OnLoad>(so_symbol(module,"JNI_OnLoad"));
        if(onload && onload(vm,nullptr)<0) return 1;
    }
    auto set_paths=required<donor::Cocos2dxActivity_nativeSetPaths_8>(
        cocos_module,donor::Cocos2dxActivity_nativeSetPaths_8_symbol);
    auto set_package=required<donor::Cocos2dxActivity_nativeSetPackageName_7>(
        cocos_module,donor::Cocos2dxActivity_nativeSetPackageName_7_symbol);
    const std::string base=donor_root+"/";
    set_paths(env,(jclass)&activity_class,env->NewStringUTF(base.c_str()),
              env->NewStringUTF(apk.c_str()),env->NewStringUTF("R800i"));
    set_package(env,(jclass)&activity_class,env->NewStringUTF("org.ubisoft.premium.POPClassic"));
    if(auto fn=reinterpret_cast<donor::Cocos2dxActivity_nativeSetNumOfCPUCores_6>(
        so_symbol(cocos_module,donor::Cocos2dxActivity_nativeSetNumOfCPUCores_6_symbol)))
        fn(env,(jclass)&activity_class,4);
    auto init=required<donor::Cocos2dxRenderer_nativeInit_12>(
        game_module,donor::Cocos2dxRenderer_nativeInit_12_symbol);
    auto render=required<donor::Cocos2dxRenderer_nativeRender_18>(
        cocos_module,donor::Cocos2dxRenderer_nativeRender_18_symbol);
    init(env,(jclass)&renderer_class,width,height);
    pop_input_init(cocos_module,env,&renderer_class,width,height);
    int frames=0;
    int frame_limit=getenv("POPCLASSIC_TEST_FRAMES")?atoi(getenv("POPCLASSIC_TEST_FRAMES")):0;
    while(!android_app_exit_requested()) {
        const Uint64 frame_start=SDL_GetPerformanceCounter();
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type==SDL_QUIT) android_app_request_exit("SDL quit");
            else pop_input_event(event);
        }
        test_tap(frames+1);
        test_control(frames+1);
        pop_video_tick(env);
        pop_input_frame(SDL_GetTicks());
        if(android_app_exit_requested()) break;
        render(env,(jclass)&renderer_class);
        if(frames==0)pop_input_hide_touch_controls();
        if(frame_limit>0) android_fb_probe(frames+1,draw_w,draw_h);
        capture_frame(frames+1,draw_w,draw_h);
        android_cursor_draw(draw_w,draw_h);
        SDL_GL_SwapWindow(window);
        if(++frames==1) fprintf(stderr,"Prince of Persia: first frame returned\n");
        if(frame_limit>0 && frames>=frame_limit) break;
        if(!display_sync && frame_ticks && performance_frequency) {
            const Uint64 elapsed=SDL_GetPerformanceCounter()-frame_start;
            if(elapsed<frame_ticks) {
                const Uint64 remaining=frame_ticks-elapsed;
                const Uint32 delay_ms=static_cast<Uint32>(remaining*1000/performance_frequency);
                if(delay_ms)SDL_Delay(delay_ms);
            }
        }
    }
    fprintf(stderr,"Prince of Persia: rendered %d frames\n",frames);
    if(auto pause=reinterpret_cast<donor::Cocos2dxRenderer_nativeOnPause_16>(
        so_symbol(cocos_module,donor::Cocos2dxRenderer_nativeOnPause_16_symbol)))
        pause(env,(jclass)&renderer_class);
    flush_saves();
    pop_audio_shutdown();
    SDL_Quit();
    fflush(nullptr);
    // Guest static destructors assume Android's C++ runtime and corrupt the host heap.
    std::quick_exit(0);
}
