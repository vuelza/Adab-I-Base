#include "InteractivePlotter.hpp"
#include "Renderer.hpp"
#include "Math.hpp"
#include <iostream>
#include <numbers>
#include <limits>
#include <string>
#include <algorithm>
#include <cmath>

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// GLFW hata geri çağırma fonksiyonu
static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

// Veri setlerine göre grid boyutunu hesaplayan yardımcı fonksiyon
std::pair<float, int> calculate_grid_properties(const std::vector<SearchableDataset>& datasets) {
    float max_abs_coord = 1000.0f;
    bool data_found = false;
    for (const auto& ds : datasets) {
        if (!ds.points.empty()) {
            data_found = true;
            double current_max = 0.0;
            for (const auto& p : ds.points) {
                if (std::abs(p.x) > current_max) current_max = std::abs(p.x);
                if (std::abs(p.y) > current_max) current_max = std::abs(p.y);
                if (std::abs(p.z) > current_max) current_max = std::abs(p.z);
            }
            if (current_max > max_abs_coord) {
                max_abs_coord = static_cast<float>(current_max);
            }
        }
    }
    if (!data_found) return { 2000.0f, 20 };
    float grid_size = std::pow(10, std::ceil(std::log10(max_abs_coord + 1.0f)));
    if (grid_size < 100) grid_size = 100;
    return { grid_size * 2.0f, 20 };
}


// Ana uygulama sınıfı
class PlotterApp {
public:
    PlotterApp(const std::vector<SearchableDataset>& datasets, const DataPoint& radar_pos_relative);
    ~PlotterApp();
    void run();

private:
    // Pencere ve Durum
    GLFWwindow* window = nullptr;
    const std::vector<SearchableDataset>& datasets;
    Renderer renderer;
    Math::Mat4 projection_matrix;
    Math::Mat4 view_matrix;

    // Kamera
    float cam_yaw = -2.3f, cam_pitch = 0.8f, cam_distance;
    Math::Vec3 cam_center = { 0, 0, 0 };
    float grid_size;

    // Sahne Nesneleri
    Math::Mat4 radar_model_matrix;
    Math::Mat4 target_model_matrix;
    DataPoint radar_pos_relative;

    // Animasyon
    float current_time = 0.0f;
    float max_time = 0.0f;
    bool is_playing = false;
    float playback_speed = 1.0f;

    // UI Durumu
    std::vector<bool> visibility;
    size_t selected_dataset_idx = std::numeric_limits<size_t>::max();
    size_t selected_point_idx = std::numeric_limits<size_t>::max();
    struct PinnedPoint {
        size_t uid;
        size_t dataset_idx;
        size_t point_idx;
        bool show_ecef = true;
        bool show_error_vector = false;
    };
    std::vector<PinnedPoint> pinned_points;
    size_t next_pin_uid = 0;

    // Arama
    int search_dataset_idx_int = 0;
    char searchQuery[128] = "";
    bool show_search_warning = false;


    // Ana Fonksiyonlar
    void init_window();
    void init_imgui();
    void main_loop();
    void shutdown();

    // Yardımcı Fonksiyonlar
    void update(float dt);
    size_t find_truth_dataset_idx();
    size_t find_closest_time_index(const SearchableDataset& ds, double time);
    void handle_input(const ImVec2& viewport_pos, const ImVec2& viewport_size);
    void render_ui();
    void render_scene();
    void render_pinned_point_windows(const ImVec2& viewport_pos, const ImVec2& viewport_size);
};

