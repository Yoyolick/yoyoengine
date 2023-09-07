#ifndef ECS_H
#define ECS_H

#include <yoyoengine/yoyoengine.h>
#include <yoyoengine/utils.h>
#include <stdbool.h>

/*
    Linked list structure for storing entities

    The intent of this is to keep lists of entities that contain
    certain components, so that systems can iterate through them
    and act upon their components.
*/
struct ye_entity_list_item {
    struct ye_entity *entity;
    struct ye_entity_list_item *next;
};

/*
    Entity
    
    An entity is a collection of components that make up a game object.
*/
struct ye_entity {
    bool active;        // controls whether system will act upon this entity and its components

    int id;             // unique id for this entity
    char *name;         // name that can also be used to access the entity
    char *tags[10];     // up to 10 tags that can also be used to access the entity

    struct ye_component_transform *transform;       // transform component
    struct ye_component_renderer *renderer;         // renderer component
    struct ye_component_script *script;             // script component
    struct ye_component_interactible *interactible; // interactible component
};

/*
    Transform component
    
    Describes where the entity sits in the world.
    In 2D the Z axis is the layer the entity sits on. (High Z overpaints low Z)
*/
struct ye_component_transform {
    bool active;    // controls whether system will act upon this component

    SDL_Rect bounds;                // original desired pixel location of entity
    enum ye_alignment alignment;    // alignment of entity within its bounds
    SDL_Rect rect;                  // real location of entity computed from desired alignment
};

/*
    Script component
    
    Holds information on a script that is attatched to an entity.
    This script will have its main loop run once per frame.
*/
struct ye_component_script {
    bool active;    // controls whether system will act upon this component

    char *script_path;
};

/*
    Enum to store different unique types of renderers.
    This is how we identify steps needed to render different types of entities.

    Ex: animation renderer knows it needs to increment frames, text renderer knows how to reconstruct text, etc
*/
enum ye_component_renderer_type {
    YE_RENDERER_TYPE_TEXT,
    YE_RENDERER_TYPE_IMAGE,
    YE_RENDERER_TYPE_ANIMATION
};

struct ye_component_renderer {
    bool active;    // controls whether system will act upon this component

    SDL_Texture *texture;   // texture to render

    // bool texture_cached;   // whether the texture is cached or not

    enum ye_component_renderer_type type;   // denotes which renderer is needed for this entity

    union { // hold the data for the specific renderer type
        struct ye_component_renderer_text *text;
        struct ye_component_renderer_image *image;
        struct ye_component_renderer_animation *animation;
    };
};

/*
    Interactible component
    
    Holds information on how an entity can be interacted with.
*/
struct ye_component_interactible {
    bool active;    // controls whether system will act upon this component

    void *data;                     // data to communicate when callback finishes
    void (*callback)(void *data);   // callback to run when entity is interacted with

    /*
        Currently, we will get the transform of an object with a
        interactible component to check bounds for clicks.

        In the future, we would want a collider_2d component with
        an assosciated type and relative position so we can more
        presicely check for interactions. I dont forsee the need
        for this for a while though. Maybe future me in a year reading
        this wishes I did it now xD
    */
};

struct ye_entity * ye_create_entity();

void ye_init_ecs();

void ye_shutdown_ecs();

// helper functions

void ye_print_entities();

#endif