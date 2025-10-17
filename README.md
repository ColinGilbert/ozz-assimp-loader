Assimp can be a real pain to use with skeletal information.

Therefore, I wrote a single-header utility that loads a file with Assimp, uses Lemon Graph Library (https://lemon.cs.elte.hu/trac/lemon) to parse it, and exports the info to ozz-animation's own binary format. Furthermore, it uses CapnProto  to create a binary dump of the model and its corresponding meshes into your game engine, all neatly bundled up and ready to use.

Relevant source files:
```
./mesh_import.hpp holds the bulk of the work
./test_mesh_output.cpp tests the binary exported by Cereal.
```

To build the example (*nix):
```
git clone --recursive https://www.github.com/ColinGilbert/ozz-assimp-loader &&
cd ozz-assimp-loader &&
mkdir build &&
cmake ..
&& make
```

To run the Assimp to ozz importer, run:
```
./mesh_importer seymour.dae
```
It creates a directory called "output" in its local directory and spits out all necessary files.

To test the results, run:
```
./test_mesh_output
```


Enjoy!
