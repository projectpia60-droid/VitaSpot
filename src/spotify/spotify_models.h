#pragma once

#include <stdint.h>

// ============================================================================
// Spotify Models — Data structures para API responses
// ============================================================================

#define MAX_NAME_LEN     256
#define MAX_URI_LEN      128
#define MAX_URL_LEN      512
#define MAX_TRACKS       500
#define MAX_PLAYLISTS    50
#define MAX_DEVICES      10

// ============================================================================
// Spotify Playlist
// ============================================================================

typedef struct {
    char id[64];
    char name[MAX_NAME_LEN];
    char uri[MAX_URI_LEN];
    char image_url[MAX_URL_LEN];
    int  track_count;
    char owner_display_name[128];
    char owner_id[64];
} SpotifyPlaylist;

// ============================================================================
// Spotify Track
// ============================================================================

typedef struct {
    char id[64];
    char name[MAX_NAME_LEN];
    char artist[MAX_NAME_LEN];
    char album[MAX_NAME_LEN];
    char uri[MAX_URI_LEN];
    char album_image_url[MAX_URL_LEN];
    int  duration_ms;
    int  track_number;
    int  popularity;
} SpotifyTrack;

// ============================================================================
// Spotify Playback State
// ============================================================================

typedef struct {
    SpotifyTrack current_track;
    int          is_playing;
    int          progress_ms;
    int          shuffle_state;    // 0 = off, 1 = on
    char         repeat_state[16]; // "off", "track", "context"
    char         device_id[64];
    char         device_name[MAX_NAME_LEN];
    int          volume_percent;
} SpotifyPlaybackState;

// ============================================================================
// Spotify Device
// ============================================================================

typedef struct {
    char id[64];
    char name[MAX_NAME_LEN];
    char type[32];  // "Computer", "Smartphone", "Speaker", "TV"
    int  is_active;
    int  volume_percent;
    char device_href[MAX_URL_LEN];
} SpotifyDevice;

// ============================================================================
// Spotify User Profile
// ============================================================================

typedef struct {
    char id[64];
    char display_name[MAX_NAME_LEN];
    char email[256];
    char profile_image_url[MAX_URL_LEN];
    int  followers;
    char href[MAX_URL_LEN];
} SpotifyUser;

// ============================================================================
// Container for API List Responses
// ============================================================================

typedef struct {
    SpotifyPlaylist *items;
    int total;
    int offset;
    int limit;
} SpotifyPlaylistsResponse;

typedef struct {
    SpotifyTrack *items;
    int total;
    int offset;
    int limit;
} SpotifyTracksResponse;

typedef struct {
    SpotifyDevice *items;
    int total;
} SpotifyDevicesResponse;

#endif // SPOTIFY_MODELS_H
