#include <psp2/apputil.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysclibrary.h>
#include <psp2/ctrl.h>
#include <psp2/power.h>
#include <psp2/sysmodule.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Message Bus
#include "message_bus/bus.h"

// Agents
#include "agents/auth/auth_agent.h"
#include "agents/api/api_agent.h"
#include "agents/playback/playback_agent.h"
#include "agents/ui/ui_agent.h"
#include "agents/cache/cache_agent.h"

// Utils
#include "utils/logger.h"
#include "utils/http.h"

// ============================================================================
// PSVita Entry Point & Definitions
// ============================================================================

PSP2_MODULE_INFO(0, 0, "VitaSpot");

#define VITA_APP_NAME       "VitaSpot"
#define VITA_TITLEID        "VTSP00001"
#define VITA_VERSION_MAJOR  0
#define VITA_VERSION_MINOR  1
#define VITA_VERSION_PATCH  0

// ============================================================================
// Global Agent Thread UIDs
// ============================================================================

static SceUID g_auth_thread = -1;
static SceUID g_api_thread = -1;
static SceUID g_playback_thread = -1;
static SceUID g_ui_thread = -1;
static SceUID g_cache_thread = -1;

// ============================================================================
// Power Management
// ============================================================================

static void init_power_management(void) {
    // Mantener pantalla encendida (prevenir screen timeout)
    scePowerSetIdleTimerDisabled(1);
    
    // Mantener Wi-Fi activo
    // (In PSVita, network activity keeps Wi-Fi alive automatically)
    
    log_info("[Main] Power management initialized");
}

// ============================================================================
// System Initialization
// ============================================================================

static int init_system(void) {
    // Cargar módulos de SceKernel requeridos
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    
    // Inicializar logger
    if (logger_init("ux0:data/vitaspot/vitaspot.log", LOG_INFO) != 0) {
        printf("FATAL: Failed to initialize logger\n");
        return -1;
    }

    log_info("=== VitaSpot v%d.%d.%d Starting ===",
             VITA_VERSION_MAJOR, VITA_VERSION_MINOR, VITA_VERSION_PATCH);

    // Inicializar HTTP (libcurl)
    if (http_init() != 0) {
        log_error("Failed to initialize HTTP");
        return -1;
    }

    // Inicializar Message Bus
    if (bus_init() != 0) {
        log_error("Failed to initialize Message Bus");
        return -1;
    }

    // Inicializar power management
    init_power_management();

    // Inicializar controles
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);

    log_info("System initialization complete");
    return 0;
}

// ============================================================================
// Agent Startup
// ============================================================================

static int start_agents(void) {
    log_info("Starting all agents...");

    // Orden importante: Auth primero, luego los demás
    g_auth_thread = auth_agent_start();
    if (g_auth_thread < 0) {
        log_error("Failed to start Auth Agent");
        return -1;
    }
    log_info("Auth Agent started (thread: 0x%x)", g_auth_thread);

    // Esperar un poco para que Auth Agent se inicialice
    sceKernelDelayThread(500 * 1000);

    g_api_thread = api_agent_start();
    if (g_api_thread < 0) {
        log_error("Failed to start API Agent");
        return -1;
    }
    log_info("API Agent started (thread: 0x%x)", g_api_thread);

    g_playback_thread = playback_agent_start();
    if (g_playback_thread < 0) {
        log_error("Failed to start Playback Agent");
        return -1;
    }
    log_info("Playback Agent started (thread: 0x%x)", g_playback_thread);

    g_cache_thread = cache_agent_start();
    if (g_cache_thread < 0) {
        log_error("Failed to start Cache Agent");
        return -1;
    }
    log_info("Cache Agent started (thread: 0x%x)", g_cache_thread);

    // UI Agent al final (main loop)
    g_ui_thread = ui_agent_start();
    if (g_ui_thread < 0) {
        log_error("Failed to start UI Agent");
        return -1;
    }
    log_info("UI Agent started (thread: 0x%x)", g_ui_thread);

    log_info("All agents started successfully");
    return 0;
}

// ============================================================================
// Agent Shutdown
// ============================================================================

static void stop_agents(void) {
    log_info("Stopping all agents...");

    // Enviar mensaje de shutdown a todos los agentes
    bus_post(MSG_SHUTDOWN, NULL, 0);

    // Esperar a que se detengan
    if (g_ui_thread >= 0) ui_agent_stop(g_ui_thread);
    if (g_playback_thread >= 0) playback_agent_stop(g_playback_thread);
    if (g_api_thread >= 0) api_agent_stop(g_api_thread);
    if (g_cache_thread >= 0) cache_agent_stop(g_cache_thread);
    if (g_auth_thread >= 0) auth_agent_stop(g_auth_thread);

    log_info("All agents stopped");
}

// ============================================================================
// System Cleanup
// ============================================================================

static void cleanup_system(void) {
    log_info("System cleanup...");

    // Destruir bus
    bus_destroy();

    // Limpiar HTTP
    http_cleanup();

    // Permitir screen timeout
    scePowerSetIdleTimerDisabled(0);

    // Cerrar logger
    logger_close();
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(void) {
    // Inicializar sistema
    if (init_system() != 0) {
        printf("FATAL: System initialization failed\n");
        return EXIT_FAILURE;
    }

    // Iniciar agentes
    if (start_agents() != 0) {
        log_error("Agent startup failed");
        cleanup_system();
        return EXIT_FAILURE;
    }

    // Esperar a que el UI Agent termine (loop principal está en UI Agent)
    // El main thread simplemente espera
    log_info("VitaSpot initialized. UI Agent running...");

    // Esperar hasta que se pida shutdown
    sceKernelWaitThreadEnd(g_ui_thread, NULL, NULL);

    // Cleanup
    stop_agents();
    cleanup_system();

    log_info("=== VitaSpot Shutdown Complete ===");
    return EXIT_SUCCESS;
}
