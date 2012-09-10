#include "myr.h"

// GMat4
void g_mat4_identity( GMat4 *mat ) {
    float *m = (float*) mat;
    m[0] = 1; m[4] = 0; m[8]  = 0; m[12] = 0;
    m[1] = 0; m[5] = 1; m[9]  = 0; m[13] = 0;
    m[2] = 0; m[6] = 0; m[10] = 1; m[14] = 0;
    m[3] = 0; m[7] = 0; m[11] = 0; m[15] = 1;
}

void g_mat4_add( GMat4 *out, GMat4 *m1, GMat4 *m2 ) {
    float *r = (float *) out, *m = (float *) m1, *n = (float *) m2;
    int i;
    for( i=0; i<16; i++ ) r[i] = m[i] + n[i];
}

void g_mat4_mul_scalar( GMat4 *out, GMat4 *m1, float scalar ) {
    float *r = (float *) out, *m = (float *) m1;
    int i;
    for( i = 0; i<16; i++ ) r[i] = m[i] * scalar;
}

void g_mat4_mul( GMat4 *out, GMat4 *m1, GMat4 *m2 ) {
    GMat4 temp1, temp2;
    int i,j;
    if( m1 == out ){ temp1 = *m1; m1 = &temp1; }
    if( m2 == out ){ temp2 = *m2; m2 = &temp2; }

    float *r = (float*) out, *m = (float*) m1, *n = (float*) m2;
    for( j=0; j < 4; ++j ) {
        for (i=0; i < 4; ++i)
            r[4*j+i] = m[i]*n[4*j] + m[4+i]*n[4*j+1] + m[8+i]*n[4*j+2] + m[12+i]*n[4*j+3];
    }
}

void g_mat4_vec_mul( GVec *out, GMat4 *mat, GVec *in ) {
    float *m = (float*) mat;
    GVec v;
    if( in == out ){ v = *in; in = &v; } // copy if it's in-place
    out->x = m[0] * in->x + m[4] * in->y + m[8]  * in->z + m[12];
    out->y = m[1] * in->x + m[5] * in->y + m[9]  * in->z + m[13];
    out->z = m[2] * in->x + m[6] * in->y + m[10] * in->z + m[14];
}

void g_mat4_transpose( GMat4 *mat ) {
    float *m = (float *) mat;
    int i,j;
    for( j=0; j < 4; ++j ) {
        for( i=j+1; i < 4; ++i ) {
            float t = m[i*4+j];
            m[i*4+j] = m[j*4+i];
            m[j*4+i] = t;
        }
    }
}

void g_mat4_from_quat_vec(GMat4 *m, GQuat *q, GVec *v) {
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

    m->v[0].x  = 1 - qy2_2 - qz2_2;
    m->v[1].x  =     qxy_2 - qzw_2;
    m->v[2].x  =     qxz_2 + qyw_2;
    m->v[3].x = v->x;

    m->v[0].y =     qxy_2 + qzw_2;
    m->v[1].y = 1 - qx2_2 - qz2_2;
    m->v[2].y =     qyz_2 - qxw_2;
    m->v[3].y = v->y;

    m->v[0].z =     qxz_2 - qyw_2;
    m->v[1].z =     qyz_2 + qxw_2;
    m->v[2].z = 1 - qx2_2 - qy2_2;
    m->v[3].z = v->z;

    m->v[0].w = 0;
    m->v[1].w = 0;
    m->v[2].w = 0;
    m->v[3].w = 1;
}

void g_mat4_ortho( GMat4 *m, float w, float h, float n, float f ) {
    g_mat4_identity( m );
    m->v[0].x = 2.0f/w;
    m->v[1].y = 2.0f/h;
    m->v[2].z = -2.0f/(f - n);
    m->v[3].z = -(f + n)/(f - n);
}

void g_mat4_persp( GMat4 *m, float fovy, float aspect, float znear, float zfar ) {
    float f = 1.0/tanf(fovy/360.0*G_PI);

    g_mat4_identity( m );
    m->v[0].x = f/aspect;
    m->v[1].y = f;
    m->v[2].z = (zfar + znear)/(znear - zfar);
    m->v[2].w = -1.0f;
    m->v[3].z = 2.0f*zfar*znear/(znear - zfar);
}

