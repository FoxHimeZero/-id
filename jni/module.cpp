#include <sys/types.h>
#include <android/log.h>
#include <dlfcn.h>
#include <pthread.h>
#include <fstream>
#include <string>
#include <cstdio>

extern "C" {
    void A64HookFunction(void *const symbol, void *const replace, void **result);
}

#define LOG_TAG "WidevineSpoof"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// SHA256 的上下文结构（我们只当它是一个"盒子"，具体里面什么样不重要）
struct SHA256_CTX_OPAQUE;

static int (*orig_SHA256_Init)  (void*) = nullptr;
static int (*orig_SHA256_Update)(void*, const void*, size_t) = nullptr;
static int (*orig_SHA256_Final) (unsigned char*, void*) = nullptr;

static unsigned char g_fakeId[16];
static bool g_idLoaded = false;
static pthread_mutex_t g_idLock = PTHREAD_MUTEX_INITIALIZER;

// 从文件读取伪造 ID
static bool loadFakeId() {
    pthread_mutex_lock(&g_idLock);
    if (g_idLoaded) { pthread_mutex_unlock(&g_idLock); return true; }

    const char* paths[] = {
        "/data/local/tmp/widevine-spoof/custom_id",
        "/data/adb/drmid_spoof/custom_id",
        "/data/adb/all-id-spoof/drm_id.txt",
    };
    std::ifstream f;
    const char* used = nullptr;
    for (auto p : paths) {
        f.open(p);
        if (f.good()) { used = p; break; }
        f.clear();
    }
    if (!f.is_open()) {
        LOGE("找不到 ID 文件");
        pthread_mutex_unlock(&g_idLock);
        return false;
    }
    std::string hex;
    f >> hex;
    if (hex.size() < 32) {
        LOGE("ID 长度不对");
        pthread_mutex_unlock(&g_idLock);
        return false;
    }
    for (int i = 0; i < 16; i++) {
        unsigned int b = 0;
        sscanf(hex.substr(i*2, 2).c_str(), "%02x", &b);
        g_fakeId[i] = (unsigned char)b;
    }
    g_idLoaded = true;
    LOGI("已加载 ID @ %s", used);
    pthread_mutex_unlock(&g_idLock);
    return true;
}

// 记录每个 SHA256 盒子是否刚被"清空"过
struct Slot { void* ctx; int state; };
static Slot g_slots[256];
static pthread_mutex_t g_slotLock = PTHREAD_MUTEX_INITIALIZER;

static Slot* slotFind(void* ctx) {
    for (auto& s : g_slots) if (s.ctx == ctx) return &s;
    for (auto& s : g_slots) if (!s.ctx) { s.ctx = ctx; s.state = 0; return &s; }
    return nullptr;
}
static void slotClear(void* ctx) {
    for (auto& s : g_slots) if (s.ctx == ctx) { s.ctx = nullptr; s.state = 0; return; }
}

// Hook SHA256_Init：门神清空搅拌机，我们记下"这个盒子刚开工"
static int hook_SHA256_Init(void* ctx) {
    if (ctx) {
        pthread_mutex_lock(&g_slotLock);
        if (auto* s = slotFind(ctx)) s->state = 1;
        pthread_mutex_unlock(&g_slotLock);
    }
    return orig_SHA256_Init(ctx);
}

// Hook SHA256_Final：门神要出锅了，我们偷偷加一勺料
static int hook_SHA256_Final(unsigned char* md, void* ctx) {
    if (!ctx) return orig_SHA256_Final(md, ctx);

    bool doInject = false;
    if (loadFakeId()) {
        pthread_mutex_lock(&g_slotLock);
        if (auto* s = slotFind(ctx)) {
            if (s->state == 1) {
                s->state = 2;   // 只注入一次，防止污染其它功能
                doInject = true;
            }
        }
        pthread_mutex_unlock(&g_slotLock);
    }

    if (doInject) {
        LOGI("注入 16 字节");
        orig_SHA256_Update(ctx, g_fakeId, 16);
    }

    int rc = orig_SHA256_Final(md, ctx);

    pthread_mutex_lock(&g_slotLock);
    slotClear(ctx);
    pthread_mutex_unlock(&g_slotLock);
    return rc;
}

// 找 libcrypto.so
static void* tryLoadLibcrypto() {
    const char* names[] = {
        "libcrypto.so", "libcrypto.so.3", "libcrypto.so.1.1",
        "libcrypto.so.1.0.0", "libboringcrypto.so",
    };
    for (auto n : names) {
        void* h = dlopen(n, RTLD_NOW | RTLD_NOLOAD);
        if (h) { LOGI("找到 %s", n); return h; }
    }
    for (auto n : names) {
        void* h = dlopen(n, RTLD_NOW);
        if (h) { LOGI("加载 %s", n); return h; }
    }
    return nullptr;
}

// 库被加载时自动执行
__attribute__((constructor))
static void init_hook() {
    LOGI("开始初始化");
    void* handle = tryLoadLibcrypto();
    if (!handle) { LOGE("找不到 libcrypto"); return; }

    void* init_addr   = dlsym(handle, "SHA256_Init");
    void* update_addr = dlsym(handle, "SHA256_Update");
    void* final_addr  = dlsym(handle, "SHA256_Final");

    if (!init_addr || !update_addr || !final_addr) {
        LOGE("符号没找到");
        return;
    }
    LOGI("Init=%p Update=%p Final=%p", init_addr, update_addr, final_addr);

    A64HookFunction(init_addr,  (void*)hook_SHA256_Init,  (void**)&orig_SHA256_Init);
    A64HookFunction(final_addr, (void*)hook_SHA256_Final, (void**)&orig_SHA256_Final);
    orig_SHA256_Update = (int(*)(void*, const void*, size_t))update_addr;

    if (!orig_SHA256_Init || !orig_SHA256_Final) {
        LOGE("Hook 失败");
        return;
    }

    loadFakeId();
    LOGI("Hook 完成");
}
