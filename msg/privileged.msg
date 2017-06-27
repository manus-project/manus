

enumerate PrivilegedCommandType {SHUTDOWN, REBOOT, UPGRADE}

message PrivilegedCommand {
	string key;
	PrivilegedCommandType command;
	string[] arguments;
}
