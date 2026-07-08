// ============================================================
//  سباق الشوارع — BMW X6
//  محرك اللعبة بلغة C++ باستخدام SDL2 (يترجم إلى WebAssembly)
//  المنطق والرسم كله هنا؛ الواجهة العربية والأصوات في shell.html
// ============================================================
#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// ---------------- جسر JavaScript ----------------
EM_JS(void, js_state,  (const char* s), { uiState(UTF8ToString(s)); });
EM_JS(void, js_hud,    (int score, int coins, int kmh, float nitro, int nitroOn),
      { uiHud(score, coins, kmh, nitro, nitroOn); });
EM_JS(void, js_count,  (int n), { uiCount(n); });
EM_JS(void, js_over,   (int score, int coins, int dist10, int best, int isBest),
      { uiOver(score, coins, dist10, best, isBest); });
EM_JS(void, js_popup,  (float x, float y, int type), { uiPopup(x, y, type); });
EM_JS(void, js_sfx,    (int id), { sfxEvent(id); });
EM_JS(void, js_engine, (float ratio, int on), { sfxEngine(ratio, on); });
EM_JS(int,  js_get_best, (), { return +(localStorage.getItem('bmwRaceCpp_best') || 0); });
EM_JS(void, js_set_best, (int v), { localStorage.setItem('bmwRaceCpp_best', v); });
EM_JS(int,  js_inner_w, (), { return innerWidth; });
EM_JS(int,  js_inner_h, (), { return innerHeight; });

// ---------------- أدوات عامة ----------------
struct Col { Uint8 r, g, b, a; };

static Col hexCol(unsigned hex, Uint8 a = 255) {
    return Col{ Uint8(hex >> 16 & 255), Uint8(hex >> 8 & 255), Uint8(hex & 255), a };
}
static Col shade(Col c, float f) {
    auto s = [&](Uint8 v) {
        int x = int(std::lround(v + 255.f * f));
        return Uint8(std::max(0, std::min(255, x)));
    };
    return Col{ s(c.r), s(c.g), s(c.b), c.a };
}
static Col lerpCol(Col a, Col b, float t) {
    auto L = [&](Uint8 x, Uint8 y) { return Uint8(x + (y - x) * t); };
    return Col{ L(a.r, b.r), L(a.g, b.g), L(a.b, b.b), L(a.a, b.a) };
}
static float frand(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }
static float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

// ---------------- الثوابت والحالة ----------------
static const float LH = 800.f;      // الارتفاع المنطقي
static const float ROAD_W = 440.f;  // عرض الطريق
static const int   LANES = 4;
static const float LANE_W = ROAD_W / LANES;

static SDL_Window*   win = nullptr;
static SDL_Renderer* ren = nullptr;

static float gscale = 1, logicalW = 520, logicalH = 800, roadX = 40;
static float shakeX = 0, shakeY = 0;

enum State { ST_MENU, ST_COUNT, ST_PLAY, ST_CRASH, ST_OVER, ST_PAUSE };
static State state = ST_MENU;

static struct {
    float t = 0, speed = 0, baseSpeed = 300;
    float dist = 0, score = 0;
    int   coins = 0;
    float nitro = 100; bool nitroOn = false;
    float roadOffset = 0, shake = 0, crashT = 0, countT = 0;
    float spawnT = 0, coinT = 0, exhaustT = 0;
} game;

static struct {
    float x = 0, y = 0, w = 84, h = 172, targetX = 0, tilt = 0;
    int color = 0;
} player;

struct Car      { int lane; float x, y, w, h, speed; bool passed; int spr; };
struct CoinObj  { float x, y, spin; };
struct Deco     { float x, y; int type; bool flip; float s; };
struct Particle { float x, y, vx, vy, life, max, size, grav; Col c; };

static std::vector<Car>      traffic;
static std::vector<CoinObj>  coinsV;
static std::vector<Deco>     decos;
static std::vector<Particle> parts;
static float laneSpeeds[LANES];

static bool keyLeft = false, keyRight = false, keyNitroKb = false;
static bool nitroHeld = false, dragging = false;
static int  lastCount = 99;

// ---------------- أدوات رسم (بالبكسل، تستخدم عند تجهيز الرسوم) ----------------
static void setC(Col c) { SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a); }

static void fillRectPx(float x, float y, float w, float h, Col c) {
    setC(c);
    SDL_FRect r{ x, y, w, h };
    SDL_RenderFillRectF(ren, &r);
}

// مستطيل بزوايا دائرية (نصف قطر علوي rt وسفلي rb) — رسم بالأسطر
static void fillRRect(float x, float y, float w, float h, float rt, float rb, Col c) {
    setC(c);
    int rows = (int)std::ceil(h);
    for (int i = 0; i < rows; i++) {
        float yy = i + 0.5f, inset = 0;
        if (yy < rt) {
            float dy = rt - yy;
            inset = rt - std::sqrt(std::max(0.f, rt * rt - dy * dy));
        } else if (yy > h - rb) {
            float dy = yy - (h - rb);
            inset = rb - std::sqrt(std::max(0.f, rb * rb - dy * dy));
        }
        SDL_FRect r{ x + inset, y + i, w - 2 * inset, 1.f };
        SDL_RenderFillRectF(ren, &r);
    }
}

static void fillCirclePx(float cx, float cy, float rad, Col c) {
    setC(c);
    int rows = (int)std::ceil(rad * 2);
    for (int i = 0; i < rows; i++) {
        float dy = (i + 0.5f) - rad;
        float dx = std::sqrt(std::max(0.f, rad * rad - dy * dy));
        SDL_FRect r{ cx - dx, cy - rad + i, dx * 2, 1.f };
        SDL_RenderFillRectF(ren, &r);
    }
}

// شبه منحرف (زجاج السيارات)
static void fillTrap(float xt1, float xt2, float yt, float xb1, float xb2, float yb, Col c) {
    setC(c);
    int rows = (int)std::ceil(yb - yt);
    for (int i = 0; i < rows; i++) {
        float t = (i + 0.5f) / (yb - yt);
        float x1 = xt1 + (xb1 - xt1) * t;
        float x2 = xt2 + (xb2 - xt2) * t;
        SDL_FRect r{ x1, yt + i, x2 - x1, 1.f };
        SDL_RenderFillRectF(ren, &r);
    }
}

