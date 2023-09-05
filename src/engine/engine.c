/*
    TODO: ENGINE
    - (maybe) seperate some prinf output feedbacks into debug only
    - run every file in gpt4 and ask for issues
    - update all header files with the comment descriptions of the functions
*/

#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#include <yoyoengine/yoyoengine.h>

// engine version to be displayed on splash screen
char * engine_version = "v0.0.1 dev";

// buffer to hold filepath strings
// will be modified by getPath()
char path_buffer[1024];

// get the base path
char *base_path = NULL;

// initialize engine internal variable globals to NULL
SDL_Color *pEngineFontColor = NULL;
TTF_Font *pEngineFont = NULL;

// console overlay state controller
bool consoleOverlay = false;

// helper function to get the screen size
// TODO: consider moving graphics.c TODO: yes move to graphics.c
struct ScreenSize getScreenSize(){
    // initialize video
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        logMessage(error, "SDL could not initialize!\n");
        exit(1);
    }

    // use video to initialize display mode
    SDL_DisplayMode displayMode;
    if (SDL_GetCurrentDisplayMode(0, &displayMode) != 0) {
        logMessage(error, "SDL_GetCurrentDisplayMode failed!\n");
        exit(1);
    }

    int screenWidth = displayMode.w;
    int screenHeight = displayMode.h;
    
    char buffer[100];
    snprintf(buffer, sizeof(buffer),  "Inferred screen size: %dx%d\n", screenWidth, screenHeight);
    logMessage(debug, buffer);

    // return a ScreenSize struct with the screen width and height
    struct ScreenSize screenSize = {screenWidth, screenHeight};
    return screenSize;
}

// Global variables for resource paths
static const char *resources_path = NULL;
static const char *engine_resources_path = NULL;
char *executable_path = NULL;

char* getResourceStatic(const char *sub_path) {
    static char resource_buffer[256];  // Adjust the buffer size as per your requirement

    if (resources_path == NULL) {
        logMessage(error, "Resource paths not set!\n");
        return NULL;
    }

    snprintf(resource_buffer, sizeof(resource_buffer), "%s/%s", resources_path, sub_path);
    return strdup(resource_buffer);
}

char* getEngineResourceStatic(const char *sub_path) {
    static char engine_reserved_buffer[256];  // Adjust the buffer size as per your requirement

    if (engine_resources_path == NULL) {
        logMessage(error, "Engine reserved paths not set!\n");
        return NULL;
    }

    snprintf(engine_reserved_buffer, sizeof(engine_reserved_buffer), "%s/%s", engine_resources_path, sub_path);
    return strdup(engine_reserved_buffer);
}

/*
    get dynamically allocated path which must be freed
    TODO: REMOVEME PLEASE/REPLACEME
*/
char *getPathDynamic(const char *path) {
    if (base_path == NULL) {
        base_path = SDL_GetBasePath();
        if (base_path == NULL) {
            logMessage(error, "Error getting base path!\n");
            return NULL;
        }
    }

    size_t buffer_size = strlen(base_path) + strlen("../../resources/") + strlen(path) + 1;
    char *path_buffer = malloc(buffer_size);
    if (path_buffer == NULL) {
        logMessage(error, "Error allocating memory for path!\n");
        return NULL;
    }

    snprintf(path_buffer, buffer_size, "%s../../resources/%s", base_path, path);
    return path_buffer;
}

