# 3d Software Graphics Pipeline
An entirely CPU based 3d renderer with these features:
- Orthographic and perspective projections
- Depth buffer
- Affine and perspective correct texture mapping
- Optimizations:
  - Culling triangles outside of view frustrum and facing away from camera

## How to Use
- Mouse wheel zooms in/out
- Left click and drag rotates camera around origin
- Space bar changes between orthographic and perspective projection
- Z key draws the depth buffer to the screen
- W key draws the triangles of the meshes
- Tab key switches between drawing different meshes
- S key shows performance metrics
- Esc key quits application

## Improvements
- Upper bounds of 53 fps at a 400x250 resolution (release build, every pixel drawn to on screen each frame)
- Clip triangles to screen boundaries
- Cull back facing triangles
- Smarter triangle drawing algorithm
- Perspective correct texture mapping
- Use threads to render a smaller portion of full screen