// طلاء معدني: تدرج أفقي بخمس درجات (حواف غامقة ووسط لامع)
static float metalF(float t) {
    const float ts[5] = { 0, 0.28f, 0.5f, 0.72f, 1 };
    const float fs[5] = { -0.18f, 0.12f, 0.22f, 0.12f, -0.18f };
    for (int i = 1; i < 5; i++)
        if (t <= ts[i]) {
            float u = (t - ts[i - 1]) / (ts[i] - ts[i - 1]);
            return fs[i - 1] + (fs[i] - fs[i - 1]) * u;
        }
    return fs[4];
}
static void metalRRect(float x, float y, float w, float h, float rt, float rb, Col base) {
    int cols = (int)std::ceil(w);
    for (int i = 0; i < cols; i++) {
        float xm = i + 0.5f;
        setC(shade(base, metalF(xm / w)));
        float d = std::min(xm, w - xm), topIn = 0, botIn = 0;
        if (d < rt) { float dx = rt - d; topIn = rt - std::sqrt(std::max(0.f, rt * rt - dx * dx)); }
        if (d < rb) { float dx = rb - d; botIn = rb - std::sqrt(std::max(0.f, rb * rb - dx * dx)); }
        SDL_FRect r{ x + i, y + topIn, 1.f, h - topIn - botIn };
        SDL_RenderFillRectF(ren, &r);
    }
}
// تدرج أفقي بسيط: حافة → وسط → حافة (حاوية الشاحنة)
static void grad3RRect(float x, float y, float w, float h, float rt, float rb, Col edge, Col mid) {
    int cols = (int)std::ceil(w);
    for (int i = 0; i < cols; i++) {
        float xm = i + 0.5f;
        float t = 1.f - std::fabs(xm / w - 0.5f) * 2.f;
        setC(lerpCol(edge, mid, t));
        float d = std::min(xm, w - xm), topIn = 0, botIn = 0;
        if (d < rt) { float dx = rt - d; topIn = rt - std::sqrt(std::max(0.f, rt * rt - dx * dx)); }
        if (d < rb) { float dx = rb - d; botIn = rb - std::sqrt(std::max(0.f, rb * rb - dx * dx)); }
        SDL_FRect r{ x + i, y + topIn, 1.f, h - topIn - botIn };
        SDL_RenderFillRectF(ren, &r);
    }
}

// ---------------- تجهيز الرسوم (Textures) ----------------
static const float SPR_K = 3.f;  // دقة الرسم الداخلية

static SDL_Texture* beginTarget(int w, int h) {
    SDL_Texture* t = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_TARGET, w, h);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(ren, t);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 0);
    SDL_RenderClear(ren);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    return t;
}
static void endTarget() { SDL_SetRenderTarget(ren, nullptr); }

// العجلات (مشتركة بين السيارات)
static void drawWheelsK(float k, float w, float h) {
    Col dark = hexCol(0x0a0c0f);
    float px[4][2] = { {-3, h * 0.14f}, {w - 11, h * 0.14f}, {-3, h * 0.68f}, {w - 11, h * 0.68f} };
    for (auto& p : px)
        fillRRect(p[0] * k, p[1] * k, 14 * k, h * 0.17f * k, 4 * k, 4 * k, dark);
}

// —— سيارة اللاعب BMW X6 —— (بوحدات 88×180)
static void drawX6(float k, float w, float h, Col color) {
    drawWheelsK(k, w, h);
    // إطار غامق ثم الطلاء المعدني
    fillRRect(4.6f * k, 2.6f * k, (w - 9.2f) * k, (h - 5.2f) * k, 27 * k, 19 * k, hexCol(0x000000, 120));
    metalRRect(6 * k, 4 * k, (w - 12) * k, (h - 8) * k, 26 * k, 18 * k, color);
    // المرايات
    Col mir = shade(color, -0.1f);
    fillRRect(0, h * 0.30f * k, 8 * k, 11 * k, 3 * k, 3 * k, mir);
    fillRRect((w - 8) * k, h * 0.30f * k, 8 * k, 11 * k, 3 * k, 3 * k, mir);
    // الزجاج الأمامي + انعكاس
    Col glass = hexCol(0x16202e);
    fillTrap(13 * k, (w - 13) * k, h * 0.32f * k, 18 * k, (w - 18) * k, h * 0.44f * k, glass);
    fillTrap(16 * k, w * 0.50f * k, h * 0.33f * k, 20 * k, w * 0.42f * k, h * 0.43f * k,
             Col{ 140, 190, 255, 56 });
    // السقف وفتحة السقف
    fillRRect(16 * k, h * 0.44f * k, (w - 32) * k, h * 0.34f * k, 9 * k, 9 * k, shade(color, -0.06f));
    fillRRect(24 * k, h * 0.47f * k, (w - 48) * k, h * 0.13f * k, 5 * k, 5 * k, hexCol(0x10161f));
    // الزجاج الخلفي المائل (طراز الكوبيه)
    fillTrap(19 * k, (w - 19) * k, h * 0.78f * k, 14 * k, (w - 14) * k, h * 0.90f * k, glass);
    // خطوط الكبوت
    Col ln = hexCol(0x000000, 55);
    fillRectPx(22 * k, h * 0.08f * k, 1.6f * k, h * 0.21f * k, ln);
    fillRectPx((w - 23.6f) * k, h * 0.08f * k, 1.6f * k, h * 0.21f * k, ln);
    // شبك BMW (الكلى) بإطار كروم
    fillRRect((w / 2 - 14) * k, 4 * k, 13 * k, 10 * k, 3 * k, 3 * k, hexCol(0x9aa7b5));
    fillRRect((w / 2 + 1) * k, 4 * k, 13 * k, 10 * k, 3 * k, 3 * k, hexCol(0x9aa7b5));
    fillRRect((w / 2 - 13) * k, 5 * k, 11 * k, 8 * k, 2.5f * k, 2.5f * k, hexCol(0x0c0f13));
    fillRRect((w / 2 + 2) * k, 5 * k, 11 * k, 8 * k, 2.5f * k, 2.5f * k, hexCol(0x0c0f13));
    // المصابيح الأمامية
    fillRRect(10 * k, 5 * k, 15 * k, 7 * k, 3 * k, 3 * k, hexCol(0xdff2ff));
    fillRRect((w - 25) * k, 5 * k, 15 * k, 7 * k, 3 * k, 3 * k, hexCol(0xdff2ff));
    fillRectPx(11 * k, 10 * k, 13 * k, 2 * k, hexCol(0x7fc4ff));
    fillRectPx((w - 24) * k, 10 * k, 13 * k, 2 * k, hexCol(0x7fc4ff));
    // المصابيح الخلفية
    fillRRect(10 * k, (h - 9) * k, 20 * k, 5 * k, 2.5f * k, 2.5f * k, hexCol(0xe02020));
    fillRRect((w - 30) * k, (h - 9) * k, 20 * k, 5 * k, 2.5f * k, 2.5f * k, hexCol(0xe02020));
    fillRectPx(30 * k, (h - 8) * k, (w - 60) * k, 3 * k, Col{ 255, 80, 60, 128 });
}

