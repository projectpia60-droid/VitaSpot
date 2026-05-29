#include "ui_agent.h"
#include "../../message_bus/bus.h"
#include "../../spotify/spotify_models.h"
#include "../../utils/logger.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/ctrl.h>
#include <stdio.h>

// ============================================================================
// Screen Enumeration
// ============================================================================

typedef enum {
    SCREEN_LOGIN,
    SCREEN_MAIN,
    SCREEN_PLAYLISTS,
    SCREEN_TRACKS,
    SCREEN_DEVICES
} AppScreen;

// ============================================================================
// Global State
// ============================================================================

static AppScreen    g_current_screen = SCREEN_LOGIN;
static int          g_agent_running = 1;

// Estado para login
static char g_user_code[16] = {0};
static char g_verification_url[256] = {0};

// Estado para playback
static SpotifyPlaybackState g_playback = {0};

// ============================================================================
// Simple Text Rendering (sin vita2d por ahora, solo terminal output)
// ============================================================================

static void render_login_screen(void) {
    printf("\033[2J\033[H"); // Clear screen y home
    printf("\n");
    printf("  ╔═══════════════════════════════════════╗\n");
    printf("  ║         VitaSpot Login Screen         ║\n");
    printf("  ╚═══════════════════════════════════════╝\n\n");

    printf("Para conectar tu Spotify, en tu celular o PC:\n\n");
    printf("1. Visita esta URL:\n");
    printf("   %s\n\n", g_verification_url);
    printf("2. Ingresa este codigo:\n");
    printf("   %s\n\n", g_user_code);
    printf("Esperando autorización...\n");
}

static void render_now_playing(void) {
    printf("\033[2J\033[H"); // Clear screen
    printf("\n");
    printf("  ╔═══════════════════════════════════════╗\n");
    printf("  ║          Now Playing - VitaSpot       ║\n");
    printf("  ╚═══════════════════════════════════════╝\n\n");

    printf("Track:  %s\n", g_playback.current_track.name);
    printf("Artist: %s\n", g_playback.current_track.artist);
    printf("Album:  %s\n", g_playback.current_track.album);
    printf("\n");

    int progress_s = g_playback.progress_ms / 1000;
    int duration_s = g_playback.current_track.duration_ms / 1000;
    printf("Progress: %d:%02d / %d:%02d\n", progress_s / 60, progress_s % 60,
           duration_s / 60, duration_s % 60);
    printf("Status: %s\n", g_playback.is_playing ? "Playing" : "Paused");
    printf("Device: %s\n", g_playback.device_name);
    printf("\n");

    printf("Controls:\n");
    printf("  [Cross]     Play/Pause\n");
    printf("  [D-Pad </>] Previous/Next\n");
    printf("  [L]         Shuffle: %s\n", g_playback.shuffle_state ? "ON" : "OFF");
    printf("  [R]         Repeat: %s\n", g_playback.repeat_state);
    printf("  [O]         Playlists\n");
    printf("  [Select]    Devices\n");
}

// ============================================================================
// Input Handling
// ============================================================================

static void handle_input(void) {
    SceCtrlData ctrl;
    static unsigned int prev_buttons = 0;

    sceCtrlReadBufferPositive(0, &ctrl, 1);

    unsigned int pressed = ctrl.buttons & ~prev_buttons;
    prev_buttons = ctrl.buttons;

    if (pressed == 0) {
        return; // No new button press
    }

    switch (g_current_screen) {
        case SCREEN_MAIN:
            if (pressed & SCE_CTRL_CROSS) {
                bus_post(MSG_USER_PRESS_PLAY, NULL, 0);
            }
            if (pressed & SCE_CTRL_LEFT) {
                bus_post(MSG_USER_PRESS_PREV, NULL, 0);
            }
            if (pressed & SCE_CTRL_RIGHT) {
                bus_post(MSG_USER_PRESS_NEXT, NULL, 0);
            }
            if (pressed & SCE_CTRL_LTRIGGER) {
                bus_post(MSG_USER_TOGGLE_SHUFFLE, NULL, 0);
            }
            if (pressed & SCE_CTRL_RTRIGGER) {
                bus_post(MSG_USER_TOGGLE_REPEAT, NULL, 0);
            }
            if (pressed & SCE_CTRL_CIRCLE) {
                g_current_screen = SCREEN_PLAYLISTS;
            }
            if (pressed & SCE_CTRL_SELECT) {
                g_current_screen = SCREEN_DEVICES;
            }
            break;

        case SCREEN_PLAYLISTS:
            if (pressed & SCE_CTRL_CIRCLE) {
                g_current_screen = SCREEN_MAIN;
            }
            break;

        default:
            break;
    }
}

// ============================================================================
// UI Agent Main Thread
// ============================================================================

int ui_agent_thread(SceSize args, void *argp) {
    (void)args;
    (void)argp;

    log_info("[UIAgent] Iniciando...");

    // Inicializar entrada de control
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);

    Message msg;

    while (g_agent_running) {
        // Procesar mensajes del bus (no bloqueante)
        while (bus_try_receive(&msg) == 0) {
            switch (msg.type) {
                case MSG_AUTH_SHOW_CODE: {
                    DeviceCodeInfo *info = (DeviceCodeInfo *)msg.payload;
                    strncpy(g_user_code, info->user_code, sizeof(g_user_code) - 1);
                    strncpy(g_verification_url, info->verification_url,
                            sizeof(g_verification_url) - 1);
                    g_current_screen = SCREEN_LOGIN;
                    log_info("[UIAgent] Showing login screen");
                    break;
                }

                case MSG_AUTH_OK:
                    g_current_screen = SCREEN_MAIN;
                    log_info("[UIAgent] Authenticated, showing main screen");
                    // Solicitar lista de playlists
                    bus_post(MSG_API_FETCH_PLAYLISTS, NULL, 0);
                    break;

                case MSG_UI_UPDATE_PLAYBACK: {
                    if (msg.payload) {
                        memcpy(&g_playback, msg.payload, sizeof(SpotifyPlaybackState));
                    }
                    break;
                }

                default:
                    break;
            }

            if (msg.payload) {
                free(msg.payload);
            }
        }

        // Capturar input
        handle_input();

        // Renderizar frame actual
        switch (g_current_screen) {
            case SCREEN_LOGIN:
                render_login_screen();
                break;
            case SCREEN_MAIN:
                render_now_playing();
                break;
            default:
                printf("\033[2J\033[H");
                printf("Screen: %d\n", g_current_screen);
                break;
        }

        // Frame rate: ~60 FPS (16.67ms)
        sceKernelDelayThread(16 * 1000); // 16ms en microsegundos
    }

    log_info("[UIAgent] Shutting down");
    return 0;
}

// ============================================================================
// Agent Management
// ============================================================================

SceUID ui_agent_start(void) {
    g_agent_running = 1;

    SceUID thread = sceKernelCreateThread("ui_agent", ui_agent_thread, 0x10000100,
                                           8192, 0, 0, NULL);

    if (thread < 0) {
        log_error("[UIAgent] Failed to create thread");
        return -1;
    }

    sceKernelStartThread(thread, 0, NULL);
    return thread;
}

int ui_agent_stop(SceUID thread_uid) {
    g_agent_running = 0;
    sceKernelWaitThreadEnd(thread_uid, NULL, NULL);
    sceKernelDeleteThread(thread_uid);
    return 0;
}
