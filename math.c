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
