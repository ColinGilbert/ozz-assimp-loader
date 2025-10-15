#include <fstream>

#include <cereal/types/string.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/set.hpp>
#include <cereal/archives/binary.hpp>
#include <iostream>

class loader
{
	public:
		template <class Archive>
			void serialize(Archive& ar)
			{
				ar(meshes);
			}

		struct mesh_vertex
		{
			template <class Archive>
				void serialize( Archive & ar )
				{
					ar(position, normal, uv, bone_names, bone_indices, bone_weights);
				}
			mesh_vertex() : position( {0.0f, 0.0f, 0.0f }), normal({ 0.0f, 0.0f, 0.0f }), uv({ 0.0f, 0.0f} ), bone_names({ "", "", "", "" }), bone_indices({ 0, 0, 0, 0 }), bone_weights({ 0.0f, 0.0f, 0.0f, 0.0f}) {}
			std::array<float, 3> position, normal;
			std::array<float, 2> uv;
			std::array<std::string, 4> bone_names;
			std::array<size_t, 4> bone_indices;
			std::array<float, 4> bone_weights;
		};

		struct mesh
		{
			template <class Archive>
				void serialize(Archive & ar)
				{
					ar(translation, scale, dimensions, rotation, name, vertices, indices, bone_names);
				}
			std::array<float, 3> translation, scale, dimensions;
			std::array<float, 4> rotation;
			std::string name;
			std::vector<mesh_vertex> vertices;
			std::vector<uint32_t> indices;
			std::vector<std::string> bone_names;
		};

		std::vector<loader::mesh> meshes;
};


int main(int argc, char** argv)
{
	loader _loader;

	std::string mesh_filename= "./output/seymour.dae/meshes.bin";
	std::ifstream is(mesh_filename.c_str(), std::ios::binary);
	cereal::BinaryInputArchive archive(is);
	archive(_loader);
	
	for (loader::mesh m : _loader.meshes)
	{
		std::cout << "Mesh name: " << m.name << std::endl << "Number of vertices: " << m.vertices.size() << std::endl << "Number of indices: " << m.indices.size() << std::endl << "Bones: ";
		size_t index = 0;
		for (std::string s : m.bone_names)
		{
			std::cout << "(index: " << index << ", name: " << s << ") ";
		}

		std::cout << std::endl;

		std::cout << "Displaying vertex info:" << std::endl;

		for (size_t i = 0; i < m.vertices.size(); ++i)
		{
			loader::mesh_vertex v = m.vertices[i];
			
			std::cout << "Mesh vertex " << i << ": Position =";
			for (float f : v.position)
			{
				std::cout << " " << f;
			}

			std::cout << ". Normal =";
			for (float f : v.normal)
			{
				std::cout << " " << f;
			}
			
			std::cout << ". UV =";
			for (float f : v.uv)
			{
				std::cout << " " << f;
			}
			
			std::cout << "." << std::endl << "Bones: " << std::endl;
			for (size_t ii = 0; ii < v.bone_names.size(); ++ii)
			{
				std::string s = v.bone_names[ii];
				//size_t u = static_cast<size_t>(v.bone_indices[ii]);
				size_t u = v.bone_indices[ii];
				float f = v.bone_weights[ii];
				std::cout << "  Name = " << s << " Index = " << u << " Weight = " << f << std::endl;
			}
			
			std::cout << std::endl;
		}
	}

	return 0;
}
