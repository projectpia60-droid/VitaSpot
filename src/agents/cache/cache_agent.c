#include "cache_agent.h"
#include "../../message_bus/bus.h"
#include "../../utils/logger.h"
#include "../../utils/http.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/sysmem.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ============================================================================
// Configuration
// ============================================================================

#define CACHE_DIR         "ux0:data/vitaspot/"
#define ALBUM_ART_DIR     "ux0:data/vitaspot/art/"
#define MAX_CACHED_IMAGES 50

// ============================================================================
// Image Cache Entry
// ============================================================================

typedef struct {
    char url[512];
    char local_path[256];
} ImageCacheEntry;

// ============================================================================
// Global State
// ============================================================================

static ImageCacheEntry g_image_cache[MAX_CACHED_IMAGES] = {0};
static int g_cache_count = 0;
static SceMutex g_cache_mutex = -1;
static int g_agent_running = 1;

// ============================================================================
// Helper Functions
// ============================================================================

static unsigned int simple_hash(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static int cache_ensure_directories(void) {
    // Crear directorio raíz
    sceIoMkdir(CACHE_DIR, 0777);
    // Crear directorio de imágenes
    sceIoMkdir(ALBUM_ART_DIR, 0777);
    return 0;
}

// ============================================================================
// Cache Operations
// ============================================================================

static int cache_get_image(const char *url, char *out_path, size_t path_len) {
    if (!url || !out_path || path_len < 256) {
        return -1;
    }

    sceKernelLockMutex(g_cache_mutex, 1, NULL);

    // Buscar en caché en memoria
    for (int i = 0; i < g_cache_count; i++) {
        if (strcmp(g_image_cache[i].url, url) == 0) {
            strncpy(out_path, g_image_cache[i].local_path, path_len - 1);
            sceKernelUnlockMutex(g_cache_mutex, 1);
            log_debug("[CacheAgent] Cache hit for image");
            bus_post(MSG_CACHE_HIT, out_path, strlen(out_path) + 1);
            return 0;
        }
    }

    sceKernelUnlockMutex(g_cache_mutex, 1);

    // No encontrado: descargar
    char filename[64];
    snprintf(filename, sizeof(filename), "%08x.jpg", simple_hash(url));
    char local_path[256];
    snprintf(local_path, sizeof(local_path), "%s%s", ALBUM_ART_DIR, filename);

    // Descargar archivo
    if (http_download_file(url, local_path) == 0) {
        // Agregar a caché en memoria
        sceKernelLockMutex(g_cache_mutex, 1, NULL);
        if (g_cache_count < MAX_CACHED_IMAGES) {
            strncpy(g_image_cache[g_cache_count].url, url, 511);
            strncpy(g_image_cache[g_cache_count].local_path, local_path, 255);
            g_cache_count++;
        }
        sceKernelUnlockMutex(g_cache_mutex, 1);

        strncpy(out_path, local_path, path_len - 1);
        log_debug("[CacheAgent] Image downloaded and cached");
        bus_post(MSG_CACHE_IMAGE_READY, out_path, strlen(out_path) + 1);
        return 0;
    }

    log_error("[CacheAgent] Failed to download image: %s", url);
    return -1;
}

// ============================================================================
// Cache Agent Main Thread
// ============================================================================

int cache_agent_thread(SceSize args, void *argp) {
    (void)args;
    (void)argp;

    log_info("[CacheAgent] Iniciando...");

    // Crear directorios necesarios
    cache_ensure_directories();

    // Loop principal: escuchar mensajes de caché
    Message msg;

    while (g_agent_running) {
        if (bus_receive(&msg, 100) != 0) {
            // Timeout, continuar
            continue;
        }

        switch (msg.type) {
            // Placeholder para mensajes futuros de caché
            default:
                break;
        }

        if (msg.payload) {
            free(msg.payload);
        }
    }

    log_info("[CacheAgent] Shutting down");
    return 0;
}

// ============================================================================
// Agent Management
// ============================================================================

SceUID cache_agent_start(void) {
    g_agent_running = 1;

    // Crear mutex para sincronización
    g_cache_mutex = sceKernelCreateMutex("cache_mutex", 0, 0, NULL);
    if (g_cache_mutex < 0) {
        log_error("[CacheAgent] Failed to create mutex");
        return -1;
    }

    SceUID thread = sceKernelCreateThread("cache_agent", cache_agent_thread,
                                           100, 16 * 1024, 0, 0, NULL);

    if (thread < 0) {
        log_error("[CacheAgent] Failed to create thread");
        sceKernelDeleteMutex(g_cache_mutex);
        return -1;
    }

    sceKernelStartThread(thread, 0, NULL);
    return thread;
}

int cache_agent_stop(SceUID thread_uid) {
    g_agent_running = 0;
    sceKernelWaitThreadEnd(thread_uid, NULL, NULL);
    sceKernelDeleteThread(thread_uid);

    if (g_cache_mutex >= 0) {
        sceKernelDeleteMutex(g_cache_mutex);
    }

    return 0;
}
