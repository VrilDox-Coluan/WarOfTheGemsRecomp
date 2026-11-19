#include "wotg_menu.h"
#include "beam_frame_driver.h"
#include "cpu_state.h"
#include "snes_overlay_draw.h"
#include "variables.h"

#include <SDL3/SDL_keycode.h>
#include <stdio.h>
#include <string.h>

extern uint8_t g_ram[];
extern CpuState g_cpu;
extern int g_recomp_stack_top;

typedef struct {
    uint8_t id;
    const char *name;
} WotgStageEntry;

static const WotgStageEntry kStages[] = {
    { 0x00, "00 - AVENGERS BASE" },
    { 0x01, "01 - DR. DOOM'S CASTLE" },
    { 0x02, "02 - AMAZON" },
    { 0x03, "03 - ALASKA" },
    { 0x04, "04 - BOSTON AQUARIUM" },
    { 0x06, "06 - SPACESHIP" },
    { 0x08, "08 - EGYPT" },
    { 0x09, "09 - MT. VESUVIUS" },
    { 0x0B, "0B - ARIZONA POWER PLANT" },
    { 0x0C, "0C - ASTEROID BELT" },
    { 0x0D, "0D - NEBULA" },
    { 0x0E, "0E - FORTRESS OF THANOS" },
};
enum { kNumStages = (int)(sizeof(kStages) / sizeof(kStages[0])) };

enum {
    kRowStage = 0,
    kRowSave,
    kRowLoad,
    kRowBrowser,
    kRowPause,
    kRowReset,
    kRowClose,
    kNumRows
};

static uint32_t s_panel[WOTG_MENU_W * WOTG_MENU_H];
static int s_open = 0;
static int s_row = 0;
static int s_stage_idx = 3; /* 03 - Alaska default */
static int s_slot = 0;      /* 0..9 -> Slot 1..10 */
static int s_paused_snap = 0;
static int s_pending_act = WOTG_MENU_ACT_NONE;
static int s_pending_slot = 0;
static int s_stage_lock_frames = 0;
static uint8_t s_stage_lock_val = 0;
static char s_status[64] = "";

static void SyncStageFromRam(void) {
    uint8_t cur = g_ram[0x3F4E];
    for (int i = 0; i < kNumStages; i++) {
        if (kStages[i].id == cur) {
            s_stage_idx = i;
            return;
        }
    }
}

void WotgMenu_Open(int is_paused) {
    s_open = 1;
    s_paused_snap = is_paused;
    s_pending_act = WOTG_MENU_ACT_NONE;
    s_status[0] = '\0';
    SyncStageFromRam();
}

void WotgMenu_Close(void) {
    s_open = 0;
}

int WotgMenu_IsOpen(void) {
    return s_open;
}

int WotgMenu_TakeAction(int *out_slot) {
    int a = s_pending_act;
    if (out_slot) *out_slot = s_pending_slot;
    s_pending_act = WOTG_MENU_ACT_NONE;
    return a;
}

void WotgMenu_TickBeforeFrame(void) {
    if (s_stage_lock_frames > 0) {
        g_ram[0x3F4E] = s_stage_lock_val;
        s_stage_lock_frames--;
    }
}

void WotgMenu_HandleKey(int key, int repeat) {
    if (repeat || !s_open) return;

    switch (key) {
    case SDLK_ESCAPE:
    case SDLK_BACKSPACE:
    case SDLK_M:
        s_open = 0;
        break;
    case SDLK_UP:
    case SDLK_W:
        s_row = (s_row + kNumRows - 1) % kNumRows;
        break;
    case SDLK_DOWN:
    case SDLK_S:
        s_row = (s_row + 1) % kNumRows;
        break;
    case SDLK_LEFT:
    case SDLK_A:
        if (s_row == kRowStage) {
            s_stage_idx = (s_stage_idx + kNumStages - 1) % kNumStages;
        } else if (s_row == kRowSave || s_row == kRowLoad) {
            s_slot = (s_slot + 9) % 10;
        }
        break;
    case SDLK_RIGHT:
    case SDLK_D:
        if (s_row == kRowStage) {
            s_stage_idx = (s_stage_idx + 1) % kNumStages;
        } else if (s_row == kRowSave || s_row == kRowLoad) {
            s_slot = (s_slot + 1) % 10;
        }
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_SPACE:
        switch (s_row) {
        case kRowStage:
            s_stage_lock_val = kStages[s_stage_idx].id;
            if (g_ram[0x3F5E] == 0 && g_ram[0x3F60] == 0) {
                /* Title / boot screen: apply C09483 new-game defaults (HP + continues) */
                g_ram[0x1FF80] = 3; g_ram[0x1FF81] = 0;
                g_ram[0x3F5E] = 0x40; g_ram[0x3F60] = 0x40;
                g_ram[0x3F62] = 0x40; g_ram[0x3F64] = 0x40;
                g_ram[0x3F66] = 0x40;
                g_ram[0x3F50] = 0; g_ram[0x3F52] = 0; g_ram[0x3F54] = 0;
                g_ram[0x3F56] = 0; g_ram[0x3F58] = 0; g_ram[0x3F5A] = 0;
                g_ram[0x3F5C] = 0;
            }
            g_ram[0x3F4E] = s_stage_lock_val;
            g_ram[0x3F4F] = 0;
            if (g_ram[0x00F3] > 4 || g_ram[0x00F4] != 0) {
                g_ram[0x00F3] = 2; /* Captain America default if uninitialized */
                g_ram[0x00F4] = 0;
            }
            g_cpu.S = 0x1FFF;
            g_cpu.D = 0x0000;
            g_cpu.DB = 0x7E;
            g_cpu.P = 0x00;
            g_cpu.m_flag = 0;
            g_cpu.x_flag = 0;
            g_cpu._flag_I = 0;
            g_cpu.emulation = 0;
            g_recomp_stack_top = 0;
            snes_beam_frame_driver_warp_pc(0xC091E6u);
            s_stage_lock_frames = 30;
            snprintf(s_status, sizeof(s_status), "WARP: %s", kStages[s_stage_idx].name);
            s_open = 0;
            break;
        case kRowSave:
            s_pending_act = WOTG_MENU_ACT_SAVE;
            s_pending_slot = s_slot;
            s_open = 0;
            break;
        case kRowLoad:
            s_pending_act = WOTG_MENU_ACT_LOAD;
            s_pending_slot = s_slot;
            s_open = 0;
            break;
        case kRowBrowser:
            s_pending_act = WOTG_MENU_ACT_STATE_BROWSER;
            s_open = 0;
            break;
        case kRowPause:
            s_pending_act = WOTG_MENU_ACT_TOGGLE_PAUSE;
            s_open = 0;
            break;
        case kRowReset:
            s_pending_act = WOTG_MENU_ACT_RESET;
            s_open = 0;
            break;
        case kRowClose:
            s_open = 0;
            break;
        }
        break;
    default:
        break;
    }
}

