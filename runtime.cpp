#include "log.h"
#include "mesh.h"
#include "viewport.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui_enums.h"
#include "raygui.h"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Cube
{
    glm::vec3 position;
    glm::vec3 rotation_axis;
    glm::vec3 scale;
    glm::vec4 color;
    float angular_speed;
};

struct DirectionalLight
{
    glm::vec3 direction;
    glm::vec4 color;
    ftype intensity;
};

MyMesh g_mesh;
DirectionalLight g_main_light;
Viewport g_main_viewport;
Viewport g_axis_viewport;
ftype g_since_start = 0.0f;
ftype g_frame_time = 0.0f;
Image g_sprite_atlas;
bool g_is_rending_depth_buffer = false;
bool g_draw_triangle_edges = false;
bool g_is_viewing_performance_metrics = false;
float g_bias = 0.0f;
float g_wall_x = 0;
float g_wall_y = 10;
int g_wall_column = 0;
int g_wall_row = 0;
glm::vec2 g_ui_zone{175, 220};
int g_pixels_outside_screen = 0;
int g_pixels_behind_other_pixels = 0;
int g_backfacing_triangles = 0;

void InitializeRuntime();
void InitializeCamera(Viewport& viewport, const glm::ivec4& transform, const ftype fov, const ftype zoom_speed);
void RunGame();
void CloseGame();
void Update();
void UpdateLight(DirectionalLight& light, const glm::vec2 move);
void UpdateCamera(Viewport& viewport, const ftype zoom, const glm::vec2 move, const glm::vec2 screen_resize_factor);
void UpdateViewport(Viewport& viewport, const glm::vec2 screen_resize_factor);
void ReloadBuffers(Viewport& viewport, const ftype width, const ftype height);
void Render();
void RenderWorld(Viewport& viewport);
void RenderUI();
void DrawPerformanceMetrics();
void DrawMyMesh(Viewport& viewport, const MyMesh& mesh);
void DrawAxis(const Viewport& viewport, const glm::vec4 position);
void DrawLine3d(const Viewport& viewport, const glm::vec4 start, const glm::vec4 end, const glm::vec4 color);
void DrawColorPixel(Viewport& viewport, const int x, const int y, const ftype z, const glm::vec4 color);
void DrawTextureSampledPixel(Viewport& viewport, const int x, const int y, const ftype z, const glm::vec2 uv, const glm::vec4 add_color);
void DrawPixel(Viewport& viewport, const int x, const int y, const ftype z, const glm::vec4 color);
void Draw3dTriangle(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const glm::vec2* uv, const glm::vec4 add_color, const bool edges_only);
void DrawTriangle(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const glm::vec2* uv, const glm::vec4 add_color, const bool edges_only);
ftype GetSmoothedMouseWheelScroll();
glm::vec2 GetSmoothedMouseMove(const int button);
glm::vec2 GetScreenResizeFactor();
glm::mat4 ClipToScreenSpaceMatrix(const Viewport& viewport);
glm::mat4 TRSMatrix(const glm::vec3 position,const glm::vec3 rotation_axis,const glm::vec3 scale, const float angle);
glm::mat4 ProjectionMatrix(const Viewport& viewport);
glm::mat4 OrthographicProjectionMatrix(const ftype fov, const ftype aspect, const ftype near, const ftype far);
glm::mat4 PerspectiveProjectionMatrix(const ftype fov, const ftype aspect, const ftype near, const ftype far);
glm::mat4 RotationMatrix(const glm::vec3 axis, const ftype angle);
glm::mat4 LookAt(const glm::vec3 position, const glm::vec3 look_at, const glm::vec3 up);
glm::mat4 Mat4(const glm::vec4 column1, const glm::vec4 column2, const glm::vec4 column3, const glm::vec4 column4);
bool IsTopLeftOfTriangle(const glm::vec2 from, const glm::vec2 to);

int main() 
{
    InitializeRuntime();
    RunGame();
    CloseGame();
}

