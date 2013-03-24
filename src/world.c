#include "land.h"

int debug_disable_grid;
int stats_drawn_objects;

extern Vec obj_pos[4];

World *world_new(void) {
    World *self = calloc(1, sizeof *self);
    return self;
}

void world_add_object( World *self, Object *object ) {
    self->o = realloc(self->o, (self->on + 1) * sizeof *self->o);
    self->o[self->on] = object;
    self->on++;
}

Object *object_new( Vec pos, Model *mdl, Program* program ) {
    Object *self = g_new0( Object, 1 );

    self->pos = pos;
    self->rot = (Quat){ .0f, .0f, .0f, 1.0f };
    self->anim_frame = 0.0f;

    self->model = mdl;
    if( mdl ) self->rr = model_calculate_bounding_sphere( mdl );
    if( program ) self->program = *program;
    return self;
}

void object_draw( Object *self, Mat4* vp ) {
    Mat4 model, mvp;
    mat4_from_quat_vec( &model, &self->rot, &self->pos );
    mat4_mul( &mvp, vp, &model );

    model_draw( self->model, &self->program, &mvp, 0.0 );  //self->anim_frame
}

void world_draw( World *self, Camera *camera ) {
    int i;
    Mat4 vp;
    mat4_mul( &vp, &camera->projection, &camera->view );
    for( i = 0; i < self->on; i++ ) {
        Object *o = self->o[i];
        // frustum culling here
        // portal rendering here ??

        if( o->model && o->program.object ) object_draw( o, &vp );
    }
}

int object_collision( Object *o, Vec* pos, Vec* dir, float radius, Vec *result ) {
    Vec a, c, dirpos, relpos, reldir, relresult;
    vec_sub( &a, &o->pos, pos );
    vec_cross( &c, &a, dir );
    float dd = vec_dot( &c, &c );
    if( dd < o->rr ) {
        vec_add( &dirpos, pos, dir );
        object_transform( &relpos, pos, o );
        object_transform( &dirpos, &dirpos, o );
        vec_sub( &reldir, &dirpos, &relpos );

        int r = model_collision( o->model, &relpos, &reldir, radius, &relresult );
        if( r ) {
            object_back_transform( result, &relresult, o );
            return 1;
        }
    }
    return 0;
}

Object *world_collision(World *self, Vec* pos, Vec* dir, float radius, Vec *result) {
    int i;
    float min = 10000000000;
    Object *col = NULL;
    for (i = 0; i < self->on; i++) {
        Object *o = self->o[i];
        Vec tmp1, tmp2;
        if( object_collision(o, pos, dir, radius, &tmp1) ) {
            float d = vec_len( vec_sub(&tmp2, &tmp1, pos) );
            if( d < min ) {
                *result = tmp1;
                col = o;
                min = d;
            }
        }
    }
    return col;
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
