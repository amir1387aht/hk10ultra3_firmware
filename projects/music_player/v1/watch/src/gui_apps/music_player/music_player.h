#ifndef MUSIC_PLAYER_APP_H
#define MUSIC_PLAYER_APP_H

#include <stdbool.h>
#include <stdint.h>
#include "lv_ext_resource_manager.h"

// App ID for use with gui_app_fwk
#define MUSIC_PLAYER_APP_ID "MusicPlayer"

// Music states
typedef enum
{
    MUSIC_STATE_STOPPED,
    MUSIC_STATE_PLAYING,
    MUSIC_STATE_PAUSED
} music_state_t;

// Music info struct
typedef struct
{
    char title[64];
    char artist[64];
    uint32_t duration_sec;
    uint32_t position_sec;
    music_state_t state;
} music_info_t;

/**
 * @brief Get current music playback info
 * @return Pointer to a static music_info_t struct
 */
const music_info_t *music_player_get_info(void);

void set_timer_status(bool enable);

/**
 * @brief Start/resume playback
 */
void music_player_play(void);

/**
 * @brief Pause playback
 */
void music_player_pause(void);

/**
 * @brief Stop playback and reset position
 */
void music_player_stop(void);

/**
 * @brief Toggle between play and pause
 */
void music_player_toggle(void);

/**
 * @brief Set a new track with title, artist, and duration
 */
void music_player_set_track(const char *title, const char *artist,
                            uint32_t duration_sec);

#endif // MUSIC_PLAYER_APP_H
