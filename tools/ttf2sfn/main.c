#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "stb_truetype.h"
#include "stb_image.h"

// =============================================
typedef struct {
    float u0, v0;
    float u1, v1;

    float x0, y0;
    float x1, y1;

    float advance;

} Glyph;

typedef struct {
    char id[4];
    int version;
    int tex_w, tex_h;
    int start, end;
    int avgh, avgw;
}Font_header;
// =============================================

void usage(){
    printf("ttf2sfn ttffont [out.sfn] [opts]\n");
    printf("\t-w : (power of two) width of bitmap font (default: 128)\n");
    printf("\t-h : (power of two) height of bitmap font (default: 128)\n");
    printf("\t-ib: begin of char range (default: 32)\n");
    printf("\t-ie: end of char range (default: 127)\n");
    printf("\t-s : size of the font in pixels (default: 16px)\n");
    printf("\t-i : index of font within the ttf file (default: 0)\n");
    printf("\t-nobitmap: dont generate the companion *.bmp image\n");

}

int main( int argc, char**argv ) {
    //parameters
    char* filename = NULL;
    char* output = "out";
    int index = 0;
    int size = 16;
    int start = 32;
    int end = 127;
    int texw = 128;
    int texh = 128;
    int genbitmap = 1;

    int tw = 0;
    int th = 0;

    if( argc < 2 ) {
        usage();
        return 0;
    }

    int i = 3;
    if( argv[2][0] == '-' ) i = 2;

    for ( ; i < argc; i++) {
        if (!strcmp(argv[i], "-w")) {
            i++;
            texw = atoi(argv[i]);
        } else if (!strcmp(argv[i], "-h")) {
            i++;
            texh = atoi(argv[i]);
        } else if (!strcmp(argv[i], "-ib")) {
            i++;
            start = atoi(argv[i]);
        } else if (!strcmp(argv[i], "-ie")) {
            i++;
            end = atoi(argv[i]);
        } else if (!strcmp(argv[i], "-s")) {
            i++;
            size = atoi(argv[i]);
        } else if (!strcmp(argv[i], "-i")) {
            i++;
            index = atoi(argv[i]);
        } else if (!strcmp(argv[i], "-nobitmap")) {
            genbitmap = 0;
        }  else {
            usage();
            return 0;
        }
    }

    filename = argv[1];
    if( argc >= 3 && argv[2][0] != '-' ) output = argv[2];

    FILE* filein;
    FILE* fileout;
    int file_length;
    unsigned char *ttf_buffer;

    Font_header fnt;
    Glyph *glyph;
    unsigned char * pixels;

    float scale;
    int x,y,bottom_y;
    stbtt_fontinfo f;

    if( start >= end ) return 1;

    float ipw = 1.0f / texw, iph = 1.0f / texh;


    // Open the TTF File
    filein = fopen(filename, "rb");
    if( filein == NULL ) {
        printf("Cannot open file: %s\n", filename);
        return 1;
    }

    // Find Length of File
    fseek(filein, 0, SEEK_END);
    file_length = ftell(filein);

    ttf_buffer = (unsigned char*) malloc( file_length );
    if( !ttf_buffer ) {
        printf("Not enough memory to allocate TTF buffer");
        return 1;
    }

    fseek( filein, 0, SEEK_SET );
    fread( ttf_buffer, 1, file_length, filein );

    fclose( filein );

    if( !stbtt_InitFont(&f, ttf_buffer, index) ) {
        free(ttf_buffer);
        printf("TTF buffer cannot be initialized");
        return 1;
    }

    glyph = (Glyph*) malloc( sizeof(Glyph)*(end - start) );
    if( !glyph ) {
        free(ttf_buffer);
        printf("Not enough memory for Glyph list");
        return 1;
    }

    pixels = (uint8_t*) malloc( texw*texh );
    if( !pixels ) {
        free(ttf_buffer);
        free( glyph );
        printf("Not enough memory for pixel data");
        return 1;
    }

    fnt.id[0] = 'S';
    fnt.id[1] = 'F';
    fnt.id[2] = 'N';
    fnt.id[3] = 'T';
    fnt.version = 3;
    fnt.start = start;
    fnt.end = end;
    fnt.tex_w = texw;
    fnt.tex_h = texh;

    memset( pixels, 0, texw*texh ); // background of 0 around pixels
    x = y = 1;
    bottom_y = 1;

    scale = stbtt_ScaleForPixelHeight( &f, size );

    for( i = start; i < end; ++i ) {

        int advance, lsb, x0,y0,x1,y1,gw,gh;
        int g = stbtt_FindGlyphIndex( &f, i );

        stbtt_GetGlyphHMetrics( &f, g, &advance, &lsb );
        stbtt_GetGlyphBitmapBox( &f, g, scale, scale, &x0, &y0, &x1, &y1 );

        gw = x1-x0;
        gh = y1-y0;

        tw += gw;
        th += gh;

        if( x + gw + 1 >= texw ) y = bottom_y, x = 1; // advance to next row
        if( y + gh + 1 >= texh ) { // check if it fits vertically AFTER potentially moving to next row
            fnt.end -= i;
            free( ttf_buffer );
            free( glyph );
            free( pixels );
            printf("Character interval doesn't fit in the texture size specified\n %d characters left\n", fnt.end );
            return 1;
        }

        assert( x+gw < texw);
        assert( y+gh < texh);

        stbtt_MakeGlyphBitmap( &f, pixels+x+y*texw, gw, gh, texw, scale, scale, g);

        glyph[i-start].u0 = x * ipw;
        glyph[i-start].v0 = y * iph;
        glyph[i-start].u1 = (x + gw) * ipw;
        glyph[i-start].v1 = (y + gh) * iph;

        glyph[i-start].x0 = (float)(0.0);
        glyph[i-start].y0 = (float)-( y0+gh);
        glyph[i-start].x1 = (float)gw;
        glyph[i-start].y1 = (float)(gh + ( -y0 - gh));
        glyph[i-start].advance = scale * advance;

        x = x + gw + 2;

        if( y+gh+2 > bottom_y ) bottom_y = y+gh+2;
    }


    fnt.avgw = tw / (fnt.end - fnt.start);
    fnt.avgh = th / (fnt.end - fnt.start);

    char ofile[512];

    if( !output ) snprintf( ofile, 512, "%s.%s", basename(filename), "sfn" );
    else snprintf( ofile, 512, "%s.%s", output, "sfn" );

    fileout = fopen( ofile, "wb" );
    if (!fileout) {
        printf("Cannot open output file: %s\n", ofile);
        return 1;
    }

    fwrite( &fnt, sizeof(Font_header), 1, fileout);
    fwrite( glyph, sizeof(Glyph), fnt.end - fnt.start, fileout );
    fwrite( pixels, 1, fnt.tex_w*fnt.tex_h, fileout );

    fclose( fileout );

    if( !output ) snprintf( ofile, 512, "%s.%s", basename(filename), "bmp" );
    else snprintf( ofile, 512, "%s.%s", output, "bmp" );

    if( genbitmap ) {
        if ( !stbi_write_bmp( ofile, texw, texh, 1, pixels) ) {
            printf("Cannot write to bmp file");
            return 1;
        }
    }

    free( ttf_buffer );
    free( glyph );
    free( pixels );


    printf("Font generation succesfull...\n");
    printf("start: %d, end: %d\n", start, end);
    printf("avgW: %d, avgH: %d \n", fnt.avgw, fnt.avgh );

    return 0;
}
