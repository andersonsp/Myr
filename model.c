#include "land.h"

#define IQM_MAGIC "INTERQUAKEMODEL"
#define IQM_VERSION 2

typedef struct {
    char magic[16];
    unsigned int version;
    unsigned int filesize;
    unsigned int flags;
    unsigned int num_text, ofs_text;
    unsigned int num_meshes, ofs_meshes;
    unsigned int num_vertexarrays, num_vertexes, ofs_vertexarrays;
    unsigned int num_triangles, ofs_triangles, ofs_adjacency;
    unsigned int num_joints, ofs_joints;
    unsigned int num_poses, ofs_poses;
    unsigned int num_anims, ofs_anims;
    unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
    unsigned int num_comment, ofs_comment;
    unsigned int num_extensions, ofs_extensions;
} iqmheader;

typedef struct {
    unsigned int name, material;
    unsigned int first_vertex, num_vertexes;
    unsigned int first_triangle, num_triangles;
} IqmMesh;

enum {
    IQM_POSITION     = 0,
    IQM_TEXCOORD     = 1,
    IQM_NORMAL       = 2,
    IQM_TANGENT      = 3,
    IQM_BLENDINDEXES = 4,
    IQM_BLENDWEIGHTS = 5,
    IQM_COLOR        = 6,
    IQM_CUSTOM       = 0x10
};

enum {
    IQM_BYTE   = 0,
    IQM_UBYTE  = 1,
    IQM_SHORT  = 2,
    IQM_USHORT = 3,
    IQM_INT    = 4,
    IQM_UINT   = 5,
    IQM_HALF   = 6,
    IQM_FLOAT  = 7,
    IQM_DOUBLE = 8,
};

typedef struct {
    unsigned int vertex[3];
} IqmTriangle;

typedef struct {
    unsigned int name;
    int parent;
    Vec translate;
    Quat rotate;
    Vec scale;
} IqmJoint;

typedef struct {
    int parent;
    unsigned int mask;
    float channeloffset[10], channelscale[10];
} IqmPose;

typedef struct {
    unsigned int name, first_frame, num_frames;
    float framerate;
    unsigned int flags;
} IqmAnim;

enum { IQM_LOOP = 1<<0 };

typedef struct {
    unsigned int type, flags, format, size, offset;
} IqmVertexArray;

typedef struct {
    float bbmin[3], bbmax[3];
    float xyradius, radius;
} IqmBounds;

typedef struct{
    Vec loc, normal;
    Vec2 texcoord;
    Vec4 tangent, bitangent;
    unsigned char blendindex[4], blendweight[4];
} IqmVertex;

struct _Model {
    int num_meshes, num_verts, num_tris, num_joints, num_frames, num_anims;
    char *str;

    IqmMesh *meshes;
    IqmVertex *verts, *out_verts;
    IqmTriangle *tris, *adjacency;
    GLuint *textures;
    IqmJoint *joints;
    IqmPose *poses;
    IqmAnim *anims;
    IqmBounds *bounds;

    // used to transform animations
    // DualQuat *base, *inversebase, *outframe, *frames;
};



