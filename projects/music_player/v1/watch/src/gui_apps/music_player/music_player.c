#include "./music_player.h"
#include "gui_app_fwk.h"
#include <rtdevice.h>
#include <string.h>

#define APP_ID MUSIC_PLAYER_APP_ID

// Static instance of the music state
static music_info_t music_state = {.title = "Unknown Track",
                                   .artist = "Unknown Artist",
                                   .duration_sec = 180,
                                   .position_sec = 0,
                                   .state = MUSIC_STATE_STOPPED};

static lv_timer_t *music_timer = NULL;

void update_song_data()
{
    music_state.position_sec++;

    if (music_state.position_sec > music_state.duration_sec)
    {
        music_player_stop();

        // TODO: next play song
    }
}

static void update_song_data_timer_cb(lv_timer_t *timer)
{
    update_song_data();
}

void set_timer_status(bool enable)
{
    if (music_timer)
    {
        lv_timer_del(music_timer);
        music_timer = NULL;
    }

    if (enable)
        music_timer = lv_timer_create(update_song_data_timer_cb, 1000, NULL);
}

// Public getter
const music_info_t *music_player_get_info(void)
{
    return &music_state;
}

// Play function
void music_player_play(void)
{
    if (music_state.state != MUSIC_STATE_PLAYING)
    {
        music_state.state = MUSIC_STATE_PLAYING;
        set_timer_status(true);
        rt_kprintf("[Music] Playing: %s - %s\n", music_state.title,
                   music_state.artist);
    }
}

// Pause function
void music_player_pause(void)
{
    if (music_state.state == MUSIC_STATE_PLAYING)
    {
        music_state.state = MUSIC_STATE_PAUSED;
        set_timer_status(false);
        rt_kprintf("[Music] Paused\n");
    }
}

// Stop function
void music_player_stop(void)
{
    music_state.state = MUSIC_STATE_STOPPED;
    music_state.position_sec = 0;
    set_timer_status(false);
    rt_kprintf("[Music] Stopped\n");
}

// Toggle function
void music_player_toggle(void)
{
    if (music_state.state == MUSIC_STATE_PLAYING)
    {
        music_player_pause();
    }
    else
    {
        music_player_play();
    }
}

// Set a new track
void music_player_set_track(const char *title, const char *artist,
                            uint32_t duration_sec)
{
    strncpy(music_state.title, title, sizeof(music_state.title) - 1);
    music_state.title[sizeof(music_state.title) - 1] = '\0';

    strncpy(music_state.artist, artist, sizeof(music_state.artist) - 1);
    music_state.artist[sizeof(music_state.artist) - 1] = '\0';

    music_state.duration_sec = duration_sec;
    music_state.position_sec = 0;
    music_state.state = MUSIC_STATE_STOPPED;

    set_timer_status(false);

    rt_kprintf("[Music] Track Set: %s - %s (%d sec)\n", music_state.title,
               music_state.artist, music_state.duration_sec);
}

// App lifecycle hooks
static void on_start(void)
{
    rt_kprintf("[MusicApp] on_start\n");
}

static void on_resume(void)
{
    rt_kprintf("[MusicApp] on_resume\n");
}

static void on_pause(void)
{
    rt_kprintf("[MusicApp] on_pause\n");
}

static void on_stop(void)
{
    rt_kprintf("[MusicApp] on_stop\n");
}

// GUI App FWK message handler
static void msg_handler(gui_app_msg_type_t msg, void *param)
{
    switch (msg)
    {
    case GUI_APP_MSG_ONSTART:
        on_start();
        break;
    case GUI_APP_MSG_ONRESUME:
        on_resume();
        break;
    case GUI_APP_MSG_ONPAUSE:
        on_pause();
        break;
    case GUI_APP_MSG_ONSTOP:
        on_stop();
        break;
    default:
        break;
    }
}

// App entry point
static int app_musicplayer(intent_t i)
{
    gui_app_regist_msg_handler(APP_ID, msg_handler);
    return 0;
}

// Export to GUI framework
BUILTIN_APP_EXPORT(LV_EXT_STR_ID(setting_time), NULL, APP_ID, app_musicplayer);
