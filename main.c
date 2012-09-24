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
Mesh *landscape_meshdata, *monster_meshdata, *monster2_meshdata;
Player *player;
Monster *monster[MONSTER_COUNT];
Object *landscape;

int boss;

int gamestate = 0;
int counter = 0;

int monsters_alive;



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

    Vector b = vector_scale(camera->b, kz * f);
    camera->p = vector_add(camera->p, b);

    Vector r = vector_scale(camera->r, kx * f);
    camera->p = vector_add(camera->p, r);

    Vector u = vector_scale(camera->u, ky * f);
    camera->p = vector_add(camera->p, u);

    float x = xm * 0.1 * G_PI / 180.0;
    camera_turn(camera, x);

    float y = ym * 0.1 * G_PI / 180.0;
    camera_pitch(camera, y);

    Object *col;
    Vector pos;
    Vector dir[] = {camera->r, camera->u, camera->b,
        vector_scale(camera->r, -1.0), vector_scale(camera->u, -1.0),
        vector_scale(camera->b, -1.0)};
    /* Push away from nearby geometry. */
    for (i = 0; i < 6; i++) {
        col = world_collision(world, camera->p, dir[i], &pos);
        if (col) {
            Vector diff = vector_sub(pos, camera->p);
            if (vector_dot(diff, diff) < 1) {
                camera->p = vector_add( pos, vector_scale(dir[i], -1) );
            }
        }
    }

    /* Ground collision. */
    // TODO: Jumping
    col = world_collision(world, camera->p, (Vector){0, -1, 0}, &self->footpoint);
    if (col) {
        Vector diff = vector_sub(self->footpoint, camera->p);
        if ( vector_dot(diff, diff) < 9.0 ) {
            camera->p = vector_add(self->footpoint, (Vector){0, 2, 0});
        } else { /* fall down */
            camera->p.y -= 0.5;
        }
    } else {/* uh oh - fell out of level */
        if (camera->p.y > -100) camera->p.y--;
    }

    Vector forward = vector_scale(camera->b, -1);
    self->hit = world_collision(world, camera->p, forward, &self->hitpoint);

//    if (self->shot_pause)
//        self->shot_pause--;
//    else
//    {
//        if (land_mouse_b())
//        {
//            self->shot_pause = 50;
//            if (self->hit)
//            {
//                self->hit->hits++;
//
//                Vector rgb;
//                if (self->hit == landscape)
//                {
//                    float c = land_rnd(0, 1);
//                    rgb = (Vector){c, c, c};
//                }
//                else
//                {
//                    float c = land_rnd(0, 0.5);
//                    rgb = (Vector){1, c, c};
//                }
//
//                for (i = 0; i < 11; i++)
//                {
//                    particle_add(self->hitpoint, rgb);
//                }
//            }
//        }
//    }
}





void g_configure( GConfig *conf ) {
    // width, height
    // title
    // flags : GC_MULTISAMPLING | GC_CORE_PROFILE | GC_FULLSCREEN | GC_HIDE_CURSOR | GC_VERTICAL_SYNC | GC_IGNORE_KEYREPEAT
    // gl_version
    // data

    conf->width = 800;
    conf->height = 600;
    conf->flags = GC_IGNORE_KEYREPEAT;
    conf->gl_version = 21;
    conf->data = NULL;
    conf->title = strdup("Land");
}

void g_initialize( int width, int height, void *data ) {
    glViewport(0, 0, width, height);

    glClearColor(0, 0.5, 0.5, 0);
    glClearDepth(1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float w = width;
    float h = height;
    float d = 0.1;
    glFrustum(-w * d / w, w * d / w, -h * d / w, h * d / w, d, 1000);

    glMatrixMode(GL_MODELVIEW);


    world = world_new();

    landscape_meshdata = landscape_mesh();
    monster_meshdata = monster_mesh();
    monster2_meshdata = monster2_mesh();

    landscape = object_new((Vector){0, 0, 0}, landscape_meshdata);
    world_add_object(world, landscape);

    // int i;
    // for (i = 0; i < MONSTER_COUNT; i++) {
    //     float x = g_rand(-10, 10);
    //     float z = g_rand(-10, 10);
    //     Object *ob = object_new((Vector){x, 25, z}, monster_meshdata);
    //     monster[i] = monster_new(ob);
    //     world_add_object(world, ob);
    // }

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


    glLoadIdentity();
    apply_camera(camera);

    {
        GLfloat ambient[] = { 0, 0, 0, 1.0 };
        //GLfloat diffuse[] = { 1, 0, 0, 1.0 };
        GLfloat emission[] = { 0.0, 0.0, 0.0, 1.0 };
        GLfloat specular[] = { 0, 0, 0, 0 };

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
        //glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1);

        glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    }

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

    {
        GLfloat lightpos[] = { 5, 5, 5, 1 };
        GLfloat ambient[] = { 0, 0, 0, 1.0 };
        GLfloat diffuse[] = { 1, 1, 1, 1.0 };
        GLfloat specular[] = { 0, 0, 0, 0 };

        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
        glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
        glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    }

    stats_drawn_objects = 0;
    world_draw(world, &player->camera);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    if (player->hit)
    {
        Vector hitpoint = player->hitpoint;
        Vector vl = vector_add(hitpoint, vector_scale(camera->r, 0.9));
        Vector vr = vector_sub(hitpoint, vector_scale(camera->r, 0.9));
        Vector vu = vector_add(hitpoint, vector_scale(camera->u, 0.9));
        Vector vd = vector_sub(hitpoint, vector_scale(camera->u, 0.9));
        Vector vl2 = vector_add(hitpoint, vector_scale(camera->r, 0.4));
        Vector vr2 = vector_sub(hitpoint, vector_scale(camera->r, 0.4));
        Vector vu2 = vector_add(hitpoint, vector_scale(camera->u, 0.4));
        Vector vd2 = vector_sub(hitpoint, vector_scale(camera->u, 0.4));

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

    if (gamestate == 0) {
        if (player->camera.p.y < -90) player->health = 0;
        if (player->health <= 0) gamestate = 1;
    } else {
        counter++;
    }

    // monsters_alive = 0;
    // int i;
    // for (i = 0; i < MONSTER_COUNT; i++) {
    //     monster_ai(monster[i]);
    //     if (monster[i]->object->hits < monster[i]->hitpoints) {
    //         monsters_alive++;
    //     }
    // }

    // if( monsters_alive == 0 ) {
    //     if( !boss ) {
    //         monster[0]->object->camera.p = (Vector){0, 100, -200};
    //         monster[0]->object->mesh = monster2_meshdata;
    //         monster[0]->object->hits = 0;
    //         monster[0]->hitpoints = 111;
    //         monster[0]->finished = 0;
    //         monster[0]->speed = 1;
    //         boss++;
    //     } else {
    //         gamestate = 2;
    //     }
    // }
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
