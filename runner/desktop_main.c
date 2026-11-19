#include "host_main.h"
#include "wotg_game.h"

int main(int argc, char **argv) {
    static const SnesDesktopHostGame game = {
        .display_name             = "Marvel Super Heroes: War of the Gems",
        .window_title             = "Marvel Super Heroes: War of the Gems (Recompiled)",
        .region                   = "USA",
        .rom_file                 = "Marvel Super Heroes - War of the Gems (U).smc",
        .expected_sha256_hex      = "49dd77b310b476c875633335243553be59ecfb0bffae62e46f2e53ff05c20fcd",
        .expected_crc32_hex       = "00af56e8",
        .game_id                  = "wotg",
        .build_version            = "dev",
        .env_prefix               = "WOTG",
        .game_info                = &kWotgGameInfo,
        .num_players              = 1,
        .state_menu_hotkeys       = 1,
        .display_aspect_supported = 1,
        .shader_supported         = 0,
    };
    return snesrecomp_desktop_main(&game, argc, argv);
}
