// Catalog of the text the host shows the player (ui_text.h). One array per catalog language, indexed by UiTextId: a new
// string gets an id in ui_text.h and an entry in every language that has one (a missing entry falls back to English).
// The menu has English and the menu languages (menu_lang); the software keyboard (the game's name entry) has a row for
// every game language.
#include <stdatomic.h>
#include <string.h>

#include "cfg.h"
#include "ov.h"
#include "platform/platform.h"
#include "ui_text.h"

typedef struct { const char *lang; const char *const *text; } Row;

// The language names are the same in every row.
#define LANG_NAMES \
    [UI_MENU_LANGNAME_EN] = "English", [UI_MENU_LANGNAME_JA] = "日本語", [UI_MENU_LANGNAME_FR] = "Français", \
    [UI_MENU_LANGNAME_DE] = "Deutsch", [UI_MENU_LANGNAME_IT] = "Italiano", [UI_MENU_LANGNAME_ES] = "Español", \
    [UI_MENU_LANGNAME_NL] = "Nederlands", [UI_MENU_LANGNAME_RU] = "Русский", \
    [UI_MENU_LANGNAME_ZH_HANS] = "简体中文", [UI_MENU_LANGNAME_ZH_HANT] = "繁體中文", [UI_MENU_LANGNAME_KO] = "한국어", \
    [UI_MENU_LANGNAME_PT_BR] = "Portuguese (Brazil)"

// SWKBD: TITLE, PROMPT, DEFAULT_NAME (at most 9 UTF-16 units, no '@' '%' '\'), OK, CANCEL, PAD_HINT.
#define SWKBD(title, prompt, name, ok, cancel, hint) \
    [UI_SWKBD_TITLE] = title, [UI_SWKBD_PROMPT] = prompt, [UI_SWKBD_DEFAULT_NAME] = name, [UI_SWKBD_OK] = ok, \
    [UI_SWKBD_CANCEL] = cancel, [UI_SWKBD_PAD_HINT] = hint, LANG_NAMES

