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

typedef struct  { float x, y, z; } Vector;
typedef struct  { Vector p, r, u, b; } Camera;


#define OCTREE_SIZE 10
typedef struct Grid Grid;
typedef struct Cell Cell;

typedef struct {
    Vector p, n;
} Vertex;

typedef struct {
    float r, g, b, a;
    Vertex *v1, *v2, *v3;
} Triangle;

typedef struct {
    int vn;
    Vertex **v;

    int tn;
    Triangle **t;

    float rr; // squared bounding sphere radius, with position as center
    // Grid *grid; // An octree to speed up finding of triangles for collision.
} Mesh;

typedef struct {
    Mesh *mesh;
    Camera camera;

    int hits;
} Object;

typedef struct {
    int on;
    Object **o;
} World;


typedef struct {
    Camera camera;
    Vector hitpoint;
    Vector footpoint;
    Object *hit;

    float health;
} Player;

typedef struct {
    Object *object;
    Vector velocity;
    int tick;
    int hitpoints;

    Vector avoid;
    int avoid_time;

    float speed;
    int finished;
} Monster;



//math.c
Vector vector_add(Vector a, Vector b);
Vector vector_sub(Vector a, Vector b);
Vector vector_scale(Vector a, float s);
float vector_dot(Vector a, Vector b);
Vector vector_cross(Vector a, Vector b);
float vector_len(Vector a);
Vector vector_normalize(Vector a);

Vector vector_transform(Vector a, Camera *camera);
Vector vector_backtransform(Vector a, Camera *camera);
Vector vector_rotate(Vector a, Vector b, float angle);

int ray_intersects_triangle(Vector pos, Vector dir, Vector v0, Vector v1, Vector v2, Vector *result);
int line_intersects_triangle(Vector pos1, Vector pos2, Vector v0, Vector v1, Vector v2, Vector *result);

void camera_turn(Camera *camera, float a);
void camera_pitch(Camera *camera, float a);
void camera_yaw(Camera *camera, float a);
void camera_roll(Camera *camera, float a);

//mesh_data.c
Mesh *landscape_mesh(void);
Mesh *monster_mesh(void);
Mesh *monster2_mesh(void);

// world.c
Vertex *vertex_new(float x, float y, float z, float nx, float ny, float nz);
Triangle *triangle_new(Vertex *v1, Vertex *v2, Vertex *v3, float r, float g, float b, float a);
World *world_new(void);
void world_add_object(World *self, Object *object);
Object *object_new(Vector pos, Mesh *mesh);
void triangle_draw(Triangle *self);
void mesh_draw(Mesh *self);
void object_draw(Object *self);
void world_draw(World *self, Camera *camera);
int mesh_collision(Mesh *self, Vector pos, Vector dir, Vector *result);
int object_collision(Object *o, Vector pos, Vector dir, Vector *result);
Object *world_collision(World *self, Vector pos, Vector dir, Vector *result);
void mesh_calculate_bounding_sphere(Mesh *self);
// void mesh_create_grid(Mesh *self);
void apply_camera(Camera *camera);
void apply_orientation(Camera *camera);

// // octree.c
// int in_cell(Cell *self, Vector pos);
// int cell_has_triangle(Cell *self, Triangle *t);
// void cell_add_triangle(Cell *self, Triangle *t);
// void cell_subdivide(Grid *grid, Cell *self);
// Cell *cell_new(Grid *grid, Cell *parent, float x1, float x2, float y1, float y2, float z1, float z2);
// Grid *grid_new(Mesh *mesh, float s);
// Cell *cell_get_cell(Cell *self, Vector pos);
// Cell *grid_get_cell(Grid *self, Vector pos);
// void cell_draw_debug_colored(Cell *self);
// void cell_draw_debug(Cell *self);
// int grid_collision_recurse(Grid *self, Cell *cell, Vector pos, Vector dir, Vector *result);
// int grid_collision(Grid *self, Vector pos, Vector dir, Vector *result);
// void grid_draw_debug(Grid *self, Vector pos, Vector dir);



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
