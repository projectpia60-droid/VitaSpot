#pragma once

#include "spotify_models.h"

// ============================================================================
// Spotify API Helpers — Wrappers para Spotify Web API
// ============================================================================

/**
 * Obtiene el perfil del usuario actual
 * @param access_token Token de acceso OAuth
 * @param out_user Buffer para SpotifyUser
 * @return 0 si OK, <0 si error
 */
int spotify_api_get_current_user(const char *access_token, SpotifyUser *out_user);

/**
 * Obtiene playlists del usuario
 * @param access_token Token de acceso OAuth
 * @param limit Número máximo de playlists
 * @param offset Offset para paginación
 * @param out_response Buffer para respuesta
 * @return 0 si OK, <0 si error
 */
int spotify_api_get_playlists(const char *access_token, int limit, int offset,
                               SpotifyPlaylistsResponse *out_response);

/**
 * Obtiene tracks de una playlist
 * @param access_token Token de acceso OAuth
 * @param playlist_id ID de la playlist
 * @param limit Número máximo de tracks
 * @param offset Offset para paginación
 * @param out_response Buffer para respuesta
 * @return 0 si OK, <0 si error
 */
int spotify_api_get_playlist_tracks(const char *access_token, const char *playlist_id,
                                     int limit, int offset,
                                     SpotifyTracksResponse *out_response);

/**
 * Obtiene estado actual de reproducción
 * @param access_token Token de acceso OAuth
 * @param out_state Buffer para SpotifyPlaybackState
 * @return 0 si OK, <0 si error o sin dispositivo activo
 */
int spotify_api_get_playback_state(const char *access_token,
                                    SpotifyPlaybackState *out_state);

/**
 * Obtiene dispositivos disponibles
 * @param access_token Token de acceso OAuth
 * @param out_response Buffer para respuesta
 * @return 0 si OK, <0 si error
 */
int spotify_api_get_devices(const char *access_token,
                             SpotifyDevicesResponse *out_response);

/**
 * Inicia reproducción
 * @param access_token Token de acceso OAuth
 * @param context_uri URI del contexto (playlist, album, artist)
 * @param device_id ID del dispositivo (puede ser NULL para el activo)
 * @return 0 si OK, <0 si error
 */
int spotify_api_play(const char *access_token, const char *context_uri,
                     const char *device_id);

/**
 * Pausa reproducción
 * @param access_token Token de acceso OAuth
 * @param device_id ID del dispositivo (puede ser NULL)
 * @return 0 si OK, <0 si error
 */
int spotify_api_pause(const char *access_token, const char *device_id);

/**
 * Siguiente canción
 * @param access_token Token de acceso OAuth
 * @param device_id ID del dispositivo (puede ser NULL)
 * @return 0 si OK, <0 si error
 */
int spotify_api_next(const char *access_token, const char *device_id);

/**
 * Canción anterior
 * @param access_token Token de acceso OAuth
 * @param device_id ID del dispositivo (puede ser NULL)
 * @return 0 si OK, <0 si error
 */
int spotify_api_previous(const char *access_token, const char *device_id);

/**
 * Cambia estado de shuffle
 * @param access_token Token de acceso OAuth
 * @param state 1 para on, 0 para off
 * @param device_id ID del dispositivo (puede ser NULL)
 * @return 0 si OK, <0 si error
 */
int spotify_api_set_shuffle(const char *access_token, int state,
                            const char *device_id);

/**
 * Cambia modo repeat
 * @param access_token Token de acceso OAuth
 * @param state "off", "track", o "context"
 * @param device_id ID del dispositivo (puede ser NULL)
 * @return 0 si OK, <0 si error
 */
int spotify_api_set_repeat(const char *access_token, const char *state,
                           const char *device_id);

#endif // SPOTIFY_API_H
