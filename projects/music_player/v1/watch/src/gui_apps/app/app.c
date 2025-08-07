/*********************
 *      INCLUDES
 *********************/
#include "flashdb/fdb.h"
#include "gui_app_fwk.h"
#include "littlevgl2rtt.h"
#include "lv_ext_resource_manager.h"
#include "lvgl.h"
#include "lvsf.h"
#include "utils/styles/gl_styles.h"
#include "utils/time/time_utils.h"
#include <rtdevice.h>
#include <stdio.h>
#include <time.h>
#include "gui_apps/music_player/music_player.h"
#include <stdio.h>
#include <string.h>

#define APP_ID "Main"

LV_IMG_DECLARE(bg_blue)
LV_IMG_DECLARE(music_bg)
LV_IMG_DECLARE(music_icon)
LV_IMG_DECLARE(pause_icon)
LV_IMG_DECLARE(play_icon)
LV_IMG_DECLARE(next_icon)

lv_obj_t *timeText;
lv_obj_t *dateText;
lv_obj_t *music_title;
lv_obj_t *duration_title;
lv_obj_t *state_icon;
lv_obj_t *music_island_bg;

music_state_t cache_state = MUSIC_STATE_STOPPED;

struct tm tm_now;

static lv_timer_t *data_timer = NULL;

static void music_state_button_cb(lv_event_t *e)
{
    const music_info_t *info = music_player_get_info();

    if(info->state != MUSIC_STATE_STOPPED)
        music_player_toggle();
}

void create_music_island(const music_info_t *info)
{
    lv_obj_t *screen = lv_scr_act();

    music_island_bg = lv_obj_create(screen);
    lv_obj_set_style_bg_color(music_island_bg, lv_color_black(), 0);
    lv_obj_set_style_radius(music_island_bg, 90, 0);
    lv_obj_align(music_island_bg, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_size(music_island_bg, 340, 53);

    lv_obj_t *music_icon_bg = lv_obj_create(music_island_bg);
    lv_obj_set_style_bg_color(music_icon_bg, lv_color_hex(0x424242), 0);
    lv_obj_set_style_radius(music_icon_bg, 90, 0);
    lv_obj_align(music_icon_bg, LV_ALIGN_LEFT_MID, 9, 0);
    lv_obj_set_size(music_icon_bg, 39, 39);

    lv_obj_t *music_icon_left = lv_img_create(music_icon_bg);
    lv_img_set_src(music_icon_left, LV_EXT_IMG_GET(music_icon));
    lv_obj_align(music_icon_left, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(music_icon_left, 30, 30);

    music_title = lv_label_create(music_island_bg);
    lv_obj_set_style_text_color(music_title, lv_color_white(), 0);
    lv_obj_align(music_title, LV_ALIGN_LEFT_MID, 52, -3);
    lv_obj_add_style(music_title, &small_font_style, 0);
    lv_obj_set_style_text_align(music_title, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_size(music_title, 135, 28);

    lv_obj_t *music_state_bg = lv_btn_create(music_island_bg);
    lv_obj_set_style_bg_color(music_state_bg, lv_color_hex(0x424242), 0);
    lv_obj_set_style_radius(music_state_bg, 90, 0);
    lv_obj_align(music_state_bg, LV_ALIGN_RIGHT_MID, -9, 0);
    lv_obj_set_size(music_state_bg, 39, 39);

    lv_obj_add_event_cb(music_state_bg, music_state_button_cb, LV_EVENT_CLICKED, NULL);

    state_icon = lv_img_create(music_state_bg);
    lv_img_set_src(state_icon, info->state == MUSIC_STATE_PLAYING
                                   ? LV_EXT_IMG_GET(play_icon)
                                   : LV_EXT_IMG_GET(pause_icon));
    lv_obj_align(state_icon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(state_icon, 30, 30);
    lv_obj_add_event_cb(state_icon, music_state_button_cb, LV_EVENT_CLICKED,
                        NULL);

    duration_title = lv_label_create(music_island_bg);
    lv_obj_align(duration_title, LV_ALIGN_RIGHT_MID, -55, -5);
    lv_obj_add_style(duration_title, &small_font_style, 0);
    lv_obj_set_style_text_align(duration_title, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_size(duration_title, 92, 22);
    lv_obj_set_style_text_color(duration_title, lv_color_white(), 0);
}

void update_all_texts()
{
    time_utils_get_tm(&tm_now);

    const music_info_t *info = music_player_get_info();

    lv_label_set_text_fmt(timeText, "%02d:%02d", tm_now.tm_hour, tm_now.tm_min);
    lv_label_set_text(dateText, date_utils_get_str());

    if (info->state == MUSIC_STATE_STOPPED)
    {
        if (music_island_bg)
        {
            lv_obj_del(music_island_bg);
            music_island_bg = NULL;
            music_title = NULL;
            duration_title = NULL;
            state_icon = NULL;
        }
        return;
    }

    if (!music_island_bg)
    {
        create_music_island(info);
    }

    // Update music title
    lv_label_set_text(music_title, info->title);
    lv_label_set_text_fmt(duration_title, "%d:%02d/%d:%02d",
                          info->duration_sec / 60, info->duration_sec % 60,
                          info->position_sec / 60, info->position_sec % 60);

    if (strlen(info->title) > 15)
    {
        lv_label_set_long_mode(music_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_add_style(music_title, &moving_text_style, LV_PART_MAIN);
    }
    else
    {
        lv_label_set_long_mode(music_title, LV_LABEL_LONG_CLIP);
        lv_obj_remove_style(music_title, &moving_text_style, LV_PART_MAIN);
    }

    if (cache_state != info->state)
    {
        lv_img_set_src(state_icon, info->state == MUSIC_STATE_PLAYING
                                       ? LV_EXT_IMG_GET(play_icon)
                                       : LV_EXT_IMG_GET(pause_icon));
        cache_state = info->state;
    }
}

static void update_all_texts_timer_cb(lv_timer_t *timer)
{
    update_all_texts();
}

static void on_start(void)
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_clean(screen);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

    lv_obj_t *bg = lv_img_create(screen);
    lv_img_set_src(bg, LV_EXT_IMG_GET(bg_blue));
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(bg, 390, 450);

    timeText = lv_label_create(screen);
    lv_obj_set_style_text_color(timeText, lv_color_hex(0x444444), 0);
    lv_obj_align(timeText, LV_ALIGN_TOP_MID, 0, 85);
    lv_obj_add_style(timeText, &black_font_style, 0);
    lv_obj_set_style_text_align(timeText, LV_TEXT_ALIGN_CENTER, 0);

    dateText = lv_label_create(screen);
    lv_obj_set_style_text_color(dateText, lv_color_hex(0x444444), 0);
    lv_obj_align(dateText, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_add_style(dateText, &medium_font_style, 0);
    lv_obj_set_style_text_align(dateText, LV_TEXT_ALIGN_CENTER, 0);

    music_player_set_track("Default Song T", "Demo Artist", 150);
    music_player_play();

    update_all_texts();

    if (data_timer)
    {
        lv_timer_del(data_timer);
        data_timer = NULL;
    }

    data_timer = lv_timer_create(update_all_texts_timer_cb, 1000, NULL);
}

static void on_resume(void)
{
}

static void on_pause(void)
{
}

static void on_stop(void)
{
}

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

static int app_mainmenu(intent_t i)
{
    gui_app_regist_msg_handler(APP_ID, msg_handler);
    return 0;
}

BUILTIN_APP_EXPORT(LV_EXT_STR_ID(mainmenu), NULL, APP_ID, app_mainmenu);