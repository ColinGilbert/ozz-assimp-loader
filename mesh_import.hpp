// This loader loads skeletal meshes from Assimp into ozz-animation format, and loads proper mesh data to use alongside it in an engine neutral format. Enjoy.
// MIT license.
// Colin Gilbert

#include <array>
#include <vector>
#include <set>
#include <unordered_map>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <lemon/list_graph.h>

#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/runtime/animation.h>


#include "format.h"

class loader
{
	public:
		class hierarchy
		{
			public:
				hierarchy(): _translation(_graph), _scale(_graph), _rotation(_graph), _name(_graph) {}

				void init(const aiScene* scene, const std::set<std::string>& bone_names)
				{
					for (std::string s : bone_names)
					{
						aiNode* ai_node = scene->mRootNode->FindNode(s.c_str());
						lemon::ListDigraph::Node lemon_node = add(ai_node);
						nodes.insert(std::make_pair(ai_node, lemon_node));
					}

					recursive_build(scene->mRootNode, bone_names);
				}

				ozz::animation::offline::RawSkeleton make_raw_skeleton()
				{
					std::vector<lemon::ListDigraph::Node> roots = find_roots();

					std::cout << "Skeleton roots: ";
					for (lemon::ListDigraph::Node r : roots)
					{
						std::cout << _name[r] << " ";
					}
					std::cout << std::endl;

					size_t num_roots = roots.size();
					ozz::animation::offline::RawSkeleton raw_skeleton;
					raw_skeleton.roots.resize(num_roots);

					std::cout << "About to start building skeleton." << std::endl;
					for (size_t i = 0; i < num_roots; i++)
					{
						lemon::ListDigraph::Node r = roots[i];
						ozz::animation::offline::RawSkeleton::Joint& root_joint = raw_skeleton.roots[i];
						std::cout << "Starting recursive build from root " << _name[r] << std::endl;
						recursive_ozz_helper(r, root_joint, i);
					}
					return raw_skeleton;
				}

				void print_info()
				{
					size_t count = 0;

					for (lemon::ListDigraph::NodeIt it(_graph); it != lemon::INVALID; ++it)
					{
						count++;
						lemon::ListDigraph::Node n(it);
						std::cout << "Node " << count << " name = " << _name[n] << std::endl;
					}

					std::cout << "Count = " << count << std::endl;

					// recursive_print();
				}

			protected:

				std::vector<lemon::ListDigraph::Node> find_roots()
				{
					std::vector<lemon::ListDigraph::Node> roots;
					for (lemon::ListDigraph::NodeIt it(_graph); it != lemon::INVALID; ++it)
					{
						size_t count = 0;
						lemon::ListDigraph::Node n(it);

						for (lemon::ListDigraph::InArcIt e(_graph, n); e != lemon::INVALID; ++e)
						{
							++count;
						}

						if (count == 0)
						{
							roots.push_back(n);
						}
					}
					return roots;
				}

				void recursive_build(aiNode* current, const std::set<std::string>& bone_names)
				{
					std::string current_node_name = std::string(current->mName.C_Str());
					for (size_t i = 0; i < current->mNumChildren; i++)
					{
						aiNode* child = current->mChildren[i];

						if (bone_names.find(current_node_name) != bone_names.end())
						{
							std::string child_name = std::string(current->mChildren[i]->mName.C_Str());

							if (bone_names.find(child_name) != bone_names.end())
							{
								std::cout << "Linking parent " << current_node_name << " and child " << child_name << std::endl;
								link(current, child);
							}

						}
						recursive_build(child, bone_names);
					}
				}

				void link(aiNode* parent, aiNode* child)
				{
					lemon::ListDigraph::Node p = nodes.find(parent)->second;
					lemon::ListDigraph::Node c = nodes.find(child)->second;
					_graph.addArc(p, c);
				}