static const char *const text_en[UI_TEXT_COUNT] = {
    [UI_MENU_QUICK_TITLE] = "Age of Calamity Recomp  ·  quick menu",
    [UI_MENU_QUICK_HINT] = "Hold L3+R3 to open or close. B closes.",
    [UI_MENU_QUICK_FPS] = "Frame rate",
    [UI_MENU_QUICK_FPS_DESC] = "Images per second. 30 is the original game, 60 is smoother and uses about twice the "
                               "CPU. 120 and up are experimental and need a fast monitor.",
    [UI_MENU_QUICK_RES] = "Console mode",
    [UI_MENU_QUICK_RES_DESC] = "Makes the game act as if the Switch is on TV or in handheld. Does not change the "
                               "picture size.",
    [UI_MENU_QUICK_RES_HANDHELD] = "Handheld",
    [UI_MENU_QUICK_RES_DOCKED] = "TV (docked)",
    [UI_MENU_QUICK_FULL] = "Full menu",
    [UI_MENU_QUICK_MARK] = "Mark this moment",
    SWKBD("Text entry", "Enter a name", "Horse", "OK", "Cancel", "Controller: A = OK, B = Cancel"),

    [UI_MENU_TITLE] = "Age of Calamity Recomp",
    [UI_MENU_TAB_SCREEN] = "Screen", [UI_MENU_TAB_GAME] = "Game", [UI_MENU_TAB_MODS] = "Mods", [UI_MENU_TAB_SOUND] = "Sound",
    [UI_MENU_TAB_CONTROLS] = "Controls", [UI_MENU_TAB_AMIIBO] = "Amiibo", [UI_MENU_TAB_ADVANCED] = "Advanced",
    [UI_MENU_TAB_LANGUAGE] = "Language",
    [UI_MENU_CONTINUE] = "Continue", [UI_MENU_QUIT] = "Quit game",
    [UI_MENU_ON] = "On", [UI_MENU_OFF] = "Off", [UI_MENU_AUTO] = "Automatic", [UI_MENU_DEFAULT_MARK] = "default",
    [UI_MENU_AFTER_RESTART] = "after restart", [UI_MENU_SET_BY_ENV] = "set by environment variable",
    [UI_MENU_SAVE_FAILED] = "could not save settings",
    [UI_MENU_VERSION] = "Settings  ·  version %s  ·  game %s",

    [UI_MENU_FULLSCREEN] = "Fullscreen", [UI_MENU_FULLSCREEN_DESC] = "Fills the whole screen. F11 also switches it.",
    [UI_MENU_WINDOW_SIZE] = "Window size", [UI_MENU_WINDOW_SIZE_DESC] = "Size of the window when the game is not "
                                                                        "fullscreen.",
    [UI_MENU_PRESENT] = "Presentation", [UI_MENU_PRESENT_DESC] = "How images reach the screen. Mailbox: smooth with low"
                                                                 " delay. VSync: no tearing, a little more delay. "
                                                                 "Immediate: lowest delay, the image can tear.",
    [UI_MENU_PRESENT_MAILBOX] = "Mailbox", [UI_MENU_PRESENT_VSYNC] = "VSync", [UI_MENU_PRESENT_IMMEDIATE] = "Immediate",
    [UI_MENU_PRESENT_ADAPTIVE] = "Adaptive VSync",
    [UI_MENU_PRESENT_IN_USE] = "in use: %s",
    [UI_MENU_GPU] = "Graphics card", [UI_MENU_GPU_DESC] = "Which graphics card runs the game. Useful on a PC with two.",
    [UI_MENU_GPU_AUTO] = "Automatic",
    [UI_MENU_PERF_OVERLAY] = "Performance overlay",
    [UI_MENU_PERF_OVERLAY_DESC] = "Shows the frame rate in the corner. Full adds CPU, GPU, memory and temperatures. F10"
                                  " changes the mode.",
    [UI_MENU_PERF_OFF] = "Off", [UI_MENU_PERF_FPS] = "FPS", [UI_MENU_PERF_FULL] = "Full",
    // printf format of the overlay line in FPS mode: the frame rate
    [UI_PERF_LINE_FPS] = "%.0f fps",
    [UI_MENU_GRAPHICS] = "Picture",
    [UI_MENU_OUTPUT_RES] = "Game resolution",
    [UI_MENU_OUTPUT_RES_DESC] = "Size of the game's picture and interface. 1920 x 1080 is the default. Higher: sharper "
                                "interface, more GPU load. 1280 x 720 suits the Steam Deck.",
    [UI_MENU_RENDER_SCALE] = "3D render scale",
    [UI_MENU_RENDER_SCALE_DESC] = "Sharpness of the 3D world, as a percentage of the game resolution. Higher is sharper"
                                  " but lowers FPS. 100 % is full size, 140 % needs about twice the GPU power, 200 % "
                                  "four times.",
    [UI_MENU_RENDER_SCALE_GAME] = "Game (75 %)",
    [UI_MENU_DYNAMIC_RES] = "Dynamic resolution",
    [UI_MENU_DYNAMIC_RES_DESC] = "Game: the 3D image gets less sharp for a moment in big fights to keep the frame rate."
                                 " Off: always sharp, but heavier on the GPU.",
    [UI_MENU_DYNAMIC_RES_GAME] = "Game", [UI_MENU_DYNAMIC_RES_OFF] = "Off (full size)",
    [UI_MENU_UPSCALER] = "Scaling to the window",
    [UI_MENU_UPSCALER_DESC] = "How the picture is enlarged when the window is bigger than the game resolution. FSR is "
                              "sharper than Bilinear.",
    [UI_MENU_UPSCALER_FSR] = "AMD FSR 1", [UI_MENU_UPSCALER_BILINEAR] = "Bilinear",
    [UI_MENU_ASPECT] = "Picture shape",
    [UI_MENU_ASPECT_DESC] = "For screens that are not 16:9. Black bars keep the right proportions, Stretch fills the "
                            "screen but distorts the picture.",
    [UI_MENU_ASPECT_FIT] = "Black bars", [UI_MENU_ASPECT_STRETCH] = "Stretch",
    [UI_MENU_ANISO] = "Texture filtering",
    [UI_MENU_ANISO_DESC] = "Keeps the ground and walls sharp in the distance. Game: the same level as on Switch.",
    [UI_MENU_ANISO_GAME] = "Game",
    [UI_MENU_SHARPNESS] = "FSR sharpness",
    [UI_MENU_SHARPNESS_DESC] = "Sharpens the picture after FSR scaling. 0 turns it off.",
    [UI_MENU_COLOR] = "Colour",
    [UI_MENU_COLOR_PRESET] = "Colour profile",
    [UI_MENU_COLOR_PRESET_DESC] = "Ready-made colour settings. Vivid: stronger colours and contrast, made for OLED. "
                                  "Clarity: slightly brighter and crisper. Changing a value below switches to Custom.",
    [UI_MENU_COLOR_DEFAULT] = "Game's own", [UI_MENU_COLOR_VIVID] = "Vivid (OLED)", [UI_MENU_COLOR_CLARITY] = "Clarity",
    [UI_MENU_COLOR_CUSTOM] = "Custom",
    [UI_MENU_COLOR_BRIGHTNESS] = "Brightness", [UI_MENU_COLOR_BRIGHTNESS_DESC] = "Makes the whole picture lighter or "
                                                                                 "darker. 0 changes nothing.",
    [UI_MENU_COLOR_CONTRAST] = "Contrast",
    [UI_MENU_COLOR_CONTRAST_DESC] = "Difference between light and dark areas. 100 % changes nothing.",
    [UI_MENU_COLOR_SATURATION] = "Saturation",
    [UI_MENU_COLOR_SATURATION_DESC] = "Strength of the colours. 100 % changes nothing.",
    [UI_MENU_COLOR_GAMMA] = "Gamma",
    [UI_MENU_COLOR_GAMMA_DESC] = "Below 1: brighter shadows and mid-tones. Above 1: darker. 1 changes nothing.",
    [UI_MENU_HDR_GROUP] = "HDR",
    [UI_MENU_HDR] = "HDR output",
    [UI_MENU_HDR_DESC] = "Brighter highlights on an HDR screen. HDR must also be on in Windows or your Wayland "
                         "compositor. Applies after a restart.",
    [UI_MENU_HDR_PEAK] = "Peak brightness",
    [UI_MENU_HDR_PEAK_DESC] = "Set this to your screen's maximum brightness, in nits.",
    [UI_MENU_HDR_GAME] = "Game brightness",
    [UI_MENU_HDR_GAME_DESC] = "Brightness of white in the game, in nits. 203 is the standard. Raise it in a bright "
                              "room.",
    [UI_MENU_HDR_UI] = "HUD and menu brightness",
    [UI_MENU_HDR_UI_DESC] = "Brightness of white in the HUD and this menu, in nits.",
    [UI_MENU_SHADOW_SCALE] = "Shadow resolution",
    [UI_MENU_SHADOW_SCALE_DESC] = "Higher values make shadow edges sharper and use more GPU. Applies after a restart.",
    [UI_MENU_PCSS] = "Soft shadows",
    [UI_MENU_PCSS_DESC] = "Shadows stay sharp near objects and get softer further away. 0 keeps the game's original "
                          "shadows. Applies after a restart.",
    [UI_MENU_DLAA] = "DLSS / FSR 3.1",
    [UI_MENU_DLAA_DESC] = "Smooths edges and reduces shimmer in the 3D image. Uses NVIDIA DLSS on RTX cards, AMD FSR "
                          "3.1 on the others. Applies after a restart.",
    [UI_MENU_DLAA_ENGINE] = "Upscaler",
    [UI_MENU_DLAA_ENGINE_DESC] = "Automatic: DLSS if your card supports it, otherwise FSR 3.1. FSR 3.1 works on every "
                                 "graphics card. Applies after a restart.",
    [UI_MENU_DLAA_ENGINE_AUTO] = "Automatic", [UI_MENU_DLAA_ENGINE_DLSS] = "NVIDIA DLSS",
    [UI_MENU_DLAA_ENGINE_FSR3] = "AMD FSR 3.1",
    [UI_MENU_FSR3_SHARPNESS] = "FSR 3.1 sharpness",
    [UI_MENU_FSR3_SHARPNESS_DESC] = "Sharpens the 3D image, with FSR 3.1 only. 0 turns it off.",
    [UI_MENU_DLSS] = "Mode",
    [UI_MENU_DLSS_DESC] = "Native: the best image. Quality, Balanced and Performance draw at 67, 58 and 50 % and "
                          "enlarge it, for more FPS. Applies after a restart.",
    [UI_MENU_DLSS_DLAA] = "Native (DLAA)", [UI_MENU_DLSS_QUALITY] = "Quality", [UI_MENU_DLSS_BALANCED] = "Balanced",
    [UI_MENU_DLSS_PERFORMANCE] = "Performance",
    [UI_MENU_DLSS_PRESET] = "DLSS model",
    [UI_MENU_DLSS_PRESET_DESC] = "The DLSS version used. Keep Automatic if unsure. K: the sharpest. J: less ghosting "
                                 "behind moving characters, a little more flicker.",
    [UI_MENU_DLSS_PRESET_DEFAULT] = "Automatic", [UI_MENU_DLSS_PRESET_K] = "K (sharpest)",
    [UI_MENU_DLSS_PRESET_J] = "J (less ghosting)",
    [UI_MENU_FXAA] = "Anti-aliasing (FXAA)",
    [UI_MENU_FXAA_DESC] = "Smooths jagged edges and slightly softens detail. Not used while DLSS or FSR 3.1 is on.",

    [UI_MENU_FPS_LIMIT] = "Frame rate",
    [UI_MENU_FPS_LIMIT_DESC] = "Images per second. 30 is the original game, 60 is smoother and uses about twice the "
                               "CPU. 120 and up are experimental and need a fast monitor.",
    [UI_MENU_FPS_30] = "30 FPS", [UI_MENU_FPS_60] = "60 FPS", [UI_MENU_FPS_120] = "120 FPS (experimental)",
    [UI_MENU_FPS_144] = "144 FPS (experimental)", [UI_MENU_FPS_240] = "240 FPS (experimental)",
    [UI_MENU_GAME_LANG] = "Game language",
    [UI_MENU_GAME_LANG_DESC] = "Language of the game's text and voices. Automatic follows your system.",
    [UI_MENU_GAME_LANG_AUTO] = "Automatic",
    [UI_MENU_OPERATION_MODE] = "Console mode",
    [UI_MENU_OPERATION_MODE_DESC] = "Makes the game act as if the Switch is docked or in handheld.",
    [UI_MENU_MODE_DOCKED] = "Docked", [UI_MENU_MODE_HANDHELD] = "Handheld",
    [UI_MENU_FRAME_QUEUE] = "Frame queue",
    [UI_MENU_FRAME_QUEUE_DESC] = "Prepares the next image in advance: fewer stutters, up to one frame more input delay.",
    [UI_MENU_ASSUME_FOCUS] = "Keep running out of focus",
    [UI_MENU_ASSUME_FOCUS_DESC] = "On: the game keeps running when you use another window. Off: it pauses, but a "
                                  "loading screen interrupted this way can get stuck.",

    [UI_MENU_MODS_INSTALLED] = "Installed mods",
    [UI_MENU_MODS_NONE] = "No mods yet. Unpack a mod into the mods folder, one folder per mod with its romfs folder "
                          "inside, and restart the game. TKMM can export its merged mods into a folder there too.",
    [UI_MENU_MODS_OPEN] = "Open mods folder",
    [UI_MENU_MODS_OPEN_DESC] = "Opens the folder where mods go.",
    [UI_MENU_MODS_ROW_DESC] = "Game files it replaces: %u. New files it adds: %u.",
    [UI_MENU_MODS_OVERRIDDEN] = "%u of its files come from %s, which is higher in the list.",
    [UI_MENU_MODS_MOVE_HINT] = "The top face button or Ctrl+Up moves it up, the left face button or Ctrl+Down moves it "
                               "down. When two mods change the same file, the higher one wins. Changes apply after a "
                               "restart.",
    [UI_MENU_MODS_EMPTY] = "This folder has no romfs, exefs or cheats files.",
    [UI_MENU_MODS_SUBSDK] = "This mod brings its own game code (a subsdk plugin or a replaced main). The port cannot "
                            "load code modules, so the whole mod stays off.",
    [UI_MENU_MODS_EXEFS_LATER] = "This mod patches the game code (exefs). The game builds the patches in when it "
                                 "starts next (a rebuild of a few minutes); until then the whole mod stays off.",
    [UI_MENU_MODS_CHEATS] = "Cheats: %d. Switch them on the Cheats tab.",
    [UI_MENU_MODS_CHEATS_FILE] = "Cheats for this game version are read at start from cheats\\%s in this mod.",
    [UI_MENU_MODS_BAD_NAME] = "The folder name contains |, which the mod list uses between names. Rename the folder.",
    [UI_MENU_MODS_OTHER_VERSION] = "%u of its files are made for another game version and are left out. Look for a "
                                   "version of this mod made for your game version.",
    [UI_MENU_MODS_UNUSED] = "not used",

    [UI_MENU_VOLUME] = "Volume", [UI_MENU_VOLUME_DESC] = "Overall volume of the game.",
    [UI_MENU_MUTE] = "Mute", [UI_MENU_MUTE_DESC] = "Silences the game without changing the volume.",
    [UI_MENU_MUTE_BG] = "Mute in the background",
    [UI_MENU_MUTE_BG_DESC] = "Silences the game while you are in another window.",
    [UI_MENU_AUDIO_DEVICE] = "Output device",
    [UI_MENU_AUDIO_DEVICE_DESC] = "Where the sound plays. System default follows Windows.",
    [UI_MENU_AUDIO_DEVICE_DEFAULT] = "System default",
    [UI_MENU_AUDIO_SURROUND] = "5.1 surround",
    [UI_MENU_AUDIO_SURROUND_DESC] = "Sends the game's six audio channels to a 5.1 system. Leave off for stereo speakers or headphones.",
    [UI_MENU_INVERT_LX] = "Invert left stick X", [UI_MENU_INVERT_LY] = "Invert left stick Y",
    [UI_MENU_INVERT_RX] = "Invert right stick X (camera)", [UI_MENU_INVERT_RY] = "Invert right stick Y (camera)",
    [UI_MENU_INVERT_DESC] = "Reverses this stick direction.",

    [UI_MENU_AIM_LEFT_STICK] = "Aim with the left stick",
    [UI_MENU_AIM_LEFT_STICK_DESC] = "On: the left stick also aims the bow, as in the original game. Off: only the right"
                                    " stick aims.",
    [UI_MENU_MOTION] = "Motion controls", [UI_MENU_MOTION_DESC] = "Aim by tilting the controller (gyroscope).",
    [UI_MENU_GYRO_CALIBRATION] = "Gyro calibration",
    [UI_MENU_GYRO_CALIBRATION_DESC] = "Corrects the gyroscope's drift. Off: raw gyroscope.",
    [UI_MENU_CALIBRATE_GYRO] = "Calibrate gyro",
    [UI_MENU_CALIBRATE_GYRO_DESC] = "Put the controller on a flat surface, press, and do not touch it for 2 seconds. "
                                    "Stops the aim drifting sideways.",
    [UI_MENU_CALIB_NONE] = "not calibrated", [UI_MENU_CALIB_COLLECTING] = "keep it still...",
    [UI_MENU_CALIB_DONE] = "calibrated", [UI_MENU_CALIB_FAILED] = "no gyro data",
    [UI_MENU_CALIB_MOVED] = "it moved, try again",
    [UI_MENU_RUMBLE_STRENGTH] = "Vibration strength",
    [UI_MENU_RUMBLE_STRENGTH_DESC] = "How strongly the controller vibrates.",
    [UI_MENU_RUMBLE_CURVE] = "Vibration curve",
    [UI_MENU_RUMBLE_CURVE_DESC] = "Lower values make weak vibrations easier to feel.",
    [UI_MENU_RUMBLE_TRIGGERS] = "Trigger vibration (Xbox)",
    [UI_MENU_RUMBLE_TRIGGERS_DESC] = "Also vibrates the triggers of an Xbox controller.",
    [UI_MENU_PAD_LAYOUT] = "Button layout",
    [UI_MENU_PAD_LAYOUT_DESC] = "Which physical button acts as A. Nintendo: the right one. Xbox and PlayStation: the "
                                "bottom one. Automatic picks from your controller.",
    [UI_MENU_PAD_LAYOUT_AUTO] = "Automatic", [UI_MENU_PAD_LAYOUT_NINTENDO] = "Nintendo (A right)",
    [UI_MENU_PAD_LAYOUT_XBOX] = "Xbox (A bottom)",
    [UI_MENU_PAD_LAYOUT_PLAYSTATION] = "PlayStation (cross A)",
    [UI_MENU_PAD_LAYOUT_IN_USE] = "in use: %s",
    [UI_MENU_PROMPTS] = "Button prompts",
    [UI_MENU_PROMPTS_DESC] = "Button icons shown in the game. A button you remap keeps its original icon.",
    [UI_MENU_PROMPTS_NINTENDO] = "Nintendo", [UI_MENU_PROMPTS_XBOX] = "Xbox",
    [UI_MENU_PROMPTS_PLAYSTATION] = "PlayStation",
    [UI_MENU_PROMPTS_MISSING] = "prompt files missing: run the setup again",
    [UI_MENU_PROMPTS_FAILED] = "prompt files damaged: run the setup again",
    [UI_MENU_PROMPTS_RESTART] = "prompts change after restart",
    [UI_MENU_RESTART] = "Restart now",
    [UI_MENU_RESTART_DESC] = "Restarts the game to apply the new icons. Unsaved progress is lost.",
    [UI_MENU_RESTART_CONFIRM] = "Restart the game now? Progress since your last save is lost.",
    [UI_MENU_RESTART_YES] = "Restart",
    [UI_MENU_RESTART_SAVING] = "The game is saving: Restart is possible in a moment.",
    [UI_MENU_LSTICK_DEADZONE] = "Left stick dead zone", [UI_MENU_RSTICK_DEADZONE] = "Right stick dead zone",
    [UI_MENU_STICK_DEADZONE_DESC] = "How far a stick must move to count. If the character or camera moves alone, set it"
                                    " a little above the \"stick now\" value.",
    [UI_MENU_STICK_DEADZONE_NOW] = "stick now: %.0f %%",
    [UI_MENU_MOUSE_SENS] = "Mouse camera sensitivity",
    [UI_MENU_MOUSE_SENS_DESC] = "Camera speed with the mouse.",
    [UI_MENU_WALK_KEY] = "Walk key",
    [UI_MENU_WALK_KEY_DESC] = "Hold this key to walk instead of run on the keyboard.",
    [UI_MENU_WALK_KEY_TAKEN] = "also %s",
    [UI_MENU_WALK_SCALE] = "Walk scale",
    [UI_MENU_WALK_SCALE_DESC] = "Speed while the walk key is held. Half is a walk, full is a run.",

    [UI_BINDS_HELP] = "Select a cell and press A or Enter, then the key, mouse button or controller button. Delete or "
                      "X clears a cell. A key in use moves here; a controller button swaps with the one it replaces.",
    [UI_BINDS_COL_KEY1] = "Key 1", [UI_BINDS_COL_KEY2] = "Key 2", [UI_BINDS_COL_PAD] = "Controller",
    [UI_BINDS_GROUP_BUTTONS] = "Buttons", [UI_BINDS_GROUP_MOVE] = "Movement", [UI_BINDS_GROUP_CAMERA] = "Camera",
    [UI_BINDS_PRESS_KEY] = "Press a key or mouse button for %s",
    [UI_BINDS_PRESS_PAD] = "Press a controller button for %s",
    [UI_BINDS_CANCEL_HINT] = "Esc cancels, or wait %d s",
    [UI_BINDS_TAKEN] = "%s was taken from %s", [UI_BINDS_SWAPPED] = "%s swapped with %s",
    [UI_BINDS_RESET] = "Reset all buttons",
    [UI_BINDS_RESET_DESC] = "Back to the default keys and controller buttons. The button layout above applies again: remapped controller buttons stay where you put them until this reset.",
    [UI_BIND_A] = "A", [UI_BIND_B] = "B", [UI_BIND_X] = "X", [UI_BIND_Y] = "Y",
    [UI_BIND_LCLICK] = "Left stick click", [UI_BIND_RCLICK] = "Right stick click", [UI_BIND_L] = "L",
    [UI_BIND_R] = "R", [UI_BIND_ZL] = "ZL", [UI_BIND_ZR] = "ZR", [UI_BIND_PLUS] = "Plus", [UI_BIND_MINUS] = "Minus",
    [UI_BIND_LEFT] = "D-pad left", [UI_BIND_UP] = "D-pad up", [UI_BIND_RIGHT] = "D-pad right",
    [UI_BIND_DOWN] = "D-pad down",
    [UI_BIND_LSTICK_LEFT] = "Move left", [UI_BIND_LSTICK_UP] = "Move forward", [UI_BIND_LSTICK_RIGHT] = "Move right",
    [UI_BIND_LSTICK_DOWN] = "Move back",
    [UI_BIND_RSTICK_LEFT] = "Camera left", [UI_BIND_RSTICK_UP] = "Camera up", [UI_BIND_RSTICK_RIGHT] = "Camera right",
    [UI_BIND_RSTICK_DOWN] = "Camera down",
    [UI_BIND_DESC_A] = "Interact, pick up, talk.", [UI_BIND_DESC_B] = "Run, cancel, go back.",
    [UI_BIND_DESC_X] = "Jump.", [UI_BIND_DESC_Y] = "Attack.", [UI_BIND_DESC_LCLICK] = "Crouch.",
    [UI_BIND_DESC_RCLICK] = "First-person view.", [UI_BIND_DESC_L] = "Abilities.",
    [UI_BIND_DESC_R] = "Throw a weapon.", [UI_BIND_DESC_ZL] = "Shield, lock on.", [UI_BIND_DESC_ZR] = "Bow.",
    [UI_BIND_DESC_PLUS] = "Menu, inventory.", [UI_BIND_DESC_MINUS] = "Map.",
    [UI_BIND_DESC_DPAD] = "Quick-select.",
    [UI_BIND_DESC_MOVE] = "Moves Link like the left stick. The controller's left stick always works too.",
    [UI_BIND_DESC_CAMERA] = "Turns the camera like the right stick. The mouse and the controller's right stick always "
                            "work too.",
    [UI_KEY_SPACE] = "Space", [UI_KEY_TAB] = "Tab", [UI_KEY_LSHIFT] = "Left Shift", [UI_KEY_LCTRL] = "Left Ctrl",
    [UI_KEY_UP] = "Up arrow", [UI_KEY_DOWN] = "Down arrow", [UI_KEY_LEFT] = "Left arrow",
    [UI_KEY_RIGHT] = "Right arrow",
    [UI_KEY_MOUSE_L] = "Left mouse", [UI_KEY_MOUSE_R] = "Right mouse", [UI_KEY_MOUSE_M] = "Middle mouse",
    [UI_KEY_MOUSE_X1] = "Mouse 4", [UI_KEY_MOUSE_X2] = "Mouse 5",
    [UI_KEY_ENTER] = "Enter", [UI_KEY_BACKSPACE] = "Backspace", [UI_KEY_RSHIFT] = "Right Shift",
    [UI_KEY_RCTRL] = "Right Ctrl", [UI_KEY_LALT] = "Alt", [UI_KEY_CAPSLOCK] = "Caps Lock", [UI_KEY_INSERT] = "Insert",
    [UI_KEY_DELETE] = "Delete", [UI_KEY_HOME] = "Home", [UI_KEY_END] = "End", [UI_KEY_PAGEUP] = "Page Up",
    [UI_KEY_PAGEDOWN] = "Page Down", [UI_KEY_NUM_FMT] = "Num %s",
    [UI_PAD_SOUTH] = "Bottom button", [UI_PAD_EAST] = "Right button", [UI_PAD_WEST] = "Left button",
    [UI_PAD_NORTH] = "Top button", [UI_PAD_LSTICK] = "Left stick", [UI_PAD_RSTICK] = "Right stick",
    [UI_PAD_L] = "Left bumper", [UI_PAD_R] = "Right bumper", [UI_PAD_START] = "Start", [UI_PAD_BACK] = "Back",
    [UI_PAD_DPAD_LEFT] = "D-pad left", [UI_PAD_DPAD_UP] = "D-pad up", [UI_PAD_DPAD_RIGHT] = "D-pad right",
    [UI_PAD_DPAD_DOWN] = "D-pad down", [UI_PAD_ZL] = "Left trigger", [UI_PAD_ZR] = "Right trigger",
    [UI_PAD_LPADDLE1] = "Left back button", [UI_PAD_RPADDLE1] = "Right back button",
    [UI_PAD_LPADDLE2] = "Left back button 2", [UI_PAD_RPADDLE2] = "Right back button 2",
    [UI_PAD_CROSS] = "Cross", [UI_PAD_CIRCLE] = "Circle", [UI_PAD_SQUARE] = "Square", [UI_PAD_TRIANGLE] = "Triangle",

    [UI_MENU_AMIIBO_DIR] = "Amiibo folder",
    [UI_MENU_AMIIBO_OPEN] = "Open",
    [UI_MENU_AMIIBO_DEFAULT_DIR] = "Default folder",
    [UI_MENU_AMIIBO_REFRESH] = "Refresh",
    [UI_MENU_AMIIBO_PICK] = "Amiibo dumps (540-byte .bin or .nfc files):",
    [UI_MENU_AMIIBO_NO_DIR] = "Cannot open the folder %s",
    [UI_MENU_AMIIBO_READER] = "NFC reader",
    [UI_MENU_AMIIBO_STATE] = "Reader:",
    [UI_MENU_AMIIBO_STATE_IDLE] = "ready",
    [UI_MENU_AMIIBO_STATE_SEARCHING] = "the game is waiting for an amiibo",
    [UI_MENU_AMIIBO_STATE_FOUND] = "amiibo on the reader",
    [UI_MENU_AMIIBO_STATE_MOUNTED] = "the game is reading the amiibo",
    [UI_MENU_AMIIBO_STATE_OFF] = "off",
    [UI_MENU_AMIIBO_FILE] = "Selected file:",
    [UI_MENU_AMIIBO_NO_FILE] = "(no file selected)",
    [UI_MENU_AMIIBO_TAP] = "Tap the amiibo",
    [UI_MENU_AMIIBO_REMOVE] = "Lift the amiibo",
    [UI_MENU_AMIIBO_AUTO_REMOVE] = "Lift the amiibo by itself after a tap",
    [UI_MENU_AMIIBO_HOLD] = "Time on the reader",
    [UI_MENU_AMIIBO_SECONDS] = "%.1f s",
    [UI_MENU_AMIIBO_TAG] = "Amiibo on the reader:",
    [UI_MENU_AMIIBO_NO_TAG] = "No amiibo on the reader.",
    [UI_MENU_AMIIBO_TAG_FILE] = "File: %s",
    [UI_MENU_AMIIBO_TAG_ID] = "Character %s, series %02x, number %04x, type %02x",
    [UI_MENU_AMIIBO_TAG_UID] = "UID: %s",
    [UI_MENU_AMIIBO_READY] = "Ready",
    [UI_MENU_AMIIBO_TAPPED] = "Tapped: %s",
    [UI_MENU_AMIIBO_LOAD_FAILED] = "Not an amiibo dump (540-byte NTAG215): %s",
    [UI_MENU_AMIIBO_REMOVED] = "Amiibo lifted",
    [UI_MENU_AMIIBO_STATUS] = "Status: %s",
    [UI_MENU_MENU_LANG] = "Menu language",
    [UI_MENU_MENU_LANG_DESC] = "Language of this menu. Automatic follows your system if available, otherwise English.",
    [UI_MENU_MENU_LANG_AUTO] = "Automatic (system)",

    [UI_MENU_ADV_WARNING] = "These settings are for testing and troubleshooting. Change them only when you know why.",
    [UI_MENU_GROUP_SCHED] = "Scheduling", [UI_MENU_GROUP_SHADERS] = "Shaders and pipelines",
    [UI_MENU_GROUP_TEXMEM] = "Textures and memory", [UI_MENU_GROUP_FIXES] = "Render fixes",
    [UI_MENU_GROUP_VULKAN] = "Vulkan", [UI_MENU_GROUP_DIAG] = "Diagnostics",
    [UI_MENU_ASYNC_PREEMPT] = "Virtual core preemption",
    [UI_MENU_ASYNC_PREEMPT_DESC] = "Lets one game thread interrupt another. Keep the default unless troubleshooting.",
    [UI_MENU_PREEMPT_OFF] = "Off", [UI_MENU_PREEMPT_ON] = "On", [UI_MENU_PREEMPT_INHERIT] = "Inheritance only",
    [UI_MENU_REPLAY_ASYNC] = "GPU frame prep on its own thread",
    [UI_MENU_REPLAY_ASYNC_DESC] = "Prepares GPU work on a separate thread. Helps hold 60 FPS in big fights, adds up to "
                                  "one frame of delay.",
    [UI_MENU_TIMER_1MS] = "1 ms Windows timer",
    [UI_MENU_TIMER_1MS_DESC] = "Uses a more precise Windows timer for steadier frame pacing.",
    [UI_MENU_PIPE_WAIT] = "Waiting draw handling",
    [UI_MENU_PIPE_WAIT_DESC] = "How draws wait for a pipeline being built. Keep Default. Old is the previous behaviour.",
    [UI_MENU_PIPE_WAIT_DEFAULT] = "Default", [UI_MENU_PIPE_WAIT_DRAW] = "Old (draw)",
    [UI_MENU_PIPE_SKIP] = "Draws skipped while a pipeline builds",
    [UI_MENU_PIPE_SKIP_DESC] = "Which objects are briefly not drawn while their pipeline is being built.",
    [UI_MENU_PIPE_SKIP_STRICT] = "Strict", [UI_MENU_PIPE_SKIP_ANY] = "Any",
    [UI_MENU_PIPE_SKIP_FULLSCREEN] = "Fullscreen passes",
    [UI_MENU_LIB_PREBUILD] = "Stage library prebuild",
    [UI_MENU_LIB_PREBUILD_DESC] = "Prepares shaders in the background to reduce stutter.",
    [UI_MENU_LIB_PREBUILD_THREADS] = "Stage library prebuild threads",
    [UI_MENU_LIB_PREBUILD_THREADS_DESC] = "CPU threads used for that preparation. Automatic adapts to your CPU.",
    [UI_MENU_PIPE_PREBUILD] = "Pipeline prebuild",
    [UI_MENU_PIPE_PREBUILD_DESC] = "Prepares known pipelines in advance to reduce stutter.",
    [UI_MENU_PREWARM_XLATE] = "Translator prewarm",
    [UI_MENU_PREWARM_XLATE_DESC] = "CPU threads that prepare known shaders at startup. Automatic uses half of them, up "
                                   "to 16.",
    [UI_MENU_GPL] = "Graphics pipeline library",
    [UI_MENU_GPL_DESC] = "Builds pipelines faster with a Vulkan feature. Turn it off if your driver has problems with "
                         "it.",
    [UI_MENU_ASTC] = "ASTC texture decoding", [UI_MENU_ASTC_DESC] = "Where the game's compressed textures are decoded. "
                                                                    "Keep Automatic.",
    [UI_MENU_ASTC_AUTO] = "Automatic", [UI_MENU_ASTC_GPU] = "GPU shader", [UI_MENU_ASTC_NATIVE] = "Native",
    [UI_MENU_ASTC_CPU] = "CPU",
    [UI_MENU_MEMORY] = "GPU memory mode", [UI_MENU_MEMORY_DESC] = "How game memory is shared with the GPU. Keep "
                                                                  "Automatic.",
    [UI_MENU_MEMORY_AUTO] = "Automatic", [UI_MENU_MEMORY_IMPORT] = "Import", [UI_MENU_MEMORY_MIRROR] = "Mirror",
    [UI_MENU_MEMORY_VERIFY] = "Verify",
    [UI_MENU_TEX_ALIAS] = "Texture aliasing", [UI_MENU_TEX_ALIAS_DESC] = "A new texture placed over an old one in "
                                                                         "memory starts with its content. For image "
                                                                         "bugs.",
    [UI_MENU_TEX_VRAM_PRESSURE] = "Texture memory limit", [UI_MENU_TEX_VRAM_PRESSURE_DESC] = "Frees unused textures "
                                                                                             "once video memory passes "
                                                                                             "this percentage. 0 never "
                                                                                             "frees them.",
    [UI_MENU_ASTC_BC] = "Compress ASTC to BC3", [UI_MENU_ASTC_BC_DESC] = "Uses four times less video memory, with "
                                                                         "slightly blockier textures. For GPUs with 4 "
                                                                         "GB or less.",
    [UI_MENU_FIX_C0] = "Constant buffer fix",
    [UI_MENU_FIX_C0_DESC] = "Gives shaders data the console's driver normally provides, instead of zeros. For image "
                            "bugs.",
    [UI_MENU_FIX_ZERO] = "Zero new GPU memory",
    [UI_MENU_FIX_ZERO_DESC] = "New GPU memory starts empty, as on the Switch.",
    [UI_MENU_BIND_SKIP] = "Skip repeated binds",
    [UI_MENU_BIND_SKIP_DESC] = "Speed optimisation: skips bindings that did not change.",
    [UI_MENU_SHADER_MEMO] = "Shader memo", [UI_MENU_SHADER_MEMO_DESC] = "Speed optimisation: remembers shader lookups.",
    [UI_MENU_PIPE_MEMO] = "Pipeline memo", [UI_MENU_PIPE_MEMO_DESC] = "Speed optimisation: reuses the previous pipeline"
                                                                      " when nothing changed.",
    [UI_MENU_TEX_MEMO] = "Texture binding memo", [UI_MENU_TEX_MEMO_DESC] = "Speed optimisation: remembers textures "
                                                                           "between draws.",
    [UI_MENU_DESC_TEMPLATE] = "Descriptor templates",
    [UI_MENU_DESC_TEMPLATE_DESC] = "Speed optimisation: updates descriptors in bulk.",
    [UI_MENU_PUSH_SKIP] = "Skip repeated push constants",
    [UI_MENU_PUSH_SKIP_DESC] = "Speed optimisation: skips push constants that did not change.",
    [UI_MENU_SPLIT_DRAWS] = "Early GPU submission",
    [UI_MENU_SPLIT_DRAWS_DESC] = "Sends GPU work in parts so it starts sooner. The value is the draws per part. 0 turns"
                                 " it off.",
    [UI_MENU_VK_VALIDATION] = "Validation layer",
    [UI_MENU_VK_VALIDATION_DESC] = "For debugging only: checks every Vulkan call and slows the game a lot.",
    [UI_MENU_VK_VALIDATION_MISSING] = "The validation layer is not installed.",
    [UI_MENU_DLAA_MISSING] = "DLSS is not available: it needs an NVIDIA RTX card and nvngx_dlss.dll, which the Windows build downloads. Build again, or put the file next to AgeOfCalamityRecomp.exe, and restart. Or set the upscaler to Automatic or AMD FSR 3.1.",
    [UI_MENU_VK_DYN_VERTEX] = "Dynamic vertex state",
    [UI_MENU_VK_DYN_VERTEX_DESC] = "Forces this Vulkan feature on or off. The default depends on your GPU.",
    [UI_MENU_VK_DYN_A2C] = "Dynamic alpha to coverage",
    [UI_MENU_VK_DYN_A2C_DESC] = "Forces this Vulkan feature on or off. Default: off on AMD RDNA 2 and older, where it "
                                "turns characters black.",
    [UI_MENU_VK_SUBGROUP_PIN] = "Pin subgroup size", [UI_MENU_VK_SUBGROUP_PIN_DESC] = "Forces a fixed shader subgroup "
                                                                                      "size. For troubleshooting.",
    [UI_MENU_TRACE] = "Performance log", [UI_MENU_TRACE_DESC] = "Writes frame timings to a file.",
    [UI_MENU_FLIGHT] = "Flight recorder",
    [UI_MENU_FLIGHT_DESC] = "Saves recent activity when a frame freezes, to help report bugs.",
    [UI_MENU_FLIGHT_SPIKE_MS] = "Freeze threshold (ms)",
    [UI_MENU_FLIGHT_SPIKE_MS_DESC] = "Frame time, in ms, that counts as a freeze.",
    [UI_MENU_INPUT_RECORD] = "Input recording",
    [UI_MENU_INPUT_RECORD_DESC] = "Writes your inputs to a file for bug reports.",
    [UI_MENU_OPEN_LOG] = "Open log folder", [UI_MENU_OPEN_LOG_DESC] = "Opens the folder with the log files.",
    [UI_MENU_COPY_INFO] = "Copy system info",
    [UI_MENU_COPY_INFO_DESC] = "Copies your PC details and changed settings, to paste in a bug report.",
    [UI_MENU_COPY_INFO_DONE] = "Copied", [UI_MENU_ACTION_FAILED] = "did not work",
    [UI_MENU_RESET] = "Reset all settings to defaults", [UI_MENU_RESET_DESC] = "Puts every setting in every tab back to"
                                                                               " its default.",
    [UI_MENU_RESET_CONFIRM] = "Reset every setting to its default?", [UI_MENU_RESET_YES] = "Reset",
    [UI_MENU_RESET_NO] = "Cancel",

    [UI_MENU_OV_FPS] = "%.0f FPS", [UI_MENU_OV_FRAME_MS] = "%.1f ms", [UI_MENU_OV_GAME_PCT] = "game %.0f%%",
    [UI_MENU_OV_MAX_MS] = "max %.1f ms",
    [UI_MENU_OV_GPU] = "GPU", [UI_MENU_OV_CPU] = "CPU", [UI_MENU_OV_RAM] = "RAM", [UI_MENU_OV_VRAM] = "VRAM",
    [UI_MENU_OV_NONE] = "-",
    [UI_MENU_OV_REPLAY] = "GPU frame prep", [UI_MENU_OV_GPU_TIME] = "GPU busy per frame",
    [UI_MENU_OV_PIPE_WAIT] = "Pipeline waits",
    [UI_MENU_OV_SHADER_WAIT] = "Shader waits", [UI_MENU_OV_TEX_UPLOADS] = "Texture uploads",
    [UI_MENU_OV_FMT_PCT] = "%.0f%%", [UI_MENU_OV_FMT_TEMP] = "%.0f °C", [UI_MENU_OV_FMT_POWER] = "%.0f W",
    [UI_MENU_OV_FMT_MHZ] = "%.0f MHz", [UI_MENU_OV_FMT_GB] = "%.1f GB", [UI_MENU_OV_FMT_GB_OF] = "/ %.1f GB",
    [UI_MENU_OV_GAME_GB] = "game %.1f GB", [UI_MENU_OV_FMT_MS] = "%.2f ms", [UI_MENU_OV_PER_FRAME] = "%.1f per frame",
    [UI_MENU_OV_WINDOW] = "Window", [UI_MENU_OV_OUTPUT] = "Game image", [UI_MENU_OV_SCENE] = "3D scene",
    [UI_MENU_OV_SCALING] = "Scaling", [UI_MENU_OV_LIMIT] = "Frame limit", [UI_MENU_OV_PRESENT] = "Presentation",
    [UI_MENU_OV_FMT_SIZE] = "%ux%u", [UI_MENU_OV_FMT_SCENE] = "%ux%u, %.0f%%",
    [UI_MENU_OV_FMT_FSR] = "AMD FSR 1, sharpness %d%%", [UI_MENU_OV_SCALE_NONE] = "none (1:1)",
    [UI_MENU_OV_SCALE_BILINEAR] = "bilinear",
    [UI_MENU_OV_SCALE_DOWN] = "bilinear (FSR only enlarges)",
    [UI_MENU_TAB_CHEATS] = "Cheats",
    [UI_MENU_CHEATS_NONE] = "No cheats for this game version. A mod's cheats are an Atmosphere cheat file in its "
                            "cheats folder, named after the game's build: mods\\<mod>\\cheats\\%s. Restart the game to "
                            "read them. For cheats that change the game's code, the game rebuilds itself at start (a "
                            "few minutes).",
    [UI_MENU_CHEATS_MOD] = "%s: %d cheats",
    [UI_MENU_CHEAT_CODE_DESC] = "Changes the game's code. Switches at once.",
    [UI_MENU_CHEAT_KEY_DESC] = "Changes the game's code while you hold %s. Switches at once.",
    [UI_MENU_CHEAT_DATA_DESC] = "Writes the game's memory every frame while it is on.",
    [UI_MENU_CHEAT_MASTER] = "always on",
    [UI_MENU_CHEAT_MASTER_DESC] = "The file's master code: it is always on.",
    [UI_MENU_CHEAT_RESTORE] = "not needed",
    [UI_MENU_CHEAT_RESTORE_DESC] = "It only writes the game's own code back. Switching a cheat off already does that.",
    [UI_MENU_CHEAT_REBUILD] = "after the rebuild",
    [UI_MENU_CHEAT_REBUILD_DESC] = "This cheat changes the game's code and is new or edited since the game was built. "
                                   "The game builds it in when it starts next (a few minutes), or use Rebuild the game "
                                   "now.",
    [UI_MENU_CHEAT_UNSUPPORTED] = "not supported",
    [UI_MENU_CHEAT_UNSUPPORTED_DESC] = "This cheat cannot be used with this version of the game:",
    [UI_MENU_CHEATS_REBUILD] = "Rebuild the game now",
    [UI_MENU_CHEATS_REBUILD_DESC] = "Some cheats change the game's code and are not built into it: they are new or "
                                    "edited, or their last rebuild failed or was cancelled. Closes the game; the setup "
                                    "builds them in (a few minutes) and starts the game again. Progress since your "
                                    "last save is lost.",
    [UI_MENU_MODS_EXEFS_UNSUPPORTED] = "This mod patches the game code (exefs) in a way the port cannot apply, so the "
                                       "whole mod stays off. %s",
    [UI_MENU_MODS_EXEFS_OTHER] = "Its code patches (exefs) are made for another game version.",
    [UI_MENU_MODS_EXEFS_ON] = "Code patches in use: %d files.",
    [UI_MENU_MODS_REPLACES_BUILTIN] = "Code patches in use: %d files. They replace the port's own patch %s, which "
                                      "stays off while this mod is on.",
};

