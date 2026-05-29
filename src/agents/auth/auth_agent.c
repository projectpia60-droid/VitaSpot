#include "auth_agent.h"
#include "../../message_bus/bus.h"
#include "../../spotify/spotify_auth.h"
#include "../../utils/logger.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/io/fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// Configuration
// ============================================================================

#define SPOTIFY_CLIENT_ID   "4bac12f38a2744a6ade4bbb242a44650"
#define SPOTIFY_SCOPE       "user-read-playback-state user-modify-playback-state user-read-currently-playing playlist-read-private"
#define TOKEN_FILE_PATH     "ux0:data/vitaspot/tokens.bin"
#define TOKEN_EXPIRY_BUFFER 60   // Refrescar 60s antes de que expire

// ============================================================================
// Global State
// ============================================================================

static SpotifyAuthState g_auth_state = {0};
static int g_agent_running = 1;
static int g_authenticated = 0;

// ============================================================================
// Token Persistence
// ============================================================================

static int auth_save_tokens_to_disk(const SpotifyAuthState *state) {
    if (!state) {
        return -1;
    }

    // Crear directorio si no existe
    sceIoMkdir("ux0:data/vitaspot", 0777);

    // Abrir/crear archivo
    SceUID fd = sceIoOpen(TOKEN_FILE_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0644);
    if (fd < 0) {
        log_error("[AuthAgent] Failed to open token file for writing");
        return -1;
    }

    // Escribir estructura binaria
    sceIoWrite(fd, (void *)state, sizeof(SpotifyAuthState));
    sceIoSync(fd, "");
    sceIoClose(fd);

    log_info("[AuthAgent] Tokens saved to disk");
    return 0;
}

static int auth_load_tokens_from_disk(SpotifyAuthState *state) {
    if (!state) {
        return -1;
    }

    // Intentar abrir archivo
    SceUID fd = sceIoOpen(TOKEN_FILE_PATH, SCE_O_RDONLY, 0);
    if (fd < 0) {
        log_warn("[AuthAgent] No saved tokens found");
        return -1;
    }

    // Leer estructura
    sceIoRead(fd, (void *)state, sizeof(SpotifyAuthState));
    sceIoClose(fd);

    log_info("[AuthAgent] Tokens loaded from disk");
    return 0;
}

// ============================================================================
// Auth Agent Main Thread
// ============================================================================

int auth_agent_thread(SceSize args, void *argp) {
    (void)args;
    (void)argp;

    log_info("[AuthAgent] Iniciando...");

    // 1. Intentar cargar tokens guardados
    if (auth_load_tokens_from_disk(&g_auth_state) == 0) {
        log_info("[AuthAgent] Tokens loaded from disk");

        // Verificar si están vencidos
        if (spotify_auth_is_token_expired(&g_auth_state, TOKEN_EXPIRY_BUFFER)) {
            log_info("[AuthAgent] Token expired, refreshing...");

            int new_expires_in;
            if (spotify_auth_refresh_token(SPOTIFY_CLIENT_ID, g_auth_state.refresh_token,
                                           g_auth_state.access_token, &new_expires_in) == 0) {
                g_auth_state.expires_in = new_expires_in;
                g_auth_state.token_acquired_at = time(NULL) * 1000;
                auth_save_tokens_to_disk(&g_auth_state);
                g_authenticated = 1;
                bus_post(MSG_AUTH_OK, g_auth_state.access_token,
                         strlen(g_auth_state.access_token) + 1);
            } else {
                log_warn("[AuthAgent] Failed to refresh token, starting device flow");
                bus_post(MSG_AUTH_FAILED, NULL, 0);
                goto device_flow;
            }
        } else {
            // Token aún válido
            g_authenticated = 1;
            bus_post(MSG_AUTH_OK, g_auth_state.access_token,
                     strlen(g_auth_state.access_token) + 1);
        }
    } else {
        device_flow:
        // 2. Iniciar Device Authorization Flow
        log_info("[AuthAgent] Starting Device Authorization Flow");

        if (spotify_auth_request_device_code(SPOTIFY_CLIENT_ID, SPOTIFY_SCOPE,
                                             &g_auth_state) != 0) {
            log_error("[AuthAgent] Failed to request device code");
            bus_post(MSG_AUTH_FAILED, NULL, 0);
            return -1;
        }

        // 3. Notificar a UI que muestre el código
        DeviceCodeInfo info = {0};
        strncpy(info.user_code, g_auth_state.user_code, sizeof(info.user_code) - 1);
        strncpy(info.verification_url, g_auth_state.verification_url,
                sizeof(info.verification_url) - 1);
        bus_post(MSG_AUTH_SHOW_CODE, &info, sizeof(DeviceCodeInfo));

        log_info("[AuthAgent] Waiting for user authorization...");

        // 4. Polling hasta que el usuario autorize
        while (g_agent_running) {
            sceKernelDelayThread(g_auth_state.interval * 1000 * 1000); // microsegundos

            int result = spotify_auth_poll_token(SPOTIFY_CLIENT_ID, &g_auth_state);

            if (result == 0) {
                // Token obtenido
                log_info("[AuthAgent] Authorization successful!");
                auth_save_tokens_to_disk(&g_auth_state);
                g_authenticated = 1;
                bus_post(MSG_AUTH_OK, g_auth_state.access_token,
                         strlen(g_auth_state.access_token) + 1);
                break;
            } else if (result == -2) {
                // Error irrecuperable (device code expiró, etc)
                log_error("[AuthAgent] Device code expired");
                bus_post(MSG_AUTH_FAILED, NULL, 0);
                return -1;
            }
            // result == -1 significa authorization_pending, continuar
        }
    }

    // 5. Loop de mantenimiento: refrescar token antes de que expire
    while (g_agent_running && g_authenticated) {
        sceKernelDelayThread(30 * 1000 * 1000); // Chequear cada 30 segundos

        if (spotify_auth_is_token_expired(&g_auth_state, TOKEN_EXPIRY_BUFFER)) {
            log_info("[AuthAgent] Refreshing token...");

            int new_expires_in;
            if (spotify_auth_refresh_token(SPOTIFY_CLIENT_ID, g_auth_state.refresh_token,
                                           g_auth_state.access_token, &new_expires_in) == 0) {
                g_auth_state.expires_in = new_expires_in;
                g_auth_state.token_acquired_at = time(NULL) * 1000;
                auth_save_tokens_to_disk(&g_auth_state);
                bus_post(MSG_AUTH_TOKEN_REFRESHED, g_auth_state.access_token,
                         strlen(g_auth_state.access_token) + 1);
                log_info("[AuthAgent] Token refreshed");
            } else {
                log_error("[AuthAgent] Failed to refresh token");
                g_authenticated = 0;
            }
        }
    }

    log_info("[AuthAgent] Shutting down");
    return 0;
}

// ============================================================================
// Agent Management
// ============================================================================

SceUID auth_agent_start(void) {
    g_agent_running = 1;

    SceUID thread = sceKernelCreateThread("auth_agent", auth_agent_thread, 0x10000100,
                                           4096, 0, 0, NULL);

    if (thread < 0) {
        log_error("[AuthAgent] Failed to create thread");
        return -1;
    }

    sceKernelStartThread(thread, 0, NULL);
    return thread;
}

int auth_agent_stop(SceUID thread_uid) {
    g_agent_running = 0;
    sceKernelWaitThreadEnd(thread_uid, NULL, NULL);
    sceKernelDeleteThread(thread_uid);
    return 0;
}
