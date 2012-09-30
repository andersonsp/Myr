#include "land.h"


#define MONSTER_COUNT 33
#define PARTICLE_COUNT 33

extern Player *player;
extern World *world;
extern Object *landscape;
extern int stats_triangle_intersections;
extern int debug_disable_grid;
extern int stats_drawn_objects;

World *world;
Model *level_mdl;
Player *player;
// Monster *monster[MONSTER_COUNT];
Object *landscape;

int boss;

int gamestate = 0;
int counter = 0;

// int monsters_alive;



//
// Entities
//
int key_pressed[GK_KEY_MAX] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

Player *player_new(void) {
    Player *self = calloc(1, sizeof *self);
    self->camera.r.x = 1;
    self->camera.u.y = 1;
    self->camera.b.z = 1;
    self->health = 1;
    return self;
}

void player_ai(Player *self) {
    int i;
    Camera *camera = &self->camera;

    int kx = 0, ky = 0, kz = 0, xm = 0, ym = 0;
    if( key_pressed[GK_A] ) kx = -3;
    if( key_pressed[GK_D] ) kx = 3;
    if( key_pressed[GK_W] ) kz = -3;
    if( key_pressed[GK_S] ) kz = 3;
    if( key_pressed[GK_LEFT]  ) xm = -10;
    if( key_pressed[GK_RIGHT] ) xm = 10;
    if( key_pressed[GK_UP]    ) ym = -10;
    if( key_pressed[GK_DOWN]  ) ym = 10;

    float f = 0.1;

    Vec b = vec_scale(camera->b, kz * f);
    camera->p = vec_add(camera->p, b);

    Vec r = vec_scale(camera->r, kx * f);
    camera->p = vec_add(camera->p, r);

    Vec u = vec_scale(camera->u, ky * f);
    camera->p = vec_add(camera->p, u);

    float x = xm * 0.1 * G_PI / 180.0;
    camera_turn(camera, x);

    float y = ym * 0.1 * G_PI / 180.0;
    camera_pitch(camera, y);

    Object *col;
    Vec pos;
    Vec dir[] = {camera->r, camera->u, camera->b,
        vec_scale(camera->r, -1.0), vec_scale(camera->u, -1.0),
        vec_scale(camera->b, -1.0)};
    // Push away from nearby geometry.
    for( i = 0; i < 6; i++ ) {
        col = world_collision(world, camera->p, dir[i], &pos);
        if( col ) {
            Vec diff = vec_sub(pos, camera->p);
            if( vec_dot(diff, diff) < 1 ) {
                camera->p = vec_add( pos, vec_scale(dir[i], -1) );
            }
        }
    }

    // Ground collision.
    // TODO: Jumping
    col = world_collision(world, camera->p, (Vec){0, -1, 0}, &self->footpoint);
    if (col) {
        Vec diff = vec_sub(self->footpoint, camera->p);
        if ( vec_dot(diff, diff) < 9.0 ) {
            camera->p = vec_add(self->footpoint, (Vec){0, 2, 0});
        } else { // fall down
            camera->p.y -= 0.5;
        }
    } else {// uh oh - fell out of level
        if (camera->p.y > -100) camera->p.y--;
    }

    Vec forward = vec_scale(camera->b, -1);
    self->hit = world_collision(world, camera->p, forward, &self->hitpoint);
}


//
// SYSTEM
//

void g_configure( GConfig *conf ) {
    // width, height
    // title
    // flags : GC_MULTISAMPLING | GC_CORE_PROFILE | GC_FULLSCREEN | GC_HIDE_CURSOR | GC_VERTICAL_SYNC | GC_IGNORE_KEYREPEAT
    // gl_version
    // data

    conf->width = 800;
    conf->height = 600;
    conf->flags = GC_IGNORE_KEYREPEAT | GC_HIDE_CURSOR;
    conf->gl_version = 21;
    conf->data = NULL;
    conf->title = strdup("Land");
}

void g_initialize( int width, int height, void *data ) {
    glViewport(0, 0, width, height);

    glClearColor(0, 0.5, 0.5, 0);
    glClearDepth(1);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float w = width;
    float h = height;
    float d = 0.1;
    glFrustum(-w * d / w, w * d / w, -h * d / w, h * d / w, d, 1000);

    glMatrixMode(GL_MODELVIEW);


    world = world_new();
    level_mdl = model_load( "level_concept.iqm" );
    if( !level_mdl ) g_fatal_error( "couldn't load level_concept.iqm model" );

    landscape = object_new( (Vec){0, 0, 0}, level_mdl );
    world_add_object( world, landscape );

    player = player_new();
}

void g_render( void *data ) {
    Camera *camera = &player->camera;
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_CULL_FACE);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor4f(1, 1, 1, 1);

    glLoadIdentity();
    apply_camera(camera);


    GLfloat ambient[] = { 0, 0, 0, 1.0 };
    GLfloat emission[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat specular[] = { 0, 0, 0, 0 };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1);

    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);


    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

    GLfloat lightpos[] = { 5, 5, 5, 1 };
    GLfloat diffuse[] = { 1, 1, 1, 1.0 };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);


    stats_drawn_objects = 0;
    world_draw(world, &player->camera);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // render crosshair
    if( player->hit ) {
        Vec hitpoint = player->hitpoint;
        Vec vl = vec_add(hitpoint, vec_scale(camera->r, 0.9));
        Vec vr = vec_sub(hitpoint, vec_scale(camera->r, 0.9));
        Vec vu = vec_add(hitpoint, vec_scale(camera->u, 0.9));
        Vec vd = vec_sub(hitpoint, vec_scale(camera->u, 0.9));
        Vec vl2 = vec_add(hitpoint, vec_scale(camera->r, 0.4));
        Vec vr2 = vec_sub(hitpoint, vec_scale(camera->r, 0.4));
        Vec vu2 = vec_add(hitpoint, vec_scale(camera->u, 0.4));
        Vec vd2 = vec_sub(hitpoint, vec_scale(camera->u, 0.4));

        glBegin(GL_LINES);
        glColor4f(0, 1, 0, 1);
        glVertex3f(vl.x, vl.y, vl.z);
        glVertex3f(vl2.x, vl2.y, vl2.z);

        glVertex3f(vr.x, vr.y, vr.z);
        glVertex3f(vr2.x, vr2.y, vr2.z);

        glVertex3f(vu.x, vu.y, vu.z);
        glVertex3f(vu2.x, vu2.y, vu2.z);

        glVertex3f(vd.x, vd.y, vd.z);
        glVertex3f(vd2.x, vd2.y, vd2.z);

        glEnd();
    }
}

void g_update( unsigned int milliseconds, void *data ) {
    if( counter < 300 ) player_ai(player);

    if( gamestate == 0 ) {
        if (player->camera.p.y < -90) player->health = 0;
        if (player->health <= 0) gamestate = 1;
    } else {
        counter++;
    }

}

int g_handle_event( GEvent *event, void *data ) {
    switch( event->type ){
        case GE_KEYDOWN:{
            if(event->value == GK_ESCAPE ) return 0;
            else key_pressed[event->value] = 1;
            break;
        }
        case GE_KEYUP:{
            if(event->value == GK_ESCAPE ) return 0;
            else key_pressed[event->value] = 0;
            break;
        }
    }
    return 1;
}

void g_cleanup( void *data ) {
    g_debug_str( "Exit cleanly...\n" );
}