// —— سيارة عادية / تكسي —— (بوحدات 78×145)
static void drawSedan(float k, float w, float h, Col color, bool taxi) {
    drawWheelsK(k, w, h);
    fillRRect(4.8f * k, 2.8f * k, (w - 9.6f) * k, (h - 5.6f) * k, 21 * k, 17 * k, hexCol(0x000000, 110));
    metalRRect(6 * k, 4 * k, (w - 12) * k, (h - 8) * k, 20 * k, 16 * k, color);
    Col glass = hexCol(0x16202e);
    fillTrap(13 * k, (w - 13) * k, h * 0.28f * k, 17 * k, (w - 17) * k, h * 0.42f * k, glass);
    fillRRect(15 * k, h * 0.42f * k, (w - 30) * k, h * 0.30f * k, 7 * k, 7 * k, shade(color, -0.05f));
    fillTrap(17 * k, (w - 17) * k, h * 0.72f * k, 13 * k, (w - 13) * k, h * 0.85f * k, glass);
    if (taxi) {
        fillRRect((w / 2 - 12) * k, h * 0.50f * k, 24 * k, 10 * k, 3 * k, 3 * k, hexCol(0x111111));
        fillRectPx((w / 2 - 8) * k, (h * 0.50f + 3) * k, 16 * k, 4 * k, hexCol(0xffd75e));
    }
    fillRRect(10 * k, 5 * k, 13 * k, 6 * k, 3 * k, 3 * k, hexCol(0xdff2ff));
    fillRRect((w - 23) * k, 5 * k, 13 * k, 6 * k, 3 * k, 3 * k, hexCol(0xdff2ff));
    fillRRect(10 * k, (h - 9) * k, 15 * k, 5 * k, 2 * k, 2 * k, hexCol(0xe02020));
    fillRRect((w - 25) * k, (h - 9) * k, 15 * k, 5 * k, 2 * k, 2 * k, hexCol(0xe02020));
}

// —— شاحنة —— (بوحدات 92×235)
static void drawTruck(float k, float w, float h, Col color) {
    drawWheelsK(k, w, h);
    Col dark = hexCol(0x0a0c0f);
    fillRRect(-3 * k, h * 0.42f * k, 14 * k, h * 0.14f * k, 4 * k, 4 * k, dark);
    fillRRect((w - 11) * k, h * 0.42f * k, 14 * k, h * 0.14f * k, 4 * k, 4 * k, dark);
    metalRRect(8 * k, 4 * k, (w - 16) * k, h * 0.26f * k, 14 * k, 4 * k, color);
    fillTrap(14 * k, (w - 14) * k, h * 0.12f * k, 17 * k, (w - 17) * k, h * 0.21f * k, hexCol(0x16202e));
    grad3RRect(5 * k, h * 0.30f * k, (w - 10) * k, h * 0.66f * k, 5 * k, 5 * k,
               hexCol(0x8d97a3), hexCol(0xc3ccd6));
    Col ln = hexCol(0x000000, 50);
    for (int i = 1; i < 5; i++)
        fillRectPx(8 * k, (h * 0.30f + h * 0.66f * i / 5) * k, (w - 16) * k, 1.2f * k, ln);
    fillRRect(11 * k, 5 * k, 12 * k, 6 * k, 3 * k, 3 * k, hexCol(0xdff2ff));
    fillRRect((w - 23) * k, 5 * k, 12 * k, 6 * k, 3 * k, 3 * k, hexCol(0xdff2ff));
    fillRectPx(8 * k, (h - 8) * k, 16 * k, 4 * k, hexCol(0xe02020));
    fillRectPx((w - 24) * k, (h - 8) * k, 16 * k, 4 * k, hexCol(0xe02020));
}

// —— باص —— (بوحدات 90×265)
static void drawBus(float k, float w, float h, Col color) {
    drawWheelsK(k, w, h);
    Col dark = hexCol(0x0a0c0f);
    fillRRect(-3 * k, h * 0.44f * k, 14 * k, h * 0.13f * k, 4 * k, 4 * k, dark);
    fillRRect((w - 11) * k, h * 0.44f * k, 14 * k, h * 0.13f * k, 4 * k, 4 * k, dark);
    fillRRect(4.8f * k, 2.8f * k, (w - 9.6f) * k, (h - 5.6f) * k, 13 * k, 13 * k, hexCol(0x000000, 110));
    metalRRect(6 * k, 4 * k, (w - 12) * k, (h - 8) * k, 12 * k, 12 * k, color);
    fillTrap(12 * k, (w - 12) * k, h * 0.08f * k, 15 * k, (w - 15) * k, h * 0.16f * k, hexCol(0x16202e));
    for (int i = 0; i < 3; i++)
        fillRRect((w / 2 - 14) * k, h * (0.24f + i * 0.22f) * k, 28 * k, h * 0.10f * k,
                  4 * k, 4 * k, Col{ 15, 22, 32, 230 });
    fillRectPx(10 * k, h * 0.20f * k, (w - 20) * k, 1.2f * k, Col{ 255, 255, 255, 64 });
    fillRRect(10 * k, 5 * k, 13 * k, 6 * k, 3 * k, 3 * k, hexCol(0xdff2ff));
    fillRRect((w - 23) * k, 5 * k, 13 * k, 6 * k, 3 * k, 3 * k, hexCol(0xdff2ff));
    fillRectPx(9 * k, (h - 8) * k, 16 * k, 4 * k, hexCol(0xe02020));
    fillRectPx((w - 25) * k, (h - 8) * k, 16 * k, 4 * k, hexCol(0xe02020));
}

