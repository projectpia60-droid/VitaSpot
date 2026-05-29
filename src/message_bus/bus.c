#include "bus.h"
#include <psp2/kernel/sysmem.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// Global State
// ============================================================================

static Message                g_queue[BUS_QUEUE_SIZE];
static int                    g_head = 0;
static int                    g_tail = 0;
static int                    g_count = 0;
static SceMutex               g_mutex = -1;
static SceConditionVariable   g_cond = -1;
static int                    g_bus_initialized = 0;

// ============================================================================
// Bus Implementation
// ============================================================================

int bus_init(void) {
    if (g_bus_initialized) {
        return 0; // Ya inicializado
    }

    // Crear mutex
    g_mutex = sceKernelCreateMutex("bus_mutex", 0, 0, NULL);
    if (g_mutex < 0) {
        return -1;
    }

    // Crear condition variable
    g_cond = sceKernelCreateConditionVariable("bus_cond", 0, NULL);
    if (g_cond < 0) {
        sceKernelDeleteMutex(g_mutex);
        return -1;
    }

    // Inicializar cola
    memset(g_queue, 0, sizeof(g_queue));
    g_head = 0;
    g_tail = 0;
    g_count = 0;

    g_bus_initialized = 1;
    return 0;
}

void bus_destroy(void) {
    if (!g_bus_initialized) {
        return;
    }

    // Liberar payloads pendientes
    for (int i = 0; i < g_count; i++) {
        int idx = (g_head + i) % BUS_QUEUE_SIZE;
        if (g_queue[idx].payload) {
            free(g_queue[idx].payload);
        }
    }

    // Destruir sincronización
    if (g_cond >= 0) {
        sceKernelDeleteConditionVariable(g_cond);
    }
    if (g_mutex >= 0) {
        sceKernelDeleteMutex(g_mutex);
    }

    g_bus_initialized = 0;
}

int bus_post(MessageType type, const void *payload, size_t size) {
    if (!g_bus_initialized) {
        return -1;
    }

    sceKernelLockMutex(g_mutex, 1, NULL);

    // Verificar si hay espacio
    if (g_count >= BUS_QUEUE_SIZE) {
        sceKernelUnlockMutex(g_mutex, 1);
        return -1; // Cola llena
    }

    // Obtener la posición de escritura
    Message *msg = &g_queue[g_tail];

    // Limpiar mensaje anterior
    if (msg->payload) {
        free(msg->payload);
    }

    // Copiar datos
    msg->type = type;
    msg->payload_size = size;
    msg->timestamp = sceKernelGetProcessTimeWide();

    if (payload && size > 0) {
        msg->payload = malloc(size);
        if (msg->payload) {
            memcpy(msg->payload, payload, size);
        } else {
            // Error de memoria, pero continuamos (payload será NULL)
            msg->payload = NULL;
        }
    } else {
        msg->payload = NULL;
    }

    // Avanzar tail
    g_tail = (g_tail + 1) % BUS_QUEUE_SIZE;
    g_count++;

    // Señal a los threads esperando
    sceKernelSignalConditionVariable(g_cond);
    sceKernelUnlockMutex(g_mutex, 1);

    return 0;
}

int bus_receive(Message *out, unsigned int timeout_ms) {
    if (!g_bus_initialized || !out) {
        return -1;
    }

    SceUInt timeout_us = (timeout_ms == 0) ? 0 : timeout_ms * 1000;

    sceKernelLockMutex(g_mutex, 1, NULL);

    // Esperar mientras la cola esté vacía
    while (g_count == 0) {
        int ret;
        if (timeout_ms == 0) {
            // Espera indefinida
            ret = sceKernelWaitConditionVariable(g_cond, g_mutex, NULL, NULL);
        } else {
            // Espera con timeout
            ret = sceKernelWaitConditionVariable(g_cond, g_mutex, &timeout_us, NULL);
        }

        if (ret < 0) {
            sceKernelUnlockMutex(g_mutex, 1);
            return -1; // Timeout o error
        }
    }

    // Obtener mensaje
    *out = g_queue[g_head];
    memset(&g_queue[g_head], 0, sizeof(Message));

    g_head = (g_head + 1) % BUS_QUEUE_SIZE;
    g_count--;

    sceKernelUnlockMutex(g_mutex, 1);
    return 0;
}

int bus_try_receive(Message *out) {
    if (!g_bus_initialized || !out) {
        return -1;
    }

    sceKernelLockMutex(g_mutex, 1, NULL);

    if (g_count == 0) {
        sceKernelUnlockMutex(g_mutex, 1);
        return -1; // Cola vacía
    }

    // Obtener mensaje
    *out = g_queue[g_head];
    memset(&g_queue[g_head], 0, sizeof(Message));

    g_head = (g_head + 1) % BUS_QUEUE_SIZE;
    g_count--;

    sceKernelUnlockMutex(g_mutex, 1);
    return 0;
}

int bus_wait_for(Message *out, MessageType type, unsigned int timeout_ms) {
    if (!g_bus_initialized || !out) {
        return -1;
    }

    SceUInt start_time = sceKernelGetProcessTimeWide();
    SceUInt timeout_us = timeout_ms * 1000;

    while (1) {
        sceKernelLockMutex(g_mutex, 1, NULL);

        // Buscar el mensaje en la cola
        for (int i = 0; i < g_count; i++) {
            int idx = (g_head + i) % BUS_QUEUE_SIZE;
            if (g_queue[idx].type == type) {
                // Encontrado
                *out = g_queue[idx];
                memset(&g_queue[idx], 0, sizeof(Message));

                // Compactar cola si es necesario
                if (idx == g_head) {
                    g_head = (g_head + 1) % BUS_QUEUE_SIZE;
                } else {
                    // Mover elementos hacia adelante
                    for (int j = idx; j != g_tail; j = (j + 1) % BUS_QUEUE_SIZE) {
                        int next_j = (j + 1) % BUS_QUEUE_SIZE;
                        g_queue[j] = g_queue[next_j];
                    }
                    g_tail = (g_tail == 0) ? BUS_QUEUE_SIZE - 1 : g_tail - 1;
                }
                g_count--;

                sceKernelUnlockMutex(g_mutex, 1);
                return 0;
            }
        }

        // Verificar timeout
        SceUInt elapsed = sceKernelGetProcessTimeWide() - start_time;
        if (elapsed >= timeout_us) {
            sceKernelUnlockMutex(g_mutex, 1);
            return -1; // Timeout
        }

        // Esperar condition variable con timeout reducido
        SceUInt remaining_us = timeout_us - elapsed;
        sceKernelWaitConditionVariable(g_cond, g_mutex, &remaining_us, NULL);
        sceKernelUnlockMutex(g_mutex, 1);
    }

    return -1;
}
