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
( (struct_type *) malloc((sizeof(struct_type)) * (n_structs)) )

#define g_new0(struct_type, n_structs) \
( (struct_type *) calloc((n_structs), (sizeof(struct_type))) )

#define g_renew(struct_type, mem, n_structs)  \
( (struct_type *) realloc((mem), (sizeof(struct_type)) * (n_structs)) )

#define g_free( pointer ) free( pointer )

//
//math.c
//
typedef struct { float m[16];    } Mat4;
typedef struct { float x, y, z;  } Vec;
typedef struct { short s, t;     } Vec2s;
typedef struct { float s, t;     } Vec2;
typedef struct { float x,y,z,w;  } Vec4;
typedef struct { float x,y,z,w;  } Quat;

void mat4_identity( Mat4 *mat );
void mat4_mul( Mat4 *out, const Mat4 *m1, const Mat4 *m2 );
void mat4_ortho( Mat4 *mat, float width, float height, float znear, float zfar );
void mat4_persp( Mat4 *mat, float fovy, float aspect, float znear, float zfar );
void mat4_look_at( Mat4 *mat, const Vec *eye, const Vec *target, const Vec *up );
void mat4_from_quat_vec( Mat4 *mat, const Quat *q, const Vec *v );

Vec* vec_add( Vec* r, const Vec* a, const Vec* b );
Vec* vec_sub( Vec* r, const Vec* a, const Vec* b );
Vec* vec_scale( Vec* r, const Vec* a, float s );
Vec* vec_cross( Vec* r, const Vec* a, const Vec* b );
Vec* vec_normalize( Vec* r, const Vec* a );
Vec* vec_rotate( Vec* r, const Vec* a, const Vec* b, float angle );
float vec_dot( const Vec* a, const Vec* b );
float vec_len( const Vec* a );

Quat* quat_invert( Quat *r, const Quat *q );
Quat* quat_normalize( Quat *r, const Quat *q );
Quat* quat_mul( Quat *r, const Quat *q1, const Quat *q2 );
Quat* quat_from_axis_angle( Quat *q, const Vec *axis, float ang );
Quat* quat_from_mat4( Quat* q, const Mat4* m );
Vec* quat_vec_mul( Vec *r, const Quat *q, const Vec *v );

static const Vec zero   = { 0.0, 0.0, 0.0 };
static const Vec x_axis = { 1.0, 0.0, 0.0 };
static const Vec y_axis = { 0.0, 1.0, 0.0 };
static const Vec z_axis = { 0.0, 0.0, 1.0 };
static const Vec neg_z_axis = {0.0f, 0.0f, -1.0f};

//
// assets.c
//
typedef struct _Font TexFont;
typedef struct {
    GLuint id, bpp;
    GLint width, height;
} Texture;

typedef struct {
    GLuint vs, fs, object;
    GLint u_mvp, u_sampler; // uniforms
} Program;

int texture_load( Texture *tex, const char* filename );
TexFont* tex_font_new (char *filename);
void tex_font_render( TexFont *fnt, char *str );
GLuint program_load_shader( const GLchar *src, GLenum type );
int program_link( Program *program, const char **attribs );

//
// model.c
//
typedef struct _Model Model;

Model* model_load( const char *filename );
void model_destroy( Model* mdl );
void model_draw( Model* mdl, Program* program, Mat4* mvp, float frame );
int model_collision( Model *mdl, Vec* pos, Vec* dir, float radius, Vec *result );
float model_calculate_bounding_sphere( Model *mdl );

//
// collision.c - low level collission functions
//
typedef struct {
    Vec start, scaled_start, intersect_point;
    Vec vel, scaled_vel, norm_vel;
    float t, radius, inv_radius;
    int collision;
} TraceInfo;

void trace_init( TraceInfo* trace, Vec* start, Vec* vel, float radius );
void trace_end( TraceInfo* trace, Vec* end );
float trace_dist( TraceInfo* trace );
void trace_sphere_triangle( TraceInfo* trace, Vec* p0, Vec* p1, Vec* p2 );
void trace_ray_triangle( TraceInfo* trace, Vec* v0, Vec* v1, Vec* v2 );

//
// Camera handling (based on http://dhporware.com TPS camera tutorial)
//
typedef struct {
    int spring_system; // set to 0 to disable the spring system
    Mat4 view, projection;

    float spring, damping, offset;  //for the spring system

    Vec pos, up, velocity;
    Quat rot;
} Camera;

typedef struct {
    Vec4 plane[6];
    float fovy, aspect, znear, zfar;
} Frustum;

void camera_init( Camera* cam );
void camera_look_at( Camera* cam, const Vec *eye, const Vec *target, const Vec *up);
void camera_update( Camera* cam, const Vec* target, float pitch, float heading, int millis );

//
// world.c
//
typedef struct {
    Model *model;
    Program program;
    float rr; //squared radius
    Vec pos;
    Quat rot;

    int hits;
    int type;
} Object;

typedef struct {
    int on;
    Object **o;
} World;

World *world_new( void );
void world_add_object( World *self, Object *object );
void world_draw( World *self, Camera *camera );
Object *world_collision( World *self, Vec* pos, Vec* dir, float radius, Vec *result );

Object *object_new( Vec pos, Model *mdl, Program* program );
void object_draw( Object* self, Mat4* vp );
Vec* object_transform( Vec* r, Vec* a, Object* o );
Vec* object_back_transform( Vec* r, Vec* a, Object* o );
int object_collision( Object *o, Vec* pos, Vec* dir, float radius, Vec *result );


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
