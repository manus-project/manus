

message ConfigEntry {
	string key;
	string value;
}

message ConfigBatch {
	ConfigEntry[] entries;
}

