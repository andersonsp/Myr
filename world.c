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
    apply_orientation( &self->camera );

    model_draw( self->model, 0 );
    glPopMatrix();
}

void world_draw(World *self, Camera *camera) {
    int i;
    for( i = 0; i < self->on; i++ ) {
        Object *o = self->o[i];
        Vec a = vec_sub(o->camera.p, camera->p);
        Vec c = vec_cross(a, camera->b);
        float sidesquare = vec_dot(c, c);
        float distsquare = vec_dot(a, a);
        float dot = vec_dot(camera->b, a);
        if( dot > 0 && distsquare > o->rr ) continue;
        if( sidesquare > distsquare + o->rr ) continue;

        object_draw( o );
    }
}

int object_collision(Object *o, Vec pos, Vec dir, Vec *result) {
    Vec a = vec_sub(o->camera.p, pos);
    Vec c = vec_cross(a, dir);
    float dd = vec_dot(c, c);
    if( dd < o->rr ) {
        Vec dirpos = vec_add( pos, dir );
        Vec relpos = vec_transform( pos, &o->camera );
        dirpos = vec_transform( dirpos, &o->camera );
        Vec reldir = vec_sub( dirpos, relpos );
        Vec relresult;
        int r;
        // if (debug_disable_grid)
            r = model_collision( o->model, relpos, reldir, &relresult );
        // else r = grid_collision(o->mesh->grid, relpos, reldir, &relresult);
        if( r ) {
            Vec where = vec_backtransform( relresult, &o->camera );
            *result = where;
            return 1;
        }
    }
    return 0;
}

Object *world_collision(World *self, Vec pos, Vec dir, Vec *result) {
    int i;
    float min = 10000000000;
    Object *col = NULL;
    for (i = 0; i < self->on; i++) {
        Object *o = self->o[i];
        Vec temp;
        if( object_collision(o, pos, dir, &temp) ) {
            float d = vec_len(vec_sub(temp, pos));
            if( d < min ) {
                *result = temp;
                col = o;
                min = d;
            }
        }
    }
    return col;
}

void apply_camera(Camera *camera) {
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

void apply_orientation(Camera *camera) {
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