PlotterApp::PlotterApp(const std::vector<SearchableDataset>& datasets, const DataPoint& radar_pos_relative)
    : datasets(datasets), radar_pos_relative(radar_pos_relative)
{
    auto [size, lines] = calculate_grid_properties(datasets);
    grid_size = size;
    cam_distance = grid_size * 0.5f;
    visibility.resize(datasets.size(), true);

    // Max animasyon zamanını bul
    for (const auto& ds : datasets) {
        if (!ds.time_data.empty() && ds.time_data.back() > max_time) {
            max_time = (float)ds.time_data.back();
        }
    }

    init_window();
    init_imgui();
    renderer.initialize(datasets, grid_size);

    // Radar model matrisini ayarla
    Math::Vec3 radar_gl_pos = { (float)radar_pos_relative.x, (float)radar_pos_relative.z, (float)-radar_pos_relative.y };
    radar_model_matrix = Math::translate(Math::identity(), radar_gl_pos);
}

PlotterApp::~PlotterApp() {
    shutdown();
}

void PlotterApp::init_window() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) throw std::runtime_error("GLFW baslatilamadi");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    window = glfwCreateWindow(1920, 1080, "Adab-I-Sim Hedef Simulatoru", NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("GLFW penceresi olusturulamadi");
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("GLAD baslatilamadi");
    }
}

void PlotterApp::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void PlotterApp::run() {
    main_loop();
}

void PlotterApp::main_loop() {
    float last_time = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float current_frame_time = (float)glfwGetTime();
        float dt = current_frame_time - last_time;
        last_time = current_frame_time;

        glfwPollEvents();
        update(dt);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        render_ui();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}

void PlotterApp::update(float dt) {
    if (is_playing) {
        current_time += dt * playback_speed;
        if (current_time > max_time) current_time = 0; // Başa sar
    }

    size_t truth_idx = find_truth_dataset_idx();
    if (truth_idx != std::numeric_limits<size_t>::max()) {
        const auto& truth_ds = datasets[truth_idx];
        if (truth_ds.points.size() > 1) {
            auto it = std::lower_bound(truth_ds.time_data.begin(), truth_ds.time_data.end(), current_time);
            size_t idx1 = std::distance(truth_ds.time_data.begin(), it);
            if (idx1 == 0) idx1 = 1;
            if (idx1 >= truth_ds.points.size()) idx1 = truth_ds.points.size() - 1;
            size_t idx0 = idx1 - 1;

            const auto& p0 = truth_ds.points[idx0];
            const auto& p1 = truth_ds.points[idx1];
            double t0 = truth_ds.time_data[idx0];
            double t1 = truth_ds.time_data[idx1];
            float factor = (t1 - t0 > 1e-5) ? (float)((current_time - t0) / (t1 - t0)) : 0.0f;

            Math::Vec3 pos = {
                (float)(p0.x + (p1.x - p0.x) * factor),
                (float)(p0.z + (p1.z - p0.z) * factor),
                (float)-(p0.y + (p1.y - p0.y) * factor)
            };

            Math::Vec3 world_fwd_vec = { (float)(p1.x - p0.x), (float)(p1.z - p0.z), (float)-(p1.y - p0.y) };
            float len = sqrtf(world_fwd_vec.x * world_fwd_vec.x + world_fwd_vec.y * world_fwd_vec.y + world_fwd_vec.z * world_fwd_vec.z);
            if (len > 1e-5) {
                world_fwd_vec.x /= len; world_fwd_vec.y /= len; world_fwd_vec.z /= len;
            }
            else {
                if (idx0 > 0) {
                    const auto& p_prev = truth_ds.points[idx0 - 1];
                    world_fwd_vec = { (float)(p0.x - p_prev.x), (float)(p0.z - p_prev.z), (float)-(p0.y - p_prev.y) };
                    len = sqrtf(world_fwd_vec.x * world_fwd_vec.x + world_fwd_vec.y * world_fwd_vec.y + world_fwd_vec.z * world_fwd_vec.z);
                    if (len > 1e-5) {
                        world_fwd_vec.x /= len; world_fwd_vec.y /= len; world_fwd_vec.z /= len;
                    }
                    else { world_fwd_vec = { 1,0,0 }; }
                }
                else { world_fwd_vec = { 1,0,0 }; }
            }

            Math::Vec3 global_up = { 0,1,0 };
            Math::Vec3 world_right_vec = { global_up.y * world_fwd_vec.z - global_up.z * world_fwd_vec.y, global_up.z * world_fwd_vec.x - global_up.x * world_fwd_vec.z, global_up.x * world_fwd_vec.y - global_up.y * world_fwd_vec.x };
            len = sqrtf(world_right_vec.x * world_right_vec.x + world_right_vec.y * world_right_vec.y + world_right_vec.z * world_right_vec.z);
            if (len > 1e-5) {
                world_right_vec.x /= len; world_right_vec.y /= len; world_right_vec.z /= len;
            }
            else { world_right_vec = { 0,0,1 }; }

            Math::Vec3 world_up_vec = { world_fwd_vec.y * world_right_vec.z - world_fwd_vec.z * world_right_vec.y, world_fwd_vec.z * world_right_vec.x - world_fwd_vec.x * world_right_vec.z, world_fwd_vec.x * world_right_vec.y - world_fwd_vec.y * world_right_vec.x };

            // Modelin lokal -X ekseni (burnu) -> dünyanın ileri vektörüne
            // Modelin lokal +Y ekseni (üstü) -> dünyanın yukarı vektörüne
            // Modelin lokal +Z ekseni (sağ kanat) -> dünyanın sağ vektörüne
            Math::Mat4 rotation_matrix = Math::identity();
            // Sütun 0 (Lokal X'in dünya uzayındaki karşılığı)
            rotation_matrix.m[0] = -world_fwd_vec.x;
            rotation_matrix.m[1] = -world_fwd_vec.y;
            rotation_matrix.m[2] = -world_fwd_vec.z;
            // Sütun 1 (Lokal Y'nin dünya uzayındaki karşılığı)
            rotation_matrix.m[4] = world_up_vec.x;
            rotation_matrix.m[5] = world_up_vec.y;
            rotation_matrix.m[6] = world_up_vec.z;
            // Sütun 2 (Lokal Z'nin dünya uzayındaki karşılığı)
            rotation_matrix.m[8] = world_right_vec.x;
            rotation_matrix.m[9] = world_right_vec.y;
            rotation_matrix.m[10] = world_right_vec.z;

            // Önce döndür, sonra pozisyona taşı
            Math::Mat4 translation_matrix = Math::translate(Math::identity(), pos);
            target_model_matrix = Math::multiply(translation_matrix, rotation_matrix);
        }
    }
}


