#include "log.h"
#include "mesh.h"
#include "viewport.h"

#include "raylib.h"
#include "raymath.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui_enums.h"
#include "raygui.h"

struct Vertex
{
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
};

struct Cube
{
    Vector3 position;
    Vector3 rotation_axis;
    Vector3 scale;
    Vector4 color;
    float angular_speed;
};

struct DirectionalLight
{
    Vector3 direction;
    Vector4 color;
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
Vector2 g_ui_zone{175, 220};
int g_pixels_outside_screen = 0;
int g_pixels_behind_other_pixels = 0;
int g_backfacing_triangles = 0;

void InitializeRuntime();
void InitializeCamera(Viewport& viewport, const Vector4& transform, const ftype fov, const ftype zoom_speed);
void RunGame();
void CloseGame();
void Update();
void UpdateLight(DirectionalLight& light, const Vector2 move);
void UpdateCamera(Viewport& viewport, const ftype zoom, const Vector2 move, const Vector2 screen_resize_factor);
void UpdateViewport(Viewport& viewport, const Vector2 screen_resize_factor);
void ReloadBuffers(Viewport& viewport, const ftype width, const ftype height);
void Render();
void RenderWorld(Viewport& viewport);
void RenderUI();
void DrawPerformanceMetrics();
void DrawMyMesh(Viewport& viewport, const MyMesh& mesh);
void DrawAxis(const Viewport& viewport, const Vector4 position);
void DrawLine3d(const Viewport& viewport, const Vector4 start, const Vector4 end, const Vector4 color);
void DrawColorPixel(Viewport& viewport, const int x, const int y, const ftype z, const Vector4 color);
void DrawTextureSampledPixel(Viewport& viewport, const int x, const int y, const ftype z, const Vector2 uv, const Vector4 add_color);
void DrawPixel(Viewport& viewport, const int x, const int y, const ftype z, const Vector4 color);
void Draw3dTriangle(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const Vector2* uv, const Vector4 add_color, const bool edges_only);
void DrawTriangle(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const Vector2* uv, const Vector4 add_color, const bool edges_only);
ftype GetSmoothedMouseWheelScroll();
Vector2 GetSmoothedMouseMove(const int button);
Vector2 GetScreenResizeFactor();
Matrix ClipToScreenSpaceMatrix(const Viewport& viewport);
Matrix ProjectionMatrix(const Viewport& viewport);
Matrix OrthographicProjectionMatrix(const ftype fov, const ftype aspect, const ftype near, const ftype far);
Matrix PerspectiveProjectionMatrix(const ftype fov, const ftype aspect, const ftype near, const ftype far);
Matrix Mat4(const Vector4 column1, const Vector4 column2, const Vector4 column3, const Vector4 column4);
bool IsTopLeftOfTriangle(const Vector2 from, const Vector2 to);

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

    g_main_light.direction = Vector3Normalize({1.0f, 1.0f, 1.0f});
    g_main_light.color = Vector4{1.0f, 1.0f, 1.0f, 1.0f};
    g_main_light.intensity = 1.0f;
}

