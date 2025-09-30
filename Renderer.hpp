#pragma once

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <vector>
#include <utility>
#include "DataStructures.hpp"
#include "Math.hpp"

// Çizilecek bir OpenGL nesnesini temsil eder.
struct RenderObject {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei vertex_count = 0;
    GLenum draw_mode = GL_TRIANGLES;

    void cleanup();
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    void initialize(const std::vector<SearchableDataset>& datasets, float grid_size);
    void draw(const Math::Mat4& view, const Math::Mat4& projection, const std::vector<bool>& visibility,
        const Math::Mat4& radar_model, const Math::Mat4& target_model, const Math::Mat4& selection_sphere_model);
    void update_error_vectors(const std::vector<std::pair<DataPoint, DataPoint>>& vectors);

    // Framebuffer Object (FBO) yönetimi
    void bind_fbo();
    void unbind_fbo();
    GLuint get_fbo_texture() const { return fbo_texture; }

private:
    GLuint color_shader;

    // Sahne Nesneleri
    RenderObject grid, axes, radar_obj, target_obj, selection_sphere;
    std::vector<RenderObject> dataset_render_objects;
    RenderObject error_vector_obj;

    // FBO
    GLuint fbo = 0, fbo_texture = 0, rbo_depth = 0;

    void setup_shaders();
    void create_scene_objects(const std::vector<SearchableDataset>& datasets, float grid_size);
    void setup_render_object(RenderObject& object, const std::vector<float>& vertices, GLenum draw_mode, GLenum usage = GL_STATIC_DRAW);
    void setup_indexed_render_object(RenderObject& object, const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    void create_fbo(int width, int height);
};

