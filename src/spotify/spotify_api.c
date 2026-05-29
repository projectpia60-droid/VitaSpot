#include "spotify_api.h"
#include "../utils/http.h"
#include "../utils/json.h"
#include "../utils/logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ============================================================================
// Spotify Web API Base
// ============================================================================

#define SPOTIFY_API_BASE "https://api.spotify.com/v1"

static void build_auth_header(const char *access_token, char *out_header, size_t len) {
    snprintf(out_header, len, "Authorization: Bearer %s", access_token);
}

static char *api_get(const char *endpoint, const char *access_token) {
    char url[512];
    char auth_header[600];

    snprintf(url, sizeof(url), "%s%s", SPOTIFY_API_BASE, endpoint);
    build_auth_header(access_token, auth_header, sizeof(auth_header));

    return http_get(url, auth_header);
}

static int api_put(const char *endpoint, const char *body, const char *access_token) {
    char url[512];
    char auth_header[600];

    snprintf(url, sizeof(url), "%s%s", SPOTIFY_API_BASE, endpoint);
    build_auth_header(access_token, auth_header, sizeof(auth_header));

    return http_put(url, body, auth_header);
}

// ============================================================================
// API Implementations
// ============================================================================

int spotify_api_get_current_user(const char *access_token, SpotifyUser *out_user) {
    if (!access_token || !out_user) {
        return -1;
    }

    char *response = api_get("/me", access_token);
    if (!response) {
        return -1;
    }

    json_t *root = json_parse(response);
    free(response);

    if (!root) {
        return -1;
    }

    // Extraer campos
    const char *id = json_get_string(root, "id");
    const char *display_name = json_get_string(root, "display_name");
    const char *email = json_get_string(root, "email");

    if (id) strncpy(out_user->id, id, 63);
    if (display_name) strncpy(out_user->display_name, display_name, MAX_NAME_LEN - 1);
    if (email) strncpy(out_user->email, email, 255);

    // Extraer imagen de perfil
    json_t *images = json_get_array(root, "images");
    if (images && json_array_size(images) > 0) {
        json_t *img = json_array_get(images, 0);
        const char *url = json_get_string(img, "url");
        if (url) strncpy(out_user->profile_image_url, url, MAX_URL_LEN - 1);
    }

    // Followers
    json_t *followers_obj = json_get_object(root, "followers");
    if (followers_obj) {
        out_user->followers = json_get_integer(followers_obj, "total");
    }

    json_decref(root);
    return 0;
}

int spotify_api_get_playlists(const char *access_token, int limit, int offset,
                               SpotifyPlaylistsResponse *out_response) {
    if (!access_token || !out_response) {
        return -1;
    }

    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "/me/playlists?limit=%d&offset=%d", limit, offset);

    char *response = api_get(endpoint, access_token);
    if (!response) {
        return -1;
    }

    json_t *root = json_parse(response);
    free(response);

    if (!root) {
        return -1;
    }

    // Extraer metadata
    out_response->total = json_get_integer(root, "total");
    out_response->offset = json_get_integer(root, "offset");
    out_response->limit = json_get_integer(root, "limit");

    // Extraer items (playlists)
    json_t *items = json_get_array(root, "items");
    if (items) {
        size_t num_items = json_array_size(items);
        if (num_items > MAX_PLAYLISTS) num_items = MAX_PLAYLISTS;

        out_response->items = malloc(num_items * sizeof(SpotifyPlaylist));
        out_response->total = num_items;

        for (size_t i = 0; i < num_items; i++) {
            json_t *item = json_array_get(items, i);
            SpotifyPlaylist *pl = &out_response->items[i];

            const char *id = json_get_string(item, "id");
            const char *name = json_get_string(item, "name");
            const char *uri = json_get_string(item, "uri");

            if (id) strncpy(pl->id, id, 63);
            if (name) strncpy(pl->name, name, MAX_NAME_LEN - 1);
            if (uri) strncpy(pl->uri, uri, MAX_URI_LEN - 1);

            // Track count
            pl->track_count = json_get_integer(json_get_object(item, "tracks"), "total");

            // Imagen
            json_t *images = json_get_array(item, "images");
            if (images && json_array_size(images) > 0) {
                const char *img_url = json_get_string(json_array_get(images, 0), "url");
                if (img_url) strncpy(pl->image_url, img_url, MAX_URL_LEN - 1);
            }
        }
    }

    json_decref(root);
    return 0;
}