static int loadiqmmeshes( Model *mdl, const char *filename, const iqmheader *hdr, unsigned char *buf ) {
    mdl->num_meshes = hdr->num_meshes;
    mdl->num_tris = hdr->num_triangles;
    mdl->num_verts = hdr->num_vertexes;
    mdl->num_joints = hdr->num_joints;

    mdl->meshes = g_new( IqmMesh, mdl->num_meshes );
    mdl->tris = g_new( IqmTriangle, mdl->num_tris );
    mdl->verts = g_new( IqmVertex, mdl->num_verts );
    mdl->joints = g_new( IqmJoint, mdl->num_joints );
    mdl->textures = g_new0( GLuint, mdl->num_meshes );

    const char *str = hdr->ofs_text ? (char *)&buf[hdr->ofs_text] : "";
    IqmVertexArray *vas = (IqmVertexArray *)&buf[hdr->ofs_vertexarrays];

    float *loc=NULL, *normal=NULL, *texcoord=NULL, *tangent=NULL;
    unsigned char *blendindex = NULL, *blendweight=NULL;

    int i,j;
    for( i = 0; i < (int)hdr->num_vertexarrays; i++ ) {
        IqmVertexArray *va = &vas[i];
        switch(va->type) {
            case IQM_POSITION:
            if(va->format != IQM_FLOAT || va->size != 3) return 0;
            loc = (float *) &buf[va->offset];
            break;

            case IQM_NORMAL:
            if(va->format != IQM_FLOAT || va->size != 3) return 0;
            normal = (float *) &buf[va->offset];
            break;

            case IQM_TANGENT:
            if(va->format != IQM_FLOAT || va->size != 4) return 0;
            tangent = (float *) &buf[va->offset];
            break;

            case IQM_TEXCOORD:
            if(va->format != IQM_FLOAT || va->size != 2) return 0;
            texcoord = (float *) &buf[va->offset];
            break;

            case IQM_BLENDINDEXES:
            if(va->format != IQM_UBYTE || va->size != 4) return 0;
            blendindex = (unsigned char*) &buf[va->offset];
            break;

            case IQM_BLENDWEIGHTS:
            if(va->format != IQM_UBYTE || va->size != 4) return 0;
            blendweight = (unsigned char*) &buf[va->offset];
            break;
        }
    }

    for( j=0; j<mdl->num_verts; j++ ){
        IqmVertex *v = &mdl->verts[j];
        if( loc ) memcpy( &v->loc, &loc[j*3], sizeof(Vec) );
        if( normal ) memcpy( &v->normal, &normal[j*3], sizeof(Vec) );
        if( tangent ) memcpy( &v->tangent, &tangent[j*4], sizeof(Vec4) );
        if( texcoord ) memcpy( &v->texcoord, &texcoord[j*2], sizeof(Vec2) );
        if( blendindex ) memcpy( &v->blendindex, &blendindex[j*4], sizeof(unsigned char)*4 );
        if( blendweight ) memcpy( &v->blendweight, &blendweight[j*4], sizeof(unsigned char)*4 );
    }

    IqmTriangle * tris = (IqmTriangle *) &buf[hdr->ofs_triangles];
    IqmMesh* meshes = (IqmMesh *) &buf[hdr->ofs_meshes];
    IqmJoint* joints = (IqmJoint *) &buf[hdr->ofs_joints];
//    if( hdr->ofs_adjacency ) adjacency = ( IqmTriangle *) &buf[hdr->ofs_adjacency];

    for( i=0; i<mdl->num_tris; i++ ) mdl->tris[i] = tris[i];
    for( i=0; i<mdl->num_meshes; i++ ) mdl->meshes[i] = meshes[i];
    for( i=0; i<mdl->num_joints; i++ ) mdl->joints[i] = joints[i];

    // mdl->base = g_new( DualQuat, hdr->num_joints );
    // mdl->inversebase = g_new( DualQuat, hdr->num_joints );
    // for( i = 0; i < (int) hdr->num_joints; i++ ) {
    //     IqmJoint *j = &joints[i];
    //     g_quat_normalize( &j->rotate );
    //     g_dual_quat_from_quat_vec( &mdl->base[i], &j->rotate, &j->translate );

    //     if( j->parent >= 0)
    //         g_dual_quat_mul( &mdl->base[i], &mdl->base[j->parent], &mdl->base[i] );

    //     g_dual_quat_invert( &mdl->inversebase[i], &mdl->base[i] );
    // }

    Texture tex;
    for( i = 0; i < (int)hdr->num_meshes; i++ ) {
        IqmMesh *m = &mdl->meshes[i];
        g_debug_str("%s: loaded mesh: %s\n", filename, &str[m->name]);

        mdl->textures[i] = texture_load( &tex, &str[m->material] );
        if( mdl->textures[i] ) g_debug_str("%s: loaded material: %s\n", filename, &str[m->material]);
        else g_debug_str("%s: couldn't load material: %s\n", filename, &str[m->material]);
    }

    return 1;
}

//TODO: add a resource manager for this assets
Model* model_load( const char *filename ) {
    char filepath[256];
    sprintf( filepath, "data/models/%s", filename );

    FILE *f = fopen(filepath, "rb");
    if(!f) return NULL;
    Model* mdl = g_new0( Model, 1 );

    unsigned char *buf = NULL;
    iqmheader hdr;
    if( fread(&hdr, 1, sizeof(hdr), f) != sizeof(hdr) || memcmp(hdr.magic, IQM_MAGIC, sizeof(hdr.magic)) )
        goto error;

    if( hdr.version != IQM_VERSION || hdr.filesize > (16<<20) ) goto error; // sanity check... don't load files bigger than 16 MB

    buf = (unsigned char*) malloc( hdr.filesize );
    if( fread(buf + sizeof(hdr), 1, hdr.filesize - sizeof(hdr), f) != hdr.filesize - sizeof(hdr) )
        goto error;

    if( hdr.num_meshes > 0 && !loadiqmmeshes( mdl, filename, &hdr, buf) ) goto error;
    // if( hdr.num_anims > 0 && !loadiqmanims( mdl, filename, &hdr, buf) ) goto error;

    fclose(f);
    free(buf);
    return mdl;

    error:
    g_fatal_error("%s: error while loading\n", filename);
    free( buf );
    model_destroy( mdl );
    fclose(f);
    return NULL;
}