void g_mat4_look_at( GMat4 *m, GVec *eye, GVec *target, GVec *up ){
    GVec x_vec, y_vec, z_vec;
    g_vec_sub( &z_vec, eye, target);
    g_vec_normalize( &z_vec );

    g_vec_cross( &x_vec, up, &z_vec );
    g_vec_normalize( &x_vec );

    g_vec_cross( &y_vec, &z_vec, &x_vec );
    g_vec_normalize( &y_vec );

    m->v[0].x = x_vec.x;
    m->v[1].x = x_vec.y;
    m->v[2].x = x_vec.z;
    m->v[3].x = -g_vec_dot( &x_vec, eye );

    m->v[0].y = y_vec.x;
    m->v[1].y = y_vec.y;
    m->v[2].y = y_vec.z;
    m->v[3].y = -g_vec_dot( &y_vec, eye );

    m->v[0].z = z_vec.x;
    m->v[1].z = z_vec.y;
    m->v[2].z = z_vec.z;
    m->v[3].z = -g_vec_dot( &z_vec, eye );

    m->v[0].w = 0;
    m->v[1].w = 0;
    m->v[2].w = 0;
    m->v[3].w = 1;
}

//GVec
void g_vec_add( GVec *r, GVec *u, GVec *v ) {
    r->x = u->x + v->x;
    r->y = u->y + v->y;
    r->z = u->z + v->z;
}

void g_vec_sub( GVec *r, GVec *u, GVec *v ) {
    r->x = u->x - v->x;
    r->y = u->y - v->y;
    r->z = u->z - v->z;
}

void g_vec_mul_scalar( GVec *r, GVec *u, float scalar ) {
    r->x = u->x * scalar;
    r->y = u->y * scalar;
    r->z = u->z * scalar;
}

float g_vec_dot( GVec *v0, GVec *v1 ) {
    return v0->x*v1->x + v0->y*v1->y + v0->z*v1->z;
}

void g_vec_cross( GVec *r, GVec *v0, GVec *v1 ) {
    r->x = v0->y * v1->z - v0->z * v1->y;
    r->y = v0->z * v1->x - v0->x * v1->z;
    r->z = v0->x * v1->y - v0->y * v1->x; // right hand rule: i x j = k
}

void g_vec_lerp( GVec *r, GVec *a, GVec *b, float t ) {
    r->x = a->x + t * (b->x - a->x);
    r->y = a->y + t * (b->y - a->y);
    r->z = a->z + t * (b->z - a->z);
}

void g_vec_scale_add( GVec *r, GVec *q, float sc ) {
    r->x += q->x * sc;
    r->y += q->y * sc;
    r->z += q->z * sc;
}

float g_vec_mag( GVec *v ) {
    return (float) sqrt( v->x*v->x + v->y*v->y + v->z*v->z );
}

float g_vec_dist( GVec *v0, GVec *v1 ) {
    float x = v0->x - v1->x;
    float y = v0->y - v1->y;
    float z = v0->z - v1->z;
    return (float) sqrt( x*x + y*y + z*z );
}

void g_vec_normalize( GVec *v ) {
    float mag = g_vec_mag(v);
    v->x /= mag;
    v->y /= mag;
    v->z /= mag;
}

// GQuat
void g_quat_identity( GQuat *q ) {
    q->x = q->y = q->z = 0;
    q->w = 1;
}

void g_quat_invert( GQuat *q ) {
    q->x = -q->x;
    q->y = -q->y;
    q->z = -q->z;
}

void g_quat_normalize( GQuat *q ) {
    float d = (float) sqrt(q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w);
    if (d >= 0.00001) {
        d = 1/d;
        q->x *= d;
        q->y *= d;
        q->z *= d;
        q->w *= d;
    } else {
        g_quat_identity(q);
    }
}

