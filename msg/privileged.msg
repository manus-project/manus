

enumerate PrivilegedCommandType {SYSTEM, NETWORK, HARDWARE}

message PrivilegedCommand {
	string secret;
	PrivilegedCommandType command;
	string[] arguments;
}

