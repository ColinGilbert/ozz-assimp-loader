#include <fstream>

#include <cereal/types/string.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/set.hpp>
#include <cereal/archives/binary.hpp>

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
					ar(position, normal, uv, bone_names, bone_weights);
				}
			mesh_vertex() : position( {0.0f, 0.0f, 0.0f }), normal({ 0.0f, 0.0f, 0.0f }), uv({ 0.0f, 0.0f} ), bone_names({ "", "", "", "" }), bone_weights({ 0.0f, 0.0f, 0.0f, 0.0f} ) {}
			std::array<float, 3> position, normal;
			std::array<float, 2> uv;
			std::array<std::string, 4> bone_names;
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
		std::cout << "Mesh name: " << m.name << std::endl << "Number of indices: " << m.indices.size() << ". Bone names: ";

		for (std::string s : m.bone_names)
		{
			std::cout << s << " ";
		}

		std::cout << std::endl;
	}

	return 0;
}
