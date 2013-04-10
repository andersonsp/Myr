#include "land.h"

#define EPSILON 0.002f
#define LMESH_MAGIC "_LMesh_"
#define LMESH_VERSION 2

typedef struct {
  char magic[8];
  unsigned int version;
  unsigned int filesize;
  unsigned int flags, size;
  unsigned int num_text, ofs_text;
  unsigned int num_meshes, ofs_meshes;
  unsigned int num_vertexes, ofs_vertexes;
  unsigned int num_triangles, ofs_triangles;
  unsigned int num_joints, ofs_joints;
  unsigned int num_poses, ofs_poses;
  unsigned int num_anims, ofs_anims;
  unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
} ModelHeader;

typedef struct {
  unsigned int name, material;
  unsigned int first_vertex, num_vertexes;
  unsigned int first_triangle, num_triangles;
} Mesh;

typedef struct {
  unsigned int vertex[3];
} Triangle;

typedef struct {
  unsigned int name;
  int parent;
  Vec translate, inv_translate;
  Quat rotate, inv_rotate;
  Vec scale, inv_scale;
} Joint;

typedef struct {
  int parent;
  unsigned int mask;
  float chan_ofs[10], chan_scale[10];
} Pose;

typedef struct {
  unsigned int name, first_frame, num_frames;
  float framerate;
  unsigned int flags;
} Anim;

enum { ANIM_LOOP = 1<<0 };

typedef struct {
  float bbmin[3], bbmax[3];
  float xyradius, radius;
} Bounds;

typedef struct{
  Vec loc, normal;
  Vec2 texcoord;
  Vec4 tangent;
  GLubyte blendindex[4], blendweight[4];
} Vertex;

typedef struct {
    Quat q;  // rotation
    Vec  v;  // translation
} FramePose;

struct _Model {
  int num_meshes, num_verts, num_tris, num_joints, num_frames, num_anims;
  char *str;

  Mesh *meshes;
  Vertex *verts;
  Triangle *tris;
  GLuint *textures;
  Joint *joints;
  Pose *poses;
  Anim *anims;
  Bounds *bounds;

  GLuint vbo, ibo;

  // DualQuat *frame; //in iqm demo its a 3x4 matrix
  Mat4 *outframe; //in iqm demo its a 3x4 matrix
  FramePose *base, *inv, *frame, *out;
};

static int load_meshes( Model *mdl, const char *filename, const ModelHeader *hdr, unsigned char *buf ) {
    mdl->num_meshes = hdr->num_meshes;
    mdl->num_tris = hdr->num_triangles;
    mdl->num_verts = hdr->num_vertexes;
    mdl->num_joints = hdr->num_joints;
    mdl->meshes = g_new( Mesh, mdl->num_meshes );
    mdl->tris = g_new( Triangle, mdl->num_tris );
    mdl->verts = g_new( Vertex, mdl->num_verts );
    mdl->textures = g_new0( GLuint, mdl->num_meshes );

    const char *str = hdr->ofs_text ? (char *)&buf[hdr->ofs_text] : "";
    memcpy( mdl->verts, &buf[hdr->ofs_vertexes], (mdl->num_verts*sizeof(Vertex)) );
    memcpy( mdl->tris, &buf[hdr->ofs_triangles], (mdl->num_tris*sizeof(Triangle)) );
    memcpy( mdl->meshes, &buf[hdr->ofs_meshes], (mdl->num_meshes*sizeof(Mesh)) );

    if( mdl->num_joints > 0 ) {
        mdl->joints = g_new( Joint, mdl->num_joints );
        mdl->base = g_new( FramePose, hdr->num_joints );
        mdl->inv = g_new( FramePose, hdr->num_joints );
        memcpy( mdl->joints, &buf[hdr->ofs_joints], (mdl->num_joints*sizeof(Joint)) );
    }

    int i;
    for( i = 0; i < (int) hdr->num_joints; i++ ) {
        Joint *j = &mdl->joints[i];
        quat_normalize( &j->rotate, &j->rotate );
        mdl->base[i].v = j->translate;
        mdl->base[i].q = j->rotate;

        quat_normalize( &mdl->inv[i].q, quat_invert(&mdl->inv[i].q, &mdl->base[i].q) );
        vec_scale( &mdl->inv[i].v, &j->translate, -1.0 );
        quat_vec_mul( &mdl->inv[i].v, &mdl->inv[i].q, &mdl->inv[i].v );
    }

    Texture tex;
    for( i = 0; i < (int)hdr->num_meshes; i++ ) {
        Mesh *m = &mdl->meshes[i];
        g_debug_str("%s: loaded mesh: %s\n", filename, &str[m->name]);

        mdl->textures[i] = texture_load( &tex, &str[m->material] );
        if( mdl->textures[i] ) g_debug_str("%s: loaded material: %s\n", filename, &str[m->material]);
        else g_debug_str("%s: couldn't load material: %s\n", filename, &str[m->material]);
    }

    return 1;
}

