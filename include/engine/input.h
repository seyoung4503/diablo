#ifndef DIABLO_INPUT_H
#define DIABLO_INPUT_H

#include "common.h"

typedef struct InputState {
    /* Keyboard */
    const Uint8 *keys;                     /* current frame key state (SDL_GetKeyboardState) */
    Uint8 keys_prev[SDL_NUM_SCANCODES];    /* previous frame for "just pressed" detection */

    /* Mouse */
    int mouse_x, mouse_y;                  /* screen position */
    Uint32 mouse_buttons;                  /* current frame */
    Uint32 mouse_buttons_prev;             /* previous frame */
} InputState;

/* Initialize input state. Call once before the game loop. */
void input_init(InputState *input);

/*
 * Save previous frame state and snapshot current keyboard/mouse.
 * Must be called BEFORE SDL_PollEvent each frame.
 */
void input_update(InputState *input);

/* Returns true while the key is held down. */
bool input_key_held(const InputState *input, SDL_Scancode key);

/* Returns true only on the frame the key was first pressed. */
bool input_key_pressed(const InputState *input, SDL_Scancode key);

/* Returns true while the mouse button is held down (SDL_BUTTON_LEFT, etc). */
bool input_mouse_held(const InputState *input, int button);

/* Returns true only on the frame the mouse button was first clicked. */
bool input_mouse_clicked(const InputState *input, int button);

#endif /* DIABLO_INPUT_H */
