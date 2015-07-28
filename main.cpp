#include "mesh_import.hpp"

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