void InitializeRuntime()
{
    const int screen_width = 800;
    const int screen_height = 600;
    InitWindow(screen_width, screen_height, "3D Demo");
    //SetWindowState(FLAG_WINDOW_RESIZABLE);
    GuiLoadStyleDefault();

    SetTraceLogLevel(LOG_DEBUG);

    InitializeCamera(g_main_viewport, {0, 0, screen_width, screen_height}, 20.0f, 250.0f);
    InitializeCamera(g_axis_viewport, {screen_width - 100, 0, 100, 100}, 5.0f, 0.0f);
    SetTargetFPS(60);

    g_sprite_atlas = LoadImage("assets/WallpaperAtlas.png");
    g_mesh = ParseObjFile("assets/Suzanne.obj");
    //g_mesh = ParseObjFile("assets/Cube.obj");

    g_main_light.direction = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    g_main_light.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    g_main_light.intensity = 1.0f;
}

void InitializeCamera(Viewport& viewport, const glm::ivec4& transform, const ftype fov, const ftype zoom_speed)
{
    const ftype near_plane = 4.5f;
    const ftype far_plane = 100.0f;
    
    MyCamera& camera = viewport.camera;
    camera.position = glm::vec3(0.0f, 0.0f, 15.0f);
    camera.lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.near_plane = near_plane;
    camera.far_plane = far_plane;
    camera.fov = fov;
    camera.zoom_speed = zoom_speed;
    camera.rotation_speed = 50.0f;
    camera.is_orthographic = false;
    
    viewport.transform = transform;
    ReloadBuffers(viewport, (ftype)transform.z, (ftype)transform.w);
    UpdateViewport(viewport, {1, 1});
}

void RunGame()
{
    while (!WindowShouldClose()) 
    {
        g_since_start = (ftype)GetTime();
        g_frame_time = GetFrameTime();
        Update();
        Render();
    }
}

void CloseGame()
{
    if(IsImageReady(g_sprite_atlas))
    {
        UnloadImage(g_sprite_atlas);
    }

    Viewport viewports[] = {g_main_viewport, g_axis_viewport};
    for(auto& viewport : viewports)
    {
        if(IsImageReady(viewport.color_buffer))
        {
            UnloadImage(viewport.color_buffer);
        }
    
        if(IsTextureReady(viewport.color_tex2d))
        {
            UnloadTexture(viewport.color_tex2d);
        }
    
        if(IsImageReady(viewport.z_buffer))
        {
            UnloadImage(viewport.z_buffer);
        }
    
        if(IsTextureReady(viewport.z_tex2d))
        {
            UnloadTexture(viewport.z_tex2d);
        }
    }

    CloseWindow();
}

void Update()
{
    const ftype zoom = GetSmoothedMouseWheelScroll();
    const glm::vec2 left_mouse_delta = GetSmoothedMouseMove(MOUSE_LEFT_BUTTON);
    const glm::vec2 right_mouse_delta = GetSmoothedMouseMove(MOUSE_RIGHT_BUTTON);
    const glm::vec2 screen_resize_factor = GetScreenResizeFactor();

    const bool is_zkey_pressed = IsKeyPressed(KEY_Z);
    if(is_zkey_pressed && !g_is_rending_depth_buffer)
    {
        g_is_rending_depth_buffer = true;
    }
    else if(is_zkey_pressed && g_is_rending_depth_buffer)
    {
        g_is_rending_depth_buffer = false;
    }

    const bool is_wkey_pressed = IsKeyPressed(KEY_W);
    if(is_wkey_pressed & !g_draw_triangle_edges)
    {
        g_draw_triangle_edges = true;
    }
    else if(is_wkey_pressed && g_draw_triangle_edges)
    {
        g_draw_triangle_edges = false;
    }

    const bool is_skey_pressed = IsKeyPressed(KEY_S);
    if(is_skey_pressed && !g_is_viewing_performance_metrics)
    {
        g_is_viewing_performance_metrics = true;
    }
    else if(is_skey_pressed && g_is_viewing_performance_metrics)
    {
        g_is_viewing_performance_metrics = false;
    }

    UpdateLight(g_main_light, right_mouse_delta);
    UpdateCamera(g_main_viewport, zoom, left_mouse_delta, screen_resize_factor);
    UpdateCamera(g_axis_viewport, zoom, left_mouse_delta, screen_resize_factor);
}

