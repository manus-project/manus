

enumerate PrivilegedCommandType {SHUTDOWN, REBOOT, UPGRADE}

message PrivilegedCommand {
	string secret;
	PrivilegedCommandType command;
	string[] arguments;
}
