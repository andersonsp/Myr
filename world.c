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
    Object *self = calloc(1, sizeof *self);
    self->camera.p = pos;
    self->camera.r.x = 1;
    self->camera.u.y = 1;
    self->camera.b.z = 1;
    self->model = mdl;
    self->rr = model_calculate_bounding_sphere( mdl );
    return self;
}

void object_draw( Object *self ) {
    stats_drawn_objects++;
    glPushMatrix();
    camera_apply_orientation( &self->camera );

    model_draw( self->model, 0 );
    glPopMatrix();
}

void world_draw(World *self, Camera *camera) {
    int i;
    for( i = 0; i < self->on; i++ ) {
        Vec a, c;
        Object *o = self->o[i];
        vec_sub( &a, &o->camera.p, &camera->p );
        vec_cross( &c, &a, &camera->b );
        float sidesquare = vec_dot( &c, &c );
        float distsquare = vec_dot( &a, &a );
        float dot = vec_dot( &camera->b, &a );
        if( dot > 0 && distsquare > o->rr ) continue;
        if( sidesquare > distsquare + o->rr ) continue;

        object_draw( o );
    }
}

int object_collision( Object *o, Vec* pos, Vec* dir, Vec *result ) {
    Vec a, c, dirpos, relpos, reldir, relresult;
    vec_sub( &a, &o->camera.p, pos );
    vec_cross( &c, &a, dir );
    float dd = vec_dot( &c, &c );
    if( dd < o->rr ) {
        vec_add( &dirpos, pos, dir );
        vec_transform( &relpos, pos, &o->camera );
        vec_transform( &dirpos, &dirpos, &o->camera );
        vec_sub( &reldir, &dirpos, &relpos );
        int r;
        // if (debug_disable_grid)
            r = model_collision( o->model, &relpos, &reldir, &relresult );
        // else r = grid_collision(o->mesh->grid, relpos, reldir, &relresult);
        if( r ) {
            vec_backtransform( result, &relresult, &o->camera );
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
