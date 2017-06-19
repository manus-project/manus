
namespace manus.apps;

enumerate AppEventType {STOP, START}
enumerate AppCommandType {EXECUTE, TERMINATE, RESCAN}

structure AppData {
	string id;
	string name;
	string script;
	float version = 1.0;
	bool listed = true;
}

message AppListing {
	AppData[] apps;
}

message AppEvent {
	AppData app;
	AppEventType event;
}

message AppCommand {
	AppCommandType type;
	string id;
}