void g_quat_mul( GQuat *r, GQuat *q1, GQuat *q2 ) {
    GQuat temp;
    if (r == q1) { temp = *q1; q1 = &temp; }
    if (r == q2) { temp = *q2; q2 = &temp; }

    r->x = q1->w*q2->x + q1->x*q2->w + q1->y*q2->z - q1->z*q2->y;
    r->y = q1->w*q2->y - q1->x*q2->z + q1->y*q2->w + q1->z*q2->x;
    r->z = q1->w*q2->z + q1->x*q2->y - q1->y*q2->x + q1->z*q2->w;
    r->w = q1->w*q2->w - q1->x*q2->x - q1->y*q2->y - q1->z*q2->z;
}

void g_quat_scale_add( GQuat *r, GQuat *q, float sc ) {
    r->x += q->x * sc;
    r->y += q->y * sc;
    r->z += q->z * sc;
    r->w += q->w * sc;
}

void g_quat_vec_mul( GVec *r, GQuat *q, GVec *v ) {
    GQuat qvec = { v->x, v->y, v->z, 0 };
    GQuat qinv = { -q->x, -q->y, -q->z, q->w };
    GQuat temp;

    g_quat_mul( &temp, q, &qvec );
    g_quat_mul( &temp, &temp, &qinv );
    r->x = temp.x;
    r->y = temp.y;
    r->z = temp.z;
}

void g_quat_from_axis_angle( GQuat *q, GVec *axis, float ang ) {
    q->w = (float) cos(ang/2);
    g_vec_mul_scalar((GVec *) q, axis, (float) sin(ang/2));
}

void g_quat_from_mat4( GQuat* q, GMat4* m ){
    q->x = 1 + m->v[0].x - m->v[1].y - m->v[2].z;
    if( q->x < 0 ) q->x = 0;
    else q->x = (float) sqrt(q->x)/2;

    q->y = 1 - m->v[0].x + m->v[1].y - m->v[2].z;
    if( q->y < 0 ) q->y = 0;
    else q->y = (float) sqrt(q->y)/2;

    q->z = 1 - m->v[0].x - m->v[1].y + m->v[2].z;
    if( q->z < 0 ) q->z = 0;
    else q->z = (float) sqrt(q->z)/2;

    q->w = 1 + m->v[0].x + m->v[1].y + m->v[2].z;
    if( q->w < 0 ) q->w = 0;
    else q->w = (float) sqrt(q->w)/2;

    if( m->v[1].z - m->v[2].y < 0 ) q->x = -q->x;
    if( m->v[2].x - m->v[0].z < 0 ) q->y = -q->y;
    if( m->v[0].y - m->v[1].x < 0 ) q->z = -q->z;
}

// Dual Quaternions
void g_dual_quat_scale_add( GDualQuat* r, GDualQuat* dq, float t ){
    float dot_real = r->q.x*dq->q.x + r->q.y*dq->q.y + r->q.z*dq->q.z + r->q.w*dq->q.w;
    float k = dot_real < 0 ? -t : t;
    g_quat_scale_add( &r->q, &dq->q, k );
    g_quat_scale_add( &r->d, &dq->d, k );
}

void g_dual_quat_lerp( GDualQuat* r, GDualQuat* d1, GDualQuat* d2, float t ) {
    float dot_real = d1->q.x*d2->q.x + d1->q.y*d2->q.y + d1->q.z*d2->q.z + d1->q.w*d2->q.w;
    float k = dot_real < 0 ? -t : t;

    r->q.x = d1->q.x*(1-t) + d2->q.x*k;
    r->q.y = d1->q.y*(1-t) + d2->q.y*k;
    r->q.z = d1->q.z*(1-t) + d2->q.z*k;
    r->q.w = d1->q.w*(1-t) + d2->q.w*k;

    r->d.x = d1->d.x*(1-t) + d2->d.x*k;
    r->d.y = d1->d.y*(1-t) + d2->d.y*k;
    r->d.z = d1->d.z*(1-t) + d2->d.z*k;
    r->d.w = d1->d.w*(1-t) + d2->d.w*k;
}

void g_dual_quat_from_quat_vec( GDualQuat *dq, GQuat *q, GVec *v ) {
    dq->q = *q;
    GQuat qvec = { 0.5*v->x, 0.5*v->y, 0.5*v->z, 0 };
    g_quat_mul( &dq->d, &qvec, q );
}

