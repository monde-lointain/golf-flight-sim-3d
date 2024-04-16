// clang-format off
#include <Framework/Application.hpp>
#include <Framework/MatrixStack.hpp>

#define CGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <cgltf.h>
#include <stb_image.h>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include "GolfFlightSim3D.hpp"
#include "MemoryArena.hpp"
#include "Main.hpp"

#include "GolfFlightSim3D.cpp"
#include "MemoryArena.cpp"
// clang-format on

#define GIGABYTES(n) ((n) * 1024ULL * 1024ULL * 1024ULL)

#define mphToMs(n) ((n) * 0.44704F)
#define msToMph(n) ((n) * 2.23694F)

#define ftSToMs(n) ((n) * 0.3048F)
#define msToFtS(n) ((n) * 3.28084F)

#define metersToYards(n) ((n) * 1.09361F)
#define yardsToMeters(n) ((n) * 0.9144F)

const char *coloredVertexShader = R"FOO(
#version 330 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec4 vertexColor;
layout(location = 3) in vec2 vertexUV;

out vec4 fragmentColor;
out vec2 fragmentUV;
out vec3 worldNormal;

uniform mat4 modelViewProjection;

void main()
{
    gl_Position = modelViewProjection * vec4(vertexPosition, 1.0F);
    fragmentColor = vertexColor, 1.0F;
    fragmentUV = vertexUV;
    worldNormal = vertexNormal;
}
)FOO";

const char *coloredFragmentShader = R"FOO(
#version 330 core

in vec4 fragmentColor;
in vec2 fragmentUV;
in vec3 worldNormal;

out vec4 outColor;

uniform vec4 uniformColor;

void main()
{
    outColor = fragmentColor * uniformColor;
}
)FOO";

const char *texturedVertexShader = R"FOO(
#version 330 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec4 vertexColor;
layout(location = 3) in vec2 vertexUV;

out vec4 fragmentColor;
out vec2 fragmentUV;
out vec3 worldNormal;

uniform float uvScale;
uniform mat4 modelViewProjection;

void main()
{
    gl_Position = modelViewProjection * vec4(vertexPosition, 1.0F);
    fragmentColor = vertexColor, 1.0F;
    fragmentUV = vertexUV * uvScale;
    worldNormal = vertexNormal;
}
)FOO";

const char *texturedFragmentShader = R"FOO(
#version 330 core

in vec4 fragmentColor;
in vec2 fragmentUV;
in vec3 worldNormal;

out vec4 outColor;

uniform sampler2D textureSampler;

void main()
{
    vec4 texColor = texture(textureSampler, fragmentUV);
    outColor = fragmentColor * texColor;
}
)FOO";

