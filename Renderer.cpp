#include "Renderer.hpp"
#include <numbers>
#include <iostream>
#include "imgui.h" // Renk dönüşümü için

// Shader kaynak kodları
const char* color_vertex_shader = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aColor;
    out vec3 FragColor;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        FragColor = aColor;
    }
)";

const char* color_fragment_shader = R"(
    #version 330 core
    in vec3 FragColor;
    out vec4 OutColor;
    void main() {
        OutColor = vec4(FragColor, 1.0);
    }
)";

// Shader derleme ve program oluşturma yardımcı fonksiyonları
GLuint compile_shader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader derleme hatasi:\n" << infoLog << std::endl;
    }
    return shader;
}

GLuint create_shader_program(const char* vs_src, const char* fs_src) {
    GLuint vs = compile_shader(vs_src, GL_VERTEX_SHADER);
    GLuint fs = compile_shader(fs_src, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Shader link hatasi:\n" << infoLog << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}


// --- RenderObject Metotları ---
void RenderObject::cleanup() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
    vao = vbo = ebo = 0;
}


// --- Renderer Metotları ---
Renderer::Renderer() : color_shader(0) {}

Renderer::~Renderer() {
    grid.cleanup();
    axes.cleanup();
    radar_obj.cleanup();
    target_obj.cleanup();
    selection_sphere.cleanup();
    error_vector_obj.cleanup();
    for (auto& obj : dataset_render_objects) {
        obj.cleanup();
    }
    if (color_shader) glDeleteProgram(color_shader);
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (fbo_texture) glDeleteTextures(1, &fbo_texture);
    if (rbo_depth) glDeleteRenderbuffers(1, &rbo_depth);
}

void Renderer::initialize(const std::vector<SearchableDataset>& datasets, float grid_size) {
    setup_shaders();
    create_scene_objects(datasets, grid_size);
    create_fbo(1920, 1080);
}

void Renderer::setup_shaders() {
    color_shader = create_shader_program(color_vertex_shader, color_fragment_shader);
}

void Renderer::setup_render_object(RenderObject& object, const std::vector<float>& vertices, GLenum draw_mode, GLenum usage) {
    if (object.vao == 0) glGenVertexArrays(1, &object.vao);
    if (object.vbo == 0) glGenBuffers(1, &object.vbo);

    glBindVertexArray(object.vao);
    glBindBuffer(GL_ARRAY_BUFFER, object.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), usage);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    object.vertex_count = static_cast<GLsizei>(vertices.size() / 6);
    object.draw_mode = draw_mode;

    glBindVertexArray(0);
}

