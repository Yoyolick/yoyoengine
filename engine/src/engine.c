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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

#include <yoyoengine/yoyoengine.h>

// buffer to hold filepath strings
// will be modified by getPath()
char path_buffer[1024];

// get the base path
char *base_path = NULL;

// expose our engine state data to the whole engine
struct ye_engine_state YE_STATE; // TODO: should null out pointers and stuff? maybe some of them are only used SOMETIMES but not all the time

// Global variables for resource paths
char *executable_path = NULL;

char* ye_get_resource_static(const char *sub_path) {
    static char resource_buffer[256];  // Adjust the buffer size as per your requirement

    if (YE_STATE.engine.game_resources_path == NULL) {
        ye_logf(error, "Resource paths not set!\n");
        return NULL;
    }

    snprintf(resource_buffer, sizeof(resource_buffer), "%s/%s", YE_STATE.engine.game_resources_path, sub_path);
    return resource_buffer;
}

char* ye_get_engine_resource_static(const char *sub_path) {
    static char engine_reserved_buffer[256];  // Adjust the buffer size as per your requirement

    if (YE_STATE.engine.engine_resources_path == NULL) {
        ye_logf(error, "Engine reserved paths not set!\n");
        return NULL;
    }

    snprintf(engine_reserved_buffer, sizeof(engine_reserved_buffer), "%s/%s", YE_STATE.engine.engine_resources_path, sub_path);
    return engine_reserved_buffer;
}

// event polled for per frame
SDL_Event e;

int last_frame_time = 0;
bool console_visible = false;
void ye_process_frame(){
    // bool to track resize events
    bool resized = false;

    // update time delta
    YE_STATE.runtime.delta_time = (SDL_GetTicks64() - last_frame_time) / 1000.0f;
    last_frame_time = SDL_GetTicks64();

    // C pre frame callback
    if(YE_STATE.engine.callbacks.pre_frame != NULL){
        YE_STATE.engine.callbacks.pre_frame();
    }

    int input_time = SDL_GetTicks64();
    ui_begin_input_checks();
    while (SDL_PollEvent(&e)) {
        ui_handle_input(&e);

        // if resize event, set resized to true
        // if(e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED){
        //     resized = true;
        // }

        // switch is prob faster here
        switch(e.type){
            // check for any reserved engine buttons (console, etc)
            case SDL_KEYDOWN:
                // if freecam is on (rare) TODO: allow changing of freecam scale
                if(YE_STATE.editor.freecam_enabled){
                    switch(e.key.keysym.sym){     
                        case SDLK_LEFT:
                            YE_STATE.engine.target_camera->transform->x -= 100.0;
                            break;
                        case SDLK_RIGHT:
                            YE_STATE.engine.target_camera->transform->x += 100.0;
                            break;
                        case SDLK_UP:
                            YE_STATE.engine.target_camera->transform->y -= 100.0;
                            break;
                        case SDLK_DOWN:
                            YE_STATE.engine.target_camera->transform->y += 100.0;
                            break;
                    }
                }

                switch(e.key.keysym.sym){

                    // console toggle //

                    case SDLK_BACKQUOTE:
                        if(console_visible){
                            console_visible = false;
                            remove_ui_component("console");
                        }
                        else{
                            console_visible = true;
                            ui_register_component("console",ye_paint_console);
                        }
                        break;
                    //default:
                    //    break;
                }
                break; // breaks out of keydown

            // window events //
            case SDL_WINDOWEVENT:
                switch(e.window.event){
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        resized = true;
                        break;
                }
                break;

            //default:
            //    break;
        }

        // send event to callback specified by game (if needed)
        if(YE_STATE.engine.callbacks.input_handler != NULL){
            YE_STATE.engine.callbacks.input_handler(e);
        }
    }
    ui_end_input_checks();
    YE_STATE.runtime.input_time = SDL_GetTicks64() - input_time;

    int physics_time = SDL_GetTicks64();
    if(!YE_STATE.editor.editor_mode){
        // update physics
        ye_system_physics(); // TODO: decouple from framerate
    }
    YE_STATE.runtime.physics_time = SDL_GetTicks64() - physics_time;

    // if we resized, update all the meta that we need so we can render a new clean frame
    if(resized){
        int width, height;
        SDL_GetWindowSize(YE_STATE.runtime.window, &width, &height);
        
        // log the new size
        ye_logf(debug, "Window resized to %d, %d.\n", width, height);

        // update the engine state
        YE_STATE.engine.screen_width = width;
        YE_STATE.engine.screen_height = height;

        // editor camera update handled in editor_input.c

        // recompute pillar or letter boxing
        ye_recompute_boxing();
    }

    // run all trick update callbacks
    ye_run_trick_updates();

    // run all scripting before the frame is rendered
    ye_system_lua_scripting();

    // render frame
    ye_render_all();

    YE_STATE.runtime.frame_time = SDL_GetTicks64() - last_frame_time;

    // C post frame callback
    if(YE_STATE.engine.callbacks.post_frame != NULL){
        YE_STATE.engine.callbacks.post_frame();
    }
}