int spotify_api_get_playlist_tracks(const char *access_token, const char *playlist_id,
                                     int limit, int offset,
                                     SpotifyTracksResponse *out_response) {
    if (!access_token || !playlist_id || !out_response) {
        return -1;
    }

    char endpoint[512];
    snprintf(endpoint, sizeof(endpoint), "/playlists/%s/tracks?limit=%d&offset=%d",
             playlist_id, limit, offset);

    char *response = api_get(endpoint, access_token);
    if (!response) {
        return -1;
    }

    json_t *root = json_parse(response);
    free(response);

    if (!root) {
        return -1;
    }

    // Extraer metadata
    out_response->total = json_get_integer(root, "total");
    out_response->offset = json_get_integer(root, "offset");
    out_response->limit = json_get_integer(root, "limit");

    // Extraer items (tracks)
    json_t *items = json_get_array(root, "items");
    if (items) {
        size_t num_items = json_array_size(items);
        if (num_items > MAX_TRACKS) num_items = MAX_TRACKS;

        out_response->items = malloc(num_items * sizeof(SpotifyTrack));
        out_response->total = num_items;

        for (size_t i = 0; i < num_items; i++) {
            json_t *item = json_array_get(items, i);
            json_t *track = json_get_object(item, "track");
            SpotifyTrack *t = &out_response->items[i];

            if (!track) continue;

            const char *id = json_get_string(track, "id");
            const char *name = json_get_string(track, "name");
            const char *uri = json_get_string(track, "uri");

            if (id) strncpy(t->id, id, 63);
            if (name) strncpy(t->name, name, MAX_NAME_LEN - 1);
            if (uri) strncpy(t->uri, uri, MAX_URI_LEN - 1);

            t->duration_ms = json_get_integer(track, "duration_ms");
            t->track_number = json_get_integer(track, "track_number");
            t->popularity = json_get_integer(track, "popularity");

            // Artista
            json_t *artists = json_get_array(track, "artists");
            if (artists && json_array_size(artists) > 0) {
                const char *artist_name = json_get_string(json_array_get(artists, 0), "name");
                if (artist_name) strncpy(t->artist, artist_name, MAX_NAME_LEN - 1);
            }

            // Álbum
            json_t *album = json_get_object(track, "album");
            if (album) {
                const char *album_name = json_get_string(album, "name");
                if (album_name) strncpy(t->album, album_name, MAX_NAME_LEN - 1);

                json_t *images = json_get_array(album, "images");
                if (images && json_array_size(images) > 0) {
                    const char *img_url = json_get_string(json_array_get(images, 0), "url");
                    if (img_url) strncpy(t->album_image_url, img_url, MAX_URL_LEN - 1);
                }
            }
        }
    }

    json_decref(root);
    return 0;
}

int spotify_api_get_playback_state(const char *access_token,
                                    SpotifyPlaybackState *out_state) {
    if (!access_token || !out_state) {
        return -1;
    }

    char *response = api_get("/me/player", access_token);
    if (!response) {
        return -1;
    }

    json_t *root = json_parse(response);
    free(response);

    if (!root) {
        return -1;
    }

    // Extraer estado básico
    out_state->is_playing = json_get_boolean(root, "is_playing");
    out_state->progress_ms = json_get_integer(root, "progress_ms");
    out_state->shuffle_state = json_get_boolean(root, "shuffle_state");

    const char *repeat_state = json_get_string(root, "repeat_state");
    if (repeat_state) strncpy(out_state->repeat_state, repeat_state, 15);

    // Track actual
    json_t *item = json_get_object(root, "item");
    if (item) {
        const char *id = json_get_string(item, "id");
        const char *name = json_get_string(item, "name");
        const char *uri = json_get_string(item, "uri");

        if (id) strncpy(out_state->current_track.id, id, 63);
        if (name) strncpy(out_state->current_track.name, name, MAX_NAME_LEN - 1);
        if (uri) strncpy(out_state->current_track.uri, uri, MAX_URI_LEN - 1);

        out_state->current_track.duration_ms = json_get_integer(item, "duration_ms");

        // Artista
        json_t *artists = json_get_array(item, "artists");
        if (artists && json_array_size(artists) > 0) {
            const char *artist = json_get_string(json_array_get(artists, 0), "name");
            if (artist) strncpy(out_state->current_track.artist, artist, MAX_NAME_LEN - 1);
        }

        // Álbum
        json_t *album = json_get_object(item, "album");
        if (album) {
            const char *album_name = json_get_string(album, "name");
            if (album_name) strncpy(out_state->current_track.album, album_name, MAX_NAME_LEN - 1);

            json_t *images = json_get_array(album, "images");
            if (images && json_array_size(images) > 0) {
                const char *img_url = json_get_string(json_array_get(images, 0), "url");
                if (img_url) strncpy(out_state->current_track.album_image_url, img_url, MAX_URL_LEN - 1);
            }
        }
    }

    // Dispositivo
    json_t *device = json_get_object(root, "device");
    if (device) {
        const char *device_id = json_get_string(device, "id");
        const char *device_name = json_get_string(device, "name");

        if (device_id) strncpy(out_state->device_id, device_id, 63);
        if (device_name) strncpy(out_state->device_name, device_name, MAX_NAME_LEN - 1);
        out_state->volume_percent = json_get_integer(device, "volume_percent");
    }

    json_decref(root);
    return 0;
}

