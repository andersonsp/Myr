#include "land.h"


// #define MONSTER_COUNT 33
// #define PARTICLE_COUNT 33

extern int stats_drawn_objects;

typedef struct {
    Object *object;
    Object *hit;
    Vec hitpoint;
    Vec footpoint;

    float health;
} Player;

typedef struct {
    Object *object;
    Vec velocity;
    int tick;
    int hitpoints;

    Vec avoid;
    int avoid_time;

    float speed;
    int finished;
} Monster;


World *world;
Model *level_mdl;
Player *player;
// Monster *monster[MONSTER_COUNT];
Object *landscape;

int boss;

int gamestate = 0;
int counter = 0;

Mat4 ortho, persp, view;
TexFont* fnt;

// int monsters_alive;

static Vec x_axis = {1.0f, 0.0f, 0.0f},
    y_axis = {0.0f, 1.0f, 0.0f},
    neg_z_axis = {0.0f, 0.0f, -1.0f};

//
// Entities
//
int key_pressed[GK_KEY_MAX] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

Player *player_new(void) {
    Player *self = g_new0(Player, 1);
    self->object = object_new( (Vec){ .0, .0, .0}, NULL );
    self->health = 1;
    return self;
}

void player_ai(Player *self) {
    int i;
    Object *o = self->object;
    float kx = .0f, ky = .0f, kz = .0f;
    int xm = 0, ym = 0;
    if( key_pressed[GK_A] ) kx = -0.3f;
    if( key_pressed[GK_D] ) kx = 0.3f;
    if( key_pressed[GK_W] ) kz = 0.3f;
    if( key_pressed[GK_S] ) kz = -0.3f;
    if( key_pressed[GK_LEFT]  ) xm = 1.0f;
    if( key_pressed[GK_RIGHT] ) xm = -1.0f;
    if( key_pressed[GK_UP]    ) ym = 1.0f;
    if( key_pressed[GK_DOWN]  ) ym = -1.0f;

    object_move( o, kx, ky, kz );

    object_turn( o, g_radians(xm) );
    object_pitch( o, g_radians(ym) );

    Object *col;
    Vec pos;
    Vec dir[6];
    quat_vec_mul( &dir[0], &o->rot, &x_axis );
    dir[1] = y_axis;
    vec_cross( &dir[2], &dir[0], &dir[1] );
    vec_scale( &dir[3], &dir[0], -1.0 );
    vec_scale( &dir[4], &dir[1], -1.0 );
    vec_scale( &dir[5], &dir[2], -1.0 );
    // Push away from nearby geometry.
    for( i = 0; i < 6; i++ ) {
        col = world_collision( world, &o->pos, &dir[i], &pos );
        if( col ) {
            Vec diff, s_dir;
            vec_sub( &diff, &pos, &o->pos );
            if( vec_dot(&diff, &diff) < 1 ) {
                vec_add( &o->pos, &pos, vec_scale(&s_dir, &dir[i], -1) );
            }
        }
    }

    // Ground collision.
    // TODO: Jumping
    Vec forward, down = {0, -1, 0}, fall_delta = {0, 2, 0};
    col = world_collision(world, &o->pos, &down, &self->footpoint);
    if( col ) {
        Vec diff;
        vec_sub( &diff, &self->footpoint, &o->pos );
        if ( vec_dot(&diff, &diff) < 9.0 ) {
            vec_add( &o->pos, &self->footpoint, &fall_delta );
        } else { // fall down
            o->pos.y -= 0.5;
        }
    } else {// uh oh - fell out of level
        if (o->pos.y > -100) o->pos.y--;
    }

    quat_vec_mul( &forward, &o->rot, &neg_z_axis );
    self->hit = world_collision( world, &o->pos, &forward, &self->hitpoint );
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
    float ar = (float) width / (float) height;
    glViewport(0, 0, width, height);

    glClearColor(0, 0.5, 0.5, 0);
    glClearDepth(1);

    glEnable( GL_BLEND );
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    mat4_persp( &persp, 60.0f, ar, 0.2f, 1000.0f );
    glMultMatrixf( persp.m );

    glMatrixMode(GL_MODELVIEW);

    mat4_ortho( &ortho, width, height, 0.0f, 1.0f );

    fnt = tex_font_new( "dejavu16.sfn" );
    if(!fnt) g_fatal_error( "couldn't load dejavu16.sfn font" );

    world = world_new();
    level_mdl = model_load( "castle_map.iqm" );
    if( !level_mdl ) g_fatal_error( "couldn't load castle_map.iqm model" );

    landscape = object_new( (Vec){ .0f, .0f, .0f}, level_mdl );
    world_add_object( world, landscape );

    player = player_new();
}

