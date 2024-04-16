enum OpenGLProgramID
{
    OPENGL_PROGRAM_TEXTURED_VERTICES,
    OPENGL_PROGRAM_COLORED_VERTICES,

    OPENGL_PROGRAM_COUNT,
};

struct OpenGLProgram
{
    GLuint id;
    GLuint positionAttribute;
    GLuint normalAttribute;
    GLuint colorAttribute;
    GLuint uvAttribute;
    GLuint modelViewProjectionUniform;
    GLuint colorUniform;
    GLuint textureUniform;
    GLuint uvScaleUniform;
};

enum MeshID
{
    MESH_GROUND,
    MESH_SPHERE,
    MESH_LINE,

    MESH_SKYBOX_SKY,
    MESH_SKYBOX_BG,
    MESH_SKYBOX_GROUND,

    MESH_GOLF_BALL,
    MESH_ARROW,

    MESH_COUNT,
};

enum TextureID
{
    TEXTURE_NONE,

    TEXTURE_SKYBOX_SKY,
    TEXTURE_SKYBOX_BG,
    TEXTURE_SKYBOX_GROUND,

    TEXTURE_FAIRWAY,
    TEXTURE_GOLF_BALL,

    TEXTURE_COUNT,
};

struct ColoredVertex
{
    glm::vec3 position;
    glm::vec3 color;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
    glm::vec2 uv;
};

struct Mesh
{
    const GLvoid *vertexData;
    const GLvoid *indexData;
    GLsizeiptr vertexDataSize;
    GLsizeiptr indexDataSize;

    const GLvoid *positionDataOffset;
    GLsizei positionDataStride;
    const GLvoid *normalDataOffset;
    GLsizei normalDataStride;
    const GLvoid *colorDataOffset;
    GLsizei colorDataStride;
    const GLvoid *uvDataOffset;
    GLsizei uvDataStride;

    size_t vertexCount;
};

struct OpenGLRenderer
{
    GLuint vertexArrays[MESH_COUNT];
    GLuint vertexBuffers[MESH_COUNT];
    GLuint indexBuffers[MESH_COUNT];
    GLuint vertexCounts[MESH_COUNT];

    GLuint textures[TEXTURE_COUNT];

    OpenGLProgram programs[OPENGL_PROGRAM_COUNT];
};

enum Color
{
    WHITE,
    BLACK,
    RED,
    GREEN,
    BLUE,
    CYAN,
    YELLOW,
    MAGENTA,

    COLOR_COUNT,
};

struct ColorRGBA
{
    f32 r, g, b, a;
    ColorRGBA(f32 red, f32 green, f32 blue, f32 alpha) : r(red), g(green), b(blue), a(alpha) {}
};

enum CameraID
{
    CAMERA_1,
    CAMERA_2,
    CAMERA_3,
    CAMERA_4,

    CAMERA_COUNT,
};

struct Camera
{
    glm::vec3 position;
    glm::vec3 up;
    glm::vec3 target;
};

class GolfFlightSim3D : public Application
{
protected:
    bool initialize() override;
    bool load() override;
    void renderScene(f32 frameTime) override;
    void renderUI(f32 frameTime) override;
    void update(f32 frameTime) override;

private:
    MemoryArena mainArena;
    World *world;
    World *previous;

    CollisionGeometry *collidableTriangles;

    OpenGLRenderer *renderer;

    bool loadTexture(TextureID textureID, const char *filepath, GLint wrapS, GLint wrapT);
    void loadCollidableGeometry(Mesh *mesh);
    void loadMesh(MeshID meshId,
                  Vertex *vertexData,
                  u16 *indexData,
                  size_t vertexDataSize,
                  size_t indexDataSize,
                  size_t vertexCount);
    bool loadMeshGLTF(MeshID meshId, const char *filepath, bool collidable);

    void drawTextured(glm::mat4 *model, GLenum mode, MeshID meshId, TextureID textureID, f32 uvScale);
    void drawTexturedTriangles(glm::mat4 *model, MeshID meshId, TextureID textureID, f32 uvScale);
    void drawTexturedLines(glm::mat4 *model, MeshID meshId, TextureID textureID, f32 uvScale);
    void drawTexturedMesh(MeshID meshId, glm::mat4 *transform, TextureID textureID, f32 uvScale);

    void drawColored(glm::mat4 *model, GLenum mode, MeshID meshId, f32 r, f32 g, f32 b, f32 a);
    void drawColoredTriangles(glm::mat4 *model, MeshID meshId, f32 r, f32 g, f32 b, f32 a);
    void drawColoredLines(glm::mat4 *model, MeshID meshId, f32 r, f32 g, f32 b, f32 a);
    void drawColoredMesh(MeshID meshId, glm::mat4 *transform, f32 r, f32 g, f32 b, f32 a);

    void drawMesh(MeshID meshId, glm::mat4 *transform, Color color);
    void drawMesh(MeshID meshId, glm::mat4 *transform, f32 r, f32 g, f32 b, f32 a);
    void drawMesh(MeshID meshId, glm::mat4 *transform, TextureID texture, f32 uvScale);

    void drawVector(const glm::vec3 &v, const glm::vec3 &origin, Color color);
    void drawWindArrow(f32 x, f32 y, f32 scale);
};