void Renderer::setup_indexed_render_object(RenderObject& object, const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    glGenVertexArrays(1, &object.vao);
    glGenBuffers(1, &object.vbo);
    glGenBuffers(1, &object.ebo);
    glBindVertexArray(object.vao);
    glBindBuffer(GL_ARRAY_BUFFER, object.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    object.vertex_count = static_cast<GLsizei>(indices.size());
    object.draw_mode = GL_TRIANGLES;
    glBindVertexArray(0);
}


void Renderer::create_scene_objects(const std::vector<SearchableDataset>& datasets, float grid_size) {
    // Grid, Eksenler... (önceki gibi)
    std::vector<float> grid_vertices;
    float half_size = grid_size / 2.0f;
    int lines = 20;
    for (int i = 0; i <= lines; ++i) {
        float pos = -half_size + (grid_size * i / lines);
        grid_vertices.insert(grid_vertices.end(), { pos, 0.0f, -half_size, 0.3f, 0.3f, 0.3f });
        grid_vertices.insert(grid_vertices.end(), { pos, 0.0f, half_size,  0.3f, 0.3f, 0.3f });
        grid_vertices.insert(grid_vertices.end(), { -half_size, 0.0f, pos, 0.3f, 0.3f, 0.3f });
        grid_vertices.insert(grid_vertices.end(), { half_size,  0.0f, pos, 0.3f, 0.3f, 0.3f });
    }
    setup_render_object(grid, grid_vertices, GL_LINES);
    float axis_length = grid_size * 0.55f;
    std::vector<float> axes_vertices = {
        0.0f, 0.0f, 0.0f,   1.0f, 0.2f, 0.2f, axis_length, 0.0f, 0.0f, 1.0f, 0.2f, 0.2f,
        0.0f, 0.0f, 0.0f,   0.2f, 0.2f, 1.0f, 0.0f, axis_length, 0.0f, 0.2f, 0.2f, 1.0f,
        0.0f, 0.0f, 0.0f,   0.2f, 1.0f, 0.2f, 0.0f, 0.0f, -axis_length, 0.2f, 1.0f, 0.2f,
    };
    setup_render_object(axes, axes_vertices, GL_LINES);

    // SİPER ALP RADAR MODELİ
    std::vector<float> radar_vertices;
    float base_rad = 50.f, base_h = 15.f;
    int segs = 24;
    for (int i = 0; i < segs; ++i) {
        float a1 = 2 * (float)std::numbers::pi * i / segs;
        float a2 = 2 * (float)std::numbers::pi * (i + 1) / segs;
        // Taban
        radar_vertices.insert(radar_vertices.end(), { 0,0,0, 0.4f,0.45f,0.5f });
        radar_vertices.insert(radar_vertices.end(), { base_rad * cosf(a1), 0, -base_rad * sinf(a1), 0.4f,0.45f,0.5f });
        radar_vertices.insert(radar_vertices.end(), { base_rad * cosf(a2), 0, -base_rad * sinf(a2), 0.4f,0.45f,0.5f });
        // Yan yüzeyler
        radar_vertices.insert(radar_vertices.end(), { base_rad * cosf(a1), 0, -base_rad * sinf(a1), 0.5f,0.55f,0.6f });
        radar_vertices.insert(radar_vertices.end(), { base_rad * cosf(a2), base_h, -base_rad * sinf(a2), 0.5f,0.55f,0.6f });
        radar_vertices.insert(radar_vertices.end(), { base_rad * cosf(a1), base_h, -base_rad * sinf(a1), 0.5f,0.55f,0.6f });
        radar_vertices.insert(radar_vertices.end(), { base_rad * cosf(a1), 0, -base_rad * sinf(a1), 0.5f,0.55f,0.6f });
        radar_vertices.insert(radar_vertices.end(), { base_rad * cosf(a2), 0, -base_rad * sinf(a2), 0.5f,0.55f,0.6f });
        radar_vertices.insert(radar_vertices.end(), { base_rad * cosf(a2), base_h, -base_rad * sinf(a2), 0.5f,0.55f,0.6f });
    }
    // Radar Paneli (Sekizgen)
    float panel_rad = 60.f, panel_h = 70.f, panel_thick = 8.f;
    for (int i = 0; i < 8; ++i) {
        float a1 = 2 * (float)std::numbers::pi * i / 8.0f;
        float a2 = 2 * (float)std::numbers::pi * (i + 1) / 8.0f;
        // Ön yüz
        radar_vertices.insert(radar_vertices.end(), { 0, base_h + panel_h / 2, 0, 0.2f,0.2f,0.25f });
        radar_vertices.insert(radar_vertices.end(), { panel_rad * cosf(a1), base_h + panel_h * sinf(a1), 0, 0.2f,0.2f,0.25f });
        radar_vertices.insert(radar_vertices.end(), { panel_rad * cosf(a2), base_h + panel_h * sinf(a2), 0, 0.2f,0.2f,0.25f });
        // Yan yüzeyler
        radar_vertices.insert(radar_vertices.end(), { panel_rad * cosf(a1), base_h + panel_h * sinf(a1), 0, 0.3f,0.3f,0.35f });
        radar_vertices.insert(radar_vertices.end(), { panel_rad * cosf(a2), base_h + panel_h * sinf(a2), -panel_thick, 0.3f,0.3f,0.35f });
        radar_vertices.insert(radar_vertices.end(), { panel_rad * cosf(a1), base_h + panel_h * sinf(a1), -panel_thick, 0.3f,0.3f,0.35f });
        radar_vertices.insert(radar_vertices.end(), { panel_rad * cosf(a1), base_h + panel_h * sinf(a1), 0, 0.3f,0.3f,0.35f });
        radar_vertices.insert(radar_vertices.end(), { panel_rad * cosf(a2), base_h + panel_h * sinf(a2), 0, 0.3f,0.3f,0.35f });
        radar_vertices.insert(radar_vertices.end(), { panel_rad * cosf(a2), base_h + panel_h * sinf(a2), -panel_thick, 0.3f,0.3f,0.35f });
    }
    setup_render_object(radar_obj, radar_vertices, GL_TRIANGLES);


    // HEDEF MODELİ (F-16 Benzeri)
    std::vector<float> target_vertices = {
        // Pozisyonlar (x, y, z), Renkler (r, g, b)
        // Gövde
        -30.f, 0.f, 0.f,  0.8f, 0.85f, 0.9f,  // 0 Burun
        35.f, 0.f, 0.f,   0.6f, 0.65f, 0.7f,  // 1 Egzoz
        -10.f, 5.f, 0.f,  0.9f, 0.95f, 1.0f,  // 2 Kanopi Üst
        5.f, -4.f, 0.f,  0.5f, 0.55f, 0.6f,  // 3 Gövde Alt
        // Ana Kanatlar
        15.f, 0.f, -45.f, 0.7f, 0.75f, 0.8f,  // 4 Sol Kanat Ucu
        15.f, 0.f, 45.f,  0.7f, 0.75f, 0.8f,  // 5 Sağ Kanat Ucu
        -5.f, 0.f, -10.f, 0.7f, 0.75f, 0.8f,  // 6 Sol Kanat Kökü
        -5.f, 0.f, 10.f,  0.7f, 0.75f, 0.8f,  // 7 Sağ Kanat Kökü
        // Dikey Stabilizatör
        30.f, 25.f, 0.f,  0.7f, 0.75f, 0.8f,  // 8 Kuyruk Üst
        40.f, 0.f, 0.f,   0.6f, 0.65f, 0.7f,  // 9 Kuyruk Alt
        // Yatay Stabilizatörler
        38.f, 0.f, -18.f, 0.7f, 0.75f, 0.8f, // 10 Sol
        38.f, 0.f, 18.f,  0.7f, 0.75f, 0.8f,  // 11 Sağ
    };
    std::vector<unsigned int> target_indices = {
        // Gövde Üst
        0, 2, 7,  2, 1, 7,
        0, 2, 6,  2, 1, 6,
        // Gövde Alt
        0, 3, 7,  3, 1, 7,
        0, 3, 6,  3, 1, 6,
        // Kanatlar
        6, 7, 4,  7, 5, 4,
        // Dikey Stabilizatör
        1, 8, 9,
        // Yatay Stabilizatörler
        1, 11, 10
    };
    setup_indexed_render_object(target_obj, target_vertices, target_indices);

    // Seçim Küresi
    std::vector<float> sphere_vertices;
    std::vector<unsigned int> sphere_indices;
    float radius = 1.0f; int sectors = 12, stacks = 12;
    for (int i = 0; i <= stacks; ++i) {
        float stack_angle = (float)std::numbers::pi / 2 - i * (float)std::numbers::pi / stacks;
        float xy = radius * cosf(stack_angle);
        float z_coord = radius * sinf(stack_angle);
        for (int j = 0; j <= sectors; ++j) {
            float sector_angle = j * 2 * (float)std::numbers::pi / sectors;
            float x_coord = xy * cosf(sector_angle);
            float y_coord = xy * sinf(sector_angle);
            sphere_vertices.insert(sphere_vertices.end(), { x_coord, y_coord, z_coord, 1.0f, 1.0f, 0.0f });
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int first = (i * (sectors + 1)) + j;
            int second = first + sectors + 1;
            sphere_indices.insert(sphere_indices.end(), { (unsigned int)first, (unsigned int)second, (unsigned int)(first + 1) });
            sphere_indices.insert(sphere_indices.end(), { (unsigned int)second, (unsigned int)(second + 1), (unsigned int)(first + 1) });
        }
    }
    setup_indexed_render_object(selection_sphere, sphere_vertices, sphere_indices);

    // Hata Vektörü
    setup_render_object(error_vector_obj, {}, GL_LINES, GL_DYNAMIC_DRAW);

    // Veri Seti (Yörünge Çizgileri)
    dataset_render_objects.resize(datasets.size());
    for (size_t i = 0; i < datasets.size(); ++i) {
        std::vector<float> vertices;
        for (const auto& p : datasets[i].points) {
            vertices.push_back((float)p.x);
            vertices.push_back((float)p.z);
            vertices.push_back((float)-p.y);
            ImU32 color = (datasets[i].name.find("Gercek") != std::string::npos) ? IM_COL32(0, 255, 0, 100) : IM_COL32(255, 0, 0, 100);
            vertices.push_back(((color >> 0) & 0xFF) / 255.f);
            vertices.push_back(((color >> 8) & 0xFF) / 255.f);
            vertices.push_back(((color >> 16) & 0xFF) / 255.f);
        }
        if (vertices.empty()) continue;
        setup_render_object(dataset_render_objects[i], vertices, datasets[i].is_line_series ? GL_LINE_STRIP : GL_POINTS);
    }
}

void Renderer::draw(const Math::Mat4& view, const Math::Mat4& projection, const std::vector<bool>& visibility,
    const Math::Mat4& radar_model, const Math::Mat4& target_model, const Math::Mat4& selection_sphere_model) {
    glUseProgram(color_shader);
    glUniformMatrix4fv(glGetUniformLocation(color_shader, "projection"), 1, GL_FALSE, &projection.m[0]);
    glUniformMatrix4fv(glGetUniformLocation(color_shader, "view"), 1, GL_FALSE, &view.m[0]);
    Math::Mat4 identity_model = Math::identity();

    // Grid, Eksenler...
    glUniformMatrix4fv(glGetUniformLocation(color_shader, "model"), 1, GL_FALSE, &identity_model.m[0]);
    glLineWidth(1.0f);
    glBindVertexArray(grid.vao);
    glDrawArrays(grid.draw_mode, 0, grid.vertex_count);
    glLineWidth(2.5f);
    glBindVertexArray(axes.vao);
    glDrawArrays(axes.draw_mode, 0, axes.vertex_count);

    // Yörünge çizgileri ve noktaları
    for (size_t i = 0; i < dataset_render_objects.size(); ++i) {
        if (!visibility[i] || dataset_render_objects[i].vertex_count == 0) continue;
        glBindVertexArray(dataset_render_objects[i].vao);
        if (dataset_render_objects[i].draw_mode == GL_POINTS) {
            glPointSize(5.0f);
        }
        else {
            glLineWidth(2.0f);
        }
        glDrawArrays(dataset_render_objects[i].draw_mode, 0, dataset_render_objects[i].vertex_count);
    }

    // Radar Modeli
    glUniformMatrix4fv(glGetUniformLocation(color_shader, "model"), 1, GL_FALSE, &radar_model.m[0]);
    glBindVertexArray(radar_obj.vao);
    glDrawArrays(radar_obj.draw_mode, 0, radar_obj.vertex_count);

    // Hedef Modeli
    glUniformMatrix4fv(glGetUniformLocation(color_shader, "model"), 1, GL_FALSE, &target_model.m[0]);
    glBindVertexArray(target_obj.vao);
    glDrawElements(target_obj.draw_mode, target_obj.vertex_count, GL_UNSIGNED_INT, 0);

    // Seçim Küresi
    if (selection_sphere_model.m[0] != 0 || selection_sphere_model.m[5] != 0) { // Geçerli bir model mi diye kontrol
        glUniformMatrix4fv(glGetUniformLocation(color_shader, "model"), 1, GL_FALSE, &selection_sphere_model.m[0]);
        glBindVertexArray(selection_sphere.vao);
        glDrawElements(selection_sphere.draw_mode, selection_sphere.vertex_count, GL_UNSIGNED_INT, 0);
    }


    // Hata Vektörleri
    if (error_vector_obj.vertex_count > 0) {
        glLineWidth(1.5f);
        glUniformMatrix4fv(glGetUniformLocation(color_shader, "model"), 1, GL_FALSE, &identity_model.m[0]);
        glBindVertexArray(error_vector_obj.vao);
        glDrawArrays(error_vector_obj.draw_mode, 0, error_vector_obj.vertex_count);
    }
}


void Renderer::update_error_vectors(const std::vector<std::pair<DataPoint, DataPoint>>& vectors) {
    if (vectors.empty()) {
        error_vector_obj.vertex_count = 0;
        return;
    }
    std::vector<float> vertices;
    for (const auto& vec : vectors) {
        const auto& p1 = vec.first;
        const auto& p2 = vec.second;
        vertices.insert(vertices.end(), { (float)p1.x, (float)p1.z, (float)-p1.y, 1.0f, 1.0f, 1.0f });
        vertices.insert(vertices.end(), { (float)p2.x, (float)p2.z, (float)-p2.y, 1.0f, 1.0f, 1.0f });
    }
    glBindVertexArray(error_vector_obj.vao);
    glBindBuffer(GL_ARRAY_BUFFER, error_vector_obj.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    error_vector_obj.vertex_count = static_cast<GLsizei>(vertices.size() / 6);
    glBindVertexArray(0);
}


void Renderer::create_fbo(int width, int height) {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &fbo_texture);
    glBindTexture(GL_TEXTURE_2D, fbo_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);
    glGenRenderbuffers(1, &rbo_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Framebuffer tamamlanamadi!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::bind_fbo() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Renderer::unbind_fbo() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