void g_render( void *data ) {
    glEnable( GL_TEXTURE_2D );
    // glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_CULL_FACE);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor4f(1, 1, 1, 1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMultMatrixf( persp.m );
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    apply_camera( player->object );


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
    Object *o = player->object;
    world_draw( world, o );

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    // render crosshair
    if( player->hit ) {
        Vec hitpoint = player->hitpoint;
        Vec vl, vr, vu, vd, vl2, vr2, vu2, vd2;
        //until I learn how to do it the right way
        Vec r, u;

        quat_vec_mul( &r, &o->rot, &x_axis );
        quat_vec_mul( &u, &o->rot, &y_axis );

        vec_add( &vl, &hitpoint, vec_scale(&vl, &r, 0.9) );
        vec_sub( &vr, &hitpoint, vec_scale(&vr, &r, 0.9) );
        vec_add( &vu, &hitpoint, vec_scale(&vu, &u, 0.9) );
        vec_sub( &vd, &hitpoint, vec_scale(&vd, &u, 0.9) );
        vec_add( &vl2, &hitpoint, vec_scale(&vl2, &r, 0.4));
        vec_sub( &vr2, &hitpoint, vec_scale(&vr2, &r, 0.4));
        vec_add( &vu2, &hitpoint, vec_scale(&vu2, &u, 0.4));
        vec_sub( &vd2, &hitpoint, vec_scale(&vd2, &u, 0.4));

        glBegin( GL_LINES );
        glColor4f( 0, 1, 0, 1 );
            glVertex3f( vl.x, vl.y, vl.z );
            glVertex3f( vl2.x, vl2.y, vl2.z );

            glVertex3f( vr.x, vr.y, vr.z );
            glVertex3f( vr2.x, vr2.y, vr2.z );

            glVertex3f( vu.x, vu.y, vu.z );
            glVertex3f( vu2.x, vu2.y, vu2.z );

            glVertex3f( vd.x, vd.y, vd.z );
            glVertex3f( vd2.x, vd2.y, vd2.z );
        glEnd();
    }


    // draw 2D composition layer ( HUD )
    char buffer[512];
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMultMatrixf( ortho.m );
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor4f( 1.0, 1.0, 1.0, .3 );
    glBegin(GL_QUADS);
        glVertex3f( -395.0f, 295.0f, 0.0f);              // Top Left
        glVertex3f( 1.0f, 295.0f, 0.0f);              // Top Right
        glVertex3f( 1.0f, 180.0f, 0.0f);              // Bottom Right
        glVertex3f( -395.0, 180.0, 0.0 );              // Bottom Left
    glEnd();                            // Done Drawing The Quad

    glEnable( GL_TEXTURE_2D );
    glColor4f( 0.0, 0.0, 0.0, 1 );
    glTranslatef( -390, 280.0, 0.0 );
    tex_font_render( fnt, "Testing Myr Engine by aSP" );

    // // glTranslatef( 0.0, -15.0, 0.0 );
    // // sprintf(buffer, "Entities drawn: %d", count);
    // // g_font_render( fnt, buffer );

    glTranslatef( 0.0, -15.0, 0.0 );
    sprintf(buffer, "Player loc: %1.3f, %1.3f, %1.3f", o->pos.x, o->pos.y, o->pos.z );
    tex_font_render( fnt, buffer );

    // // glTranslatef( 0.0, -15.0, 0.0 );
    // // sprintf(buffer, "Collided entity: %d", collide_entity );
    // // g_font_render( fnt, buffer );

    glEnable( GL_DEPTH_TEST );
}

void g_update( unsigned int milliseconds, void *data ) {
    if( counter < 300 ) player_ai(player);

    if( gamestate == 0 ) {
        if (player->object->pos.y < -90) player->health = 0;
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
