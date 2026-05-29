#include "api_agent.h"
#include "../../message_bus/bus.h"
#include "../../spotify/spotify_api.h"
#include "../../spotify/spotify_models.h"
#include "../../utils/logger.h"
#include <psp2/kernel/threadmgr.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// Configuration
// ============================================================================

#define POLL_PLAYBACK_INTERVAL_MS 1000  // Polling cada 1 segundo

// ============================================================================
// Global State
// ============================================================================

static char g_access_token[512] = {0};
static int  g_authenticated = 0;
static int  g_agent_running = 1;
static SpotifyPlaybackState g_current_playback = {0};

// ============================================================================
// API Agent Main Thread
// ============================================================================

int api_agent_thread(SceSize args, void *argp) {
    (void)args;
    (void)argp;

    log_info("[APIAgent] Iniciando, esperando autenticación...");

    // 1. Esperar token del Auth Agent via bus
    Message msg;
    while (g_agent_running) {
        if (bus_receive(&msg, 5000) == 0) {
            if (msg.type == MSG_AUTH_OK) {
                strncpy(g_access_token, (char *)msg.payload, sizeof(g_access_token) - 1);
                if (msg.payload) free(msg.payload);
                g_authenticated = 1;
                log_info("[APIAgent] Token recibido, iniciando polling");
                break;
            }
            if (msg.payload) free(msg.payload);
        }
    }

    if (!g_authenticated) {
        log_error("[APIAgent] No authentication, shutting down");
        return -1;
    }

    // 2. Loop de polling del estado de playback
    int poll_counter = 0;

    while (g_agent_running && g_authenticated) {
        // Procesar mensajes del bus (no bloqueante)
        Message incoming;
        while (bus_try_receive(&incoming) == 0) {
            switch (incoming.type) {
                case MSG_AUTH_TOKEN_REFRESHED:
                    strncpy(g_access_token, (char *)incoming.payload,
                            sizeof(g_access_token) - 1);
                    log_info("[APIAgent] Token actualizado");
                    break;

                case MSG_API_FETCH_PLAYLISTS: {
                    log_debug("[APIAgent] Fetching playlists...");
                    SpotifyPlaylistsResponse response = {0};
                    if (spotify_api_get_playlists(g_access_token, 50, 0, &response) == 0) {
                        bus_post(MSG_PLAYLISTS_READY, &response, sizeof(response));
                    } else {
                        bus_post(MSG_API_ERROR, "Failed to fetch playlists", 25);
                    }
                    break;
                }

                case MSG_API_PLAY:
                    log_debug("[APIAgent] Play command");
                    spotify_api_play(g_access_token, NULL, NULL);
                    break;

                case MSG_API_PAUSE:
                    log_debug("[APIAgent] Pause command");
                    spotify_api_pause(g_access_token, NULL);
                    break;

                case MSG_API_SHUFFLE: {
                    int state = (int)(intptr_t)incoming.payload;
                    log_debug("[APIAgent] Shuffle: %d", state);
                    spotify_api_set_shuffle(g_access_token, state, NULL);
                    break;
                }

                default:
                    break;
            }

            if (incoming.payload) free(incoming.payload);
        }

        // Polling cada POLL_PLAYBACK_INTERVAL_MS
        if (poll_counter >= POLL_PLAYBACK_INTERVAL_MS / 100) {
            SpotifyPlaybackState state = {0};
            if (spotify_api_get_playback_state(g_access_token, &state) == 0) {
                // Copiar estado y postear
                memcpy(&g_current_playback, &state, sizeof(SpotifyPlaybackState));
                bus_post(MSG_PLAYBACK_STATE_UPDATED, &state, sizeof(SpotifyPlaybackState));
            }
            poll_counter = 0;
        }

        poll_counter++;
        sceKernelDelayThread(100 * 1000); // 100ms delay
    }

    log_info("[APIAgent] Shutting down");
    return 0;
}

// ============================================================================
// Agent Management
// ============================================================================

SceUID api_agent_start(void) {
    g_agent_running = 1;
    g_authenticated = 0;

    SceUID thread = sceKernelCreateThread("api_agent", api_agent_thread, 0x10000100,
                                           4096, 0, 0, NULL);

    if (thread < 0) {
        log_error("[APIAgent] Failed to create thread");
        return -1;
    }

    sceKernelStartThread(thread, 0, NULL);
    return thread;
}

int api_agent_stop(SceUID thread_uid) {
    g_agent_running = 0;
    sceKernelWaitThreadEnd(thread_uid, NULL, NULL);
    sceKernelDeleteThread(thread_uid);
    return 0;
}