Vertex lineVertexData[] = {
    {{0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {1.0F, 1.0F, 1.0F, 1.0F}, {0.0F, 0.0F}},
    {{1.0F, 1.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, {1.0F, 1.0F, 1.0F, 1.0F}, {1.0F, 1.0F}},
};
u16 lineIndexData[] = {0, 1};

Camera cameras[] = {
    {{-200.0F, 1.0F, 150.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.1F, 150.0F}},
    {{0.0F, 200.0F, 150.0F}, {0.0F, 1.0F, 0.0F}, {-1.0F, 0.1F, 150.0F}},
    {{-3.0F, 1.0F, -5.0F}, {0.0F, 1.0F, 0.0F}, {-1.0F, 0.1F, 150.0F}},
    {{0.0F, 0.1F, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.3F, 1.0F}},
};

static CameraID currentCamera = CAMERA_3;

static bool wireframe = false;
static bool showForceVectors = true;

static glm::mat4 projection;
static glm::mat4 view;

static f32 elapsedTime = 0.0F;
static f32 deltaTime = 1.0F / 60.0F;
static f32 accumulator = 0.0F;

static void onResize(GLFWwindow *window, s32 width, s32 height)
{
    (void)window;

    glViewport(0, 0, width, height);

    f32 fovy = 45.0F;
    f32 aspect = (f32)width / (f32)height;
    f32 znear = 0.1F;
    f32 zfar = 1000.0F;

    projection = glm::perspective(fovy, aspect, znear, zfar);
}

static void onKeyPressed(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    (void)window;
    (void)mods;
    (void)action;
    (void)scancode;

    switch (key)
    {
        case GLFW_KEY_ESCAPE:
        {
            glfwSetWindowShouldClose(window, 1);
            break;
        }
        case GLFW_KEY_1:
        {
            currentCamera = CAMERA_1;
            break;
        }
        case GLFW_KEY_2:
        {
            currentCamera = CAMERA_2;
            break;
        }
        case GLFW_KEY_3:
        {
            currentCamera = CAMERA_3;
            break;
        }
        case GLFW_KEY_Z:
        {
            wireframe = true;
            break;
        }
        case GLFW_KEY_X:
        {
            wireframe = false;
            break;
        }
        default:
        {
            break;
        }
    }
}

static GLuint openGLCreateShader(GLenum shaderType, const char *shaderSource)
{
    assert(shaderType == GL_VERTEX_SHADER || shaderType == GL_FRAGMENT_SHADER);

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);
    return shader;
}

static void openGLCreateProgram(OpenGLProgram *result, const char *vertexShaderSource, const char *fragmentShaderSource)
{
    GLuint vertexShader = openGLCreateShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = openGLCreateShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glValidateProgram(program);
    GLint status;
    char vertexLog[4096];
    char fragmentLog[4096];
    char programLog[4096];
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        glGetShaderInfoLog(vertexShader, sizeof(vertexLog), NULL, vertexLog);
        glGetShaderInfoLog(fragmentShader, sizeof(fragmentLog), NULL, fragmentLog);
        glGetProgramInfoLog(program, sizeof(programLog), NULL, programLog);

        spdlog::error("OpenGL: Shader validation failed");
        assert(false);
    }

    result->id = program;
    result->positionAttribute = 0;
    result->normalAttribute = 1;
    result->colorAttribute = 2;
    result->uvAttribute = 3;
    result->modelViewProjectionUniform = glGetUniformLocation(program, "modelViewProjection");
    result->colorUniform = glGetUniformLocation(program, "uniformColor");
    result->textureUniform = glGetUniformLocation(program, "textureSampler");
    result->uvScaleUniform = glGetUniformLocation(program, "uvScale");

    glBindAttribLocation(program, result->positionAttribute, "vertexPosition");
    glBindAttribLocation(program, result->normalAttribute, "vertexNormal");
    glBindAttribLocation(program, result->colorAttribute, "vertexColor");
    glBindAttribLocation(program, result->uvAttribute, "vertexUV");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

static ColorRGBA getColorRGBA(Color color)
{
    switch (color)
    {
        case WHITE:
        {
            ColorRGBA result = {1.0F, 1.0F, 1.0F, 1.0F};
            return result;
        }
        case BLACK:
        {
            ColorRGBA result = {0.0F, 0.0F, 0.0F, 1.0F};
            return result;
        }
        case RED:
        {
            ColorRGBA result = {1.0F, 0.0F, 0.0F, 1.0F};
            return result;
        }
        case GREEN:
        {
            ColorRGBA result = {0.0F, 1.0F, 0.0F, 1.0F};
            return result;
        }
        case BLUE:
        {
            ColorRGBA result = {0.0F, 0.0F, 1.0F, 1.0F};
            return result;
        }
        case CYAN:
        {
            ColorRGBA result = {0.0F, 1.0F, 1.0F, 1.0F};
            return result;
        }
        case YELLOW:
        {
            ColorRGBA result = {1.0F, 1.0F, 0.0F, 1.0F};
            return result;
        }
        case MAGENTA:
        {
            ColorRGBA result = {1.0F, 0.0F, 1.0F, 1.0F};
            return result;
        }
        default:
        {
            invalidDefaultCase;
        }
    }

    // Silence the compiler warning. We should never get here
    ColorRGBA result = {1.0F, 0.0F, 1.0F, 1.0F};
    return result;
}

static bool loadGLTF(const char *filepath, Mesh *result)
{
    cgltf_options options = {};
    cgltf_data *gltfData = NULL;
    if (cgltf_parse_file(&options, filepath, &gltfData) != cgltf_result_success)
    {
        spdlog::error("Could not load \"{}\"", filepath);
        return false;
    }

    cgltf_load_buffers(&options, gltfData, filepath);

    cgltf_mesh *mesh = &gltfData->meshes[0];
    cgltf_primitive *primitive = &mesh->primitives[0];

    cgltf_buffer_view *positionData = NULL;
    cgltf_buffer_view *normalData = NULL;
    cgltf_buffer_view *colorData = NULL;
    cgltf_buffer_view *uvData = NULL;

    for (size_t attributeIndex = 0; attributeIndex < primitive->attributes_count; attributeIndex++)
    {
        cgltf_attribute *attribute = &primitive->attributes[attributeIndex];

        switch (attribute->type)
        {
            case cgltf_attribute_type_position:
            {
                positionData = attribute->data->buffer_view;
                break;
            }
            case cgltf_attribute_type_normal:
            {
                normalData = attribute->data->buffer_view;
                break;
            }
            case cgltf_attribute_type_color:
            {
                colorData = attribute->data->buffer_view;
                break;
            }
            case cgltf_attribute_type_texcoord:
            {
                uvData = attribute->data->buffer_view;
                break;
            }
            default:
            {
                spdlog::debug("Unused cgltf attribute type: {}", attribute->type);
                break;
            }
        }
    }

    if (positionData == NULL)
    {
        spdlog::error("No position data for \"{}\"", filepath);
        return false;
    }
    if (normalData == NULL)
    {
        spdlog::error("No normal data for \"{}\"", filepath);
        return false;
    }
    if (colorData == NULL)
    {
        spdlog::error("No color data for \"{}\"", filepath);
        return false;
    }
    if (uvData == NULL)
    {
        spdlog::error("No UV data for \"{}\"", filepath);
        return false;
    }

    cgltf_buffer *vertexData = positionData->buffer;

    result->vertexData = vertexData->data;
    result->vertexDataSize = (GLsizeiptr)vertexData->size;

    result->positionDataOffset = (const GLvoid *)positionData->offset;
    result->positionDataStride = (GLsizei)positionData->stride;
    result->normalDataOffset = (const GLvoid *)normalData->offset;
    result->normalDataStride = (GLsizei)normalData->stride;
    result->colorDataOffset = (const GLvoid *)colorData->offset;
    result->colorDataStride = (GLsizei)colorData->stride;
    result->uvDataOffset = (const GLvoid *)uvData->offset;
    result->uvDataStride = (GLsizei)uvData->stride;

    cgltf_buffer_view *indexData = primitive->indices->buffer_view;
    result->indexData = (const GLvoid *)((size_t)indexData->buffer->data + indexData->offset);
    result->indexDataSize = (GLsizeiptr)indexData->size;

    result->vertexCount = primitive->indices->count;

    return true;
}

bool GolfFlightSim3D::loadTexture(TextureID textureID,
                                  const char *filepath,
                                  GLint wrapS = GL_REPEAT,
                                  GLint wrapT = GL_REPEAT)
{
    s32 width, height, comp;
    u8 *pixels = stbi_load(filepath, &width, &height, &comp, STBI_rgb_alpha);
    if (pixels == NULL)
    {
        spdlog::error("Failed to load \"{}\"", filepath);
        return false;
    }

    glGenTextures(1, &renderer->textures[textureID]);
    glBindTexture(GL_TEXTURE_2D, renderer->textures[textureID]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid *)pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free((void *)pixels);

    return true;
}

void GolfFlightSim3D::loadCollidableGeometry(Mesh *mesh)
{
    size_t numVertices = mesh->vertexDataSize / sizeof(Vertex);

    for (size_t baseVertexIndex = collidableTriangles->vertexCount;
         baseVertexIndex < collidableTriangles->vertexCount + numVertices; baseVertexIndex++)
    {
        Vtx *vertex = &collidableTriangles->vertices[baseVertexIndex];

        size_t vertexIndex = baseVertexIndex - collidableTriangles->vertexCount;

        u8 *basePtr = (u8 *)mesh->vertexData;

        vertex->position = *((glm::vec3 *)(basePtr + (size_t)mesh->positionDataOffset) + vertexIndex);
        vertex->normal = *((glm::vec3 *)(basePtr + (size_t)mesh->normalDataOffset) + vertexIndex);
    }

    collidableTriangles->vertexCount += numVertices;

    size_t numTriangles = mesh->vertexCount / 3;

    for (size_t baseTriangleIndex = collidableTriangles->triangleCount;
         baseTriangleIndex < collidableTriangles->triangleCount + numTriangles; baseTriangleIndex++)
    {
        Triangle *triangle = &collidableTriangles->triangles[baseTriangleIndex];

        size_t triangleIndex = baseTriangleIndex - collidableTriangles->triangleCount;

        size_t offsetA = (triangleIndex * 3) * sizeof(u16);
        size_t offsetB = ((triangleIndex * 3) + 1) * sizeof(u16);
        size_t offsetC = ((triangleIndex * 3) + 2) * sizeof(u16);

        triangle->a = *((u8 *)mesh->indexData + offsetA);
        triangle->b = *((u8 *)mesh->indexData + offsetB);
        triangle->c = *((u8 *)mesh->indexData + offsetC);

        glm::vec3 n0 = collidableTriangles->vertices[triangle->a].normal;
        glm::vec3 n1 = collidableTriangles->vertices[triangle->b].normal;
        glm::vec3 n2 = collidableTriangles->vertices[triangle->c].normal;

        glm::vec3 triangleNormal = glm::normalize((n0 + n1 + n2) / 3.0F);

        triangle->normal = triangleNormal;
    }

    collidableTriangles->triangleCount += numTriangles;
}

void GolfFlightSim3D::loadMesh(MeshID meshId,
                               Vertex *vertexData,
                               u16 *indexData,
                               size_t vertexDataSize,
                               size_t indexDataSize,
                               size_t vertexCount)
{
    glGenVertexArrays(1, &renderer->vertexArrays[meshId]);
    glBindVertexArray(renderer->vertexArrays[meshId]);

    glGenBuffers(1, &renderer->vertexBuffers[meshId]);

    glBindBuffer(GL_ARRAY_BUFFER, renderer->vertexBuffers[meshId]);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertexDataSize, vertexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, NULL);

    glGenBuffers(1, &renderer->indexBuffers[meshId]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->indexBuffers[meshId]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)indexDataSize, indexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, NULL);

    OpenGLProgram *coloredVertexProgram = &renderer->programs[OPENGL_PROGRAM_COLORED_VERTICES];
    OpenGLProgram *texturedVertexProgram = &renderer->programs[OPENGL_PROGRAM_COLORED_VERTICES];

    glBindBuffer(GL_ARRAY_BUFFER, renderer->vertexBuffers[meshId]);
    glEnableVertexAttribArray(coloredVertexProgram->positionAttribute);
    glEnableVertexAttribArray(coloredVertexProgram->normalAttribute);
    glEnableVertexAttribArray(coloredVertexProgram->colorAttribute);
    glEnableVertexAttribArray(coloredVertexProgram->uvAttribute);
    glVertexAttribPointer(coloredVertexProgram->positionAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const GLvoid *)offsetof(Vertex, position));
    glVertexAttribPointer(coloredVertexProgram->normalAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const GLvoid *)offsetof(Vertex, normal));
    glVertexAttribPointer(coloredVertexProgram->colorAttribute, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const GLvoid *)offsetof(Vertex, color));
    glVertexAttribPointer(coloredVertexProgram->uvAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const GLvoid *)offsetof(Vertex, uv));

    glEnableVertexAttribArray(texturedVertexProgram->positionAttribute);
    glEnableVertexAttribArray(texturedVertexProgram->normalAttribute);
    glEnableVertexAttribArray(texturedVertexProgram->colorAttribute);
    glEnableVertexAttribArray(texturedVertexProgram->uvAttribute);
    glVertexAttribPointer(texturedVertexProgram->positionAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const GLvoid *)offsetof(Vertex, position));
    glVertexAttribPointer(texturedVertexProgram->normalAttribute, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const GLvoid *)offsetof(Vertex, normal));
    glVertexAttribPointer(texturedVertexProgram->colorAttribute, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const GLvoid *)offsetof(Vertex, color));
    glVertexAttribPointer(texturedVertexProgram->uvAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const GLvoid *)offsetof(Vertex, uv));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->indexBuffers[meshId]);

    glBindVertexArray(NULL);

    renderer->vertexCounts[meshId] = (GLuint)vertexCount;
}

