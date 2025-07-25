#include "quantum.h"
#include "kirby.h"
#include "satisfaction_core.h"

#define min(x, y) (((x) >= (y)) ? (y) : (x))

typedef enum {
    KIRBY_IDLE,
    KIRBY_WALK,
    KIRBY_JUMP,
    KIRBY_INHALE,
    KIRBY_INHALED_IDLE,
    KIRBY_INHALED_WALK,
    KIRBY_EXHALE,
} kirby_state_t;

static kirby_state_t kirby_state       = KIRBY_WALK;
static kirby_state_t prev_kirby_state  = KIRBY_WALK;
static uint8_t       kirby_frame_index = 0;
static uint32_t      kirby_timer       = 0;
static bool          prev_jump_state   = false;
static bool          is_jump           = false;
// スペースキー入力を検出するためのキー入力イベントのフック
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_SPC:
            if (record->event.pressed) {
                if (!prev_jump_state) {
                    is_jump = true;
                } else {
                    is_jump = false;
                }

                prev_jump_state = true;
            } else {
                is_jump         = false;
                prev_jump_state = false;
            }
            break;
        default:
            break;
    }

    return true;
}

// 時計の描画
static void draw_clock(void) {
    oled_set_cursor(13, 0);
    uint8_t  hour   = last_minute / 60;
    uint16_t minute = last_minute % 60;
    bool     is_pm  = (hour / 12) > 0;
    hour            = hour % 12;
    if (hour == 0) {
        hour = 12;
    }

    static char time_str[8] = "";
    sprintf(time_str, "%02d:%02d%s", hour, minute, is_pm ? "pm" : "am");
    oled_write(time_str, false);
}

void draw_kirby(void) {
    uint8_t mod = get_mods();
    bool shift   = mod & MOD_MASK_SHIFT;
    bool ctrl = mod & MOD_MASK_CTRL;

    // 状態遷移
    switch (kirby_state) {
        case KIRBY_WALK:
            if (shift || ctrl) {
                kirby_state = KIRBY_INHALE;
            } else if (is_jump) {
                kirby_state = KIRBY_JUMP;
                is_jump = false;
            }
            break;
        case KIRBY_JUMP:
            if (shift || ctrl) {
                kirby_state = KIRBY_INHALE;
            } else if (kirby_frame_index >= KIRBY_JUMP_FRAMES) {
                kirby_state = KIRBY_WALK;
            }
            break;
        case KIRBY_INHALE:
            if (!shift && !ctrl) {
                kirby_state = KIRBY_EXHALE;
            } else if (kirby_frame_index >= KIRBY_INHALE_FRAMES) {
                kirby_state = KIRBY_INHALED_IDLE;
            }
            break;
        case KIRBY_INHALED_IDLE:
            if (!shift && !ctrl) {
                kirby_state = KIRBY_EXHALE;
            }
            break;
        case KIRBY_EXHALE:
            if (shift || ctrl) {
                kirby_state = KIRBY_INHALE;
            } else if (kirby_frame_index >= KIRBY_EXHALE_FRAMES) {
                kirby_state = KIRBY_WALK;
            }
            break;
    }

    if (prev_kirby_state != kirby_state) {
        kirby_frame_index = 0;
        kirby_timer       = timer_read32();
        prev_kirby_state  = kirby_state;
    }

    uint16_t    duration = 0;
    const void *img      = NULL;
    switch (kirby_state) {
        case KIRBY_WALK:
            duration = KIRBY_WALK_FRAME_DURATION;
            img      = kirby_walk[kirby_frame_index];
            break;
        case KIRBY_JUMP:
            duration = KIRBY_JUMP_FRAME_DURATION;
            img      = kirby_jump[kirby_frame_index];
            break;
        case KIRBY_INHALE:
            duration = KIRBY_INHALE_FRAME_DURATION;
            img      = kirby_inhale[kirby_frame_index];
            break;
        case KIRBY_INHALED_IDLE:
            duration = KIRBY_INHALED_IDLE_FRAME_DURATION;
            img      = kirby_inhaled_idle[kirby_frame_index];
            break;
        case KIRBY_EXHALE:
            duration = KIRBY_EXHALE_FRAME_DURATION;
            img      = kirby_exhale[kirby_frame_index];
            break;
    }

    oled_set_cursor(0, 0);
    oled_write_raw_P(img, KIRBY_DEFAULT_ANIM_SIZE);

    // 時間判定＆描画
    if (timer_elapsed32(kirby_timer) >= duration) {
        kirby_frame_index++;

        switch (kirby_state) {
            case KIRBY_WALK:
                // ループ再生のためインデックスが超えたら1フレーム目にリセットする
                if (kirby_frame_index > (KIRBY_WALK_FRAMES - 1)) {
                    kirby_frame_index = 0;
                }

                break;
            case KIRBY_INHALED_IDLE:
                // ループ再生のためインデックスが超えたら1フレーム目にリセットする
                if (kirby_frame_index > (KIRBY_INHALED_IDLE_FRAMES - 1)) {
                    kirby_frame_index = 0;
                }

                break;
            case KIRBY_INHALE:
            case KIRBY_EXHALE:
            case KIRBY_JUMP:
                break;
        }

        kirby_timer = timer_read32();
    }

    draw_clock();
}
