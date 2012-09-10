#ifndef MYR_H_INCLUDED
#define MYR_H_INCLUDED

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

#define g_new(struct_type, n_structs)		\
((struct_type *) malloc (( sizeof (struct_type)) * (n_structs)))

#define g_new0(struct_type, n_structs)		\
((struct_type *) calloc ((n_structs), ( sizeof (struct_type))))

#define g_renew(struct_type, mem, n_structs)	\
((struct_type *) realloc ((mem), (sizeof (struct_type)) * (n_structs)))

#define g_free( pointer ) free( pointer )

// ===============================================================
// Math (math.c)
// ===============================================================
typedef struct { float x,y,z  ; }  GVec;
typedef struct { float x,y,z,w; }  GVec4;
typedef struct { short s,t    ; }  GVec2s;
typedef struct { float s,t    ; }  GVec2;
typedef struct { GVec v[3]    ; }  GMat3;
typedef struct { GVec4 v[4]   ; }  GMat4;
typedef struct { float x,y,z,w; }  GQuat;
typedef struct { GQuat q, d;    }  GDualQuat;

//GMat4
void g_mat4_identity( GMat4 *m );
void g_mat4_add( GMat4 *out, GMat4 *m1, GMat4 *m2 );
void g_mat4_mul( GMat4 *out, GMat4 *m1, GMat4 *m2 );
void g_mat4_mul_scalar( GMat4 *out, GMat4 *m1, float scalar );
void g_mat4_vec_mul( GVec *out, GMat4 *m, GVec *in );
void g_mat4_transpose( GMat4 *m );

void g_mat4_ortho( GMat4 *m, float width, float height, float znear, float zfar );
void g_mat4_persp( GMat4 *m, float fovy, float aspect, float znear, float zfar );
void g_mat4_look_at( GMat4 *m, GVec *eye, GVec *target, GVec *up );
void g_mat4_from_quat_vec( GMat4 *m, GQuat *q, GVec *v );

//GVec
void g_vec_add( GVec *r, GVec *u, GVec *v );
void g_vec_sub( GVec *r, GVec *u, GVec *v );
void g_vec_mul_scalar( GVec *r, GVec *u, float scalar );
float g_vec_dot( GVec *v0, GVec *v1 );
void g_vec_cross( GVec *r, GVec *u, GVec *v );
void g_vec_lerp( GVec *r, GVec *a, GVec *b, float weight );
void g_vec_scale_add( GVec *r, GVec *q, float sc );

float g_vec_mag( GVec *v );
float g_vec_dist( GVec *v0, GVec *v1 );
void g_vec_normalize( GVec *v );

// GQuat
void g_quat_identity( GQuat *q );
void g_quat_invert( GQuat *q );
void g_quat_normalize( GQuat *q );
void g_quat_mul( GQuat *r, GQuat *q1, GQuat *q2 );

void g_quat_scale_add( GQuat *r, GQuat *q, float sc );
void g_quat_vec_mul( GVec *r, GQuat *q, GVec *v );

void g_quat_from_axis_angle( GQuat *q, GVec *axis, float ang );
void g_quat_from_mat4( GQuat* r, GMat4* m );

// GDualQuat
void g_dual_quat_invert( GDualQuat* r, GDualQuat* d );
void g_dual_quat_normalize( GDualQuat* d );
void g_dual_quat_mul( GDualQuat* r, GDualQuat* d1, GDualQuat* d2 );
void g_dual_quat_vec_mul( GVec* r, GDualQuat* dq, GVec* v );
void g_dual_quat_from_quat_vec( GDualQuat* r, GQuat* q, GVec* v );
void g_dual_quat_scale_add( GDualQuat* r, GDualQuat* dq, float s );
void g_dual_quat_lerp( GDualQuat* r, GDualQuat* d1, GDualQuat* d2, float t );


// ===============================================================
// Camera and Culling (camera.c)
// ===============================================================
typedef struct _GCamera {
    int build_frustum; // set to 0 if the camera will not be used for frustum culling
    GMat4 view, proj;

    float heading, pitch, offset;

    GVec eye, target, target_up;
    GQuat orientation;
    GVec4 frustum[6];
} GCamera;

void g_camera_create( GCamera* cam );
void g_camera_set_persp( GCamera* cam, float nearplane, float farplane, float fov, float ar );
void g_camera_set_ortho( GCamera* cam, float width, float height, float nearplane, float farplane );
void g_camera_look_at( GCamera* cam, GVec *eye, GVec *target, GVec *up);
void g_camera_update( GCamera* cam, int millis );

enum{ G_OUTSIDE, G_INSIDE, G_PARTIAL_INSIDE };
int g_camera_frustum_test( GCamera* cam, GVec* pt, float radius );


// ===============================================================
// Model and Collision (model.c)
// ===============================================================
typedef struct _GModel GModel;

GModel* g_model_load( const char* filename );
void g_model_destroy( GModel* mdl );
void g_model_draw( GModel* mdl, float frame );


// ===============================================================
// Texture and Font loading (assets.c)
// ===============================================================
typedef struct {
    GLuint id, bpp;
    GLint width, height;
} GTexture;
typedef struct _GFont GFont;

int g_texture_load( GTexture *t, const char *filename );
// destroy with glDeleteTex()

GFont* g_font_new (char *filename);
void g_font_render( GFont *fnt, char *str );
// destroy with g_free();

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
