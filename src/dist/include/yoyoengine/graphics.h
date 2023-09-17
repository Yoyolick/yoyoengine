#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdbool.h>

#include <SDL2/SDL_ttf.h>

#include <yoyoengine/yoyoengine.h>

int convertToRealPixelWidth(float in);

int convertToRealPixelHeight(float in);

SDL_Rect createRealPixelRect(bool centered, float x, float y, float w, float h);

TTF_Font *loadFont(char *pFontPath, int fontSize);

/*
    struct holding info on texture creations
    TODO: in the future will we need refcounts for arbitrary cache objects holding more than just their texture?
*/
struct textureInfo {
    SDL_Texture *pTexture;
    bool cached;
};

// Create a texture from image path, returns NULL for failure
struct textureInfo createImageTexture(char *pPath, bool shouldCache);

SDL_Texture *createTextTextureWithOutline();

SDL_Texture *createTextTexture(const char *pText, TTF_Font *pFont, SDL_Color *pColor);

TTF_Font *getFont(char *key);

SDL_Color *getColor(char *key, SDL_Color color);

void autoFitBounds(SDL_Rect* bounds, SDL_Rect* obj, Alignment alignment);

void renderAll();

void setViewport(int screenWidth, int screenHeight);

void changeWindowMode(Uint32 mode);

void changeFPS(int cap);

struct ScreenSize getCurrentResolution();

void changeResolution(int width, int height);

void initGraphics(int screenWidth,int screenHeight, int windowMode, int framecap, char *title, char *icon_path);

void shutdownGraphics();

#endif