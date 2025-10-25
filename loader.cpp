#include "loader.hpp"

#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <kj/array.h>
#include <kj/io.h>

#include "model3d_schema.capnp.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

void loader::hierarchy::init(const aiScene *scene,
                             const std::set<std::string> &bone_names) {
  for (std::string s : bone_names) {
    aiNode *ai_node = scene->mRootNode->FindNode(s.c_str());
    lemon::ListDigraph::Node lemon_node = add(ai_node);
    nodes.insert(std::make_pair(ai_node, lemon_node));
  }

  recursive_build(scene->mRootNode, bone_names);
}

ozz::animation::offline::RawSkeleton loader::hierarchy::make_raw_skeleton() {
  std::vector<lemon::ListDigraph::Node> roots = find_roots();

  std::cout << "Skeleton roots: ";
  for (lemon::ListDigraph::Node r : roots) {
    std::cout << _name[r] << " ";
  }
  std::cout << std::endl;

  size_t num_roots = roots.size();
  ozz::animation::offline::RawSkeleton raw_skeleton;
  raw_skeleton.roots.resize(num_roots);

  std::cout << "About to start building skeleton." << std::endl;
  for (size_t i = 0; i < num_roots; i++) {
    lemon::ListDigraph::Node r = roots[i];
    ozz::animation::offline::RawSkeleton::Joint &root_joint =
        raw_skeleton.roots[i];
    // std::cout << "Starting recursive build from root " << _name[r] <<
    // std::endl;
    recursive_ozz_helper(r, root_joint, i);
  }
  return raw_skeleton;
}

void loader::hierarchy::print_info() {
  size_t count = 0;

  for (lemon::ListDigraph::NodeIt it(_graph); it != lemon::INVALID; ++it) {
    count++;
    lemon::ListDigraph::Node n(it);
    std::cout << "Node " << count << " name = " << _name[n] << std::endl;
  }

  std::cout << "Node Count = " << count << std::endl;

  // recursive_print();
}

void loader::hierarchy::recursive_ozz_helper(
    const lemon::ListDigraph::Node &n,
    ozz::animation::offline::RawSkeleton::Joint &caller_joint, size_t index) {
  size_t children_count = 0;

  caller_joint.name = _name[n].c_str();

  caller_joint.transform.translation = ozz::math::Float3(
      _translation[n][0], _translation[n][1], _translation[n][2]);
  caller_joint.transform.rotation = ozz::math::Quaternion(
      _rotation[n][0], _rotation[n][1], _rotation[n][2], _rotation[n][3]);
  caller_joint.transform.scale =
      ozz::math::Float3(_scale[n][0], _scale[n][1], _scale[n][2]);

  for (lemon::ListDigraph::OutArcIt it(_graph, n); it != lemon::INVALID; ++it) {
    children_count++;
  }

  // std::cout << "Adding joint to skeleton. Name = " << _name[n]
  //          << ", number of children = " << children_count << std::endl;

  caller_joint.children.resize(children_count);

  size_t current_index = 0;

  for (lemon::ListDigraph::OutArcIt it(_graph, n); it != lemon::INVALID; ++it) {
    lemon::ListDigraph::Arc arc(it);
    lemon::ListDigraph::Node next_node = _graph.target(it);

    ozz::animation::offline::RawSkeleton::Joint &current_joint =
        caller_joint.children[current_index];
    recursive_ozz_helper(next_node, current_joint, current_index);
    current_index++;
  }
}

std::vector<lemon::ListDigraph::Node> loader::hierarchy::find_roots() {
  std::vector<lemon::ListDigraph::Node> roots;
  for (lemon::ListDigraph::NodeIt it(_graph); it != lemon::INVALID; ++it) {
    size_t count = 0;
    lemon::ListDigraph::Node n(it);

    for (lemon::ListDigraph::InArcIt e(_graph, n); e != lemon::INVALID; ++e) {
      ++count;
    }

    if (count == 0) {
      roots.push_back(n);
    }
  }
  return roots;
}

