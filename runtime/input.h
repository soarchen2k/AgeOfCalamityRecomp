// Host input sampling (runtime/input.c, with input_win.c or input_sdl.c): SDL3 gamepad and the keyboard as one Switch npad.
#pragma once
#include <stdint.h>

// Bits of libnx HidNpadButton (A=0 B=1 X=2 Y=3 StickL=4 StickR=5 L=6 R=7 ZL=8 ZR=9 Plus=10 Minus=11 Left=12 Up=13 Right=14
// Down=15) and the stick directions (StickLLeft..Down 16-19, StickRLeft..Down 20-23, derived from the sticks).
// Sticks are -32767..32767, up is +y. ACR_INPUT=none reports no input. Callable from any thread.
void input_poll(uint64_t *buttons, int32_t *lx, int32_t *ly, int32_t *rx, int32_t *ry);
// The same for host UI (the text entry): never blocked. input_block(1) while a text entry is open: input_poll then
// reports nothing; input_block(0) when it closes: nothing until all buttons and stick directions are released.
void input_poll_ui(uint64_t *buttons, int32_t *lx, int32_t *ly, int32_t *rx, int32_t *ry);
void input_block(int on);
// The in-game menu (ui/menu.h): input_menu(1) when it opens (input_poll reports nothing, the cursor is released), (0) when
// it closes (nothing until every button and stick direction is released). Independent of input_block.
void input_menu(int open);
// The gamepad alone, for the menu: Switch button bits and the left stick (up +y). 0 without a connected gamepad.
// Minus+Plus combo and the neutral pad while the menu is open (input_map.c, window-system free): *held is the caller's memory
// of the combo, sticks are lx, ly, rx, ry. Returns 1 in the poll where the combo goes down (toggle the menu once).
int input_menu_gate(int *held, int menu_open, uint64_t *buttons, int32_t sticks[4]);
int input_overlay_gate(int *held, uint64_t *buttons);   // L3+R3: 1 in the poll both go down
// L3+R3 tapped (1, on release) or held for hold_ns (2, once per hold): the overlay step and the quick menu.
typedef struct { int held, fired; uint64_t since; } InputQuickGate;
int input_quick_gate(InputQuickGate *q, uint64_t *buttons, uint64_t now_ns, uint64_t hold_ns);
int input_pad_read(uint64_t *buttons, int32_t *lx, int32_t *ly);
// Player 2 (co-op, ov_input.c npad id 1): the second connected gamepad alone, 0 without one (ACR_PLAYER2=0: never).
// Nothing while a menu or a text entry is open. Any thread.
int input_poll_player2(uint64_t *buttons, int32_t *lx, int32_t *ly, int32_t *rx, int32_t *ry);
int input_player2_connected(void);
int input_rumble2(uint16_t low, uint16_t high, uint32_t ms);   // input_rumble on player 2's gamepad
// The gamepad layout in effect (PAD_LAYOUT_*, input_map.h; ACR_PAD_LAYOUT, live); -1 without a gamepad. Any thread.
int input_pad_layout(void);
// Hands prompts.c (prompts_face) the face buttons' bindings in effect now, before the first poll too: the game uploads
// its button atlas once, at start. Any thread.
void input_face_refresh(void);
// How far the open gamepad's left (pct[0]) and right (pct[1]) sticks are from the centre now, in percent of full
// deflection, before the dead zone: the menu shows it on the dead zone rows, so a drifting stick's resting value can be
// read off. 0 (and both 0) without a connected gamepad. Any thread.
int input_pad_stick_percent(double pct[2]);
// The label on a face button of the open gamepad (input_map.h PAD_SOUTH..PAD_NORTH, as SDL reports it); PAD_LABEL_NONE for another
// button, an unknown label or no gamepad. Any thread.
enum { PAD_LABEL_NONE, PAD_LABEL_A, PAD_LABEL_B, PAD_LABEL_X, PAD_LABEL_Y, PAD_LABEL_CROSS, PAD_LABEL_CIRCLE,
       PAD_LABEL_SQUARE, PAD_LABEL_TRIANGLE };
int input_pad_label(int pad);
// The menu's remapping rows (Controls tab). input_map_snapshot copies the tables in effect (after any changed ACR_BIND_* line).
// input_capture polls the keys, the mouse buttons and the gamepad for a press: reset 1 takes what is held now as the
// start (a held key counts once released and pressed again) and returns NONE; then it returns KEY or PAD with *id the
// key or pad id (input_map.h) pressed since the last call, or CANCEL for Esc. Keys count only while the game window is in front.
enum { INPUT_CAPTURE_NONE, INPUT_CAPTURE_KEY, INPUT_CAPTURE_PAD, INPUT_CAPTURE_CANCEL };
struct InputMap;   // input_map.h (not included here: swkbd_win.c has button names of its own)
void input_map_snapshot(struct InputMap *m);
int input_capture(int reset, int *id);
// The character on a punctuation key (KEY_MINUS..KEY_GRAVE) in the keyboard layout in use, 0 for another key.
uint32_t input_key_char(int key);
// F8, the flight recorder's key (no KEY_ id), held while the game window is in front.
int input_flight_key(void);
int input_stepscan_key(void);   // F9 held while the game window has focus (stepscan.c)
void input_force_lstick(int on, int32_t x, int32_t y);   // hold this left-stick tilt over the pads (stepscan.c)

