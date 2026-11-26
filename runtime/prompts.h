// Xbox and PlayStation button prompts in place of the textures of the game's buttons (prompts.c).
#pragma once
#include <stddef.h>
#include <stdint.h>

// FNV-1a 64 of n bytes (a texture's fingerprint; tools/make_prompts.py computes the same).
uint64_t prompts_hash(const uint8_t *p, size_t n);
enum { PROMPTS_FILES = 4 };   // romfs.c prompts_files: Xbox and PlayStation, each by position and by label
// Loads a file of tools/make_prompts.py into slot 0..PROMPTS_FILES-1: 1 when it is whole and well formed, 0 otherwise
// (the slot stays empty then).
int prompts_load(int slot, const char *path);
// The slot whose buttons uploads take from now on, -1 (or an empty slot) for the game's own. Live: a change bumps
// prompts_gen, and vk_texture.c uploads the textures prompts_swap recognised again.
void prompts_select(int slot);
uint32_t prompts_gen(void);
// A texture upload's blocks (w x h, BC3 when bc3, size bytes in rows): 1 when they are one of the game's button
// textures (the selected file's blocks are then copied over them; the game's own stay with none selected), else 0.
int prompts_swap(uint32_t w, uint32_t h, int bc3, uint8_t *blocks, size_t size);
// The pad in use: its layout (input_map.h PAD_LAYOUT_*: 0 Nintendo, 1 Xbox) and, for Switch A, B, X, Y, the face button
// that presses it (0 south, 1 east, 2 west, 3 north; -1 none). A change bumps prompts_gen, and the button atlas
// uploads again with the icons of remapped buttons moved: each button shows the one that now presses it.
enum { PROMPTS_LAYOUTS = 2 };
void prompts_face(int layout, const int8_t phys[4]);
