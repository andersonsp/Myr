#include "land.h"


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
    const float ar = (float) width / (float) height;
    glViewport(0, 0, width, height);

    glClearColor(0, 0.5, 0.5, 0);
    glClearDepth(1);
    glDisable(GL_FOG);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    // glFrontFace(GL_CW);

    glEnable( GL_TEXTURE_2D );  // Texture please
    glDisable( GL_LIGHTING );   // No lighting
    glEnable( GL_BLEND );       // Enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Source alpha-based weighted average blending

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
}

void g_render( void *data ) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // draw 3D scene
}

void g_update( unsigned int milliseconds, void *data ) {

}

int g_handle_event( GEvent *event, void *data ) {
    switch( event->type ) {
        case GE_KEYDOWN: {
            if( event->value == GK_ESCAPE ) return 0;
//            switch( event->value ){
//                case GK_ESCAPE: key_pressed[]return 0;
//                case GK_Q: ; break;
//                case GK_E: ; break;
//                case GK_S: ; break;
//                case GK_W: ; break;
//                case GK_A: ; break;
//                case GK_D: ; break;
//                case GK_LEFT: ; break;
//                case GK_RIGHT: ; break;
//                case GK_UP: ; break;
//                case GK_DOWN: ; break;
//                case GK_SPACE: break;
//            }
            break;
        }
        case GE_KEYUP: {
            // if(event->value == GK_ESCAPE ) return 0;
            // else key_pressed[event->value] = 1;
            break;
        }
    }
    return 1;
}

void g_cleanup( void *data ) {
    g_debug_str( "Exit cleanly...\n" );
}