void UpdateLight(DirectionalLight& light, const glm::vec2 move)
{
    const ftype rotation_speed = 0.5f;
    const ftype length = glm::length(light.direction);
    const glm::vec3 forward = glm::normalize(light.direction);
    const glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 move_delta = right * move.x + glm::vec3(0.0f, 1.0f, 0.0f) * move.y;
    light.direction += rotation_speed * g_frame_time * move_delta;
    light.direction = length * glm::normalize(light.direction);
}

void UpdateCamera(Viewport& viewport, const ftype zoom, const glm::vec2 move, const glm::vec2 screen_resize_factor)
{
    MyCamera& camera = viewport.camera;

    // Min fov at 20 for now so fps doesn't drop too much
    camera.fov += -zoom * camera.zoom_speed;
    camera.fov = glm::clamp<float>(camera.fov, 5, 180);
    
    const ftype length = glm::length(camera.position);
    const ftype rotation_speed = camera.rotation_speed * length;
    
    const glm::vec3 forward = glm::normalize(camera.lookAt - camera.position);
    const glm::vec3 right = glm::cross(forward, camera.up);
    const glm::vec3 move_delta = right * move.x + camera.up * move.y;
    camera.position += rotation_speed * g_frame_time * move_delta;
    camera.position = length * glm::normalize(camera.position);
    camera.up = glm::cross(right, forward);

    bool do_update_projection_matrix = false;
    do_update_projection_matrix = IsKeyPressed(KEY_SPACE);
    camera.is_orthographic = do_update_projection_matrix ? !camera.is_orthographic : camera.is_orthographic;

    do_update_projection_matrix = viewport.last_fov != camera.fov || do_update_projection_matrix;
    viewport.last_fov = do_update_projection_matrix ? camera.fov : viewport.last_fov;

    do_update_projection_matrix = viewport.last_near_z != camera.near_plane || do_update_projection_matrix;
    viewport.last_near_z = do_update_projection_matrix ? camera.near_plane : viewport.last_near_z;

    do_update_projection_matrix = viewport.last_far_z != camera.far_plane || do_update_projection_matrix;
    viewport.last_far_z = do_update_projection_matrix ? camera.far_plane : viewport.last_far_z;

    do_update_projection_matrix = move.x != 0 || move.y != 0 || do_update_projection_matrix;

    do_update_projection_matrix = screen_resize_factor.x != 1.0f || screen_resize_factor.y != 1.0f || do_update_projection_matrix;

    if(do_update_projection_matrix)
    {
        UpdateViewport(viewport, screen_resize_factor);
    }
}

void UpdateViewport(Viewport& viewport, const glm::vec2 screen_resize_factor)
{
    viewport.transform.z = (ftype)glm::round(viewport.transform.z * screen_resize_factor.x);
    viewport.transform.w = (ftype)glm::round(viewport.transform.w * screen_resize_factor.y);
    const ftype width = (ftype)viewport.transform.z;
    const ftype height = (ftype)viewport.transform.w;
    MyCamera& camera = viewport.camera;

    camera.aspect = width / height;

    const glm::mat4 worldToCameraSpace = LookAt(camera.position, camera.lookAt, camera.up);
    //LogMat4("World To Camera", worldToCameraSpace);

    const glm::mat4 projectionMatrix = ProjectionMatrix(viewport);
    //LogMat4("Projection", projectionMatrix);
    
    const glm::mat4 clipToScreenSpace = ClipToScreenSpaceMatrix(viewport);
    //LogMat4("Clip To Screen", clipToScreenSpace);

    camera.worldToScreenSpace = clipToScreenSpace * projectionMatrix * worldToCameraSpace;

    if(screen_resize_factor.x == 1.0f || screen_resize_factor.y == 1.0f)
    {
        // only reload the z_buffer and color_buffer if the screen size has changed
        return;
    }

    ReloadBuffers(viewport, width, height);
}

