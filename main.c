#include "myr.h"

int keypressed[GK_KEY_MAX];

GMat4 projection, model, ortho;

// GModel *mdl, *sky, *map;
GFont *fnt;


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
    conf->title = strdup("Myr");
}

void g_initialize( int width, int height, void *data ) {
    const float ar = (float) width / (float) height;
    glViewport(0, 0, width, height);

    glClearColor(0, 0.5, 0.5, 0);
    glClearDepth(1);
    glDisable(GL_FOG);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    glEnable( GL_TEXTURE_2D );  // Texture please
    glDisable( GL_LIGHTING );   // No lighting
    glEnable( GL_BLEND );       // Enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Source alpha-based weighted average blending

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    g_mat4_persp( &projection, 60.0, ar, 1.0, 100.0 );
    glMultMatrixf( (GLfloat*) &projection );

    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    g_mat4_ortho( &ortho, width, height, 0.0, 1.0 );


    fnt = g_font_new( "dejavu16.sfn" );
    if(!fnt) g_fatal_error( "couldn't load dejavu16.sfn font\n" );
}

void g_render( void *data ) {
    char buffer[512];

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor4f( 1.0, 1.0, 1.0, 1.0 );       // White



    // draw 2D composition layer ( HUD )
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMultMatrixf( (GLfloat*) &ortho );
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable( GL_DEPTH_TEST );

    glColor4f( 0.0, 0.0, 0.0, 1 );
    glTranslatef( -390, 280.0, 0.0 );
    g_font_render( fnt, "Testing Myr Engine by aSP" );

    // glTranslatef( 0.0, -15.0, 0.0 );
    // sprintf(buffer, "Player loc: %1.3f, %1.3f, %1.3f", player.loc.x, player.loc.y, player.loc.z );
    // g_font_render( fnt, buffer );

    glEnable( GL_DEPTH_TEST );
}

void g_update( unsigned int milliseconds, void *data ) {

}

int g_handle_event( GEvent *event, void *data ) {
    switch( event->type ){
        case GE_KEYDOWN:{
            if( event->value == GK_ESCAPE ) return 0;
            else keypressed[event->value] = GL_TRUE;
            break;
        }
        case GE_KEYUP:{
            keypressed[event->value] = GL_FALSE;
            break;
        }
    }
    return 1;
}

void g_cleanup( void *data ) {
    g_debug_str( "Exit cleanly...\n" );
}