void toggleConsole(){
    if(consoleOverlay){
        logMessage(debug, "Toggled Console Overlay Off.\n");
        removeRenderObject(-901);
        removeRenderObject(-902);
    }
    else{
        logMessage(debug, "Toggled Console Overlay On.\n");

        // add back panel to debug overlay
        struct textureInfo info = createImageTexture(getEngineResourceStatic("dimpanel.png"),false);

        renderObject panel = {
            -901, // we set ID each time
            900,
            renderType_Image,
            info.pTexture,
            createRealPixelRect(false,0,.9,.4,.1), // dummy rect
            NULL, 
            true, // cache this (eventually between scene loads we will want to keep it, actually we might already)
            createRealPixelRect(false,0,.9,.4,.1),
            false, // not centered
            .ImageData = {
                "images/ui/dimpanel.png"
            }
        };
        
        addRenderObject(panel);

        renderObject text = {
            -902, // we set ID each time
            901,
            renderType_Text,
            createTextTexture("console init...",pEngineFont,pEngineFontColor), // we set texture each time
            createRealPixelRect(false,0,.9,.4,.1), // dummy rect
            NULL, 
            false, // no cache
            createRealPixelRect(false,0,.9,.4,.1),
            false, // not centered
            ALIGN_MID_LEFT,
            .TextData = {
                pEngineFont,
                0,
                pEngineFontColor,
                NULL, // no outline color
                ">",
            }
        };
        
        addRenderObject(text);
        updateText(-902,">");
    }
    consoleOverlay = !consoleOverlay;
}

// some functions to apply a value if its uninitialized /////////////////////

int applyDefaultInt(int value, int defaultValue) {
    if (value != 0) {
        return value;
    }
    return defaultValue;
}

bool applyDefaultBool(bool value, bool defaultValue) {
    if (value) {
        return value;
    }
    return defaultValue;
}

char* applyDefaultString(char* value, char* defaultValue) {
    if (value != NULL && strlen(value) > 0) {
        return value;
    }
    return (char*)defaultValue; // Cast away const for default value
}

/////////////////////////////////////////////////////////////////////////////

// combines two paths together (sprintf's to result buffer)
void constructPath(char *result, size_t result_size, const char *base_path, const char *supplied_path, const char *default_path) {
    if (supplied_path) {
        snprintf(result, result_size, "%s%s", base_path, supplied_path);
    } else {
        strncpy(result, default_path, result_size);
    }
}

/*
    Potentially clever thinking by me - just set defaults for non overridden values
    For booleans, we can completely skip this because they are initialized to false and need to be overriden to true
    (potential issue if we want it to default to true)
*/
void configute_defaults(struct engine_data *data){
    if(!data->override_screen_width)
        data->screen_width = 1920;
    if(!data->override_screen_height)
        data->screen_height = 1080;
    if(!data->override_volume)
        data->volume = 0;
    if(!data->override_window_mode)
        data->window_mode = 0;
    if(!data->override_framecap)
        data->framecap = -1;
    if(!data->override_log_level)
        data->log_level = 0;
    if(!data->override_window_title)
        data->window_title = "Yoyo Engine Window";
    // we already have a ternery checking for uninitialized icon path
    // game/engine resource paths are handed in init engine... move into here?
}