static int load_anims( Model* mdl, const char *filename, const ModelHeader *hdr, unsigned char *buf ) {
    if( (int)hdr->num_poses != mdl->num_joints ) return 0;

    const char *str = hdr->ofs_text ? (char *)&buf[hdr->ofs_text] : "";
    mdl->num_anims = hdr->num_anims;
    mdl->num_frames = hdr->num_frames;
    mdl->anims = (Anim *)&buf[hdr->ofs_anims];
    mdl->poses = (Pose *)&buf[hdr->ofs_poses];
    mdl->frame = g_new( FramePose, hdr->num_frames * hdr->num_poses );
    mdl->out = g_new( FramePose, hdr->num_joints);
    // mdl->frame = g_new( DualQuat, hdr->num_joints);
    mdl->outframe = g_new( Mat4, hdr->num_joints);

    //TODO: load bounds data
    unsigned short *framedata = (unsigned short *)&buf[hdr->ofs_frames];
    // if( hdr->ofs_bounds ) mdl->bounds = (IqmBounds *)&buf[hdr->ofs_bounds];

    int i, j;
    for( i = 0; i < (int)hdr->num_frames; i++ ) {
        for( j = 0; j < (int)hdr->num_poses; j++ ) {
            Pose *p = &mdl->poses[j];
            Vec trans, rv, pv;
            Quat rot, rq;

            trans = (Vec){ p->chan_ofs[0], p->chan_ofs[1], p->chan_ofs[2] };
            if( p->mask & 0x01 ) trans.x += *framedata++ * p->chan_scale[0];
            if( p->mask & 0x02 ) trans.y += *framedata++ * p->chan_scale[1];
            if( p->mask & 0x04 ) trans.z += *framedata++ * p->chan_scale[2];

            rot = (Quat){ p->chan_ofs[3], p->chan_ofs[4], p->chan_ofs[5], p->chan_ofs[6] };
            if( p->mask & 0x08 ) rot.x += *framedata++ * p->chan_scale[3];
            if( p->mask & 0x10 ) rot.y += *framedata++ * p->chan_scale[4];
            if( p->mask & 0x20 ) rot.z += *framedata++ * p->chan_scale[5];
            if( p->mask & 0x40 ) rot.w += *framedata++ * p->chan_scale[6];

            if( (p->mask & 0x80) || (p->mask & 0x100) || (p->mask & 0x200) ) {
              g_debug_str("bone scaling is disabled...\n");
              return 0;
            }

            // Concatenate each pose with the inverse base pose to avoid doing this at animation time.
            // If the joint has a parent, then it needs to be pre-concatenated with its parent's base pose.
            int k = i*hdr->num_poses + j;
            quat_normalize( &rot, &rot );
            quat_mul( &rq, &rot, &mdl->inv[j].q );
            quat_vec_mul( &rv, &rot, &mdl->inv[j].v );
            vec_add( &rv, &rv, &trans );

            if( p->parent >= 0) {
                quat_mul( &mdl->frame[k].q, &mdl->base[p->parent].q, &rq );
                quat_vec_mul( &pv, &mdl->base[p->parent].q, &rv );
                vec_add( &mdl->frame[k].v, &pv, &mdl->base[p->parent].v );
            } else {
                mdl->frame[k].v = rv;
                mdl->frame[k].q = rq;
            }
        }
    }

    for( i = 0; i < (int)hdr->num_anims; i++ )
        g_debug_str("%s: loaded anim: %s\n", filename, &str[mdl->anims[i].name]);

    g_free( mdl->base );
    g_free( mdl->inv );
    return 1;
}

static void animate_model( Model *mdl, float curframe ) {
    if(!mdl->num_frames) return;

    int frame1 = (int)floor(curframe), frame2 = frame1 + 1;
    float frameoffset = curframe - frame1;
    frame1 = (frame1 % mdl->num_frames) * mdl->num_joints;
    frame2 = (frame2 % mdl->num_frames) * mdl->num_joints;

    // Interpolate transforms between the two closest frames and concatenate with parent transform if necessary.
    // Animation blending and inter-frame blending  goes here...
    int i;
    for( i = 0; i < mdl->num_joints; i++ ) {
        FramePose  rf, f1 = mdl->frame[frame1+i], f2 = mdl->frame[frame2+i];
        FramePose *out = mdl->out;

        vec_lerp( &rf.v, &f1.v, &f2.v, frameoffset );
        quat_lerp( &rf.q, &f1.q, &f2.q, frameoffset );
        if( mdl->joints[i].parent >= 0) {
            int parent = mdl->joints[i].parent;

            quat_mul( &out[i].q, &out[parent].q, &rf.q );
            quat_vec_mul( &out[i].v, &out[parent].q, &rf.v );
            vec_add( &out[i].v, &out[i].v, &out[parent].v );

            quat_normalize( &mdl->out[i].q, &mdl->out[i].q );
        } else {
            out[i].v = rf.v;
            out[i].q = rf.q;
        }
        mat4_from_quat_vec( &mdl->outframe[i], &out[i].q, &out[i].v );
    }
}

