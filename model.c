#include "myr.h"

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
} LMeshHeader;

typedef struct {
  unsigned int name, material;
  unsigned int first_vertex, num_vertexes;
  unsigned int first_triangle, num_triangles;
} LMeshSubMesh;

typedef struct {
  unsigned int vertex[3];
} LMeshTriangle;

typedef struct {
  unsigned int name;
  int parent;
  GVec translate, inv_translate;
  GQuat rotate, inv_rotate;
  GVec scale, inv_scale;
} LMeshJoint;

typedef struct {
  int parent;
  unsigned int mask;
  float channeloffset[10], channelscale[10];
} LMeshPose;

typedef struct {
  unsigned int name, first_frame, num_frames;
  float framerate;
  unsigned int flags;
} LMeshAnim;

enum { IQM_LOOP = 1<<0 };

typedef struct {
  float bbmin[3], bbmax[3];
  float xyradius, radius;
} IqmBounds;

typedef struct{
  GVec loc, normal;
  GVec2 texcoord;
  GVec4 tangent;
  unsigned char blendindex[4], blendweight[4];
} LMeshVertex;

struct _GModel {
  int num_meshes, num_verts, num_tris, num_joints, num_frames, num_anims;
  char *str;

  LMeshSubMesh *meshes;
  LMeshVertex *verts, *out_verts;
  LMeshTriangle *tris;
  GLuint *textures;
  LMeshJoint *joints;
  LMeshPose *poses;
  LMeshAnim *anims;
  IqmBounds *bounds;

  GDualQuat *base, *inversebase, *outframe, *frames; //in iqm demo its a 3x4 matrix
  GVec *base_trans, *inv_trans, *frame_trans, *out_trans;
  GQuat *base_rot, *inv_rot, *frame_rot, *out_rot;
};


static int loadiqmmeshes( GModel *mdl, const char *filename, const LMeshHeader *hdr, unsigned char *buf ) {
    mdl->num_meshes = hdr->num_meshes;
    mdl->num_tris = hdr->num_triangles;
    mdl->num_verts = hdr->num_vertexes;
    mdl->num_joints = hdr->num_joints;

    mdl->meshes = g_new( LMeshSubMesh, mdl->num_meshes );
    mdl->tris = g_new( LMeshTriangle, mdl->num_tris );
    mdl->verts = g_new( LMeshVertex, mdl->num_verts );
    mdl->joints = g_new( LMeshJoint, mdl->num_joints );
    mdl->textures = g_new0( GLuint, mdl->num_meshes );

    const char *str = hdr->ofs_text ? (char *)&buf[hdr->ofs_text] : "";

    int i;
    LMeshVertex* verts = (LMeshVertex *)&buf[hdr->ofs_vertexes];
    LMeshTriangle * tris = (LMeshTriangle *) &buf[hdr->ofs_triangles];
    LMeshSubMesh* meshes = (LMeshSubMesh *) &buf[hdr->ofs_meshes];
    LMeshJoint* joints = (LMeshJoint *) &buf[hdr->ofs_joints];

    for( i=0; i<mdl->num_verts; i++ ) mdl->verts[i] = verts[i];
    for( i=0; i<mdl->num_tris; i++ ) mdl->tris[i] = tris[i];
    for( i=0; i<mdl->num_meshes; i++ ) mdl->meshes[i] = meshes[i];
    for( i=0; i<mdl->num_joints; i++ ) mdl->joints[i] = joints[i];

    mdl->base_trans = g_new( GVec, hdr->num_joints );
    mdl->inv_trans = g_new( GVec, hdr->num_joints );

    mdl->base_rot = g_new( GQuat, hdr->num_joints );
    mdl->inv_rot = g_new( GQuat, hdr->num_joints );

    for( i = 0; i < (int) hdr->num_joints; i++ ) {
        LMeshJoint *j = &joints[i];
        g_quat_normalize( &j->rotate );
        mdl->base_trans[i] = j->translate;
        mdl->inv_rot[i] = mdl->base_rot[i] = j->rotate;

        g_quat_invert( &mdl->inv_rot[i] );
        g_quat_normalize( &mdl->inv_rot[i] );
        g_vec_mul_scalar( &mdl->inv_trans[i], &j->translate, -1.0 );
        g_quat_vec_mul( &mdl->inv_trans[i], &mdl->inv_rot[i], &mdl->inv_trans[i] );
    }

    GTexture tex;
    for( i = 0; i < (int)hdr->num_meshes; i++ ) {
        LMeshSubMesh *m = &mdl->meshes[i];
        g_debug_str("%s: loaded mesh: %s\n", filename, &str[m->name]);

        mdl->textures[i] = g_texture_load( &tex, &str[m->material] );
        if( mdl->textures[i] ) g_debug_str("%s: loaded material: %s\n", filename, &str[m->material]);
        else g_debug_str("%s: couldn't load material: %s\n", filename, &str[m->material]);
    }

    return 1;
}