// ---------------- بناء الرسوم ----------------
static SDL_Texture* playerTex[4];
static const Col PLAYER_COLS[4] = {
    hexCol(0xe9edf2), hexCol(0x23272e), hexCol(0x1a5fb4), hexCol(0xb01820)
};

struct TrafSpr { int type; float w, h; SDL_Texture* tex; };
static std::vector<TrafSpr> trafSprs;
static SDL_Texture *treeTex, *lampTex, *bushTex, *coinTex, *softTex;

static SDL_Texture* makeVehicleTex(int type, float w, float h, Col col) {
    SDL_Texture* t = beginTarget((int)(w * SPR_K), (int)(h * SPR_K));
    if (type == 0) drawSedan(SPR_K, w, h, col, false);
    else if (type == 1) drawSedan(SPR_K, w, h, hexCol(0xf2c012), true);
    else if (type == 2) drawSedan(SPR_K, w, h, col, false);
    else if (type == 3) drawTruck(SPR_K, w, h, col);
    else if (type == 4) drawBus(SPR_K, w, h, col);
    else drawX6(SPR_K, w, h, col);
    endTarget();
    return t;
}

static void buildSprites() {
    // اللاعب بأربعة ألوان
    for (int i = 0; i < 4; i++) {
        playerTex[i] = beginTarget((int)(88 * SPR_K), (int)(180 * SPR_K));
        drawX6(SPR_K, 88, 180, PLAYER_COLS[i]);
        endTarget();
    }
    // المرور
    const unsigned tc[] = { 0xb02525, 0x2456a8, 0x3c8a46, 0x7c4fb0, 0xc2c8cf, 0x31363e, 0xc47a1d, 0x7d2f57 };
    for (int i = 0; i < 6; i++) trafSprs.push_back({ 0, 78, 145, makeVehicleTex(0, 78, 145, hexCol(tc[i])) });
    trafSprs.push_back({ 1, 78, 145, makeVehicleTex(1, 78, 145, hexCol(0xf2c012)) });
    for (int i = 0; i < 4; i++) trafSprs.push_back({ 2, 84, 162, makeVehicleTex(2, 84, 162, hexCol(tc[(i + 3) % 8])) });
    for (int i = 0; i < 3; i++) trafSprs.push_back({ 3, 92, 235, makeVehicleTex(3, 92, 235, hexCol(tc[(i * 2 + 1) % 8])) });
    trafSprs.push_back({ 4, 90, 265, makeVehicleTex(4, 90, 265, hexCol(0x2456a8)) });
    trafSprs.push_back({ 4, 90, 265, makeVehicleTex(4, 90, 265, hexCol(0x3c8a46)) });

    // شجرة (90×100)
    treeTex = beginTarget(270, 300);
    {
        float k = SPR_K, w = 90, h = 100;
        fillRectPx((w / 2 - 5) * k, h * 0.55f * k, 10 * k, h * 0.40f * k, hexCol(0x5a3d22));
        const unsigned lay[3] = { 0x173e1f, 0x1e5429, 0x2a6e36 };
        const float rr[3] = { 40, 32, 24 };
        for (int i = 0; i < 3; i++)
            fillCirclePx(w / 2 * k, (h * 0.42f - i * 8) * k, rr[i] * k, hexCol(lay[i]));
        fillCirclePx((w / 2 - 10) * k, h * 0.28f * k, 12 * k, Col{ 255, 255, 255, 30 });
    }
    endTarget();

    // عمود إنارة (60×110)
    lampTex = beginTarget(180, 330);
    {
        float k = SPR_K, w = 60, h = 110;
        Col pole = hexCol(0x3a4350);
        fillRectPx((w / 2 - 3) * k, 12 * k, 6 * k, (h - 14) * k, pole);
        fillRRect((w / 2 - 3) * k, 8 * k, 30 * k, 6 * k, 3 * k, 3 * k, pole);
        for (int i = 0; i < 3; i++)
            fillCirclePx((w / 2 + 24) * k, 11 * k, (22 - i * 6) * k, Col{ 255, 233, 168, Uint8(18 + i * 10) });
        fillCirclePx((w / 2 + 24) * k, 11 * k, 6 * k, hexCol(0xffe9a8));
    }
    endTarget();

    // شجيرة (60×40)
    bushTex = beginTarget(180, 120);
    {
        float k = SPR_K, w = 60, h = 40;
        Col g = hexCol(0x1c4d26);
        fillCirclePx(w * 0.30f * k, h * 0.65f * k, 13 * k, g);
        fillCirclePx(w * 0.60f * k, h * 0.55f * k, 15 * k, g);
        fillCirclePx(w * 0.78f * k, h * 0.70f * k, 10 * k, g);
        fillCirclePx(w * 0.55f * k, h * 0.45f * k, 8 * k, Col{ 255, 255, 255, 20 });
    }
    endTarget();

    // عملة ذهبية (40×40)
    coinTex = beginTarget(120, 120);
    {
        float k = SPR_K, cx = 20, cy = 20;
        fillCirclePx(cx * k, cy * k, 17.5f * k, hexCol(0xa86f00));
        Col cA = hexCol(0xfff3b0), cB = hexCol(0xffd23f), cC = hexCol(0xd99a00);
        for (int i = 0; i <= 16; i++) {
            float t = i / 16.f;                    // 0 = خارجي، 1 = مركز
            Col c = t < 0.45f ? lerpCol(cC, cB, t / 0.45f) : lerpCol(cB, cA, (t - 0.45f) / 0.55f);
            float r = 16.f * (1 - t) + 2.f * t;
            fillCirclePx((cx - 3 * t) * k, (cy - 3 * t) * k, r * k, c);
        }
        fillCirclePx(cx * k, cy * k, 11.5f * k, Col{ 255, 255, 255, 150 });
        fillCirclePx(cx * k, cy * k, 10.0f * k, cB);
    }
    endTarget();

    // نسيج دائري ناعم (للظلال والجزيئات)
    softTex = beginTarget(96, 96);
    for (int i = 0; i < 26; i++)
        fillCirclePx(48, 48, 46.f * (1 - i / 26.f) + 2, Col{ 255, 255, 255, 16 });
    endTarget();
}

