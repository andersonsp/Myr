#include "land.h"


GLchar* vs_glsl =
    "uniform mat4 mvp;"
    "attribute vec3 pos;\n"
    "attribute vec3 normal;\n"
    "attribute vec2 tex;\n"
    "varying vec2 tex_fs;\n"
    "varying vec4 norm_fs;\n"
    "void main() {\n"
    "    gl_Position = mvp*vec4(pos, 1.0);\n"
    "    norm_fs = vec4(normal, 1.0);\n"
    "    tex_fs = tex;\n"
    "}\n";

GLchar* fs_glsl =
    "varying vec2 tex_fs;\n"
    "varying vec4 norm_fs;\n"
    "uniform sampler2D sampler;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D( sampler, tex_fs );\n"
    "}\n";


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
    self->object = object_new( (Vec){ .0, 5.0, .0}, NULL, NULL );
    self->health = 1;
    return self;
}

void player_ai(Player *self) {
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

    Vec dir, orig = o->pos;
    object_move( o, kx, ky, kz );
    vec_sub( &dir, &o->pos, &orig );

    object_turn( o, g_radians(xm) );
    object_pitch( o, g_radians(ym) );

    Vec pos;
    Object *col = world_collision( world, &orig, &dir, 3.0f, &pos );
    if( col ) {
        // collide & slide
        o->pos = pos;
    }

    // Ground collision.
    // TODO: Jumping
    Vec forward, down = {0, -1, 0};
    col = world_collision(world, &o->pos, &down, 3.0f, &self->footpoint);
    if( col ) {
        o->pos = self->footpoint;
    } else {// uh oh - fell out of level
        if (o->pos.y > -100) o->pos.y--;
    }

    quat_vec_mul( &forward, &o->rot, &neg_z_axis );
    self->hit = world_collision( world, &o->pos, &forward, 0.0f, &self->hitpoint );
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
    // glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    mat4_persp( &persp, 60.0f, ar, 0.2f, 1000.0f );
    mat4_ortho( &ortho, width, height, 0.0f, 1.0f );

    // fnt = tex_font_new( "dejavu16.sfn" );
    // if(!fnt) g_fatal_error( "couldn't load dejavu16.sfn font" );

    world = world_new();
    level_mdl = model_load( "castle_map.iqm" );
    if( !level_mdl ) g_fatal_error( "couldn't load castle_map.iqm model" );


    Program p;
    p.vs = program_load_shader( vs_glsl, GL_VERTEX_SHADER );
    p.fs = program_load_shader( fs_glsl, GL_FRAGMENT_SHADER );
    if( !p.vs || !p.fs ) g_fatal_error( "Shader compilation failed\n" );

    const char *attribs[] = { "pos", "normal", "tex", 0 };
    if( !program_link(&p, attribs) ) g_fatal_error( "Shader link failed\n" );

    p.a_pos = glGetAttribLocation( p.object, "pos" );
    if( p.a_pos == -1 ) g_fatal_error( "Could not bind attribute pos\n" );

    p.a_normal = glGetAttribLocation( p.object, "normal" );
    if( p.a_normal == -1 ) g_fatal_error( "Could not bind attribute normal\n" );

    p.a_tex = glGetAttribLocation( p.object, "tex" );
    if( p.a_tex == -1 ) g_fatal_error( "Could not bind attribute tex\n" );

    p.u_mvp = glGetUniformLocation( p.object, "mvp" );
    if( p.u_mvp == -1) g_fatal_error( "Could not bind uniform mvp\n" );

    p.u_sampler = glGetUniformLocation( p.object, "sampler" );
    if( p.u_sampler == -1) g_fatal_error( "Could not bind uniform sampler\n" );


    landscape = object_new( (Vec){ .0f, .0f, .0f}, level_mdl, &p );
    world_add_object( world, landscape );

    player = player_new();

    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

void g_render( void *data ) {
    glEnable( GL_TEXTURE_2D );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    stats_drawn_objects = 0;
    Object *o = player->object;
    world_draw( world, o, &persp );

/*
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
*/
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