//TODO: add a resource manager for this assets
Model* model_load( const char *filename ) {
    unsigned char *buf = NULL;
    char filepath[1024];
    sprintf( filepath, "../data/models/%s", filename );

    FILE *f = fopen(filepath, "rb");
    if(!f) return NULL;
    Model* mdl = g_new0( Model, 1 );

    ModelHeader hdr;
    if( fread(&hdr, 1, sizeof(hdr), f) != sizeof(hdr) || memcmp(hdr.magic, LMESH_MAGIC, sizeof(hdr.magic)) )
        goto error;

    if( hdr.version != LMESH_VERSION || hdr.filesize > (16<<20) ) goto error; // sanity check... don't load files bigger than 16 MB

    buf = (unsigned char*) malloc( hdr.filesize );
    if( fread(buf + sizeof(hdr), 1, hdr.filesize - sizeof(hdr), f) != hdr.filesize - sizeof(hdr) )
        goto error;

    if( hdr.num_meshes > 0 && !load_meshes( mdl, filename, &hdr, buf) ) goto error;
    if( hdr.num_anims > 0 && !load_anims( mdl, filename, &hdr, buf) ) goto error;

    fclose(f);
    free(buf);
    return mdl;

error:
    g_debug_str("%s: error while loading\n", filename);
    model_destroy( mdl );
    free( buf );
    fclose( f );
    return NULL;
}

void model_destroy( Model *mdl ){
    if( !mdl ) return;
    if( mdl->str ) g_free( mdl->str );
    if( mdl->meshes ) g_free( mdl->meshes );
    if( mdl->tris ) g_free( mdl->tris );
    if( mdl->verts ) g_free( mdl->verts );
    if( mdl->joints ) g_free( mdl->joints );
    if( mdl->outframe ) g_free( mdl->outframe );
    if( mdl->frame ) g_free( mdl->frame );
    if( mdl->out ) g_free( mdl->out );
    if( mdl->textures ) g_free( mdl->textures );
    g_free( mdl );
}

void model_setup_buffers( Model *mdl ) {
    glGenBuffers( 1, &mdl->vbo );
    glBindBuffer( GL_ARRAY_BUFFER, mdl->vbo );
    glBufferData( GL_ARRAY_BUFFER, mdl->num_verts * sizeof(Vertex), mdl->verts, GL_STATIC_DRAW);

    glGenBuffers( 1, &mdl->ibo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mdl->ibo );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, mdl->num_tris * sizeof(Triangle), mdl->tris, GL_STATIC_DRAW);
}

//TODO: add support for normals and normal mapping
void model_draw( Model *mdl, Program* program, Mat4* mvp, float frame ){
    int i;
    animate_model( mdl, frame );
    glUseProgram( program->object );
    glBindBuffer( GL_ARRAY_BUFFER, mdl->vbo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mdl->ibo );

    for( i=0; i < 6; i++ ) glEnableVertexAttribArray( i ); // pos, normal, uv, tangent, bone_id, bone_weight

    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)0 );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)12 );
    glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)24 );
    glVertexAttribPointer( 3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)32 );
    glVertexAttribPointer( 4, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (GLvoid *)48 );
    glVertexAttribPointer( 5, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLvoid *)52 );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, mdl->textures[0] );
    glUniformMatrix4fv( program->uniforms[0], 1, GL_FALSE, mvp->m );
    glUniform1i( program->uniforms[1], 0); //GL_TEXTURE
    glUniformMatrix4fv( program->uniforms[2], mdl->num_joints, GL_FALSE, mdl->outframe[0].m );

    for( i=0; i<mdl->num_meshes; i++ ) {
        Mesh *m = &mdl->meshes[i];
        glDrawElements( GL_TRIANGLES, 3*m->num_triangles, GL_UNSIGNED_INT, (GLvoid*)(m->first_triangle*sizeof(Triangle)) );
    }

    for( i=0; i < 6; i++ ) glDisableVertexAttribArray( i ); // pos, normal, uv, tangent, bone_id, bone_weight
}

#define DIST_EPSILON    (0.01f)  // 2 cm epsilon for triangle collision
#define MIN_FRACTION    (0.0005f) // at least 0.5% movement along the direction vector

int model_collision( Model *mdl, Vec* pos, Vec* dir, float radius, Vec *result ) {
    int i, j;
    TraceInfo ti;
    trace_init( &ti, pos, dir, radius );
    for( i = 0; i < mdl->num_meshes; i++ ) {
        Mesh* m = &mdl->meshes[i];
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
            // Vec norm;
            // vec_normalize( &norm, vec_sub(&norm, &ti.start, &ti.intersect_point) );
            // float t = ti.t + DIST_EPSILON/vec_dot( &norm, &ti.vel );
            // if( t < MIN_FRACTION ) t = .0f;
            vec_add( result, &ti.start, vec_scale( result, &ti.vel, ti.t) );
        } else {
            *result = ti.intersect_point;
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
