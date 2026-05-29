#pragma once

#include <stdio.h>
#include <time.h>

// ============================================================================
// Logger — Sistema de logging para VitaSpot
// ============================================================================

/**
 * Niveles de logging
 */
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3,
} LogLevel;

/**
 * Inicializa el logger
 * @param log_file Ruta del archivo de log (ej: "ux0:data/vitaspot/vitaspot.log")
 * @param level Nivel mínimo de logging (LOG_DEBUG, LOG_INFO, etc.)
 * @return 0 si OK, <0 si error
 */
int logger_init(const char *log_file, LogLevel level);

/**
 * Cierra el logger
 */
void logger_close(void);

/**
 * Macros de logging
 */
#define log_debug(fmt, ...) logger_log(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)  logger_log(LOG_INFO, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)  logger_log(LOG_WARN, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) logger_log(LOG_ERROR, fmt, ##__VA_ARGS__)

/**
 * Función base de logging (no usar directamente, usar macros)
 */
void logger_log(LogLevel level, const char *fmt, ...);

#endif // LOGGER_H
