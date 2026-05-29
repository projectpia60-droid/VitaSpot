#pragma once

#include <psp2/kernel/threadmgr.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// Message Types — Todos los tipos de mensajes del sistema VitaSpot
// ============================================================================

typedef enum {
    // Auth Agent messages
    MSG_AUTH_OK              = 0x0001,
    MSG_AUTH_FAILED          = 0x0002,
    MSG_AUTH_SHOW_CODE       = 0x0003,
    MSG_AUTH_TOKEN_REFRESHED = 0x0004,

    // API Agent commands (UI/Playback → API Agent)
    MSG_API_FETCH_PLAYLISTS  = 0x0101,
    MSG_API_FETCH_TRACKS     = 0x0102,
    MSG_API_FETCH_DEVICES    = 0x0103,
    MSG_API_PLAY             = 0x0104,
    MSG_API_PAUSE            = 0x0105,
    MSG_API_NEXT             = 0x0106,
    MSG_API_PREV             = 0x0107,
    MSG_API_SHUFFLE          = 0x0108,
    MSG_API_REPEAT           = 0x0109,
    MSG_API_ERROR            = 0x01FF,

    // API Agent responses (API Agent → otros)
    MSG_PLAYLISTS_READY      = 0x0201,
    MSG_TRACKS_READY         = 0x0202,
    MSG_DEVICES_READY        = 0x0203,
    MSG_PLAYBACK_STATE_UPDATED = 0x0204,

    // User input (UI Agent → Playback Agent)
    MSG_USER_PRESS_PLAY      = 0x0301,
    MSG_USER_PRESS_NEXT      = 0x0302,
    MSG_USER_PRESS_PREV      = 0x0303,
    MSG_USER_TOGGLE_SHUFFLE  = 0x0304,
    MSG_USER_TOGGLE_REPEAT   = 0x0305,
    MSG_USER_SELECT_PLAYLIST = 0x0306,

    // UI updates (Playback Agent → UI Agent)
    MSG_UI_UPDATE_PLAYBACK   = 0x0401,

    // Cache messages
    MSG_CACHE_HIT            = 0x0501,
    MSG_CACHE_IMAGE_READY    = 0x0502,

    // Control messages
    MSG_SHUTDOWN             = 0xFFFF,

} MessageType;

// ============================================================================
// Message Structure
// ============================================================================

typedef struct {
    MessageType type;
    void       *payload;      // heap-allocated, receptor es responsable de liberar
    size_t      payload_size;
    SceUInt64   timestamp;    // tick timestamp del SceKernel
} Message;

// ============================================================================
// Device Code Info (para MSG_AUTH_SHOW_CODE)
// ============================================================================

typedef struct {
    char user_code[16];
    char verification_url[256];
} DeviceCodeInfo;

// ============================================================================
// Bus API
// ============================================================================

#define BUS_QUEUE_SIZE 64

/**
 * Inicializa el bus de mensajes (mutex + condition variable)
 * Debe llamarse antes que cualquier otra función del bus
 * @return 0 si OK, <0 si error
 */
int bus_init(void);

/**
 * Destruye el bus (mutex y condition variable)
 */
void bus_destroy(void);

/**
 * Postea un mensaje en la cola (no bloqueante)
 * @param type Tipo de mensaje
 * @param payload Puntero a los datos (puede ser NULL)
 * @param size Tamaño en bytes del payload
 * @return 0 si OK, -1 si cola llena
 */
int bus_post(MessageType type, const void *payload, size_t size);

/**
 * Recibe un mensaje bloqueante con timeout
 * @param out Puntero a Message para almacenar resultado
 * @param timeout_ms Timeout en milisegundos (0 = esperar indefinidamente)
 * @return 0 si recibió un mensaje, -1 si timeout
 */
int bus_receive(Message *out, unsigned int timeout_ms);

/**
 * Intenta recibir un mensaje sin bloquear
 * @param out Puntero a Message para almacenar resultado
 * @return 0 si recibió un mensaje, -1 si la cola está vacía
 */
int bus_try_receive(Message *out);

/**
 * Espera hasta recibir un mensaje de un tipo específico (bloqueante con timeout)
 * @param out Puntero a Message para almacenar resultado
 * @param type Tipo de mensaje a esperar
 * @param timeout_ms Timeout en milisegundos
 * @return 0 si recibió el mensaje, -1 si timeout o error
 */
int bus_wait_for(Message *out, MessageType type, unsigned int timeout_ms);

#endif // BUS_H