void PlotterApp::render_ui() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Ana Pencere", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Üst Kontrol Paneli
    ImGui::BeginChild("KontrolPaneli", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6.0f), true);
    ImGui::Columns(2, "KontrolSutunlari", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.25f);
    for (size_t i = 0; i < datasets.size(); ++i) {
        std::string label = visibility[i] ? "Gizle: " : "Goster: ";
        label += datasets[i].name;
        if (ImGui::Button(label.c_str(), ImVec2(-1, 0))) {
            visibility[i] = !visibility[i];
        }
    }
    ImGui::NextColumn();
    // Arama Arayüzü
    if (!datasets.empty()) {
        const auto& selected_dataset_for_search = datasets[search_dataset_idx_int];
        if (ImGui::BeginCombo("Veri Seti", selected_dataset_for_search.name.c_str())) {
            for (int n = 0; n < (int)datasets.size(); n++) {
                if (ImGui::Selectable(datasets[n].name.c_str(), search_dataset_idx_int == n)) {
                    search_dataset_idx_int = n;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::InputText("Zaman (s)", searchQuery, IM_ARRAYSIZE(searchQuery));
        ImGui::SameLine();
        if (ImGui::Button("Bul")) {
            try {
                double searchValue = std::stod(searchQuery);
                size_t found_idx = find_closest_time_index(selected_dataset_for_search, searchValue);

                if (found_idx != std::numeric_limits<size_t>::max() && std::abs(selected_dataset_for_search.time_data[found_idx] - searchValue) < 0.1) {
                    selected_dataset_idx = (size_t)search_dataset_idx_int;
                    selected_point_idx = found_idx;
                    bool already_pinned = false;
                    for (const auto& pinned : pinned_points) {
                        if (pinned.dataset_idx == selected_dataset_idx && pinned.point_idx == selected_point_idx) {
                            already_pinned = true;
                            break;
                        }
                    }
                    if (!already_pinned) {
                        pinned_points.push_back({ next_pin_uid++, selected_dataset_idx, selected_point_idx, true, false });
                    }
                }
                else { show_search_warning = true; }
            }
            catch (...) { show_search_warning = true; }
        }
    }
    ImGui::Columns(1);
    ImGui::EndChild();

    // Alt Animasyon Kontrol Paneli
    ImGui::BeginChild("AnimasyonPaneli", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 4.0f), true, ImGuiWindowFlags_NoScrollbar);
    if (ImGui::Button(is_playing ? "Durdur" : "Oynat")) {
        is_playing = !is_playing;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-150); // Genişliği ayarla
    ImGui::SliderFloat("Zaman", &current_time, 0.0f, max_time);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("Hiz", &playback_speed, 0.1f, 10.0f);
    ImGui::EndChild();

    if (show_search_warning) {
        ImGui::OpenPopup("Uyari");
        show_search_warning = false;
    }
    if (ImGui::BeginPopupModal("Uyari", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Aramayla eslesen nokta bulunamadi.");
        if (ImGui::Button("Tamam")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImVec2 viewport_pos = ImGui::GetCursorScreenPos();
    ImVec2 viewport_size = ImGui::GetContentRegionAvail();

    // Çizilecek tüm hata vektörlerini topla
    std::vector<std::pair<DataPoint, DataPoint>> vectors_to_draw;
    size_t truth_ds_idx = find_truth_dataset_idx();
    if (truth_ds_idx != std::numeric_limits<size_t>::max()) {
        const auto& truth_ds = datasets[truth_ds_idx];
        for (const auto& pinned : pinned_points) {
            if (pinned.show_error_vector) {
                const auto& ds = datasets[pinned.dataset_idx];
                if (ds.name.find("Radar") != std::string::npos) {
                    double measurement_time = ds.time_data[pinned.point_idx];
                    size_t truth_idx = find_closest_time_index(truth_ds, measurement_time);
                    if (truth_idx != std::numeric_limits<size_t>::max()) {
                        vectors_to_draw.push_back({ ds.points[pinned.point_idx], truth_ds.points[truth_idx] });
                    }
                }
            }
        }
    }
    renderer.update_error_vectors(vectors_to_draw);

    render_scene();

    ImGui::SetCursorScreenPos(viewport_pos);
    ImGui::Image((void*)(intptr_t)renderer.get_fbo_texture(), viewport_size, ImVec2(0, 1), ImVec2(1, 0));

    handle_input(viewport_pos, viewport_size);

    render_pinned_point_windows(viewport_pos, viewport_size);

    ImGui::End();
}

void PlotterApp::render_scene() {
    renderer.bind_fbo();
    glViewport(0, 0, 1920, 1080);
    glClearColor(0.01f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    projection_matrix = Math::perspective(45.0f * (float)std::numbers::pi / 180.0f, 1920.f / 1080.f, 1.0f, grid_size * 5.0f);
    Math::Vec3 eye = {
        cam_center.x + cam_distance * cosf(cam_pitch) * sinf(cam_yaw),
        cam_center.y + cam_distance * sinf(cam_pitch),
        cam_center.z + cam_distance * cosf(cam_pitch) * cosf(cam_yaw)
    };
    view_matrix = Math::lookAt(eye, cam_center, { 0,1,0 });

    Math::Mat4 selection_sphere_model = {};
    if (selected_dataset_idx != std::numeric_limits<size_t>::max() && selected_point_idx != std::numeric_limits<size_t>::max()) {
        const auto& p = datasets[selected_dataset_idx].points[selected_point_idx];
        Math::Vec3 gl_pos = { (float)p.x, (float)p.z, (float)-p.y };
        Math::Mat4 point_model = Math::translate(Math::identity(), gl_pos);

        float dx = eye.x - gl_pos.x;
        float dy = eye.y - gl_pos.y;
        float dz = eye.z - gl_pos.z;
        float dist_to_point = sqrtf(dx * dx + dy * dy + dz * dz);
        if (dist_to_point < 1.0f) dist_to_point = 1.0f;

        float sphere_scale = dist_to_point * 0.004f;
        selection_sphere_model = Math::scale(point_model, { sphere_scale, sphere_scale, sphere_scale });
    }

    renderer.draw(view_matrix, projection_matrix, visibility, radar_model_matrix, target_model_matrix, selection_sphere_model);

    renderer.unbind_fbo();
}

void PlotterApp::handle_input(const ImVec2& viewport_pos, const ImVec2& viewport_size) {
    if (ImGui::IsItemHovered()) {
        ImGuiIO& io = ImGui::GetIO();

        if (ImGui::IsMouseClicked(0) && !ImGui::IsMouseDragging(0)) {
            size_t closest_ds_idx = std::numeric_limits<size_t>::max();
            size_t closest_pt_idx = std::numeric_limits<size_t>::max();
            float min_dist_sq = 15.0f * 15.0f;

            for (size_t i = 0; i < datasets.size(); ++i) {
                if (!visibility[i]) continue;
                for (size_t j = 0; j < datasets[i].points.size(); ++j) {
                    const auto& p = datasets[i].points[j];
                    Math::Vec3 screen_pos_3d;
                    if (Math::project({ (float)p.x, (float)p.z, (float)-p.y }, view_matrix, projection_matrix, (int)viewport_size.x, (int)viewport_size.y, screen_pos_3d)) {
                        float dx = io.MousePos.x - (screen_pos_3d.x + viewport_pos.x);
                        float dy = io.MousePos.y - (screen_pos_3d.y + viewport_pos.y);
                        if ((dx * dx + dy * dy) < min_dist_sq) {
                            min_dist_sq = dx * dx + dy * dy;
                            closest_ds_idx = i;
                            closest_pt_idx = j;
                        }
                    }
                }
            }
            selected_dataset_idx = closest_ds_idx;
            selected_point_idx = closest_pt_idx;

            if (selected_dataset_idx != std::numeric_limits<size_t>::max()) {
                bool already_pinned = false;
                for (const auto& pinned : pinned_points) {
                    if (pinned.dataset_idx == selected_dataset_idx && pinned.point_idx == selected_point_idx) {
                        already_pinned = true;
                        break;
                    }
                }
                if (!already_pinned) {
                    pinned_points.push_back({ next_pin_uid++, selected_dataset_idx, selected_point_idx, true, false });
                }
            }
        }

        if (ImGui::IsMouseDragging(0)) {
            cam_yaw -= io.MouseDelta.x * 0.005f;
            cam_pitch -= io.MouseDelta.y * 0.005f;
            cam_pitch = std::clamp(cam_pitch, -1.55f, 1.55f);
        }
        if (ImGui::IsMouseDragging(1)) {
            Math::Vec3 right = { view_matrix.m[0], view_matrix.m[4], view_matrix.m[8] };
            Math::Vec3 up = { view_matrix.m[1], view_matrix.m[5], view_matrix.m[9] };
            float scale = cam_distance * 0.001f;
            cam_center.x -= right.x * io.MouseDelta.x * scale;
            cam_center.y -= right.y * io.MouseDelta.x * scale;
            cam_center.z -= right.z * io.MouseDelta.x * scale;
            cam_center.x += up.x * io.MouseDelta.y * scale;
            cam_center.y += up.y * io.MouseDelta.y * scale;
            cam_center.z += up.z * io.MouseDelta.y * scale;
        }
        cam_distance -= io.MouseWheel * (cam_distance * 0.1f);
        cam_distance = std::max(1.0f, cam_distance);
    }
}


size_t PlotterApp::find_truth_dataset_idx() {
    for (size_t i = 0; i < datasets.size(); ++i) {
        if (datasets[i].name.find("Gercek") != std::string::npos) return i;
    }
    return std::numeric_limits<size_t>::max();
}

size_t PlotterApp::find_closest_time_index(const SearchableDataset& ds, double time) {
    if (ds.time_data.empty()) return std::numeric_limits<size_t>::max();
    auto it = std::lower_bound(ds.time_data.begin(), ds.time_data.end(), time);
    if (it == ds.time_data.end()) return ds.time_data.size() - 1;
    if (it == ds.time_data.begin()) return 0;
    size_t idx = std::distance(ds.time_data.begin(), it);
    if ((time - *(it - 1)) < (*it - time)) {
        return idx - 1;
    }
    return idx;
}

void PlotterApp::render_pinned_point_windows(const ImVec2& viewport_pos, const ImVec2& viewport_size) {
    for (size_t i = 0; i < pinned_points.size(); ) {
        auto& pinned = pinned_points[i];
        const auto& ds = datasets[pinned.dataset_idx];

        char window_title[256];
        snprintf(window_title, sizeof(window_title), "%s - Zaman %.2f##%zu", ds.name.c_str(), ds.time_data[pinned.point_idx], pinned.uid);

        bool window_is_open = true;
        ImGui::SetNextWindowPos(ImVec2(viewport_pos.x + 10 + i * 20, viewport_pos.y + 10 + i * 20), ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(280, 0), ImGuiCond_Appearing);
        ImGui::SetNextWindowBgAlpha(0.75f);

        if (ImGui::Begin(window_title, &window_is_open, ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::Text("Zaman : %.4f s", ds.time_data[pinned.point_idx]);
            ImGui::Separator();
            if (pinned.show_ecef) {
                ImGui::Text("X : %.4f m", ds.x_data[pinned.point_idx]);
                ImGui::Text("Y : %.4f m", ds.y_data[pinned.point_idx]);
                ImGui::Text("Z : %.4f m", ds.z_data[pinned.point_idx]);
            }
            else {
                ImGui::Text("Menzil : %.4f m", ds.range_data[pinned.point_idx]);
                ImGui::Text("Yanca : %.4f deg", ds.bearing_data[pinned.point_idx]);
                ImGui::Text("Yukselis: %.4f deg", ds.elevation_data[pinned.point_idx]);
            }
            if (ds.has_rbe_capability) {
                ImGui::Separator();
                if (ImGui::Button("Koordinat Sistemini Degistir")) {
                    pinned.show_ecef = !pinned.show_ecef;
                }
            }
            size_t truth_ds_idx = find_truth_dataset_idx();
            if (ds.name.find("Radar") != std::string::npos && truth_ds_idx != std::numeric_limits<size_t>::max()) {
                ImGui::Separator();
                if (ImGui::Button("Hata Vektorunu Goster/Gizle")) {
                    pinned.show_error_vector = !pinned.show_error_vector;
                }
            }
        }
        ImGui::End();

        if (!window_is_open) {
            pinned_points.erase(pinned_points.begin() + i);
        }
        else {
            i++;
        }
    }
}

void PlotterApp::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

// Ana Başlatıcı Fonksiyon
void launchInteractivePlotter(const std::vector<SearchableDataset>& datasets, const DataPoint& radar_pos_relative) {
    try {
        PlotterApp app(datasets, radar_pos_relative);
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Bir hata olustu: " << e.what() << std::endl;
    }
}

