#pragma once

// ============================================================================
// Spotify Auth Helpers — OAuth 2.0 Device Authorization Flow
// ============================================================================

/**
 * Estructura para almacenar tokens de sesión
 */
typedef struct {
    char access_token[512];
    char refresh_token[256];
    char device_code[128];
    char user_code[16];
    char verification_url[256];
    int  expires_in;        // En segundos
    int  interval;          // Polling interval en segundos
    unsigned long token_acquired_at;  // timestamp en ms
} SpotifyAuthState;

/**
 * Solicita un device_code a Spotify
 * @param client_id Client ID de la app registrada
 * @param scope OAuth scopes deseados
 * @param out_state Estructura para almacenar respuesta
 * @return 0 si OK, <0 si error
 */
int spotify_auth_request_device_code(const char *client_id, const char *scope,
                                      SpotifyAuthState *out_state);

/**
 * Realiza polling para obtener el token después que el usuario autoriza
 * @param client_id Client ID
 * @param state Estructura con device_code
 * @return 0 si OK (token obtenido), -1 si authorization_pending, -2 si error
 */
int spotify_auth_poll_token(const char *client_id, SpotifyAuthState *state);

/**
 * Refresca un access_token usando el refresh_token
 * @param client_id Client ID
 * @param refresh_token Token de refresco
 * @param out_new_token Buffer para nuevo token (mínimo 512 bytes)
 * @param out_expires_in Puntero para recibir new expires_in
 * @return 0 si OK, <0 si error
 */
int spotify_auth_refresh_token(const char *client_id, const char *refresh_token,
                                char *out_new_token, int *out_expires_in);

/**
 * Verifica si un token está vencido
 * @param state Estado de auth
 * @param buffer_seconds Margen de tiempo antes de considerar "vencido" (ej: 60)
 * @return 1 si está vencido, 0 si válido
 */
int spotify_auth_is_token_expired(const SpotifyAuthState *state, int buffer_seconds);

#endif // SPOTIFY_AUTH_H
