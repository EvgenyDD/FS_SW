#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    int16_t debounce_config;
    int16_t debounce_config_long;
    int16_t debounce_timer;
    int16_t press_counter;
    bool pressed;
    bool pressed_shot;
    bool pressed_shot_long;
    bool unpressed_shot;

    bool pressed_long;
} button_ctrl_t;

/**
 * @brief Debounce structure initializer
 * 
 * @param ctrl 
 * @param timeout 
 */
inline void debounce_init(button_ctrl_t *ctrl, int16_t timeout, int16_t timeout_long_press)
{
    memset((void *)ctrl, 0, sizeof(button_ctrl_t));
    ctrl->debounce_config = timeout;
    ctrl->debounce_config_long = timeout_long_press;
}

/**
 * @brief Callback for debounce engine
 * 
 * @param ctrl 
 * @param time_cb_diff
 * @return true state changed
 * @return false 
 */
inline bool debounce_cb(button_ctrl_t *ctrl, bool state_now, int16_t time_cb_diff)
{
    if(ctrl->debounce_timer != 0)
    {
        if(ctrl->debounce_timer < time_cb_diff)
            ctrl->debounce_timer = 0;
        else
            ctrl->debounce_timer -= time_cb_diff;
    }

    ctrl->pressed_shot = false;
    ctrl->unpressed_shot = false;
    ctrl->pressed_shot_long = false;

    if((ctrl->pressed != state_now) &&
       (ctrl->debounce_timer == 0))
    {
        ctrl->pressed_long = false;
        ctrl->pressed_shot_long = false;
        ctrl->press_counter = 0;
        ctrl->debounce_timer = ctrl->debounce_config;
        ctrl->pressed = state_now;
        if(ctrl->pressed)
            ctrl->pressed_shot = true;
        else
            ctrl->unpressed_shot = true;
        return true;
    }

    if(ctrl->pressed)
    {
        ctrl->press_counter += time_cb_diff;
        if(ctrl->press_counter > ctrl->debounce_config_long)
        {
            if(ctrl->pressed_long == false)
                ctrl->pressed_shot_long = true;
            ctrl->pressed_long = true;
        }
    }
    else
    {
        ctrl->pressed_long = false;
        ctrl->pressed_shot_long = false;
        ctrl->press_counter = 0;
    }

    return false;
}

#endif // DEBOUNCE_H