void g_dual_quat_invert( GDualQuat *r, GDualQuat *dq ) {
    //compute the dual normal
    double real =         dq->q.w*dq->q.w + dq->q.x*dq->q.x + dq->q.y*dq->q.y + dq->q.z*dq->q.z;
    double dual =  2.0 * (dq->q.w*dq->d.w + dq->q.x*dq->d.x + dq->q.y*dq->d.y + dq->q.z*dq->d.z);

    //set the inverse dual_quat
    r->q.x = -dq->q.x * real;
    r->q.y = -dq->q.y * real;
    r->q.z = -dq->q.z * real;
    r->q.w =  dq->q.w * real;
    r->d.x = dq->d.x * (dual-real);
    r->d.y = dq->d.y * (dual-real);
    r->d.z = dq->d.z * (dual-real);
    r->d.w = dq->d.w * (real-dual);
}

void g_dual_quat_mul( GDualQuat *dq, GDualQuat *a, GDualQuat *b ) {
    GDualQuat temp;
    if (dq == a) { temp = *a; a = &temp; }
    if (dq == b) { temp = *b; b = &temp; }

    dq->q.w = a->q.w*b->q.w - a->q.x*b->q.x - a->q.y*b->q.y - a->q.z*b->q.z;
    dq->q.x = a->q.w*b->q.x + a->q.x*b->q.w + a->q.y*b->q.z - a->q.z*b->q.y;
    dq->q.y = a->q.w*b->q.y + a->q.y*b->q.w - a->q.x*b->q.z + a->q.z*b->q.x;
    dq->q.z = a->q.w*b->q.z + a->q.z*b->q.w + a->q.x*b->q.y - a->q.y*b->q.x;

// Dual unit Quaternion.
    dq->d.x  =  a->d.x*b->q.w + a->q.w*b->d.x + a->d.w*b->q.x + a->q.x*b->d.w -
                a->d.z*b->q.y + a->q.y*b->d.z + a->d.y*b->q.z - a->q.z*b->d.y;
    dq->d.y  =  a->d.y*b->q.w + a->q.w*b->d.y + a->d.z*b->q.x - a->q.x*b->d.z +
                a->d.w*b->q.y + a->q.y*b->d.w - a->d.x*b->q.z + a->q.z*b->d.x;
    dq->d.z  =  a->d.z*b->q.w + a->q.w*b->d.z - a->d.y*b->q.x + a->q.x*b->d.y +
                a->d.x*b->q.y - a->q.y*b->d.x + a->d.w*b->q.z + a->q.z*b->d.w;
    dq->d.w  =  a->d.w*b->q.w + a->q.w*b->d.w - a->q.x*b->d.x - a->d.x*b->q.x -
                a->q.y*b->d.y - a->d.y*b->q.y - a->q.z*b->d.z - a->d.z*b->q.z;
}

void g_dual_quat_vec_mul( GVec *r, GDualQuat *dq, GVec *v ) {
    g_quat_vec_mul( r, &dq->q, v );
    r->x += 2.0 * ( dq->q.w*dq->d.x - dq->q.x*dq->d.w + dq->q.y*dq->d.z - dq->q.z*dq->d.y );
    r->y += 2.0 * ( dq->q.w*dq->d.y - dq->q.y*dq->d.w - dq->q.x*dq->d.z + dq->q.z*dq->d.x );
    r->z += 2.0 * ( dq->q.w*dq->d.z - dq->q.z*dq->d.w + dq->q.x*dq->d.y - dq->q.y*dq->d.x );
}

void g_dual_quat_normalize( GDualQuat *dq ) {
    float d = sqrtf( dq->q.x*dq->q.x + dq->q.y*dq->q.y + dq->q.z*dq->q.z + dq->q.w*dq->q.w );
    if( d >= 0.00001 ) {
        d = 1/d;
        dq->q.x *= d;
        dq->q.y *= d;
        dq->q.z *= d;
        dq->q.w *= d;

        dq->d.x *= d;
        dq->d.y *= d;
        dq->d.z *= d;
        dq->d.w *= d;
    } else {
        *dq = (GDualQuat){{.0, .0, .0, 1.0}, {.0, .0, .0, .0}};
    }
}
