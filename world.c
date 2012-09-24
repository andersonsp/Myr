#include "land.h"

int debug_disable_grid;
int stats_drawn_objects;

Vertex *vertex_new(float x, float y, float z, float nx, float ny, float nz) {
    Vertex *self = calloc(1, sizeof *self);
    self->p.x = x;
    self->p.y = y;
    self->p.z = z;
    self->n.x = nx;
    self->n.y = ny;
    self->n.z = nz;
    return self;
}

Triangle *triangle_new(Vertex *v1, Vertex *v2, Vertex *v3, float r, float g, float b, float a) {
    Triangle *self = calloc(1, sizeof *self);
    self->v1 = v1;
    self->v2 = v2;
    self->v3 = v3;
    self->r = r;
    self->g = g;
    self->b = b;
    self->a = a;
    return self;
}

World *world_new(void) {
    World *self = calloc(1, sizeof *self);
    return self;
}

void world_add_object(World *self, Object *object) {
    self->o = realloc(self->o, (self->on + 1) * sizeof *self->o);
    self->o[self->on] = object;
    self->on++;
}

Object *object_new(Vector pos, Mesh *mesh) {
    Object *self = calloc(1, sizeof *self);
    self->camera.p = pos;
    self->camera.r.x = 1;
    self->camera.u.y = 1;
    self->camera.b.z = 1;
    self->mesh = mesh;
    return self;
}

void triangle_draw(Triangle *self) {
    glBegin(GL_TRIANGLES);
    glColor4f(self->r, self->g, self->b, self->a);
    glNormal3f(self->v1->n.x, self->v1->n.y, self->v1->n.z);
    glVertex3f(self->v1->p.x, self->v1->p.y, self->v1->p.z);
    glNormal3f(self->v2->n.x, self->v2->n.y, self->v2->n.z);
    glVertex3f(self->v2->p.x, self->v2->p.y, self->v2->p.z);
    glNormal3f(self->v3->n.x, self->v3->n.y, self->v3->n.z);
    glVertex3f(self->v3->p.x, self->v3->p.y, self->v3->p.z);
    glEnd();
}

void mesh_draw(Mesh *self) {
    int i;
    for (i = 0; i < self->tn; i++) {
        triangle_draw(self->t[i]);
    }
}

void object_draw(Object *self) {
    stats_drawn_objects++;
    glPushMatrix();
    apply_orientation(&self->camera);
    // TODO: use octree for possible early exclusion of invisible triangles?
    // right now we draw everything, even if outside the viewing frustum
    mesh_draw(self->mesh);
    glPopMatrix();
}

void world_draw(World *self, Camera *camera) {
    int i;
    for (i = 0; i < self->on; i++) {
        Object *o = self->o[i];
        Vector a = vector_sub(o->camera.p, camera->p);
        Vector c = vector_cross(a, camera->b);
        float sidesquare = vector_dot(c, c);
        float distsquare = vector_dot(a, a);
        float dot = vector_dot(camera->b, a);
        if (dot > 0 && distsquare > o->mesh->rr) continue;
        if (sidesquare > distsquare + o->mesh->rr) continue;

        object_draw(o);
    }
}

int mesh_collision(Mesh *self, Vector pos, Vector dir, Vector *result) {
    int i;
    for (i = 0; i < self->tn; i++) {
        if (ray_intersects_triangle(pos, dir, self->t[i]->v1->p,
            self->t[i]->v2->p, self->t[i]->v3->p, result))
            return 1;
    }
    return 0;
}

int object_collision(Object *o, Vector pos, Vector dir, Vector *result) {
    Vector a = vector_sub(o->camera.p, pos);
    Vector c = vector_cross(a, dir);
    float dd = vector_dot(c, c);
    if (dd < o->mesh->rr) {
        Vector dirpos = vector_add(pos, dir);
        Vector relpos = vector_transform(pos, &o->camera);
        dirpos = vector_transform(dirpos, &o->camera);
        Vector reldir = vector_sub(dirpos, relpos);
        Vector relresult;
        int r;
        // if (debug_disable_grid)
            r = mesh_collision(o->mesh, relpos, reldir, &relresult);
        // else r = grid_collision(o->mesh->grid, relpos, reldir, &relresult);
        if( r ) {
            Vector where = vector_backtransform(relresult, &o->camera);
            *result = where;
            return 1;
        }
    }
    return 0;
}

Object *world_collision(World *self, Vector pos, Vector dir, Vector *result) {
    int i;
    float min = 10000000000;
    Object *col = NULL;
    for (i = 0; i < self->on; i++) {
        Object *o = self->o[i];
        Vector temp;
        if( object_collision(o, pos, dir, &temp) ) {
            float d = vector_len(vector_sub(temp, pos));
            if( d < min ) {
                *result = temp;
                col = o;
                min = d;
            }
        }
    }
    return col;
}

void mesh_calculate_bounding_sphere(Mesh *self) {
    int i;
    float rr = 0;
    for (i = 0; i < self->vn; i++) {
        float dx = self->v[i]->p.x;
        float dy = self->v[i]->p.y;
        float dz = self->v[i]->p.z;
        float dd = dx * dx + dy * dy + dz * dz;
        if (dd > rr) rr = dd;
    }
    self->rr = rr;
    printf("Bounding sphere radius: %f\n", sqrt(rr));
}

// void mesh_create_grid(Mesh *self) {
//     self->grid = grid_new(self, OCTREE_SIZE);
// }

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