void model_destroy( Model *mdl ){
    if( !mdl ) return;
    if( mdl->str ) g_free( mdl->str );
    if( mdl->meshes ) g_free( mdl->meshes );
    if( mdl->tris ) g_free( mdl->tris );
    if( mdl->verts ) g_free( mdl->verts );
    if( mdl->out_verts ) g_free( mdl->out_verts );
    if( mdl->joints ) g_free( mdl->joints );
    // if( mdl->frames ) g_free( mdl->frames );
    // if( mdl->outframe ) g_free( mdl->outframe );

    if( mdl->textures ) g_free( mdl->textures );
    g_free( mdl );
}

//TODO: add support for normals and normal mapping
void model_draw( Model *mdl, float frame ){
    // animateiqm( mdl, frame );

    IqmVertex* v = mdl->verts; //(mdl->num_frames > 0 ? mdl->out_verts : mdl->verts);
    glVertexPointer(3, GL_FLOAT, sizeof(IqmVertex), &v[0].loc );


    glNormalPointer( GL_FLOAT, sizeof(IqmVertex), &v[0].normal );
    glTexCoordPointer(2, GL_FLOAT, sizeof(IqmVertex), &mdl->verts[0].texcoord );

    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );

    int i;
    for( i = 0; i < mdl->num_meshes; i++ ) {
      IqmMesh *m = &mdl->meshes[i];
      if( mdl->textures[i] ) glBindTexture( GL_TEXTURE_2D, mdl->textures[i] );
      glDrawElements( GL_TRIANGLES, 3*m->num_triangles, GL_UNSIGNED_INT, &mdl->tris[m->first_triangle] );
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

#define DIST_EPSILON    (0.01f)
#define MIN_FRACTION    (0.0005f)

int model_collision( Model *mdl, Vec* pos, Vec* dir, float radius, Vec *end, Vec* intersect ) {
    int i, j;
    TraceInfo ti;
    trace_init( &ti, pos, dir, radius );
    for( i = 0; i < mdl->num_meshes; i++ ) {
        IqmMesh* m = &mdl->meshes[i];
        for( j=m->first_triangle; j<m->num_triangles; j++ ) {
            Vec p1, p2, p3;

            p3 = mdl->verts[mdl->tris[j].vertex[0]].loc;
            p2 = mdl->verts[mdl->tris[j].vertex[1]].loc;
            p1 = mdl->verts[mdl->tris[j].vertex[2]].loc;
            if( radius > 0.0f ) trace_sphere_triangle(&ti, &p1, &p2, &p3);
            else trace_ray_triangle(&ti, &p1, &p2, &p3);
        }
    }
    if( ti.collision ) {
        if( radius > 0.0f ) {
            Vec norm;
            // we'll' move close but not to the exact point
            vec_normalize( &norm, vec_sub(&norm, &ti.start, &ti.intersect_point) );
            float t = ti.t + DIST_EPSILON/vec_dot( &norm, &ti.vel );
            if( t < MIN_FRACTION ) t = .0f;
            vec_add( end, &ti.start, vec_scale(end, &ti.vel, t) );

            // shift intersection point, to account we aren't moving to the exact point we should
            vec_add( intersect, &ti.intersect_point, vec_scale(&norm, &norm, DIST_EPSILON) );
        } else {
            *end = ti.intersect_point;
            *intersect = ti.intersect_point;
        }
        return 1;
    }
    return 0;
}

float model_calculate_bounding_sphere( Model *mdl ) {
    int i;
    float rr = 0;
    for( i = 0; i < mdl->num_verts; i++ ) {
        float dx = mdl->verts[i].loc.x;
        float dy = mdl->verts[i].loc.y;
        float dz = mdl->verts[i].loc.z;
        float dd = dx * dx + dy * dy + dz * dz;
        if( dd > rr ) rr = dd;
    }

    printf("Bounding sphere radius: %f\n", sqrt(rr));
    return rr;
}
