import "geometry.msg";

enumerate MarkerOperation {OVERWRITE, UPDATE, EXTEND}

structure Marker {
	string id;
	Point3D location;
	Rotation3D rotation;
}

message Markers {
	MarkerOperation operation;
	string owner;
	Marker[] markers;
}