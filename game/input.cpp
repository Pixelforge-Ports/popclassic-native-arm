// Copyright (c) 2026 Pixelforge Ports contributors
#include "native_bindings.h"
#include "jni_internals.h"
#include "so_util.h"
#include "input_bridge.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static JNIEnv *env=nullptr;
static Class *renderer=nullptr;
extern Class activity_class;
static donor::Cocos2dxRenderer_nativeKeyDown_14 key_down=nullptr;
static donor::Cocos2dxRenderer_nativeKeyUp_15 key_up=nullptr;
static donor::Cocos2dxActivity_SetControlInVisible_2 hide_touch_controls=nullptr;
static donor::Cocos2dxActivity_SetControlVisible_3 show_touch_controls=nullptr;
static donor::Cocos2dxRenderer_nativeTouchesBegin_19 touch_down=nullptr;
static donor::Cocos2dxRenderer_nativeTouchesEnd_21 touch_up=nullptr;
static donor::Cocos2dxRenderer_nativeTouchesMove_22 touch_move=nullptr;
static SDL_GameController *pad=nullptr;
static SDL_Joystick *raw_pad=nullptr;
static SDL_JoystickID raw_instance=-1;
static int width=720,height=480;
static float cx=360,cy=240;
static bool touching=false;
static bool mouse_mode=false;
static bool select_held=false;
static bool held[256]{};
static bool shoulder_button[2]{};
static bool shoulder_trigger[2]{};
static bool touch_controls_hidden=true;
static bool shoulder_combo_down=false;
static bool trigger_combo_down=false;
static bool input_debug=false;
static Uint32 left_since=0,right_since=0;
static bool left_running=false,right_running=false;
static float left_x=0,left_y=0,right_x=0,right_y=0;
static float raw_left_x=0,raw_left_y=0,raw_right_x=0,raw_right_y=0;

void pop_input_hide_touch_controls() {
    if(hide_touch_controls && env) {
        hide_touch_controls(env,(jclass)&activity_class);
        touch_controls_hidden=true;
    }
}
static void pop_input_show_touch_controls() {
    if(show_touch_controls && env) {
        show_touch_controls(env,(jclass)&activity_class);
        touch_controls_hidden=false;
    }
}
static void update_touch_control_combo(bool trigger) {
    bool down=trigger?(shoulder_trigger[0] && shoulder_trigger[1])
                     :(shoulder_button[0] && shoulder_button[1]);
    bool &was_down=trigger?trigger_combo_down:shoulder_combo_down;
    if(down && !was_down) {
        if(touch_controls_hidden)pop_input_show_touch_controls();
        else pop_input_hide_touch_controls();
    }
    was_down=down;
}