				lemon::ListDigraph::Node add(aiNode* assimp_node)
				{
					std::string name = std::string(assimp_node->mName.C_Str());

					aiMatrix4x4 mat = assimp_node->mTransformation;
					aiVector3D scale, trans;
					aiQuaternion rot;
					mat.Decompose(scale, rot, trans);

					lemon::ListDigraph::Node graph_node = _graph.addNode();

					_translation[graph_node] = {trans.x, trans.y, trans.z };
					_scale[graph_node] = { scale.x, scale.y, scale.z };
					_rotation[graph_node] = { rot.x, rot.y, rot.z, rot.w };
					_name[graph_node] = name;

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

					std::cout << "Adding joint to skeleton. Name = " << _name[n] << ", number of children = " << children_count << std::endl;

					caller_joint.children.resize(children_count);

					size_t current_index = 0;

					for (lemon::ListDigraph::OutArcIt it(_graph, n); it != lemon::INVALID; ++it)
					{
						lemon::ListDigraph::Arc arc(it);
						lemon::ListDigraph::Node next_node = _graph.target(it);

						ozz::animation::offline::RawSkeleton::Joint& current_joint = caller_joint.children[current_index];
						recursive_ozz_helper(next_node, current_joint, current_index);
						current_index++;
					}
				}

				void recursive_print(lemon::ListDigraph::Node n)
				{
					std::cout << "Node name = " << _name[n] << std::endl;

					for (lemon::ListDigraph::OutArcIt it(_graph, n); it != lemon::INVALID; ++it)
					{
						lemon::ListDigraph::Arc arc(it);
						lemon::ListDigraph::Node next_node = _graph.target(it);
						recursive_print(next_node);
					}
				}

				lemon::ListDigraph _graph;
				lemon::ListDigraph::NodeMap<std::array<float, 3>> _translation, _scale;
				lemon::ListDigraph::NodeMap<std::array<float, 4>> _rotation;
				lemon::ListDigraph::NodeMap<std::string> _name; 

				// For easy access
				std::map<aiNode*, lemon::ListDigraph::Node> nodes;
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