void ReloadBuffers(Viewport& viewport, const ftype width, const ftype height)
{
    if(IsImageReady(viewport.z_buffer))
    {
        UnloadImage(viewport.z_buffer);
    }

    if(IsTextureReady(viewport.z_tex2d))
    {
        UnloadTexture(viewport.z_tex2d);
    }
    
    if(IsImageReady(viewport.color_buffer))
    {
        UnloadImage(viewport.color_buffer);
    }
    
    if(IsTextureReady(viewport.color_tex2d))
    {
        UnloadTexture(viewport.color_tex2d);
    }
    
    viewport.z_buffer = GenImageColor((int)width, (int)height, WHITE);
    viewport.z_tex2d = LoadTextureFromImage(viewport.z_buffer);
    viewport.color_buffer = GenImageColor((int)width, (int)height, BLACK);
    viewport.color_tex2d = LoadTextureFromImage(viewport.color_buffer);
}

void Render()
{
    // reset performance counters
    g_backfacing_triangles = 0;
    g_pixels_outside_screen = 0;
    g_pixels_behind_other_pixels = 0;

    BeginDrawing();
    ClearBackground(BLACK);
    RenderWorld(g_main_viewport);
    RenderUI();
    EndDrawing();
}

void RenderWorld(Viewport& viewport)
{
    ImageClearBackground(&viewport.z_buffer, WHITE);
    ImageClearBackground(&viewport.color_buffer, BLACK);

    DrawMyMesh(viewport, g_mesh);

    if(g_is_rending_depth_buffer)
    {
        UpdateTexture(g_main_viewport.z_tex2d, g_main_viewport.z_buffer.data);
        DrawTexture(viewport.z_tex2d, 0, 0, WHITE);
        return;
    }

    UpdateTexture(viewport.color_tex2d, viewport.color_buffer.data);
    DrawTexture(viewport.color_tex2d, 0, 0, WHITE);
}

