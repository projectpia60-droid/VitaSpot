#pragma once

#include <psp2/kernel/threadmgr.h>

/**
 * Thread principal del Playback Agent
 * Traduce acciones del usuario en comandos a la API
 */
int playback_agent_thread(SceSize args, void *argp);

/**
 * Inicia el Playback Agent
 * @return UID del thread, <0 si error
 */
SceUID playback_agent_start(void);

/**
 * Detiene el Playback Agent
 * @param thread_uid UID del thread
 * @return 0 si OK, <0 si error
 */
int playback_agent_stop(SceUID thread_uid);

#endif // PLAYBACK_AGENT_H