// French: the keyboard, and the whole menu and overlay (menu language "fr").
static const char *const text_fr[UI_TEXT_COUNT] = {
    [UI_MENU_QUICK_TITLE] = "Age of Calamity Recomp  ·  menu rapide",
    [UI_MENU_QUICK_HINT] = "Maintenez L3+R3 pour ouvrir ou fermer. B ferme.",
    [UI_MENU_QUICK_FPS] = "Images par seconde",
    [UI_MENU_QUICK_FPS_DESC] = "Nombre d'images par seconde. 30 est le jeu d'origine, 60 est plus fluide et demande "
                               "environ deux fois plus de processeur. 120 et plus sont expérimentaux et demandent un "
                               "écran rapide.",
    [UI_MENU_QUICK_RES] = "Mode de la console",
    [UI_MENU_QUICK_RES_DESC] = "Le jeu se comporte comme une Switch sur TV ou en portable. Ne change pas la taille de "
                               "l'image.",
    [UI_MENU_QUICK_RES_HANDHELD] = "Portable",
    [UI_MENU_QUICK_RES_DOCKED] = "TV (station)",
    [UI_MENU_QUICK_FULL] = "Menu complet",
    [UI_MENU_QUICK_MARK] = "Marquer ce moment",
    SWKBD("Saisie de texte", "Entrez un nom", "Cheval", "OK", "Annuler", "Manette : A = OK, B = Annuler"),

    [UI_MENU_TITLE] = "Age of Calamity Recomp",
    [UI_MENU_TAB_SCREEN] = "Écran", [UI_MENU_TAB_GAME] = "Jeu", [UI_MENU_TAB_MODS] = "Mods", [UI_MENU_TAB_SOUND] = "Son",
    [UI_MENU_TAB_CONTROLS] = "Commandes", [UI_MENU_TAB_AMIIBO] = "Amiibo", [UI_MENU_TAB_ADVANCED] = "Avancé",
    [UI_MENU_TAB_LANGUAGE] = "Langue",
    [UI_MENU_CONTINUE] = "Continuer", [UI_MENU_QUIT] = "Quitter le jeu",
    [UI_MENU_ON] = "Activé", [UI_MENU_OFF] = "Désactivé", [UI_MENU_AUTO] = "Automatique",
    [UI_MENU_DEFAULT_MARK] = "par défaut",
    [UI_MENU_AFTER_RESTART] = "après redémarrage", [UI_MENU_SET_BY_ENV] = "fixé par une variable d'environnement",
    [UI_MENU_SAVE_FAILED] = "impossible d'enregistrer les réglages",
    [UI_MENU_VERSION] = "Réglages  ·  version %s  ·  jeu %s",

    [UI_MENU_FULLSCREEN] = "Plein écran", [UI_MENU_FULLSCREEN_DESC] = "Le jeu occupe tout l'écran. F11 fait aussi la "
                                                                      "bascule.",
    [UI_MENU_WINDOW_SIZE] = "Taille de la fenêtre",
    [UI_MENU_WINDOW_SIZE_DESC] = "Taille de la fenêtre quand le jeu n'est pas en plein écran.",
    [UI_MENU_PRESENT] = "Présentation", [UI_MENU_PRESENT_DESC] = "Comment les images arrivent à l'écran. Mailbox : "
                                                                 "fluide avec peu de délai. VSync : pas de déchirure, "
                                                                 "un peu plus de délai. Immédiate : délai minimal, "
                                                                 "l'image peut se déchirer.",
    [UI_MENU_PRESENT_MAILBOX] = "Mailbox", [UI_MENU_PRESENT_VSYNC] = "VSync", [UI_MENU_PRESENT_IMMEDIATE] = "Immédiate",
    [UI_MENU_PRESENT_ADAPTIVE] = "VSync adaptative",
    [UI_MENU_PRESENT_IN_USE] = "utilisé : %s",
    [UI_MENU_GPU] = "Carte graphique", [UI_MENU_GPU_DESC] = "La carte graphique qui fait tourner le jeu. Utile sur un "
                                                            "PC qui en a deux.",
    [UI_MENU_GPU_AUTO] = "Automatique",
    [UI_MENU_PERF_OVERLAY] = "Affichage des performances",
    [UI_MENU_PERF_OVERLAY_DESC] = "Affiche les FPS dans le coin. Complet ajoute processeur, carte graphique, mémoire et"
                                  " températures. F10 change de mode.",
    [UI_MENU_PERF_OFF] = "Désactivé", [UI_MENU_PERF_FPS] = "FPS", [UI_MENU_PERF_FULL] = "Complet",
    [UI_PERF_LINE_FPS] = "%.0f fps",
    [UI_MENU_GRAPHICS] = "Image",
    [UI_MENU_OUTPUT_RES] = "Résolution du jeu",
    [UI_MENU_OUTPUT_RES_DESC] = "Taille de l'image et de l'interface du jeu. 1920 x 1080 par défaut. Plus haut : "
                                "interface plus nette, carte graphique plus sollicitée. 1280 x 720 convient au Steam "
                                "Deck.",
    [UI_MENU_RENDER_SCALE] = "Échelle du rendu 3D",
    [UI_MENU_RENDER_SCALE_DESC] = "Netteté du monde en 3D, en pourcentage de la résolution du jeu. Plus haut = plus "
                                  "net, mais moins de FPS. 100 % est la taille normale, 140 % demande environ deux fois"
                                  " plus de carte graphique, 200 % quatre fois plus.",
    [UI_MENU_RENDER_SCALE_GAME] = "Jeu (75 %)",
    [UI_MENU_DYNAMIC_RES] = "Résolution dynamique",
    [UI_MENU_DYNAMIC_RES_DESC] = "Jeu : l'image 3D devient un instant moins nette dans les gros combats pour garder les"
                                 " FPS. Désactivée : toujours net, mais plus lourd pour la carte graphique.",
    [UI_MENU_DYNAMIC_RES_GAME] = "Jeu", [UI_MENU_DYNAMIC_RES_OFF] = "Désactivée (taille réelle)",
    [UI_MENU_UPSCALER] = "Mise à l'échelle de la fenêtre",
    [UI_MENU_UPSCALER_DESC] = "Comment l'image est agrandie quand la fenêtre dépasse la résolution du jeu. FSR est plus"
                              " net que Bilinéaire.",
    [UI_MENU_UPSCALER_FSR] = "AMD FSR 1", [UI_MENU_UPSCALER_BILINEAR] = "Bilinéaire",
    [UI_MENU_ASPECT] = "Forme de l'image",
    [UI_MENU_ASPECT_DESC] = "Pour les écrans qui ne sont pas en 16:9. Les bandes noires gardent les bonnes proportions,"
                            " Étirer remplit l'écran mais déforme l'image.",
    [UI_MENU_ASPECT_FIT] = "Bandes noires", [UI_MENU_ASPECT_STRETCH] = "Étirer",
    [UI_MENU_ANISO] = "Filtrage des textures",
    [UI_MENU_ANISO_DESC] = "Garde le sol et les murs nets au loin. Jeu : le même niveau que sur Switch.",
    [UI_MENU_ANISO_GAME] = "Jeu",
    [UI_MENU_SHARPNESS] = "Netteté FSR",
    [UI_MENU_SHARPNESS_DESC] = "Accentue la netteté après l'agrandissement FSR. 0 désactive.",
    [UI_MENU_COLOR] = "Couleur",
    [UI_MENU_COLOR_PRESET] = "Profil de couleur",
    [UI_MENU_COLOR_PRESET_DESC] = "Réglages de couleur tout prêts. Vif : couleurs et contraste renforcés, pensé pour "
                                  "l'OLED. Clarté : un peu plus lumineux et net. Modifier une valeur ci-dessous passe "
                                  "en Personnalisé.",
    [UI_MENU_COLOR_DEFAULT] = "D'origine", [UI_MENU_COLOR_VIVID] = "Vif (OLED)", [UI_MENU_COLOR_CLARITY] = "Clarté",
    [UI_MENU_COLOR_CUSTOM] = "Personnalisé",
    [UI_MENU_COLOR_BRIGHTNESS] = "Luminosité",
    [UI_MENU_COLOR_BRIGHTNESS_DESC] = "Éclaircit ou assombrit toute l'image. 0 ne change rien.",
    [UI_MENU_COLOR_CONTRAST] = "Contraste",
    [UI_MENU_COLOR_CONTRAST_DESC] = "Écart entre les zones claires et sombres. 100 % ne change rien.",
    [UI_MENU_COLOR_SATURATION] = "Saturation",
    [UI_MENU_COLOR_SATURATION_DESC] = "Intensité des couleurs. 100 % ne change rien.",
    [UI_MENU_COLOR_GAMMA] = "Gamma",
    [UI_MENU_COLOR_GAMMA_DESC] = "Sous 1 : ombres et tons moyens plus clairs. Au-dessus de 1 : plus sombres. 1 ne "
                                 "change rien.",
    [UI_MENU_HDR_GROUP] = "HDR",
    [UI_MENU_HDR] = "Sortie HDR",
    [UI_MENU_HDR_DESC] = "Hautes lumières plus éclatantes sur un écran HDR. Le HDR doit aussi être activé dans Windows "
                         "ou votre compositeur Wayland. S'applique après un redémarrage.",
    [UI_MENU_HDR_PEAK] = "Luminosité maximale",
    [UI_MENU_HDR_PEAK_DESC] = "Indiquez ici la luminosité maximale de votre écran, en nits.",
    [UI_MENU_HDR_GAME] = "Luminosité du jeu",
    [UI_MENU_HDR_GAME_DESC] = "Luminosité du blanc dans le jeu, en nits. 203 est la référence. Augmentez-la dans une "
                              "pièce claire.",
    [UI_MENU_HDR_UI] = "Luminosité de l'interface et du menu",
    [UI_MENU_HDR_UI_DESC] = "Luminosité du blanc de l'interface et de ce menu, en nits.",
    [UI_MENU_SHADOW_SCALE] = "Résolution des ombres",
    [UI_MENU_SHADOW_SCALE_DESC] = "Plus la valeur est haute, plus les bords des ombres sont nets et plus la carte "
                                  "graphique travaille. S'applique après un redémarrage.",
    [UI_MENU_PCSS] = "Ombres douces",
    [UI_MENU_PCSS_DESC] = "Les ombres restent nettes près des objets et s'adoucissent en s'éloignant. 0 garde les "
                          "ombres d'origine. S'applique après un redémarrage.",
    [UI_MENU_DLAA] = "DLSS / FSR 3.1",
    [UI_MENU_DLAA_DESC] = "Lisse les contours et réduit le scintillement de l'image 3D. Utilise NVIDIA DLSS sur les "
                          "cartes RTX, AMD FSR 3.1 sur les autres. S'applique après un redémarrage.",
    [UI_MENU_DLSS] = "Mode",
    [UI_MENU_DLSS_DESC] = "Natif : la meilleure image. Qualité, Équilibré et Performance dessinent à 67, 58 et 50 % "
                          "puis agrandissent, pour plus de FPS. S'applique après un redémarrage.",
    [UI_MENU_DLSS_DLAA] = "Natif (DLAA)", [UI_MENU_DLSS_QUALITY] = "Qualité", [UI_MENU_DLSS_BALANCED] = "Équilibré",
    [UI_MENU_DLSS_PERFORMANCE] = "Performance",
    [UI_MENU_DLSS_PRESET] = "Modèle DLSS",
    [UI_MENU_DLSS_PRESET_DESC] = "La version de DLSS utilisée. Gardez Automatique en cas de doute. K : la plus nette. J"
                                 " : moins de traînées derrière les personnages, un peu plus de scintillement.",
    [UI_MENU_DLSS_PRESET_DEFAULT] = "Automatique", [UI_MENU_DLSS_PRESET_K] = "K (le plus net)",
    [UI_MENU_DLSS_PRESET_J] = "J (moins de traînées)",
    [UI_MENU_FXAA] = "Anticrénelage (FXAA)",
    [UI_MENU_FXAA_DESC] = "Lisse les contours en escalier et adoucit un peu les détails. Inactif quand DLSS ou FSR 3.1 "
                          "est activé.",

    [UI_MENU_FPS_LIMIT] = "Images par seconde",
    [UI_MENU_FPS_LIMIT_DESC] = "Nombre d'images par seconde. 30 est le jeu d'origine, 60 est plus fluide et demande "
                               "environ deux fois plus de processeur. 120 et plus sont expérimentaux et demandent un "
                               "écran rapide.",
    [UI_MENU_FPS_30] = "30 FPS", [UI_MENU_FPS_60] = "60 FPS", [UI_MENU_FPS_120] = "120 FPS (expérimental)",
    [UI_MENU_FPS_144] = "144 FPS (expérimental)", [UI_MENU_FPS_240] = "240 FPS (expérimental)",
    [UI_MENU_GAME_LANG] = "Langue du jeu",
    [UI_MENU_GAME_LANG_DESC] = "Langue des textes et des voix du jeu. Automatique suit votre système.",
    [UI_MENU_GAME_LANG_AUTO] = "Automatique",
    [UI_MENU_OPERATION_MODE] = "Mode de la console",
    [UI_MENU_OPERATION_MODE_DESC] = "Le jeu se comporte comme une Switch sur station ou en portable.",
    [UI_MENU_MODE_DOCKED] = "Station (TV)", [UI_MENU_MODE_HANDHELD] = "Portable",
    [UI_MENU_FRAME_QUEUE] = "File d'images",
    [UI_MENU_FRAME_QUEUE_DESC] = "Prépare l'image suivante à l'avance : moins de saccades, jusqu'à une image de délai "
                                 "en plus sur les commandes.",
    [UI_MENU_ASSUME_FOCUS] = "Continuer en arrière-plan",
    [UI_MENU_ASSUME_FOCUS_DESC] = "Activé : le jeu continue quand vous utilisez une autre fenêtre. Désactivé : il se "
                                  "met en pause, mais un chargement interrompu ainsi peut rester bloqué.",

    [UI_MENU_MODS_INSTALLED] = "Mods installés",
    [UI_MENU_MODS_NONE] = "Aucun mod pour l'instant. Décompressez un mod dans le dossier mods, un dossier par mod avec son "
                          "dossier romfs dedans, puis relancez le jeu. TKMM peut aussi y exporter ses mods fusionnés dans "
                          "un dossier.",
    [UI_MENU_MODS_OPEN] = "Ouvrir le dossier des mods",
    [UI_MENU_MODS_OPEN_DESC] = "Ouvre le dossier où placer les mods.",
    [UI_MENU_MODS_ROW_DESC] = "Fichiers du jeu remplacés : %u. Nouveaux fichiers ajoutés : %u.",
    [UI_MENU_MODS_OVERRIDDEN] = "%u de ses fichiers viennent de %s, placé plus haut dans la liste.",
    [UI_MENU_MODS_MOVE_HINT] = "Le bouton du haut ou Ctrl+Haut le monte, le bouton de gauche ou Ctrl+Bas le descend. "
                               "Quand deux mods modifient le même fichier, celui du haut l'emporte. Les changements "
                               "s'appliquent après un redémarrage.",
    [UI_MENU_MODS_EMPTY] = "Ce dossier ne contient ni romfs, ni exefs, ni fichier de triche.",
    [UI_MENU_MODS_SUBSDK] = "Ce mod apporte son propre code de jeu (un plugin subsdk ou un main remplacé). Le portage ne "
                            "peut pas charger de modules de code, donc tout le mod reste désactivé.",
    [UI_MENU_MODS_EXEFS_LATER] = "Ce mod modifie le code du jeu (exefs). Le jeu intègre ces modifications à son prochain "
                                 "lancement (une recompilation de quelques minutes) ; d'ici là, tout le mod reste "
                                 "désactivé.",
    [UI_MENU_MODS_CHEATS] = "Codes de triche : %d. Activez-les dans l'onglet Triche.",
    [UI_MENU_MODS_CHEATS_FILE] = "Les codes de triche de cette version du jeu sont lus au lancement dans cheats\\%s de ce "
                                 "mod.",
    [UI_MENU_MODS_BAD_NAME] = "Le nom du dossier contient |, que la liste des mods utilise entre les noms. Renommez le "
                              "dossier.",
    [UI_MENU_MODS_OTHER_VERSION] = "%u de ses fichiers sont faits pour une autre version du jeu et sont ignorés. Cherchez "
                                   "une version de ce mod faite pour votre version du jeu.",
    [UI_MENU_MODS_UNUSED] = "inutilisé",

    [UI_MENU_VOLUME] = "Volume", [UI_MENU_VOLUME_DESC] = "Volume général du jeu.",
    [UI_MENU_MUTE] = "Muet", [UI_MENU_MUTE_DESC] = "Coupe le son sans changer le volume.",
    [UI_MENU_MUTE_BG] = "Muet en arrière-plan",
    [UI_MENU_MUTE_BG_DESC] = "Coupe le son quand vous êtes dans une autre fenêtre.",
    [UI_MENU_AUDIO_DEVICE] = "Périphérique de sortie",
    [UI_MENU_AUDIO_DEVICE_DESC] = "Où le son est joué. Système suit le réglage de Windows.",
    [UI_MENU_AUDIO_DEVICE_DEFAULT] = "Système (par défaut)",
    [UI_MENU_AUDIO_SURROUND] = "Son surround 5.1",
    [UI_MENU_AUDIO_SURROUND_DESC] = "Envoie les six canaux du jeu vers un système 5.1. Laissez désactivé pour des enceintes stéréo ou un casque.",
    [UI_MENU_INVERT_LX] = "Inverser le stick gauche X", [UI_MENU_INVERT_LY] = "Inverser le stick gauche Y",
    [UI_MENU_INVERT_RX] = "Inverser le stick droit X (caméra)", [UI_MENU_INVERT_RY] = "Inverser le stick droit Y (caméra)",
    [UI_MENU_INVERT_DESC] = "Inverse cette direction du stick.",

    [UI_MENU_MOTION] = "Commandes par mouvement",
    [UI_MENU_MOTION_DESC] = "Visez en inclinant la manette (gyroscope).",
    [UI_MENU_RUMBLE_STRENGTH] = "Force des vibrations",
    [UI_MENU_RUMBLE_STRENGTH_DESC] = "Force des vibrations de la manette.",
    [UI_MENU_RUMBLE_CURVE] = "Courbe des vibrations",
    [UI_MENU_RUMBLE_CURVE_DESC] = "Plus la valeur est basse, plus les petites vibrations se sentent.",
    [UI_MENU_RUMBLE_TRIGGERS] = "Vibration des gâchettes (Xbox)",
    [UI_MENU_RUMBLE_TRIGGERS_DESC] = "Fait aussi vibrer les gâchettes d'une manette Xbox.",
    [UI_MENU_PAD_LAYOUT] = "Disposition des boutons",
    [UI_MENU_PAD_LAYOUT_DESC] = "Quel bouton physique sert de A. Nintendo : celui de droite. Xbox et PlayStation : "
                                "celui du bas. Automatique choisit selon votre manette.",
    [UI_MENU_PAD_LAYOUT_AUTO] = "Automatique", [UI_MENU_PAD_LAYOUT_NINTENDO] = "Nintendo (A à droite)",
    [UI_MENU_PAD_LAYOUT_XBOX] = "Xbox (A en bas)",
    [UI_MENU_PAD_LAYOUT_PLAYSTATION] = "PlayStation (croix = A)",
    [UI_MENU_PAD_LAYOUT_IN_USE] = "utilisé : %s",
    [UI_MENU_PROMPTS] = "Icônes des boutons",
    [UI_MENU_PROMPTS_DESC] = "Icônes de boutons affichées dans le jeu. Un bouton réassigné garde son icône d'origine.",
    [UI_MENU_PROMPTS_NINTENDO] = "Nintendo", [UI_MENU_PROMPTS_XBOX] = "Xbox",
    [UI_MENU_PROMPTS_PLAYSTATION] = "PlayStation",
    [UI_MENU_PROMPTS_MISSING] = "fichiers d'icônes absents : relancez le setup",
    [UI_MENU_PROMPTS_FAILED] = "fichiers d'icônes abîmés : relancez le setup",
    [UI_MENU_PROMPTS_RESTART] = "les icônes changent après redémarrage",
    [UI_MENU_RESTART] = "Redémarrer maintenant",
    [UI_MENU_RESTART_DESC] = "Relance le jeu pour appliquer les nouvelles icônes. La progression non sauvegardée est "
                             "perdue.",
    [UI_MENU_RESTART_CONFIRM] = "Redémarrer le jeu maintenant ? La progression depuis votre dernière sauvegarde est perdue.",
    [UI_MENU_RESTART_YES] = "Redémarrer",
    [UI_MENU_RESTART_SAVING] = "Le jeu sauvegarde : le redémarrage sera possible dans un instant.",
    [UI_MENU_LSTICK_DEADZONE] = "Zone morte du stick gauche", [UI_MENU_RSTICK_DEADZONE] = "Zone morte du stick droit",
    [UI_MENU_STICK_DEADZONE_DESC] = "Distance à parcourir pour que le stick compte. Si le personnage ou la caméra bouge"
                                    " seul, réglez un peu au-dessus de la valeur \"stick actuel\".",
    [UI_MENU_STICK_DEADZONE_NOW] = "stick actuel : %.0f %%",
    [UI_MENU_MOUSE_SENS] = "Sensibilité de la caméra à la souris",
    [UI_MENU_MOUSE_SENS_DESC] = "Vitesse de la caméra à la souris.",
    [UI_MENU_WALK_KEY] = "Touche de marche",
    [UI_MENU_WALK_KEY_DESC] = "Maintenez cette touche pour marcher au lieu de courir au clavier.",
    [UI_MENU_WALK_KEY_TAKEN] = "aussi %s",
    [UI_MENU_WALK_SCALE] = "Échelle de marche",
    [UI_MENU_WALK_SCALE_DESC] = "Vitesse quand la touche de marche est maintenue. La moitié = marche, le maximum = "
                                "course.",

    [UI_BINDS_HELP] = "Choisissez une case et appuyez sur A ou Entrée, puis sur la touche, le bouton de souris ou le bouton "
                      "de manette. Suppr ou X vide une case. Une touche déjà utilisée est déplacée ici ; un bouton de "
                      "manette est échangé avec celui qu'il remplace.",
    [UI_BINDS_COL_KEY1] = "Touche 1", [UI_BINDS_COL_KEY2] = "Touche 2", [UI_BINDS_COL_PAD] = "Manette",
    [UI_BINDS_GROUP_BUTTONS] = "Boutons", [UI_BINDS_GROUP_MOVE] = "Déplacement", [UI_BINDS_GROUP_CAMERA] = "Caméra",
    [UI_BINDS_PRESS_KEY] = "Appuyez sur une touche ou un bouton de souris pour %s",
    [UI_BINDS_PRESS_PAD] = "Appuyez sur un bouton de manette pour %s",
    [UI_BINDS_CANCEL_HINT] = "Échap annule, ou attendez %d s",
    [UI_BINDS_TAKEN] = "%s a été retiré de %s", [UI_BINDS_SWAPPED] = "%s échangé avec %s",
    [UI_BINDS_RESET] = "Réinitialiser tous les boutons",
    [UI_BINDS_RESET_DESC] = "Revient aux touches et boutons de manette par défaut. La disposition des boutons ci-dessus s'applique à nouveau : les boutons de manette réassignés restent où vous les avez mis jusqu'à cette réinitialisation.",
    [UI_BIND_A] = "A", [UI_BIND_B] = "B", [UI_BIND_X] = "X", [UI_BIND_Y] = "Y",
    [UI_BIND_LCLICK] = "Clic du stick gauche", [UI_BIND_RCLICK] = "Clic du stick droit", [UI_BIND_L] = "L",
    [UI_BIND_R] = "R", [UI_BIND_ZL] = "ZL", [UI_BIND_ZR] = "ZR", [UI_BIND_PLUS] = "Plus", [UI_BIND_MINUS] = "Moins",
    [UI_BIND_LEFT] = "Croix directionnelle gauche", [UI_BIND_UP] = "Croix directionnelle haut",
    [UI_BIND_RIGHT] = "Croix directionnelle droite", [UI_BIND_DOWN] = "Croix directionnelle bas",
    [UI_BIND_LSTICK_LEFT] = "Aller à gauche", [UI_BIND_LSTICK_UP] = "Avancer", [UI_BIND_LSTICK_RIGHT] = "Aller à droite",
    [UI_BIND_LSTICK_DOWN] = "Reculer",
    [UI_BIND_RSTICK_LEFT] = "Caméra à gauche", [UI_BIND_RSTICK_UP] = "Caméra vers le haut",
    [UI_BIND_RSTICK_RIGHT] = "Caméra à droite", [UI_BIND_RSTICK_DOWN] = "Caméra vers le bas",
    [UI_BIND_DESC_A] = "Interagir, ramasser, parler.", [UI_BIND_DESC_B] = "Courir, annuler, revenir.",
    [UI_BIND_DESC_X] = "Sauter.", [UI_BIND_DESC_Y] = "Attaquer.", [UI_BIND_DESC_LCLICK] = "S'accroupir.",
    [UI_BIND_DESC_RCLICK] = "Vue à la première personne.", [UI_BIND_DESC_L] = "Capacités.",
    [UI_BIND_DESC_R] = "Lancer une arme.", [UI_BIND_DESC_ZL] = "Bouclier, verrouillage.", [UI_BIND_DESC_ZR] = "Arc.",
    [UI_BIND_DESC_PLUS] = "Menu, inventaire.", [UI_BIND_DESC_MINUS] = "Carte.",
    [UI_BIND_DESC_DPAD] = "Sélection rapide.",
    [UI_BIND_DESC_MOVE] = "Déplace Link comme le stick gauche. Le stick gauche de la manette marche toujours aussi.",
    [UI_BIND_DESC_CAMERA] = "Tourne la caméra comme le stick droit. La souris et le stick droit de la manette marchent "
                            "toujours aussi.",
    [UI_KEY_SPACE] = "Espace", [UI_KEY_TAB] = "Tab", [UI_KEY_LSHIFT] = "Maj gauche", [UI_KEY_LCTRL] = "Ctrl gauche",
    [UI_KEY_UP] = "Flèche haut", [UI_KEY_DOWN] = "Flèche bas", [UI_KEY_LEFT] = "Flèche gauche",
    [UI_KEY_RIGHT] = "Flèche droite",
    [UI_KEY_MOUSE_L] = "Clic gauche", [UI_KEY_MOUSE_R] = "Clic droit", [UI_KEY_MOUSE_M] = "Clic molette",
    [UI_KEY_MOUSE_X1] = "Souris 4", [UI_KEY_MOUSE_X2] = "Souris 5",
    [UI_KEY_ENTER] = "Entrée", [UI_KEY_BACKSPACE] = "Retour arrière", [UI_KEY_RSHIFT] = "Maj droite",
    [UI_KEY_RCTRL] = "Ctrl droit", [UI_KEY_LALT] = "Alt", [UI_KEY_CAPSLOCK] = "Verr. Maj", [UI_KEY_INSERT] = "Inser",
    [UI_KEY_DELETE] = "Suppr", [UI_KEY_HOME] = "Début", [UI_KEY_END] = "Fin", [UI_KEY_PAGEUP] = "Page préc.",
    [UI_KEY_PAGEDOWN] = "Page suiv.", [UI_KEY_NUM_FMT] = "Pavé num. %s",
    [UI_PAD_SOUTH] = "Bouton du bas", [UI_PAD_EAST] = "Bouton de droite", [UI_PAD_WEST] = "Bouton de gauche",
    [UI_PAD_NORTH] = "Bouton du haut", [UI_PAD_LSTICK] = "Stick gauche", [UI_PAD_RSTICK] = "Stick droit",
    [UI_PAD_L] = "Bouton épaule gauche", [UI_PAD_R] = "Bouton épaule droit", [UI_PAD_START] = "Start",
    [UI_PAD_BACK] = "Back",
    [UI_PAD_DPAD_LEFT] = "Croix gauche", [UI_PAD_DPAD_UP] = "Croix haut", [UI_PAD_DPAD_RIGHT] = "Croix droite",
    [UI_PAD_DPAD_DOWN] = "Croix bas", [UI_PAD_ZL] = "Gâchette gauche", [UI_PAD_ZR] = "Gâchette droite",
    [UI_PAD_LPADDLE1] = "Palette arrière gauche", [UI_PAD_RPADDLE1] = "Palette arrière droite",
    [UI_PAD_LPADDLE2] = "Palette arrière gauche 2", [UI_PAD_RPADDLE2] = "Palette arrière droite 2",
    [UI_PAD_CROSS] = "Croix", [UI_PAD_CIRCLE] = "Rond", [UI_PAD_SQUARE] = "Carré", [UI_PAD_TRIANGLE] = "Triangle",

    [UI_MENU_AMIIBO_DIR] = "Dossier des amiibo",
    [UI_MENU_AMIIBO_OPEN] = "Ouvrir",
    [UI_MENU_AMIIBO_DEFAULT_DIR] = "Dossier par défaut",
    [UI_MENU_AMIIBO_REFRESH] = "Actualiser",
    [UI_MENU_AMIIBO_PICK] = "Copies d'amiibo (fichiers .bin ou .nfc de 540 octets) :",
    [UI_MENU_AMIIBO_NO_DIR] = "Impossible d'ouvrir le dossier %s",
    [UI_MENU_AMIIBO_READER] = "Lecteur NFC",
    [UI_MENU_AMIIBO_STATE] = "Lecteur :",
    [UI_MENU_AMIIBO_STATE_IDLE] = "prêt",
    [UI_MENU_AMIIBO_STATE_SEARCHING] = "le jeu attend un amiibo",
    [UI_MENU_AMIIBO_STATE_FOUND] = "amiibo sur le lecteur",
    [UI_MENU_AMIIBO_STATE_MOUNTED] = "le jeu lit l'amiibo",
    [UI_MENU_AMIIBO_STATE_OFF] = "éteint",
    [UI_MENU_AMIIBO_FILE] = "Fichier choisi :",
    [UI_MENU_AMIIBO_NO_FILE] = "(aucun fichier choisi)",
    [UI_MENU_AMIIBO_TAP] = "Poser l'amiibo",
    [UI_MENU_AMIIBO_REMOVE] = "Retirer l'amiibo",
    [UI_MENU_AMIIBO_AUTO_REMOVE] = "Retirer l'amiibo tout seul après l'avoir posé",
    [UI_MENU_AMIIBO_HOLD] = "Temps sur le lecteur",
    [UI_MENU_AMIIBO_SECONDS] = "%.1f s",
    [UI_MENU_AMIIBO_TAG] = "Amiibo sur le lecteur :",
    [UI_MENU_AMIIBO_NO_TAG] = "Aucun amiibo sur le lecteur.",
    [UI_MENU_AMIIBO_TAG_FILE] = "Fichier : %s",
    [UI_MENU_AMIIBO_TAG_ID] = "Personnage %s, série %02x, numéro %04x, type %02x",
    [UI_MENU_AMIIBO_TAG_UID] = "UID : %s",
    [UI_MENU_AMIIBO_READY] = "Prêt",
    [UI_MENU_AMIIBO_TAPPED] = "Posé : %s",
    [UI_MENU_AMIIBO_LOAD_FAILED] = "Pas une copie d'amiibo (NTAG215 de 540 octets) : %s",
    [UI_MENU_AMIIBO_REMOVED] = "Amiibo retiré",
    [UI_MENU_AMIIBO_STATUS] = "État : %s",
    [UI_MENU_MENU_LANG] = "Langue du menu",
    [UI_MENU_MENU_LANG_DESC] = "Langue de ce menu. Automatique suit votre système si possible, sinon anglais.",
    [UI_MENU_MENU_LANG_AUTO] = "Automatique (système)",

    [UI_MENU_ADV_WARNING] = "Ces réglages servent aux tests et au dépannage. Ne les changez que si vous savez pourquoi.",
    [UI_MENU_GROUP_SCHED] = "Ordonnancement", [UI_MENU_GROUP_SHADERS] = "Shaders et pipelines",
    [UI_MENU_GROUP_TEXMEM] = "Textures et mémoire", [UI_MENU_GROUP_FIXES] = "Corrections du rendu",
    [UI_MENU_GROUP_VULKAN] = "Vulkan", [UI_MENU_GROUP_DIAG] = "Diagnostic",
    [UI_MENU_ASYNC_PREEMPT] = "Préemption des cœurs virtuels",
    [UI_MENU_ASYNC_PREEMPT_DESC] = "Permet à un thread du jeu d'en interrompre un autre. Gardez la valeur par défaut "
                                   "sauf en cas de problème.",
    [UI_MENU_PREEMPT_OFF] = "Désactivée", [UI_MENU_PREEMPT_ON] = "Activée", [UI_MENU_PREEMPT_INHERIT] = "Héritage seul",
    [UI_MENU_REPLAY_ASYNC] = "Préparation GPU sur son propre thread",
    [UI_MENU_REPLAY_ASYNC_DESC] = "Prépare le travail de la carte graphique sur un thread séparé. Aide à tenir 60 FPS "
                                  "dans les gros combats, ajoute jusqu'à une image de délai.",
    [UI_MENU_TIMER_1MS] = "Minuteur Windows à 1 ms",
    [UI_MENU_TIMER_1MS_DESC] = "Utilise un minuteur Windows plus précis pour des images plus régulières.",
    [UI_MENU_PIPE_WAIT] = "Gestion des draws en attente",
    [UI_MENU_PIPE_WAIT_DESC] = "Comment les dessins attendent un pipeline en construction. Gardez Par défaut. Ancien est le"
                               " comportement précédent.",
    [UI_MENU_PIPE_WAIT_DEFAULT] = "Par défaut", [UI_MENU_PIPE_WAIT_DRAW] = "Ancien (draw)",
    [UI_MENU_PIPE_SKIP] = "Draws sautés pendant la création d'un pipeline",
    [UI_MENU_PIPE_SKIP_DESC] = "Quels objets ne sont brièvement pas dessinés pendant la construction de leur pipeline.",
    [UI_MENU_PIPE_SKIP_STRICT] = "Strict", [UI_MENU_PIPE_SKIP_ANY] = "Tous",
    [UI_MENU_PIPE_SKIP_FULLSCREEN] = "Passes plein écran",
    [UI_MENU_LIB_PREBUILD] = "Précompilation des bibliothèques d'étapes",
    [UI_MENU_LIB_PREBUILD_DESC] = "Prépare les shaders en arrière-plan pour réduire les saccades.",
    [UI_MENU_LIB_PREBUILD_THREADS] = "Threads de précompilation",
    [UI_MENU_LIB_PREBUILD_THREADS_DESC] = "Threads du processeur utilisés pour cette préparation. Automatique s'adapte "
                                          "à votre processeur.",
    [UI_MENU_PIPE_PREBUILD] = "Précompilation des pipelines",
    [UI_MENU_PIPE_PREBUILD_DESC] = "Prépare à l'avance les pipelines connus pour réduire les saccades.",
    [UI_MENU_PREWARM_XLATE] = "Préchauffage du traducteur",
    [UI_MENU_PREWARM_XLATE_DESC] = "Threads qui préparent les shaders connus au démarrage. Automatique en utilise la "
                                   "moitié, 16 au maximum.",
    [UI_MENU_GPL] = "Graphics pipeline library",
    [UI_MENU_GPL_DESC] = "Construit les pipelines plus vite grâce à une fonction Vulkan. Désactivez-la si votre pilote "
                         "pose problème.",
    [UI_MENU_ASTC] = "Décodage des textures ASTC", [UI_MENU_ASTC_DESC] = "Où les textures compressées du jeu sont "
                                                                         "décodées. Gardez Automatique.",
    [UI_MENU_ASTC_AUTO] = "Automatique", [UI_MENU_ASTC_GPU] = "Shader GPU", [UI_MENU_ASTC_NATIVE] = "Natif",
    [UI_MENU_ASTC_CPU] = "Processeur",
    [UI_MENU_MEMORY] = "Mode de mémoire GPU", [UI_MENU_MEMORY_DESC] = "Comment la mémoire du jeu est partagée avec la "
                                                                      "carte graphique. Gardez Automatique.",
    [UI_MENU_MEMORY_AUTO] = "Automatique", [UI_MENU_MEMORY_IMPORT] = "Import", [UI_MENU_MEMORY_MIRROR] = "Miroir",
    [UI_MENU_MEMORY_VERIFY] = "Vérification",
    [UI_MENU_TEX_ALIAS] = "Alias de textures", [UI_MENU_TEX_ALIAS_DESC] = "Une nouvelle texture placée sur une ancienne"
                                                                          " en mémoire reprend son contenu. Pour les "
                                                                          "bugs d'image.",
    [UI_MENU_TEX_VRAM_PRESSURE] = "Limite de mémoire des textures", [UI_MENU_TEX_VRAM_PRESSURE_DESC] = "Libère les "
                                                                                                       "textures "
                                                                                                       "inutilisées au-"
                                                                                                       "delà de ce "
                                                                                                       "pourcentage de "
                                                                                                       "mémoire vidéo. "
                                                                                                       "0 ne les libère"
                                                                                                       " jamais.",
    [UI_MENU_ASTC_BC] = "Compresser l'ASTC en BC3", [UI_MENU_ASTC_BC_DESC] = "Utilise quatre fois moins de mémoire "
                                                                             "vidéo, avec des textures un peu plus "
                                                                             "grossières. Pour les cartes de 4 Go ou "
                                                                             "moins.",
    [UI_MENU_FIX_C0] = "Correction du constant buffer",
    [UI_MENU_FIX_C0_DESC] = "Donne aux shaders les données que le pilote de la console fournit normalement, au lieu de "
                            "zéros. Pour les bugs d'image.",
    [UI_MENU_FIX_ZERO] = "Mettre à zéro la nouvelle mémoire GPU",
    [UI_MENU_FIX_ZERO_DESC] = "La nouvelle mémoire graphique démarre vide, comme sur Switch.",
    [UI_MENU_BIND_SKIP] = "Sauter les liaisons répétées",
    [UI_MENU_BIND_SKIP_DESC] = "Optimisation : ignore les liaisons qui n'ont pas changé.",
    [UI_MENU_SHADER_MEMO] = "Mémo des shaders", [UI_MENU_SHADER_MEMO_DESC] = "Optimisation : mémorise les recherches de"
                                                                             " shaders.",
    [UI_MENU_PIPE_MEMO] = "Mémo des pipelines", [UI_MENU_PIPE_MEMO_DESC] = "Optimisation : réutilise le pipeline "
                                                                           "précédent si rien n'a changé.",
    [UI_MENU_TEX_MEMO] = "Mémo des liaisons de textures", [UI_MENU_TEX_MEMO_DESC] = "Optimisation : mémorise les "
                                                                                    "textures entre les dessins.",
    [UI_MENU_DESC_TEMPLATE] = "Modèles de descripteurs",
    [UI_MENU_DESC_TEMPLATE_DESC] = "Optimisation : met à jour les descripteurs par lots.",
    [UI_MENU_PUSH_SKIP] = "Sauter les push constants répétées",
    [UI_MENU_PUSH_SKIP_DESC] = "Optimisation : ignore les constantes qui n'ont pas changé.",
    [UI_MENU_SPLIT_DRAWS] = "Envoi anticipé au GPU",
    [UI_MENU_SPLIT_DRAWS_DESC] = "Envoie le travail graphique par morceaux pour qu'il démarre plus tôt. La valeur est "
                                 "le nombre de dessins par morceau. 0 désactive.",
    [UI_MENU_VK_VALIDATION] = "Couche de validation",
    [UI_MENU_VK_VALIDATION_DESC] = "Pour le débogage uniquement : vérifie chaque appel Vulkan et ralentit beaucoup le "
                                   "jeu.",
    [UI_MENU_VK_VALIDATION_MISSING] = "La couche de validation n'est pas installée.",
    [UI_MENU_DLAA_MISSING] = "DLSS n'est pas disponible : il faut une carte NVIDIA RTX et nvngx_dlss.dll, que le build Windows télécharge. Relancez le build, ou mettez le fichier à côté de AgeOfCalamityRecomp.exe, et redémarrez. Ou réglez le moteur sur Automatique ou AMD FSR 3.1.",
    [UI_MENU_VK_DYN_VERTEX] = "État des sommets dynamique",
    [UI_MENU_VK_DYN_VERTEX_DESC] = "Force cette fonction Vulkan. La valeur par défaut dépend de votre carte graphique.",
    [UI_MENU_VK_DYN_A2C] = "Alpha to coverage dynamique",
    [UI_MENU_VK_DYN_A2C_DESC] = "Force cette fonction Vulkan. Par défaut : désactivée sur AMD RDNA 2 et plus ancien, où"
                                " elle rend les personnages noirs.",
    [UI_MENU_VK_SUBGROUP_PIN] = "Fixer la taille des subgroups",
    [UI_MENU_VK_SUBGROUP_PIN_DESC] = "Force une taille fixe de sous-groupe des shaders. Pour le dépannage.",
    [UI_MENU_TRACE] = "Journal de performances", [UI_MENU_TRACE_DESC] = "Écrit les temps d'image dans un fichier.",
    [UI_MENU_FLIGHT] = "Enregistreur de vol",
    [UI_MENU_FLIGHT_DESC] = "Enregistre l'activité récente quand une image se fige, pour signaler les bugs.",
    [UI_MENU_FLIGHT_SPIKE_MS] = "Seuil de blocage (ms)",
    [UI_MENU_FLIGHT_SPIKE_MS_DESC] = "Durée d'image, en ms, considérée comme un gel.",
    [UI_MENU_INPUT_RECORD] = "Enregistrement des entrées",
    [UI_MENU_INPUT_RECORD_DESC] = "Écrit vos commandes dans un fichier pour les rapports de bug.",
    [UI_MENU_OPEN_LOG] = "Ouvrir le dossier des journaux", [UI_MENU_OPEN_LOG_DESC] = "Ouvre le dossier des fichiers de "
                                                                                     "log.",
    [UI_MENU_COPY_INFO] = "Copier les infos système",
    [UI_MENU_COPY_INFO_DESC] = "Copie les infos de votre PC et les réglages modifiés, à coller dans un rapport de bug.",
    [UI_MENU_COPY_INFO_DONE] = "Copié", [UI_MENU_ACTION_FAILED] = "n'a pas marché",
    [UI_MENU_RESET] = "Remettre tous les réglages par défaut", [UI_MENU_RESET_DESC] = "Remet tous les réglages de tous "
                                                                                      "les onglets par défaut.",
    [UI_MENU_RESET_CONFIRM] = "Remettre chaque réglage à sa valeur par défaut ?", [UI_MENU_RESET_YES] = "Réinitialiser",
    [UI_MENU_RESET_NO] = "Annuler",

    [UI_MENU_OV_FPS] = "%.0f FPS", [UI_MENU_OV_FRAME_MS] = "%.1f ms", [UI_MENU_OV_GAME_PCT] = "jeu %.0f%%",
    [UI_MENU_OV_MAX_MS] = "max %.1f ms",
    [UI_MENU_OV_GPU] = "GPU", [UI_MENU_OV_CPU] = "CPU", [UI_MENU_OV_RAM] = "RAM", [UI_MENU_OV_VRAM] = "VRAM",
    [UI_MENU_OV_NONE] = "-",
    [UI_MENU_OV_REPLAY] = "Préparation GPU", [UI_MENU_OV_GPU_TIME] = "GPU occupé par image",
    [UI_MENU_OV_PIPE_WAIT] = "Attentes de pipelines",
    [UI_MENU_OV_SHADER_WAIT] = "Attentes de shaders", [UI_MENU_OV_TEX_UPLOADS] = "Envois de textures",
    [UI_MENU_OV_FMT_PCT] = "%.0f%%", [UI_MENU_OV_FMT_TEMP] = "%.0f °C", [UI_MENU_OV_FMT_POWER] = "%.0f W",
    [UI_MENU_OV_FMT_MHZ] = "%.0f MHz", [UI_MENU_OV_FMT_GB] = "%.1f Go", [UI_MENU_OV_FMT_GB_OF] = "/ %.1f Go",
    [UI_MENU_OV_GAME_GB] = "jeu %.1f Go", [UI_MENU_OV_FMT_MS] = "%.2f ms", [UI_MENU_OV_PER_FRAME] = "%.1f par image",
    [UI_MENU_OV_WINDOW] = "Fenêtre", [UI_MENU_OV_OUTPUT] = "Image du jeu", [UI_MENU_OV_SCENE] = "Scène 3D",
    [UI_MENU_OV_SCALING] = "Mise à l'échelle", [UI_MENU_OV_LIMIT] = "Limite d'images", [UI_MENU_OV_PRESENT] = "Présentation",
    [UI_MENU_OV_FMT_SIZE] = "%ux%u", [UI_MENU_OV_FMT_SCENE] = "%ux%u, %.0f%%",
    [UI_MENU_OV_FMT_FSR] = "AMD FSR 1, netteté %d%%", [UI_MENU_OV_SCALE_NONE] = "aucune (1:1)",
    [UI_MENU_OV_SCALE_BILINEAR] = "bilinéaire",
    [UI_MENU_OV_SCALE_DOWN] = "bilinéaire (FSR ne fait qu'agrandir)",
    [UI_MENU_TAB_CHEATS] = "Triche",
    [UI_MENU_CHEATS_NONE] = "Aucun code de triche pour cette version du jeu. Les codes d'un mod sont un fichier de triche "
                            "Atmosphère dans son dossier cheats, nommé d'après le build du jeu : "
                            "mods\\<mod>\\cheats\\%s. Relancez le jeu pour les lire. Pour les codes qui modifient le code "
                            "du jeu, le jeu se recompile au lancement (quelques minutes).",
    [UI_MENU_CHEATS_MOD] = "%s : %d codes",
    [UI_MENU_CHEAT_CODE_DESC] = "Modifie le code du jeu. Bascule tout de suite.",
    [UI_MENU_CHEAT_KEY_DESC] = "Modifie le code du jeu tant que vous maintenez %s. Bascule tout de suite.",
    [UI_MENU_CHEAT_DATA_DESC] = "Écrit dans la mémoire du jeu à chaque image tant qu'il est activé.",
    [UI_MENU_CHEAT_MASTER] = "toujours actif",
    [UI_MENU_CHEAT_MASTER_DESC] = "Le code maître du fichier : il est toujours actif.",
    [UI_MENU_CHEAT_RESTORE] = "inutile",
    [UI_MENU_CHEAT_RESTORE_DESC] = "Il ne fait que réécrire le code d'origine du jeu. Désactiver un code le fait déjà.",
    [UI_MENU_CHEAT_REBUILD] = "après la recompilation",
    [UI_MENU_CHEAT_REBUILD_DESC] = "Ce code modifie le code du jeu et il est nouveau ou modifié depuis la compilation du "
                                   "jeu. Le jeu l'intègre à son prochain lancement (quelques minutes), ou utilisez "
                                   "Recompiler le jeu maintenant.",
    [UI_MENU_CHEAT_UNSUPPORTED] = "non pris en charge",
    [UI_MENU_CHEAT_UNSUPPORTED_DESC] = "Ce code ne peut pas être utilisé avec cette version du jeu :",
    [UI_MENU_CHEATS_REBUILD] = "Recompiler le jeu maintenant",
    [UI_MENU_CHEATS_REBUILD_DESC] = "Certains codes modifient le code du jeu et n'y sont pas intégrés : ils sont nouveaux "
                                    "ou modifiés, ou leur dernière recompilation a échoué ou a été annulée. Ferme le jeu ; "
                                    "le setup les intègre (quelques minutes) et relance le jeu. La progression depuis "
                                    "votre dernière sauvegarde est perdue.",
    [UI_MENU_MODS_EXEFS_UNSUPPORTED] = "Ce mod modifie le code du jeu (exefs) d'une façon que le portage ne peut pas "
                                       "appliquer, donc tout le mod reste désactivé. %s",
    [UI_MENU_MODS_EXEFS_OTHER] = "Ses modifications de code (exefs) sont faites pour une autre version du jeu.",
    [UI_MENU_MODS_EXEFS_ON] = "Modifications de code utilisées : %d fichiers.",
    [UI_MENU_MODS_REPLACES_BUILTIN] = "Modifications de code utilisées : %d fichiers. Elles remplacent le patch %s du "
                                      "portage, qui reste désactivé tant que ce mod est actif.",
    // FSR 3.1 and gyro calibration
    [UI_MENU_DLAA_ENGINE] = "Moteur",
    [UI_MENU_DLAA_ENGINE_DESC] = "Automatique : DLSS si votre carte le permet, sinon FSR 3.1. FSR 3.1 fonctionne sur "
                                 "toutes les cartes graphiques. S'applique après un redémarrage.",
    [UI_MENU_DLAA_ENGINE_AUTO] = "Automatique",
    [UI_MENU_DLAA_ENGINE_DLSS] = "NVIDIA DLSS",
    [UI_MENU_DLAA_ENGINE_FSR3] = "AMD FSR 3.1",
    [UI_MENU_FSR3_SHARPNESS] = "Netteté FSR 3.1",
    [UI_MENU_FSR3_SHARPNESS_DESC] = "Accentue la netteté de l'image 3D, avec FSR 3.1 uniquement. 0 désactive.",
    [UI_MENU_AIM_LEFT_STICK] = "Viser avec le stick gauche",
    [UI_MENU_AIM_LEFT_STICK_DESC] = "Activé : le stick gauche vise aussi à l'arc, comme dans le jeu d'origine. "
                                    "Désactivé : seul le stick droit vise.",
    [UI_MENU_GYRO_CALIBRATION] = "Calibration du gyroscope",
    [UI_MENU_GYRO_CALIBRATION_DESC] = "Corrige la dérive du gyroscope. Désactivé : gyroscope brut.",
    [UI_MENU_CALIBRATE_GYRO] = "Calibrer le gyroscope",
    [UI_MENU_CALIBRATE_GYRO_DESC] = "Posez la manette à plat, appuyez et ne la touchez plus pendant 2 secondes. Empêche"
                                    " la visée de dériver.",
    [UI_MENU_CALIB_NONE] = "non calibré",
    [UI_MENU_CALIB_COLLECTING] = "ne bougez plus...",
    [UI_MENU_CALIB_DONE] = "calibré",
    [UI_MENU_CALIB_FAILED] = "pas de données du gyroscope",
    [UI_MENU_CALIB_MOVED] = "elle a bougé, recommencez",
};