float ye_delta_time(){
    return YE_STATE.runtime.delta_time;
}

void set_setting_string(char* key, char** value, json_t* settings) {
    const char * json_value;
    if (ye_json_string(settings, key, &json_value)) {
        /*
            The existing value already exists as a default, so we need to free it
        */
        free(*value);

        // set the value to the new one
        *value = strdup(json_value);
    }
}

void set_setting_int(char* key, int* value, json_t* settings) {
    int json_value;
    if (ye_json_int(settings, key, &json_value)) {
        *value = json_value;
    }
}

void set_setting_bool(char* key, bool* value, json_t* settings) {
    bool json_value;
    if (ye_json_bool(settings, key, &json_value)) {
        *value = json_value;
    }
}

void set_setting_float(char* key, float* value, json_t* settings) {
    float json_value;
    if (ye_json_float(settings, key, &json_value)) {
        *value = json_value;
    }
}

void ye_update_resources(char *path){
    // update the engine state
    free(YE_STATE.engine.game_resources_path);
    YE_STATE.engine.game_resources_path = strdup(path);
}

void ye_init_engine() {
    // Get the path to our executable
    executable_path = SDL_GetBasePath(); // Don't forget to free memory later

    // Set default paths for engineResourcesPath and gameResourcesPath
    char engine_default_path[256], game_default_path[256], log_default_path[256];
    snprintf(engine_default_path, sizeof(engine_default_path), "%sengine_resources", executable_path);
    snprintf(game_default_path, sizeof(game_default_path), "%sresources", executable_path);
    snprintf(log_default_path, sizeof(log_default_path), "%sdebug.log", executable_path);

    // set defaults for engine state
    YE_STATE.engine.framecap = -1;
    YE_STATE.engine.screen_width = 1920;
    YE_STATE.engine.screen_height = 1080;
    YE_STATE.engine.window_mode = 0;
    YE_STATE.engine.volume = 64;
    YE_STATE.engine.window_title = strdup("Yoyo Engine Window");

    // set default paths, if we have an override we can change them later
    YE_STATE.engine.engine_resources_path = strdup(engine_default_path);
    YE_STATE.engine.game_resources_path = strdup(game_default_path);
    YE_STATE.engine.log_file_path = strdup(log_default_path);
    YE_STATE.engine.icon_path = strdup(ye_get_engine_resource_static("enginelogo.png"));

    YE_STATE.engine.log_level = 4;

    // check if ./settings.yoyo exists (if not, use defaults)
    json_t *SETTINGS = ye_json_read(ye_get_resource_static("../settings.yoyo"));
    if (SETTINGS == NULL) {
        ye_logf(warning, "No settings.yoyo file found, using defaults.\n");
    }
    else{
        ye_logf(info, "Found settings.yoyo file, using values from it.\n");

        set_setting_string("engine_resources_path", &YE_STATE.engine.engine_resources_path, SETTINGS);
        set_setting_string("game_resources_path", &YE_STATE.engine.game_resources_path, SETTINGS);
        set_setting_string("log_file_path", &YE_STATE.engine.log_file_path, SETTINGS);
        set_setting_string("icon_path", &YE_STATE.engine.icon_path, SETTINGS);
        set_setting_string("window_title", &YE_STATE.engine.window_title, SETTINGS);

        set_setting_int("window_mode", &YE_STATE.engine.window_mode, SETTINGS);
        set_setting_int("volume", &YE_STATE.engine.volume, SETTINGS);
        set_setting_int("log_level", &YE_STATE.engine.log_level, SETTINGS);
        set_setting_int("screen_width", &YE_STATE.engine.screen_width, SETTINGS);
        set_setting_int("screen_height", &YE_STATE.engine.screen_height, SETTINGS);
        set_setting_int("framecap", &YE_STATE.engine.framecap, SETTINGS);

        set_setting_bool("debug_mode", &YE_STATE.engine.debug_mode, SETTINGS);
        set_setting_bool("skip_intro", &YE_STATE.engine.skipintro, SETTINGS);
        set_setting_bool("editor_mode", &YE_STATE.editor.editor_mode, SETTINGS);

        set_setting_bool("stretch_resolution", &YE_STATE.engine.stretch_resolution, SETTINGS);

        // we will decref settings later on after we load the scene, so the path to the entry scene still exists
    }

    // initialize some editor state
    YE_STATE.editor.scene_default_camera = NULL;
    YE_STATE.editor.selected_entity = NULL;

    // ----------------- Begin Setup -------------------

    // initialize graphics systems, creating window renderer, etc
    // TODO: should this just take in engine state struct? would make things a lot easier tbh
    ye_init_graphics();

    // init timers
    ye_init_timers();

    // initialize the cache
    ye_init_cache();

    // load a font for use in engine (value of global in engine.h modified) this will be used to return working fonts if a user specified one cannot be loaded
    YE_STATE.engine.pEngineFont = ye_load_font(ye_get_engine_resource_static("RobotoMono-Light.ttf"));
    // since we are bypassing the cache to do this, we need to customly resize this
    TTF_SetFontSize(YE_STATE.engine.pEngineFont, 128); // high enough to be readable where-ever (I think)

    // allocate memory for and create a pointer to our engineFontColor struct for use in graphics.c
    // this is also returned as a color if a user specified one cannot be loaded
    SDL_Color engineFontColor = {255, 255, 0, 255};
    YE_STATE.engine.pEngineFontColor = &engineFontColor;
    YE_STATE.engine.pEngineFontColor = malloc(sizeof(SDL_Color));
    YE_STATE.engine.pEngineFontColor->r = 255;
    YE_STATE.engine.pEngineFontColor->g = 255;
    YE_STATE.engine.pEngineFontColor->b = 0;
    YE_STATE.engine.pEngineFontColor->a = 255;

    // no matter what we will initialize log level with what it should be. default is nothing but dev can override
    ye_log_init(YE_STATE.engine.log_file_path);

    if(YE_STATE.editor.editor_mode){
        ye_logf(info, "Detected editor mode.\n");
    }

    // initialize entity component system
    ye_init_ecs();

    // if we are in debug mode
    if(YE_STATE.engine.debug_mode){
        // display in console
        ye_logf(debug, "Debug mode enabled.\n");
    }

    // startup audio systems
    ye_audio_init();

    // before we play our loud ass startup sound, lets check what volume the game wants
    // the engine to be at initially
    ye_set_volume(-1, YE_STATE.engine.volume);

    // initialize and load tricks (modules/plugins)
    ye_init_tricks();

    // set our last frame time now because we might play the intro
    last_frame_time = SDL_GetTicks64();

    /*
        Part of the engine startup which isnt configurable by the game is displaying
        a splash screen with the engine title and logo for 2550ms and playing a
        startup noise
    */
    if(YE_STATE.engine.skipintro){
        ye_logf(info,"Skipping Intro.\n");
    }
    else{
        ye_play_sound(ye_get_engine_resource_static("startup.mp3"),0,0); // play startup sound

        // im not a particularly massive fan of using the unstable ECS just yet, but might as well
        struct ye_entity * splash_cam = ye_create_entity();
        ye_add_transform_component(splash_cam, 0,0);
        ye_add_camera_component(splash_cam, 999, (SDL_Rect){0,0,1920,1080});
        ye_set_camera(splash_cam);

        // background for splash
        struct ye_entity * splash_bg = ye_create_entity();
        ye_add_transform_component(splash_bg, 0,0);
        ye_temp_add_image_renderer_component(splash_bg, 1, ye_get_engine_resource_static("splash_bg.png"));
        splash_bg->renderer->rect = (struct ye_rectf){0,0,1920,1080};

        // foreground logo for splash
        struct ye_entity * splash_y = ye_create_entity();
        ye_add_transform_component(splash_y, 1920/2 - 350/2 - 100,1080/2 - 350/2);
        ye_temp_add_image_renderer_component(splash_y, 1, ye_get_engine_resource_static("splash_y.png"));
        splash_y->renderer->rect = (struct ye_rectf){0,0,350,350};

        // gear spinning below logo
        struct ye_entity * splash_gear = ye_create_entity();
        ye_add_transform_component(splash_gear, 1920/2,1080/2 - 110);
        ye_temp_add_image_renderer_component(splash_gear, 1, ye_get_engine_resource_static("splash_gear.png"));
        splash_gear->renderer->rect = (struct ye_rectf){0,0,350,350};
        ye_add_physics_component(splash_gear,0,0);
        splash_gear->physics->rotational_velocity = 90;

        // TODO: version numbers back please (awaiting text renderer)

        // get current ticks
        int ticks = SDL_GetTicks();

        // until we are 2550 ticks in the future
        while(SDL_GetTicks() - ticks < 2550){
            // process frame
            ye_process_frame();
        }

        // we need to delete everything in the ECS and reset the camera
        ye_destroy_entity(splash_cam);
        ye_set_camera(NULL);
        ye_destroy_entity(splash_bg);
        ye_destroy_entity(splash_y);
        ye_destroy_entity(splash_gear);
    }

    // debug output
    ye_logf(info, "Engine Fully Initialized.\n");

    // retrieve and load the entry scene (editor mode handles this itself)
    if(!YE_STATE.editor.editor_mode){
        const char * entry_scene;
        if (ye_json_string(SETTINGS, "entry_scene", &entry_scene)) {
            ye_logf(info, "Detected entry: %s.\n", entry_scene);
            ye_load_scene(ye_get_resource_static(entry_scene));
        }
        else{
            ye_logf(warning, "No entry_scene specified in settings.yoyo, if you do not load a custom scene the engine will crash.\n");
        }
    }

    // free the settings json as needed
    if(SETTINGS != NULL)
        json_decref(SETTINGS);
} // control is now resumed by the game

