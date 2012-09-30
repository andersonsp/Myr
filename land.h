#ifndef LAND_H_INCLUDED
#define LAND_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>

#include "gl/glext.h"
#include "gl_extensions.h"


//
// Useful Macros
//
#define G_PI 3.14159265358979323

#define g_radians( x ) ((x*G_PI)/180)
#define g_degrees( x ) ((x*180)/G_PI)

#define g_new(struct_type, n_structs)   \
((struct_type *) malloc (( sizeof (struct_type)) * (n_structs)))

#define g_new0(struct_type, n_structs)    \
((struct_type *) calloc ((n_structs), ( sizeof (struct_type))))

#define g_renew(struct_type, mem, n_structs)  \
((struct_type *) realloc ((mem), (sizeof (struct_type)) * (n_structs)))

#define g_free( pointer ) free( pointer )


//

typedef struct { float x, y, z;  } Vec;
typedef struct { short s, t;     } Vec2s;
typedef struct { float s, t;     } Vec2;
typedef struct { float x,y,z,w;  } Vec4;
typedef struct { Vec p, r, u, b; } Camera;
typedef struct { float x,y,z,w;  } Quat;
typedef struct { Quat q, d;      } DualQuat;

#define OCTREE_SIZE 10

typedef struct _Model Model;

typedef struct {
    GLuint id, bpp;
    GLint width, height;
} Texture;

typedef struct {
    Model *model;
    float rr; //squared radius
    Camera camera;

    int hits;
} Object;

typedef struct {
    int on;
    Object **o;
} World;


typedef struct {
    Camera camera;
    Vec hitpoint;
    Vec footpoint;
    Object *hit;

    float health;
} Player;

typedef struct {
    Object *object;
    Vec velocity;
    int tick;
    int hitpoints;

    Vec avoid;
    int avoid_time;

    float speed;
    int finished;
} Monster;


//math.c
Vec vec_add( Vec a, Vec b );
Vec vec_sub( Vec a, Vec b );
Vec vec_scale( Vec a, float s );
float vec_dot( Vec a, Vec b );
Vec vec_cross( Vec a, Vec b );
float vec_len( Vec a );
Vec vec_normalize( Vec a );

Vec vec_transform( Vec a, Camera *camera );
Vec vec_backtransform( Vec a, Camera *camera );
Vec vec_rotate( Vec a, Vec b, float angle );

int ray_intersects_triangle( Vec pos, Vec dir, Vec v0, Vec v1, Vec v2, Vec *result );
int line_intersects_triangle( Vec pos1, Vec pos2, Vec v0, Vec v1, Vec v2, Vec *result );

void camera_turn(Camera *camera, float a);
void camera_pitch(Camera *camera, float a);
void camera_yaw(Camera *camera, float a);
void camera_roll(Camera *camera, float a);

// assets.c
int texture_load( Texture *tex, const char* filename );

// model.c
Model* model_load( const char *filename );
void model_destroy( Model* mdl );
void model_draw( Model* mdl, float frame );
int model_collision( Model *mdl, Vec pos, Vec dir, Vec *result );
float model_calculate_bounding_sphere( Model *mdl );

// world.c
World *world_new(void);
void world_add_object(World *self, Object *object);
Object *object_new( Vec pos, Model *mdl );
void object_draw( Object *self );
void world_draw( World *self, Camera *camera );
int object_collision( Object *o, Vec pos, Vec dir, Vec *result );
Object *world_collision( World *self, Vec pos, Vec dir, Vec *result );
void apply_camera( Camera *camera );
void apply_orientation( Camera *camera );

// octree.c


// ===============================================================
// System (sys_*.c)
// ===============================================================

enum { //flags
    GC_MULTISAMPLING = 1,
    GC_CORE_PROFILE = 2,
    GC_FULLSCREEN = 4,
    GC_HIDE_CURSOR = 8,
    GC_VERTICAL_SYNC = 16,
    GC_IGNORE_KEYREPEAT = 32
};

typedef struct {
    char *title;
    int width, height;
    int flags, gl_version;
    void * data;
} GConfig;

enum { // event types
  GE_KEYUP,
  GE_KEYREPEAT,
  GE_KEYDOWN,
  GE_MOUSEUP,
  GE_MOUSEDOWN,
  GE_MOUSEMOVE,
  GE_MOUSERAW,
  GE_FOCUS,
  GE_BLUR, // lose focus
  GE_QUIT
};

enum { // key definitions
  GK_UNKNOWN,
  GK_Q,
  GK_W,
  GK_E,
  GK_A,
  GK_S,
  GK_D,
  GK_UP,
  GK_DOWN,
  GK_LEFT,
  GK_RIGHT,
  GK_SPACE,
  GK_RETURN,
  GK_ESCAPE,
  GK_KEY_MAX
};

typedef struct {
  int type;
  int value;
  int x, y;
  float dx, dy; //relative device movement
} GEvent;

void g_configure( GConfig *conf );                        // called to configure the context
void g_initialize( int width, int height, void *data );   // receive window size after the context is created
void g_render( void *data );                              // draw scene (it swaps the backbuffer for you)
void g_update( unsigned int milliseconds, void *data );   // receive elapsed time (e.g., update physics)
int g_handle_event( GEvent *event, void *data );          // handle incoming events if return 0 the the app will exit
void g_cleanup( void *data );                             // in case you want to clean things up

// utility functions as alternatives to printf, exceptions, and asserts.
// On Windows, strings get sent to the debugger, so use the VS debugger window or dbgview to see them.
//
void g_debug_str( const char* str, ... );
void g_fatal_error( const char* str, ... );
// void g_debug_wstr(const wchar_t* wstr, ...);
// void g_fatal_werror( const wchar_t* pStr, ... );
#define g_check_condition(A, B) if (!(A)) { g_fatal_error(B); }
#define g_check_wcondition(A, B) if (!(A)) { g_fatal_werror(B); }

#endif // MYR_H_INCLUDED
