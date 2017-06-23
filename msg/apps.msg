
namespace manus.apps;

enumerate AppEventType {ACTIVE}
enumerate AppCommandType {EXECUTE, TERMINATE, RESCAN}

structure AppData {
	string id;
	string name;
	int version = 1;
	string description;
	string script;
	bool listed = true;
}

message AppListing {
	AppData[] apps;
}

message AppEvent {
	AppEventType type;
	AppData app;
}

message AppCommand {
	AppCommandType type;
	string[] arguments;
}