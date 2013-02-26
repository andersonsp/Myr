typedef struct {
    char magic[4]; //Land
    unsigned int version;      // 1
    unsigned int filesize;     // ??
    unsigned int flags, size;  // format and size of the vertexes (or'ed ATTR_* flags)
    unsigned int num_text,     ofs_text;
    unsigned int num_meshes,   ofs_meshes;
    unsigned int num_vertexes, ofs_vertexes;
    unsigned int num_indexes,  ofs_indexes, ofs_adjacency;
    unsigned int num_joints,   ofs_joints;
    unsigned int num_poses,    ofs_poses;
    unsigned int num_anims,    ofs_anims;
    unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
} header;

enum {
    ATTR_POSITION     = 1<<0,  // 12 bytes: Vec
    ATTR_NORMAL       = 1<<1,  // 12 bytes: Vec
    ATTR_TEXCOORD     = 1<<2,  //  8 bytes:  Vec2
    ATTR_TANGENT      = 1<<3,  // 16 bytes: Vec4
    ATTR_BLENDINDEXES = 1<<4,  //  4 bytes: 4 indexes (0..255)
    ATTR_BLENDWEIGHTS = 1<<5,  //  4 bytes: 4 weights (0..255)
    ATTR_COLOR        = 1<<6,  //  4 bytes: RGBA value
};

typedef struct {
    unsigned int name;
    int parent;
    Vec translate;
    Quat rotate;
    Vec scale;
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

struct skeleton {
    int num_joints, num_frames, num_anims;

    Joint *joints;
    Pose *poses;
    Anim *anims;
    Bounds *bounds;
    // used to transform animations
    // DualQuat *base, *inversebase, *outframe, *frames;
};

typedef struct {
    float bbmin[3], bbmax[3];
    float xyradius, radius;
} Bounds;

typedef struct {
    unsigned int name, material;
    unsigned int first_vertex, num_vertexes;
    unsigned int first_triangle, num_triangles;
} Mesh;

struct Model {
    int num_meshes, num_verts, num_indexes;
    int format, size;

    Mesh *meshes;
    void *verts;
    unsigned int *indexes, *adjacency;

    GLuint *textures;
    GLuint vbo, ibo;
};
