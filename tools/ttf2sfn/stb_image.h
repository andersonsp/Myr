#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H


#define STBI_VERSION 1

enum
{
   STBI_default = 0, // only used for req_comp

   STBI_grey       = 1,
   STBI_grey_alpha = 2,
   STBI_rgb        = 3,
   STBI_rgb_alpha  = 4,
};

typedef unsigned char stbi_uc;

#ifdef __cplusplus
extern "C" {
#endif


// write a BMP/TGA file given tightly packed 'comp' channels (no padding, nor bmp-stride-padding)
// (you must include the appropriate extension in the filename).
// returns TRUE on success, FALSE if couldn't open file, error writing file
extern int      stbi_write_bmp       (char const *filename,     int x, int y, int comp, void *data);
extern int      stbi_write_tga       (char const *filename,     int x, int y, int comp, void *data);


#ifdef __cplusplus
}
#endif

//
//
////   end header file   /////////////////////////////////////////////////////
#endif // STBI_INCLUDE_STB_IMAGE_H