static void send_key(int code,bool down) {
    if(code<0 || code>=256 || held[code]==down || !key_down || !key_up)return;
    held[code]=down;
    (down?key_down:key_up)(env,(jclass)renderer,code);
}
static void set_shoulder_source(int side,bool trigger,bool down) {
    if(side<0 || side>1)return;
    bool &source=trigger?shoulder_trigger[side]:shoulder_button[side];
    if(source==down)return;
    source=down;
    const int code=trigger?(side?105:104):(side?103:102);
    send_key(code,down);
    update_touch_control_combo(trigger);
    if(input_debug)
        fprintf(stderr,"Prince of Persia input: %s %s -> Android key %d\n",
                trigger?"trigger":"shoulder",side?"right":"left",code);
}
static void release_all() {
    for(int code=0;code<256;code++) if(held[code]) send_key(code,false);
    android_input_cursor_press(false);
    select_held=false;
    shoulder_button[0]=shoulder_button[1]=false;
    shoulder_trigger[0]=shoulder_trigger[1]=false;
    shoulder_combo_down=trigger_combo_down=false;
    left_running=right_running=false;
    left_since=right_since=0;
    left_x=left_y=right_x=right_y=0;
    raw_left_x=raw_left_y=raw_right_x=raw_right_y=0;
}
void pop_input_init(so_module *module,JNIEnv *e,Class *klass,int w,int h) {
    env=e;renderer=klass;width=w;height=h;cx=w/2;cy=h/2;
    input_debug=std::getenv("POPCLASSIC_DEBUG")!=nullptr;
    key_down=reinterpret_cast<donor::Cocos2dxRenderer_nativeKeyDown_14>(
        so_symbol(module,donor::Cocos2dxRenderer_nativeKeyDown_14_symbol));
    key_up=reinterpret_cast<donor::Cocos2dxRenderer_nativeKeyUp_15>(
        so_symbol(module,donor::Cocos2dxRenderer_nativeKeyUp_15_symbol));
    hide_touch_controls=reinterpret_cast<donor::Cocos2dxActivity_SetControlInVisible_2>(
        so_symbol(module,donor::Cocos2dxActivity_SetControlInVisible_2_symbol));
    show_touch_controls=reinterpret_cast<donor::Cocos2dxActivity_SetControlVisible_3>(
        so_symbol(module,donor::Cocos2dxActivity_SetControlVisible_3_symbol));
    touch_down=reinterpret_cast<donor::Cocos2dxRenderer_nativeTouchesBegin_19>(
        so_symbol(module,donor::Cocos2dxRenderer_nativeTouchesBegin_19_symbol));
    touch_up=reinterpret_cast<donor::Cocos2dxRenderer_nativeTouchesEnd_21>(
        so_symbol(module,donor::Cocos2dxRenderer_nativeTouchesEnd_21_symbol));
    touch_move=reinterpret_cast<donor::Cocos2dxRenderer_nativeTouchesMove_22>(
        so_symbol(module,donor::Cocos2dxRenderer_nativeTouchesMove_22_symbol));
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);
    for(int n=0;n<SDL_NumJoysticks();n++) {
        if(SDL_IsGameController(n)) {
            if(!pad)pad=SDL_GameControllerOpen(n);
        } else if(!raw_pad) {
            raw_pad=SDL_JoystickOpen(n);
            if(raw_pad)raw_instance=SDL_JoystickInstanceID(raw_pad);
        }
    }
    SDL_ShowCursor(SDL_DISABLE);
    if(hide_touch_controls) {
        pop_input_hide_touch_controls();
        SDL_Log("Prince of Persia input: requested native touch controls hidden");
    } else {
        SDL_Log("Prince of Persia input: native touch-control hide method unavailable");
    }
    SDL_Log("Prince of Persia input: controller %s",pad?SDL_GameControllerName(pad):"not found");
}
extern "C" void android_input_cursor_position(float *x,float *y,int *visible) {
    if(x)*x=cx;
    if(y)*y=cy;
    if(visible)*visible=mouse_mode?1:0;
}
void android_input_cursor_set(float x,float y) {
    cx=std::clamp(x,0.f,float(width-1));cy=std::clamp(y,0.f,float(height-1));
    if(touching && touch_move && env) {
        jint id=0; jfloat xf=cx,yf=cy;
        jintArray ids=env->NewIntArray(1);
        jfloatArray xs=env->NewFloatArray(1),ys=env->NewFloatArray(1);
        env->SetIntArrayRegion(ids,0,1,&id);
        env->SetFloatArrayRegion(xs,0,1,&xf);env->SetFloatArrayRegion(ys,0,1,&yf);
        touch_move(env,(jclass)renderer,ids,xs,ys);
        env->DeleteLocalRef(ids);env->DeleteLocalRef(xs);env->DeleteLocalRef(ys);
    }
}
void android_input_cursor_press(bool down) {
    if(!env || touching==down || !touch_down || !touch_up)return;
    touching=down;
    (down?touch_down:touch_up)(env,(jclass)renderer,0,cx,cy);
}
bool android_input_inject_control(const char *name,bool down) {
    if(!name)return false;
    if(!strcmp(name,"mouse_left")) {android_input_cursor_press(down);return true;}
    // SDL's logical face buttons are positional: A=south, B=east, X=west,
    // Y=north. On the RG34XX SP those positions are printed B, A, Y, X.
    // Forward Android gamepad keycodes so the game uses its Xperia PLAY path:
    // Cross, Circle, Square and Triangle respectively.
    if(!strcmp(name,"a")) {
        if(mouse_mode) {android_input_cursor_press(down);return true;}
        send_key(96,down);
        send_key(23,down); // Xperia PLAY Cross also confirms focused menu items.
        return true;
    }
    if(!strcmp(name,"b")) {
        if(mouse_mode) {android_input_cursor_press(down);return true;}
        send_key(97,down);return true;
    }
    if(!strcmp(name,"x")) {send_key(99,down);return true;}
    if(!strcmp(name,"y")) {send_key(100,down);return true;}
    if(!strcmp(name,"l1")) {set_shoulder_source(0,false,down);return true;}
    if(!strcmp(name,"r1")) {set_shoulder_source(1,false,down);return true;}
    if(!strcmp(name,"l2")) {set_shoulder_source(0,true,down);return true;}
    if(!strcmp(name,"r2")) {set_shoulder_source(1,true,down);return true;}
    if(!strcmp(name,"start") || !strcmp(name,"back")) {send_key(4,down);return true;}
    if(!strcmp(name,"select")) {
        if(down && !select_held) {
            mouse_mode=!mouse_mode;
            if(!mouse_mode)android_input_cursor_press(false);
        }
        select_held=down;
        return true;
    }
    if(!strcmp(name,"up")) {send_key(19,down);return true;}
    if(!strcmp(name,"down")) {send_key(20,down);return true;}
    if(!strcmp(name,"left")) {
        if(held[21]!=down) {send_key(21,down);left_since=down?SDL_GetTicks():0;left_running=false;}
        return true;
    }
    if(!strcmp(name,"right")) {
        if(held[22]!=down) {send_key(22,down);right_since=down?SDL_GetTicks():0;right_running=false;}
        return true;
    }
    return false;
}
bool android_input_inject_stick(const char *name,float x,float y) {
    if(!name)return false;
    if(!strcmp(name,"right")) {right_x=x;right_y=y;return true;}
    if(!strcmp(name,"left")) {
        left_x=x;left_y=y;
        android_input_inject_control("left",x<-.35f);
        android_input_inject_control("right",x>.35f);
        android_input_inject_control("up",y<-.35f);
        android_input_inject_control("down",y>.35f);
        return true;
    }
    return false;
}
static bool controller_maps_button(SDL_JoystickID instance,int physical) {
    SDL_GameController *controller=SDL_GameControllerFromInstanceID(instance);
    if(!controller)return false;
    for(int button=0;button<SDL_CONTROLLER_BUTTON_MAX;button++) {
        SDL_GameControllerButtonBind bind=SDL_GameControllerGetBindForButton(
            controller,static_cast<SDL_GameControllerButton>(button));
        if(bind.bindType==SDL_CONTROLLER_BINDTYPE_BUTTON && bind.value.button==physical)
            return true;
    }
    return false;
}
static bool controller_maps_axis(SDL_JoystickID instance,int physical) {
    SDL_GameController *controller=SDL_GameControllerFromInstanceID(instance);
    if(!controller)return false;
    for(int axis=0;axis<SDL_CONTROLLER_AXIS_MAX;axis++) {
        SDL_GameControllerButtonBind bind=SDL_GameControllerGetBindForAxis(
            controller,static_cast<SDL_GameControllerAxis>(axis));
        if(bind.bindType==SDL_CONTROLLER_BINDTYPE_AXIS && bind.value.axis==physical)
            return true;
    }
    return false;
}
static bool controller_maps_hat(SDL_JoystickID instance,int physical) {
    SDL_GameController *controller=SDL_GameControllerFromInstanceID(instance);
    if(!controller)return false;
    const SDL_GameControllerButton directions[]={
        SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT};
    for(SDL_GameControllerButton direction:directions) {
        SDL_GameControllerButtonBind bind=SDL_GameControllerGetBindForButton(controller,direction);
        if(bind.bindType==SDL_CONTROLLER_BINDTYPE_HAT && bind.value.hat.hat==physical)
            return true;
    }
    return false;
}
static const char *raw_button_control(Uint8 button) {
    switch(button) {
    case 0:return "a";
    case 1:return "b";
    case 2:return "x";
    case 3:return "y";
    case 4:return "l1";
    case 5:return "r1";
    case 6:return "select";
    case 7:return "start";
    default:return nullptr;
    }
}
void pop_input_event(const SDL_Event &event) {
    if(event.type==SDL_CONTROLLERDEVICEADDED) {
        if(SDL_IsGameController(event.cdevice.which)) {
            if(!pad)pad=SDL_GameControllerOpen(event.cdevice.which);
        } else if(!raw_pad) {
            raw_pad=SDL_JoystickOpen(event.cdevice.which);
            if(raw_pad)raw_instance=SDL_JoystickInstanceID(raw_pad);
        }
    }
    if(event.type==SDL_JOYDEVICEADDED && !raw_pad &&
       !SDL_IsGameController(event.jdevice.which)) {
        raw_pad=SDL_JoystickOpen(event.jdevice.which);
        if(raw_pad)raw_instance=SDL_JoystickInstanceID(raw_pad);
    }
    if(event.type==SDL_CONTROLLERDEVICEREMOVED && pad &&
       SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad))==event.cdevice.which) {
        release_all();SDL_GameControllerClose(pad);pad=nullptr;
    }
    if(event.type==SDL_JOYDEVICEREMOVED && raw_pad && raw_instance==event.jdevice.which) {
        release_all();SDL_JoystickClose(raw_pad);raw_pad=nullptr;raw_instance=-1;
    }
    if(event.type==SDL_WINDOWEVENT && event.window.event==SDL_WINDOWEVENT_FOCUS_LOST)release_all();
    if(event.type==SDL_MOUSEMOTION)android_input_cursor_set(event.motion.x,event.motion.y);
    if(event.type==SDL_MOUSEBUTTONDOWN || event.type==SDL_MOUSEBUTTONUP) {
        if(event.button.button==SDL_BUTTON_LEFT)android_input_cursor_press(event.type==SDL_MOUSEBUTTONDOWN);
    }
    if(event.type==SDL_CONTROLLERBUTTONDOWN || event.type==SDL_CONTROLLERBUTTONUP) {
        const bool down=event.type==SDL_CONTROLLERBUTTONDOWN;
        const char *name=nullptr;
        switch(event.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A:name="a";break;
        case SDL_CONTROLLER_BUTTON_B:name="b";break;
        case SDL_CONTROLLER_BUTTON_X:name="x";break;
        case SDL_CONTROLLER_BUTTON_Y:name="y";break;
        case SDL_CONTROLLER_BUTTON_START:name="start";break;
        case SDL_CONTROLLER_BUTTON_BACK:name="select";break;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:name="l1";break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:name="r1";break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:name="up";break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:name="down";break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:name="left";break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:name="right";break;
        default:break;
        }
        if(name)android_input_inject_control(name,down);
    }
    if(event.type==SDL_CONTROLLERAXISMOTION) {
        float v=event.caxis.value/32767.f;
        if(std::fabs(v)<.2f)v=0;
        if(event.caxis.axis==SDL_CONTROLLER_AXIS_TRIGGERLEFT)
            set_shoulder_source(0,true,v>.5f);
        if(event.caxis.axis==SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
            set_shoulder_source(1,true,v>.5f);
        if(event.caxis.axis==SDL_CONTROLLER_AXIS_LEFTX)
            android_input_inject_stick("left",v,left_y);
        if(event.caxis.axis==SDL_CONTROLLER_AXIS_LEFTY)
            android_input_inject_stick("left",left_x,v);
        if(event.caxis.axis==SDL_CONTROLLER_AXIS_RIGHTX)
            android_input_inject_stick("right",v,right_y);
        if(event.caxis.axis==SDL_CONTROLLER_AXIS_RIGHTY)
            android_input_inject_stick("right",right_x,v);
    }
    if(event.type==SDL_JOYBUTTONDOWN || event.type==SDL_JOYBUTTONUP) {
        if(!controller_maps_button(event.jbutton.which,event.jbutton.button)) {
            const char *name=raw_button_control(event.jbutton.button);
            if(name)android_input_inject_control(name,event.type==SDL_JOYBUTTONDOWN);
        }
    }
    if(event.type==SDL_JOYHATMOTION &&
       !controller_maps_hat(event.jhat.which,event.jhat.hat)) {
        android_input_inject_control("up",(event.jhat.value&SDL_HAT_UP)!=0);
        android_input_inject_control("down",(event.jhat.value&SDL_HAT_DOWN)!=0);
        android_input_inject_control("left",(event.jhat.value&SDL_HAT_LEFT)!=0);
        android_input_inject_control("right",(event.jhat.value&SDL_HAT_RIGHT)!=0);
    }
    if(event.type==SDL_JOYAXISMOTION &&
       !controller_maps_axis(event.jaxis.which,event.jaxis.axis)) {
        float value=event.jaxis.value/32767.f;
        if(std::fabs(value)<.2f)value=0;
        switch(event.jaxis.axis) {
        case 0:raw_left_x=value;android_input_inject_stick("left",raw_left_x,raw_left_y);break;
        case 1:raw_left_y=value;android_input_inject_stick("left",raw_left_x,raw_left_y);break;
        case 2:raw_right_x=value;android_input_inject_stick("right",raw_right_x,raw_right_y);break;
        case 3:raw_right_y=value;android_input_inject_stick("right",raw_right_x,raw_right_y);break;
        case 4:set_shoulder_source(0,true,value>.5f);break;
        case 5:set_shoulder_source(1,true,value>.5f);break;
        default:break;
        }
    }
    if(event.type==SDL_KEYDOWN || event.type==SDL_KEYUP) {
        bool down=event.type==SDL_KEYDOWN;
        if(event.key.repeat)return;
        switch(event.key.keysym.sym) {
        case SDLK_ESCAPE:android_input_inject_control("back",down);break;
        case SDLK_RETURN:android_input_inject_control("a",down);break;
        case SDLK_LEFT:android_input_inject_control("left",down);break;
        case SDLK_RIGHT:android_input_inject_control("right",down);break;
        case SDLK_UP:android_input_inject_control("up",down);break;
        case SDLK_DOWN:android_input_inject_control("down",down);break;
        default:break;
        }
    }
}
void pop_input_frame(Uint32 now) {
    if(right_x || right_y) android_input_cursor_set(cx+right_x*8,cy+right_y*8);
    if(left_since && !left_running && now-left_since>=250) {key_down(env,(jclass)renderer,21);left_running=true;}
    if(right_since && !right_running && now-right_since>=250) {key_down(env,(jclass)renderer,22);right_running=true;}
}
