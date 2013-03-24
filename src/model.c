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
  float channeloffset[10], channelscale[10];
} Pose;

typedef struct {
  unsigned int name, first_frame, num_frames;
  float framerate;
  unsigned int flags;
} Anim;

enum { IQM_LOOP = 1<<0 };

typedef struct {
  float bbmin[3], bbmax[3];
  float xyradius, radius;
} Bounds;

typedef struct{
  Vec loc, normal;
  Vec2 texcoord;
  Vec4 tangent;
  unsigned char blendindex[4], blendweight[4];
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
  Mat4 *frame; //in iqm demo its a 3x4 matrix

  Vec *base_trans, *inv_trans, *frame_trans, *out_trans;
  Quat *base_rot, *inv_rot, *frame_rot, *out_rot;
};


static int load_meshes( Model *mdl, const char *filename, const ModelHeader *hdr, unsigned char *buf ) {
    mdl->num_meshes = hdr->num_meshes;
    mdl->num_tris = hdr->num_triangles;
    mdl->num_verts = hdr->num_vertexes;
    mdl->num_joints = hdr->num_joints;

    mdl->meshes = g_new( Mesh, mdl->num_meshes );
    mdl->tris = g_new( Triangle, mdl->num_tris );
    mdl->verts = g_new( Vertex, mdl->num_verts );
    mdl->joints = g_new( Joint, mdl->num_joints );
    mdl->textures = g_new0( GLuint, mdl->num_meshes );

    const char *str = hdr->ofs_text ? (char *)&buf[hdr->ofs_text] : "";

    int i;
    Vertex* verts = (Vertex *)&buf[hdr->ofs_vertexes];
    Triangle * tris = (Triangle *) &buf[hdr->ofs_triangles];
    Mesh* meshes = (Mesh *) &buf[hdr->ofs_meshes];
    Joint* joints = (Joint *) &buf[hdr->ofs_joints];

    for( i=0; i<mdl->num_verts; i++ ) mdl->verts[i] = verts[i];
    for( i=0; i<mdl->num_tris; i++ ) mdl->tris[i] = tris[i];
    for( i=0; i<mdl->num_meshes; i++ ) mdl->meshes[i] = meshes[i];
    for( i=0; i<mdl->num_joints; i++ ) mdl->joints[i] = joints[i];

    mdl->base_trans = g_new( Vec, hdr->num_joints );
    mdl->inv_trans = g_new( Vec, hdr->num_joints );

    mdl->base_rot = g_new( Quat, hdr->num_joints );
    mdl->inv_rot = g_new( Quat, hdr->num_joints );

    for( i = 0; i < (int) hdr->num_joints; i++ ) {
        Joint *j = &joints[i];
        quat_normalize( &j->rotate, &j->rotate );
        mdl->base_trans[i] = j->translate;
        mdl->inv_rot[i] = mdl->base_rot[i] = j->rotate;

        quat_normalize( &mdl->inv_rot[i], quat_invert(&mdl->inv_rot[i], &mdl->inv_rot[i]) );
        vec_scale( &mdl->inv_trans[i], &j->translate, -1.0 );
        quat_vec_mul( &mdl->inv_trans[i], &mdl->inv_rot[i], &mdl->inv_trans[i] );
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
    if((int)hdr->num_poses != mdl->num_joints) return 0;

    const char *str = hdr->ofs_text ? (char *)&buf[hdr->ofs_text] : "";
    mdl->num_anims = hdr->num_anims;
    mdl->num_frames = hdr->num_frames;
    mdl->anims = (Anim *)&buf[hdr->ofs_anims];
    mdl->poses = (Pose *)&buf[hdr->ofs_poses];
    // mdl->frame = g_new( DualQuat, hdr->num_joints);
    mdl->frame = g_new( Mat4, hdr->num_joints);

    mdl->frame_rot = g_new( Quat, hdr->num_frames * hdr->num_poses );
    mdl->frame_trans = g_new( Vec, hdr->num_frames * hdr->num_poses );
    mdl->out_rot = g_new( Quat, hdr->num_joints);
    mdl->out_trans = g_new( Vec, hdr->num_joints);

    //TODO: load bounds data
    unsigned short *framedata = (unsigned short *)&buf[hdr->ofs_frames];
    // if( hdr->ofs_bounds ) mdl->bounds = (IqmBounds *)&buf[hdr->ofs_bounds];

    int i, j;
    for( i = 0; i < (int)hdr->num_frames; i++ ) {
        for( j = 0; j < (int)hdr->num_poses; j++ ) {
            Pose *p = &mdl->poses[j];
            Quat rotate;
            Vec translate;

            translate.x = p->channeloffset[0]; if(p->mask&0x01) translate.x += *framedata++ * p->channelscale[0];
            translate.y = p->channeloffset[1]; if(p->mask&0x02) translate.y += *framedata++ * p->channelscale[1];
            translate.z = p->channeloffset[2]; if(p->mask&0x04) translate.z += *framedata++ * p->channelscale[2];

            rotate.x = p->channeloffset[3]; if(p->mask&0x08) rotate.x += *framedata++ * p->channelscale[3];
            rotate.y = p->channeloffset[4]; if(p->mask&0x10) rotate.y += *framedata++ * p->channelscale[4];
            rotate.z = p->channeloffset[5]; if(p->mask&0x20) rotate.z += *framedata++ * p->channelscale[5];
            rotate.w = p->channeloffset[6]; if(p->mask&0x40) rotate.w += *framedata++ * p->channelscale[6];

            if( p->mask&0x80 || p->mask&0x100 || p->mask&0x200 ){
              g_debug_str("bone scaling is disabled...\n");
              return 0;
            }

            // Concatenate each pose with the inverse base pose to avoid doing this at animation time.
            // If the joint has a parent, then it needs to be pre-concatenated with its parent's base pose.
            int k = i*hdr->num_poses + j;
            quat_normalize( &rotate, &rotate );

            Vec rv, pv;
            Quat rq, pq;
            quat_mul( &rq, &rotate, &mdl->inv_rot[j] );
            quat_vec_mul( &rv, &rotate, &mdl->inv_trans[j] );
            vec_add( &rv, &rv, &translate );

            if( p->parent >= 0) {
                quat_mul( &pq, &mdl->base_rot[p->parent], &rq );
                quat_vec_mul( &pv, &mdl->base_rot[p->parent], &rv );
                vec_add( &pv, &pv, &mdl->base_trans[p->parent] );

                mdl->frame_trans[k] = pv;
                mdl->frame_rot[k] = pq;
            } else {
                mdl->frame_trans[k] = rv;
                mdl->frame_rot[k] = rq;
            }

        }
    }

    for( i = 0; i < (int)hdr->num_anims; i++ ) {
        Anim *a = &mdl->anims[i];
        g_debug_str("%s: loaded anim: %s\n", filename, &str[a->name]);
    }

    g_free( mdl->base_trans );
    g_free( mdl->base_rot );
    g_free( mdl->inv_trans );
    g_free( mdl->inv_rot );

    return 1;
}

static void animate_model( Model *mdl, float curframe ) {
    if(!mdl->num_frames) return;
    int i;

    int frame1 = (int)floor(curframe), frame2 = frame1 + 1;
    float frameoffset = curframe - frame1;
    frame1 = (frame1 % mdl->num_frames) * mdl->num_joints;
    frame2 = (frame2 % mdl->num_frames) * mdl->num_joints;

    // Interpolate matrixes between the two closest frames and concatenate with parent matrix if necessary.
    // Concatenate the result with the inverse of the base pose.
    // You would normally do animation blending and inter-frame blending here in a 3D engine.
    for( i = 0; i < mdl->num_joints; i++ ) {
        Vec  rv, dv1 = mdl->frame_trans[frame1+i], dv2 = mdl->frame_trans[frame2+i];
        Quat rq, dq1 = mdl->frame_rot[frame1+i],   dq2 = mdl->frame_rot[frame2+i];

        vec_lerp( &rv, &dv1, &dv2, frameoffset );
        quat_lerp( &rq, &dq1, &dq2, frameoffset );
        if( mdl->joints[i].parent >= 0) {
            int parent = mdl->joints[i].parent;

            quat_mul( &mdl->out_rot[i], &mdl->out_rot[parent], &rq );
            quat_vec_mul( &mdl->out_trans[i], &mdl->out_rot[parent], &rv );
            vec_add( &mdl->out_trans[i], &mdl->out_trans[i], &mdl->out_trans[parent] );

            quat_normalize( &mdl->out_rot[i], &mdl->out_rot[i] );
            mat4_from_quat_vec( &mdl->frame[i], &mdl->out_rot[i], &mdl->out_trans[i] );
        } else {
            mdl->out_trans[i] = rv;
            mdl->out_rot[i] = rq;
            mat4_from_quat_vec( &mdl->frame[i], &mdl->out_rot[i], &mdl->out_trans[i] );
        }
    }

    // The actual vertex generation based on the matrixes follows...
    // GPU skinning here
}

//TODO: add a resource manager for this assets
Model* model_load( const char *filename ) {
    char filepath[256];
    sprintf( filepath, "../data/models/%s", filename );

    FILE *f = fopen(filepath, "rb");
    if(!f) return NULL;
    Model* mdl = g_new0( Model, 1 );

    unsigned char *buf = NULL;
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
    if( mdl->frame ) g_free( mdl->frame );
    if( mdl->frame_trans ) g_free( mdl->frame_trans );
    if( mdl->frame_rot ) g_free( mdl->frame_rot );
    if( mdl->out_trans ) g_free( mdl->out_trans );
    if( mdl->out_rot ) g_free( mdl->out_rot );

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
    glUseProgram( program->object );
    glBindBuffer( GL_ARRAY_BUFFER, mdl->vbo );

    animate_model( mdl, frame );

    glEnableVertexAttribArray( 0 ); // pos
    glEnableVertexAttribArray( 1 ); // normal
    glEnableVertexAttribArray( 2 ); // tex uv
    // glEnableVertexAttribArray( 3 ); // bone_weight
    // glEnableVertexAttribArray( 4 ); // bone_id
    // glEnableVertexAttribArray( 5 ); // tangent
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)0 );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)12 );
    glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)24 );
    // glVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLvoid *)32 );
    // glVertexAttribPointer( 4, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (GLvoid *)36 );
    // glVertexAttribPointer( 5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)40 );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mdl->ibo );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, mdl->textures[0] );
    glUniform1i( program->u_sampler, /*GL_TEXTURE*/0);

    glUniformMatrix4fv( program->u_mvp, 1, GL_FALSE, mvp->m );
    // glUniformMatrix4fv( program->u_bones, mdl->num_joints, GL_FALSE, mdl->frame[0].m );

    Mesh *m = &mdl->meshes[0];
    glDrawElements( GL_TRIANGLES, 3*m->num_triangles, GL_UNSIGNED_INT, (GLvoid*)(m->first_triangle*sizeof(Triangle)) );

    glDisableVertexAttribArray( 0 );
    glDisableVertexAttribArray( 1 );
    glDisableVertexAttribArray( 2 );
    // glDisableVertexAttribArray( 3 );
    // glDisableVertexAttribArray( 4 );
    // glDisableVertexAttribArray( 5 );
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
