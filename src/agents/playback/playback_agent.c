#include "playback_agent.h"
#include "../../message_bus/bus.h"
#include "../../spotify/spotify_models.h"
#include "../../utils/logger.h"
#include <psp2/kernel/threadmgr.h>
#include <string.h>
#include <stdint.h>

// ============================================================================
// Global State
// ============================================================================

static SpotifyPlaybackState g_current_state = {0};
static int g_agent_running = 1;

// ============================================================================
// Playback Agent Main Thread
// ============================================================================

int playback_agent_thread(SceSize args, void *argp) {
    (void)args;
    (void)argp;

    log_info("[PlaybackAgent] Iniciando...");

    Message msg;

    while (g_agent_running) {
        // Esperar mensajes con timeout de 100ms
        if (bus_receive(&msg, 100) != 0) {
            // Timeout, continuar loop
            continue;
        }

        switch (msg.type) {
            case MSG_PLAYBACK_STATE_UPDATED: {
                // Actualizar estado local desde API Agent
                if (msg.payload) {
                    memcpy(&g_current_state, msg.payload, sizeof(SpotifyPlaybackState));
                    // Re-postear para que UI Agent se entere también
                    bus_post(MSG_UI_UPDATE_PLAYBACK, &g_current_state,
                             sizeof(g_current_state));
                }
                break;
            }

            case MSG_USER_PRESS_PLAY: {
                log_debug("[PlaybackAgent] Play/Pause pressed");
                if (g_current_state.is_playing) {
                    bus_post(MSG_API_PAUSE, NULL, 0);
                } else {
                    bus_post(MSG_API_PLAY, NULL, 0);
                }
                break;
            }

            case MSG_USER_PRESS_NEXT: {
                log_debug("[PlaybackAgent] Next pressed");
                bus_post(MSG_API_NEXT, NULL, 0);
                break;
            }

            case MSG_USER_PRESS_PREV: {
                log_debug("[PlaybackAgent] Previous pressed");
                bus_post(MSG_API_PREV, NULL, 0);
                break;
            }

            case MSG_USER_TOGGLE_SHUFFLE: {
                log_debug("[PlaybackAgent] Shuffle toggled");
                int new_shuffle = g_current_state.shuffle_state ? 0 : 1;
                bus_post(MSG_API_SHUFFLE, (void *)(intptr_t)new_shuffle, sizeof(int));
                break;
            }

            case MSG_USER_TOGGLE_REPEAT: {
                log_debug("[PlaybackAgent] Repeat toggled");
                const char *new_repeat;
                if (strcmp(g_current_state.repeat_state, "off") == 0) {
                    new_repeat = "context";
                } else if (strcmp(g_current_state.repeat_state, "context") == 0) {
                    new_repeat = "track";
                } else {
                    new_repeat = "off";
                }
                // TODO: Implementar MSG_API_SET_REPEAT
                break;
            }

            case MSG_AUTH_TOKEN_REFRESHED: {
                log_debug("[PlaybackAgent] Token refreshed");
                // No requiere acción, API Agent maneja el token
                break;
            }

            default:
                break;
        }

        if (msg.payload) {
            free(msg.payload);
        }
    }

    log_info("[PlaybackAgent] Shutting down");
    return 0;
}

// ============================================================================
// Agent Management
// ============================================================================

SceUID playback_agent_start(void) {
    g_agent_running = 1;

    SceUID thread = sceKernelCreateThread("playback_agent", playback_agent_thread,
                                           100, 16 * 1024, 0, 0, NULL);

    if (thread < 0) {
        log_error("[PlaybackAgent] Failed to create thread");
        return -1;
    }

    sceKernelStartThread(thread, 0, NULL);
    return thread;
}

int playback_agent_stop(SceUID thread_uid) {
    g_agent_running = 0;
    sceKernelWaitThreadEnd(thread_uid, NULL, NULL);
    sceKernelDeleteThread(thread_uid);
    return 0;
}
