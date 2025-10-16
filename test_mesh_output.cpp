#include <fstream>
#include <iostream>
#include <iterator>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <cstdio>
#include <cstdlib>
#include <kj/array.h>
#include <kj/async.h>
#include <kj/io.h>
#include <kj/memory.h>
#include <memory>
#include <vector>

#include "model3d_schema.capnp.h"

int main(int argc, char **argv) {
  std::string mesh_filename = "./output/seymour.dae/meshes.bin";
  std::ifstream file;
  std::vector<unsigned char> chars;

  file.open(mesh_filename, std::ifstream::binary);
  // Copy file to buffer
  std::copy(std::istream_iterator<unsigned char>(file),
            std::istream_iterator<unsigned char>(), std::back_inserter(chars));
  file.close();

  std::cout << "Filesize: " << chars.size() << std::endl;
  std::cout << "Buffering to kj arrayptr" << std::endl;

  kj::ArrayPtr<const unsigned char> kjchars(
      reinterpret_cast<const unsigned char *>(const_cast<unsigned char *>(&chars[0])),
      chars.size());

  std::cout << "KJChars size: " << kjchars.size() << std::endl;
  kj::ArrayInputStream buffer(kjchars);
  std::cout << "Creating messagereader" << std::endl;

  capnp::ReaderOptions options;
  options.traversalLimitInWords = 2000 * 1024 * 1024;

  capnp::PackedMessageReader message(buffer, options);
  //std::cout << "Message size in words " << message.sizeInWords() << std::endl;
  std::cout << "Getting root node" << std::endl;
  Model::Reader model = message.getRoot<Model>();
  std::cout << "Got root node" << std::endl;

  capnp::List<Mesh>::Reader meshes = model.getMeshes();
  std::cout << "Got meshes" << std::endl;

  for (auto m : meshes) {
    const std::string name = m.getName();
    auto vertices = m.getVertices();
    auto indices = m.getIndices();
    std::cout << "Mesh name: " << name << std::endl
              << "Number of vertices: " << vertices.size() << std::endl
              << "Number of indices: " << indices.size() << std::endl
              << "Bones: ";
    size_t index = 0;
    for (std::string s : m.getBoneNames()) {
      std::cout << "(index: " << index << ", name: " << s << ") ";
    }

    std::cout << std::endl;

    std::cout << "Displaying vertex info:" << std::endl;

    size_t i = 0;
    for (auto v : vertices) {
      std::cout << "Mesh vertex " << i << ": Position =";
      std::cout << " " << v.getPositionX() << " " << v.getPositionY() << " "
                << v.getPositionZ();
      std::cout << ". Normal =";
      std::cout << " " << v.getNormalX() << " " << v.getNormalY() << " "
                << v.getNormalZ();
      std::cout << ". UV =";
      std::cout << " " << v.getUvX() << " " << v.getUvY();
      std::cout << "." << std::endl << "Bones: " << std::endl;
      std::cout << " Indices = " << v.getBoneIndexX() << ","
                << v.getBoneIndexY() << "," << v.getBoneIndexZ() << ","
                << v.getBoneIndexW() << std::endl;
      std::cout << " Weights = " << v.getBoneWeightX() << ","
                << v.getBoneWeightY() << "," << v.getBoneWeightZ() << ","
                << v.getBoneWeightW() << std::endl;
      ++i;
    }

    std::cout << std::endl;
  }
  return 0;
}
