#include "land.h"

Vec* vec_add( Vec* r, Vec* a, Vec* b ) {
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    return r;
}

Vec* vec_sub( Vec* r, Vec* a, Vec* b ) {
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    return r;
}

Vec* vec_scale( Vec* r, Vec* a, float s ) {
    r->x = a->x * s;
    r->y = a->y * s;
    r->z = a->z * s;
    // g_debug_str( "scale: x=%f y=%f z=%f -- s=%f\n", r->x, r->y, r->z, s );
    return r;
}

Vec* vec_cross( Vec* r, Vec* a, Vec* b ) {
    r->x = a->y * b->z - a->z * b->y;
    r->y = a->z * b->x - a->x * b->z;
    r->z = a->x * b->y - a->y * b->x;
    return r;
}

Vec* vec_normalize( Vec* r, Vec* a ) {
    float mag = vec_len(a);
    r->x /= mag;
    r->y /= mag;
    r->z /= mag;
    return r;
}

float vec_dot( Vec* a, Vec* b ) {
    return a->x*b->x + a->y*b->y + a->z*b->z;
}

float vec_len( Vec* a ) {
    return (float) sqrt( a->x*a->x + a->y*a->y + a->z*a->z );
}

// Transform from world into camera coordinates.
Vec* vec_transform( Vec* r, Vec* a, Camera* camera ) {
    Vec u;
    vec_sub( &u, a, &camera->p );
    r->x = vec_dot( &u, &camera->r);
    r->y = vec_dot( &u, &camera->u);
    r->z = vec_dot( &u, &camera->b);
    return r;
}

// Transform from camera into world coordinates.
Vec* vec_backtransform( Vec* r, Vec* a, Camera* camera ) {
    Vec x, y, z;

    vec_scale( &x, &camera->r, a->x );
    vec_scale( &y, &camera->u, a->y );
    vec_scale( &z, &camera->b, a->z );

    return vec_add( r, vec_add(r, vec_add(r, &x, &y), &z), &camera->p );
}

// Rotate a around b by angle, store in result.
Vec* vec_rotate( Vec* r, Vec* a, Vec* b, float angle ) {
    float c = cosf(angle);
    float s = sinf(angle);

    Vec m1, m2, m3;
    vec_scale( &m1, b, b->x * (1 - c) );
    vec_scale( &m2, b, b->y * (1 - c) );
    vec_scale( &m3, b, b->z * (1 - c) );

    m1.x += c;
    m1.y += b->z * s;
    m1.z -= b->y * s;

    m2.x -= b->z * s;
    m2.y += c;
    m2.z += b->x * s;

    m3.x += b->y * s;
    m3.y -= b->x * s;
    m3.z += c;

    r->x = vec_dot( a, &m1 );
    r->y = vec_dot( a, &m2 );
    r->z = vec_dot( a, &m3 );

    return r;
}

 // * Point on ray:
 // *     pos + t * dir, t in R
 // * Point in triangle:
 // *     (1 - u - v) * v0 + u * v1 + v * v2, u >= 0, v >= 0, u + v <= 1
int ray_intersects_triangle( Vec* pos, Vec* dir, Vec* v0, Vec* v1, Vec* v2, Vec *result ) {
    Vec eu, ev, cv, vt, cu;
    vec_sub( &eu, v1, v0 );
    vec_sub( &ev, v2, v0 );

    vec_cross( &cv, dir, &ev );
    float det = vec_dot( &eu, &cv );

    // cull if in triangle plane, of hit backface
    if( det <= 0 ) return 0;

    vec_sub( &vt, pos, v0 );

    float u = vec_dot( &vt, &cv );
    if( u < 0 || u > det ) return 0;

    vec_cross( &cu, &vt, &eu );

    float v = vec_dot( dir, &cu );
    if( v < 0 || u+v > det ) return 0;

    float t = vec_dot( &ev, &cu );
    if( t < 0 ) return 0;

    t /= det;

    vec_add( result, pos, vec_scale(result, dir, t) );
    return 1;
}

int line_intersects_triangle( Vec* p1, Vec* p2, Vec* v0, Vec* v1, Vec* v2, Vec *result ) {
    Vec d, dn, s;
    vec_sub( &d, p2, p1 );
    vec_normalize( &dn, &d );
    if( ray_intersects_triangle(p1, &dn, v0, v1, v2, result) ) {
        vec_sub( &s, result, p1 );
        float ss = vec_dot( &s, &s );
        float dd = vec_dot( &d, &d );
        if( ss <= dd ) return 1;
    }
    return 0;
}

void camera_turn( Camera *camera, float a ) {
    Vec axis = {0, 1, 0};
    vec_rotate( &camera->r, &camera->r, &axis, a );
    vec_normalize( &camera->r, &camera->r );

    vec_cross( &camera->b, &camera->r, &camera->u );
    vec_normalize( &camera->b, &camera->b );
}

void camera_pitch( Camera *camera, float a ) {
    vec_rotate( &camera->b, &camera->b, &camera->r, a );
    vec_cross( &camera->u, &camera->b, &camera->r );
}

void camera_yaw( Camera *camera, float a ) {
    vec_rotate( &camera->r, &camera->r, &camera->u, a );
    vec_cross( &camera->b, &camera->r, &camera->u );
}

void camera_roll( Camera *camera, float a ) {
    vec_rotate( &camera->u, &camera->u, &camera->b, a );
    vec_cross( &camera->r, &camera->u, &camera->b );
}
