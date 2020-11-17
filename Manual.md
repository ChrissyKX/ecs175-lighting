#### ECS 175 Programming 3 Manual

Author: Xiyu Zhang   SID: 916501253



##### Interaction

- Use ./run_p3 [filename.obj] to specify input file
- Press G to enable/disable GUI
- Drag on the screen to see arcball viewing effect.
- To enable orthographic projection, check the "Enbale Orthographic projection" box. On enabled, the window is divded into four quadrants by white dividers to assist viewing. Arcball functions normally in this mode.
  - Projection onto xy-plane is on the top-left corner.
  - Projection onto xz-plane is on the bottom-left corner.
  - Projection onto yz-plane is on the top-right corner.
- To enable painter's algorithm, check the "Enbale Painter's Algorithm" box.
- To enable Half-toning, check the "Enbale Half-toning" box.
- To set parameters in Phong Lighting Model, open the "Phong Lighting Model Parameters"drop down menu.
  - In "Intensities" subsection, type in positive float input for ambient and light source intensity. 
  - In "Reflection  Coefficients" subsection, type in positive **normalized** float input for ambient, diffuse, and specular reflection coefficients.
  - In "Position of Light Source", type in x, y, z float coordinates input of the light source.
  - In "Phong Constant" subsection, either type in integer input or click on "+"/"-" .



##### Implementation Details

- Phong Ligiting Model: in shading_vertex.glsl

- Painter's Algorithm: `PainterAlgorithm(mat4 mv)` in main.cpp line 249. I added a vector `painter_tri_order` of `std::pair` in `TriangleArrayObjects::Mesh` to store the (index of trianlge, minimal z depth of the vertices of a triangle) pairs for the triangles in the sorted order based on their minimal z depth. I modified `TriangleArrayObjects::Render` that when in painter's algorithm mode, it uses the indices in `painter_tri_order` to fetch and render one triangle from `TriangleArrayObjects::Mesh.vertices` at a time.  

  Note: `BindAndRender()` in main.cpp line 288 wraps the code for rendering objects when enabling/disabling painter's algorithm.

- Half-toning: in main.cpp `main()` from line 431 to line 543. 

  - First pass shaders: shading_vertex.glsl, shading_fragment.glsl
  - Second pass shaders: shader_passthrough.glsl, shader_half_toning.glsl

  I first rendered the figure into a N/3 * M/3 texture in the first rendering pass where N, M is the width and height of the window. In the second rendering pass, specifically in shader_half_toning.glsl, I sampled from this texture to get the color of a mega pixel. Then, I compute the number of red, green, blue physical pixels to be turned on in a 3 * 3 mega pixel. The scheme I used for arranging these colored pixels in a 3 * 3 grid is from left to right and bottom to top, and render red pixels first, then green, then blue ones, and finally black(off) ones if there are pixels left in the grid. I compute the current pixel's position in a 3 * 3 grid and fetch its color using this scheme. 

  

##### Input Files

There are 6 test input files in /data folder, which are:

cube.obj with 1 object

suzanne.obj with 1 object

two_shapes.obj with 2 objects

three_shpes.obj with 3 objects

four_shapes.obj with 4 objects

five_shapes.obj with 5 objects



