#include <array>
#include <vector>
#include <unordered_map>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <lemon/list_graph.h>

#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/skeleton_builder.h"
#include "ozz/animation/runtime/skeleton.h"

#include "format.h"

class loader
{
	public:
	// Calling init() creates the root node and must be done prior to calling add().
	class hierarchy
	{
		public:
			hierarchy(): _translation(_graph), _scale(_graph), _rotation(_graph), _name(_graph) {}

			void init(aiNode* root)
			{
				lemon::ListDigraph::Node node = add(root);
				root_node = node;
			}

			void add(aiNode* parent, aiNode* child)
			{
				auto parent_it = names_map.find(std::string(parent->mName.C_Str()));
				auto child_it = names_map.find(std::string(child->mName.C_Str()));

				if (child_it == names_map.end())
				{
					lemon::ListDigraph::Node child_node = add(child);
					lemon::ListDigraph::Node parent_node;

					if (parent_it == names_map.end())
					{
						parent_node = add(parent);
					}
					else
					{
						parent_node = parent_it->second;
					}

					_graph.addArc(parent_node, child_node);
				}
			}

			void print_info()
			{
				for (lemon::ListDigraph::NodeIt n(_graph); n != lemon::INVALID; ++n)
				{
					std::cout << "Node name = " << _name[n] << std::endl;
				}

			}

			ozz::animation::offline::RawSkeleton make_raw_skeleton()
			{
				ozz::animation::offline::RawSkeleton raw_skeleton;
				raw_skeleton.roots.resize(1);
				ozz::animation::offline::RawSkeleton::Joint& root_joint = raw_skeleton.roots[0];
				std::cout << "About to create skeleton from root joint" << std::endl;
				recursive_ozz_helper(root_node, root_joint, 0);

				return raw_skeleton;
			}
			
		protected:

			lemon::ListDigraph::Node add(aiNode* assimp_node)
			{
				aiMatrix4x4 mat = assimp_node->mTransformation;
				aiVector3D scale, trans;
				aiQuaternion rot;
				mat.Decompose(scale, rot, trans);
				lemon::ListDigraph::Node graph_node = _graph.addNode();
				
				_translation[graph_node] = {trans.x, trans.y, trans.z };
				_scale[graph_node] = { scale.x, scale.y, scale.z };
				_rotation[graph_node] = { rot.x, rot.y, rot.z, rot.w };
				std::string name = std::string(assimp_node->mName.C_Str());
				_name[graph_node] = name;
			
				auto it = names_map.find(name);
				if (it == names_map.end())
				{
					names_map.insert(std::make_pair(name, graph_node));
				}
				else
				{
					std::cout << "Node " << name << " already present in graph!" << std::endl;
				}

				return graph_node;
			}

			void recursive_ozz_helper(const lemon::ListDigraph::Node& n, ozz::animation::offline::RawSkeleton::Joint& caller_joint, size_t index) 
			{
				size_t children_count = 0;

				caller_joint.name = _name[n].c_str();
				caller_joint.transform.translation = ozz::math::Float3(_translation[n][0], _translation[n][1], _translation[n][2]);
				caller_joint.transform.rotation = ozz::math::Quaternion(_rotation[n][0], _rotation[n][1], _rotation[n][2], _rotation[n][3]);
				caller_joint.transform.scale = ozz::math::Float3(_scale[n][0], _scale[n][1], _scale[n][2]);

				for (lemon::ListDigraph::OutArcIt it(_graph, n); it != lemon::INVALID; ++it)
				{
					children_count++;
				}
				
				caller_joint.children.resize(children_count);

				size_t current_index = 0;
				for (lemon::ListDigraph::OutArcIt it(_graph, n); it != lemon::INVALID; ++it)
				{
					lemon::ListDigraph::Arc arc(it);
					lemon::ListDigraph::Node next_node = _graph.target(it);//next_node(_graph, it);
					ozz::animation::offline::RawSkeleton::Joint& current_joint = caller_joint.children[current_index];
					recursive_ozz_helper(next_node, current_joint, current_index);
					current_index++;
				}
			}

