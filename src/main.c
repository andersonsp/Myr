#include "land.h"

#define HEADING_SPEED 90.0f
#define FORWARD_SPEED 6.0f
#define GRAVITY -9.8f

// GLchar* vs_glsl =
//     "uniform mat4 mvp;"
//     "attribute vec3 pos;\n"
//     "attribute vec3 normal;\n"
//     "attribute vec2 tex;\n"
//     "varying vec2 tex_fs;\n"
//     "varying vec4 norm_fs;\n"
//     "void main() {\n"
//     "    gl_Position = mvp*vec4(pos, 1.0);\n"
//     "    norm_fs = vec4(normal, 1.0);\n"
//     "    tex_fs = tex;\n"
//     "}\n";

// GLchar* fs_glsl =
//     "varying vec2 tex_fs;\n"
//     "varying vec4 norm_fs;\n"
//     "uniform sampler2D sampler;\n"
//     "void main() {\n"
//     "    gl_FragColor = texture2D( sampler, tex_fs );\n"
//     "}\n";

GLchar* skinned_model_vs_glsl =
    "uniform mat4 mvp;"
    "uniform mat4 bones[60];\n"
    "attribute vec3 pos;\n"
    "attribute vec3 normal;\n"
    "attribute vec2 tex;\n"
    "attribute vec4 tangent;\n"
    "attribute vec4 bone_id;\n"
    "attribute vec4 bone_weight;\n"
    "varying vec2 tex_coord;\n"
    "varying vec3 norm_fs;\n"
    "void main(void) {\n"
    "    mat4 m = bones[int(bone_id.x)] * bone_weight.x;\n"
    "        m += bones[int(bone_id.y)] * bone_weight.y;\n"
    "        m += bones[int(bone_id.z)] * bone_weight.z;\n"
    "        m += bones[int(bone_id.w)] * bone_weight.w;\n"
    "    gl_Position = mvp*(m*vec4(pos, 1.0));\n"
    "    mat3 madjtrans = mat3(cross(m[1].xyz, m[2].xyz), cross(m[2].xyz, m[0].xyz), cross(m[0].xyz, m[1].xyz));\n"
    "    norm_fs = normal * madjtrans;\n"
    "    vec3 mtangent = tangent.xyz * madjtrans; // tangent not used, just here as an example\n"
    "    vec3 mbitangent = cross(norm_fs, mtangent) * tangent.w; // bitangent not used, just here as an example\n"
    "    tex_coord = tex;\n"
    "}\n";

GLchar* skinned_model_fs_glsl =
    "varying vec2 tex_coord;\n"
    "varying vec3 norm_fs;\n"
    "uniform sampler2D sampler;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D( sampler, tex_coord );\n"
    "}\n";


typedef struct {
    Object *object;
    Object *hit;
    Vec hitpoint;
    Vec footpoint;

    float health;
} Player;

World *world;
Model *level_mdl, *player_mdl;
Player *player;
Object *landscape;
Camera main_cam;

Mat4 ortho;
TexFont* fnt;

Vec eye = { 0.0, 2.0, 4.0 };

//
// Entities
//
int key_pressed[GK_KEY_MAX] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

Player *player_new( Vec pos, Model *mdl, Program* program ) {
    Player *self = g_new0(Player, 1);
    self->object = object_new( pos, mdl, program );
    self->health = 1;
    return self;
}

void player_ai(Player *self, int millis) {
    Object *o = self->object;
    float kx = .0f, ky = .0f, kz = .0f;
    float xm = 0.0f, ym = 0.0f;
    if( key_pressed[GK_A] ) kx = -FORWARD_SPEED;
    if( key_pressed[GK_D] ) kx = FORWARD_SPEED;
    if( key_pressed[GK_W] ) kz = -FORWARD_SPEED;
    if( key_pressed[GK_S] ) kz = FORWARD_SPEED;
    if( key_pressed[GK_LEFT]  ) xm = HEADING_SPEED;
    if( key_pressed[GK_RIGHT] ) xm = -HEADING_SPEED;
    if( key_pressed[GK_UP]    ) ym = HEADING_SPEED;
    if( key_pressed[GK_DOWN]  ) ym = -HEADING_SPEED;

    Vec speed = {kx, ky, kz};
    Vec s_orient, orient = {g_radians(ym), g_radians(xm), 0.0f};

    if( orient.y != 0.0f) {
        Quat rot;
        vec_scale( &s_orient, &orient, millis/1000.0 );
        quat_from_axis_angle( &rot, &y_axis, s_orient.y );

        // if( kz < 0 ) quat_invert( &rot, &rot );
        quat_mul( &o->rot, &rot, &o->rot );
        quat_normalize( &o->rot, &o->rot );
    }

    vec_scale( &speed, &speed, millis/1000.0 );
    quat_vec_mul( &speed, &o->rot, &speed );
    vec_add( &o->pos, &o->pos, &speed );

    // Vec pos;
    // Object *col = world_collision( world, &orig, &dir, 3.0f, &pos );
    // if( col ) {
    //     // collide & slide
    //     o->pos = pos;
    // }

    // // Ground collision.
    // // TODO: Jumping
    Vec forward, cam_pos, down = {0, -1, 0};
    // col = world_collision(world, &o->pos, &down, 3.0f, &self->footpoint);
    // if( col ) {
    //     o->pos = self->footpoint;
    //     // g_debug_str("wtf!!\n");
    // } else {// uh oh - fell out of level
    //     if (o->pos.y > -100) o->pos.y--;
    // }

    quat_vec_mul( &forward, &o->rot, &neg_z_axis );
    self->hit = world_collision( world, &o->pos, &forward, 0.0f, &self->hitpoint );

    //
    // TPS Camera
    // TODO: move this code somewhere else
    // main_cam.target = o->pos;
    // main_cam.heading = -orient.y; //( kz < 0 ? orient.y : -orient.y );
    // main_cam.pitch = orient.x;

    camera_update( &main_cam, &o->pos, orient.x, -orient.y, millis );

    o->anim_frame += (millis/100.0);
}


