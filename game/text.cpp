// Copyright (c) 2026 Pixelforge Ports contributors
#include "native_bindings.h"
#include "so_util.h"
#include "fix_path.h"
#include "third_party/stb/stb_truetype.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

so_module *port_cocos_module();

struct Font {
    std::vector<unsigned char> bytes;
    stbtt_fontinfo info{};
};
static std::map<std::string,std::unique_ptr<Font>> fonts;

static Font *open_font(const char *requested) {
    std::string basename=requested?requested:"";
    basename=basename.substr(basename.find_last_of("/\\")+1);
    if(basename.empty())basename="UbiGameTextLReg.ttf";
    auto found=fonts.find(basename);
    if(found!=fonts.end())return found->second.get();
    const std::string path=std::string(io_game_dir())+"/apk-assets/Extra/font/"+basename;
    std::ifstream stream(path,std::ios::binary);
    if(!stream && basename!="UbiGameTextLReg.ttf")return open_font("UbiGameTextLReg.ttf");
    if(!stream) return nullptr;
    auto font=std::unique_ptr<Font>(new Font);
    font->bytes.assign(std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>());
    if(font->bytes.empty() || !stbtt_InitFont(&font->info,font->bytes.data(),0))return nullptr;
    auto result=font.get();
    fonts.emplace(basename,std::move(font));
    return result;
}

static unsigned next_utf8(const char *&cursor) {
    const unsigned char *p=reinterpret_cast<const unsigned char *>(cursor);
    unsigned code=*p++;
    if(code<0x80) {cursor=reinterpret_cast<const char *>(p);return code;}
    int extra=code<0xE0?1:code<0xF0?2:3;
    code&=(1u<<(6-extra))-1;
    while(extra-- && (*p&0xC0)==0x80)code=(code<<6)|(*p++&0x3F);
    cursor=reinterpret_cast<const char *>(p);
    return code;
}

void pop_text_bitmap(JNIEnv *env,jclass,jstring jtext,jstring jfont,jint size,jint align,jint width,jint height) {
    using Bitmap=donor::Cocos2dxBitmap_nativeInitBitmapDC_9;
    auto callback=reinterpret_cast<Bitmap>(so_symbol(port_cocos_module(),
        donor::Cocos2dxBitmap_nativeInitBitmapDC_9_symbol));
    if(!callback)return;
    const char *text=jtext?env->GetStringUTFChars(jtext,nullptr):nullptr;
    const char *font_name=jfont?env->GetStringUTFChars(jfont,nullptr):nullptr;
    Font *font=open_font(font_name);
    if(jfont)env->ReleaseStringUTFChars(jfont,font_name);
    if(!text || !font || size<=0 || size>256) {
        if(jtext && text)env->ReleaseStringUTFChars(jtext,text);
        callback(env,nullptr,0,0,nullptr);
        return;
    }
    const float scale=stbtt_ScaleForPixelHeight(&font->info,float(size));
    int ascent=0,descent=0,gap=0;
    stbtt_GetFontVMetrics(&font->info,&ascent,&descent,&gap);
    const int line_height=std::max(1,int((ascent-descent+gap)*scale));
    std::vector<std::vector<unsigned>> lines(1);
    std::vector<int> advances(1,0);
    const char *cursor=text;
    while(*cursor && lines.size()<128) {
        unsigned cp=next_utf8(cursor);
        if(cp=='\n') {lines.emplace_back();advances.push_back(0);continue;}
        int advance=0,bearing=0;
        stbtt_GetCodepointHMetrics(&font->info,int(cp),&advance,&bearing);
        int pixels=std::max(0,int(advance*scale));
        if(width>0 && advances.back()+pixels>width && !lines.back().empty()) {
            lines.emplace_back();advances.push_back(0);
        }
        lines.back().push_back(cp);
        advances.back()+=pixels;
    }
    if(width<=0)width=*std::max_element(advances.begin(),advances.end());
    if(height<=0)height=int(lines.size())*line_height;
    width=std::max(1,width);height=std::max(1,height);
    if(width>4096 || height>4096 || size_t(width)*height>16000000) {
        env->ReleaseStringUTFChars(jtext,text);
        callback(env,nullptr,0,0,nullptr);
        return;
    }
    std::vector<jbyte> rgba(size_t(width)*height*4,0);
    int top=0;
    if(((align>>4)&15)==3)top=(height-int(lines.size())*line_height)/2;
    else if(((align>>4)&15)==2)top=height-int(lines.size())*line_height;
    for(size_t line=0;line<lines.size();++line) {
        int pen=0;
        if((align&15)==3)pen=(width-advances[line])/2;
        else if((align&15)==2)pen=width-advances[line];
        const int baseline=top+int(line)*line_height+int(ascent*scale);
        for(unsigned cp:lines[line]) {
            int advance=0,bearing=0,x0=0,y0=0,x1=0,y1=0;
            stbtt_GetCodepointHMetrics(&font->info,int(cp),&advance,&bearing);
            stbtt_GetCodepointBitmapBox(&font->info,int(cp),scale,scale,&x0,&y0,&x1,&y1);
            const int gw=x1-x0,gh=y1-y0;
            if(gw>0 && gh>0 && gw<1024 && gh<1024) {
                std::vector<unsigned char> glyph(size_t(gw)*gh);
                stbtt_MakeCodepointBitmap(&font->info,glyph.data(),gw,gh,gw,scale,scale,int(cp));
                for(int gy=0;gy<gh;gy++)for(int gx=0;gx<gw;gx++) {
                    const int px=pen+x0+gx,py=baseline+y0+gy;
                    if(px<0 || px>=width || py<0 || py>=height)continue;
                    const unsigned char a=glyph[size_t(gy)*gw+gx];
                    jbyte *out=&rgba[(size_t(py)*width+px)*4];
                    out[0]=out[1]=out[2]=out[3]=jbyte(a);
                }
            }
            pen+=int(advance*scale);
        }
    }
    env->ReleaseStringUTFChars(jtext,text);
    jbyteArray bytes=env->NewByteArray(jsize(rgba.size()));
    if(bytes)env->SetByteArrayRegion(bytes,0,jsize(rgba.size()),rgba.data());
    callback(env,nullptr,width,height,bytes);
    if(bytes)env->DeleteLocalRef(bytes);
}
