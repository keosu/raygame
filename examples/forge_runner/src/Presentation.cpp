#include "Presentation.h"

namespace Forge {
namespace {
bool HasUnicode(const std::string& text) { return std::any_of(text.begin(),text.end(),[](unsigned char c){return c>=128;}); }
// Noto's ascender/descender box is taller than its CJK em square. Compensate so
// a requested 24px label has a readable 24px character body, with a 16px floor.
float TextSize(const std::string& text,float size) { return HasUnicode(text)?std::max(16.0f,size)*1.4f:size; }
}
float Presentation::Random(float min, float max) { random ^= random<<13; random ^= random>>17; random ^= random<<5; return min + (max-min)*(random%10000)/10000.0f; }
void Presentation::Init(const std::string& assets) {
    catalog.Load(assets+"/locales/zh-CN.tsv");
    chineseGlyphs=catalog.ChineseCodepoints();fontDirectory=assets+"/fonts/";
    ChineseFont(15);ChineseFont(24,true);
    body = RayGame::ResourceManager::LoadFontResource("forge-body",assets+"/fonts/Barlow-Medium.ttf",72);
    title = RayGame::ResourceManager::LoadFontResource("forge-title",assets+"/fonts/Rajdhani-SemiBold.ttf",128);
    if (!body || !title) throw std::runtime_error("FORGE fonts are missing from the assets directory");
    SetTextureFilter(body->texture,TEXTURE_FILTER_BILINEAR); SetTextureFilter(title->texture,TEXTURE_FILTER_BILINEAR);
    glow = RayGame::ResourceManager::LoadTexture("forge-glow",assets+"/glow.png");
    if (!glow) throw std::runtime_error("FORGE lighting texture is missing");
    particles.reserve(512);
}
float Presentation::Measure(const std::string& value, float size, bool heading) const {
    auto text=Localize(value);size=TextSize(text,size);
    auto font=FontFor(text,size,heading);
    return font ? MeasureTextEx(*font,text.c_str(),size,HasUnicode(text)?0.4f:1.0f).x : (float)MeasureText(text.c_str(),(int)size);
}
void Presentation::Text(const std::string& value, Vector2 position, float size, Color color, bool heading) {
    auto text=Localize(value);
    if(HasUnicode(text)) position.y-=std::max(16.0f,size)*0.18f;
    size=TextSize(text,size);
    auto font=FontFor(text,size,heading);
    if (font) DrawTextEx(*font,text.c_str(),position,size,HasUnicode(text)?0.4f:1.0f,color);
    else DrawText(text.c_str(),(int)position.x,(int)position.y,(int)size,color);
}
RayGame::FontHandle Presentation::ChineseFont(float size,bool heading) const {
    int rasterSize=(int)std::ceil(size*2);auto key=std::make_pair(rasterSize,heading);
    auto& font=chineseFonts[key];
    if(!font) {
        font=RayGame::ResourceManager::LoadFontResource("forge-sc-"+std::to_string(rasterSize)+(heading?"-title":"-body"),
            fontDirectory+(heading?"ForgeSC-SemiBold.ttf":"ForgeSC-Medium.ttf"),rasterSize,chineseGlyphs);
        if(!font)throw std::runtime_error("FORGE SC fonts are missing or invalid");
        SetTextureFilter(font->texture,TEXTURE_FILTER_BILINEAR);
    }
    return font;
}
RayGame::FontHandle Presentation::FontFor(const std::string& text,float size,bool heading) const {
    return HasUnicode(text)?ChineseFont(size,heading):(heading?title:body);
}
void Presentation::Glow(Vector2 p,float radius,Color color,float alpha) {
    if (!glow) return;
    DrawTexturePro(*glow,{0,0,(float)glow->width,(float)glow->height},{p.x-radius,p.y-radius,radius*2,radius*2},{0,0},0,Fade(color,alpha));
}
void Presentation::Burst(Vector2 p,Color c,int count,float speed) {
    for(int i=0;i<count && particles.size()<512;++i) {
        float angle = Random(0,2*PI), velocity = Random(speed*0.3f,speed);
        particles.push_back({p,{std::cos(angle)*velocity,std::sin(angle)*velocity},0,Random(0.3f,0.85f),Random(1,3),c});
    }
}
void Presentation::React(unsigned event,Vector2 p) {
    if(event & Jump) Burst(p,Muted,12,100);
    if(event & Dash) { Burst({p.x,p.y-20},Mint,24,150); shake=std::max(shake,2.0f); }
    if(event & Land) { Burst(p,Ember,14,130); landing=0.18f; shake=std::max(shake,1.6f); }
    if(event & Cell) { Burst({p.x,p.y-24},Mint,32,170); flash=0.04f; }
    if(event & Checkpoint) { Burst(p,Mint,45,230); flash=0.08f; }
    if(event & Death) { Burst({p.x,p.y-22},Red,60,300); shake=9; flash=0.13f; }
    if(event & Finish) { Burst({p.x,p.y-36},Mint,90,330); flash=0.15f; }
}
void Presentation::Update(float dt) {
    clock+=dt; shake=std::max(0.0f,shake-dt*22); flash=std::max(0.0f,flash-dt); landing=std::max(0.0f,landing-dt);
    for(auto& p:particles) { p.age+=dt; p.velocity.y+=200*dt; p.position.x+=p.velocity.x*dt; p.position.y+=p.velocity.y*dt; }
    particles.erase(std::remove_if(particles.begin(),particles.end(),[](const Spark& p){return p.age>=p.life;}),particles.end());
}
void Presentation::Follow(const World& w,Vector2 p,float dt,bool snap) {
    float width=RayGame::Display::GetWidth(),height=RayGame::Display::GetHeight();
    camera.offset={width/2,height*0.52f};
    Vector2 destination{Clamp(p.x+w.player.velocity.x*0.25f,width/2,w.level.size.x-width/2),
                         Clamp(p.y-120,height*0.52f,w.level.size.y-height*0.48f)};
    camera.target=snap ? destination : Vector2Lerp(camera.target,destination,1-std::exp(-dt*5));
}
void Presentation::Background(int theme) {
    float width=RayGame::Display::GetWidth(),height=RayGame::Display::GetHeight();
    DrawRectangleGradientV(0,0,(int)width,(int)height,{11,23,30,255},{24,41,47,255});
    Color accent=theme==2?Ember:Mint;
    // Distant roof trusses, monolithic tanks and middle-ground pipework.
    for(int layer=0;layer<3;++layer) {
        float speed=0.1f+layer*0.13f,spacing=layer==2?360.0f:470.0f;
        float offset=std::fmod(camera.target.x*speed,spacing);
        Color metal=layer==0?Color{17,32,38,255}:layer==1?Color{23,43,49,255}:Color{28,49,54,255};
        for(int i=-1;i<(int)(width/spacing)+2;++i) {
            float x=i*spacing-offset,top=height*(0.2f+layer*0.12f);
            DrawRectangleRec({x+40,top,180+layer*25.0f,height},metal);
            DrawRectangleLinesEx({x+40,top,180+layer*25.0f,height},2,Fade(Muted,0.09f));
            DrawRectangleRec({x+64,top-14,132,16},metal);
            for(int band=0;band<6;++band) {
                float y=top+38+band*95;
                DrawRectangleRec({x+40,y,180+layer*25.0f,4},Fade(Ink,0.5f));
                DrawRectangleRec({x+74,y+18,6,31},Fade(accent,layer==2?0.15f:0.07f));
            }
            if(layer==2) {
                DrawRectangleRec({x+265,top-160,16,height+200},metal);
                DrawRectangleRec({x+265,top+140,95,16},metal);
                DrawLineEx({x+45,top-60},{x+280,top-60},5,metal);
                DrawLineEx({x+45,top-60},{x+140,top-150},4,metal);
                DrawLineEx({x+140,top-150},{x+280,top-60},4,metal);
            }
        }
    }
    // Suspended fixtures break up the large machinery silhouettes.
    for(int i=-1;i<5;++i) {
        float x=i*420.0f-std::fmod(camera.target.x*0.22f,420.0f);
        DrawLineEx({x+230,0},{x+230,190},2,{53,71,72,255});
        DrawRectangleRec({x+207,185,46,9},{64,81,76,255});
        Glow({x+230,207},160,Ember,0.08f);
        DrawTriangle({x+222,197},{x+125,height*0.88f},{x+334,height*0.88f},Fade(Ember,0.025f));
    }
    for(int i=0;i<18;++i) {
        float x=std::fmod(i*83.7f+clock*(8+i%3)*0.7f+width,width);
        float y=std::fmod(i*73.1f-clock*(4+i%5)+height*10,height);
        DrawCircleV({x,y},i%3==0?2.0f:1.0f,Fade(accent,0.12f));
    }
    Glow({width*0.75f,height*0.65f},480,accent,0.13f);
    DrawRectangleGradientV(0,0,(int)width,120,Fade(Ink,0.65f),Fade(Ink,0));
}
void Presentation::DrawPlatform(const Platform& p,float time,int theme) {
    auto r=p.At(time); Color edge=p.oneWay?Mint:Ember;
    if(r.x+r.width<camera.target.x-900 || r.x>camera.target.x+900) return;
    if(p.oneWay) {
        DrawRectangleRec({r.x,r.y,r.width,r.height},{45,63,65,255});
        DrawRectangleRec({r.x,r.y,r.width,3},Fade(Mint,0.8f));
        DrawRectangleRec({r.x+6,r.y+5,r.width-12,4},{19,33,39,255});
        for(float x=r.x+12;x<r.x+r.width-8;x+=24) DrawLineEx({x,r.y+12},{x+12,r.y+5},2,Muted);
        if(p.travel.x || p.travel.y) {
            DrawLineEx({p.rect.x+p.rect.width/2,p.rect.y-230},{p.rect.x+p.rect.width/2,p.rect.y+60},3,Fade(Muted,0.3f));
            Glow({r.x+r.width/2,r.y+15},65,Mint,0.12f);
        }
        return;
    }
    DrawRectangleRec(r,{31,45,49,255});
    DrawRectangleRec({r.x,r.y+12,r.width,r.height-12},{23,34,40,255});
    for(float x=r.x+8;x<r.x+r.width-20;x+=80) {
        float w=std::min(72.0f,r.x+r.width-x-4);
        DrawRectangleLinesEx({x,r.y+30,w,104},1,{44,59,62,255});
        DrawCircleV({x+5,r.y+36},2,Muted); DrawCircleV({x+w-5,r.y+124},2,Fade(Muted,0.5f));
        for(int k=0;k<4;++k) DrawLineEx({x+16,r.y+54+k*12.0f},{x+w-14,r.y+54+k*12.0f},2,{17,27,33,255});
    }
    DrawRectangleRec({r.x,r.y,r.width,4},edge);
    DrawRectangleRec({r.x,r.y+4,r.width,8},{53,65,57,255});
    for(float x=r.x;x+20<r.x+r.width;x+=40) DrawLineEx({x+6,r.y+9},{x+18,r.y+4},3,{23,32,32,255});
    DrawRectangleRec({r.x,r.y+12,r.width,2},Ink);
    if(r.height>150) {
        for(float x=r.x+150;x<r.x+r.width-60;x+=250) {
            Glow({x,r.y+55},80,theme==2?Ember:Mint,0.13f);
            DrawRectangleRec({x-30,r.y+26,60,4},Fade(theme==2?Ember:Mint,0.7f));
        }
    }
}
void Presentation::DrawRunner(Vector2 p,const Runner& s,float scale,float alpha) {
    auto q=[&](float x,float y){return Vector2{p.x+x*scale*s.facing,p.y+y*scale};};
    auto line=[&](float x1,float y1,float x2,float y2,float thick,Color c){DrawLineEx(q(x1,y1),q(x2,y2),thick*scale,Fade(c,alpha));};
    float run=clock*17, stride=s.grounded?std::sin(run)*std::min(10.0f,std::fabs(s.velocity.x)/35):4;
    float squash=landing>0?3:0;
    if(s.dashTime>0) {
        Glow(q(-28,-22),65,Mint,alpha*0.25f);
        for(int i=0;i<4;++i) line(-18-i*14,-20+i*3,-50-i*14,-20+i*3,3,Mint);
    }
    // Scarf, articulated boots, pressure suit, brass helmet and luminous visor.
    line(-7,-32,-25,-34+std::sin(clock*8)*4,5,Red);
    line(-25,-34,-36,-27+std::sin(clock*7)*4,3,Ember);
    line(-6,-17+squash,-8+stride,-3,7,{46,65,67,255});
    line(6,-17+squash,8-stride,-3,7,{80,94,85,255});
    line(-8+stride,-3,-2+stride,-2,6,Ink); line(8-stride,-3,15-stride,-2,6,Ink);
    DrawRectangleRounded({p.x-13*scale,p.y+(-35+squash)*scale,26*scale,(22-squash)*scale},0.2f,6,Fade({35,49,49,255},alpha));
    DrawRectangleRounded({p.x-11*scale,p.y+(-35+squash)*scale,22*scale,(19-squash)*scale},0.2f,6,Fade({208,177,115,255},alpha));
    line(-11,-30,-17,-19,6,{49,65,65,255}); line(11,-29,18,-22+stride*0.3f,6,{180,152,106,255});
    DrawRectangleRec({p.x-7*scale,p.y-30*scale,14*scale,9*scale},Fade({34,59,58,255},alpha));
    DrawCircleV(q(0,-42+squash),13*scale,Fade({222,190,126,255},alpha));
    DrawCircleV(q(-1,-43+squash),10*scale,Fade({234,211,146,255},alpha));
    line(-8,-47+squash,8,-47+squash,5,{244,222,161,255});
    DrawRectangleRounded({p.x+(s.facing>0?-1:-12)*scale,p.y+(-44+squash)*scale,15*scale,8*scale},0.3f,6,Fade(Mint,alpha));
    Glow(q(7,-40+squash),26,Mint,0.22f*alpha);
    DrawCircleV(q(-6,-42+squash),3*scale,Fade({83,107,98,255},alpha));
    line(-8,-22+squash,8,-22+squash,2,{237,218,159,255});
    DrawRectangleRec({p.x-2*scale,p.y-26*scale,4*scale,5*scale},Fade(Mint,alpha));
    if(s.grounded) DrawEllipse((int)p.x,(int)(p.y+2*scale),18*scale,3*scale,Fade(Ink,alpha*0.5f));
}
void Presentation::DrawWorld(const World& w,Vector2 runner,bool menu) {
    Background(w.level.index);
    Camera2D view=camera;
    if(settings.shake && !menu) { view.offset.x+=std::sin(clock*111)*shake; view.offset.y+=std::cos(clock*97)*shake*0.6f; }
    BeginMode2D(view);
    for(const auto& p:w.level.platforms) DrawPlatform(p,w.time,w.level.index);
    for(auto h:w.level.hazards) {
        DrawRectangleRec(h,{71,34,28,255});
        for(float x=h.x;x<h.x+h.width;x+=16) DrawRectangleRec({x,h.y,8,h.height},Fade(Ember,0.3f+std::sin(clock*3+x)*0.1f));
        Glow({h.x+h.width/2,h.y},h.width,Ember,0.16f);
    }
    for(const auto& s:w.level.signs) {
        Text(s.text,s.position,13,Fade(Muted,0.75f));
        DrawLineEx({s.position.x,s.position.y+23},{s.position.x+120,s.position.y+23},1,Fade(Muted,0.25f));
    }
    for(size_t i=0;i<w.level.checkpoints.size();++i) {
        auto p=w.level.checkpoints[i]; bool active=(int)i<=w.checkpoint;
        DrawRectangleRec({p.x-8,p.y-76,16,76},Steel); DrawRectangleRec({p.x-19,p.y-7,38,7},Muted);
        DrawRectangleRec({p.x-6,p.y-70,12,32},active?Mint:Ember);
        Glow({p.x,p.y-52},70,active?Mint:Ember,active?0.22f:0.1f);
        if(active) { DrawCircleLines((int)p.x,(int)(p.y-52),25,Fade(Mint,0.3f)); }
    }
    for(size_t i=0;i<w.level.cells.size();++i) if(!(w.collected & (std::uint64_t{1}<<i))) {
        auto p=w.level.cells[i]; p.y+=std::sin(clock*3+i)*5;
        Glow(p,65,Mint,0.22f); DrawCircleLines((int)p.x,(int)p.y,21,Fade(Mint,0.3f));
        DrawPoly(p,4,13,45,{42,83,72,255}); DrawPoly(p,4,8,45,Mint);
        DrawCircleV(p,3,Light);
    }
    for(auto l:w.level.lasers) {
        bool active=l.Active(w.time); auto r=l.rect;
        DrawRectangleRec({r.x-10,r.y-12,r.width+20,14},Steel);
        DrawRectangleRec({r.x-10,r.y+r.height-2,r.width+20,12},Steel);
        if(active) {
            DrawRectangleRec({r.x-9,r.y,r.width+18,r.height},Fade(Red,0.12f));
            DrawRectangleRec({r.x+5,r.y,r.width-10,r.height},Red);
            DrawLineEx({r.x+r.width/2,r.y},{r.x+r.width/2,r.y+r.height},2,Light);
            Glow({r.x+r.width/2,r.y+r.height/2},110,Red,0.12f);
        } else {
            for(float y=r.y;y<r.y+r.height;y+=18) DrawRectangleRec({r.x+7,y,2,7},Fade(Red,0.25f));
        }
        DrawCircleV({r.x+r.width/2,r.y-7},3,active?Red:Mint);
    }
    for(auto d:w.level.drones) {
        auto p=d.At(w.time); Glow(p,65,Red,0.17f);
        DrawCircleV(p,22,Steel); DrawCircleLines((int)p.x,(int)p.y,23,Muted);
        DrawLineEx({p.x-35,p.y-2},{p.x+35,p.y-2},6,{47,67,72,255});
        DrawCircleV(p,8,Red); DrawCircleV({p.x+2,p.y-2},3,Light);
        DrawLineEx({p.x-32,p.y-10},{p.x-18,p.y-10},2,Mint); DrawLineEx({p.x+18,p.y-10},{p.x+32,p.y-10},2,Mint);
    }
    auto g=w.level.goal; bool unlocked=w.CellCount()==(int)w.level.cells.size();
    DrawRectangleRec({g.x-42,g.y-126,84,126},{29,49,51,255});
    DrawRectangleLinesEx({g.x-45,g.y-130,90,130},4,Muted);
    DrawRectangleRec({g.x-26,g.y-112,52,112},unlocked?Fade(Mint,0.25f):Ink);
    for(int i=0;i<6;++i) DrawRectangleRec({g.x-22,g.y-103+i*16.0f,44,2},unlocked?Mint:Fade(Muted,0.4f));
    Glow({g.x,g.y-70},130,unlocked?Mint:Ember,0.2f);
    Text(unlocked?"CORE ONLINE":"CELLS REQUIRED",{g.x-55,g.y-158},14,unlocked?Mint:Ember);
    if(w.deathTimer<=0) DrawRunner(runner,w.player);
    for(const auto& p:particles) {
        float a=1-p.age/p.life; DrawLineEx(p.position,{p.position.x-p.velocity.x*0.025f,p.position.y-p.velocity.y*0.025f},p.size,Fade(p.color,a));
    }
    EndMode2D();
    // Display's quantized target draws its HUD in logical canvas units.
    rlMatrixMode(RL_PROJECTION); rlLoadIdentity();
    rlOrtho(0,RayGame::Display::GetWidth(),RayGame::Display::GetHeight(),0,RL_CULL_DISTANCE_NEAR,RL_CULL_DISTANCE_FAR);
    rlMatrixMode(RL_MODELVIEW); rlLoadIdentity();
    float width=RayGame::Display::GetWidth(),height=RayGame::Display::GetHeight();
    DrawRectangleGradientV(0,(int)(height-90),(int)width,90,Fade(Ink,0),Fade(Ink,0.5f));
    if(flash>0) DrawRectangle(0,0,(int)width,(int)height,Fade(Mint,flash*0.35f));
}
}