// Mouse camera and cursor (input_win.c, Win32 window thread). The window procedure calls input_window_msg first for raw
// mouse input, cursor hiding and confinement; it returns 1 with *res set when the message is handled. input_cursor_suspend(1)
// releases the cursor around an F11 window change, (0) captures it again once the client area has its new size.
void input_cursor_suspend(int on);
int input_cursor_released(void);   // Esc or a non-client activation released the cursor (probe)

// Rumble: SDL_RumbleGamepad on the open gamepad, low (heavy, left) and high (light, right) motor 0..65535 for ms
// milliseconds. 1 if a gamepad with rumble took it. Callable from any thread.
int input_rumble(uint16_t low, uint16_t high, uint32_t ms);
// Motor value for a normalised amplitude: pow(a, curve) * strength, saturated to 0..65535. Pure.
uint16_t rumble_motor(float a, float curve, float strength);
// ACR_RUMBLE_TRIGGERS=1: input_rumble also drives the impulse triggers (Xbox) with the high motor's value.
int input_rumble_triggers_enabled(void);

// Motion (ACR_MOTION=0 turns it off, live). input_motion_start starts the sensor thread once: every 4 ms it reads the
// gamepad's gyro and accelerometer (when it has them) and feeds motion_global(). Callable from any thread.
void input_motion_start(void);
// ACR_MOTION is not "0" (a live setting: read again when cfg_version changes). Off: the sensor reads at rest.
int input_motion_enabled(void);
// The Controls tab's Calibrate gyro row: start a bias measurement on the open gamepad (starts the sensor thread when
// the game has not yet). Callable from any thread.
void input_motion_calibrate(void);
// What the Calibrate gyro row shows (MOTION_CALIB_*): NONE no bias for this controller, COLLECTING, DONE a bias in
// effect (measured in this run or loaded from settings.ini), FAILED the last attempt got no samples and no bias is in
// effect, MOVED the last attempt was rejected because the controller moved. pad receives the name of the controller the
// bias belongs to (empty when none).
enum { MOTION_CALIB_NONE, MOTION_CALIB_COLLECTING, MOTION_CALIB_DONE, MOTION_CALIB_FAILED, MOTION_CALIB_MOVED };
int input_motion_calib_state(char *pad, size_t cap);
// The bias's own switch (ACR_GYRO_CALIBRATION, live, motion_fusion.c). Off: no collection, the fusion takes the raw
// gyro, and the saved keys stay in settings.ini.
int motion_calibration_enabled(void);
// SDL sensor values (accelerometer m/s^2, gyro rad/s, SDL gamepad axes) to the Switch controller frame motion_feed
// takes (acceleration in G as gravity reads, rotation rad/s). The mapping is documented in input_win.c.
void input_motion_axes(const float sdl_acc[3], const float sdl_gyro[3], double acc_g[3], double gyro[3]);

// motion_fusion.c: nn::hid::SixAxisSensorState and the sensor fusion that produces it.
typedef struct {
    int64_t delta_ns, sampling;
    float acc[3], gyro[3], angle[3], dir[9];   // G, revolutions/s, revolutions, controller axes in the reference frame
    uint32_t attributes, reserved;             // bit 0 connected
} MotionState;
_Static_assert(sizeof(MotionState) == 0x60, "SixAxisSensorState ABI");
typedef struct MotionFusion MotionFusion;
MotionFusion *motion_new(void);
MotionFusion *motion_global(void);   // the gamepad's, fed by input_win.c
void motion_feed(MotionFusion *m, const double acc_g[3], const double gyro_rad_s[3], uint64_t now_ns);
int motion_read(MotionFusion *m, MotionState *out, int max, uint64_t now_ns);   // newest first; 0 = none in 200 ms
void motion_restart(MotionFusion *m);   // angle and direction start over at the next sample
// Start a bias measurement at now_ns: samples while the controller rests, then the bias or a rejection (MOTION_CALIB_*).
// pad is the controller's identity, written to settings.ini with the bias.
void motion_calibrate_start(MotionFusion *m, const char *pad, uint64_t now_ns);
int motion_calib_poll(MotionFusion *m, uint64_t now_ns);   // MOTION_CALIB_*; ends an attempt whose samples stopped
void motion_bias_load(MotionFusion *m);    // take the bias from settings.ini (ACR_GYRO_BIAS_*)
void motion_bias_reset(MotionFusion *m);   // no bias and no attempt (another controller)
