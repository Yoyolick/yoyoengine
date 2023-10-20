/*
    This file is a part of yoyoengine. (https://github.com/yoyolick/yoyoengine)
    Copyright (C) 2023  Ryan Zmuda

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <yoyoengine/yoyoengine.h>
#include "editor.h"
#include <math.h>

/*
    Getting this equation right was exponentially more difficult than you could possibly imagine
*/
void editor_update_mouse_world_pos(int x, int y){
    float scaleX = (float)engine_state.screen_width / (float)engine_state.target_camera->camera->view_field.w;
    float scaleY = (float)engine_state.screen_height / (float)engine_state.target_camera->camera->view_field.h;
    mouse_world_x = ((x / scaleX) + editor_camera->transform->rect.x);
    mouse_world_y = ((y / scaleY) + editor_camera->transform->rect.y);
}

/*
    Checks if the mouse is within the editor viewport
*/
bool is_hovering_editor(int x, int y){
    return (x > 0 && x < screenWidth / 1.5 &&
            y > 0 && y < screenHeight / 1.5);
}

float camera_zoom = 1.0;
float camera_zoom_sens = 1.0;
void editor_input_panning(SDL_Event event){
    // get the actual mouse position (the mouse wheel event will not give us the actual mouse position)
    int x, y;
    SDL_GetMouseState(&x, &y);

    // initial right click down, initialize panning
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_RIGHT) {
            if (is_hovering_editor(x, y) &&
                !lock_viewport_interaction)
            {
                dragging = true;
                last_x = event.button.x;
                last_y = event.button.y;
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL));
            }
        }
    }
    // release right click, stop panning TODO: add checks for if we are even panning here
    else if (event.type == SDL_MOUSEBUTTONUP) {
        if (event.button.button == SDL_BUTTON_RIGHT)
        {
            dragging = false;
            SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
        }
    }
    // mouse movement, if we are panning then move the camera
    else if (event.type == SDL_MOUSEMOTION) {
        if (dragging)
        {
            int dx = event.motion.x - last_x;
            int dy = event.motion.y - last_y;
            editor_camera->transform->rect.x -= dx;
            editor_camera->transform->rect.y -= dy;
            last_x = event.motion.x;
            last_y = event.motion.y;
        }
    }
    // camera zooming TODO: disabled. Im losing my mind.
    else if (event.type == SDL_MOUSEWHEEL)
    {
        float dt = (float)engine_runtime_state.frame_time / 1000.0;
        float zoom_factor = 0.1f; // Adjust this value to control the zoom speed

        if (event.wheel.y > 0)
        {
            camera_zoom *= pow(200.0f, zoom_factor * dt);
            // Ensure camera_zoom doesn't go above 10x
            camera_zoom = fmin(camera_zoom, 10.0f);
        }
        else if (event.wheel.y < 0)
        {
            camera_zoom /= pow(200.0f, zoom_factor * dt);
            // Ensure camera_zoom doesn't go below 0.1x
            camera_zoom = fmax(camera_zoom, 0.1f);
        }

        editor_camera->camera->view_field.w = screenWidth / camera_zoom;
        editor_camera->camera->view_field.h = screenHeight / camera_zoom;

        editor_update_mouse_world_pos(x, y);
    }
}

/*
    Selecting entities and manipulating them
*/
void editor_input_selection(SDL_Event event){
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {   
            // dont do anything if we are outside the viewport
            if (is_hovering_editor(event.button.x,event.button.y) &&
                !lock_viewport_interaction){

                bool nothing = true;
                // check what entity we clicked over, we clicked on an entity (thats ignoring our current selection) we should change the current selection, else we deselect
                struct ye_entity_node *clicked_entity = entity_list_head;
                while (clicked_entity != NULL)
                {
                    if(clicked_entity->entity == engine_runtime_state.selected_entity || clicked_entity->entity->camera != NULL)
                    {
                        clicked_entity = clicked_entity->next;
                        continue;
                    }

                    if (ye_point_in_rect(mouse_world_x, mouse_world_y, ye_convert_rectf_rect(clicked_entity->entity->transform->bounds))) // TODO: bounds vs rect here
                    {
                        // we clicked on this entity
                        if (clicked_entity != engine_runtime_state.selected_entity)
                        {
                            engine_runtime_state.selected_entity = clicked_entity->entity;
                        }
                        nothing = false;
                        break;
                    }
                    clicked_entity = clicked_entity->next; // TODO: solve world mouse point
                }
                if(nothing){
                    engine_runtime_state.selected_entity = NULL;
                }
            }
        }
    }
}

/*
    Any Misc items that need to happen on editor input
*/
void editor_input_misc(SDL_Event event){
    // update the mouse world position
    if(event.type == SDL_MOUSEMOTION){
        editor_update_mouse_world_pos(event.motion.x, event.motion.y);
    }
}

/*
    Any keyboard shortcuts for the editor
*/
void editor_input_shortcuts(SDL_Event event){
    if (event.type == SDL_KEYDOWN)
    {
        switch (event.key.keysym.sym)
        {
        case SDLK_s:
            // CTRL + S ->= save the scene
            if (event.key.keysym.mod & KMOD_CTRL)
            {
                // TODO: save the scene
                printf("TODO: IMPLEMENT SAVING SCENE. -> Ctrl+S pressed\n");
            }
            break;
        default:
            break;
        }
    }
}

/*
    Registered input handler

    TODO:
    - zoom in should center on the mouse or the center of the screen
*/
void editor_handle_input(SDL_Event event) {
    if (event.type == SDL_QUIT)
        quit = true;

    // misc input handling
    editor_input_panning(event);
    editor_input_selection(event);
    editor_input_misc(event);
    editor_input_shortcuts(event);
}