/*
    Pass in a engine_data struct, with cooresponding override flags to initialize the engine with non default values
*/
void initEngine(struct engine_data data) {
    // Get the path to our executable
    executable_path = SDL_GetBasePath(); // Don't forget to free memory later

    // ----------------- Default Checks ----------------

    // Set default paths for engineResourcesPath and gameResourcesPath
    char engine_default_path[256], game_default_path[256];
    snprintf(engine_default_path, sizeof(engine_default_path), "%sengine_resources", executable_path);
    snprintf(game_default_path, sizeof(game_default_path), "%sresources", executable_path);

    // Construct paths
    char engine_supplied_path[256], game_supplied_path[256];
    constructPath(engine_supplied_path, sizeof(engine_supplied_path), executable_path, data.engine_resources_path, engine_default_path);
    constructPath(game_supplied_path, sizeof(game_supplied_path), executable_path, data.game_resources_path, game_default_path);

    // check overrides to configure uninitialized fields to defaults
    configute_defaults(&data);

    // Apply defaults using helper functions --- COMPILER WILL INITIALIZE FIELDS TO ZERO: if the value can be zero then we dont bother applying a zero check (apply default int)
    int screenWidth = data.screen_width;
    int screenHeight = data.screen_height;
    int volume = data.volume;
    int windowMode = data.window_mode;
    int framecap = data.framecap;
    int logLevel = data.log_level;
    bool debugMode = data.debug_mode;
    bool skipIntro = data.skipintro;
    char *windowTitle = data.window_title;

    // Update global locations for resources
    engine_resources_path = strdup(engine_supplied_path); // Remember to free this memory later
    resources_path = strdup(game_supplied_path); // Remember to free this memory later

    // Get the icon path
    char *iconPath = data.icon_path ? getResourceStatic(data.icon_path) : getEngineResourceStatic("enginelogo.png");

    // ----------------- Begin Setup -------------------

    // initialize graphics systems, creating window renderer, etc
    initGraphics(screenWidth,screenHeight,windowMode,framecap,windowTitle,iconPath);

    // load a font for use in engine (value of global in engine.h modified) TODO: this will break
    pEngineFont = loadFont(getEngineResourceStatic("RobotoMono-Light.ttf"), 500);

    // allocate memory for and create a pointer to our engineFontColor struct for use in graphics.c
    // TODO: check this later because i'm so tired and perplexed with this workaround to letting the fn go out of scope
    SDL_Color engineFontColor = {255, 255, 0, 255};
    pEngineFontColor = &engineFontColor;
    pEngineFontColor = malloc(sizeof(SDL_Color));
    pEngineFontColor->r = 255;
    pEngineFontColor->g = 255;
    pEngineFontColor->b = 0;
    pEngineFontColor->a = 255;

    // no matter what we will initialize log level with what it should be. default is nothing but dev can override
    log_init(logLevel);

    // if we are in debug mode
    if(debugMode){
        // display in console
        logMessage(debug, "Debug mode enabled.\n");
        // TODO: overlay and maybe log level adjust?
    }

    // startup audio systems
    initAudio();

    // before we play our loud ass startup sound, lets check what volume the game wants
    // the engine to be at initially
    setVolume(-1, volume);

    /*
        Part of the engine startup which isnt configurable by the game is displaying
        a splash screen with the engine title and logo for 2550ms and playing a
        startup noise
    */
    if(skipIntro){
        logMessage(info,"Skipping Intro.\n");
    }
    else{
        playSound(getEngineResourceStatic("startup.mp3"),0,0); // play startup sound

        // add startup splash image
        createImage(0,.5f,.5f,1.0f,1.0f,getEngineResourceStatic("splash.png"),true,ALIGN_STRETCH);

        createText(1,.5,.95,.1,.1,engine_version,pEngineFont,&engineFontColor,true,ALIGN_MID_CENTER);
        createText(1,.5,.98,.1,.1,"Ryan Zmuda 2023",pEngineFont,&engineFontColor,true,ALIGN_MID_CENTER);

        // render everything in engine queue
        renderAll(); 

        // pause on engine splash for 2550ms (TODO: consider alternatives)
        // SDL_Delay(3000); maybe a more reasonable time scale?
        SDL_Delay(2550);
        
        // remove startup objects
        clearAll(false);
    }

    // render everything in engine queue after splash asset removal
    renderAll();

    lua_init(); // initialize lua
    logMessage(debug, "Initialized Lua.\n");

    // debug output
    logMessage(info, "Engine Fully Initialized.\n");
} // control is now resumed by the game

// function that shuts down all engine subsystems and components ()
void shutdownEngine(){
    logMessage(info, "Shutting down engine...\n");

    // shutdown lua
    lua_shutdown();
    logMessage(info, "Shut down lua.\n");

    // free the engine font color
    free(pEngineFontColor);
    pEngineFontColor = NULL;

    // shutdown graphics
    shutdownGraphics();
    logMessage(info, "Shut down graphics.\n");

    // shutdown audio
    shutdownAudio();
    logMessage(info, "Shut down audio.\n");

    // shutdown logging
    // note: must happen before SDL because it relies on SDL path to open file
    log_shutdown();
    SDL_free(base_path); // free base path after (used by logging)
    SDL_free(executable_path); // free base path after (used by logging)

    // quit SDL (should destroy anything else i forget)
    SDL_Quit();
}