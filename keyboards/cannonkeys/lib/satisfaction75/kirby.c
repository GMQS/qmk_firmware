#include "quantum.h"
#include "kirby.h"
#include "satisfaction_core.h"

typedef enum {
    KIRBY_WALK,
    KIRBY_JUMP,
    KIRBY_INHALE,
    KIRBY_INHALED_IDLE,
    KIRBY_EXHALE,
} kirby_state_t;

static kirby_state_t kirby_state       = KIRBY_WALK;
static kirby_state_t prev_kirby_state  = KIRBY_WALK;
static uint8_t       kirby_frame_index = 0;
static uint32_t      kirby_timer       = 0;

/**
 * prev_jump_stateとis_jumpはkeymap.c内のprocess_record_user関数によって変更されます。
 */
bool prev_jump_state = false;
bool is_jump         = false;

void draw_kirby(void) {
    bool active_any_modifiers = get_mods() & (MOD_MASK_CTRL | MOD_MASK_SHIFT | MOD_MASK_ALT);

    // 状態遷移
    switch (kirby_state) {
        case KIRBY_WALK:
            if (active_any_modifiers) {
                kirby_state = KIRBY_INHALE;
            } else if (is_jump) {
                kirby_state = KIRBY_JUMP;
                is_jump     = false;
            }
            break;
        case KIRBY_JUMP:
            if (active_any_modifiers) {
                kirby_state = KIRBY_INHALE;
            } else if (kirby_frame_index >= KIRBY_JUMP_FRAMES) {
                kirby_state = KIRBY_WALK;
            }
            break;
        case KIRBY_INHALE:
            if (!active_any_modifiers) {
                kirby_state = KIRBY_EXHALE;
            } else if (kirby_frame_index >= KIRBY_INHALE_FRAMES) {
                kirby_state = KIRBY_INHALED_IDLE;
            }
            break;
        case KIRBY_INHALED_IDLE:
            if (!active_any_modifiers) {
                kirby_state = KIRBY_EXHALE;
            }
            break;
        case KIRBY_EXHALE:
            if (active_any_modifiers) {
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
}
