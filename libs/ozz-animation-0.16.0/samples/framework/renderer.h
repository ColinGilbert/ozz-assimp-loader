//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#ifndef OZZ_SAMPLES_FRAMEWORK_RENDERER_H_
#define OZZ_SAMPLES_FRAMEWORK_RENDERER_H_

#include "ozz/base/platform.h"
#include "ozz/base/span.h"

namespace ozz {
namespace animation {
class Skeleton;
}
namespace math {
struct Float4x4;
struct Float3;
struct Box;
}  // namespace math
namespace sample {

// Sample framework mesh type.
struct Mesh;

// Defines render Color structure.
struct Color {
  float r, g, b, a;
};

// Color constants.
static const Color kRed = {1, 0, 0, 1};
static const Color kGreen = {0, 1, 0, 1};
static const Color kBlue = {0, 0, 1, 1};
static const Color kWhite = {1, 1, 1, 1};
static const Color kYellow = {1, 1, 0, 1};
static const Color kMagenta = {1, 0, 1, 1};
static const Color kCyan = {0, 1, 1, 1};
static const Color kGrey = {.5f, .5f, .5f, 1};
static const Color kBlack = {.5f, .5f, .5f, 1};

// Defines renderer abstract interface.
class Renderer {
 public:
  // Declares a virtual destructor to allow proper destruction.
  virtual ~Renderer() {}

  // Initializes he renderer.
  // Return true on success.
  virtual bool Initialize() = 0;

  // Renders coordinate system axes: X in red, Y in green and W in blue.
  // Axes size is given by _scale argument.
  virtual bool DrawAxes(const ozz::math::Float4x4& _transform) = 0;

  // Renders a square grid of _cell_count cells width, where each square cell
  // has a size of _cell_size.
  virtual bool DrawGrid(int _cell_count, float _cell_size) = 0;

  // Renders a skeleton in its rest pose posture.
  virtual bool DrawSkeleton(const animation::Skeleton& _skeleton,
                            const ozz::math::Float4x4& _transform,
                            bool _draw_joints = true) = 0;

  // Renders a skeleton at the specified _position in the posture given by model
  // space _matrices.
  // Returns true on success, or false if _matrices range does not match with
  // the _skeleton.
  virtual bool DrawPosture(const animation::Skeleton& _skeleton,
                           ozz::span<const ozz::math::Float4x4> _matrices,
                           const ozz::math::Float4x4& _transform,
                           bool _draw_joints = true) = 0;

  // Renders points.
  // _sizes and _colors must be either of ize 1 or equal to _positions' size.
  // If _screen_space is true, then points size is fixed in screen-space,
  // otherwise it changes with screen depth.
  virtual bool DrawPoints(const ozz::span<const float>& _positions,
                          const ozz::span<const float>& _sizes,
                          const ozz::span<const Color>& _colors,
                          const ozz::math::Float4x4& _transform,
                          bool _screen_space = false) = 0;

  // Renders a wired box at a specified location.
  virtual bool DrawBoxIm(const ozz::math::Box& _box,
                         const ozz::math::Float4x4& _transform,
                         const Color& _color) = 0;

  // Renders a box at a specified location.
  // The 2 slots of _colors array respectively defines color of the filled
  // faces and color of the box outlines.
  virtual bool DrawBoxIm(const ozz::math::Box& _box,
                         const ozz::math::Float4x4& _transform,
                         const Color _colors[2]) = 0;

  // Renders shaded boxes at specified locations.
  virtual bool DrawBoxShaded(const ozz::math::Box& _box,
                             ozz::span<const ozz::math::Float4x4> _transforms,
                             const Color& _color) = 0;

  // Renders a sphere at a specified location.
  virtual bool DrawSphereIm(float _radius,
                            const ozz::math::Float4x4& _transform,
                            const Color& _color) = 0;

  // Renders shaded spheres at specified locations.
  virtual bool DrawSphereShaded(
      float _radius, ozz::span<const ozz::math::Float4x4> _transforms,
      const Color& _color) = 0;

  struct Options {
    bool triangles;  // Show triangles mesh.
    bool texture;    // Show texture (default checkered texture).
    bool vertices;   // Show vertices as points.
    bool normals;    // Show normals.
    bool tangents;   // Show tangents.
    bool binormals;  // Show binormals, computed from the normal and tangent.
    bool colors;     // Show vertex colors.
    bool wireframe;  // Show vertex colors.
    bool skip_skinning;  // Show texture (default checkered texture).

    Options()
        : triangles(true),
          texture(false),
          vertices(false),
          normals(false),
          tangents(false),
          binormals(false),
          colors(false),
          wireframe(false),
          skip_skinning(false) {}

    Options(bool _triangles, bool _texture, bool _vertices, bool _normals,
            bool _tangents, bool _binormals, bool _colors, bool _wireframe,
            bool _skip_skinning)
        : triangles(_triangles),
          texture(_texture),
          vertices(_vertices),
          normals(_normals),
          tangents(_tangents),
          binormals(_binormals),
          colors(_colors),
          wireframe(_wireframe),
          skip_skinning(_skip_skinning) {}
  };

  // Renders a skinned mesh at a specified location.
  virtual bool DrawSkinnedMesh(const Mesh& _mesh,
                               const span<math::Float4x4> _skinning_matrices,
                               const ozz::math::Float4x4& _transform,
                               const Options& _options = Options()) = 0;

  // Renders a mesh at a specified location.
  virtual bool DrawMesh(const Mesh& _mesh,
                        const ozz::math::Float4x4& _transform,
                        const Options& _options = Options()) = 0;

  // Renders a lines. Vertices 0 and 1 are considered a line. Vertices 2 and 3
  // are considered a line. And so on. If the user specifies a non-even number
  // of vertices, then the extra vertex is ignored.
  virtual bool DrawLines(ozz::span<const math::Float3> _vertices,
                         const Color& _color,
                         const ozz::math::Float4x4& _transform) = 0;

  // Renders a line strip. Adjacent vertices are considered lines. Thus, if you
  // pass n vertices, you will get n-1 lines. If the user only specifies 1
  // vertex, the drawing command is ignored (still valid).
  virtual bool DrawLineStrip(ozz::span<const math::Float3> _vertices,
                             const Color& _color,
                             const ozz::math::Float4x4& _transform) = 0;

  // Renders vectors, defined by their starting point and a direction.
  virtual bool DrawVectors(ozz::span<const float> _positions,
                           size_t _positions_stride,
                           ozz::span<const float> _directions,
                           size_t _directions_stride, int _num_vectors,
                           float _vector_length, const Color& _color,
                           const ozz::math::Float4x4& _transform) = 0;

  // Compute binormals from normals and tangents, before displaying them.
  virtual bool DrawBinormals(
      ozz::span<const float> _positions, size_t _positions_stride,
      ozz::span<const float> _normals, size_t _normals_stride,
      ozz::span<const float> _tangents, size_t _tangents_stride,
      ozz::span<const float> _handenesses, size_t _handenesses_stride,
      int _num_vectors, float _vector_length, const Color& _color,
      const ozz::math::Float4x4& _transform) = 0;
};
}  // namespace sample
}  // namespace ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_RENDERER_H_
