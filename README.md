This little project takes Assimp and ozz-animation, then puts them together to import skeletal animations + their corresponding meshes into your game engine, all neatly bundled up and ready for use. Only external dependencies are Assimp and Boost Filesystem (version 3.)

Uses Cereal https://github.com/USCiLab/cereal to serialize mesh data.

To clone:
```
git clone --recursive https://www.github.com/ColinGilbert/ozz-assimp-loader
```

To use, copy the program to whichever directory you're using and type in:
```
./mesh_importer <filename>
```

It creates a directory called "output" in its local directory and spits out all necessary files, but soon there'll be a config file to allow the user to change that, among other things.
