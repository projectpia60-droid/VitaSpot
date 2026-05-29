#pragma once

#include <psp2/kernel/threadmgr.h>

/**
 * Thread principal del Auth Agent
 * Maneja OAuth 2.0 Device Authorization Flow con Spotify
 */
int auth_agent_thread(SceSize args, void *argp);

/**
 * Inicia el Auth Agent en un thread nuevo
 * @return UID del thread, <0 si error
 */
SceUID auth_agent_start(void);

/**
 * Detiene el Auth Agent
 * @param thread_uid UID del thread del agente
 * @return 0 si OK, <0 si error
 */
int auth_agent_stop(SceUID thread_uid);

#endif // AUTH_AGENT_H
