#include "myr.h"

GVec zero   = { 0.0f, 0.0f, 0.0f };
GVec x_axis = { 1.0f, 0.0f, 0.0f };
GVec y_axis = { 0.0f, 1.0f, 0.0f };
GVec z_axis = { 0.0f, 0.0f, 1.0f };

void g_camera_create( GCamera* cam ) {
  if(!cam) return;

  g_mat4_identity( &cam->view );
  g_mat4_identity( &cam->proj );
  g_quat_identity( &cam->orientation );

  cam->target_up = z_axis;

  cam->eye = zero;
  cam->target = zero;

  cam->offset  = 0.0;
  cam->heading = 0.0;
  cam->pitch   = 0.0;
  cam->build_frustum = 1;
}

static void g_camera_build_frustum( GCamera* cam ); // forward decl

void g_camera_set_persp( GCamera* cam, float nearplane, float farplane, float fov, float ar ) {
  if( cam ) g_mat4_persp( &cam->proj, fov, ar, nearplane, farplane );
}

void g_camera_set_ortho( GCamera* cam, float width, float height, float nearplane, float farplane ) {
  if( cam ) g_mat4_ortho( &cam->proj, width, height, nearplane, farplane );
}

void g_camera_look_at( GCamera* cam, GVec *eye, GVec *target, GVec *up) {
    cam->eye = *eye;
    cam->target = *target;
    cam->target_up = *up;

    g_mat4_look_at( &cam->view, eye, target, up );
    g_quat_from_mat4( &cam->orientation, &cam->view );

    GVec offset;
    g_vec_sub( &offset, target, eye );
    cam->offset = g_vec_mag( &offset );

    if( cam->build_frustum ) g_camera_build_frustum( cam );
}

void g_camera_update( GCamera* cam, int millis ) {
    if( !cam ) return;
    // update Orientation for the elapsed milliseconds
    cam->pitch   *= millis/1000.0;
    cam->heading *= millis/1000.0;

    GQuat rot;
    if( cam->heading != 0.0f ) {
        g_quat_from_axis_angle( &rot, &cam->target_up, cam->heading );
        g_quat_mul( &cam->orientation, &cam->orientation, &rot );
    }
    if( cam->pitch != 0.0f ) {
        g_quat_from_axis_angle( &rot, &x_axis, cam->pitch );
        g_quat_mul( &cam->orientation, &rot, &cam->orientation );
    }

    // update the view matrix
    g_mat4_from_quat_vec( &cam->view, &cam->orientation, &zero );

    GVec x_vec = { cam->view.v[0].x, cam->view.v[1].x, cam->view.v[2].x };
    GVec y_vec = { cam->view.v[0].y, cam->view.v[1].y, cam->view.v[2].y };
    GVec z_vec = { cam->view.v[0].z, cam->view.v[1].z, cam->view.v[2].z };

    GVec tmp;
    g_vec_mul_scalar( &tmp, &z_vec, cam->offset );
    g_vec_add( &cam->eye, &cam->target, &tmp );

    cam->view.v[3].x = -g_vec_dot( &x_vec, &cam->eye );
    cam->view.v[3].y = -g_vec_dot( &y_vec, &cam->eye );
    cam->view.v[3].z = -g_vec_dot( &z_vec, &cam->eye );

    if( cam->build_frustum ) g_camera_build_frustum( cam );
}

//
// For frustum culling
//
static void normalize_plane( GVec4* p ){
    float mag = sqrtf( p->x*p->x + p->y*p->y + p->z*p->z );
    p->x /= mag;
    p->y /= mag;
    p->z /= mag;
    p->w /= mag;
}

static void g_camera_build_frustum( GCamera* cam ){
    GMat4 m;
    g_mat4_mul( &m, &cam->proj, &cam->view );

    // Extract the right plane & Normalize the result
    cam->frustum[0].x = m.v[0].w - m.v[0].x;
    cam->frustum[0].y = m.v[1].w - m.v[1].x;
    cam->frustum[0].z = m.v[2].w - m.v[2].x;
    cam->frustum[0].w = m.v[3].w - m.v[3].x;
    normalize_plane( &cam->frustum[0] );

    // Extract the left plane & Normalize the result
    cam->frustum[1].x = m.v[0].w + m.v[0].x;
    cam->frustum[1].y = m.v[1].w + m.v[1].x;
    cam->frustum[1].z = m.v[2].w + m.v[2].x;
    cam->frustum[1].w = m.v[3].w + m.v[3].x;
    normalize_plane( &cam->frustum[1] );

    // Extract the bottom plane & Normalize the result
    cam->frustum[2].x = m.v[0].w + m.v[0].y;
    cam->frustum[2].y = m.v[1].w + m.v[1].y;
    cam->frustum[2].z = m.v[2].w + m.v[2].y;
    cam->frustum[2].w = m.v[3].w + m.v[3].y;
    normalize_plane( &cam->frustum[2] );

    // Extract the top plane & Normalize the result
    cam->frustum[3].x = m.v[0].w - m.v[0].y;
    cam->frustum[3].y = m.v[1].w - m.v[1].y;
    cam->frustum[3].z = m.v[2].w - m.v[2].y;
    cam->frustum[3].w = m.v[3].w - m.v[3].y;
    normalize_plane( &cam->frustum[3] );

    // Extract the far plane & Normalize the result
    cam->frustum[4].x = m.v[0].w - m.v[0].z;
    cam->frustum[4].y = m.v[1].w - m.v[1].z;
    cam->frustum[4].z = m.v[2].w - m.v[2].z;
    cam->frustum[4].w = m.v[3].w - m.v[3].z;
    normalize_plane( &cam->frustum[4] );

    // Extract the near plane & Normalize the result
    cam->frustum[5].x = m.v[0].w + m.v[0].z;
    cam->frustum[5].y = m.v[1].w + m.v[1].z;
    cam->frustum[5].z = m.v[2].w + m.v[2].z;
    cam->frustum[5].w = m.v[3].w + m.v[3].z;
    normalize_plane( &cam->frustum[5] );
}

int g_camera_frustum_test( GCamera* cam, GVec* pt, float radius ) {
    int i, j=0;
    float distance;

    for ( i=0; i<6; i++ ) {
        distance = cam->frustum[i].x*pt->x + cam->frustum[i].y*pt->y +
                    cam->frustum[i].z*pt->z + cam->frustum[i].w;

        if( distance <= -radius ) return G_OUTSIDE;
        if( distance > radius ) j++;
   }

   return (j == 6) ? G_INSIDE : G_PARTIAL_INSIDE;
}