// تدرجات الخلفية (نسيج 256×1 يمدد على الشاشة)
static SDL_Texture *grassGrad, *roadGrad;
static SDL_Texture* makeHGrad(Col edge, Col mid) {
    SDL_Texture* t = beginTarget(256, 1);
    for (int i = 0; i < 256; i++) {
        float f = 1.f - std::fabs(i / 255.f - 0.5f) * 2.f;
        fillRectPx((float)i, 0, 1, 1, lerpCol(edge, mid, f));
    }
    endTarget();
    return t;
}

// ---------------- رسم عالمي (مع الاهتزاز) ----------------
static void wRect(float x, float y, float w, float h, Col c) {
    fillRectPx(x + shakeX, y + shakeY, w, h, c);
}
static void wTex(SDL_Texture* t, float x, float y, float w, float h,
                 float angleDeg = 0, bool flip = false) {
    SDL_FRect dst{ x + shakeX, y + shakeY, w, h };
    SDL_RenderCopyExF(ren, t, nullptr, &dst, angleDeg, nullptr,
                      flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}
static void wSoft(float cx, float cy, float rx, float ry, Col tint, Uint8 alpha) {
    SDL_SetTextureColorMod(softTex, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(softTex, alpha);
    SDL_FRect dst{ cx - rx + shakeX, cy - ry + shakeY, rx * 2, ry * 2 };
    SDL_RenderCopyF(ren, softTex, nullptr, &dst);
    SDL_SetTextureColorMod(softTex, 255, 255, 255);
    SDL_SetTextureAlphaMod(softTex, 255);
}

// ---------------- منطق اللعبة ----------------
static float laneCenter(int i) { return roadX + LANE_W * (i + 0.5f); }

static void addDeco(float y) {
    bool right = rand() & 1;
    float rn = frand(0, 1);
    int type = rn < 0.5f ? 0 : (rn < 0.78f ? 1 : 2);   // شجرة / شجيرة / عمود
    float s = frand(0.8f, 1.25f);
    float w = (type == 0 ? 90.f : 60.f) * s;
    float sideW = (logicalW - ROAD_W) / 2;
    // العمود يلتصق بحافة الطريق وذراعه فوقها؛ الأشجار والشجيرات خلف الرصيف
    float margin = type == 2 ? frand(-24, -16) : frand(4, std::max(8.f, sideW - w - 10));
    float x = right ? roadX + ROAD_W + 16 + margin : roadX - 16 - margin - w;
    decos.push_back({ x, y, type, right, s });
}
static void seedDecos() {
    decos.clear();
    for (float y = -100; y < logicalH + 100; y += frand(70, 150)) addDeco(y);
}

static const float TYPE_P[5] = { 4.f, 1.2f, 2.f, 1.2f, 0.8f };
static int pickTrafSpr() {
    float tot = 0; for (float p : TYPE_P) tot += p;
    float r = frand(0, tot);
    int type = 0;
    for (int i = 0; i < 5; i++) { r -= TYPE_P[i]; if (r <= 0) { type = i; break; } }
    std::vector<int> idx;
    for (int i = 0; i < (int)trafSprs.size(); i++) if (trafSprs[i].type == type) idx.push_back(i);
    return idx[rand() % idx.size()];
}

static void spawnTraffic() {
    int nearLanes = 0;
    bool seen[LANES] = {};
    for (auto& t : traffic)
        if (t.y < 260 && !seen[t.lane]) { seen[t.lane] = true; nearLanes++; }
    if (nearLanes >= LANES - 1) return;
    std::vector<int> freeL;
    for (int i = 0; i < LANES; i++) {
        bool busy = false;
        for (auto& t : traffic) if (t.lane == i && t.y < 200) { busy = true; break; }
        if (!busy) freeL.push_back(i);
    }
    if (freeL.empty()) return;
    int lane = freeL[rand() % freeL.size()];
    int spr = pickTrafSpr();
    Car c;
    c.lane = lane; c.spr = spr;
    c.w = trafSprs[spr].w * 0.98f; c.h = trafSprs[spr].h * 0.98f;
    c.x = laneCenter(lane);
    c.y = -300 - c.h;
    c.speed = laneSpeeds[lane] * frand(0.92f, 1.08f);
    c.passed = false;
    traffic.push_back(c);
}

static void spawnCoins() {
    std::vector<int> freeL;
    for (int i = 0; i < LANES; i++) {
        bool busy = false;
        for (auto& t : traffic) if (t.lane == i && t.y < 150) { busy = true; break; }
        if (!busy) freeL.push_back(i);
    }
    if (freeL.empty()) return;
    int lane = freeL[rand() % freeL.size()];
    int n = 3 + rand() % 3;
    for (int i = 0; i < n; i++)
        coinsV.push_back({ laneCenter(lane), -320.f - i * 95.f, frand(0, 6) });
}

static void burst(float x, float y, int n, const std::vector<Col>& cols, float spd, float size) {
    for (int i = 0; i < n; i++) {
        float a = frand(0, 6.2832f), v = frand(spd * 0.3f, spd);
        Particle p;
        p.x = x; p.y = y;
        p.vx = std::cos(a) * v; p.vy = std::sin(a) * v;
        p.max = 1; p.life = frand(0.4f, 1.f);
        p.size = frand(size * 0.5f, size);
        p.c = cols[rand() % cols.size()];
        p.grav = 60;
        parts.push_back(p);
    }
}

static void doCrash(float cx, float cy) {
    state = ST_CRASH;
    game.crashT = 1.35f;
    game.shake = 1;
    burst(cx, cy, 26, { hexCol(0xff5e2e), hexCol(0xffd23f), hexCol(0xff9d2e), hexCol(0x8d97a3), hexCol(0xffffff) }, 420, 9);
    burst(cx, cy, 14, { hexCol(0x2b2f36), hexCol(0x555c66) }, 240, 12);
    js_sfx(3);
    js_engine(0, 0);
    js_state("crash");
}

static void gameOver() {
    int s = (int)game.score;
    int best = js_get_best();
    bool isBest = s > best;
    if (isBest) { js_set_best(s); best = s; }
    js_over(s, game.coins, (int)(game.dist / 500), best, isBest);
    state = ST_OVER;
    js_state("over");
}

static void updateDecos(float speed, float dt) {
    for (int i = (int)decos.size() - 1; i >= 0; i--) {
        decos[i].y += speed * dt;
        if (decos[i].y > logicalH + 150) decos.erase(decos.begin() + i);
    }
    float minY = 1e9f;
    for (auto& d : decos) minY = std::min(minY, d.y);
    if (minY > -60) addDeco(minY - frand(70, 160));
    if ((int)decos.size() < 6) addDeco(-frand(50, 300));
}

static void updateParticles(float dt) {
    for (int i = (int)parts.size() - 1; i >= 0; i--) {
        Particle& p = parts[i];
        p.x += p.vx * dt;
        p.y += p.vy * dt + (state == ST_PLAY ? game.speed * dt * 0.35f : 0);
        p.vy += p.grav * dt;
        p.life -= dt;
        if (p.life <= 0) parts.erase(parts.begin() + i);
    }
}

static void update(float dt) {
    if (state == ST_MENU) {
        game.roadOffset += 260 * dt;
        updateDecos(260, dt);
        return;
    }
    if (state == ST_COUNT) {
        game.countT -= dt;
        int n = (int)std::ceil(game.countT - 0.5f);
        if (n < lastCount && n >= 0) { lastCount = n; js_sfx(n == 0 ? 5 : 4); js_count(n); }
        game.speed = game.baseSpeed * clampf(1 - game.countT / 3.5f + 0.3f, 0.3f, 1.f);
        game.roadOffset += game.speed * dt;
        updateDecos(game.speed, dt);
        js_engine(0.2f, 1);
        if (game.countT <= 0) {
            state = ST_PLAY;
            js_count(-1);
            js_state("play");
        }
        return;
    }
    if (state == ST_CRASH) {
        game.crashT -= dt;
        game.shake = std::max(0.f, game.shake - dt * 1.2f);
        updateParticles(dt);
        if (game.crashT <= 0) gameOver();
        return;
    }
    if (state != ST_PLAY) return;

    game.t += dt;

    // السرعة والنيترو
    float ramp = std::min(430.f, game.t * 9);
    bool wantNitro = (nitroHeld || keyNitroKb) && game.nitro > 1;
    if (wantNitro && !game.nitroOn) js_sfx(2);
    game.nitroOn = wantNitro;
    if (game.nitroOn) game.nitro = std::max(0.f, game.nitro - 32 * dt);
    else game.nitro = std::min(100.f, game.nitro + 7 * dt);
    float target = (game.baseSpeed + ramp) * (game.nitroOn ? 1.5f : 1.f);
    game.speed += (target - game.speed) * std::min(1.f, dt * 2.4f);
    game.roadOffset += game.speed * dt;
    game.dist += game.speed * dt;
    game.score += game.speed * dt * (game.nitroOn ? 0.022f : 0.013f);

    // حركة اللاعب
    int dir = (keyRight ? 1 : 0) - (keyLeft ? 1 : 0);
    if (dir) player.targetX = player.x + dir * 300;
    float minX = roadX + 14 + player.w / 2, maxX = roadX + ROAD_W - 14 - player.w / 2;
    player.targetX = clampf(player.targetX, minX, maxX);
    float dx = (player.targetX - player.x) * std::min(1.f, dt * 9);
    player.x += dx;
    player.tilt += ((dx / std::max(dt, 0.001f)) * 0.00045f - player.tilt) * std::min(1.f, dt * 10);
    player.tilt = clampf(player.tilt, -0.22f, 0.22f);
    player.y = logicalH - 190;

    // عادم / لهب نيترو
    game.exhaustT -= dt;
    if (game.exhaustT <= 0) {
        game.exhaustT = game.nitroOn ? 0.016f : 0.05f;
        float ex = player.x + frand(-16, 16), ey = player.y + player.h / 2 + 4;
        Particle p;
        p.x = ex; p.y = ey; p.grav = 0;
        if (game.nitroOn) {
            p.vx = frand(-25, 25); p.vy = frand(220, 380);
            p.max = 0.3f; p.life = frand(0.15f, 0.3f);
            p.size = frand(7, 14);
            const unsigned fc[4] = { 0xff9d2e, 0xffd23f, 0x5fb8ff, 0xff5e2e };
            p.c = hexCol(fc[rand() % 4]);
        } else {
            p.vx = frand(-12, 12); p.vy = frand(60, 110);
            p.max = 0.6f; p.life = frand(0.3f, 0.6f);
            p.size = frand(5, 10);
            p.c = Col{ 160, 168, 180, 90 };
        }
        parts.push_back(p);
    }

    // توليد المرور والعملات
    float diff = clampf(game.t / 55.f, 0, 1);
    game.spawnT -= dt;
    if (game.spawnT <= 0) {
        game.spawnT = frand(0.55f, 1.05f) - diff * 0.3f;
        spawnTraffic();
    }
    game.coinT -= dt;
    if (game.coinT <= 0) {
        game.coinT = frand(1.6f, 3.2f);
        spawnCoins();
    }
    if (frand(0, 1) < dt * 0.1f) laneSpeeds[rand() % LANES] = frand(130, 245);

    // المرور
    for (int i = (int)traffic.size() - 1; i >= 0; i--) {
        Car& t = traffic[i];
        // مطابقة سرعة السيارة الأمامية في نفس الممر
        Car* ahead = nullptr;
        for (auto& o : traffic)
            if (&o != &t && o.lane == t.lane && o.y > t.y && (!ahead || o.y < ahead->y)) ahead = &o;
        float sp = t.speed;
        if (ahead && ahead->y - t.y < t.h / 2 + ahead->h / 2 + 60) sp = std::max(sp, ahead->speed);
        t.y += (game.speed - sp) * dt;
        if (t.y > logicalH + 300) { traffic.erase(traffic.begin() + i); continue; }

        // مرور خطر
        if (!t.passed && t.y > player.y + 20) {
            t.passed = true;
            float gap = std::fabs(t.x - player.x) - (t.w + player.w) / 2;
            if (gap < 26 && gap > -10) {
                game.score += 15;
                js_popup(player.x * gscale, (player.y - 40) * gscale, 1);
                js_sfx(1);
            }
        }

        // تصادم
        const float pad = 0.82f;
        if (std::fabs(t.x - player.x) < (t.w + player.w) / 2 * pad &&
            std::fabs(t.y - player.y) < (t.h + player.h) / 2 * pad) {
            doCrash((t.x + player.x) / 2, (t.y + player.y) / 2);
            return;
        }
    }

    // العملات
    for (int i = (int)coinsV.size() - 1; i >= 0; i--) {
        CoinObj& c = coinsV[i];
        c.y += game.speed * dt;
        c.spin += dt * 6;
        if (c.y > logicalH + 60) { coinsV.erase(coinsV.begin() + i); continue; }
        if (std::fabs(c.x - player.x) < player.w / 2 + 22 &&
            std::fabs(c.y - player.y) < player.h / 2 + 22) {
            game.coins++;
            game.score += 10;
            js_popup(c.x * gscale, c.y * gscale, 0);
            burst(c.x, c.y, 7, { hexCol(0xffd23f), hexCol(0xfff3b0), hexCol(0xf5a623) }, 160, 5);
            js_sfx(0);
            coinsV.erase(coinsV.begin() + i);
        }
    }

    updateDecos(game.speed, dt);
    updateParticles(dt);
    game.shake = std::max(0.f, game.shake - dt * 2);
    js_engine(clampf((game.speed - 200) / 700.f, 0, 1), 1);
}

// ---------------- الرسم ----------------
static void render() {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_RenderSetScale(ren, gscale, gscale);
    shakeX = game.shake > 0 ? frand(-1, 1) * game.shake * 14 : 0;
    shakeY = game.shake > 0 ? frand(-1, 1) * game.shake * 14 : 0;

    // العشب
    {
        SDL_FRect dst{ -20 + shakeX, -20 + shakeY, logicalW + 40, logicalH + 40 };
        SDL_RenderCopyF(ren, grassGrad, nullptr, &dst);
    }
    // خطوط حركة على العشب
    {
        float gOff = std::fmod(game.roadOffset, 160.f);
        for (float y = gOff - 160; y < logicalH + 40; y += 160)
            wRect(-20, y, logicalW + 40, 55, Col{ 255, 255, 255, 8 });
    }
    // الرصيف المخطط (أحمر/أبيض)
    {
        wRect(roadX - 16, -20, 16, logicalH + 40, hexCol(0x5d666f));
        wRect(roadX + ROAD_W, -20, 16, logicalH + 40, hexCol(0x5d666f));
        float cOff = std::fmod(game.roadOffset, 120.f);
        for (float y = cOff - 120; y < logicalH + 40; y += 120) {
            wRect(roadX - 16, y, 16, 60, hexCol(0xc8352b));
            wRect(roadX + ROAD_W, y, 16, 60, hexCol(0xc8352b));
            wRect(roadX - 16, y + 60, 16, 60, hexCol(0xe8e8e8));
            wRect(roadX + ROAD_W, y + 60, 16, 60, hexCol(0xe8e8e8));
        }
    }
    // الأسفلت
    {
        SDL_FRect dst{ roadX + shakeX, -20 + shakeY, ROAD_W, logicalH + 40 };
        SDL_RenderCopyF(ren, roadGrad, nullptr, &dst);
    }
    // الخطوط الجانبية
    wRect(roadX + 8, -20, 5, logicalH + 40, hexCol(0xdfe3e8));
    wRect(roadX + ROAD_W - 13, -20, 5, logicalH + 40, hexCol(0xdfe3e8));
    // خطوط الممرات المتقطعة
    {
        float dOff = std::fmod(game.roadOffset, 90.f);
        for (int l = 1; l < LANES; l++) {
            float x = roadX + LANE_W * l - 3;
            for (float y = dOff - 90; y < logicalH + 40; y += 90)
                wRect(x, y, 6, 45, hexCol(0xc9cdd3));
        }
    }
    // خطوط السرعة عند النيترو
    if (game.nitroOn && state == ST_PLAY) {
        float sOff = std::fmod(game.roadOffset * 2.2f, 220.f);
        for (float y = sOff - 220; y < logicalH; y += 220) {
            wRect(roadX + 26, y, 3, 90, Col{ 255, 255, 255, 26 });
            wRect(roadX + ROAD_W - 29, y, 3, 90, Col{ 255, 255, 255, 26 });
        }
    }
    // عناصر البيئة
    for (auto& d : decos) {
        SDL_Texture* t = d.type == 0 ? treeTex : d.type == 1 ? bushTex : lampTex;
        int tw, th;
        SDL_QueryTexture(t, nullptr, nullptr, &tw, &th);
        float w = tw / SPR_K * d.s, h = th / SPR_K * d.s;
        wSoft(d.x + w / 2, d.y + h * (d.type == 1 ? 0.6f : 0.9f), w * 0.45f, w * 0.18f,
              Col{ 0, 0, 0, 255 }, 90);
        wTex(t, d.x, d.y, w, h, 0, d.flip);
    }
    // العملات
    for (auto& c : coinsV) {
        float sq = std::fabs(std::cos(c.spin));
        float w = 40 * std::max(0.15f, sq);
        wSoft(c.x, c.y + 20, 14 * sq + 4, 6, Col{ 0, 0, 0, 255 }, 80);
        wTex(coinTex, c.x - w / 2, c.y - 20, w, 40);
    }
    // سيارات المرور
    for (auto& t : traffic) {
        TrafSpr& s = trafSprs[t.spr];
        wSoft(t.x, t.y + 6, s.w * 0.55f, s.h * 0.52f, Col{ 0, 0, 0, 255 }, 120);
        wTex(s.tex, t.x - s.w / 2, t.y - s.h / 2, s.w, s.h);
    }
    // اللاعب
    if (state != ST_CRASH || game.crashT > 0.9f) {
        wSoft(player.x, player.y + 6, player.w * 0.55f, player.h * 0.52f, Col{ 0, 0, 0, 255 }, 130);
        wTex(playerTex[player.color], player.x - 44, player.y - 90, 88, 180,
             player.tilt * 57.2958f);
    }
    // الجزيئات
    for (auto& p : parts) {
        float a = clampf(p.life / p.max, 0, 1);
        float sz = p.size * (0.4f + 0.6f * a);
        wSoft(p.x, p.y, sz, sz, p.c, Uint8(a * p.c.a));
    }

    SDL_RenderSetScale(ren, 1, 1);
}

// ---------------- الأحداث والحلقة ----------------
static void applySize() {
    int w = js_inner_w(), h = js_inner_h();
    SDL_SetWindowSize(win, w, h);
    gscale = std::min(h / LH, w / 520.f);
    logicalW = w / gscale;
    logicalH = h / gscale;
    roadX = (logicalW - ROAD_W) / 2;
}
static EM_BOOL onResize(int, const EmscriptenUiEvent*, void*) {
    applySize();
    return EM_TRUE;
}

static void handleKey(SDL_Scancode sc, bool down) {
    switch (sc) {
        case SDL_SCANCODE_LEFT: case SDL_SCANCODE_A: keyLeft = down; break;
        case SDL_SCANCODE_RIGHT: case SDL_SCANCODE_D: keyRight = down; break;
        case SDL_SCANCODE_SPACE: case SDL_SCANCODE_UP: case SDL_SCANCODE_W: keyNitroKb = down; break;
        case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_P:
            if (down) {
                if (state == ST_PLAY) { state = ST_PAUSE; js_engine(0, 0); js_state("pause"); }
                else if (state == ST_PAUSE) { state = ST_PLAY; js_state("play"); }
            }
            break;
        default: break;
    }
}

static void frame() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_KEYDOWN: handleKey(e.key.keysym.scancode, true); break;
            case SDL_KEYUP:   handleKey(e.key.keysym.scancode, false); break;
            case SDL_MOUSEBUTTONDOWN:
                dragging = true;
                if (state == ST_PLAY) player.targetX = e.button.x / gscale;
                break;
            case SDL_MOUSEMOTION:
                if (dragging && state == ST_PLAY) player.targetX = e.motion.x / gscale;
                break;
            case SDL_MOUSEBUTTONUP: dragging = false; break;
            case SDL_FINGERDOWN:
            case SDL_FINGERMOTION:
                if (state == ST_PLAY) player.targetX = e.tfinger.x * logicalW;
                break;
            default: break;
        }
    }

    static double last = emscripten_get_now();
    double now = emscripten_get_now();
    float dt = std::min(0.05f, float((now - last) / 1000.0));
    last = now;

    if (state != ST_PAUSE) update(dt);
    render();
    if (state == ST_COUNT || state == ST_PLAY || state == ST_CRASH)
        js_hud((int)game.score, game.coins, (int)std::lround(game.speed * 0.32f),
               game.nitro, game.nitroOn ? 1 : 0);
    SDL_RenderPresent(ren);
}