static int loadiqmanims( GModel* mdl, const char *filename, const LMeshHeader *hdr, unsigned char *buf ) {
    if((int)hdr->num_poses != mdl->num_joints) return 0;

    const char *str = hdr->ofs_text ? (char *)&buf[hdr->ofs_text] : "";
    mdl->num_anims = hdr->num_anims;
    mdl->num_frames = hdr->num_frames;
    mdl->anims = (LMeshAnim *)&buf[hdr->ofs_anims];
    mdl->poses = (LMeshPose *)&buf[hdr->ofs_poses];
    mdl->frames = g_new( GDualQuat, hdr->num_frames * hdr->num_poses );
    mdl->outframe = g_new( GDualQuat, hdr->num_joints);
    mdl->out_verts = g_new( LMeshVertex, mdl->num_verts );

    mdl->frame_rot = g_new( GQuat, hdr->num_frames * hdr->num_poses );
    mdl->frame_trans = g_new( GVec, hdr->num_frames * hdr->num_poses );
    mdl->out_rot = g_new( GQuat, hdr->num_joints);
    mdl->out_trans = g_new( GVec, hdr->num_joints);

    //TODO: load bounds data
    unsigned short *framedata = (unsigned short *)&buf[hdr->ofs_frames];
    // if( hdr->ofs_bounds ) mdl->bounds = (IqmBounds *)&buf[hdr->ofs_bounds];

    // for (int i = 0; i < nb_bones; i++) {
    //   const rend::Bone& bone = skeleton.bones[i];
    //   const rend::Keyframe& kf = keyframes[i + frame * nb_bones];

    //   // Calculate inverse base pose
    //   math::quat q0 = qConjugate(bone.base_pose.rotation);
    //   math::vec3 p0 = qTransformPos(q0, v3Negate(bone.base_pose.position));

    //   // Concatenate with animation keyframe
    //   math::quat q = qMultiply(q0, kf.rotation);
    //   math::vec3 p = qTransformPos(kf.rotation, p0);
    //   p = v3Add(p, kf.position);

    //   // Set the transform
    //   math::mat4& transform = transforms[i];
    //   transform = qToMat4(q);
    //   transform.r[3] = math::v4Make(p.x, p.y, p.z, 1);
    // }

    int i, j;
    for( i = 0; i < (int)hdr->num_frames; i++ ) {
        for( j = 0; j < (int)hdr->num_poses; j++ ) {
            LMeshPose *p = &mdl->poses[j];
            GQuat rotate;
            GVec translate;

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
            g_quat_normalize( &rotate );

            // g_dual_quat_mul( &mdl->frames[k], &mdl->frames[k], &mdl->inversebase[j] );
            GVec rv, pv;
            GQuat rq, pq;
            g_quat_mul( &rq, &rotate, &mdl->inv_rot[j] );
            g_quat_vec_mul( &rv, &rotate, &mdl->inv_trans[j] );
            g_vec_add( &rv, &rv, &translate );
            // g_quat_normalize( &rq );

            if( p->parent >= 0) {
                g_quat_mul( &pq, &mdl->base_rot[p->parent], &rq );
                g_quat_vec_mul( &pv, &mdl->base_rot[p->parent], &rv );
                g_vec_add( &pv, &pv, &mdl->base_trans[p->parent] );

                mdl->frame_trans[k] = pv;
                mdl->frame_rot[k] = pq;
            } else {
                mdl->frame_trans[k] = rv;
                mdl->frame_rot[k] = rq;
            }

        }
    }

    for( i = 0; i < (int)hdr->num_anims; i++ ) {
        LMeshAnim *a = &mdl->anims[i];
        printf("%s: loaded anim: %s\n", filename, &str[a->name]);
    }

    g_free( mdl->base );
    g_free( mdl->inversebase );

    return 1;
}