			lemon::ListDigraph _graph;
			lemon::ListDigraph::NodeMap<std::array<float, 3>> _translation, _scale;
			lemon::ListDigraph::NodeMap<std::array<float, 4>> _rotation;
			lemon::ListDigraph::NodeMap<std::string> _name; 
			
			// For easy access
			std::unordered_map<std::string, lemon::ListDigraph::Node> names_map;
			const lemon::ListDigraph::Node root_node;
	};

	struct mesh_vertex
	{
		mesh_vertex() : position( {0.0f, 0.0f, 0.0f }), normal({ 0.0f, 0.0f, 0.0f }), uv({ 0.0f, 0.0f} ), bone_names({ "", "", "", "" }), bone_weights({ 0.0f, 0.0f, 0.0f, 0.0f} ) {}
		std::array<float, 3> position, normal;
		std::array<float, 2> uv;
		std::array<std::string, 4> bone_names;
		std::array<float, 4> bone_weights;
	};

	struct mesh
	{
		std::array<float, 3> translation, scale, dimensions;
		std::array<float, 4> rotation;
		std::string name;
		std::vector<mesh_vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<std::string> bone_names;
	};

	// Returns empty string if nothing found.
	std::string find_root_bone_name(const aiScene* scene, std::vector<std::string>& bone_names)
	{
		aiNode* scene_root = scene->mRootNode;
		std::string root_name;

		for (std::string s : bone_names)
		{
			aiNode* current_node = scene_root->FindNode(s.c_str());
			aiNode* parent_node = current_node->mParent;

			if (parent_node == NULL)
			{
				return std::string(current_node->mName.C_Str());
			}
			if (std::find(bone_names.begin(), bone_names.end(), parent_node->mName.C_Str()) == bone_names.end()) 
			{
				return std::string(current_node->mName.C_Str()); 
			}
		}

		return "";
	}

