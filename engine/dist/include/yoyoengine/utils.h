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

/**
    @file utils.h
    @brief Provides some utility functions for the engine.
*/

#ifndef UTILS_H
#define UTILS_H

#include <SDL2/SDL.h>

#include <yoyoengine/logging.h>

/**
 * @brief An enum that defines the alignment of an entity within its bounds.
*/
enum ye_alignment {
    YE_ALIGN_TOP_LEFT,  YE_ALIGN_TOP_CENTER,    YE_ALIGN_TOP_RIGHT,
    YE_ALIGN_MID_LEFT,  YE_ALIGN_MID_CENTER,    YE_ALIGN_MID_RIGHT,
    YE_ALIGN_BOT_LEFT,  YE_ALIGN_BOT_CENTER,    YE_ALIGN_BOT_RIGHT,
    YE_ALIGN_STRETCH
};

struct ye_rectf; // this forward declaration ignores a bunch of compiler warnings, and i have no clue why

/**
 * @brief Clamps a value between a minimum and maximum.
 * 
 * @param value The value to clamp.
 * @param min The minimum value.
 * @param max The maximum value.
 * @return int The clamped value.
 */
int ye_clamp(int value, int min, int max);

/**
 * @brief Aligns a rectangle within another rectangle (by modifying the passed values).
 * 
 * @param bounds_f The bounds to align within.
 * @param obj_f The object to align.
 * @param alignment The alignment to use.
 * @param center The center point of the object (to be written to).
 */
void ye_auto_fit_bounds(struct ye_rectf* bounds_f, struct ye_rectf* obj_f, enum ye_alignment alignment, SDL_Point* center);

/**
 * @brief Returns the size of a texture as an SDL_Rect
 * 
 * @param pTexture The texture to query for its actual size
 * @return SDL_Rect The returned size as an SDL_Rect
 */
SDL_Rect ye_get_real_texture_size_rect(SDL_Texture *pTexture);

/**
 * @brief A rectangle made up of floats.
 */
struct ye_rectf {
    float x, y, w, h;
};

/**
 * @brief Convert a floating rectangle to an integer rectangle.
 * 
 * @param rect The floating rectangle to convert.
 * @return SDL_Rect The converted rectangle.
 */
SDL_Rect ye_convert_rectf_rect(struct ye_rectf rect);

/**
 * @brief Convert an integer rectangle to a floating rectangle.
 * 
 * @param rect The integer rectangle to convert.
 * @return struct ye_rectf The converted rectangle.
 */
struct ye_rectf ye_convert_rect_rectf(SDL_Rect rect);

/**
 * @brief A collection of enums that define the different types of components.
 */
enum ye_component_type {
    YE_COMPONENT_TRANSFORM,
    YE_COMPONENT_RENDERER,
    YE_COMPONENT_PHYSICS,
    YE_COMPONENT_COLLIDER,
    YE_COMPONENT_LUA_SCRIPT,
    YE_COMPONENT_AUDIOSOURCE,
    YE_COMPONENT_CAMERA,
    YE_COMPONENT_TAG
};

/**
 * @brief Returns the position of a component on an entity.
 * 
 * Passing YE_COMPONENT_TRANSFORM will return the position of the entity transform, whereas passing
 * a component type will take into account the position of that component as a relative or absolute
 * position.
 * 
 * Ex: if you have a renderer on a entity, but the renderer is absolute at (100, 100), and the entity
 * transform is at (200, 200), then passing YE_COMPONENT_RENDERER will return (200, 200), whereas
 * passing YE_COMPONENT_TRANSFORM will return (100, 100).
 * If that same renderer was relative, then passing YE_COMPONENT_RENDERER would return (200, 200).
 * 
 * @param entity The entity to get the position of.
 * @param type The type of component to get the position of.
 * @return struct ye_rectf The position of the component.
 */
struct ye_rectf ye_get_position(struct ye_entity *entity, enum ye_component_type type);

/**
 * @brief Wrapper for @ref ye_get_position that returns an SDL_Rect instead of a ye_rectf.
 * 
 * @param entity The entity to get the position of.
 * @param type The type of component to get the position of.
 * @return SDL_Rect The position of the component.
 */
SDL_Rect ye_get_position_rect(struct ye_entity *entity, enum ye_component_type type);

#endif