void loader::hierarchy::recursive_build(
    aiNode *current, const std::set<std::string> &bone_names) {
  std::string current_node_name = std::string(current->mName.C_Str());
  for (size_t i = 0; i < current->mNumChildren; i++) {
    aiNode *child = current->mChildren[i];

    if (bone_names.find(current_node_name) != bone_names.end()) {
      std::string child_name =
          std::string(current->mChildren[i]->mName.C_Str());

      if (bone_names.find(child_name) != bone_names.end()) {
        //  std::cout << "Linking parent " << current_node_name << " and child "
        //          << child_name << std::endl;
        link(current, child);
      }
    }
    recursive_build(child, bone_names);
  }
}

void loader::hierarchy::link(aiNode *parent, aiNode *child) {
  lemon::ListDigraph::Node p = nodes.find(parent)->second;
  lemon::ListDigraph::Node c = nodes.find(child)->second;
  _graph.addArc(p, c);
}

lemon::ListDigraph::Node loader::hierarchy::add(aiNode *assimp_node) {
  std::string name = std::string(assimp_node->mName.C_Str());

  aiMatrix4x4 mat = assimp_node->mTransformation;
  aiVector3D scale, trans;
  aiQuaternion rot;

  mat.Decompose(scale, rot, trans);

  lemon::ListDigraph::Node graph_node = _graph.addNode();

  _translation[graph_node] = {trans.x, trans.y, trans.z};
  _scale[graph_node] = {scale.x, scale.y, scale.z};
  _rotation[graph_node] = {rot.x, rot.y, rot.z, rot.w};
  _name[graph_node] = name;

  return graph_node;
}

void loader::hierarchy::recursive_print(lemon::ListDigraph::Node n) {
  std::cout << "Node name = " << _name[n] << std::endl;

  for (lemon::ListDigraph::OutArcIt it(_graph, n); it != lemon::INVALID; ++it) {
    lemon::ListDigraph::Arc arc(it);
    lemon::ListDigraph::Node next_node = _graph.target(it);
    recursive_print(next_node);
  }
}

