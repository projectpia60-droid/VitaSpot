#include "spotify_auth.h"
#include "../utils/http.h"
#include "../utils/json.h"
#include "../utils/logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// ============================================================================
// Spotify OAuth 2.0 Device Authorization Flow
// ============================================================================

#define SPOTIFY_ACCOUNTS_API "https://accounts.spotify.com/api"

int spotify_auth_request_device_code(const char *client_id, const char *scope,
                                      SpotifyAuthState *out_state) {
    if (!client_id || !scope || !out_state) {
        return -1;
    }

    // Construir body del POST
    char post_body[1024];
    snprintf(post_body, sizeof(post_body),
             "client_id=%s&scope=%s",
             client_id, scope);

    // Realizar POST a /device/code
    char url[256];
    snprintf(url, sizeof(url), "%s/device/code", SPOTIFY_ACCOUNTS_API);

    char *response = http_post(url, post_body,
                               "application/x-www-form-urlencoded", NULL);

    if (!response) {
        log_error("[SpotifyAuth] Failed to request device code");
        return -1;
    }

    // Parsear respuesta JSON
    json_t *root = json_parse(response);
    free(response);

    if (!root) {
        log_error("[SpotifyAuth] Failed to parse device code response");
        return -1;
    }

    // Extraer valores
    const char *device_code = json_get_string(root, "device_code");
    const char *user_code = json_get_string(root, "verification_uri_complete");
    int expires_in = json_get_integer(root, "expires_in");
    int interval = json_get_integer(root, "interval");

    if (!device_code || !user_code) {
        log_error("[SpotifyAuth] Missing fields in device code response");
        json_decref(root);
        return -1;
    }

    // Copiar valores
    strncpy(out_state->device_code, device_code, sizeof(out_state->device_code) - 1);
    strncpy(out_state->verification_url, user_code, sizeof(out_state->verification_url) - 1);
    out_state->expires_in = expires_in;
    out_state->interval = (interval > 0) ? interval : 5; // Default 5s si no especificado

    // Extraer user_code de verification_uri_complete
    // URL format: https://www.spotify.com/pair?uri=spotify:pair:XXXXXXXXXXXXXXXX
    // El user_code es el hash de 8 caracteres
    const char *user_code_str = json_get_string(root, "user_code");
    if (user_code_str) {
        strncpy(out_state->user_code, user_code_str, sizeof(out_state->user_code) - 1);
    }

    out_state->token_acquired_at = time(NULL) * 1000; // convertir a ms

    json_decref(root);

    log_info("[SpotifyAuth] Device code obtained, user code: %s", out_state->user_code);
    return 0;
}

int spotify_auth_poll_token(const char *client_id, SpotifyAuthState *state) {
    if (!client_id || !state) {
        return -1;
    }

    // Construir body del POST
    char post_body[512];
    snprintf(post_body, sizeof(post_body),
             "client_id=%s&device_code=%s&grant_type=urn:ietf:params:oauth:grant-type:device_code",
             client_id, state->device_code);

    // Realizar POST
    char url[256];
    snprintf(url, sizeof(url), "%s/token", SPOTIFY_ACCOUNTS_API);

    char *response = http_post(url, post_body,
                               "application/x-www-form-urlencoded", NULL);

    if (!response) {
        log_error("[SpotifyAuth] Failed to poll for token");
        return -2;
    }

    // Parsear respuesta
    json_t *root = json_parse(response);
    free(response);

    if (!root) {
        log_error("[SpotifyAuth] Failed to parse token response");
        return -2;
    }

    // Chequear si hay error
    const char *error = json_get_string(root, "error");
    if (error) {
        if (strcmp(error, "authorization_pending") == 0) {
            json_decref(root);
            return -1; // Seguir esperando
        } else {
            log_error("[SpotifyAuth] Token request error: %s", error);
            json_decref(root);
            return -2; // Error irrecuperable
        }
    }

    // Extraer tokens
    const char *access_token = json_get_string(root, "access_token");
    const char *refresh_token = json_get_string(root, "refresh_token");
    int expires_in = json_get_integer(root, "expires_in");

    if (!access_token) {
        log_error("[SpotifyAuth] No access_token in response");
        json_decref(root);
        return -2;
    }

    // Guardar tokens
    strncpy(state->access_token, access_token, sizeof(state->access_token) - 1);
    if (refresh_token) {
        strncpy(state->refresh_token, refresh_token, sizeof(state->refresh_token) - 1);
    }
    state->expires_in = expires_in;
    state->token_acquired_at = time(NULL) * 1000;

    json_decref(root);

    log_info("[SpotifyAuth] Access token obtained, expires in %d seconds", expires_in);
    return 0;
}

int spotify_auth_refresh_token(const char *client_id, const char *refresh_token,
                                char *out_new_token, int *out_expires_in) {
    if (!client_id || !refresh_token || !out_new_token) {
        return -1;
    }

    // Construir body
    char post_body[512];
    snprintf(post_body, sizeof(post_body),
             "client_id=%s&grant_type=refresh_token&refresh_token=%s",
             client_id, refresh_token);

    // POST
    char url[256];
    snprintf(url, sizeof(url), "%s/token", SPOTIFY_ACCOUNTS_API);

    char *response = http_post(url, post_body,
                               "application/x-www-form-urlencoded", NULL);

    if (!response) {
        log_error("[SpotifyAuth] Failed to refresh token");
        return -1;
    }

    // Parsear
    json_t *root = json_parse(response);
    free(response);

    if (!root) {
        log_error("[SpotifyAuth] Failed to parse refresh response");
        return -1;
    }

    const char *new_token = json_get_string(root, "access_token");
    int expires_in = json_get_integer(root, "expires_in");

    if (!new_token) {
        log_error("[SpotifyAuth] No access_token in refresh response");
        json_decref(root);
        return -1;
    }

    strncpy(out_new_token, new_token, 511);
    if (out_expires_in) {
        *out_expires_in = expires_in;
    }

    json_decref(root);

    log_info("[SpotifyAuth] Token refreshed");
    return 0;
}

int spotify_auth_is_token_expired(const SpotifyAuthState *state, int buffer_seconds) {
    if (!state || state->expires_in <= 0) {
        return 1; // Considerar expirado si no tenemos info
    }

    unsigned long now = time(NULL) * 1000;
    unsigned long elapsed = now - state->token_acquired_at;
    unsigned long expires_ms = (state->expires_in - buffer_seconds) * 1000;

    return elapsed >= expires_ms ? 1 : 0;
}
