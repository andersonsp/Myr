#include "land.h"

int debug_disable_grid;
int stats_drawn_objects;

World *world_new(void) {
    World *self = calloc(1, sizeof *self);
    return self;
}

void world_add_object( World *self, Object *object ) {
    self->o = realloc(self->o, (self->on + 1) * sizeof *self->o);
    self->o[self->on] = object;
    self->on++;
}

Object *object_new( Vec pos, Model *mdl ) {
    Object *self = g_new0( Object, 1 );

    self->pos = pos;
    self->rot = (Quat){ .0f, .0f, .0f, 1.0f };

    self->model = mdl;
    if( mdl ) self->rr = model_calculate_bounding_sphere( mdl );
    return self;
}

void object_draw( Object *self ) {
    stats_drawn_objects++;
    glPushMatrix();
    apply_orientation( self );

    model_draw( self->model, 0 );
    glPopMatrix();
}

void world_draw(World *self, Object *camera) {
    int i;
    for( i = 0; i < self->on; i++ ) {
        // Vec a, c;
        Object *o = self->o[i];
        // vec_sub( &a, &o->camera.p, &camera->p );
        // vec_cross( &c, &a, &camera->b );
        // float sidesquare = vec_dot( &c, &c );
        // float distsquare = vec_dot( &a, &a );
        // float dot = vec_dot( &camera->b, &a );
        // if( dot > 0 && distsquare > o->rr ) continue;
        // if( sidesquare > distsquare + o->rr ) continue;

        object_draw( o );
    }
}

int object_collision( Object *o, Vec* pos, Vec* dir, Vec *result ) {
    Vec a, c, dirpos, relpos, reldir, relresult;
    vec_sub( &a, &o->pos, pos );
    vec_cross( &c, &a, dir );
    float dd = vec_dot( &c, &c );
    if( dd < o->rr ) {
        vec_add( &dirpos, pos, dir );
        object_transform( &relpos, pos, o );
        object_transform( &dirpos, &dirpos, o );
        vec_sub( &reldir, &dirpos, &relpos );
        int r;
        // if (debug_disable_grid)
            r = model_collision( o->model, &relpos, &reldir, &relresult );
        // else r = grid_collision(o->mesh->grid, relpos, reldir, &relresult);
        if( r ) {
            object_back_transform( result, &relresult, o );
            return 1;
        }
    }
    return 0;
}

Object *world_collision(World *self, Vec* pos, Vec* dir, Vec *result) {
    int i;
    float min = 10000000000;
    Object *col = NULL;
    for (i = 0; i < self->on; i++) {
        Object *o = self->o[i];
        Vec tmp1, tmp2;
        if( object_collision(o, pos, dir, &tmp1) ) {
            float d = vec_len( vec_sub( &tmp2, &tmp1, pos) );
            if( d < min ) {
                *result = tmp1;
                col = o;
                min = d;
            }
        }
    }
    return col;
}

//
// camera stuff
//
void apply_camera( Object *o ) {
    Vec trans;
    Quat inv_rot;
    Mat4 matrix;
    quat_invert( &inv_rot, &o->rot );
    quat_vec_mul( &trans, &inv_rot, vec_scale(&trans, &o->pos, -1.0f) );
    mat4_from_quat_vec( &matrix, &inv_rot, &trans );
    glMultMatrixf( matrix.m );
}

void apply_orientation( Object *o ) {
    Mat4 matrix;
    mat4_from_quat_vec( &matrix, &o->rot, &o->pos );
    glMultMatrixf( matrix.m );
}

static Vec x_axis = {1.0f, .0f, .0f}, y_axis = {.0f, 1.0f, .0f}, z_axis = {.0f, .0f, 1.0f};

void object_turn( Object *o, float a ) {
    Quat q;
    quat_from_axis_angle( &q, &y_axis, a );
    quat_mul( &o->rot, &q, &o->rot );
    quat_normalize( &o->rot, &o->rot );
}

void object_pitch( Object *o, float a ) {
    Quat q;
    quat_from_axis_angle( &q, &x_axis, a );
    quat_mul( &o->rot, &o->rot, &q );
    quat_normalize( &o->rot, &o->rot );
}

void object_yaw( Object *o, float a ) {
    Quat q;
    quat_from_axis_angle( &q, &y_axis, a );
    quat_mul( &o->rot, &o->rot, &q );
    quat_normalize( &o->rot, &o->rot );
}

void object_roll( Object *o, float a ) {
    Quat q;
    quat_from_axis_angle( &q, &z_axis, a );
    quat_mul( &o->rot, &o->rot, &q );
    quat_normalize( &o->rot, &o->rot );
}

void object_move( Object *o, float x, float y, float z ) {
    Vec b, r, u;

    quat_vec_mul( &r, &o->rot, &x_axis );
    vec_normalize( &b, vec_cross(&b, &y_axis, &r) );
    vec_add( &o->pos, &o->pos, vec_scale(&b, &b, z) );
    vec_add( &o->pos, &o->pos, vec_scale(&r, &r, x) );
    vec_add( &o->pos, &o->pos, vec_scale(&u, &y_axis, y) );
}

// Transform from world into object coordinates.
Vec* object_transform( Vec* r, Vec* a, Object* cam ) {
    Vec u;
    quat_vec_mul( r, &cam->rot, vec_sub(&u, a, &cam->pos) );
    return r;
}

// Transform from object into world coordinates.
Vec* object_back_transform( Vec* r, Vec* a, Object* cam ) {
    Vec u;
    Quat iq;
    quat_invert( &iq, &cam->rot );
    vec_add( r, quat_vec_mul(&u, &iq, a), &cam->pos );
    return r;
}