bool loader::load(const aiScene *scene, const std::string &name) {
  if (!scene) {
    std::cout << "[Mesh] load(" << name << ") - cannot open" << std::endl;
    return false;
  }
  bool has_bones = false;
  std::unordered_map<std::string, size_t> joint_indices;
  size_t num_joints = 0;
  std::set<std::string> scene_bone_names;
  // aiNode* scene_root = scene->mRootNode;

  for (size_t mesh_num = 0; mesh_num < scene->mNumMeshes; mesh_num++) {
    const aiMesh *mesh_data = scene->mMeshes[mesh_num];
    loader::mesh temp_mesh;
    temp_mesh.name = std::string(mesh_data->mName.C_Str());
    std::cout << "Attempting to obtain data for mesh " << std::endl;
    size_t num_verts = mesh_data->mNumVertices;
    size_t num_faces = mesh_data->mNumFaces;
    size_t num_bones = mesh_data->mNumBones;
    has_bones = mesh_data->HasBones();
    bool has_normals = mesh_data->HasNormals();
    bool has_texcoords = mesh_data->HasTextureCoords(0);

    temp_mesh.positions.resize(num_verts);
    if (has_normals) {
      temp_mesh.normals.resize(num_verts);
    }
    if (has_texcoords) {
      temp_mesh.uvs.resize(num_verts);
    }
    std::cout << "Mesh " << temp_mesh.name << " (" << mesh_num << ") has "
              << num_verts << " verts and " << num_bones << " bones. Normals? "
              << has_normals << std::endl;

    std::array<float, 3> min_extents, max_extents;

    for (size_t n = 0; n < num_verts; ++n) {
      aiVector3D pt = mesh_data->mVertices[n];

      // mesh_vertex v;
      temp_mesh.positions[n][0] = pt[0];
      temp_mesh.positions[n][1] = pt[1];
      temp_mesh.positions[n][2] = pt[2];

      min_extents[0] = std::min(min_extents[0], pt[0]);
      min_extents[1] = std::min(min_extents[1], pt[1]);
      min_extents[2] = std::min(min_extents[2], pt[2]);
      max_extents[0] = std::max(max_extents[0], pt[0]);
      max_extents[1] = std::max(max_extents[1], pt[1]);
      max_extents[2] = std::max(max_extents[2], pt[2]);

      if (has_normals) {
        aiVector3D normal = mesh_data->mNormals[n];
        temp_mesh.normals[n][0] = normal[0];
        temp_mesh.normals[n][1] = normal[1];
        temp_mesh.normals[n][2] = normal[2];
      }
      if (has_texcoords) {
        aiVector3D uv = mesh_data->mTextureCoords[0][n];
        temp_mesh.uvs[n][0] = uv.x;
        temp_mesh.uvs[n][1] = uv.y;
      }
    }

    std::array<float, 3> temp_dims;
    for (size_t i = 0; i < 3; ++i) {
      temp_dims[i] = max_extents[i] - min_extents[i];
    }

    temp_mesh.dimensions = temp_dims;

    for (size_t face_id = 0; face_id < num_faces; ++face_id) {
      aiFace face = mesh_data->mFaces[face_id];
      if (face.mNumIndices == 3) {
        temp_mesh.indices.push_back(face.mIndices[0]);
        temp_mesh.indices.push_back(face.mIndices[1]);
        temp_mesh.indices.push_back(face.mIndices[2]);
      }
      // else {
      //   std::cout<< "Found faces with " << face.mNumIndices << " indicce" <<
      //   std::endl;
      // }
    }

    if (has_bones) {
      temp_mesh.vert_bone_names.resize(num_verts);
      // temp_mesh.bone_names.reserve(num_bones);
      temp_mesh.bone_indices.resize(num_verts);
      temp_mesh.bone_weights.resize(num_verts);

      for (size_t n = 0; n < num_bones; ++n) {
        aiBone *bone_data = mesh_data->mBones[n];
        std::string temp_bone_name = std::string(bone_data->mName.C_Str());
        // TODO: Remove
        std::cout << temp_bone_name << " " << temp_mesh.bone_names.size()
                  << std::endl;
        temp_mesh.bone_names.push_back(temp_bone_name);

        // Make sure to keep track of every bone in the scene (in order to
        // account for multimesh models)
        scene_bone_names.insert(temp_bone_name);
        // Store the bone names and weights in the vert data.
        for (uint32_t i = 0; i < bone_data->mNumWeights; ++i) {
          size_t bone_vertex_id = bone_data->mWeights[i].mVertexId;

          for (size_t j = 0; j < 4; ++j) {
            if (temp_mesh.bone_weights[bone_vertex_id][j] == 0.0) {
              temp_mesh.vert_bone_names[bone_vertex_id][j] = temp_bone_name;
              temp_mesh.bone_weights[bone_vertex_id][j] =
                  bone_data->mWeights[i].mWeight;
              break;
            }
          }
        }
      }
      temp_mesh.material_index = mesh_data->mMaterialIndex;
    }
    meshes.push_back(temp_mesh);
  }

  std::cout << "Total of " << meshes.size() << " meshes in file " << name << "."
            << std::endl;
  if (has_bones) {
    loader::hierarchy bone_hierarchy;

    bone_hierarchy.init(scene, scene_bone_names);

    bone_hierarchy.print_info();

    ozz::animation::offline::RawSkeleton raw_skel =
        bone_hierarchy.make_raw_skeleton();

    if (!raw_skel.Validate()) {
      std::cout << "Skeleton validation failed!" << std::endl;
      return false;
    } else {
      std::cout << "Skeleton validation success!" << std::endl;
    }

    ozz::animation::offline::SkeletonBuilder skel_builder;
    std::unique_ptr<ozz::animation::Skeleton,
                    ozz::Deleter<ozz::animation::Skeleton>>
        runtime_skel = skel_builder(raw_skel);

    std::string output_base_pathname = "./output/";

    try {
      boost::filesystem::create_directory(
          boost::filesystem::path(output_base_pathname));
    } catch (std::exception &e) {
      std::cout << "Could not create base path: " << output_base_pathname;
      return false;
    }
    output_pathname = output_base_pathname + name;

    try {
      boost::filesystem::create_directory(
          boost::filesystem::path(output_pathname));
    }

    catch (std::exception &e) {
      std::cout << "Could not create path: " << output_pathname;
      return false;
    }

    // Most of the time, the user will want to use the runtime skeleton, but at
    // this point we give option for both.
    std::ostringstream output_raw_skel_filename;
    output_raw_skel_filename << output_pathname << "/raw-skeleton.ozz";
    std::string filename = output_raw_skel_filename.str();
    std::cout << "Outputting raw skeleton to " << output_raw_skel_filename.str()
              << std::endl;
    ozz::io::File output_raw_skel_file(output_raw_skel_filename.str().c_str(),
                                       "wb");
    ozz::io::OArchive raw_skel_archive(&output_raw_skel_file);
    raw_skel_archive << raw_skel;

    std::ostringstream output_runtime_skel_filename;
    output_runtime_skel_filename << output_pathname << "/runtime-skeleton.ozz";
    std::cout << "Outputting runtime skeleton to "
              << output_runtime_skel_filename.str() << std::endl;
    ozz::io::File output_runtime_skel_file(
        output_runtime_skel_filename.str().c_str(), "wb");
    ozz::io::OArchive runtime_skel_archive(&output_runtime_skel_file);
    runtime_skel_archive << *runtime_skel;

    // This bit of code allows the animation to use the skeleton indices from
    // the ozz skeleton structure.
    // TODO: Find out if necessary.
    ozz::span<const char *const> joint_names = runtime_skel->joint_names();
    num_joints = runtime_skel->num_joints();

    std::vector<std::string> joint_names_str;

    for (size_t i = 0; i < num_joints; ++i) {
      std::string s = std::string(joint_names[i]);
      joint_indices.insert(std::make_pair(s, i));
      joint_names_str.push_back(s);
    }

    // std::cout << "Displaying ozz skeleton joint names and indices" <<
    // std::endl;

    //   for (auto it = joint_indices.begin(); it != joint_indices.end(); it++)
    //   {
    //     std::cout << "Name = " << it->first << ", index = " << it->second
    //               << std::endl;
    //   }
    // Now, get the bone names from each vertex and insert the matching bone
    // indices.
    for (size_t mesh_index = 0; mesh_index < meshes.size(); ++mesh_index) {
      loader::mesh m = meshes[mesh_index];
      m.bone_names = joint_names_str;
      std::cout << std::endl;
      for (size_t vert_index = 0; vert_index < m.positions.size();
           ++vert_index) {
        // mesh_vertex v = m.vertices[vert_index];
        if (m.vert_bone_names.size() == m.bone_indices.size() ==
            m.positions.size()) {
          for (size_t i = 0; i < m.vert_bone_names[vert_index].size(); ++i) {
            std::string s = m.vert_bone_names[vert_index][i];
            if (!s.empty()) {
              auto it = joint_indices.find(s);
              if (it != joint_indices.end()) {
                m.bone_indices[vert_index][i] = joint_indices.find(s)->second;
                // std::cout << "Found index " << v.bone_indices[i]
                //           << " for bone name " << v.bone_names[i] <<
                //           std::endl;
              } else {
                std::cout << "ERROR! Could not find bone index for " << s
                          << " in joint indices map!!!" << std::endl;
                return false;
              }
            }
          }
        }
      }
      meshes[mesh_index] = m;
    }
  }
  // Read in materials
  const bool has_materials = scene->HasMaterials();
  if (has_materials) {
    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
      loader::material material;
      const auto mat = scene->mMaterials[i];
      aiString texturePath;
      if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) ==
          AI_SUCCESS) {
        material.diffuse_texture_path = std::string(texturePath.C_Str());
      }
      if (mat->GetTexture(aiTextureType_NORMALS, 0, &texturePath) ==
          AI_SUCCESS) {
        material.normals_texture_path = std::string(texturePath.C_Str());
      }
      if (mat->GetTexture(aiTextureType_SPECULAR, 0, &texturePath) ==
          AI_SUCCESS) {
        material.specular_texture_path = std::string(texturePath.C_Str());
      }
      material.name = std::string(mat->GetName().C_Str());
      materials.push_back(material);

      // std::cout << texturePath.C_Str() << std::endl;
    }
  }

  // CapnProto builders
  capnp::MallocMessageBuilder message;
  Model::Builder model_output = message.initRoot<Model>();
  capnp::List<Mesh>::Builder meshes_output =
      model_output.initMeshes(meshes.size());
  capnp::List<Material>::Builder materials_output =
      model_output.initMaterials(materials.size());

  // Create the mesh in capnproto
  for (size_t i = 0; i < meshes.size(); ++i) {
    Mesh::Builder m = meshes_output[i];
    m.setTranslationX(meshes[i].translation[0]);
    m.setTranslationY(meshes[i].translation[1]);
    m.setTranslationZ(meshes[i].translation[2]);
    m.setScaleX(meshes[i].scale[0]);
    m.setScaleY(meshes[i].scale[1]);
    m.setScaleZ(meshes[i].scale[2]);
    m.setDimensionsX(meshes[i].dimensions[0]);
    m.setDimensionsY(meshes[i].dimensions[1]);
    m.setDimensionsZ(meshes[i].dimensions[2]);
    m.setRotationX(meshes[i].rotation[0]);
    m.setRotationY(meshes[i].rotation[1]);
    m.setRotationZ(meshes[i].rotation[2]);
    m.setRotationW(meshes[i].rotation[3]);
    m.setName(meshes[i].name);
    m.setMaterialIndex(meshes[i].material_index);

    capnp::List<Array3f>::Builder positions =
        m.initPositions(meshes[i].positions.size());
    for (size_t j = 0; j < meshes[i].positions.size(); ++j) {
      Array3f::Builder p = positions[j];
      p.setArray3fX(meshes[i].positions[j][0]);
      p.setArray3fY(meshes[i].positions[j][1]);
      p.setArray3fZ(meshes[i].positions[j][2]);
    }

    // capnp::List<size_t>::Builder indices =
    m.initIndices(meshes[i].indices.size());
    for (size_t j = 0; j < meshes[i].indices.size(); ++j) {

      m.getIndices().set(j, meshes[i].indices[j]);
    }

    // if (has_normals) {
    capnp::List<Array3f>::Builder normals =
        m.initNormals(meshes[i].normals.size());
    for (size_t j = 0; j < meshes[i].normals.size(); ++j) {
      Array3f::Builder n = normals[j];
      n.setArray3fX(meshes[i].normals[j][0]);
      n.setArray3fY(meshes[i].normals[j][1]);
      n.setArray3fZ(meshes[i].normals[j][2]);
    }
    //}

    // if (has_texcoords) {
    capnp::List<Array2f>::Builder texcoords = m.initUvs(meshes[i].uvs.size());
    for (size_t j = 0; j < meshes[i].uvs.size(); ++j) {
      Array2f::Builder t = texcoords[j];
      t.setArray2fX(meshes[i].uvs[j][0]);
      t.setArray2fY(meshes[i].uvs[j][1]);
    }
    // }

    // if (has_bones) {
    capnp::List<Array4u>::Builder bone_indices =
        m.initBoneIndices(meshes[i].bone_indices.size());
    for (size_t j = 0; j < meshes[i].bone_indices.size(); ++j) {
      Array4u::Builder bi = bone_indices[j];
      bi.setArray4uX(meshes[i].bone_indices[j][0]);
      bi.setArray4uY(meshes[i].bone_indices[j][1]);
      bi.setArray4uZ(meshes[i].bone_indices[j][2]);
      bi.setArray4uW(meshes[i].bone_indices[j][3]);
    }

    capnp::List<Array4f>::Builder bone_weights =
        m.initBoneWeights(meshes[i].bone_weights.size());
    for (size_t j = 0; j < meshes[i].bone_weights.size(); ++j) {
      Array4f::Builder bw = bone_weights[j];
      bw.setArray4fX(meshes[i].bone_weights[j][0]);
      bw.setArray4fY(meshes[i].bone_weights[j][1]);
      bw.setArray4fZ(meshes[i].bone_weights[j][2]);
      bw.setArray4fW(meshes[i].bone_weights[j][3]);
    }

    capnp::List<capnp::Text>::Builder bone_names =
        m.initBoneNames(meshes[i].bone_names.size());
    for (size_t j = 0; j < meshes[i].bone_names.size(); ++j) {
      m.getBoneNames().set(j, meshes[i].bone_names[j]);
    }
    // }
  }

  // Create the material buffer in capnproto
  for (size_t i = 0; i < materials.size(); ++i) {
    Material::Builder m = materials_output[i];
    m.setDiffuseTexturePath(materials[i].diffuse_texture_path);
    m.setNormalsTexturePath(materials[i].normals_texture_path);
    m.setSpecularTexturePath(materials[i].specular_texture_path);
  }

  // NOW WE WRITE OUR SERIALIZED MODEL!
  kj::VectorOutputStream output_stream;
  // And write our mesage to the output stream
  capnp::writePackedMessage(output_stream, message);
  // auto words = capnp::messageToFlatArray(message);
  // auto bytes = words.asBytes();
  const auto array = output_stream.getArray();
  auto iter = array.begin();
  std::cout << "Message size: " << array.size() << std::endl;

  std::vector<char> buffer;
  while (iter != array.end()) {
    buffer.push_back(*iter);
    ++iter;
  }

  std::ofstream output_file;
  output_file.open(output_pathname + "/model.bin", std::ios::binary);
  for (unsigned char c : buffer) {
    output_file.put(c);
  }

  std::cout << "Writing model file to " << output_pathname + "/model.bin"
            << std::endl;
  // std::cout << "Buffer size: " << buffer.size() << std::endl;

  output_file.close();

  if (has_bones) {
    std::vector<ozz::animation::offline::RawAnimation> raw_animations;

    size_t num_anims = scene->mNumAnimations;

    for (size_t anim_num = 0; anim_num < num_anims; ++anim_num) {
      aiAnimation *anim = scene->mAnimations[anim_num];
      double ticks = anim->mDuration;
      double ticks_per_sec = anim->mTicksPerSecond;
      size_t num_unfiltered_channels = anim->mNumChannels;

      if (ticks_per_sec < 0.0001) {
        ticks_per_sec = 1.0;
      }

      std::string anim_name = std::string(anim->mName.C_Str());
      std::cout << "Assimp animation " << anim_num + 1 << " out of "
                << num_anims << ". Name: " << anim_name << ". Ticks = " << ticks
                << ". Ticks per second = " << ticks_per_sec
                << ". Number of (unfiltered) channels = "
                << num_unfiltered_channels << std::endl;

      ozz::animation::offline::RawAnimation raw_animation;
      raw_animation.duration = ticks * ticks_per_sec;

      // Filter out the anim nodes that aren't bones.
      // TODO: Find out whether this is necessary or desirable
      std::set<std::tuple<size_t, aiNodeAnim *>> valid_channels;
      for (size_t num = 0; num < num_unfiltered_channels; ++num) {
        aiNodeAnim *anim_node = anim->mChannels[num];
        std::string anim_node_name = std::string(anim_node->mNodeName.C_Str());
        auto it = joint_indices.find(anim_node_name);
        if (it != joint_indices.end()) {
          size_t anim_node_skeleton_index = it->second;
          valid_channels.insert(
              std::make_tuple(anim_node_skeleton_index, anim_node));
          // std::cout << "Found node " << anim_node_name << " for animation
          // track
          // "
          //          << num << "." << std::endl;
        } else {
          // std::cout << "Could not find node " << anim_node_name
          //          << " required to build animation track " << num
          //          << ". Skipping." << std::endl;
          continue;
        }
      }

      raw_animation.tracks.resize(num_joints);
      for (std::tuple<size_t, aiNodeAnim *> chan : valid_channels) {
        size_t track_index = std::get<0>(chan);
        aiNodeAnim *anim_node = std::get<1>(chan);

        size_t num_translations = anim_node->mNumPositionKeys;
        size_t num_rotations = anim_node->mNumRotationKeys;
        size_t num_scales = anim_node->mNumScalingKeys;

        // std::cout << "Bone " << anim_node->mNodeName.C_Str()
        //           << ", skeleton index " << track_index << " - Inserting "
        //           << num_translations << " translations, " << num_rotations
        //           << " rotations, " << num_scales << " scales." << std::endl;

        for (size_t i = 0; i < num_translations; ++i) {
          // std::cout << "Inserting translation num " << i << ": ";
          aiVectorKey k = anim_node->mPositionKeys[i];
          double t = k.mTime;
          aiVector3D val = k.mValue;
          // std::cout << " time = " << t << ", (" << val.x << ", " << val.y <<
          // ",
          // "
          //          << val.z << ")" << std::endl;
          const ozz::animation::offline::RawAnimation::TranslationKey
              trans_key = {static_cast<float>(t),
                           ozz::math::Float3(static_cast<float>(val.x),
                                             static_cast<float>(val.y),
                                             static_cast<float>(val.z))};
          raw_animation.tracks[track_index].translations.push_back(trans_key);
        }
        for (size_t i = 0; i < num_rotations; ++i) {
          // // std::cout << "Inserting rotation num " << i << ": ";
          aiQuatKey k = anim_node->mRotationKeys[i];
          double t = k.mTime;
          aiQuaternion val = k.mValue;
          // std::cout << " time = " << t << ", (" << val.x << ", " << val.y <<
          // ",
          // "
          //          << val.z << ", " << val.w << ")" << std::endl;

          const ozz::animation::offline::RawAnimation::RotationKey rot_key = {
              static_cast<float>(t),
              ozz::math::Quaternion(
                  static_cast<float>(val.x), static_cast<float>(val.y),
                  static_cast<float>(val.z), static_cast<float>(val.w))};
          raw_animation.tracks[track_index].rotations.push_back(rot_key);
        }
        for (size_t i = 0; i < num_scales; ++i) {
          // std::cout << "Inserting scale num " << i << std::endl;
          aiVectorKey k = anim_node->mScalingKeys[i];
          double t = k.mTime;
          aiVector3D val = k.mValue;
          // std::cout << " time = " << t << ", (" << val.x << ", " << val.y <<
          // ",
          // "
          //          << val.z << ")" << std::endl;
          const ozz::animation::offline::RawAnimation::ScaleKey scale_key = {
              static_cast<float>(t),
              ozz::math::Float3(static_cast<float>(val.x),
                                static_cast<float>(val.y),
                                static_cast<float>(val.z))};
          raw_animation.tracks[track_index].scales.push_back(scale_key);
        }
      }
      if (!raw_animation.Validate()) {
        std::cout << "Animation validate failed! :(" << std::endl;
        return -3;
      } else {
        std::cout << "Animation validate success! :)" << std::endl;
      }

      std::ostringstream output_raw_anim_filename;
      if (anim_name.empty()) {
        output_raw_anim_filename << output_pathname << "/" << "anim-"
                                 << anim_num << "-raw.ozz";
      } else {
        output_raw_anim_filename << output_pathname << "/" << anim_name
                                 << "-raw.ozz";
      }
      std::cout << "Outputting raw animation to "
                << output_raw_anim_filename.str() << std::endl;

      ozz::io::File output_raw_anim_file(output_raw_anim_filename.str().c_str(),
                                         "wb");
      ozz::io::OArchive raw_anim_archive(&output_raw_anim_file);
      raw_anim_archive << raw_animation;

      ozz::animation::offline::AnimationBuilder builder;
      std::unique_ptr<ozz::animation::Animation,
                      ozz::Deleter<ozz::animation::Animation>>
          runtime_animation = builder(raw_animation);

      std::ostringstream output_runtime_anim_filename;
      output_runtime_anim_filename << output_pathname << "/";
      if (anim_name.empty()) {
        output_runtime_anim_filename << anim_num;
      } else {
        output_runtime_anim_filename << anim_name;
      }

      output_runtime_anim_filename << "-runtime-anim.ozz";
      std::cout << "Outputting raw animation to "
                << output_runtime_anim_filename.str() << std::endl;

      ozz::io::File output_runtime_anim_file(
          output_runtime_anim_filename.str().c_str(), "wb");
      ozz::io::OArchive runtime_anim_archive(&output_runtime_anim_file);
      runtime_anim_archive << *runtime_animation;
    }
  }

  return true;
}