void ye_shutdown_engine(){
    ye_logf(info, "Shutting down engine...\n");

    // shut tricks down
    ye_shutdown_tricks();

    // free the engine font color
    free(YE_STATE.engine.pEngineFontColor);
    YE_STATE.engine.pEngineFontColor = NULL;

    // free the engine font
    TTF_CloseFont(YE_STATE.engine.pEngineFont);

    // shutdown ECS
    ye_shutdown_ecs();

    // shutdown timers
    ye_shutdown_timers();

    // shutdown cache
    ye_shutdown_cache();

    // shutdown graphics
    ye_shutdown_graphics();
    ye_logf(info, "Shut down graphics.\n");

    // shutdown audio
    ye_audio_shutdown();
    ye_logf(info, "Shut down audio.\n");

    // shutdown logging
    // note: must happen before SDL because it relies on SDL path to open file
    ye_log_shutdown();
    free(YE_STATE.engine.log_file_path);
    free(YE_STATE.engine.engine_resources_path);
    free(YE_STATE.engine.game_resources_path);
    free(YE_STATE.engine.icon_path);
    SDL_free(base_path); // free base path after (used by logging)
    SDL_free(executable_path); // free base path after (used by logging)

    // quit SDL (should destroy anything else i forget)
    SDL_Quit();
}