void RenderUI()
{
    DrawAxis(g_axis_viewport, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    MyCamera& camera = g_main_viewport.camera;
    std::string state = camera.is_orthographic ? "Orthographic" : "Perspective";
    state = g_is_rending_depth_buffer ? "Depth Buffer" : state;
    Color text_color = g_is_rending_depth_buffer ? BLUE : YELLOW;
    DrawText(state.c_str(), 10, 10, 20, text_color);

    GuiSlider({35, 30, 100, 20}, "Col", TextFormat("%d", g_wall_column), &g_wall_x, 0, 7);
    GuiSlider({35, 50, 100, 20}, "Row", TextFormat("%d", g_wall_row), &g_wall_y, 0, 14);
    g_wall_column = (int)glm::round(g_wall_x);
    g_wall_row = (int)glm::round(g_wall_y);
    
    GuiSlider({35, 70, 100, 20}, "Near", TextFormat("%0.1f", camera.near_plane), &camera.near_plane, 0.1f, 20.0f);
    GuiSlider({35, 90, 100, 20}, "Far", TextFormat("%0.1f", camera.far_plane), &camera.far_plane, 20.1f, 100.0f);
    DrawRectangleLines(0, 0, (int)g_ui_zone.x, (int)g_ui_zone.y, WHITE);

    static Vector3 light_color = {1, 0, 1};
    GuiColorPickerHSV({35, 110, 100, 100}, "Light Color", &light_color);
    const Vector3 color = ConvertHSVtoRGB(light_color);
    g_main_light.color.r = color.x;
    g_main_light.color.g = color.y;
    g_main_light.color.b = color.z;

    if(g_is_viewing_performance_metrics)
    {
        DrawPerformanceMetrics();
    }
}

void DrawPerformanceMetrics()
{
    constexpr int font_size = 20;
    DrawRectangle(0, 0, (int)g_ui_zone.x, (int)g_ui_zone.y, Fade(BLACK, 0.8f));

    const int fps = GetFPS();
    DrawText(TextFormat("FPS: %d", fps), 10, 10, font_size, YELLOW);

    DrawText(TextFormat("Backfacing Triangles: %d", g_backfacing_triangles), 10, 30, font_size, YELLOW);
    DrawText(TextFormat("Pixels Out-of-bounds: %d", g_pixels_outside_screen), 10, 50, font_size, YELLOW);
    DrawText(TextFormat("Pixels behind pixles: %d", g_pixels_behind_other_pixels), 10, 70, font_size, YELLOW);
}

void DrawMyMesh(Viewport& viewport, const MyMesh& mesh)
{
    const glm::vec4 light_color{0, 0, 0, 0};
    for(int i = 0; i < mesh.triangle_count(); ++i)
    {
        constexpr int vertex_count = 3;
        ftype vertices[3 * vertex_count];
        ftype uvs[2 * vertex_count];
        ftype normals[3 * vertex_count];
        GetMeshTriangle(mesh, i, vertices, uvs, normals);

        Vertex a{{}}, b{{}}, c{{}};
        Vertex* vertex_container[] = {&a, &b, &c};
        for(int k = 0; k < vertex_count; ++k)
        {
            Vertex* vertex = vertex_container[k];
            const ftype x = vertices[k * 3 + 0];
            const ftype y = vertices[k * 3 + 1];
            const ftype z = vertices[k * 3 + 2];
            vertex->position = {x, y, z};

            const ftype nx = normals[k * 3 + 0];
            const ftype ny = normals[k * 3 + 1];
            const ftype nz = normals[k * 3 + 2];
            vertex->normal = {nx, ny, nz};

            const ftype u = uvs[k * 2 + 0];
            const ftype v = uvs[k * 2 + 1];
            vertex->uv = {u, v};
        }

        Draw3dTriangle(viewport, a, b, c, nullptr, light_color, g_draw_triangle_edges);
    }
}

void DrawAxis(const Viewport& viewport, const glm::vec4 position)
{
    const glm::vec4 x_axis = position + glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    const glm::vec4 y_axis = position + glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    const glm::vec4 z_axis = position + glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    DrawLine3d(viewport, position, x_axis, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    DrawLine3d(viewport, position, y_axis, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    DrawLine3d(viewport, position, z_axis, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
}

void DrawLine3d(const Viewport& viewport, const glm::vec4 start, const glm::vec4 end, const glm::vec4 color)
{
    const MyCamera& camera = viewport.camera;

    glm::vec4 clippedStart = camera.worldToScreenSpace * start;
    clippedStart /= clippedStart.w;

    glm::vec4 clippedEnd = camera.worldToScreenSpace * end;
    clippedEnd /= clippedEnd.w;

    const Color raylib_color = ColorFromNormalized({color.x, color.y, color.z, color.w});
    DrawLineEx({clippedStart.x, clippedStart.y}, {clippedEnd.x, clippedEnd.y}, 3.0f, raylib_color);
}

void DrawColorPixel(Viewport& viewport, const int x, const int y, const ftype z, const glm::vec4 color)
{
    DrawPixel(viewport, x, y, z, color);
}

void DrawTextureSampledPixel(Viewport& viewport, const int x, const int y, const ftype z, const glm::vec2 uv, const glm::vec4 add_color)
{
    static const glm::mat2 uv_matrix{
        g_sprite_atlas.width, 0,
        0, g_sprite_atlas.height
    };

    // affine texture mapping (creates the wobbly textures characteristic of PS1 games)
    const glm::vec2 uv1 = {glm::fract(uv.x), glm::fract(uv.y)};
    const glm::vec2 texcoords = uv_matrix * uv1;
    const int u = (int)glm::floor(texcoords.x);
    const int v = (int)glm::floor(texcoords.y);
    const Vector4 texture_color = ColorNormalize(GetImageColor(g_sprite_atlas, u, v));
    const glm::vec4 final_color = {
        texture_color.x * add_color.x, 
        texture_color.y * add_color.y, 
        texture_color.z * add_color.z, 
        texture_color.w
    };

    DrawPixel(viewport, x, y, z, final_color);
}

void DrawPixel(Viewport& viewport, const int x, const int y, const ftype z, const glm::vec4 color)
{
    const int screen_width = viewport.transform.z;
    const int screen_height = viewport.transform.w;
    const bool is_outside_z_bounds = z < -1 || z > 1;
    const bool is_outside_screen_bounds = x < 0 || x >= screen_width || y < 0 || y >= screen_height;
    if(is_outside_screen_bounds)
    {
        ++g_pixels_outside_screen;
        return;
    }

    const float depth = ColorNormalize(GetImageColor(viewport.z_buffer, x, y)).z;
    if(depth < z)
    {
        ++g_pixels_behind_other_pixels;
        return;
    }

    ImageDrawPixel(&viewport.z_buffer, x, y, ColorFromNormalized({z, z, z, 1.0f}));
    ImageDrawPixel(&viewport.color_buffer, x, y, ColorFromNormalized({color.r, color.g, color.b, color.a}));
}

void Draw3dTriangle(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const glm::vec2* uv, const glm::vec4 add_color, const bool edges_only)
{
    const MyCamera& camera = viewport.camera;
    glm::vec3 normal = (a.normal + b.normal + c.normal) / 3.0f;

    const glm::vec3 look_at_direction = camera.lookAt - camera.position;
    const bool is_backfacing = glm::dot(normal, look_at_direction) >= 0.0f;
    if(is_backfacing)
    {
        ++g_backfacing_triangles;
        return;
    }

    const auto facingLightFactor = glm::clamp<ftype>(-glm::dot(g_main_light.direction, normal), 0.2f, 1);
    glm::vec4 light_color = facingLightFactor * g_main_light.color;
    light_color.a = 1.0f;

    glm::vec4 a_screen = camera.worldToScreenSpace * glm::vec4(a.position, 1.0f);
    glm::vec4 b_screen = camera.worldToScreenSpace * glm::vec4(b.position, 1.0f);
    glm::vec4 c_screen = camera.worldToScreenSpace * glm::vec4(c.position, 1.0f);
    a_screen /= a_screen.w;
    b_screen /= b_screen.w;
    c_screen /= c_screen.w;

    const Vertex a1 = {a_screen, normal, a.uv};
    const Vertex b1 = {b_screen, normal, b.uv};
    const Vertex c1 = {c_screen, normal, c.uv};
    DrawTriangle(viewport, a1, b1, c1, uv, light_color, edges_only);
}

void DrawTriangle(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const glm::vec2* uv, const glm::vec4 add_color, const bool edges_only)
{
    if(edges_only)
    {
        ImageDrawLineV(&viewport.color_buffer, {a.position.x, a.position.y}, {b.position.x, b.position.y}, WHITE);
        ImageDrawLineV(&viewport.color_buffer, {b.position.x, b.position.y}, {c.position.x, c.position.y}, WHITE);
        ImageDrawLineV(&viewport.color_buffer, {c.position.x, c.position.y}, {a.position.x, a.position.y}, WHITE);
        return;
    }
    
    auto min_x = (int)glm::round(glm::min(a.position.x, glm::min(b.position.x, c.position.x)));
    auto max_x = (int)glm::round(glm::max(a.position.x, glm::max(b.position.x, c.position.x)));
    auto min_y = (int)glm::round(glm::min(a.position.y, glm::min(b.position.y, c.position.y)));
    auto max_y = (int)glm::round(glm::max(a.position.y, glm::max(b.position.y, c.position.y)));
    const auto dim_x = max_x - min_x;
    const auto dim_y = max_y - min_y;
    const auto dim = dim_x * dim_y;

    // fix overlapping triangles using top-left edge rule
    // there's issue using floating point because the floating point numbers are not evenly spaced.
    // Solution is using fixed point numbers.
    const ftype bias_0 = IsTopLeftOfTriangle(a.position, b.position) ? 0.0f : -0.0001f;
    const ftype bias_1 = IsTopLeftOfTriangle(b.position, c.position) ? 0.0f : -0.0001f;
    const ftype bias_2 = IsTopLeftOfTriangle(c.position, a.position) ? 0.0f : -0.0001f;
    
    const glm::vec3 a_to_b = b.position - a.position;
    const glm::vec3 a_to_c = c.position - a.position;
    // this is actually parallelogram area, but the ratio is the same between triangles and parallelograms
    // when calculating the barycentric coordinates
    const ftype triangle_area_recip = 1.0f / (a_to_c.x * a_to_b.y - a_to_c.y * a_to_b.x);

    for(int i = 0; i < dim; ++i)
    {
        const int x = min_x + i % dim_x;
        const int y = min_y + i / dim_x;

        // calculate the barycentric coordinates
        const glm::vec2 a_to_p{x - a.position.x, y - a.position.y};
        const ftype alpha = (a_to_p.x * a_to_b.y - a_to_p.y * a_to_b.x) * triangle_area_recip;
        const ftype beta = -(a_to_p.x * a_to_c.y - a_to_p.y * a_to_c.x) * triangle_area_recip;
        const ftype gamma = 1 - alpha - beta;
        const bool is_outside_triangle = alpha + bias_0 < 0 || beta + bias_2 < 0 || gamma + bias_1 < 0;

        if(is_outside_triangle)
        {
            //ImageDrawPixelV(&viewport.color_buffer, Vector2{(float)x, (float)y}, PINK);
            continue;
        }

        const ftype z = gamma * a.position.z + alpha * c.position.z + beta * b.position.z;
        DrawTextureSampledPixel(viewport, x, y, z, gamma * a.uv + alpha * c.uv + beta * b.uv, add_color);
    }
}

ftype GetSmoothedMouseWheelScroll()
{
    static ftype last_zoom = 0.0f;
    const ftype zoom = GetMouseWheelMove();
    const ftype avg_zoom = (zoom + last_zoom) * 0.5f;
    last_zoom = avg_zoom;
    return avg_zoom * g_frame_time;
}

glm::vec2 GetSmoothedMouseMove(const int button)
{
    const Vector2 mouse_delta = IsMouseButtonDown(button) ? GetMouseDelta() : Vector2{0.0f, 0.0f};
    const glm::vec2 glm_delta = {-mouse_delta.x, mouse_delta.y};
    const Vector2 position = GetMousePosition();
    return position.x < g_ui_zone.x && position.y < g_ui_zone.y ? glm::vec2{0.0f, 0.0f} : glm_delta;
}

glm::vec2 GetScreenResizeFactor()
{
    static int last_screen_width = GetScreenWidth();
    static int last_screen_height = GetScreenHeight();
    const ftype width_change_factor = GetScreenWidth() / (ftype)last_screen_width;
    const ftype height_change_factor = GetScreenHeight() / (ftype)last_screen_height;
    last_screen_width = GetScreenWidth();
    last_screen_height = GetScreenHeight();
    return {width_change_factor, height_change_factor};
}

glm::mat4 ClipToScreenSpaceMatrix(const Viewport& viewport)
{
    const ftype x = (ftype)viewport.transform.x;
    const ftype y = (ftype)viewport.transform.y;
    const ftype viewport_half_width = viewport.transform.z * 0.5f;
    const ftype viewport_half_height = viewport.transform.w * 0.5f;
    const glm::vec4 column1{viewport_half_width, 0, 0, 0};
    const glm::vec4 column2{0, -viewport_half_height, 0, 0};
    const glm::vec4 column3{0, 0, 1, 0};
    const glm::vec4 column4{x + viewport_half_width, y + viewport_half_height, 0, 1};
    return Mat4(column1, column2, column3, column4);
}

glm::mat4 TRSMatrix(const glm::vec3 position, const glm::vec3 rotation_axis, const glm::vec3 scale, const float angle)
{
    return glm::translate(glm::mat4(1.0f), position) * RotationMatrix(rotation_axis, angle) * glm::scale(glm::mat4(1.0f), scale);
}

glm::mat4 ProjectionMatrix(const Viewport& viewport)
{
    const MyCamera& camera = viewport.camera;
    return camera.is_orthographic 
        ? OrthographicProjectionMatrix(camera.fov, camera.aspect, camera.near_plane, camera.far_plane) 
        : PerspectiveProjectionMatrix(camera.fov, camera.aspect, camera.near_plane, camera.far_plane);
}

glm::mat4 OrthographicProjectionMatrix(const ftype fov, const ftype aspect, const ftype near, const ftype far)
{
    const ftype height = near * glm::tan(glm::radians(fov * 0.5f));
    const ftype width = aspect * height;
    const ftype depth = far - near;
    const glm::vec4 column1{1 / width, 0, 0, 0};
    const glm::vec4 column2{0, 1 / height, 0, 0};
    const glm::vec4 column3{0, 0, 1 / depth, 0};
    const glm::vec4 column4{0, 0, -0.5f * (far + near), 1};
    return Mat4(column1, column2, -column3, column4);
}

glm::mat4 PerspectiveProjectionMatrix(const ftype fov, const ftype aspect, const ftype near, const ftype far)
{
    const ftype depth = far - near;
    const ftype tan_fov = glm::tan(glm::radians(fov * 0.5f));
    const glm::vec4 column1{1 / (aspect * tan_fov), 0, 0, 0};
    const glm::vec4 column2{0, 1 / tan_fov, 0, 0};
    const glm::vec4 column3{0, 0, (far + near) / depth, 1};
    const glm::vec4 column4{0, 0, -2 * near * far / depth, 1};
    return Mat4(column1, column2, -column3, column4);
}

glm::mat4 RotationMatrix(const glm::vec3 axis, const ftype angle)
{
    /*
    v = (n * a)a
    w = n - v
    u = a x w
    n' = v + cos(t)w + sin(t)u
    */
   const glm::vec3 rotation_axis = glm::normalize(axis);
   const ftype cos_theta = glm::cos(angle);
   const ftype sin_theta = glm::sin(angle);
   glm::mat4 rot(1);

   {
       const glm::vec3 n = glm::vec3(1, 0, 0);
       const glm::vec3 v = glm::dot(rotation_axis, n) * rotation_axis;
       const glm::vec3 w = n - v;
       const glm::vec3 u = glm::cross(rotation_axis, w);
       rot[0][0] = v.x + cos_theta * w.x + sin_theta * u.x;
       rot[0][1] = v.y + cos_theta * w.y + sin_theta * u.y;
       rot[0][2] = v.z + cos_theta * w.z + sin_theta * u.z;
   }
   
   {
       const glm::vec3 n = glm::vec3(0, 1, 0);
       const glm::vec3 v = glm::dot(rotation_axis, n) * rotation_axis;
       const glm::vec3 w = n - v;
       const glm::vec3 u = glm::cross(rotation_axis, w);
       rot[1][0] = v.x + cos_theta * w.x + sin_theta * u.x;
       rot[1][1] = v.y + cos_theta * w.y + sin_theta * u.y;
       rot[1][2] = v.z + cos_theta * w.z + sin_theta * u.z;
   }
   
   {
       const glm::vec3 n = glm::vec3(0, 0, 1);
       const glm::vec3 v = glm::dot(rotation_axis, n) * rotation_axis;
       const glm::vec3 w = n - v;
       const glm::vec3 u = glm::cross(rotation_axis, w);
       rot[2][0] = v.x + cos_theta * w.x + sin_theta * u.x;
       rot[2][1] = v.y + cos_theta * w.y + sin_theta * u.y;
       rot[2][2] = v.z + cos_theta * w.z + sin_theta * u.z;
   }

   return rot;
}

glm::mat4 LookAt(const glm::vec3 position, const glm::vec3 look_at, const glm::vec3 up)
{
    const glm::vec3 lookat_direction = glm::normalize(look_at - position);
    const glm::vec3 right = glm::normalize(glm::cross(lookat_direction, up));
    const glm::vec3 up_direction = glm::cross(right, lookat_direction);
    const glm::vec4 column1{right.x, right.y, right.z, 0};
    const glm::vec4 column2{up_direction.x, up_direction.y, up_direction.z, 0};
    const glm::vec4 column3{lookat_direction.x, lookat_direction.y, lookat_direction.z, 0};
    const glm::vec4 column4{position.x, position.y, position.z, 1};

    return glm::inverse(Mat4(column1, column2, -column3, column4));
}

glm::mat4 Mat4(const glm::vec4 column1, const glm::vec4 column2, const glm::vec4 column3, const glm::vec4 column4)
{
    const glm::vec4& c1 = column1;
    const glm::vec4& c2 = column2;
    const glm::vec4& c3 = column3;
    const glm::vec4& c4 = column4;
    return glm::mat4{
        //       row1  row2  row3  row4
        /*Col 1*/c1.x, c1.y, c1.z, c1.w,
        /*Col 2*/c2.x, c2.y, c2.z, c2.w,
        /*Col 3*/c3.x, c3.y, c3.z, c3.w,
        /*Col 4*/c4.x, c4.y, c4.z, c4.w
    };
}

bool IsTopLeftOfTriangle(const glm::vec2 from, const glm::vec2 to)
{
    const glm::vec2 a_to_b = to - from;
    const bool is_flat_edge = a_to_b.y == 0 && a_to_b.x < 0;
    const bool is_left_edge = a_to_b.y > 0;
    return is_flat_edge || is_left_edge;
}