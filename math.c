#include "land.h"


Vector vector_add(Vector a, Vector b) {
    return (Vector){a.x + b.x, a.y + b.y, a.z + b.z};
}

Vector vector_sub(Vector a, Vector b) {
    return (Vector){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vector vector_scale(Vector a, float s) {
    return (Vector){a.x * s, a.y * s, a.z * s};
}

float vector_dot(Vector a, Vector b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector vector_cross(Vector a, Vector b) {
    Vector result;
    result.x = a.y * b.z - b.y * a.z;
    result.y = a.z * b.x - b.z * a.x;
    result.z = a.x * b.y - b.x * a.y;
    return result;
}

// Allegro defines vector_length already
float vector_len(Vector a) {
    return sqrtf(vector_dot(a, a));
}

Vector vector_normalize(Vector a){
    return vector_scale(a, 1.0 / vector_len(a));
}

/* Transform from world into camera coordinates. */
Vector vector_transform(Vector a, Camera *camera) {
    a = vector_sub(a, camera->p);
    return (Vector)
    {
        vector_dot(a, camera->r),
        vector_dot(a, camera->u),
        vector_dot(a, camera->b),
    };
}

/* Transform from camera into world coordinates. */
Vector vector_backtransform(Vector a, Camera *camera) {
    Vector x, y, z;

    x = vector_scale(camera->r, a.x);
    y = vector_scale(camera->u, a.y);
    z = vector_scale(camera->b, a.z);

    return vector_add(vector_add(vector_add(x, y), z), camera->p);
}

/* Rotate a around b by angle, store in result. */
Vector vector_rotate(Vector a, Vector b, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    Vector row[3] = {
        vector_scale(b, b.x * (1 - c)),
        vector_scale(b, b.y * (1 - c)),
        vector_scale(b, b.z * (1 - c))
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

    return (Vector) {
        vector_dot(a, row[0]),
        vector_dot(a, row[1]),
        vector_dot(a, row[2])
    };
}

/*
 * Point on ray:
 *     pos + t * dir, t in R
 * Point in triangle:
 *     (1 - u - v) * v0 + u * v1 + v * v2, u >= 0, v >= 0, u + v <= 1
 *
 *
 */
int ray_intersects_triangle(Vector pos, Vector dir, Vector v0, Vector v1, Vector v2, Vector *result) {
    Vector eu = vector_sub(v1, v0);
    Vector ev = vector_sub(v2, v0);

    Vector cv = vector_cross(dir, ev);
    float det = vector_dot(eu, cv);

    /* cull if in triangle plane, of hit backface */
    if (det <= 0) return 0;

    Vector vt = vector_sub(pos, v0);

    float u = vector_dot(vt, cv);
    if (u < 0 || u > det) return 0;

    Vector cu = vector_cross(vt, eu);

    float v = vector_dot(dir, cu);
    if (v < 0 || u + v > det) return 0;

    float t = vector_dot(ev, cu);
    if (t < 0) return 0;

    t /= det;
    //u /= det;
    //v /= det;

    *result = vector_add(pos, vector_scale(dir, t));
    return 1;
}

int line_intersects_triangle(Vector pos1, Vector pos2, Vector v0, Vector v1, Vector v2, Vector *result) {
    Vector d = vector_sub(pos2, pos1);
    Vector dn = vector_normalize(d);
    if( ray_intersects_triangle(pos1, dn, v0, v1, v2, result) ) {
        Vector s = vector_sub(*result, pos1);
        float ss = vector_dot(s, s);
        float dd = vector_dot(d, d);
        if (ss <= dd) return 1;
    }
    return 0;
}

void camera_turn(Camera *camera, float a) {
    camera->r = vector_rotate(camera->r, (Vector){0, 1, 0}, a);
    camera->r = vector_normalize(camera->r);

    camera->b = vector_cross(camera->r, camera->u);
    camera->b = vector_normalize(camera->b);
}

void camera_pitch(Camera *camera, float a) {
    camera->b = vector_rotate(camera->b, camera->r, a);
    camera->u = vector_cross(camera->b, camera->r);
}

void camera_yaw(Camera *camera, float a) {
    camera->r = vector_rotate(camera->r, camera->u, a);
    camera->b = vector_cross(camera->r, camera->u);
}

void camera_roll(Camera *camera, float a) {
    camera->u = vector_rotate(camera->u, camera->b, a);
    camera->r = vector_cross(camera->u, camera->b);
}
