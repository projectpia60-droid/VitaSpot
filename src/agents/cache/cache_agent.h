#pragma once

#include <psp2/kernel/threadmgr.h>

/**
 * Thread principal del Cache Agent
 * Gestiona caché de tokens, imágenes y metadatos
 */
int cache_agent_thread(SceSize args, void *argp);

/**
 * Inicia el Cache Agent
 * @return UID del thread, <0 si error
 */
SceUID cache_agent_start(void);

/**
 * Detiene el Cache Agent
 * @param thread_uid UID del thread
 * @return 0 si OK, <0 si error
 */
int cache_agent_stop(SceUID thread_uid);

#endif // CACHE_AGENT_H
