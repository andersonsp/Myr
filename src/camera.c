#include "land.h"

#define DEFAULT_SPRING  1.0f;
#define DEFAULT_DAMPING 26.0f;

void camera_init( Camera* cam ) {
  if(!cam) return;

  mat4_identity( &cam->view );
  mat4_identity( &cam->projection );
  cam->pos = zero;
  cam->up = y_axis;
  cam->velocity = zero;
  cam->rot = (Quat){0.0f, 0.0f, 0.0f, 1.0f}; // quat identity

  cam->spring  = DEFAULT_SPRING;
  cam->damping = DEFAULT_DAMPING;
  cam->offset  = 0.0;
  cam->spring_system = 1;
}

void camera_look_at( Camera* cam, const Vec *eye, const Vec *target, const Vec *up) {
    cam->pos = *eye;
    cam->up = *up;

    mat4_look_at( &cam->view, eye, target, up );
    quat_from_mat4( &cam->rot, &cam->view );

    Vec offset;
    cam->offset = vec_len( vec_sub(&offset, target, eye) );
}

static void update_view_absolute( Camera* cam, const Vec* target ) {
    mat4_from_quat_vec( &cam->view, &cam->rot, &zero );

    Vec x_vec = { cam->view.m[0], cam->view.m[4], cam->view.m[8] };
    Vec y_vec = { cam->view.m[1], cam->view.m[5], cam->view.m[9] };
    Vec z_vec = { cam->view.m[2], cam->view.m[6], cam->view.m[10] };

    Vec tmp;
    vec_scale( &tmp, &z_vec, cam->offset );
    vec_add( &cam->pos, target, &tmp );

    cam->view.m[12] = -vec_dot( &x_vec, &cam->pos );
    cam->view.m[13] = -vec_dot( &y_vec, &cam->pos );
    cam->view.m[14] = -vec_dot( &z_vec, &cam->pos );
}

static void update_view_timed( Camera* cam, const Vec* target, float millis ) {
    mat4_from_quat_vec( &cam->view, &cam->rot, &zero );
    Vec z_vec = { cam->view.m[2], cam->view.m[6], cam->view.m[10] };

    // Calculate the new camera position. The 'idealPosition' is where the
    // camera should be position. The camera should be positioned directly
    // behind the target at the required offset distance. What we're doing here
    // is rather than have the camera immediately snap to the 'idealPosition'
    // we slowly move the camera towards the 'idealPosition' using a spring
    // system.
    //
    // References:
    //   Stone, Jonathan, "Third-Person Camera Navigation," Game Programming
    //     Gems 4, Andrew Kirmse, Editor, Charles River Media, Inc., 2004.

    Vec tmp, ideal_pos, displacement, spring_accel;
    vec_scale( &tmp, &z_vec, cam->offset );
    vec_add( &ideal_pos, target, &tmp );
    vec_sub( &displacement, &cam->pos, &ideal_pos );

//Vector3 springAcceleration = (-m_springConstant * displacement) - (m_dampingConstant * m_velocity);
    vec_scale( &displacement, &displacement, -cam->spring );
    vec_scale( &tmp, &cam->velocity, cam->damping );
    vec_sub( &spring_accel, &displacement, &tmp );

//m_velocity += springAcceleration * elapsedTimeSec;
    vec_scale( &spring_accel, &spring_accel, millis/1000.0 );
    vec_add( &cam->velocity, &cam->velocity, &spring_accel );

//m_eye += m_velocity * elapsedTimeSec;
    vec_scale( &tmp, &cam->velocity, millis/1000.0 );
    vec_add( &cam->pos, &cam->pos, &cam->velocity );

    // The view matrix is always relative to the camera's current position
    // 'm_eye'. Since a spring system is being used here 'm_eye' will be
    // relative to 'idealPosition'. When the camera is no longer being
    // moved 'm_eye' will become the same as 'idealPosition'. The local
    // x, y, and z axes that were extracted from the camera's orientation
    // 'm_orienation' is correct for the 'idealPosition' only. We need
    // to recompute these axes so that they're relative to 'm_eye'. Once
    // that's done we can use those axes to reconstruct the view matrix.

    mat4_look_at( &cam->view, &cam->pos, target, &cam->up );
}

void camera_update( Camera* cam, const Vec* target, float pitch, float heading, int millis ) {
    if( !cam ) return;
    // update Orientation for the elapsed milliseconds
    pitch   *= millis/1000.0;
    heading *= millis/1000.0;

    Quat rot;
    if( heading != 0.0f ) {
        quat_from_axis_angle( &rot, &cam->up, heading );
        quat_mul( &cam->rot, &cam->rot, &rot );
    }
    if( pitch != 0.0f ) {
        quat_from_axis_angle( &rot, &x_axis, pitch );
        quat_mul( &cam->rot, &rot, &cam->rot );
    }
    if( heading != 0.0f || pitch != 0.0f ) quat_normalize( &cam->rot, &cam->rot );

    // update the view matrix
    if( cam->spring_system ) update_view_timed( cam, target, millis );
    else update_view_absolute( cam, target );
}