	bool load(const aiScene* scene, const std::string& name)
	{
		if (!scene)
		{
			std::cout << fmt::format("[Mesh] load({0}) - cannot open\n", name) << std::endl;
			return false;
		}
		
		std::vector<loader::mesh> meshes;
		std::vector<std::string> scene_bone_names;
		aiNode* scene_root = scene->mRootNode;
		for (size_t mesh_num = 0; mesh_num < scene->mNumMeshes; mesh_num++)
		{
			const aiMesh* mesh_data = scene->mMeshes[mesh_num];
			std::cout << fmt::format("Attempting to obtain data for mesh {0}\n", name);
			
			size_t num_verts = mesh_data->mNumVertices;
			size_t num_faces = mesh_data->mNumFaces;
			size_t num_bones = mesh_data->mNumBones;
			bool has_normals = mesh_data->HasNormals();
			bool has_texcoords = mesh_data->HasTextureCoords(0);
			
			std::cout << fmt::format("Mesh {0} ({1}) has {2} verts and {3} bones. Normals? {4}\n", name, mesh_num, num_verts, num_bones, has_normals);
			
			loader::mesh temp_mesh;
			temp_mesh.name = std::string(mesh_data->mName.C_Str());
			
			std::array<float, 3> min_extents, max_extents;
			
			for (size_t n = 0; n < num_verts; ++n)
			{
				aiVector3D pt = mesh_data->mVertices[n];
				
				mesh_vertex v;
				v.position[0] = pt[0];
				v.position[1] = pt[1];
				v.position[2] = pt[2];
				
				min_extents[0] = std::min(min_extents[0], v.position[0]);
				min_extents[1] = std::min(min_extents[1], v.position[1]);
				min_extents[2] = std::min(min_extents[2], v.position[2]);
				max_extents[0] = std::max(max_extents[0], v.position[0]);
				max_extents[1] = std::max(max_extents[1], v.position[1]);
				max_extents[2] = std::max(max_extents[2], v.position[2]);
				
				if (has_normals)
				{
					aiVector3D normal = mesh_data->mNormals[n];
					v.normal[0] = normal[0];
					v.normal[1] = normal[1];
					v.normal[2] = normal[2];
				}
				if (has_texcoords)
				{
					aiVector3D* uv = mesh_data->mTextureCoords[0];
					v.uv[0] = uv->x;
					v.uv[1] = uv->y;
				}
				
				temp_mesh.vertices.push_back(v);
			}
			
			std::array<float, 3> temp_dims;
			for (size_t i = 0; i < 3; ++i)
			{
				temp_dims[i] = max_extents[i] - min_extents[i];
			}
			temp_mesh.dimensions = temp_dims;
			
			for (size_t n = 0; n < num_faces; ++n)
			{
				aiFace face = mesh_data->mFaces[n];
				if (face.mNumIndices == 3)
				{
					temp_mesh.indices.push_back(face.mIndices[0]);
					temp_mesh.indices.push_back(face.mIndices[1]);
					temp_mesh.indices.push_back(face.mIndices[2]);
				}
			}
			
			for (size_t n = 0; n < num_bones; ++n)
			{
				aiBone* bone_data = mesh_data->mBones[n];
				std::string temp_bone_name = std::string(bone_data->mName.C_Str());
				temp_mesh.bone_names.push_back(temp_bone_name);
				
				// Make sure to keep track of every bone in the scene (in order to account for multimesh models)
				if (std::find(scene_bone_names.begin(), scene_bone_names.end(), temp_bone_name) == scene_bone_names.end())
				{
					scene_bone_names.push_back(temp_bone_name);
				}
				
				// Store the bone names and weights in the vert data.
				for (uint32_t i = 0; i < bone_data->mNumWeights; ++i)
				{
					uint32_t bone_vertex_id = bone_data->mWeights[i].mVertexId;
					if (bone_vertex_id < static_cast<uint32_t>(mesh_data->mNumVertices))
					{
						auto vert = temp_mesh.vertices[bone_vertex_id];
						for (size_t j = 0; j < 4; ++j)
						{
							if (vert.bone_weights[j] == 0.0)
							{
								vert.bone_names[j] = temp_bone_name;
								vert.bone_weights[j] = bone_data->mWeights[i].mWeight;
								break;
							}
						}
					}
					else
					{
						std::cout << "Vertex ID of Assimp bone higher than actual vertex count. Skipping." << std::endl;
					}
				}
			}
			meshes.push_back(temp_mesh);
		}
		
		std::string root_bone_name = find_root_bone_name(scene, scene_bone_names);
		
		if (root_bone_name == "")
		{
			std::cout << "Root bone name is empty - Should not happen." << std::endl;
		}
		else
		{
			std::cout << "Root bone name " << root_bone_name << std::endl;
		}
		aiNode* root_bone_node = scene_root->FindNode(root_bone_name.c_str());

		loader::hierarchy bone_hierarchy;
		
		bone_hierarchy.init(root_bone_node);

		for (std::string bone_name : scene_bone_names)
		{
			aiNode* temp_node = scene_root->FindNode(bone_name.c_str());
			aiNode* parent_node = scene_root->FindNode(bone_name.c_str());
			if (parent_node != NULL)
			{
				bone_hierarchy.add(parent_node, temp_node);
			}
		}

		bone_hierarchy.print_info();

		ozz::animation::offline::RawSkeleton raw_skel = bone_hierarchy.make_raw_skeleton();

		if (!raw_skel.Validate())
		{
			std::cout << "Skeleton validation failed!" << std::endl;
			return false;
		}
		else
		{
			std::cout << "Skeleton validation success!" << std::endl;
		}
		
		return true;
	}
};


int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cout << "Usage: mesh_importer <filename>" << std::endl;
		return -1;
	}
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(argv[1], aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
	loader _loader;

	bool success = _loader.load(scene, argv[1]);
	
	if (!success)
	{
		std::cout << "Import failed! :(" << std::endl;
		return -2;
	}

	return 0;
}