void camera_get_fps_view( Mat4* view, Object *o ) {
    Vec trans;
    Quat inv_rot;
    quat_invert( &inv_rot, &o->rot );
    quat_vec_mul( &trans, &inv_rot, vec_scale(&trans, &o->pos, -1.0f) );
    mat4_from_quat_vec( view, &inv_rot, &trans );
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

    Program p;
    p.vs = program_load_shader( skinned_model_vs_glsl, GL_VERTEX_SHADER );
    p.fs = program_load_shader( skinned_model_fs_glsl, GL_FRAGMENT_SHADER );
    if( !p.vs || !p.fs ) g_fatal_error( "Shader compilation failed\n" );

    const char *attribs[] = { "pos", "normal", "tex", "tangent", "bone_id", "bone_weight", 0 };
    const char *uniforms[] = { "mvp", "sampler", "bones", 0 };
    if( !program_link(&p, attribs, uniforms) ) g_fatal_error( "Shader link failed\n" );


    fnt = tex_font_new( "dejavu16.sfn" );
    if(!fnt) g_fatal_error( "couldn't load dejavu16.sfn font" );

    world = world_new();
    // level_mdl = model_load( "castle_map.iqm" );
    // if( !level_mdl ) g_fatal_error( "couldn't load castle_map.iqm model" );

    player_mdl = model_load( "player_test.lmesh" );
    if( !player_mdl ) g_fatal_error( "couldn't load player_test.lmesh model" );
    model_setup_buffers( player_mdl );

    // landscape = object_new( (Vec){ .0f, .0f, .0f}, level_mdl, &p );
    // world_add_object( world, landscape );

    player = player_new( (Vec){ .0, -2.0, .0}, player_mdl, &p );
    world_add_object( world, player->object );


    mat4_ortho( &ortho, width, height, 0.0f, 1.0f );

    camera_init( &main_cam );
    mat4_persp( &main_cam.projection, 60.0f, ar, 0.2f, 1000.0f );
    camera_look_at( &main_cam, &eye, &player->object->pos, &y_axis );

    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

void g_render( void *data ) {
    glEnable( GL_TEXTURE_2D );
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // draw 3D scene
    world_draw( world, &main_cam );

    // draw 2D composition layer ( HUD )
    glUseProgram(0);
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    char buffer[512];
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMultMatrixf( ortho.m );
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor4f( 1.0, 1.0, 1.0, .3 );
    glBegin(GL_QUADS);
        glVertex3f( -395.0f, 295.0f, 0.0f);              // Top Left
        glVertex3f( -395.0, 180.0, 0.0 );              // Bottom Left
        glVertex3f( 1.0f, 180.0f, 0.0f);              // Bottom Right
        glVertex3f( 1.0f, 295.0f, 0.0f);              // Top Right
    glEnd();                            // Done Drawing The Quad

    glEnable( GL_TEXTURE_2D );

    glColor4f( 0.0, 0.0, 0.0, 1 );
    glTranslatef( -390, 280.0, 0.0 );
    tex_font_render( fnt, "Testing Myr Engine by aSP" );

    // // glTranslatef( 0.0, -15.0, 0.0 );
    // // sprintf(buffer, "Entities drawn: %d", count);
    // // g_font_render( fnt, buffer );

    Object *o = player->object;
    glTranslatef( 0.0, -15.0, 0.0 );
    sprintf(buffer, "Player loc: %1.3f, %1.3f, %1.3f", o->pos.x, o->pos.y, o->pos.z );
    tex_font_render( fnt, buffer );

    glEnable( GL_DEPTH_TEST );
}

void g_update( unsigned int milliseconds, void *data ) {
    player_ai( player, milliseconds);
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
