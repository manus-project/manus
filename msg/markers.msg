import "geometry.msg";

enumerate MarkerOperation {OVERWRITE, UPDATE, EXTEND}

structure Color {
	int red;
	int green;
	int blue;
}

structure Marker {
	string id;
	Point3D location;
	Rotation3D rotation;
	Point3D size;
	Color color;
}

message Markers {
	MarkerOperation operation;
	string owner;
	Marker[] markers;
}