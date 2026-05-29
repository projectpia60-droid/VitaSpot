#include "logger.h"
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// ============================================================================
// Global State
// ============================================================================

static SceUID      g_log_fd = -1;
static LogLevel    g_log_level = LOG_INFO;
static const char *g_log_level_names[] = { "DEBUG", "INFO", "WARN", "ERROR" };

// ============================================================================
// Logger Implementation
// ============================================================================

int logger_init(const char *log_file, LogLevel level) {
    if (!log_file) {
        return -1;
    }

    // Crear directorio de data si no existe
    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir("ux0:data/vitaspot", 0777);
    sceIoMkdir("ux0:data/vitaspot/art", 0777);

    // Abrir archivo de log en modo append, crear si no existe
    g_log_fd = sceIoOpen(log_file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0644);
    if (g_log_fd < 0) {
        return -1;
    }

    g_log_level = level;

    // Escribir header inicial
    logger_log(LOG_INFO, "=== VitaSpot Logger Initialized ===");

    return 0;
}

void logger_close(void) {
    if (g_log_fd >= 0) {
        logger_log(LOG_INFO, "=== VitaSpot Logger Closed ===");
        sceIoClose(g_log_fd);
        g_log_fd = -1;
    }
}

void logger_log(LogLevel level, const char *fmt, ...) {
    if (!fmt || g_log_fd < 0 || level < g_log_level) {
        return;
    }

    // Construir el mensaje
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // Obtener timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // Construir línea de log
    char log_line[768];
    snprintf(log_line, sizeof(log_line), "[%s] [%s] %s\n",
             timestamp, g_log_level_names[level], buffer);

    // Escribir a archivo
    sceIoWrite(g_log_fd, log_line, strlen(log_line));
    sceIoSync(g_log_fd, "");

    // También imprimir a stdout para debugging
    printf("%s", log_line);
}
