#include "loader.hpp"

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

	// Dump meshes to file
/*
	std::string output_pathname = _loader.get_output_path();
	fmt::MemoryWriter output_mesh_filename;
	output_mesh_filename << output_pathname << "/meshes.bin";
	std::ofstream os(output_mesh_filename.c_str(), std::ios::binary);
	cereal::BinaryOutputArchive archive(os);
	archive(_loader);
*/
	return 0;
}
