//This is maybe a obscure part of the implementation
// Based on the implementation of Sean Barret's stb_gl.h

// o_O

#ifdef G_GLEXT_H_INCLUDE
//
// Extension definitions goes here
//

GLE( GenBuffers, GENBUFFERS )
GLE( BindBuffer, BINDBUFFER )
GLE( BufferData, BUFFERDATA )
GLE( VertexAttribPointer, VERTEXATTRIBPOINTER )
GLE( EnableVertexAttribArray, ENABLEVERTEXATTRIBARRAY )
GLE( GenVertexArrays, GENVERTEXARRAYS )
GLE( BindVertexArray, BINDVERTEXARRAY )
GLE( CreateShader, CREATESHADER )
GLE( ShaderSource, SHADERSOURCE )
GLE( CompileShader, COMPILESHADER )
GLE( GetShaderiv, GETSHADERIV )
GLE( GetShaderInfoLog, GETSHADERINFOLOG )
GLE( CreateProgram, CREATEPROGRAM )
GLE( AttachShader, ATTACHSHADER )
GLE( BindAttribLocation, BINDATTRIBLOCATION )
GLE( LinkProgram, LINKPROGRAM )
GLE( GetProgramiv, GETPROGRAMIV )
GLE( GetProgramInfoLog, GETPROGRAMINFOLOG )
GLE( UseProgram, USEPROGRAM )
GLE( DeleteShader, DELETESHADER )
GLE( DeleteProgram, DELETEPROGRAM )
GLE( GetUniformLocation, GETUNIFORMLOCATION )
GLE( UniformMatrix4fv, UNIFORMMATRIX4FV )
GLE( Uniform1i, UNIFORM1I )

//GLE(  )

//
// look at the code below on your own risk ;-)
//
#else
    #define GLARB(a,b) GLE(a##ARB,b##ARB)
    #define GLEXT(a,b) GLE(a##EXT,b##EXT)
    #define GLNV(a,b)  GLE(a##NV ,b##NV)
    #define GLATI(a,b) GLE(a##ATI,b##ATI)

    #ifdef G_GL_EXT_IMPLEMENT

       #ifdef _WIN32
         #define GL__GET_FUNC(x) wglGetProcAddress(x)
       #else
         #define GL__GET_FUNC(x) glXGetProcAddress((GLubyte*) x)
       #endif


       #ifdef GLE
       #undef GLE
       #endif

       #define GLE(a,b)  PFNGL##b##PROC gl##a;
       #define G_GLEXT_H_INCLUDE
       #include __FILE__

       #undef GLE
       #define GLE(a,b) gl##a = (PFNGL##b##PROC) GL__GET_FUNC("gl" #a );

       void g_init_gl_extensions(void) {
          #define G_GLEXT_H_INCLUDE
          #include __FILE__
       }

       #undef GLE
    #else
      #define GLE(a,b) extern PFNGL##b##PROC gl##a;

      #define G_GLEXT_H_INCLUDE
      #include __FILE__
    #endif

#endif // G_GLEXT_INCLUDE