void InitializeCamera(Viewport& viewport, const Vector4& transform, const ftype fov, const ftype zoom_speed)
{
    const ftype near_plane = 4.5f;
    const ftype far_plane = 100.0f;
    
    MyCamera& camera = viewport.camera;
    camera.position = Vector3{0.0f, 0.0f, 15.0f};
    camera.lookAt = Vector3{0.0f, 0.0f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
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
    if(IsImageValid(g_sprite_atlas))
    {
        UnloadImage(g_sprite_atlas);
    }

    Viewport viewports[] = {g_main_viewport, g_axis_viewport};
    for(auto& viewport : viewports)
    {
        if(IsImageValid(viewport.color_buffer))
        {
            UnloadImage(viewport.color_buffer);
        }
    
        if(IsTextureValid(viewport.color_tex2d))
        {
            UnloadTexture(viewport.color_tex2d);
        }
    
        if(IsImageValid(viewport.z_buffer))
        {
            UnloadImage(viewport.z_buffer);
        }
    
        if(IsTextureValid(viewport.z_tex2d))
        {
            UnloadTexture(viewport.z_tex2d);
        }
    }

    CloseWindow();
}

void Update()
{
    const ftype zoom = GetSmoothedMouseWheelScroll();
    const Vector2 left_mouse_delta = GetSmoothedMouseMove(MOUSE_LEFT_BUTTON);
    const Vector2 right_mouse_delta = GetSmoothedMouseMove(MOUSE_RIGHT_BUTTON);
    const Vector2 screen_resize_factor = GetScreenResizeFactor();

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

void UpdateLight(DirectionalLight& light, const Vector2 move)
{
    const ftype rotation_speed = 0.5f;
    const ftype length = Vector3Length(light.direction);
    const Vector3 forward = Vector3Normalize(light.direction);
    const Vector3 right = Vector3CrossProduct(forward, Vector3{0.0f, 1.0f, 0.0f});
    const Vector3 move_delta = right * move.x + Vector3{0.0f, 1.0f, 0.0f} * move.y;
    light.direction += Vector3Scale(move_delta, rotation_speed * g_frame_time);
    light.direction = Vector3Scale(Vector3Normalize(light.direction), length);
}

void UpdateCamera(Viewport& viewport, const ftype zoom, const Vector2 move, const Vector2 screen_resize_factor)
{
    MyCamera& camera = viewport.camera;

    // Min fov at 20 for now so fps doesn't drop too much
    camera.fov += -zoom * camera.zoom_speed;
    camera.fov = Clamp(camera.fov, 5, 180);
    
    const ftype length = Vector3Length(camera.position);
    const ftype rotation_speed = camera.rotation_speed * length;
    
    const Vector3 forward = Vector3Normalize(Vector3Subtract(camera.lookAt, camera.position));
    const Vector3 right = Vector3CrossProduct(forward, camera.up);
    const Vector3 move_delta = right * move.x + camera.up * move.y;
    camera.position += Vector3Scale(move_delta, rotation_speed * g_frame_time);
    camera.position = Vector3Scale(Vector3Normalize(camera.position), length);
    camera.up = Vector3CrossProduct(right, forward);

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

void UpdateViewport(Viewport& viewport, const Vector2 screen_resize_factor)
{
    viewport.transform.z = (ftype)round(viewport.transform.z * screen_resize_factor.x);
    viewport.transform.w = (ftype)round(viewport.transform.w * screen_resize_factor.y);
    const ftype width = (ftype)viewport.transform.z;
    const ftype height = (ftype)viewport.transform.w;
    MyCamera& camera = viewport.camera;

    camera.aspect = width / height;

    const Matrix worldToCameraSpace = MatrixLookAt(camera.position, camera.lookAt, camera.up);
    //LogMat4("World To Camera", worldToCameraSpace);

    const Matrix projectionMatrix = ProjectionMatrix(viewport);
    //LogMat4("Projection", projectionMatrix);
    
    const Matrix clipToScreenSpace = ClipToScreenSpaceMatrix(viewport);
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
    if(IsImageValid(viewport.z_buffer))
    {
        UnloadImage(viewport.z_buffer);
    }

    if(IsTextureValid(viewport.z_tex2d))
    {
        UnloadTexture(viewport.z_tex2d);
    }
    
    if(IsImageValid(viewport.color_buffer))
    {
        UnloadImage(viewport.color_buffer);
    }
    
    if(IsTextureValid(viewport.color_tex2d))
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
        UpdateTexture(viewport.z_tex2d, viewport.z_buffer.data);
        DrawTexture(viewport.z_tex2d, 0, 0, WHITE);
        return;
    }

    UpdateTexture(viewport.color_tex2d, viewport.color_buffer.data);
    DrawTexture(viewport.color_tex2d, 0, 0, WHITE);
}

void RenderUI()
{
    DrawAxis(g_axis_viewport, Vector4{0.0f, 0.0f, 0.0f, 1.0f});

    MyCamera& camera = g_main_viewport.camera;
    std::string state = camera.is_orthographic ? "Orthographic" : "Perspective";
    state = g_is_rending_depth_buffer ? "Depth Buffer" : state;
    Color text_color = g_is_rending_depth_buffer ? BLUE : YELLOW;
    DrawText(state.c_str(), 10, 10, 20, text_color);

    GuiSlider({35, 30, 100, 20}, "Col", TextFormat("%d", g_wall_column), &g_wall_x, 0, 7);
    GuiSlider({35, 50, 100, 20}, "Row", TextFormat("%d", g_wall_row), &g_wall_y, 0, 14);
    g_wall_column = (int)round(g_wall_x);
    g_wall_row = (int)round(g_wall_y);
    
    GuiSlider({35, 70, 100, 20}, "Near", TextFormat("%0.1f", camera.near_plane), &camera.near_plane, 0.1f, 20.0f);
    GuiSlider({35, 90, 100, 20}, "Far", TextFormat("%0.1f", camera.far_plane), &camera.far_plane, 20.1f, 100.0f);
    DrawRectangleLines(0, 0, (int)g_ui_zone.x, (int)g_ui_zone.y, WHITE);

    static Vector3 light_color = {1, 0, 1};
    GuiColorPickerHSV({35, 110, 100, 100}, "Light Color", &light_color);
    const Vector3 color = ConvertHSVtoRGB(light_color);
    g_main_light.color.x = color.x;
    g_main_light.color.y = color.y;
    g_main_light.color.z = color.z;

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
    const Vector4 light_color{0, 0, 0, 0};
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

void DrawAxis(const Viewport& viewport, const Vector4 position)
{
    const Vector4 x_axis = Vector4Add(position, Vector4{1.0f, 0.0f, 0.0f, 1.0f});
    const Vector4 y_axis = Vector4Add(position, Vector4{0.0f, 1.0f, 0.0f, 1.0f});
    const Vector4 z_axis = Vector4Add(position, Vector4{0.0f, 0.0f, 1.0f, 1.0f});

    DrawLine3d(viewport, position, x_axis, Vector4{1.0f, 0.0f, 0.0f, 1.0f});
    DrawLine3d(viewport, position, y_axis, Vector4{0.0f, 1.0f, 0.0f, 1.0f});
    DrawLine3d(viewport, position, z_axis, Vector4{0.0f, 0.0f, 1.0f, 1.0f});
}

void DrawLine3d(const Viewport& viewport, const Vector4 start, const Vector4 end, const Vector4 color)
{
    const MyCamera& camera = viewport.camera;

    Vector4 clippedStart = start * camera.worldToScreenSpace;
    clippedStart /= clippedStart.w;

    Vector4 clippedEnd = end * camera.worldToScreenSpace;
    clippedEnd /= clippedEnd.w;

    const Color raylib_color = ColorFromNormalized({color.x, color.y, color.z, color.w});
    DrawLineEx({clippedStart.x, clippedStart.y}, {clippedEnd.x, clippedEnd.y}, 3.0f, raylib_color);
}

void DrawColorPixel(Viewport& viewport, const int x, const int y, const ftype z, const Vector4 color)
{
    DrawPixel(viewport, x, y, z, color);
}

void DrawTextureSampledPixel(Viewport& viewport, const int x, const int y, const ftype z, const Vector2 uv, const Vector4 add_color)
{
    static const Matrix uv_matrix = MatrixScale(g_sprite_atlas.width, g_sprite_atlas.height, 1.0f);

    // affine texture mapping (creates the wobbly textures characteristic of PS1 games)
    const Vector2 uv1 = {(float)((int)uv.x) - uv.x, (float)((int)uv.y) - uv.y};
    const Vector2 texcoords = uv1 * uv_matrix;
    const int u = (int)floor(texcoords.x);
    const int v = (int)floor(texcoords.y);
    const Vector4 texture_color = ColorNormalize(GetImageColor(g_sprite_atlas, u, v));
    const Vector4 final_color = {
        texture_color.x * add_color.x, 
        texture_color.y * add_color.y, 
        texture_color.z * add_color.z, 
        texture_color.w
    };

    DrawPixel(viewport, x, y, z, final_color);
}

void DrawPixel(Viewport& viewport, const int x, const int y, const ftype z, const Vector4 color)
{
    const int screen_width = viewport.transform.z;
    const int screen_height = viewport.transform.w;
    const bool is_outside_z_bounds = z < -1 || z > 1;
    const bool is_outside_screen_bounds = x < 0 || x >= screen_width || y < 0 || y >= screen_height;
    if(is_outside_screen_bounds || is_outside_z_bounds)
    {
        ++g_pixels_outside_screen;
        return;
    }

    const ftype z1 = z * 0.5f + 0.5f; // remap z from 0 to 1
    const float depth = ColorNormalize(GetImageColor(viewport.z_buffer, x, y)).z;
    if(depth < z1)
    {
        // values closer to 1 are further away from the camera
        ++g_pixels_behind_other_pixels;
        return;
    }

    ImageDrawPixel(&viewport.z_buffer, x, y, ColorFromNormalized({z1, z1, z1, 1.0f}));
    ImageDrawPixel(&viewport.color_buffer, x, y, ColorFromNormalized(color));
}

void Draw3dTriangle(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const Vector2* uv, const Vector4 add_color, const bool edges_only)
{
    const MyCamera& camera = viewport.camera;
    Vector3 normal = (a.normal + b.normal + c.normal) / 3.0f;

    const Vector3 look_at_direction = camera.lookAt - camera.position;
    const bool is_backfacing = Vector3DotProduct(normal, look_at_direction) >= 0.0f;
    if(is_backfacing)
    {
        ++g_backfacing_triangles;
        return;
    }

    const auto facingLightFactor = Clamp(-Vector3DotProduct(g_main_light.direction, normal), 0.2f, 1);
    Vector4 light_color = Vector4Scale(g_main_light.color, facingLightFactor);
    light_color.w = 1.0f;

    Vector4 a_screen = Vector4{a.position.x, a.position.y, a.position.z, 1.0f} * camera.worldToScreenSpace;
    Vector4 b_screen = Vector4{b.position.x, b.position.y, b.position.z, 1.0f} * camera.worldToScreenSpace;
    Vector4 c_screen = Vector4{c.position.x, c.position.y, c.position.z, 1.0f} * camera.worldToScreenSpace;
    a_screen /= a_screen.w;
    b_screen /= b_screen.w;
    c_screen /= c_screen.w;

    const Vertex a1{{a_screen.x, a_screen.y, a_screen.z}, {normal.x, normal.y, normal.z}, {a.uv.x, a.uv.y}};
    const Vertex b1{{b_screen.x, b_screen.y, b_screen.z}, {normal.x, normal.y, normal.z}, {b.uv.x, b.uv.y}};
    const Vertex c1{{c_screen.x, c_screen.y, c_screen.z}, {normal.x, normal.y, normal.z}, {c.uv.x, c.uv.y}};
    DrawTriangle(viewport, a1, b1, c1, uv, light_color, edges_only);
}

void DrawTriangle(Viewport& viewport, const Vertex& a, const Vertex& b, const Vertex& c, const Vector2* uv, const Vector4 add_color, const bool edges_only)
{
    if(edges_only)
    {
        ImageDrawLineV(&viewport.color_buffer, {a.position.x, a.position.y}, {b.position.x, b.position.y}, WHITE);
        ImageDrawLineV(&viewport.color_buffer, {b.position.x, b.position.y}, {c.position.x, c.position.y}, WHITE);
        ImageDrawLineV(&viewport.color_buffer, {c.position.x, c.position.y}, {a.position.x, a.position.y}, WHITE);
        return;
    }

    int x1 = (int)floor(a.position.x);
    int y1 = (int)floor(a.position.y);
    int x2 = (int)floor(b.position.x);
    int y2 = (int)floor(b.position.y);
    int x3 = (int)floor(c.position.x);
    int y3 = (int)floor(c.position.y);

    if(y1 == y2 && y2 == y3)
    {
        // all points are on the same line, no need to draw anything
        return;
    }

    if(y1 > y2)
    {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }
    
    if(y1 > y3)
    {
        std::swap(x1, x3);
        std::swap(y1, y3);
    }

    if(y2 > y3)
    {
        std::swap(x2, x3);
        std::swap(y2, y3);
    }
    
    const Vector3 a_to_b = b.position - a.position;
    const Vector3 a_to_c = c.position - a.position;
    // this is actually parallelogram area, but the ratio is the same between triangles and parallelograms
    // when calculating the barycentric coordinates
    const ftype triangle_area_recip = 1.0f / (a_to_c.x * a_to_b.y - a_to_c.y * a_to_b.x);

    const auto DrawTriangle = [&](Viewport& viewport, const int y_start, const int y_end, const int x_off_1, const int y_off_1, const int x_off_2, const int y_off_2, const ftype slope_1, const ftype slope_2){
        for(int y = y_start; y < y_end; ++y)
        {
            int start_x = (int)floor(slope_1 * (y - y_off_1)) + x_off_1;
            int end_x = (int)floor(slope_2 * (y - y_off_2)) + x_off_2;
    
            if(start_x > end_x)
            {
                std::swap(start_x, end_x);
            }
    
            for(int x = start_x; x <= end_x; ++x)
            {
                // calculate the barycentric coordinates
                const Vector2 a_to_p{x - a.position.x, y - a.position.y};
                const ftype alpha = (a_to_p.x * a_to_b.y - a_to_p.y * a_to_b.x) * triangle_area_recip;
                const ftype beta = -(a_to_p.x * a_to_c.y - a_to_p.y * a_to_c.x) * triangle_area_recip;
                const ftype gamma = 1 - alpha - beta;
                
                const ftype z = gamma * a.position.z + alpha * c.position.z + beta * b.position.z;
                DrawTextureSampledPixel(viewport, x, y, z, a.uv * gamma + c.uv * alpha + b.uv * beta, add_color);
                //DrawColorPixel(viewport, x, y, z, Vector4(gamma, beta, alpha, 1.0f));
            }
        }
    };

    if(y1 == y2)
    {
        // top edge is horizontal
        const ftype d_x3_y1 = (x3 - x1) / (ftype)(y3 - y1);
        const ftype d_x3_y2 = (x3 - x2) / (ftype)(y3 - y2);
        DrawTriangle(viewport, y2, y3, x1, y1, x2, y2, d_x3_y1, d_x3_y2);
    }
    else if(y2 == y3)
    {
        // bottom edge is horizontal
        const ftype d_x2_y1 = (x2 - x1) / (ftype)(y2 - y1);
        const ftype d_x3_y1 = (x3 - x1) / (ftype)(y3 - y1);
        DrawTriangle(viewport, y1, y2, x1, y1, x1, y1, d_x2_y1, d_x3_y1);
    }
    else
    {
        const ftype d_x2_y1 = (x2 - x1) / (ftype)(y2 - y1);
        const ftype d_x3_y1 = (x3 - x1) / (ftype)(y3 - y1);
        const ftype d_x3_y2 = (x3 - x2) / (ftype)(y3 - y2);
        DrawTriangle(viewport, y1, y2, x1, y1, x1, y1, d_x2_y1, d_x3_y1);
        DrawTriangle(viewport, y2, y3, x1, y1, x2, y2, d_x3_y1, d_x3_y2);
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

Vector2 GetSmoothedMouseMove(const int button)
{
    const Vector2 mouse_delta = IsMouseButtonDown(button) ? GetMouseDelta() : Vector2{0.0f, 0.0f};
    const Vector2 glm_delta = {-mouse_delta.x, mouse_delta.y};
    const Vector2 position = GetMousePosition();
    return position.x < g_ui_zone.x && position.y < g_ui_zone.y ? Vector2{0.0f, 0.0f} : glm_delta;
}

Vector2 GetScreenResizeFactor()
{
    static int last_screen_width = GetScreenWidth();
    static int last_screen_height = GetScreenHeight();
    const ftype width_change_factor = GetScreenWidth() / (ftype)last_screen_width;
    const ftype height_change_factor = GetScreenHeight() / (ftype)last_screen_height;
    last_screen_width = GetScreenWidth();
    last_screen_height = GetScreenHeight();
    return {width_change_factor, height_change_factor};
}

Matrix ClipToScreenSpaceMatrix(const Viewport& viewport)
{
    const ftype x = (ftype)viewport.transform.x;
    const ftype y = (ftype)viewport.transform.y;
    const ftype viewport_half_width = viewport.transform.z * 0.5f;
    const ftype viewport_half_height = viewport.transform.w * 0.5f;
    const Vector4 column1{viewport_half_width, 0, 0, 0};
    const Vector4 column2{0, -viewport_half_height, 0, 0};
    const Vector4 column3{0, 0, 1, 0};
    const Vector4 column4{x + viewport_half_width, y + viewport_half_height, 0, 1};
    return Mat4(column1, column2, column3, column4);
}

Matrix ProjectionMatrix(const Viewport& viewport)
{
    const MyCamera& camera = viewport.camera;
    return camera.is_orthographic 
        ? OrthographicProjectionMatrix(camera.fov, camera.aspect, camera.near_plane, camera.far_plane) 
        : PerspectiveProjectionMatrix(camera.fov, camera.aspect, camera.near_plane, camera.far_plane);
}

Matrix OrthographicProjectionMatrix(const ftype fov, const ftype aspect, const ftype near, const ftype far)
{
    return MatrixOrtho(-fov * aspect, fov * aspect, -fov, fov, near, far);
}

Matrix PerspectiveProjectionMatrix(const ftype fov, const ftype aspect, const ftype near, const ftype far)
{
    return MatrixPerspective(fov, aspect, near, far);
}

Matrix Mat4(const Vector4 column1, const Vector4 column2, const Vector4 column3, const Vector4 column4)
{
    const Vector4& c1 = column1;
    const Vector4& c2 = column2;
    const Vector4& c3 = column3;
    const Vector4& c4 = column4;
    return Matrix{
        //       row1  row2  row3  row4
        /*Col 1*/c1.x, c1.y, c1.z, c1.w,
        /*Col 2*/c2.x, c2.y, c2.z, c2.w,
        /*Col 3*/c3.x, c3.y, c3.z, c3.w,
        /*Col 4*/c4.x, c4.y, c4.z, c4.w
    };
}

bool IsTopLeftOfTriangle(const Vector2 from, const Vector2 to)
{
    const Vector2 a_to_b = to - from;
    const bool is_flat_edge = a_to_b.y == 0 && a_to_b.x < 0;
    const bool is_left_edge = a_to_b.y > 0;
    return is_flat_edge || is_left_edge;
}