#define SWKBD_ONLY(name, ...) static const char *const name[UI_TEXT_COUNT] = {SWKBD(__VA_ARGS__)}
SWKBD_ONLY(text_ja, "文字入力", "名前を入力してください", "ウマ", "OK", "キャンセル", "コントローラー: A = OK、B = キャンセル");
SWKBD_ONLY(text_de, "Texteingabe", "Gib einen Namen ein", "Pferd", "OK", "Abbrechen", "Controller: A = OK, B = Abbrechen");
SWKBD_ONLY(text_it, "Inserimento testo", "Inserisci un nome", "Cavallo", "OK", "Annulla", "Controller: A = OK, B = Annulla");
SWKBD_ONLY(text_es, "Introducir texto", "Introduce un nombre", "Caballo", "Aceptar", "Cancelar",
           "Mando: A = Aceptar, B = Cancelar");
SWKBD_ONLY(text_nl, "Tekstinvoer", "Voer een naam in", "Paard", "OK", "Annuleren", "Controller: A = OK, B = Annuleren");
SWKBD_ONLY(text_zh_hant, "文字輸入", "請輸入名字", "馬兒", "確定", "取消", "手把：A 確定，B 取消");
SWKBD_ONLY(text_ko, "문자 입력", "이름을 입력하세요", "말", "확인", "취소", "컨트롤러: A 확인, B 취소");
SWKBD_ONLY(text_ru, "Ввод текста", "Введите имя", "Лошадь", "ОК", "Отмена", "Контроллер: A — ОК, B — отмена");
// Simplified Chinese: the keyboard, and the whole menu and overlay (menu language "zh-Hans").
static const char *const text_zh_hans[UI_TEXT_COUNT] = {
    [UI_MENU_QUICK_TITLE] = "Age of Calamity Recomp  ·  快捷菜单",
    [UI_MENU_QUICK_HINT] = "按住 L3+R3 打开或关闭，按 B 关闭。",
    [UI_MENU_QUICK_FPS] = "帧率",
    [UI_MENU_QUICK_FPS_DESC] = "每秒画面数。30 是原版游戏，60 更流畅，CPU 占用约为两倍。120 及以上为实验性功能，需要高刷新率显示器。",
    [UI_MENU_QUICK_RES] = "主机模式",
    [UI_MENU_QUICK_RES_DESC] = "让游戏以为 Switch 处于电视模式或掌机模式。不改变画面尺寸。",
    [UI_MENU_QUICK_RES_HANDHELD] = "掌机",
    [UI_MENU_QUICK_RES_DOCKED] = "电视（底座）",
    [UI_MENU_QUICK_FULL] = "完整菜单",
    [UI_MENU_QUICK_MARK] = "标记此刻",
    SWKBD("文字输入", "请输入名字", "马儿", "确定", "取消", "手柄：A 确定，B 取消"),

    [UI_MENU_TITLE] = "Age of Calamity Recomp",
    [UI_MENU_TAB_SCREEN] = "画面", [UI_MENU_TAB_GAME] = "游戏", [UI_MENU_TAB_MODS] = "模组", [UI_MENU_TAB_SOUND] = "声音",
    [UI_MENU_TAB_CONTROLS] = "操作", [UI_MENU_TAB_AMIIBO] = "Amiibo", [UI_MENU_TAB_ADVANCED] = "高级",
    [UI_MENU_TAB_LANGUAGE] = "语言",
    [UI_MENU_CONTINUE] = "继续", [UI_MENU_QUIT] = "退出游戏",
    [UI_MENU_ON] = "开", [UI_MENU_OFF] = "关", [UI_MENU_AUTO] = "自动", [UI_MENU_DEFAULT_MARK] = "默认",
    [UI_MENU_AFTER_RESTART] = "重启后生效", [UI_MENU_SET_BY_ENV] = "已由环境变量设定",
    [UI_MENU_SAVE_FAILED] = "无法保存设置",
    [UI_MENU_VERSION] = "设置  ·  版本 %s  ·  游戏 %s",

    [UI_MENU_FULLSCREEN] = "全屏", [UI_MENU_FULLSCREEN_DESC] = "铺满整个屏幕。也可以按 F11 切换。",
    [UI_MENU_WINDOW_SIZE] = "窗口大小", [UI_MENU_WINDOW_SIZE_DESC] = "游戏不全屏时的窗口尺寸。",
    [UI_MENU_PRESENT] = "画面呈现方式",
    [UI_MENU_PRESENT_DESC] = "画面送到屏幕的方式。Mailbox：流畅、延迟低。垂直同步：无撕裂，延迟稍高。立即：延迟最低，画面可能撕裂。",
    [UI_MENU_PRESENT_MAILBOX] = "Mailbox", [UI_MENU_PRESENT_VSYNC] = "垂直同步", [UI_MENU_PRESENT_IMMEDIATE] = "立即",
    [UI_MENU_PRESENT_ADAPTIVE] = "自适应垂直同步",
    [UI_MENU_PRESENT_IN_USE] = "实际使用：%s",
    [UI_MENU_GPU] = "显卡", [UI_MENU_GPU_DESC] = "用哪块显卡运行游戏。适用于有两块显卡的电脑。",
    [UI_MENU_GPU_AUTO] = "自动",
    [UI_MENU_PERF_OVERLAY] = "性能信息",
    [UI_MENU_PERF_OVERLAY_DESC] = "在角落显示帧率。完整模式还会显示 CPU、GPU、内存和温度。按 F10 切换模式。",
    [UI_MENU_PERF_OFF] = "关", [UI_MENU_PERF_FPS] = "帧率", [UI_MENU_PERF_FULL] = "完整",
    [UI_PERF_LINE_FPS] = "%.0f fps",
    [UI_MENU_GRAPHICS] = "画质",
    [UI_MENU_OUTPUT_RES] = "游戏分辨率",
    [UI_MENU_OUTPUT_RES_DESC] = "游戏画面和界面的尺寸，默认 1920 x 1080。更高：界面更清晰，GPU 负担更重。1280 x 720 适合 Steam Deck。",
    [UI_MENU_RENDER_SCALE] = "3D 渲染比例",
    [UI_MENU_RENDER_SCALE_DESC] = "3D 世界的清晰度，以游戏分辨率的百分比表示。越高越清晰，但帧率越低。100 % 为原尺寸，140 % 约需两倍 GPU 性能，200 % 约需四倍。",
    [UI_MENU_RENDER_SCALE_GAME] = "游戏默认（75 %）",
    [UI_MENU_DYNAMIC_RES] = "动态分辨率",
    [UI_MENU_DYNAMIC_RES_DESC] = "游戏默认：大规模战斗时 3D 画面会短暂变模糊，以保持帧率。关：始终清晰，但 GPU 负担更重。",
    [UI_MENU_DYNAMIC_RES_GAME] = "游戏默认", [UI_MENU_DYNAMIC_RES_OFF] = "关（原尺寸）",
    [UI_MENU_UPSCALER] = "缩放到窗口",
    [UI_MENU_UPSCALER_DESC] = "窗口比游戏分辨率大时，画面的放大方式。FSR 比双线性更清晰。",
    [UI_MENU_UPSCALER_FSR] = "AMD FSR 1", [UI_MENU_UPSCALER_BILINEAR] = "双线性",
    [UI_MENU_ASPECT] = "画面比例",
    [UI_MENU_ASPECT_DESC] = "用于非 16:9 的屏幕。黑边：保持正确比例。拉伸：铺满屏幕，但画面会变形。",
    [UI_MENU_ASPECT_FIT] = "黑边", [UI_MENU_ASPECT_STRETCH] = "拉伸",
    [UI_MENU_ANISO] = "纹理过滤",
    [UI_MENU_ANISO_DESC] = "让远处的地面和墙壁保持清晰。游戏默认：与 Switch 相同的级别。",
    [UI_MENU_ANISO_GAME] = "游戏默认",
    [UI_MENU_SHARPNESS] = "FSR 锐度",
    [UI_MENU_SHARPNESS_DESC] = "FSR 放大后再锐化画面。0 为关闭。",
    [UI_MENU_COLOR] = "色彩",
    [UI_MENU_COLOR_PRESET] = "色彩方案",
    [UI_MENU_COLOR_PRESET_DESC] = "预设的色彩设置。鲜艳：色彩和对比度更强，适合 OLED 屏幕。清晰：稍亮、更通透。修改下面任意一项会切换为自定义。",
    [UI_MENU_COLOR_DEFAULT] = "游戏原色", [UI_MENU_COLOR_VIVID] = "鲜艳（OLED）", [UI_MENU_COLOR_CLARITY] = "清晰",
    [UI_MENU_COLOR_CUSTOM] = "自定义",
    [UI_MENU_COLOR_BRIGHTNESS] = "亮度", [UI_MENU_COLOR_BRIGHTNESS_DESC] = "让整个画面变亮或变暗。0 为不变。",
    [UI_MENU_COLOR_CONTRAST] = "对比度",
    [UI_MENU_COLOR_CONTRAST_DESC] = "亮部与暗部的差别。100 % 为不变。",
    [UI_MENU_COLOR_SATURATION] = "饱和度",
    [UI_MENU_COLOR_SATURATION_DESC] = "色彩的浓度。100 % 为不变。",
    [UI_MENU_COLOR_GAMMA] = "伽马",
    [UI_MENU_COLOR_GAMMA_DESC] = "小于 1：阴影和中间调更亮。大于 1：更暗。1 为不变。",
    [UI_MENU_HDR_GROUP] = "HDR",
    [UI_MENU_HDR] = "HDR 输出",
    [UI_MENU_HDR_DESC] = "在 HDR 屏幕上让高光更亮。Windows 或 Wayland 合成器里也必须开启 HDR。重启后生效。",
    [UI_MENU_HDR_PEAK] = "峰值亮度",
    [UI_MENU_HDR_PEAK_DESC] = "设为你屏幕的最大亮度，单位为尼特。",
    [UI_MENU_HDR_GAME] = "游戏亮度",
    [UI_MENU_HDR_GAME_DESC] = "游戏中白色的亮度，单位为尼特。标准值为 203。房间较亮时可调高。",
    [UI_MENU_HDR_UI] = "HUD 与菜单亮度",
    [UI_MENU_HDR_UI_DESC] = "HUD 和本菜单中白色的亮度，单位为尼特。",
    [UI_MENU_SHADOW_SCALE] = "阴影分辨率",
    [UI_MENU_SHADOW_SCALE_DESC] = "数值越高阴影边缘越清晰，GPU 占用也越多。重启后生效。",
    [UI_MENU_PCSS] = "柔和阴影",
    [UI_MENU_PCSS_DESC] = "阴影在物体附近保持清晰，离得越远越柔和。0 保留游戏原本的阴影。重启后生效。",
    [UI_MENU_DLAA] = "DLSS / FSR 3.1",
    [UI_MENU_DLAA_DESC] = "让 3D 画面边缘更平滑、减少闪烁。RTX 显卡使用 NVIDIA DLSS，其他显卡使用 AMD FSR 3.1。重启后生效。",
    [UI_MENU_DLAA_ENGINE] = "超分技术",
    [UI_MENU_DLAA_ENGINE_DESC] = "自动：显卡支持就用 DLSS，否则用 FSR 3.1。FSR 3.1 适用于所有显卡。重启后生效。",
    [UI_MENU_DLAA_ENGINE_AUTO] = "自动", [UI_MENU_DLAA_ENGINE_DLSS] = "NVIDIA DLSS",
    [UI_MENU_DLAA_ENGINE_FSR3] = "AMD FSR 3.1",
    [UI_MENU_FSR3_SHARPNESS] = "FSR 3.1 锐度",
    [UI_MENU_FSR3_SHARPNESS_DESC] = "锐化 3D 画面，仅限 FSR 3.1。0 为关闭。",
    [UI_MENU_DLSS] = "模式",
    [UI_MENU_DLSS_DESC] = "原生：画质最好。质量、平衡、性能分别以 67、58、50 % 渲染再放大，帧率更高。重启后生效。",
    [UI_MENU_DLSS_DLAA] = "原生（DLAA）", [UI_MENU_DLSS_QUALITY] = "质量", [UI_MENU_DLSS_BALANCED] = "平衡",
    [UI_MENU_DLSS_PERFORMANCE] = "性能",
    [UI_MENU_DLSS_PRESET] = "DLSS 模型",
    [UI_MENU_DLSS_PRESET_DESC] = "使用的 DLSS 版本。不确定就保持自动。K：最清晰。J：移动角色身后的拖影更少，闪烁稍多。",
    [UI_MENU_DLSS_PRESET_DEFAULT] = "自动", [UI_MENU_DLSS_PRESET_K] = "K（最清晰）",
    [UI_MENU_DLSS_PRESET_J] = "J（拖影更少）",
    [UI_MENU_FXAA] = "抗锯齿（FXAA）",
    [UI_MENU_FXAA_DESC] = "平滑锯齿边缘，细节会稍微变软。开启 DLSS 或 FSR 3.1 时不使用。",

    [UI_MENU_FPS_LIMIT] = "帧率",
    [UI_MENU_FPS_LIMIT_DESC] = "每秒画面数。30 是原版游戏，60 更流畅，CPU 占用约为两倍。120 及以上为实验性功能，需要高刷新率显示器。",
    [UI_MENU_FPS_30] = "30 FPS", [UI_MENU_FPS_60] = "60 FPS", [UI_MENU_FPS_120] = "120 FPS（实验性）",
    [UI_MENU_FPS_144] = "144 FPS（实验性）", [UI_MENU_FPS_240] = "240 FPS（实验性）",
    [UI_MENU_GAME_LANG] = "游戏语言",
    [UI_MENU_GAME_LANG_DESC] = "游戏文字和语音的语言。自动：跟随系统。",
    [UI_MENU_GAME_LANG_AUTO] = "自动",
    [UI_MENU_OPERATION_MODE] = "主机模式",
    [UI_MENU_OPERATION_MODE_DESC] = "让游戏以为 Switch 插在底座上或处于掌机模式。",
    [UI_MENU_MODE_DOCKED] = "底座", [UI_MENU_MODE_HANDHELD] = "掌机",
    [UI_MENU_FRAME_QUEUE] = "帧队列",
    [UI_MENU_FRAME_QUEUE_DESC] = "提前准备下一帧画面：卡顿更少，输入延迟最多增加一帧。",
    [UI_MENU_ASSUME_FOCUS] = "失去焦点时继续运行",
    [UI_MENU_ASSUME_FOCUS_DESC] = "开：切到其他窗口时游戏继续运行。关：游戏暂停，但这样打断读取画面可能会卡住。",

    [UI_MENU_MODS_INSTALLED] = "已安装的模组",
    [UI_MENU_MODS_NONE] = "还没有模组。把模组解压到 mods 文件夹，每个模组一个文件夹，里面放它的 romfs 文件夹，然后重启游戏。TKMM 也可以把合并后的模组导出到那里的一个文件夹。",
    [UI_MENU_MODS_OPEN] = "打开模组文件夹",
    [UI_MENU_MODS_OPEN_DESC] = "打开存放模组的文件夹。",
    [UI_MENU_MODS_ROW_DESC] = "替换的游戏文件：%u 个。新增的文件：%u 个。",
    [UI_MENU_MODS_OVERRIDDEN] = "其中 %u 个文件来自列表中更靠前的 %s。",
    [UI_MENU_MODS_MOVE_HINT] = "按上方的按键或 Ctrl+上 上移，按左侧的按键或 Ctrl+下 下移。两个模组改同一个文件时，靠前的生效。重启后生效。",
    [UI_MENU_MODS_EMPTY] = "这个文件夹里没有 romfs、exefs 或 cheats 文件。",
    [UI_MENU_MODS_SUBSDK] = "这个模组自带游戏代码（subsdk 插件或替换的 main）。本移植版无法加载代码模块，所以整个模组保持关闭。",
    [UI_MENU_MODS_EXEFS_LATER] = "这个模组会修改游戏代码（exefs）。游戏下次启动时会把补丁编译进去（需要重新构建几分钟）；在那之前整个模组保持关闭。",
    [UI_MENU_MODS_CHEATS] = "金手指：%d 个。在“金手指”页开关。",
    [UI_MENU_MODS_CHEATS_FILE] = "本游戏版本的金手指在启动时从这个模组的 cheats\\%s 读取。",
    [UI_MENU_MODS_BAD_NAME] = "文件夹名里含有 |，模组列表用它分隔名称。请重命名这个文件夹。",
    [UI_MENU_MODS_OTHER_VERSION] = "其中 %u 个文件是给其他游戏版本做的，已被忽略。请找适用于你的游戏版本的模组。",
    [UI_MENU_MODS_UNUSED] = "未使用",

    [UI_MENU_VOLUME] = "音量", [UI_MENU_VOLUME_DESC] = "游戏的总音量。",
    [UI_MENU_MUTE] = "静音", [UI_MENU_MUTE_DESC] = "让游戏静音，不改变音量设置。",
    [UI_MENU_MUTE_BG] = "后台时静音",
    [UI_MENU_MUTE_BG_DESC] = "切到其他窗口时让游戏静音。",
    [UI_MENU_AUDIO_DEVICE] = "输出设备",
    [UI_MENU_AUDIO_DEVICE_DESC] = "声音从哪里播放。系统默认：跟随系统设置。",
    [UI_MENU_AUDIO_DEVICE_DEFAULT] = "系统默认",
    [UI_MENU_AUDIO_SURROUND] = "5.1 环绕声",
    [UI_MENU_AUDIO_SURROUND_DESC] = "把游戏的六个声道输出到 5.1 音响系统。使用立体声音箱或耳机时请保持关闭。",
    [UI_MENU_INVERT_LX] = "反转左摇杆 X 轴", [UI_MENU_INVERT_LY] = "反转左摇杆 Y 轴",
    [UI_MENU_INVERT_RX] = "反转右摇杆 X 轴（镜头）", [UI_MENU_INVERT_RY] = "反转右摇杆 Y 轴（镜头）",
    [UI_MENU_INVERT_DESC] = "反转这个摇杆方向。",

    [UI_MENU_AIM_LEFT_STICK] = "用左摇杆瞄准",
    [UI_MENU_AIM_LEFT_STICK_DESC] = "开：左摇杆也能瞄准弓箭，和原版游戏一样。关：只有右摇杆能瞄准。",
    [UI_MENU_MOTION] = "体感操作", [UI_MENU_MOTION_DESC] = "倾斜手柄来瞄准（陀螺仪）。",
    [UI_MENU_GYRO_CALIBRATION] = "陀螺仪校准",
    [UI_MENU_GYRO_CALIBRATION_DESC] = "修正陀螺仪漂移。关：使用原始陀螺仪数据。",
    [UI_MENU_CALIBRATE_GYRO] = "校准陀螺仪",
    [UI_MENU_CALIBRATE_GYRO_DESC] = "把手柄平放，按下后 2 秒内不要碰它。可防止瞄准向一侧漂移。",
    [UI_MENU_CALIB_NONE] = "未校准", [UI_MENU_CALIB_COLLECTING] = "请保持不动……",
    [UI_MENU_CALIB_DONE] = "已校准", [UI_MENU_CALIB_FAILED] = "没有陀螺仪数据",
    [UI_MENU_CALIB_MOVED] = "手柄动了，请重试",
    [UI_MENU_RUMBLE_STRENGTH] = "震动强度",
    [UI_MENU_RUMBLE_STRENGTH_DESC] = "手柄震动的强弱。",
    [UI_MENU_RUMBLE_CURVE] = "震动曲线",
    [UI_MENU_RUMBLE_CURVE_DESC] = "数值越低，轻微的震动越容易感觉到。",
    [UI_MENU_RUMBLE_TRIGGERS] = "扳机震动（Xbox）",
    [UI_MENU_RUMBLE_TRIGGERS_DESC] = "让 Xbox 手柄的扳机也震动。",
    [UI_MENU_PAD_LAYOUT] = "按键布局",
    [UI_MENU_PAD_LAYOUT_DESC] = "哪个实体按键作为 A。任天堂：右边那个。Xbox 和 PlayStation：下面那个。自动：根据你的手柄选择。",
    [UI_MENU_PAD_LAYOUT_AUTO] = "自动", [UI_MENU_PAD_LAYOUT_NINTENDO] = "任天堂（A 在右）",
    [UI_MENU_PAD_LAYOUT_XBOX] = "Xbox（A 在下）",
    [UI_MENU_PAD_LAYOUT_PLAYSTATION] = "PlayStation（叉键为 A）",
    [UI_MENU_PAD_LAYOUT_IN_USE] = "实际使用：%s",
    [UI_MENU_PROMPTS] = "按键提示",
    [UI_MENU_PROMPTS_DESC] = "游戏里显示的按键图标。重新映射过的按键仍显示原来的图标。",
    [UI_MENU_PROMPTS_NINTENDO] = "任天堂", [UI_MENU_PROMPTS_XBOX] = "Xbox",
    [UI_MENU_PROMPTS_PLAYSTATION] = "PlayStation",
    [UI_MENU_PROMPTS_MISSING] = "缺少按键提示文件：请重新运行安装程序",
    [UI_MENU_PROMPTS_FAILED] = "按键提示文件已损坏：请重新运行安装程序",
    [UI_MENU_PROMPTS_RESTART] = "按键提示在重启后更换",
    [UI_MENU_RESTART] = "立即重启",
    [UI_MENU_RESTART_DESC] = "重启游戏以应用新图标。未保存的进度会丢失。",
    [UI_MENU_RESTART_CONFIRM] = "现在重启游戏吗？上次存档之后的进度会丢失。",
    [UI_MENU_RESTART_YES] = "重启",
    [UI_MENU_RESTART_SAVING] = "游戏正在存档：稍后才能重启。",
    [UI_MENU_LSTICK_DEADZONE] = "左摇杆死区", [UI_MENU_RSTICK_DEADZONE] = "右摇杆死区",
    [UI_MENU_STICK_DEADZONE_DESC] = "摇杆要推多远才算输入。如果角色或镜头自己在动，把它设得比“当前摇杆”数值稍高一点。",
    [UI_MENU_STICK_DEADZONE_NOW] = "当前摇杆：%.0f %%",
    [UI_MENU_MOUSE_SENS] = "鼠标镜头灵敏度",
    [UI_MENU_MOUSE_SENS_DESC] = "用鼠标转动镜头的速度。",
    [UI_MENU_WALK_KEY] = "步行键",
    [UI_MENU_WALK_KEY_DESC] = "用键盘时按住这个键会步行而不是奔跑。",
    [UI_MENU_WALK_KEY_TAKEN] = "也是 %s",
    [UI_MENU_WALK_SCALE] = "步行速度",
    [UI_MENU_WALK_SCALE_DESC] = "按住步行键时的速度。一半为步行，满值为奔跑。",

    [UI_BINDS_HELP] = "选中一格，按 A 或回车，然后按下要用的键盘键、鼠标键或手柄按键。按 Delete 或 X 清除一格。已被占用的键会移到这里；手柄按键会和被替换的那个互换。",
    [UI_BINDS_COL_KEY1] = "按键 1", [UI_BINDS_COL_KEY2] = "按键 2", [UI_BINDS_COL_PAD] = "手柄",
    [UI_BINDS_GROUP_BUTTONS] = "按键", [UI_BINDS_GROUP_MOVE] = "移动", [UI_BINDS_GROUP_CAMERA] = "镜头",
    [UI_BINDS_PRESS_KEY] = "请为 %s 按下键盘键或鼠标键",
    [UI_BINDS_PRESS_PAD] = "请为 %s 按下手柄按键",
    [UI_BINDS_CANCEL_HINT] = "按 Esc 取消，或等待 %d 秒",
    [UI_BINDS_TAKEN] = "%s 已从 %s 移过来", [UI_BINDS_SWAPPED] = "%s 已与 %s 互换",
    [UI_BINDS_RESET] = "重置所有按键",
    [UI_BINDS_RESET_DESC] = "恢复默认的键盘键和手柄按键。上面的按键布局会重新生效：在这次重置之前，重新映射过的手柄按键会保持你设置的位置。",
    [UI_BIND_A] = "A", [UI_BIND_B] = "B", [UI_BIND_X] = "X", [UI_BIND_Y] = "Y",
    [UI_BIND_LCLICK] = "按下左摇杆", [UI_BIND_RCLICK] = "按下右摇杆", [UI_BIND_L] = "L",
    [UI_BIND_R] = "R", [UI_BIND_ZL] = "ZL", [UI_BIND_ZR] = "ZR", [UI_BIND_PLUS] = "+", [UI_BIND_MINUS] = "-",
    [UI_BIND_LEFT] = "十字键左", [UI_BIND_UP] = "十字键上", [UI_BIND_RIGHT] = "十字键右",
    [UI_BIND_DOWN] = "十字键下",
    [UI_BIND_LSTICK_LEFT] = "向左移动", [UI_BIND_LSTICK_UP] = "向前移动", [UI_BIND_LSTICK_RIGHT] = "向右移动",
    [UI_BIND_LSTICK_DOWN] = "向后移动",
    [UI_BIND_RSTICK_LEFT] = "镜头向左", [UI_BIND_RSTICK_UP] = "镜头向上", [UI_BIND_RSTICK_RIGHT] = "镜头向右",
    [UI_BIND_RSTICK_DOWN] = "镜头向下",
    [UI_BIND_DESC_A] = "互动、拾取、对话。", [UI_BIND_DESC_B] = "奔跑、取消、返回。",
    [UI_BIND_DESC_X] = "跳跃。", [UI_BIND_DESC_Y] = "攻击。", [UI_BIND_DESC_LCLICK] = "蹲下。",
    [UI_BIND_DESC_RCLICK] = "第一人称视角。", [UI_BIND_DESC_L] = "能力。",
    [UI_BIND_DESC_R] = "投掷武器。", [UI_BIND_DESC_ZL] = "举盾、锁定。", [UI_BIND_DESC_ZR] = "弓。",
    [UI_BIND_DESC_PLUS] = "菜单、背包。", [UI_BIND_DESC_MINUS] = "地图。",
    [UI_BIND_DESC_DPAD] = "快捷选择。",
    [UI_BIND_DESC_MOVE] = "像左摇杆一样移动角色。手柄的左摇杆也始终可用。",
    [UI_BIND_DESC_CAMERA] = "像右摇杆一样转动镜头。鼠标和手柄的右摇杆也始终可用。",
    [UI_KEY_SPACE] = "空格", [UI_KEY_TAB] = "Tab", [UI_KEY_LSHIFT] = "左 Shift", [UI_KEY_LCTRL] = "左 Ctrl",
    [UI_KEY_UP] = "方向键上", [UI_KEY_DOWN] = "方向键下", [UI_KEY_LEFT] = "方向键左",
    [UI_KEY_RIGHT] = "方向键右",
    [UI_KEY_MOUSE_L] = "鼠标左键", [UI_KEY_MOUSE_R] = "鼠标右键", [UI_KEY_MOUSE_M] = "鼠标中键",
    [UI_KEY_MOUSE_X1] = "鼠标侧键 4", [UI_KEY_MOUSE_X2] = "鼠标侧键 5",
    [UI_KEY_ENTER] = "回车", [UI_KEY_BACKSPACE] = "退格", [UI_KEY_RSHIFT] = "右 Shift",
    [UI_KEY_RCTRL] = "右 Ctrl", [UI_KEY_LALT] = "Alt", [UI_KEY_CAPSLOCK] = "大写锁定", [UI_KEY_INSERT] = "Insert",
    [UI_KEY_DELETE] = "Delete", [UI_KEY_HOME] = "Home", [UI_KEY_END] = "End", [UI_KEY_PAGEUP] = "Page Up",
    [UI_KEY_PAGEDOWN] = "Page Down", [UI_KEY_NUM_FMT] = "小键盘 %s",
    [UI_PAD_SOUTH] = "下按键", [UI_PAD_EAST] = "右按键", [UI_PAD_WEST] = "左按键",
    [UI_PAD_NORTH] = "上按键", [UI_PAD_LSTICK] = "左摇杆", [UI_PAD_RSTICK] = "右摇杆",
    [UI_PAD_L] = "左肩键", [UI_PAD_R] = "右肩键", [UI_PAD_START] = "Start", [UI_PAD_BACK] = "Back",
    [UI_PAD_DPAD_LEFT] = "十字键左", [UI_PAD_DPAD_UP] = "十字键上", [UI_PAD_DPAD_RIGHT] = "十字键右",
    [UI_PAD_DPAD_DOWN] = "十字键下", [UI_PAD_ZL] = "左扳机", [UI_PAD_ZR] = "右扳机",
    [UI_PAD_LPADDLE1] = "左背键", [UI_PAD_RPADDLE1] = "右背键",
    [UI_PAD_LPADDLE2] = "左背键 2", [UI_PAD_RPADDLE2] = "右背键 2",
    [UI_PAD_CROSS] = "叉", [UI_PAD_CIRCLE] = "圆", [UI_PAD_SQUARE] = "方", [UI_PAD_TRIANGLE] = "三角",

    [UI_MENU_AMIIBO_DIR] = "Amiibo 文件夹",
    [UI_MENU_AMIIBO_OPEN] = "打开",
    [UI_MENU_AMIIBO_DEFAULT_DIR] = "默认文件夹",
    [UI_MENU_AMIIBO_REFRESH] = "刷新",
    [UI_MENU_AMIIBO_PICK] = "Amiibo 数据（540 字节的 .bin 或 .nfc 文件）：",
    [UI_MENU_AMIIBO_NO_DIR] = "无法打开文件夹 %s",
    [UI_MENU_AMIIBO_READER] = "NFC 读取器",
    [UI_MENU_AMIIBO_STATE] = "读取器：",
    [UI_MENU_AMIIBO_STATE_IDLE] = "就绪",
    [UI_MENU_AMIIBO_STATE_SEARCHING] = "游戏正在等待 amiibo",
    [UI_MENU_AMIIBO_STATE_FOUND] = "amiibo 已放在读取器上",
    [UI_MENU_AMIIBO_STATE_MOUNTED] = "游戏正在读取 amiibo",
    [UI_MENU_AMIIBO_STATE_OFF] = "关闭",
    [UI_MENU_AMIIBO_FILE] = "选中的文件：",
    [UI_MENU_AMIIBO_NO_FILE] = "（未选择文件）",
    [UI_MENU_AMIIBO_TAP] = "感应 amiibo",
    [UI_MENU_AMIIBO_REMOVE] = "拿开 amiibo",
    [UI_MENU_AMIIBO_AUTO_REMOVE] = "感应后自动拿开 amiibo",
    [UI_MENU_AMIIBO_HOLD] = "在读取器上的时间",
    [UI_MENU_AMIIBO_SECONDS] = "%.1f 秒",
    [UI_MENU_AMIIBO_TAG] = "读取器上的 amiibo：",
    [UI_MENU_AMIIBO_NO_TAG] = "读取器上没有 amiibo。",
    [UI_MENU_AMIIBO_TAG_FILE] = "文件：%s",
    [UI_MENU_AMIIBO_TAG_ID] = "角色 %s，系列 %02x，编号 %04x，类型 %02x",
    [UI_MENU_AMIIBO_TAG_UID] = "UID：%s",
    [UI_MENU_AMIIBO_READY] = "就绪",
    [UI_MENU_AMIIBO_TAPPED] = "已感应：%s",
    [UI_MENU_AMIIBO_LOAD_FAILED] = "不是 amiibo 数据（540 字节的 NTAG215）：%s",
    [UI_MENU_AMIIBO_REMOVED] = "amiibo 已拿开",
    [UI_MENU_AMIIBO_STATUS] = "状态：%s",
    [UI_MENU_MENU_LANG] = "菜单语言",
    [UI_MENU_MENU_LANG_DESC] = "本菜单的语言。自动：系统语言有翻译时跟随系统，否则用英文。",
    [UI_MENU_MENU_LANG_AUTO] = "自动（跟随系统）",

    [UI_MENU_ADV_WARNING] = "这些设置用于测试和排查问题。只有在清楚原因时才修改。",
    [UI_MENU_GROUP_SCHED] = "调度", [UI_MENU_GROUP_SHADERS] = "着色器与管线",
    [UI_MENU_GROUP_TEXMEM] = "纹理与内存", [UI_MENU_GROUP_FIXES] = "渲染修正",
    [UI_MENU_GROUP_VULKAN] = "Vulkan", [UI_MENU_GROUP_DIAG] = "诊断",
    [UI_MENU_ASYNC_PREEMPT] = "虚拟核心抢占",
    [UI_MENU_ASYNC_PREEMPT_DESC] = "允许一个游戏线程打断另一个。除非排查问题，请保持默认。",
    [UI_MENU_PREEMPT_OFF] = "关", [UI_MENU_PREEMPT_ON] = "开", [UI_MENU_PREEMPT_INHERIT] = "仅优先级继承",
    [UI_MENU_REPLAY_ASYNC] = "GPU 帧准备使用独立线程",
    [UI_MENU_REPLAY_ASYNC_DESC] = "在单独的线程上准备 GPU 工作。有助于在大规模战斗中保持 60 FPS，最多增加一帧延迟。",
    [UI_MENU_TIMER_1MS] = "1 毫秒 Windows 计时器",
    [UI_MENU_TIMER_1MS_DESC] = "使用更精确的 Windows 计时器，让帧间隔更稳定。",
    [UI_MENU_PIPE_WAIT] = "等待管线时的绘制处理",
    [UI_MENU_PIPE_WAIT_DESC] = "绘制如何等待正在构建的管线。请保持默认。旧版为之前的行为。",
    [UI_MENU_PIPE_WAIT_DEFAULT] = "默认", [UI_MENU_PIPE_WAIT_DRAW] = "旧版（绘制）",
    [UI_MENU_PIPE_SKIP] = "管线构建时跳过的绘制",
    [UI_MENU_PIPE_SKIP_DESC] = "管线构建期间，哪些物体会短暂不绘制。",
    [UI_MENU_PIPE_SKIP_STRICT] = "严格", [UI_MENU_PIPE_SKIP_ANY] = "任意",
    [UI_MENU_PIPE_SKIP_FULLSCREEN] = "全屏处理",
    [UI_MENU_LIB_PREBUILD] = "着色器阶段库预构建",
    [UI_MENU_LIB_PREBUILD_DESC] = "在后台准备着色器，减少卡顿。",
    [UI_MENU_LIB_PREBUILD_THREADS] = "阶段库预构建线程数",
    [UI_MENU_LIB_PREBUILD_THREADS_DESC] = "用于这项准备的 CPU 线程数。自动：根据你的 CPU 调整。",
    [UI_MENU_PIPE_PREBUILD] = "管线预构建",
    [UI_MENU_PIPE_PREBUILD_DESC] = "提前准备已知的管线，减少卡顿。",
    [UI_MENU_PREWARM_XLATE] = "着色器翻译预热",
    [UI_MENU_PREWARM_XLATE_DESC] = "启动时准备已知着色器所用的 CPU 线程数。自动：使用一半线程，最多 16 个。",
    [UI_MENU_GPL] = "图形管线库",
    [UI_MENU_GPL_DESC] = "借助一项 Vulkan 功能更快地构建管线。如果你的驱动有问题，请关掉它。",
    [UI_MENU_ASTC] = "ASTC 纹理解码", [UI_MENU_ASTC_DESC] = "游戏压缩纹理在哪里解码。请保持自动。",
    [UI_MENU_ASTC_AUTO] = "自动", [UI_MENU_ASTC_GPU] = "GPU 着色器", [UI_MENU_ASTC_NATIVE] = "原生",
    [UI_MENU_ASTC_CPU] = "CPU",
    [UI_MENU_MEMORY] = "GPU 内存模式", [UI_MENU_MEMORY_DESC] = "游戏内存如何与 GPU 共享。请保持自动。",
    [UI_MENU_MEMORY_AUTO] = "自动", [UI_MENU_MEMORY_IMPORT] = "导入", [UI_MENU_MEMORY_MIRROR] = "镜像",
    [UI_MENU_MEMORY_VERIFY] = "校验",
    [UI_MENU_TEX_ALIAS] = "纹理别名", [UI_MENU_TEX_ALIAS_DESC] = "在内存中覆盖旧纹理的新纹理会带着旧内容开始。用于画面错误。",
    [UI_MENU_TEX_VRAM_PRESSURE] = "纹理内存上限",
    [UI_MENU_TEX_VRAM_PRESSURE_DESC] = "显存占用超过这个百分比后释放不用的纹理。0 为从不释放。",
    [UI_MENU_ASTC_BC] = "把 ASTC 压缩为 BC3",
    [UI_MENU_ASTC_BC_DESC] = "显存占用减少到四分之一，纹理会稍有色块。适用于 4 GB 及以下显存的显卡。",
    [UI_MENU_FIX_C0] = "常量缓冲区修正",
    [UI_MENU_FIX_C0_DESC] = "为着色器提供主机驱动通常会给的数据，而不是全零。用于画面错误。",
    [UI_MENU_FIX_ZERO] = "新 GPU 内存清零",
    [UI_MENU_FIX_ZERO_DESC] = "新分配的 GPU 内存从空白开始，和 Switch 上一样。",
    [UI_MENU_BIND_SKIP] = "跳过重复绑定",
    [UI_MENU_BIND_SKIP_DESC] = "速度优化：跳过没有变化的绑定。",
    [UI_MENU_SHADER_MEMO] = "着色器缓存记忆", [UI_MENU_SHADER_MEMO_DESC] = "速度优化：记住着色器查找结果。",
    [UI_MENU_PIPE_MEMO] = "管线记忆", [UI_MENU_PIPE_MEMO_DESC] = "速度优化：没有变化时复用上一个管线。",
    [UI_MENU_TEX_MEMO] = "纹理绑定记忆", [UI_MENU_TEX_MEMO_DESC] = "速度优化：在绘制之间记住纹理。",
    [UI_MENU_DESC_TEMPLATE] = "描述符模板",
    [UI_MENU_DESC_TEMPLATE_DESC] = "速度优化：批量更新描述符。",
    [UI_MENU_PUSH_SKIP] = "跳过重复的推送常量",
    [UI_MENU_PUSH_SKIP_DESC] = "速度优化：跳过没有变化的推送常量。",
    [UI_MENU_SPLIT_DRAWS] = "提前提交 GPU 工作",
    [UI_MENU_SPLIT_DRAWS_DESC] = "把 GPU 工作分批提交，让它更早开始。数值是每批的绘制次数。0 为关闭。",
    [UI_MENU_VK_VALIDATION] = "验证层",
    [UI_MENU_VK_VALIDATION_DESC] = "仅用于调试：检查每个 Vulkan 调用，会让游戏变得很慢。",
    [UI_MENU_VK_VALIDATION_MISSING] = "未安装验证层。",
    [UI_MENU_DLAA_MISSING] = "DLSS 不可用：需要 NVIDIA RTX 显卡和 nvngx_dlss.dll（Windows 版构建时会下载）。请重新构建，或把这个文件放到 AgeOfCalamityRecomp.exe 旁边，然后重启。也可以把超分技术设为自动或 AMD FSR 3.1。",
    [UI_MENU_VK_DYN_VERTEX] = "动态顶点状态",
    [UI_MENU_VK_DYN_VERTEX_DESC] = "强制开启或关闭这项 Vulkan 功能。默认值取决于你的 GPU。",
    [UI_MENU_VK_DYN_A2C] = "动态 Alpha to Coverage",
    [UI_MENU_VK_DYN_A2C_DESC] = "强制开启或关闭这项 Vulkan 功能。默认：在 AMD RDNA 2 及更早的显卡上关闭，否则角色会变黑。",
    [UI_MENU_VK_SUBGROUP_PIN] = "固定子组大小", [UI_MENU_VK_SUBGROUP_PIN_DESC] = "强制使用固定的着色器子组大小。用于排查问题。",
    [UI_MENU_TRACE] = "性能日志", [UI_MENU_TRACE_DESC] = "把帧耗时写入文件。",
    [UI_MENU_FLIGHT] = "飞行记录仪",
    [UI_MENU_FLIGHT_DESC] = "画面卡住时保存最近的运行情况，方便报告问题。",
    [UI_MENU_FLIGHT_SPIKE_MS] = "卡顿阈值（毫秒）",
    [UI_MENU_FLIGHT_SPIKE_MS_DESC] = "单帧耗时超过多少毫秒算作卡住。",
    [UI_MENU_INPUT_RECORD] = "输入录制",
    [UI_MENU_INPUT_RECORD_DESC] = "把你的操作写入文件，用于报告问题。",
    [UI_MENU_OPEN_LOG] = "打开日志文件夹", [UI_MENU_OPEN_LOG_DESC] = "打开存放日志文件的文件夹。",
    [UI_MENU_COPY_INFO] = "复制系统信息",
    [UI_MENU_COPY_INFO_DESC] = "复制你的电脑信息和改过的设置，可粘贴到问题报告里。",
    [UI_MENU_COPY_INFO_DONE] = "已复制", [UI_MENU_ACTION_FAILED] = "操作失败",
    [UI_MENU_RESET] = "所有设置恢复默认", [UI_MENU_RESET_DESC] = "把所有页面的每一项设置恢复为默认值。",
    [UI_MENU_RESET_CONFIRM] = "把所有设置恢复为默认值吗？", [UI_MENU_RESET_YES] = "恢复",
    [UI_MENU_RESET_NO] = "取消",

    [UI_MENU_OV_FPS] = "%.0f FPS", [UI_MENU_OV_FRAME_MS] = "%.1f ms", [UI_MENU_OV_GAME_PCT] = "游戏 %.0f%%",
    [UI_MENU_OV_MAX_MS] = "最大 %.1f ms",
    [UI_MENU_OV_GPU] = "GPU", [UI_MENU_OV_CPU] = "CPU", [UI_MENU_OV_RAM] = "内存", [UI_MENU_OV_VRAM] = "显存",
    [UI_MENU_OV_NONE] = "-",
    [UI_MENU_OV_REPLAY] = "GPU 帧准备", [UI_MENU_OV_GPU_TIME] = "每帧 GPU 占用",
    [UI_MENU_OV_PIPE_WAIT] = "管线等待",
    [UI_MENU_OV_SHADER_WAIT] = "着色器等待", [UI_MENU_OV_TEX_UPLOADS] = "纹理上传",
    [UI_MENU_OV_FMT_PCT] = "%.0f%%", [UI_MENU_OV_FMT_TEMP] = "%.0f °C", [UI_MENU_OV_FMT_POWER] = "%.0f W",
    [UI_MENU_OV_FMT_MHZ] = "%.0f MHz", [UI_MENU_OV_FMT_GB] = "%.1f GB", [UI_MENU_OV_FMT_GB_OF] = "/ %.1f GB",
    [UI_MENU_OV_GAME_GB] = "游戏 %.1f GB", [UI_MENU_OV_FMT_MS] = "%.2f ms", [UI_MENU_OV_PER_FRAME] = "每帧 %.1f",
    [UI_MENU_OV_WINDOW] = "窗口", [UI_MENU_OV_OUTPUT] = "游戏画面", [UI_MENU_OV_SCENE] = "3D 场景",
    [UI_MENU_OV_SCALING] = "缩放", [UI_MENU_OV_LIMIT] = "帧率上限", [UI_MENU_OV_PRESENT] = "呈现方式",
    [UI_MENU_OV_FMT_SIZE] = "%ux%u", [UI_MENU_OV_FMT_SCENE] = "%ux%u，%.0f%%",
    [UI_MENU_OV_FMT_FSR] = "AMD FSR 1，锐度 %d%%", [UI_MENU_OV_SCALE_NONE] = "无（1:1）",
    [UI_MENU_OV_SCALE_BILINEAR] = "双线性",
    [UI_MENU_OV_SCALE_DOWN] = "双线性（FSR 只用于放大）",
    [UI_MENU_TAB_CHEATS] = "金手指",
    [UI_MENU_CHEATS_NONE] = "没有适用于本游戏版本的金手指。模组的金手指是放在它 cheats 文件夹里的 Atmosphere 金手指文件，以游戏的构建号命名：mods\\<模组>\\cheats\\%s。重启游戏后读取。修改游戏代码的金手指会在启动时重新构建游戏（需要几分钟）。",
    [UI_MENU_CHEATS_MOD] = "%s：%d 个金手指",
    [UI_MENU_CHEAT_CODE_DESC] = "修改游戏代码。立即切换。",
    [UI_MENU_CHEAT_KEY_DESC] = "按住 %s 时修改游戏代码。立即切换。",
    [UI_MENU_CHEAT_DATA_DESC] = "开启时每帧写入游戏内存。",
    [UI_MENU_CHEAT_MASTER] = "始终开启",
    [UI_MENU_CHEAT_MASTER_DESC] = "这是文件的主代码：始终开启。",
    [UI_MENU_CHEAT_RESTORE] = "不需要",
    [UI_MENU_CHEAT_RESTORE_DESC] = "它只是把游戏原本的代码写回去。关闭金手指本身就会这样做。",
    [UI_MENU_CHEAT_REBUILD] = "重新构建后可用",
    [UI_MENU_CHEAT_REBUILD_DESC] = "这个金手指会修改游戏代码，而且是在游戏构建之后新增或修改的。游戏下次启动时会把它编译进去（需要几分钟），也可以用“立即重新构建游戏”。",
    [UI_MENU_CHEAT_UNSUPPORTED] = "不支持",
    [UI_MENU_CHEAT_UNSUPPORTED_DESC] = "这个金手指不能用于本版本的游戏：",
    [UI_MENU_CHEATS_REBUILD] = "立即重新构建游戏",
    [UI_MENU_CHEATS_REBUILD_DESC] = "有些金手指会修改游戏代码，但还没编译进游戏：它们是新增或修改过的，或者上次重新构建失败或被取消了。这会关闭游戏；安装程序把它们编译进去（需要几分钟）后再启动游戏。上次存档之后的进度会丢失。",
    [UI_MENU_MODS_EXEFS_UNSUPPORTED] = "这个模组修改游戏代码（exefs）的方式本移植版无法应用，所以整个模组保持关闭。%s",
    [UI_MENU_MODS_EXEFS_OTHER] = "它的代码补丁（exefs）是给其他游戏版本做的。",
    [UI_MENU_MODS_EXEFS_ON] = "正在使用的代码补丁：%d 个文件。",
    [UI_MENU_MODS_REPLACES_BUILTIN] = "正在使用的代码补丁：%d 个文件。它们替换了本移植版自带的补丁 %s，开启这个模组时该补丁保持关闭。",
};

