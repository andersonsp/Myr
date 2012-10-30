#include "land.h"

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

void camera_apply(Camera *camera) {
    GLfloat matrix[16];
    matrix[0] = camera->r.x;
    matrix[1] = camera->u.x;
    matrix[2] = camera->b.x;
    matrix[3] = 0;
    matrix[4] = camera->r.y;
    matrix[5] = camera->u.y;
    matrix[6] = camera->b.y;
    matrix[7] = 0;
    matrix[8] = camera->r.z;
    matrix[9] = camera->u.z;
    matrix[10] = camera->b.z;
    matrix[11] = 0;
    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1;
    glMultMatrixf(matrix);

    glTranslatef(-camera->p.x, -camera->p.y, -camera->p.z);
}

void camera_apply_orientation(Camera *camera) {
    glTranslatef(camera->p.x, camera->p.y, camera->p.z);

    GLfloat matrix[16];
    matrix[0] = camera->r.x;
    matrix[1] = camera->r.y;
    matrix[2] = camera->r.z;
    matrix[3] = 0;
    matrix[4] = camera->u.x;
    matrix[5] = camera->u.y;
    matrix[6] = camera->u.z;
    matrix[7] = 0;
    matrix[8] = camera->b.x;
    matrix[9] = camera->b.y;
    matrix[10] = camera->b.z;
    matrix[11] = 0;
    matrix[12] = 0;
    matrix[13] = 0;
    matrix[14] = 0;
    matrix[15] = 1;
    glMultMatrixf(matrix);
}
