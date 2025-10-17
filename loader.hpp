// This loader loads skeletal meshes from Assimp into ozz-animation format, and
// loads proper mesh data to use alongside it in an engine neutral format.
// Enjoy. MIT license. Colin Gilbert

// TODO: Options via config file

#include <array>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>
#include <iostream>
#include <lemon/list_graph.h>
#include <set>
#include <unordered_map>
#include <vector>

#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

#include <boost/filesystem.hpp>

#include <iostream>
#include <map>
#include <memory>
#include <string>

class loader {
public:
//   struct mesh_vertex {
//     mesh_vertex()
//         : position({0.0f, 0.0f, 0.0f}), normal({0.0f, 0.0f, 0.0f}),
//           uv({0.0f, 0.0f}), bone_names({"", "", "", ""}),
//           bone_indices({0, 0, 0, 0}), bone_weights({0.0f, 0.0f, 0.0f, 0.0f}) {}
//     std::array<float, 3> position, normal;
//     std::array<float, 2> uv;
//     std::array<std::string, 4> bone_names;
//     std::array<size_t, 4> bone_indices;
//     std::array<float, 4> bone_weights;
//   };

  struct mesh {
    std::array<float, 3> translation, scale, dimensions;
    std::array<float, 4> rotation;
    std::string name;
    std::vector<std::array<float, 3>> positions, normals;
    std::vector<std::array<float, 2>> uvs;
    std::vector<std::array<std::string, 4>>
        vert_bone_names; // Used to calculate bone indices and weights
    std::vector<std::array<size_t, 4>> bone_indices;
    std::vector<std::array<float, 4>> bone_weights;
    std::vector<uint32_t> indices;
    std::vector<std::string> bone_names;
    unsigned int material_index;
  };
  enum texture_type { NONE = 0, DIFFUSE = 1, NORMAL = 2, SPECULAR = 3};
  struct material {
    std::string texture_path;
    texture_type tex_type;
  };

  class hierarchy {
  public:
    hierarchy()
        : _translation(_graph), _scale(_graph), _rotation(_graph),
          _name(_graph) {}

    void init(const aiScene *scene, const std::set<std::string> &bone_names);

    ozz::animation::offline::RawSkeleton make_raw_skeleton();

    void print_info();

  protected:
    std::vector<lemon::ListDigraph::Node> find_roots();

    void recursive_build(aiNode *current,
                         const std::set<std::string> &bone_names);

    void link(aiNode *parent, aiNode *child);

    lemon::ListDigraph::Node add(aiNode *assimp_node);

    void recursive_ozz_helper(
        const lemon::ListDigraph::Node &n,
        ozz::animation::offline::RawSkeleton::Joint &caller_joint,
        size_t index);

    void recursive_print(lemon::ListDigraph::Node n);

    lemon::ListDigraph _graph;
    lemon::ListDigraph::NodeMap<std::array<float, 3>> _translation, _scale;
    lemon::ListDigraph::NodeMap<std::array<float, 4>> _rotation;
    lemon::ListDigraph::NodeMap<std::string> _name;

    // For easy access
    std::map<aiNode *, lemon::ListDigraph::Node> nodes;
  };

  bool load(const aiScene *scene, const std::string &name);

  std::string get_output_path() const { return output_pathname; }

protected:
  std::vector<loader::mesh> meshes;
  std::vector<loader::material> materials;
  std::string output_pathname;
};