int spotify_api_get_devices(const char *access_token,
                             SpotifyDevicesResponse *out_response) {
    if (!access_token || !out_response) {
        return -1;
    }

    char *response = api_get("/me/player/devices", access_token);
    if (!response) {
        return -1;
    }

    json_t *root = json_parse(response);
    free(response);

    if (!root) {
        return -1;
    }

    json_t *devices = json_get_array(root, "devices");
    if (devices) {
        size_t num_devices = json_array_size(devices);
        if (num_devices > MAX_DEVICES) num_devices = MAX_DEVICES;

        out_response->items = malloc(num_devices * sizeof(SpotifyDevice));
        out_response->total = num_devices;

        for (size_t i = 0; i < num_devices; i++) {
            json_t *dev = json_array_get(devices, i);
            SpotifyDevice *d = &out_response->items[i];

            const char *id = json_get_string(dev, "id");
            const char *name = json_get_string(dev, "name");
            const char *type = json_get_string(dev, "type");

            if (id) strncpy(d->id, id, 63);
            if (name) strncpy(d->name, name, MAX_NAME_LEN - 1);
            if (type) strncpy(d->type, type, 31);

            d->is_active = json_get_boolean(dev, "is_active");
            d->volume_percent = json_get_integer(dev, "volume_percent");
        }
    }

    json_decref(root);
    return 0;
}

int spotify_api_play(const char *access_token, const char *context_uri,
                     const char *device_id) {
    if (!access_token) {
        return -1;
    }

    char endpoint[256] = "/me/player/play";
    char body[512] = "{}";

    if (context_uri) {
        snprintf(body, sizeof(body), "{\"context_uri\":\"%s\"}", context_uri);
    }

    if (device_id) {
        snprintf(endpoint, sizeof(endpoint), "/me/player/play?device_id=%s", device_id);
    }

    return api_put(endpoint, body, access_token);
}

int spotify_api_pause(const char *access_token, const char *device_id) {
    if (!access_token) {
        return -1;
    }

    char endpoint[256] = "/me/player/pause";

    if (device_id) {
        snprintf(endpoint, sizeof(endpoint), "/me/player/pause?device_id=%s", device_id);
    }

    return api_put(endpoint, "{}", access_token);
}

int spotify_api_next(const char *access_token, const char *device_id) {
    if (!access_token) {
        return -1;
    }

    char endpoint[256] = "/me/player/next";

    if (device_id) {
        snprintf(endpoint, sizeof(endpoint), "/me/player/next?device_id=%s", device_id);
    }

    // POST, no PUT - implementar si es necesario
    return 0; // TODO: Implementar POST
}

int spotify_api_previous(const char *access_token, const char *device_id) {
    if (!access_token) {
        return -1;
    }

    char endpoint[256] = "/me/player/previous";

    if (device_id) {
        snprintf(endpoint, sizeof(endpoint), "/me/player/previous?device_id=%s", device_id);
    }

    // POST
    return 0; // TODO: Implementar POST
}

int spotify_api_set_shuffle(const char *access_token, int state,
                            const char *device_id) {
    if (!access_token) {
        return -1;
    }

    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "/me/player/shuffle?state=%s%s%s",
             state ? "true" : "false",
             device_id ? "&device_id=" : "",
             device_id ? device_id : "");

    return api_put(endpoint, "{}", access_token);
}

int spotify_api_set_repeat(const char *access_token, const char *state,
                           const char *device_id) {
    if (!access_token || !state) {
        return -1;
    }

    char endpoint[256];
    snprintf(endpoint, sizeof(endpoint), "/me/player/repeat?state=%s%s%s",
             state,
             device_id ? "&device_id=" : "",
             device_id ? device_id : "");

    return api_put(endpoint, "{}", access_token);
}
