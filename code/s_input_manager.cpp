/* ========================================================================
   $File: s_input_manager.cpp $
   $Date: Sat, 23 Aug 25: 11:57AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "s_input_manager.h"

/*===========================================
  =============== GENERAL API ===============
  ===========================================*/

internal void
s_input_manager_reset_controller_states(input_manager_t *input_manager)
{
    for(u32 controller_index = 0;
        controller_index < MAX_INPUT_CONTROLLERS;
        ++controller_index)
    {
        input_controller_t *controller = input_manager->controllers + controller_index;
        if(controller->is_valid)
        {
            switch(controller->type)
            {
                case IM_CONTROLLER_KEYBOARD:
                {
                    for(u32 key_index = 0;
                        key_index < SDL_SCANCODE_MAX;
                        ++key_index)
                    {
                        action_button_t *button = controller->keyboard.input + key_index;
                        button->is_down                 = false;
                        button->is_released             = false;
                        button->is_pressed              = false;
                        button->half_transition_counter = 0;
                    }
                }break;
                case IM_CONTROLLER_GAMEPAD:
                {
                    for(u32 button_index = 0;
                        button_index < SDL_GAMEPAD_BUTTON_COUNT;
                        ++button_index)
                    {
                        action_button *button = controller->gamepad.digital_buttons + button_index;
                        button->is_down                 = false;
                        button->is_released             = false;
                        button->is_pressed              = false;
                        button->half_transition_counter = 0;
                    }

                    for(u32 analog_button_index = 0;
                        analog_button_index < SDL_GAMEPAD_AXIS_COUNT;
                        ++analog_button_index)
                    {
                        analog_button_t *button = controller->gamepad.analog_buttons + analog_button_index;
                        button->value = 0;
                    }
                }break;
                default: {InvalidCodePath;}break;
            }
        }
    }
}

internal void
s_input_manager_initialize_keyboard_controller(input_manager_t *input_manager, s32 index)
{
    input_controller_t *controller = input_manager->controllers + index;
    ZeroStruct(*controller);

    controller->is_valid  = true;
    controller->is_analog = false;
    controller->type      = IM_CONTROLLER_KEYBOARD;
}


internal input_controller_t *
s_input_manager_get_primary_controller(input_manager_t *input_manager)
{
    input_controller_t *result = null;
    result = input_manager->controllers + input_manager->primary_controller_index;

    return(result);
}

internal input_controller_t *
s_input_manager_get_controller_at_index(input_manager_t *input_manager, s32 index)
{
    Assert(index < MAX_INPUT_CONTROLLERS);
    
    input_controller_t *result = null;
    result = input_manager->controllers + index;

    return(result);
}

internal input_controller_t *
s_input_manager_get_active_controller(input_manager_t *input_manager)
{
    input_controller_t *result = null;
    result = input_manager->controllers + input_manager->active_controller_index;

    return(result);
}
/*==============================================
  =============== KEYBOARD INPUT ===============
  ==============================================*/

internal bool8
s_input_manager_is_keyboard_key_pressed(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);
    bool8 result = false;

    action_button_t *button = controller->keyboard.input + key_index;

    result = button->is_pressed;
    return(result);
}

internal bool8
s_input_manager_is_keyboard_key_down(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);
    bool8 result = false;

    action_button_t *button = controller->keyboard.input + key_index;

    result = button->is_down;
    return(result);
}

internal bool8
s_input_manager_is_keyboard_key_released(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);
    bool8 result = false;

    action_button_t *button = controller->keyboard.input + key_index;

    result = button->is_released;
    return(result);
}

internal void 
s_input_manager_consume_keyboard_key_press(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);

    action_button_t *button         = controller->keyboard.input + key_index;
    button->is_pressed              = false;
    button->half_transition_counter = 0;
}

internal void 
s_input_manager_consume_keyboard_key_down(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);

    action_button_t *button         = controller->keyboard.input + key_index;
    button->is_down                 = false;
    button->half_transition_counter = 0;
}

internal void 
s_input_manager_consume_keyboard_key_release(input_controller_t *controller, s32 key_index)
{
    Assert(controller->type == IM_CONTROLLER_KEYBOARD);

    action_button_t *button         = controller->keyboard.input + key_index;
    button->is_released             = false;
    button->half_transition_counter = 0;
}

/*=============================================
  =============== GAMEPAD INPUT ===============
  =============================================*/

internal bool8
s_input_manager_is_gamepad_button_pressed(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);
    bool8 result = false;

    action_button_t *button = controller->gamepad.digital_buttons + button_index;

    result = button->is_pressed;
    return(result);
}

internal bool8
s_input_manager_is_gamepad_button_down(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);
    bool8 result = false;

    action_button_t *button = controller->gamepad.digital_buttons + button_index;

    result = button->is_down;
    return(result);
}

internal bool8
s_input_manager_is_gamepad_button_released(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);
    bool8 result = false;

    action_button_t *button = controller->gamepad.digital_buttons + button_index;

    result = button->is_released;
    return(result);
}

internal void 
s_input_manager_consume_gamepad_button_press(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);

    action_button_t *button         = controller->gamepad.digital_buttons + button_index;
    button->is_pressed              = false;
    button->half_transition_counter = 0;
}

internal void 
s_input_manager_consume_gamepad_button_down(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);

    action_button_t *button         = controller->gamepad.digital_buttons + button_index;
    button->is_down                 = false;
    button->half_transition_counter = 0;
}

internal void 
s_input_manager_consume_gamepad_button_release(input_controller_t *controller, s32 button_index)
{
    Assert(controller->type == IM_CONTROLLER_GAMEPAD);

    action_button_t *button         = controller->gamepad.digital_buttons + button_index;
    button->is_released             = false;
    button->half_transition_counter = 0;
}

/*===============================================
  =============== GAME ACTION API ===============
  ===============================================*/
