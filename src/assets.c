#include "land.h"

typedef struct {
    char id[4];
    int version;
    int tex_w, tex_h;
    int start, end;
    int avgh, avgw;
} Font_header;

typedef struct {
    float u0, v0, u1, v1;
    float x0, y0, x1, y1;

    float advance;
} Glyph;

struct _Font {
    unsigned int tex;
    int start, end;
    Glyph glyph[1];
};

// Rigth now it uses only the fixed function pipeline
// in the future a streaming VBO will be used if available
//
TexFont* tex_font_new( char *filename ){
    FILE* filein;
    Font_header header;
    TexFont* fnt = NULL;
    unsigned char * pixels;

    char filepath[256];
    sprintf( filepath, "../data/fonts/%s", filename );
    filein = fopen(filepath, "rb");
    if( !filein ) return NULL;

    fread( &header, sizeof(Font_header), 1, filein );
    if( strncmp(header.id, "SFNT", 4) != 0 || header.version != 3 ) goto error;

    fnt = (TexFont*) malloc( sizeof(TexFont) + sizeof(Glyph)*(header.end - header.start) );
    if( !fnt ) goto error;

    pixels = g_new( unsigned char, header.tex_w*header.tex_h );
    if( !pixels ) goto error;

    fnt->start = header.start;
    fnt->end = header.end;

    fread( fnt->glyph, sizeof(Glyph), (header.end - header.start), filein );
    fread( pixels, 1, (header.tex_w*header.tex_h), filein );

    glGenTextures(1, &fnt->tex);
    glBindTexture(GL_TEXTURE_2D, fnt->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, header.tex_w, header.tex_h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    g_free( pixels );
    fclose( filein );
    return fnt;

error:
    g_debug_str( "%s: error while loading\n", filename );
    if( fnt ) free( fnt );
    if( pixels ) g_free( pixels );
    if( filein ) fclose( filein );
    return NULL;
}

void tex_font_render ( TexFont *fnt, char *str ){
    if(!fnt) return;

    int x = 0, y = 0, c;
    glBindTexture( GL_TEXTURE_2D, fnt->tex );
    glBegin( GL_QUADS );
    while( *str ) {
        if( *str >= fnt->start && *str < fnt->end ) {
            c = *str - fnt->start;
            glTexCoord2f(fnt->glyph[ c ].u0, fnt->glyph[ c ].v1); glVertex2f( x+fnt->glyph[ c ].x0, y+fnt->glyph[ c ].y0 );
            glTexCoord2f(fnt->glyph[ c ].u1, fnt->glyph[ c ].v1); glVertex2f( x+fnt->glyph[ c ].x1, y+fnt->glyph[ c ].y0 );
            glTexCoord2f(fnt->glyph[ c ].u1, fnt->glyph[ c ].v0); glVertex2f( x+fnt->glyph[ c ].x1, y+fnt->glyph[ c ].y1 );
            glTexCoord2f(fnt->glyph[ c ].u0, fnt->glyph[ c ].v0); glVertex2f( x+fnt->glyph[ c ].x0, y+fnt->glyph[ c ].y1 );

            x += fnt->glyph[ c ].advance;
        }
        ++str;
    }
    glEnd();
}

//
// Texture
//
int texture_load( Texture *tex, const char* filename ) {
    unsigned char *pix, tmp, *src, *dst;
    int pixsize, k, n, i,j;

    FILE* tga_file;
    unsigned char buf[ 18 ];

    char filepath[256];
    sprintf( filepath, "../data/textures/%s", filename );

    tga_file = fopen( filepath, "rb" );
    if( tga_file == NULL ) return 0;

    fread( buf, 18, 1, tga_file );

    // Interpret header (endian independent parsing)
    int idlen      = (int) buf[0];
    int type       = (int) buf[2];
    int width      = (int) buf[12] | (((int) buf[13]) << 8);
    int height     = (int) buf[14] | (((int) buf[15]) << 8);
    int hbpp       = (int) buf[16];
    int imageinfo  = (int) buf[17];

    // Validate TGA header (is this a TGA file?)
    if( (type == 2 || type == 3 || type == 10 || type == 11) && (hbpp == 8 || hbpp == 24 || hbpp == 32) ) {
        fseek( tga_file, idlen, SEEK_CUR ); // Skip the ID field
    } else {
        goto error;
    }

    int bytes_per_pixel = hbpp / 8;     // Bytes per pixel (pixel data - unexpanded)
    pixsize = width * height * bytes_per_pixel;  // Size of pixel data

    pix = g_new( unsigned char, pixsize );
    if( !pix ) goto error;

    // Read pixel data from file
    if( type == 10 || type == 11 ) {
        int i, size;
        GLubyte packet_header, rgba[4];
        GLubyte *ptr = pix;

        while( ptr < pix + pixsize ) {
            packet_header = (GLubyte) fgetc( tga_file ); // Read first byte
            size = 1 + (packet_header & 0x7f);

            if( packet_header & 0x80 ) { // Run-length packet
                fread( rgba, sizeof (GLubyte), bytes_per_pixel, tga_file );
                for( i = 0; i < size; ++i, ptr += bytes_per_pixel ) {
                    for( k = 0; k < bytes_per_pixel; k++ ) ptr[k] = rgba[k];
                }
            } else { // Non run-length packet
                for( i = 0; i < size; ++i, ptr += bytes_per_pixel )
                    fread( ptr, sizeof(GLubyte), bytes_per_pixel, tga_file );
            }
        }
    } else {
        fread( pix, pixsize, 1, tga_file );
    }

    // Convert image pixel format (BGR -> RGB or BGRA -> RGBA)
    if( bytes_per_pixel == 3 || bytes_per_pixel == 4 ) {
        src = pix;
        dst = &pix[ 2 ];
        for( n = 0; n < height * width; n++ ) {
            tmp  = *src;
            *src = *dst;
            *dst = tmp;
            src += bytes_per_pixel;
            dst += bytes_per_pixel;
        }
    }

    // tga inverted (code from stb_image.c)
    if( 1-((imageinfo >> 5) & 1) ) {
        for( j = 0; j*2 < height; ++j )   {
            int index1 = j * width * bytes_per_pixel;
            int index2 = (height - 1 - j) * width * bytes_per_pixel;

            for( i = width * bytes_per_pixel; i > 0; --i )  {
                unsigned char temp = pix[index1];
                pix[index1] = pix[index2];
                pix[index2] = temp;
                ++index1;
                ++index2;
            }
        }
    }

    GLuint texture, mode;
    if( bytes_per_pixel == 1 ) mode = GL_ALPHA;
    else mode = ( bytes_per_pixel == 3 ? GL_RGB : GL_RGBA );

    glGenTextures( 1, &texture );
    glBindTexture( GL_TEXTURE_2D, texture );
    glTexImage2D( GL_TEXTURE_2D, 0, mode, width, height, 0, mode, GL_UNSIGNED_BYTE, pix );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    tex->id = texture;
    tex->width = width;
    tex->height = height;
    tex->bpp = bytes_per_pixel;

    fclose( tga_file );
    g_free( pix );
    return texture;

error:
    fclose( tga_file );
    tex->id = 0;
    return 0;
}

//
// Program (Shaders)
//
GLuint program_load_shader( const GLchar *src, GLenum type ) {
    GLuint shader;
    GLint compiled;

    shader = glCreateShader(type);
    if( shader == 0 ) return 0;

    glShaderSource( shader, 1, &src, 0 );
    glCompileShader( shader );
    glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );

    if( !compiled ) {
        GLint len = 0;
        glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &len );
        if( len > 0 ) {
            char *err = g_new( char, len );
            glGetShaderInfoLog( shader, len, 0, err );
            fprintf( stderr, "error: could not compile shader: %s\n", err );
            free( err );
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

int program_link( Program *program, const char **attribs, const char **uniforms ) {
    GLint linked;

    program->object = glCreateProgram();
    if( program->object == 0 ) return 0;

    glAttachShader( program->object, program->vs );
    glAttachShader( program->object, program->fs );

    int i;
    for( i = 0; attribs[i] != 0; ++i ) {
        glBindAttribLocation( program->object, i, attribs[i] );
    }

    glLinkProgram( program->object );
    glGetProgramiv( program->object, GL_LINK_STATUS, &linked );

    if( !linked ) {
        GLint len = 0;
        glGetShaderiv( program->object, GL_INFO_LOG_LENGTH, &len );
        if( len > 0 ) {
            char *err = g_new( char, len );
            glGetShaderInfoLog( program->object, len, 0, err );
            g_debug_str( "error: could not link program: %s\n", err );
            free( err );
        }
        goto error;
    }

    for( i = 0; uniforms[i] != 0; ++i ) {
        program->uniforms[i] = glGetUniformLocation( program->object, uniforms[i] );
        if( program->uniforms[i] == -1) {
            g_fatal_error( "error: could not bind uniform %s\n", uniforms[i] );
            goto error;
        }
    }

    return 1;
error:
    glDeleteProgram( program->object );
    return 0;
}
