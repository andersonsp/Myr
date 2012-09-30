#include "land.h"


Vec vec_add( Vec a, Vec b ) {
    return (Vec){a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec vec_sub( Vec a, Vec b ) {
    return (Vec){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec vec_scale( Vec a, float s ) {
    return (Vec){a.x * s, a.y * s, a.z * s};
}

float vec_dot( Vec a, Vec b ) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec vec_cross( Vec a, Vec b ) {
    Vec result;
    result.x = a.y * b.z - b.y * a.z;
    result.y = a.z * b.x - b.z * a.x;
    result.z = a.x * b.y - b.x * a.y;
    return result;
}

float vec_len( Vec a ) {
    return sqrtf(vec_dot(a, a));
}

Vec vec_normalize(Vec a){
    return vec_scale(a, 1.0 / vec_len(a));
}

// Transform from world into camera coordinates.
Vec vec_transform(Vec a, Camera *camera) {
    a = vec_sub(a, camera->p);
    return (Vec) {
        vec_dot(a, camera->r),
        vec_dot(a, camera->u),
        vec_dot(a, camera->b),
    };
}

// Transform from camera into world coordinates.
Vec vec_backtransform(Vec a, Camera *camera) {
    Vec x, y, z;

    x = vec_scale(camera->r, a.x);
    y = vec_scale(camera->u, a.y);
    z = vec_scale(camera->b, a.z);

    return vec_add(vec_add(vec_add(x, y), z), camera->p);
}

// Rotate a around b by angle, store in result.
Vec vec_rotate(Vec a, Vec b, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    Vec row[3] = {
        vec_scale(b, b.x * (1 - c)),
        vec_scale(b, b.y * (1 - c)),
        vec_scale(b, b.z * (1 - c))
    };
    row[0].x += c;
    row[0].y += b.z * s;
    row[0].z -= b.y * s;

    row[1].x -= b.z * s;
    row[1].y += c;
    row[1].z += b.x * s;

    row[2].x += b.y * s;
    row[2].y -= b.x * s;
    row[2].z += c;

    return (Vec) {
        vec_dot(a, row[0]),
        vec_dot(a, row[1]),
        vec_dot(a, row[2])
    };
}


 // * Point on ray:
 // *     pos + t * dir, t in R
 // * Point in triangle:
 // *     (1 - u - v) * v0 + u * v1 + v * v2, u >= 0, v >= 0, u + v <= 1

int ray_intersects_triangle(Vec pos, Vec dir, Vec v0, Vec v1, Vec v2, Vec *result) {
    Vec eu = vec_sub(v1, v0);
    Vec ev = vec_sub(v2, v0);

    Vec cv = vec_cross(dir, ev);
    float det = vec_dot(eu, cv);

    // cull if in triangle plane, of hit backface
    if (det <= 0) return 0;

    Vec vt = vec_sub(pos, v0);

    float u = vec_dot(vt, cv);
    if (u < 0 || u > det) return 0;

    Vec cu = vec_cross(vt, eu);

    float v = vec_dot(dir, cu);
    if (v < 0 || u + v > det) return 0;

    float t = vec_dot(ev, cu);
    if (t < 0) return 0;

    t /= det;

    *result = vec_add(pos, vec_scale(dir, t));
    return 1;
}

int line_intersects_triangle(Vec pos1, Vec pos2, Vec v0, Vec v1, Vec v2, Vec *result) {
    Vec d = vec_sub(pos2, pos1);
    Vec dn = vec_normalize(d);
    if( ray_intersects_triangle(pos1, dn, v0, v1, v2, result) ) {
        Vec s = vec_sub(*result, pos1);
        float ss = vec_dot(s, s);
        float dd = vec_dot(d, d);
        if (ss <= dd) return 1;
    }
    return 0;
}

void camera_turn(Camera *camera, float a) {
    camera->r = vec_rotate(camera->r, (Vec){0, 1, 0}, a);
    camera->r = vec_normalize(camera->r);

    camera->b = vec_cross(camera->r, camera->u);
    camera->b = vec_normalize(camera->b);
}

void camera_pitch(Camera *camera, float a) {
    camera->b = vec_rotate(camera->b, camera->r, a);
    camera->u = vec_cross(camera->b, camera->r);
}

void camera_yaw(Camera *camera, float a) {
    camera->r = vec_rotate(camera->r, camera->u, a);
    camera->b = vec_cross(camera->r, camera->u);
}

void camera_roll(Camera *camera, float a) {
    camera->u = vec_rotate(camera->u, camera->b, a);
    camera->r = vec_cross(camera->u, camera->b);
}