static const Row rows[] = {
    {"en", text_en}, {"ja", text_ja}, {"fr", text_fr}, {"de", text_de}, {"it", text_it}, {"es", text_es},
    {"nl", text_nl}, {"ru", text_ru}, {"zh-Hans", text_zh_hans}, {"zh-Hant", text_zh_hant},
    {"ko", text_ko},
};

const char *ui_text_lang(const char *code) {
    static const struct { const char *code, *lang; } map[] = {
        {"en-US", "en"}, {"en-GB", "en"}, {"fr-CA", "fr"}, {"es-419", "es"},
        {"zh-CN", "zh-Hans"}, {"zh-TW", "zh-Hant"},
    };
    for (size_t i = 0; i < sizeof map / sizeof *map; i++)
        if (!strcmp(code, map[i].code)) return map[i].lang;
    return code;
}

static const Row *find_row(const char *lang) {
    for (size_t i = 0; i < sizeof rows / sizeof *rows; i++)
        if (!strcmp(rows[i].lang, lang)) return &rows[i];
    return NULL;
}

int ui_text_defined(const char *lang, UiTextId id) {
    const Row *r = find_row(lang);
    return (unsigned)id < UI_TEXT_COUNT && r && r->text[id] && *r->text[id];
}

const char *ui_text_in(const char *lang, UiTextId id) {
    if ((unsigned)id >= UI_TEXT_COUNT) return "";
    return ui_text_defined(lang, id) ? find_row(lang)->text[id] : rows[0].text[id];
}

const char *ui_text(UiTextId id) { return ui_text_in(ui_text_lang(ov_language()), id); }

// A language's own strings, for the font that draws them: the merged system font must cover the whole menu of the
// language, not only its name (ui/menu.cpp fonts_up).
void ui_text_visit(const char *lang, void (*visit)(void *ctx, const char *text), void *ctx) {
    const Row *r = find_row(lang);
    if (!r) return;
    for (int id = 0; id < (int)UI_TEXT_COUNT; id++)
        if (r->text[id] && *r->text[id]) visit(ctx, r->text[id]);
}

// A language the menu is translated into: its row has the menu's own texts, not only the keyboard's.
static const Row *menu_row(const char *lang) {
    const Row *r = find_row(ui_text_lang(lang));
    return r && r->text[UI_MENU_CONTINUE] ? r : NULL;
}

const char *menu_lang(void) {
    const char *v = cfg_get("ACR_MENU_LANG");
    const Row *r = v && *v ? menu_row(v) : menu_row(ov_system_language());
    return r ? r->lang : "en";
}

const char *menu_text(UiTextId id) { return ui_text_in(menu_lang(), id); }
