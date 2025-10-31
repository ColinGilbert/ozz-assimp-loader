@0xacf01638e9ed1b9c;

struct Array4u {
	array4uX @0 :UInt32;
	array4uY @1 :UInt32;
	array4uZ @2 :UInt32;
	array4uW @3 :UInt32;
}

struct Array4f {
	array4fX @0 :Float32;
	array4fY @1 :Float32;
	array4fZ @2 :Float32;
	array4fW @3 :Float32;
}

struct Array3f {
	array3fX @0 :Float32;
	array3fY @1 :Float32;
	array3fZ @2 :Float32;
}

struct Array2f {
	array2fX @0 :Float32;
	array2fY @1 :Float32;
}

struct Mesh {
    translationX @0 :Float32;
    translationY @1 :Float32;
	translationZ @2 :Float32;
    scaleX @3 :Float32;
	scaleY @4 :Float32;
	scaleZ @5 :Float32;
	dimensionsX @6 :Float32;
    dimensionsY @7 :Float32;
	dimensionsZ @8 :Float32;
	rotationX @9 :Float32;
	rotationY @10 :Float32;
	rotationZ @11 :Float32;
	rotationW @12 :Float32;
	name @13 :Text;
	indices @14:List(UInt64);
	positions @15 :List(Array3f);
	normals @16: List(Array3f);
	uvs @17: List(Array2f);
	boneIndices @18: List(Array4u);
	boneWeights @19: List(Array4f);
	boneNames @20 :List(Text);
	materialIndex @21: UInt32;
}

struct Material {
	diffuseTexturePath @0: Text;
	normalsTexturePath @1: Text;
	specularTexturePath @2: Text;
	name @3: Text;

}

struct Model {
	meshes @0: List(Mesh);
	materials @1: List(Material);
}