		bool load(const aiScene* scene, const std::string& name)
		{
			if (!scene)
			{
				std::cout << fmt::format("[Mesh] load({0}) - cannot open\n", name) << std::endl;
				return false;
			}

			std::vector<loader::mesh> meshes;
			std::set<std::string> scene_bone_names;
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
					scene_bone_names.insert(temp_bone_name);

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

			loader::hierarchy bone_hierarchy;

			bone_hierarchy.init(scene, scene_bone_names);

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

			ozz::animation::offline::SkeletonBuilder skel_builder;
			ozz::animation::Skeleton* skeleton = skel_builder(raw_skel);


			const char* const* joint_names = skeleton->joint_names();
			size_t num_joints = skeleton->num_joints();

			std::unordered_map<std::string, size_t> joint_indices;
			for (size_t i = 0; i < num_joints; ++i)
			{
				std::string s = std::string(joint_names[i]);
				joint_indices.insert(std::make_pair(s, i));
			}

			std::cout << "Displaying ozz skeleton joint names and indices" << std::endl;

			for(auto it = joint_indices.begin(); it != joint_indices.end(); it++)
			{
				std::cout << "Name = " << it->first << ", index = " << it->second << std::endl;
			}

			// TODO: Build animations
			std::vector<ozz::animation::offline::RawAnimation> raw_animations;
			size_t num_anims = scene->mNumAnimations;

			for (size_t anim_num = 0; anim_num < num_anims; ++anim_num)
			{
				aiAnimation* anim = scene->mAnimations[anim_num];
				double ticks = anim->mDuration;
				double ticks_per_sec = anim->mTicksPerSecond;
				size_t num_unfiltered_channels = anim->mNumChannels;

				if (ticks_per_sec < 0.0001)
				{
					ticks_per_sec = 1.0;
				}

				std::string anim_name = std::string(anim->mName.C_Str());
				std::cout << "Assimp animation " << anim_num + 1 << " out of " << num_anims << ". Name: " << anim_name << ". Ticks = " << ticks << ". Ticks per second = " << ticks_per_sec << ". Number of (unfiltered) channels = " << num_unfiltered_channels << std::endl;

				ozz::animation::offline::RawAnimation raw_animation;
				raw_animation.duration = ticks * ticks_per_sec;

				// Filter out the anim nodes that aren't bones.
				// TODO: Find out whether this is necessary or desirable
				std::set<std::tuple<size_t, aiNodeAnim*>> valid_channels;
				for (size_t num = 0; num < num_unfiltered_channels; ++num)
				{
					aiNodeAnim* anim_node = anim->mChannels[num];
					std::string anim_node_name = std::string(anim_node->mNodeName.C_Str());
					auto it = joint_indices.find(anim_node_name);

					if(it != joint_indices.end())
					{
						size_t anim_node_skeleton_index = it->second;
						valid_channels.insert(std::make_tuple(anim_node_skeleton_index, anim_node));
						std:: cout << "Found node " << anim_node_name << " for animation track " << num << "." << std::endl;
					}
					else
					{
						std::cout << "Could not find node " << anim_node_name << " required to build animation track " << num <<  ". SKipping." << std::endl;
						continue;
					}
				}

				raw_animation.tracks.resize(num_joints);
				for (std::tuple<size_t, aiNodeAnim*> chan : valid_channels)
				{
					size_t track_index = std::get<0>(chan);
					aiNodeAnim* anim_node = std::get<1>(chan);
					
					size_t num_translations = anim_node->mNumPositionKeys;
					size_t num_rotations = anim_node->mNumRotationKeys;
					size_t num_scales = anim_node->mNumScalingKeys;
					
					std::cout << "Bone "<< anim_node->mNodeName.C_Str() << ", skeleton index " << track_index << " - Inserting " << num_translations << " translations, " << num_rotations << " rotations, " << num_scales << " scales." << std::endl;

					for (size_t i = 0; i < num_translations; ++i)
					{
						std::cout << "Inserting translation num " << i << ": ";
						aiVectorKey k = anim_node->mPositionKeys[i];
						double t = k.mTime;
						aiVector3D val = k.mValue;
						std::cout << " time = " << t << ", (" << val.x << ", " << val.y << ", " << val.z << ")" << std::endl;
						const ozz::animation::offline::RawAnimation::TranslationKey trans_key = { t, ozz::math::Float3(val.x, val.y, val.z) };
						raw_animation.tracks[track_index].translations.push_back(trans_key);
					}
					for (size_t i = 0; i < num_rotations; ++i)
					{
						std::cout << "Inserting rotation num " << i << ": ";
						aiQuatKey k = anim_node->mRotationKeys[i];
						double t = k.mTime;
						aiQuaternion val = k.mValue;
						std::cout << " time = " << t << ", (" << val.x << ", " << val.y << ", " << val.z << ", " << val.w << ")" << std::endl;

						const ozz::animation::offline::RawAnimation::RotationKey rot_key = { t, ozz::math::Quaternion(val.x, val.y, val.z, val.w) };
						raw_animation.tracks[track_index].rotations.push_back(rot_key);
					}
					for (size_t i = 0; i < num_scales; ++i)
					{
						std::cout << "Inserting scale num " << i << std::endl;
						aiVectorKey k = anim_node->mScalingKeys[i];
						double t = k.mTime;
						aiVector3D val = k.mValue;
						std::cout << " time = " << t << ", (" << val.x << ", " << val.y << ", " << val.z << ")" << std::endl;
						const ozz::animation::offline::RawAnimation::ScaleKey scale_key = { t, ozz::math::Float3(val.x, val.y, val.z) };
						raw_animation.tracks[track_index].scales.push_back(scale_key);
					}
				}
				if (!raw_animation.Validate())
				{
					std::cout << "Animation validate failed! :(" << std::endl;
					return -3;
				}
				else
				{
					std::cout << "Animation validate success! :)" << std::endl;
				}

				ozz::animation::offline::AnimationBuilder builder;
				ozz::animation::Animation* animation = builder(raw_animation);
				// TODO: Optimize animation.

				// TODO: Binary blob of mesh data

				// TODO: Proper directory structure for output

				// TODO: Options via config file

			}



			return true;
		}
};