bool GolfFlightSim3D::loadMeshGLTF(MeshID meshId, const char *filepath, bool collidable = false)
{
    Mesh mesh;

    if (!loadGLTF(filepath, &mesh))
    {
        spdlog::error("Failed to load \"{}\"", filepath);
        return false;
    }

    glGenVertexArrays(1, &renderer->vertexArrays[meshId]);
    glBindVertexArray(renderer->vertexArrays[meshId]);

    glGenBuffers(1, &renderer->vertexBuffers[meshId]);

    glBindBuffer(GL_ARRAY_BUFFER, renderer->vertexBuffers[meshId]);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertexDataSize, mesh.vertexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, NULL);

    glGenBuffers(1, &renderer->indexBuffers[meshId]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->indexBuffers[meshId]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indexDataSize, mesh.indexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, NULL);

    OpenGLProgram *coloredVertexProgram = &renderer->programs[OPENGL_PROGRAM_COLORED_VERTICES];
    OpenGLProgram *texturedVertexProgram = &renderer->programs[OPENGL_PROGRAM_COLORED_VERTICES];

    glBindBuffer(GL_ARRAY_BUFFER, renderer->vertexBuffers[meshId]);
    glEnableVertexAttribArray(coloredVertexProgram->positionAttribute);
    glEnableVertexAttribArray(coloredVertexProgram->normalAttribute);
    glEnableVertexAttribArray(coloredVertexProgram->colorAttribute);
    glEnableVertexAttribArray(coloredVertexProgram->uvAttribute);
    glVertexAttribPointer(coloredVertexProgram->positionAttribute, 3, GL_FLOAT, GL_FALSE, mesh.positionDataStride,
                          mesh.positionDataOffset);
    glVertexAttribPointer(coloredVertexProgram->normalAttribute, 3, GL_FLOAT, GL_FALSE, mesh.normalDataStride,
                          mesh.normalDataOffset);
    glVertexAttribPointer(coloredVertexProgram->colorAttribute, 4, GL_FLOAT, GL_FALSE, mesh.colorDataStride,
                          mesh.colorDataOffset);
    glVertexAttribPointer(coloredVertexProgram->uvAttribute, 2, GL_FLOAT, GL_FALSE, mesh.uvDataStride,
                          mesh.uvDataOffset);

    glEnableVertexAttribArray(texturedVertexProgram->positionAttribute);
    glEnableVertexAttribArray(texturedVertexProgram->normalAttribute);
    glEnableVertexAttribArray(texturedVertexProgram->colorAttribute);
    glEnableVertexAttribArray(texturedVertexProgram->uvAttribute);
    glVertexAttribPointer(texturedVertexProgram->positionAttribute, 3, GL_FLOAT, GL_FALSE, mesh.positionDataStride,
                          mesh.positionDataOffset);
    glVertexAttribPointer(texturedVertexProgram->normalAttribute, 3, GL_FLOAT, GL_FALSE, mesh.normalDataStride,
                          mesh.normalDataOffset);
    glVertexAttribPointer(texturedVertexProgram->colorAttribute, 4, GL_FLOAT, GL_FALSE, mesh.colorDataStride,
                          mesh.colorDataOffset);
    glVertexAttribPointer(texturedVertexProgram->uvAttribute, 2, GL_FLOAT, GL_FALSE, mesh.uvDataStride,
                          mesh.uvDataOffset);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->indexBuffers[meshId]);

    glBindVertexArray(NULL);

    renderer->vertexCounts[meshId] = (GLuint)mesh.vertexCount;

    if (collidable)
    {
        loadCollidableGeometry(&mesh);
    }

    return true;
}