int WotgMenu_OverlayImage(const uint32_t **pixels, int *w, int *h) {
    if (!s_open) {
        if (pixels) *pixels = NULL;
        if (w) *w = 0;
        if (h) *h = 0;
        return 0;
    }

    memset(s_panel, 0, sizeof(s_panel));

    const int bx = 36, by = 48, bw = 440, bh = 352;
    snes_ovl_fill_rect(s_panel, WOTG_MENU_W, WOTG_MENU_H, bx, by, bw, bh, 0xEA0C0822u);
    snes_ovl_stroke_rect(s_panel, WOTG_MENU_W, WOTG_MENU_H, bx, by, bw, bh, 0xFFFFD24Du);
    snes_ovl_stroke_rect(s_panel, WOTG_MENU_W, WOTG_MENU_H, bx + 2, by + 2, bw - 4, bh - 4, 0xFF7050C0u);

    snes_ovl_draw_text(s_panel, WOTG_MENU_W, WOTG_MENU_H, bx + 20, by + 18,
                       "WAR OF THE GEMS - MENU", 0xFFFFD24Du, 2);
    snes_ovl_fill_rect(s_panel, WOTG_MENU_W, WOTG_MENU_H, bx + 16, by + 42, bw - 32, 2, 0xFF7050C0u);

    char lines[kNumRows][64];
    snprintf(lines[kRowStage],   64, "STAGE: < %s >", kStages[s_stage_idx].name);
    snprintf(lines[kRowSave],    64, "SAVE STATE: < SLOT %d >", s_slot + 1);
    snprintf(lines[kRowLoad],    64, "LOAD STATE: < SLOT %d >", s_slot + 1);
    snprintf(lines[kRowBrowser], 64, "SAVESTATE BROWSER (F7)");
    snprintf(lines[kRowPause],   64, "%s", s_paused_snap ? "RESUME EMULATION" : "PAUSE EMULATION");
    snprintf(lines[kRowReset],   64, "RESET GAME");
    snprintf(lines[kRowClose],   64, "BACK TO GAME");

    for (int i = 0; i < kNumRows; i++) {
        int ry = by + 62 + i * 34;
        if (i == s_row) {
            snes_ovl_fill_rect(s_panel, WOTG_MENU_W, WOTG_MENU_H, bx + 16, ry - 4, bw - 32, 24, 0xFF282058u);
            snes_ovl_stroke_rect(s_panel, WOTG_MENU_W, WOTG_MENU_H, bx + 16, ry - 4, bw - 32, 24, 0xFFFFD24Du);
        }
        uint32_t col = (i == s_row) ? 0xFFFFFFFFu : 0xFFC8C8E0u;
        snes_ovl_draw_text(s_panel, WOTG_MENU_W, WOTG_MENU_H, bx + 26, ry, lines[i], col, 1);
    }

    if (s_status[0]) {
        snes_ovl_draw_text(s_panel, WOTG_MENU_W, WOTG_MENU_H, bx + 20, by + bh - 44, s_status, 0xFF6BE06Bu, 1);
    }
    snes_ovl_draw_text(s_panel, WOTG_MENU_W, WOTG_MENU_H, bx + 20, by + bh - 24,
                       "UP/DN: ROW  LT/RT: CHANGE  ENTER: OK  M/ESC: CLOSE", 0xFFA0A0C8u, 1);

    if (pixels) *pixels = s_panel;
    if (w) *w = WOTG_MENU_W;
    if (h) *h = WOTG_MENU_H;
    return 1;
}