// ---------------- تحكم من الواجهة (JS → C++) ----------------
extern "C" {

EMSCRIPTEN_KEEPALIVE void start_game() {
    traffic.clear(); coinsV.clear(); parts.clear();
    game.t = 0; game.dist = 0; game.score = 0; game.coins = 0;
    game.nitro = 100; game.nitroOn = false;
    game.speed = 0; game.shake = 0;
    game.spawnT = 1; game.coinT = 2; game.countT = 3.5f;
    for (int i = 0; i < LANES; i++) laneSpeeds[i] = frand(130, 235);
    player.x = player.targetX = logicalW / 2;
    player.y = logicalH - 190;
    player.tilt = 0;
    seedDecos();
    lastCount = 99;
    state = ST_COUNT;
    js_state("count");
}

EMSCRIPTEN_KEEPALIVE void resume_game() {
    if (state == ST_PAUSE) { state = ST_PLAY; js_state("play"); }
}

EMSCRIPTEN_KEEPALIVE void pause_toggle() {
    if (state == ST_PLAY) { state = ST_PAUSE; js_engine(0, 0); js_state("pause"); }
    else if (state == ST_PAUSE) { state = ST_PLAY; js_state("play"); }
}

EMSCRIPTEN_KEEPALIVE void force_pause() {
    if (state == ST_PLAY) { state = ST_PAUSE; js_engine(0, 0); js_state("pause"); }
}

EMSCRIPTEN_KEEPALIVE void to_menu() {
    state = ST_MENU;
    js_engine(0, 0);
    js_state("menu");
    seedDecos();
}

EMSCRIPTEN_KEEPALIVE void set_color(int i) {
    player.color = std::max(0, std::min(3, i));
}

EMSCRIPTEN_KEEPALIVE void set_nitro(int on) { nitroHeld = on != 0; }

} // extern "C"

// ---------------- البداية ----------------
int main() {
    srand((unsigned)emscripten_get_now());
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    win = SDL_CreateWindow("street-race",
                           SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                           js_inner_w(), js_inner_h(), 0);
    ren = SDL_CreateRenderer(win, -1,
                             SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC |
                             SDL_RENDERER_TARGETTEXTURE);
    buildSprites();
    grassGrad = makeHGrad(hexCol(0x10331a), hexCol(0x17431f));
    roadGrad = makeHGrad(hexCol(0x272b31), hexCol(0x33383f));
    applySize();
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_FALSE, onResize);
    seedDecos();
    js_state("menu");
    emscripten_set_main_loop(frame, 0, 1);
    return 0;
}
