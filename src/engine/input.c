#include "engine/input.h"

void input_init(InputState *input)
{
    memset(input, 0, sizeof(*input));
    input->keys = SDL_GetKeyboardState(NULL);
}

void input_update(InputState *input)
{
    /* Save previous keyboard state */
    memcpy(input->keys_prev, input->keys, SDL_NUM_SCANCODES);

    /* Save previous mouse state */
    input->mouse_buttons_prev = input->mouse_buttons;

    /* Snapshot current mouse state */
    input->mouse_buttons = SDL_GetMouseState(&input->mouse_x, &input->mouse_y);
}

bool input_key_held(const InputState *input, SDL_Scancode key)
{
    return input->keys[key];
}

bool input_key_pressed(const InputState *input, SDL_Scancode key)
{
    return input->keys[key] && !input->keys_prev[key];
}

bool input_mouse_held(const InputState *input, int button)
{
    return (input->mouse_buttons & SDL_BUTTON(button)) != 0;
}

bool input_mouse_clicked(const InputState *input, int button)
{
    return (input->mouse_buttons & SDL_BUTTON(button)) != 0 &&
           (input->mouse_buttons_prev & SDL_BUTTON(button)) == 0;
}
