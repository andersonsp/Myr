#include "land.h"

//
// Texture
//
int texture_load( Texture *tex, const char* filename ) {
    unsigned char *pix, tmp, *src, *dst;
    int pixsize, k, n, i,j;

    FILE* tga_file;
    unsigned char buf[ 18 ];

    char filepath[256];
    sprintf( filepath, "data/textures/%s", filename );

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

    // Allocate memory for pixel data
    pix = (unsigned char *) malloc( pixsize );
    if( !pix ) goto error;

    // Read pixel data from file
    if( type == 10 || type == 11 ) {
      int i, size;
      GLubyte packet_header, rgba[4];
      GLubyte *ptr = pix;

      while (ptr < pix + pixsize) {
        packet_header = (GLubyte)fgetc (tga_file); // Read first byte
        size = 1 + (packet_header & 0x7f);

        if (packet_header & 0x80) { // Run-length packet
          fread (rgba, sizeof (GLubyte), bytes_per_pixel, tga_file);
          for (i = 0; i < size; ++i, ptr += bytes_per_pixel)
            for( k = 0; k < bytes_per_pixel; k++ ) ptr[k] = rgba[k];

        } else { // Non run-length packet
          for (i = 0; i < size; ++i, ptr += bytes_per_pixel)
            fread (ptr, sizeof (GLubyte), bytes_per_pixel, tga_file);
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

    if( 1-((imageinfo >> 5) & 1) ) {  //tga inverted (code from stb_image.c)
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

    fclose(tga_file);

    GLuint texture;
    GLuint mode;
    if( bytes_per_pixel == 1 ) mode = GL_ALPHA;
    else mode = ( bytes_per_pixel == 3 ? GL_RGB : GL_RGBA );

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, mode, width, height, 0, mode, GL_UNSIGNED_BYTE, pix);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    tex->id = texture;
    tex->width = width;
    tex->height = height;
    tex->bpp = bytes_per_pixel;
    return texture;

error: fclose( tga_file );
      tex->id = 0;
      return 0;
}
