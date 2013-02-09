#include "land.h"

void mat4_identity( Mat4 *mat ) {
    float* m = mat->m;
    m[0] = 1; m[4] = 0; m[8]  = 0; m[12] = 0;
    m[1] = 0; m[5] = 1; m[9]  = 0; m[13] = 0;
    m[2] = 0; m[6] = 0; m[10] = 1; m[14] = 0;
    m[3] = 0; m[7] = 0; m[11] = 0; m[15] = 1;
}


void mat4_from_quat_vec( Mat4 *mat, Quat *q, Vec *v ) {
    float *m = mat->m;
    float x2    = 2 * q->x;
    float qx2_2 = x2 * q->x;
    float qxy_2 = x2 * q->y;
    float qxz_2 = x2 * q->z;
    float qxw_2 = x2 * q->w;

    float y2    = 2 * q->y;
    float qy2_2 = y2 * q->y;
    float qyz_2 = y2 * q->z;
    float qyw_2 = y2 * q->w;

    float z2    = 2 * q->z;
    float qz2_2 = z2 * q->z;
    float qzw_2 = z2 * q->w;

    m[0]  = 1 - qy2_2 - qz2_2;
    m[4]  =     qxy_2 - qzw_2;
    m[8]  =     qxz_2 + qyw_2;
    m[12] = v->x;

    m[1]  =     qxy_2 + qzw_2;
    m[5]  = 1 - qx2_2 - qz2_2;
    m[9]  =     qyz_2 - qxw_2;
    m[13] = v->y;

    m[2]  =     qxz_2 - qyw_2;
    m[6]  =     qyz_2 + qxw_2;
    m[10] = 1 - qx2_2 - qy2_2;
    m[14] = v->z;

    m[3]  = 0;
    m[7]  = 0;
    m[11] = 0;
    m[15] = 1;
}

void mat4_mul( Mat4 *out, Mat4 *m1, Mat4 *m2 ) {
  Mat4 temp1, temp2;
  int i,j;
  if( m1 == out ){ temp1 = *m1; m1 = &temp1; }
  if( m2 == out ){ temp2 = *m2; m2 = &temp2; }

  float *r = (float*) out->m, *m = (float*) m1->m, *n = (float*) m2->m;
  for (j=0; j < 4; ++j)
    for (i=0; i < 4; ++i)
       r[4*j+i] = m[i]*n[4*j] + m[4+i]*n[4*j+1] + m[8+i]*n[4*j+2] + m[12+i]*n[4*j+3];
}

void mat4_ortho( Mat4 *mat, float w, float h, float n, float f ) {
    mat4_identity( mat );
    float *m = mat->m;
    m[0] = 2.0f/w;
    m[5] = 2.0f/h;
    m[10] = -2.0f/(f - n);
    m[14] = -(f + n)/(f - n);
}

void mat4_persp( Mat4 *mat, float fovy, float aspect, float znear, float zfar ) {
    float *m = mat->m, f = 1.0/tanf(fovy/360.0*G_PI);

    mat4_identity( mat );
    m[0] = f/aspect;
    m[5] = f;
    m[10] = (zfar + znear)/(znear - zfar);
    m[11] = -1.0f;
    m[14] = 2.0f*zfar*znear/(znear - zfar);
}

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
    if( mag == 0 ) return r;
    r->x = a->x / mag;
    r->y = a->y / mag;
    r->z = a->z / mag;
    return r;
}

float vec_dot( Vec* a, Vec* b ) {
    return a->x*b->x + a->y*b->y + a->z*b->z;
}

float vec_len( Vec* a ) {
    return (float) sqrt( a->x*a->x + a->y*a->y + a->z*a->z );
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


Quat* quat_invert( Quat *r, Quat *q ) {
    r->x = -q->x;
    r->y = -q->y;
    r->z = -q->z;
    r->w =  q->w;
    return r;
}

Quat* quat_normalize( Quat *r, Quat *q ) {
    float d = (float) sqrt(q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w);
    if (d >= 0.00001f) {
        d = 1/d;
        r->x = q->x * d;
        r->y = q->y * d;
        r->z = q->z * d;
        r->w = q->w * d;
    } else {
        *r = (Quat){ .0f, .0f, .0f, 1.0f};
    }
    return r;
}

Quat* quat_mul( Quat *r, Quat *q1, Quat *q2 ) {
    Quat temp;
    if (r == q1) { temp = *q1; q1 = &temp; }
    if (r == q2) { temp = *q2; q2 = &temp; }

    r->x = q1->w*q2->x + q1->x*q2->w + q1->y*q2->z - q1->z*q2->y;
    r->y = q1->w*q2->y - q1->x*q2->z + q1->y*q2->w + q1->z*q2->x;
    r->z = q1->w*q2->z + q1->x*q2->y - q1->y*q2->x + q1->z*q2->w;
    r->w = q1->w*q2->w - q1->x*q2->x - q1->y*q2->y - q1->z*q2->z;
    return r;
}

Quat* quat_from_axis_angle( Quat *q, Vec *axis, float ang ) {
    q->w = (float) cos(ang/2);
    vec_scale( (Vec*) q, axis, (float) sin(ang/2) );
    return q;
}

Vec* quat_vec_mul( Vec *r, Quat *q, Vec *v ) {
    Quat qvec = { v->x, v->y, v->z, 0 };
    Quat qinv = { -q->x, -q->y, -q->z, q->w };
    Quat temp;

    quat_mul( &temp, quat_mul(&temp, q, &qvec), &qinv );
    r->x = temp.x;
    r->y = temp.y;
    r->z = temp.z;
    return r;
}
