#ifndef WOTG_MENU_H_GUARD
#define WOTG_MENU_H_GUARD

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WOTG_MENU_W 512
#define WOTG_MENU_H 448

enum {
    WOTG_MENU_ACT_NONE = 0,
    WOTG_MENU_ACT_SAVE = 1,
    WOTG_MENU_ACT_LOAD = 2,
    WOTG_MENU_ACT_STATE_BROWSER = 3,
    WOTG_MENU_ACT_TOGGLE_PAUSE = 4,
    WOTG_MENU_ACT_RESET = 5,
};

void WotgMenu_Open(int is_paused);
void WotgMenu_Close(void);
int  WotgMenu_IsOpen(void);
void WotgMenu_HandleKey(int key, int repeat);
int  WotgMenu_TakeAction(int *out_slot);
int  WotgMenu_OverlayImage(const uint32_t **pixels, int *w, int *h);
void WotgMenu_TickBeforeFrame(void);

#ifdef __cplusplus
}
#endif

#endif