void GolfFlightSim3D::drawTextured(glm::mat4 *model,
                                   GLenum mode,
                                   MeshID meshId,
                                   TextureID textureID,
                                   f32 uvScale = 1.0F)
{
    glm::mat4 modelViewProjection = projection * view * *model;
    OpenGLProgram *texturedVertexProgram = &renderer->programs[OPENGL_PROGRAM_TEXTURED_VERTICES];

    glUseProgram(texturedVertexProgram->id);

    glBindVertexArray(renderer->vertexArrays[meshId]);
    glUniformMatrix4fv((GLint)texturedVertexProgram->modelViewProjectionUniform, 1, GL_FALSE,
                       glm::value_ptr(modelViewProjection));
    glUniform1f((GLint)texturedVertexProgram->uvScaleUniform, uvScale);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->textures[textureID]);

    if (wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glDrawElements(mode, (GLsizei)renderer->vertexCounts[meshId], GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

    glUseProgram(0);
}

void GolfFlightSim3D::drawTexturedTriangles(glm::mat4 *model, MeshID meshId, TextureID textureID, f32 uvScale = 1.0F)
{
    drawTextured(model, GL_TRIANGLES, meshId, textureID, uvScale);
}

void GolfFlightSim3D::drawTexturedLines(glm::mat4 *model, MeshID meshId, TextureID textureID, f32 uvScale = 1.0F)
{
    drawTextured(model, GL_LINES, meshId, textureID, uvScale);
}

void GolfFlightSim3D::drawTexturedMesh(MeshID meshId, glm::mat4 *transform, TextureID textureID, f32 uvScale = 1.0F)
{
    switch (meshId)
    {
        case MESH_GROUND:
        case MESH_SPHERE:
        case MESH_SKYBOX_SKY:
        case MESH_SKYBOX_BG:
        case MESH_SKYBOX_GROUND:
        case MESH_GOLF_BALL:
        case MESH_ARROW:
        {
            drawTexturedTriangles(transform, meshId, textureID, uvScale);
            break;
        }
        case MESH_LINE:
        {
            drawTexturedLines(transform, meshId, textureID, uvScale);
            break;
        }
        default:
        {
            invalidDefaultCase;
        }
    }
}

void GolfFlightSim3D::drawColored(glm::mat4 *model, GLenum mode, MeshID meshId, f32 r, f32 g, f32 b, f32 a)
{
    glm::mat4 modelViewProjection = projection * view * *model;
    OpenGLProgram *coloredVertexProgram = &renderer->programs[OPENGL_PROGRAM_COLORED_VERTICES];

    glUseProgram(coloredVertexProgram->id);

    glBindVertexArray(renderer->vertexArrays[meshId]);
    glUniformMatrix4fv((GLint)coloredVertexProgram->modelViewProjectionUniform, 1, GL_FALSE,
                       glm::value_ptr(modelViewProjection));

    glUniform4f((GLint)coloredVertexProgram->colorUniform, r, g, b, a);

    if (wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glDrawElements(mode, (GLsizei)renderer->vertexCounts[meshId], GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

    glUseProgram(0);
}

void GolfFlightSim3D::drawColoredTriangles(glm::mat4 *model, MeshID meshId, f32 r, f32 g, f32 b, f32 a)
{
    drawColored(model, GL_TRIANGLES, meshId, r, g, b, a);
}

void GolfFlightSim3D::drawColoredLines(glm::mat4 *model, MeshID meshId, f32 r, f32 g, f32 b, f32 a)
{
    drawColored(model, GL_LINES, meshId, r, g, b, a);
}

void GolfFlightSim3D::drawColoredMesh(MeshID meshId, glm::mat4 *transform, f32 r, f32 g, f32 b, f32 a)
{
    switch (meshId)
    {
        case MESH_GROUND:
        case MESH_SPHERE:
        case MESH_SKYBOX_SKY:
        case MESH_SKYBOX_BG:
        case MESH_SKYBOX_GROUND:
        case MESH_GOLF_BALL:
        case MESH_ARROW:
        {
            drawColoredTriangles(transform, meshId, r, g, b, a);
            break;
        }
        case MESH_LINE:
        {
            drawColoredLines(transform, meshId, r, g, b, a);
            break;
        }
        default:
        {
            invalidDefaultCase;
        }
    }
}

void GolfFlightSim3D::drawMesh(MeshID meshId, glm::mat4 *transform, Color color)
{
    ColorRGBA rgba = getColorRGBA(color);
    drawColoredMesh(meshId, transform, rgba.r, rgba.g, rgba.b, rgba.a);
}

void GolfFlightSim3D::drawMesh(MeshID meshId, glm::mat4 *transform, f32 r, f32 g, f32 b, f32 a)
{
    drawColoredMesh(meshId, transform, r, g, b, a);
}

void GolfFlightSim3D::drawMesh(MeshID meshId, glm::mat4 *transform, TextureID texture, f32 uvScale = 1.0F)
{
    drawTexturedMesh(meshId, transform, texture, uvScale);
}

void GolfFlightSim3D::drawVector(const glm::vec3 &v, const glm::vec3 &origin, Color color)
{
    MatrixStack stack;
    stack.push();
    stack.translate(origin);
    stack.scale(v);
    drawMesh(MESH_LINE, stack.top(), color);
    stack.pop();
}

void GolfFlightSim3D::drawWindArrow(f32 x, f32 y, f32 scale)
{
    MatrixStack stack;

    f32 aspect = (f32)_windowWidth / (f32)_windowHeight;

    stack.push();
    stack.translate(x, y, 0.5F);
    stack.rotateX(glm::radians(18.0F));
    stack.rotateY(world->wind.direction);
    stack.scale(scale, scale * aspect, scale);
    drawMesh(MESH_ARROW, stack.top(), 0.0F, 0.75F, 1.0F, 1.0F);
    stack.pop();
}

bool GolfFlightSim3D::initialize()
{
    if (!Application::initialize())
    {
        return false;
    }

    if (!mainArena.initialize(GIGABYTES(1)))
    {
        return false;
    }

    world = (World *)mainArena.allocateFromArena(sizeof(World));
    previous = (World *)mainArena.allocateFromArena(sizeof(World));
    collidableTriangles = (CollisionGeometry *)mainArena.allocateFromArena(sizeof(CollisionGeometry));
    renderer = (OpenGLRenderer *)mainArena.allocateFromArena(sizeof(OpenGLRenderer));

    glfwSetFramebufferSizeCallback(_windowHandle, onResize);
    glfwSetKeyCallback(_windowHandle, onKeyPressed);

    return true;
}

bool GolfFlightSim3D::load()
{
    Application::load();

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);

    openGLCreateProgram(&renderer->programs[OPENGL_PROGRAM_COLORED_VERTICES], coloredVertexShader,
                        coloredFragmentShader);
    openGLCreateProgram(&renderer->programs[OPENGL_PROGRAM_TEXTURED_VERTICES], texturedVertexShader,
                        texturedFragmentShader);

    loadTexture(TEXTURE_SKYBOX_SKY, "./assets/textures/skybox_sky.png");
    loadTexture(TEXTURE_SKYBOX_BG, "./assets/textures/skybox_bg.png", GL_REPEAT, GL_CLAMP_TO_EDGE);
    loadTexture(TEXTURE_SKYBOX_GROUND, "./assets/textures/skybox_ground.png");
    loadTexture(TEXTURE_FAIRWAY, "./assets/textures/fairway.png");
    loadTexture(TEXTURE_GOLF_BALL, "./assets/textures/golf_ball.png");

    loadMeshGLTF(MESH_GROUND, "./assets/primitives/plane.glb", true);
    loadMeshGLTF(MESH_SPHERE, "./assets/primitives/sphere.glb");
    loadMesh(MESH_LINE, lineVertexData, lineIndexData, sizeof(lineVertexData), sizeof(lineIndexData),
             arrayCount(lineIndexData));

    loadMeshGLTF(MESH_SKYBOX_SKY, "./assets/models/skybox_sky.glb");
    loadMeshGLTF(MESH_SKYBOX_BG, "./assets/models/skybox_bg.glb");
    loadMeshGLTF(MESH_SKYBOX_GROUND, "./assets/models/skybox_ground.glb");

    loadMeshGLTF(MESH_GOLF_BALL, "./assets/models/golf_ball.glb");

    loadMeshGLTF(MESH_ARROW, "./assets/models/arrow.glb");

    world->wind.direction = glm::radians(180.0F);
    world->wind.speed = 0.0F;
    world->wind.logWind = false;

    return true;
}

void GolfFlightSim3D::update(float frameTime)
{
    accumulator += frameTime;

    while (accumulator >= deltaTime)
    {
        previous = world;

        world->update(collidableTriangles, deltaTime);

        accumulator -= deltaTime;
    }
}

void GolfFlightSim3D::renderScene(f32 frameTime)
{
    (void)frameTime;

    Camera *camera = &cameras[currentCamera];
    glm::vec3 target = camera->target;

    BallManager *ballManager = &world->ballManager;

    if (ballManager->activeBalls > 0 && currentCamera == CAMERA_3)
    {
        size_t currentBallIndex = ballManager->activeBalls - 1;
        camera->position = ballManager->getBall(currentBallIndex)->position + glm::vec3(-0.6F, 1.05F, -3.0F);
        target = ballManager->getBall(currentBallIndex)->position;
    }

    view = glm::lookAt(camera->position, target, camera->up);

    f32 fovy = 45.0F;
    f32 aspect = (f32)_windowWidth / (f32)_windowHeight;
    f32 znear = 0.1F;
    f32 zfar = 1000.0F;

    projection = glm::perspective(fovy, aspect, znear, zfar);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    MatrixStack stack;

    stack.push();
    stack.rotateY(elapsedTime * world->wind.speed * 0.00033F);
    stack.scale(1000.0F);
    stack.translate(0.0F, -0.1F, 0.0F);
    drawMesh(MESH_SKYBOX_SKY, stack.top(), TEXTURE_SKYBOX_SKY);
    stack.pop();

    stack.push();
    stack.scale(1000.0F);
    stack.translate(0.0F, -0.1F, 0.0F);
    drawMesh(MESH_SKYBOX_GROUND, stack.top(), TEXTURE_SKYBOX_GROUND);
    drawMesh(MESH_SKYBOX_BG, stack.top(), TEXTURE_SKYBOX_BG);
    stack.pop();

    stack.push();
    stack.scale(1000.0F);
    drawMesh(MESH_GROUND, stack.top(), TEXTURE_FAIRWAY, 1000.0F);
    stack.pop();

    f32 alpha = accumulator / deltaTime;

    BallManager *ballManagerCurrentIteration = &world->ballManager;
    BallManager *ballManagerPreviousIteration = &previous->ballManager;

    for (size_t ballIndex = 0; ballIndex < world->ballManager.activeBalls; ballIndex++)
    {
        Ball *ballCurrentIteration = ballManagerCurrentIteration->getBall(ballIndex);
        Ball *ballPreviousIteration = ballManagerPreviousIteration->getBall(ballIndex);

        glm::vec3 &previousPosition = ballPreviousIteration->position;
        glm::vec3 &currentPosition = ballCurrentIteration->position;
        glm::vec3 interpolatedPosition = glm::mix(previousPosition, currentPosition, alpha);

        stack.push();
        stack.translate(interpolatedPosition);
        drawMesh(MESH_GOLF_BALL, stack.top(), TEXTURE_GOLF_BALL);
        stack.pop();

        if (glm::length2(ballCurrentIteration->velocity) <= FLT_EPSILON || !showForceVectors)
        {
            continue;
        }

        glDisable(GL_DEPTH_TEST);

        // Gravity
        drawVector(ballCurrentIteration->gravityForce * INV_BALL_MASS, interpolatedPosition, CYAN);

        // Lift
        drawVector(ballCurrentIteration->liftForce * INV_BALL_MASS, interpolatedPosition, YELLOW);

        // Drag
        drawVector(ballCurrentIteration->dragForce * INV_BALL_MASS, interpolatedPosition, MAGENTA);

        // Wind
        drawVector(ballCurrentIteration->windVector, interpolatedPosition, GREEN);

        // Velocity
        drawVector(ballCurrentIteration->velocity, interpolatedPosition, BLUE);

        // Acceleration
        drawVector(ballCurrentIteration->acceleration, interpolatedPosition, RED);

        // Rotation axis
        drawVector(ballCurrentIteration->rotationAxis, interpolatedPosition, WHITE);

        glEnable(GL_DEPTH_TEST);
    }
}

void GolfFlightSim3D::renderUI(f32 frameTime)
{
    view = glm::mat4(1.0F);

    f32 top = 1.0F;
    f32 bottom = -1.0F;
    f32 left = -1.0F;
    f32 right = 1.0F;
    f32 znear = -1.0F;
    f32 zfar = 1.0F;

    projection = glm::ortho(left, right, bottom, top, znear, zfar);

    const f32 arrowPosX = 1.0F - 0.2F;
    const f32 arrowPosY = 1.0F - (4.0F / 15.0F);
    const f32 arrowScale = 0.08F;
    drawWindArrow(arrowPosX, arrowPosY, arrowScale);

    const ImGuiWindowFlags standardWindowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
    const f32 bgAlpha = 0.15F;

    f32 launchParamWindowWidth = 0.0F;
    ImGui::SetNextWindowBgAlpha(bgAlpha);
    if (ImGui::Begin("Configure Launch Parameters", NULL, standardWindowFlags))
    {
        ImGui::SetWindowPos(ImVec2(0, 0));

        static f32 launchSpeedMph = 167.0F;
        static f32 launchAngleDegrees = 10.9F;
        static f32 launchHeadingDegrees = 0.0F;
        static f32 launchSpinRate = 2600.0F;
        static f32 spinAngleDegrees = 0.0F;

        ImGui::Text("Launch Conditions");
        ImGui::Spacing();

        ImGui::SliderFloat("Launch Speed (mph)", &launchSpeedMph, 10.0F, 300.0F);
        ImGui::SliderFloat("Launch Angle (deg)", &launchAngleDegrees, 0.0F, 40.0F);
        ImGui::SliderFloat("Launch Heading (deg)", &launchHeadingDegrees, -45.0F, 45.0F);
        ImGui::SliderFloat("Spin Rate (rpm)", &launchSpinRate, 0.0F, 20000.0F);
        ImGui::SliderFloat("Spin Axis (deg)", &spinAngleDegrees, -90.0F, 90.0F);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Wind Settings");
        ImGui::Spacing();

        // Adjust the wind vector in real time based on these fields
        static f32 windSpeedMph;
        ImGui::SliderFloat("Wind Speed (mph)", &windSpeedMph, 0.0F, 30.0F);
        ImGui::SliderAngle("Wind Heading (deg)", &world->wind.direction, 0.0F, 360.0F);
        ImGui::Checkbox("Use logarithmic wind model", &world->wind.logWind);
        world->wind.speed = mphToMs(windSpeedMph);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Misc.");
        ImGui::Spacing();

        ImGui::Checkbox("Show forces", &showForceVectors);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Launch Ball"))
        {
            f32 launchSpeedMs = mphToMs(launchSpeedMph);
            f32 launchAngleRadians = glm::radians(launchAngleDegrees);
            f32 launchHeadingRadians = glm::radians(launchHeadingDegrees);

            f32 spinAngleRadians = glm::radians(spinAngleDegrees);
            world->ballManager.spawnBall(launchSpeedMs, launchAngleRadians, launchHeadingRadians, launchSpinRate,
                                         spinAngleRadians);
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear Balls"))
        {
            bzero(&world->ballManager, sizeof(BallManager));
        }

        launchParamWindowWidth = ImGui::GetWindowWidth();
    }
    ImGui::End();

    f32 ballInfoWindowHeight = 206.0F;
    ImGui::SetNextWindowBgAlpha(bgAlpha);
    if (ImGui::Begin("Ball Info", NULL,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::SetWindowPos(ImVec2(launchParamWindowWidth, 0));
        ImGui::SetWindowSize(ImVec2(380.0F, ballInfoWindowHeight));

        if (ImGui::BeginTable("ballInfo", 1))
        {
            BallManager *ballManager = &world->ballManager;

            for (size_t ballIndex = 0; ballIndex < ballManager->activeBalls; ballIndex++)
            {
                Ball *ball = ballManager->getBall(ballIndex);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);

                if (ImGui::TreeNodeEx((void *)ballIndex, ImGuiTreeNodeFlags_DefaultOpen, "Ball %zu", ballIndex + 1))
                {
                    ImGui::Text("Position (yds): (%.2f, %.2f, %.2f)", metersToYards(ball->position.x),
                                metersToYards(ball->position.y), metersToYards(ball->position.z));

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Height (m): %.2f", ball->height);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Max height (m): %.2f", ball->maxHeight);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Velocity (ft/s): (%.2f, %.2f, %.2f)", msToFtS(ball->velocity.x),
                                msToFtS(ball->velocity.y), msToFtS(ball->velocity.z));

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Acceleration (ft/s^2): (%.2f, %.2f, %.2f)", msToFtS(ball->acceleration.x),
                                msToFtS(ball->acceleration.y), msToFtS(ball->acceleration.z));

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Spin Rate (rpm): (%.2f)", ball->spinRate);

                    ImGui::TreePop();
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();

    if (showForceVectors)
    {
        // Show the vectors key
        ImGui::SetNextWindowBgAlpha(bgAlpha);
        if (ImGui::Begin("Vectors", NULL, standardWindowFlags))
        {
            const f32 margin = 2.0F;

            ImGui::SetWindowPos(ImVec2(launchParamWindowWidth + margin, ballInfoWindowHeight + margin));

            ImGui::Text("Vectors:");
            ImGui::Spacing();

            ImGui::TextColored(ImVec4(0.0F, 0.0F, 1.0F, 1.0F), "  Velocity");
            ImGui::TextColored(ImVec4(1.0F, 0.0F, 0.0F, 1.0F), "  Acceleration");
            ImGui::TextColored(ImVec4(0.0F, 1.0F, 1.0F, 1.0F), "  Gravity");
            ImGui::TextColored(ImVec4(0.0F, 1.0F, 0.0F, 1.0F), "  Wind");
            ImGui::TextColored(ImVec4(1.0F, 1.0F, 0.0F, 1.0F), "  Lift");
            ImGui::TextColored(ImVec4(1.0F, 0.0F, 1.0F, 1.0F), "  Drag");
            ImGui::TextColored(ImVec4(1.0F, 1.0F, 1.0F, 1.0F), "  Rotation Axis");
        }
        ImGui::End();
    }

    ImGui::SetNextWindowBgAlpha(bgAlpha);
    if (ImGui::Begin("Counters", NULL, standardWindowFlags))
    {
        ImGui::Text("Balls Spawned: %d", (s32)world->ballManager.activeBalls);

        ImGui::Spacing();

        ImGui::Text("FPS: %.2f", 1.0F / frameTime);

        s32 width, height;
        glfwGetWindowSize(_windowHandle, &width, &height);

        ImGui::SetWindowPos(ImVec2(0, (f32)height - ImGui::GetWindowHeight()));
    }
    ImGui::End();

    ImGui::SetNextWindowBgAlpha(bgAlpha);
    if (ImGui::Begin("Info Hint", NULL, standardWindowFlags))
    {
        ImGui::Text("Cameras: 1-3");
        ImGui::Text("Quit: ESC");

        s32 width, height;
        glfwGetWindowSize(_windowHandle, &width, &height);

        ImGui::SetWindowPos(ImVec2((f32)width - ImGui::GetWindowWidth(), (f32)height - ImGui::GetWindowHeight()));
    }
    ImGui::End();

    // ImGui::ShowDemoWindow();

    elapsedTime += frameTime;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    GolfFlightSim3D golfFlightSim3D;
    golfFlightSim3D.run();

    return 0;
}