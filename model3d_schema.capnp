@0xacf01638e9ed1b9c;

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
	vertices @14 :List(MeshVertex);
	indices @15:List(UInt64);
	boneNames @16 :List(Text);
}

struct MeshVertex {
	positionX @0 :Float32;
    positionY @1 :Float32;
	positionZ @2 :Float32;
    normalX @3 :Float32;
	normalY @4 :Float32;
	normalZ @5 :Float32;
    uvX @6 :Float32;
    uvY @7 :Float32;
    boneIndexX @8 :UInt32;
    boneIndexY @9 :UInt32;
	boneIndexZ @10 :UInt32;
    boneIndexW @11 :UInt32;
    boneWeightX @12 :Float32;
    boneWweightY @13 :Float32;
    boneWeightZ @14 :Float32;
    boneWeightW @15 :Float32;
}

struct Model {
	meshes @0: List(Mesh);
}