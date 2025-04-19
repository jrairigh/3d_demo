# 3d Software Graphics Pipeline
An entirely CPU based 3d renderer with these features:
- Orthographic and perspective projections
- Depth buffer
- Texture mapping
- Directional light
- .OBJ Support

## Goal
Purely an educational project to better grasp modern 3D graphics pipeline. I'm specifically focused on black box parts handled by GPU like rasterization.

## How to Use
- Mouse wheel zooms in/out
- Left click and drag rotates camera around origin
- Right click and drag rotates the scene directional light
- Space bar changes between orthographic and perspective projection
- Z key draws the depth buffer to the screen
- W key draws the triangles of the meshes
- S key shows performance metrics
- Esc key quits application

## Future Enhancements
- Perspective correct texture mapping
- Clip triangles to screen boundaries
- Fixed floating point math for better subpixel accuracy
- Use software threads to render a smaller portion of full screen