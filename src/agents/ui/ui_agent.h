#pragma once

#include <psp2/kernel/threadmgr.h>

/**
 * Thread principal del UI Agent
 * Renderiza interfaz gráfica y captura input del usuario
 */
int ui_agent_thread(SceSize args, void *argp);

/**
 * Inicia el UI Agent
 * @return UID del thread, <0 si error
 */
SceUID ui_agent_start(void);

/**
 * Detiene el UI Agent
 * @param thread_uid UID del thread
 * @return 0 si OK, <0 si error
 */
int ui_agent_stop(SceUID thread_uid);

#endif // UI_AGENT_H
