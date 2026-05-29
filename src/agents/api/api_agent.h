#pragma once

#include <psp2/kernel/threadmgr.h>

/**
 * Thread principal del API Agent
 * Ejecuta llamadas a Spotify Web API y polling de estado
 */
int api_agent_thread(SceSize args, void *argp);

/**
 * Inicia el API Agent
 * @return UID del thread, <0 si error
 */
SceUID api_agent_start(void);

/**
 * Detiene el API Agent
 * @param thread_uid UID del thread
 * @return 0 si OK, <0 si error
 */
int api_agent_stop(SceUID thread_uid);

#endif // API_AGENT_H