static void animateiqm( GModel *mdl, float curframe ) {
    if(!mdl->num_frames) return;
    int i, j;

    int frame1 = (int)floor(curframe), frame2 = frame1 + 1;
    float frameoffset = curframe - frame1;
    frame1 = (frame1 % mdl->num_frames) * mdl->num_joints;
    frame2 = (frame2 % mdl->num_frames) * mdl->num_joints;

    // Interpolate matrixes between the two closest frames and concatenate with parent matrix if necessary.
    // Concatenate the result with the inverse of the base pose.
    // You would normally do animation blending and inter-frame blending here in a 3D engine.
    for( i = 0; i < mdl->num_joints; i++ ) {

        GVec  rv, dv1 = mdl->frame_trans[frame1+i], dv2 = mdl->frame_trans[frame2+i];
        GQuat rq, dq1 = mdl->frame_rot[frame1+i],   dq2 = mdl->frame_rot[frame2+i];

        // g_dual_quat_lerp( &r, &d1, &d2, frameoffset );
        g_vec_lerp( &rv, &dv1, &dv2, frameoffset );
        g_quat_lerp( &rq, &dq1, &dq2, frameoffset );
        if( mdl->joints[i].parent >= 0) {
          int parent = mdl->joints[i].parent;

          g_quat_mul( &mdl->out_rot[i], &mdl->out_rot[parent], &rq );
          g_quat_vec_mul( &mdl->out_trans[i], &mdl->out_rot[parent], &rv );
          g_vec_add( &mdl->out_trans[i], &mdl->out_trans[i], &mdl->out_trans[parent] );

          g_quat_normalize( &mdl->out_rot[i] );
        } else {
          mdl->out_trans[i] = rv;
          mdl->out_rot[i] = rq;
        }
    }

    // The actual vertex generation based on the matrixes follows...
    for( i = 0; i < mdl->num_verts; i++) {
        LMeshVertex* v = &mdl->verts[i];
        // weighted blend of bone transformations assigned to this vert ( here for fixed pipeline )
        GQuat rq = {.0, .0, .0, .0};
        GVec rv = {.0, .0, .0};
        for( j = 0; j < 4 && v->blendweight[j]; j++ ) {
            int blend = v->blendindex[j];
            float weight = v->blendweight[j] / 255.0f;

            g_quat_scale_add( &rq, &mdl->out_rot[blend], weight );
            g_vec_scale_add( &rv, &mdl->out_trans[blend], weight );
        }
        g_quat_normalize( &rq );

        // Transform attributes by the blended dual quaternion.
        LMeshVertex* ov = &mdl->out_verts[i];
        g_quat_vec_mul( &ov->loc, &rq, &v->loc );
        g_vec_add( &ov->loc, &ov->loc, &rv );

//  *dstnorm = matnorm.transform(*srcnorm);
        // Note that input tangent data has 4 coordinates,
        // so only transform the first 3 as the tangent vector.
//  *dsttan = matnorm.transform(Vec3(*srctan));
        // Note that bitangent = cross(normal, tangent) * sign,
        // where the sign is stored in the 4th coordinate of the input tangent data.
//  *dstbitan = dstnorm->cross(*dsttan) * srctan->w;
    }
}

//TODO: add a resource manager for this assets
GModel* g_model_load( const char *filename ){
    char filepath[256];
    sprintf( filepath, "data/models/%s", filename );

    FILE *f = fopen(filepath, "rb");
    if(!f) return NULL;
    GModel* mdl = g_new0( GModel, 1 );

    unsigned char *buf = NULL;
    LMeshHeader hdr;
    if( fread(&hdr, 1, sizeof(hdr), f) != sizeof(hdr) || memcmp(hdr.magic, LMESH_MAGIC, sizeof(hdr.magic)) )
        goto error;

    if( hdr.version != LMESH_VERSION || hdr.filesize > (16<<20) ) goto error; // sanity check... don't load files bigger than 16 MB

    buf = (unsigned char*) malloc( hdr.filesize );
    if( fread(buf + sizeof(hdr), 1, hdr.filesize - sizeof(hdr), f) != hdr.filesize - sizeof(hdr) )
      goto error;

    if( hdr.num_meshes > 0 && !loadiqmmeshes( mdl, filename, &hdr, buf) ) goto error;
    if( hdr.num_anims > 0 && !loadiqmanims( mdl, filename, &hdr, buf) ) goto error;

    fclose(f);
    free(buf);
    return mdl;

    error:
    g_fatal_error("%s: error while loading\n", filename);
    free( buf );
    g_model_destroy( mdl );
    fclose(f);
    return NULL;
}

void g_model_destroy( GModel *mdl ){
    if( !mdl ) return;
    if( mdl->str ) g_free( mdl->str );
    if( mdl->meshes ) g_free( mdl->meshes );
    if( mdl->tris ) g_free( mdl->tris );
    if( mdl->verts ) g_free( mdl->verts );
    if( mdl->out_verts ) g_free( mdl->out_verts );
    if( mdl->joints ) g_free( mdl->joints );
    if( mdl->frames ) g_free( mdl->frames );
    if( mdl->outframe ) g_free( mdl->outframe );

    if( mdl->textures ) g_free( mdl->textures );
    g_free( mdl );
}

//TODO: add support for normals and normal mapping
void g_model_draw( GModel *mdl, float frame ){
    animateiqm( mdl, frame );

    LMeshVertex* v = (mdl->num_frames > 0 ? mdl->out_verts : mdl->verts);
    glVertexPointer(3, GL_FLOAT, sizeof(LMeshVertex), &v[0].loc );

//    glNormalPointer(GL_FLOAT, 0, numframes > 0 ? outnormal : innormal);
    glTexCoordPointer(2, GL_FLOAT, sizeof(LMeshVertex), &mdl->verts[0].texcoord );

    glEnableClientState( GL_VERTEX_ARRAY );
//    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );

    int i;
    for( i = 0; i < mdl->num_meshes; i++ ) {
      LMeshSubMesh *m = &mdl->meshes[i];
      glBindTexture( GL_TEXTURE_2D, mdl->textures[i] );
      glDrawElements( GL_TRIANGLES, 3*m->num_triangles, GL_UNSIGNED_INT, &mdl->tris[m->first_triangle] );
    }

    glDisableClientState(GL_VERTEX_ARRAY);
//    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}
