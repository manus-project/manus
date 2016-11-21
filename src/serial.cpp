// version: 0.6

#include "serial.h"
#include "debug.h"
#include <stdio.h>

#define FIXJ 4
#define FIXN 1

SerialPortRobotArm::SerialPortRobotArm(const string& device, int rate) {

	setDefaultParameters();

    con_param.device = device;
    con_param.baud = rate;

}

SerialPortRobotArm::~SerialPortRobotArm()
{
    disconnect();
}


//********************************
//		setArmParameters
//********************************
int SerialPortRobotArm::setDefaultParameters () {
	// todo: read "connection parameters" and "arm info" from file
	// tu nastavimo vse statične podatke:

	// connwction parameters:
#if defined (_WIN32) || defined( _WIN64)
	con_param.device = "COM1";
#else
	con_param.device = "/dev/ttyACM0";
	//con_param.device = (char *)"/dev/ttyUSB1";
#endif
	con_param.baud = 19200;

	// arm info:
	arm_info.id = 0;
	arm_info.version = 0.0f;
	arm_info.name = "USB Robot Arm";
	arm_info.joints = 0;
	arm_info.update_rate = 0;
	arm_info.update_rate_max = 0;

	// arm data:
	arm_data.id = 0;
	arm_data.state = UNKNOWN;
	//arm_data.hand_sensor = -1;
	arm_data.input_voltage = -1.0f;
	arm_data.motors_voltage = -1.0f;
	arm_data.motors_current = -1.0f;
	arm_data.error_id = 0;

	// motor info
	// motor data

	//last_id
	last_id.arm_info_id = 0;
	last_id.arm_data_id = 0;
	// za motorje nastavi naknadno..,

	// in_state
	in_state.control_started = 0;
	in_state.device_state = 0;
	in_state.valid_arm_info = 0;
	in_state.valid_arm_data = 0;
	in_state.valid_motor_info = 0;
	in_state.valid_motor_data = 0;
	in_state.sended_start = 0;
	in_state.sended_stop = 0;
	in_state.sended_pause = 0;
	in_state.sended_continue = 0;
	in_state.sended_fold_arm = 0;
	in_state.sended_callibration = 0;
	in_state.sended_ping = 0;
	in_state.connection_started = 0;

	// reed..:
	additional_data_in_buffer = false;
	command = 0; // no command
	data_length = 0; // no data

	//faktor_pwm = (1<<16)-1;
	faktor_pwm = 65535;

	return 1;
}

//********************************
//		setArmParametersDynamic
//********************************
int SerialPortRobotArm::setArmParametersDynamic() {

	// arm info
	// arm data
	//last_id
	last_id.motor_info_id.resize(arm_info.joints);
	last_id.motor_data_id.resize(arm_info.joints);

	// motors data and information
	motor_info.resize(arm_info.joints);
	motor_data.resize(arm_info.joints);

    // TODO: temporary hardcoding
    /*
    motor_info[0] = createJointInfo(0, ROTATION, 	0, 90, 65,   0, -135, 135);
    motor_info[1] = createJointInfo(1, ROTATION,  120, 	0,  0,  90,    0, 180);
    motor_info[2] = createJointInfo(2, ROTATION,  -90, 	0,  0, 132, -150, 150);
    motor_info[3] = createJointInfo(3, ROTATION,    0, 	0,  0,  60,  -60, 60);
    motor_info[4] = createJointInfo(4, GRIPPER,     0,-90,  0,  20,    0, 1);
    motor_info[5] = createJointInfo(4, GRIPPER,     0,-90,  0,  20,    0, 1);
    */
    /*
    joint_info.push_back(createJointInfo(0, ROTATION, 0, 90, 65, 0, -180, 180));
    joint_data.push_back(createJointData(0, 0));

    joint_info.push_back(createJointInfo(1, ROTATION, 90, 0, 0, 90, 0, 180));
    joint_data.push_back(createJointData(1, 90));

    joint_info.push_back(createJointInfo(2, ROTATION, -90, 0, 0, 132, -110, 110));
    joint_data.push_back(createJointData(2, -90));

    joint_info.push_back(createJointInfo(3, ROTATION, 0, 0, 0, 60, -90, 90));
    joint_data.push_back(createJointData(3, 0));

    joint_info.push_back(createJointInfo(4, ROTATION, 0, 0, 0, 20, -90, 90));
    joint_data.push_back(createJointData(4, 0));

    joint_info.push_back(createJointInfo(5, FIXED, 90, 90, 0, 0, 0, 0));
    joint_data.push_back(createJointData(5, 0));

    joint_info.push_back(createJointInfo(6, ROTATION, 0, 0, 20, 0, -90, 90));
    joint_data.push_back(createJointData(6, 0));

    joint_info.push_back(createJointInfo(7, GRIPPER, 0, 0, 0, 0, 0, 1));
    joint_data.push_back(createJointData(7, 0));
    */
    motor_info[0] = createJointInfo(0, ROTATION,    0, 90, 113,   0, -135, 135);
    motor_info[1] = createJointInfo(1, ROTATION,   140,  0,  0, 108,    0, 180);
    motor_info[2] = createJointInfo(2, ROTATION,  -80, 	0,  0, 112, -150, 150);
    motor_info[3] = createJointInfo(3, ROTATION,    -50, 	0,  0,  20,  -60, 60);
    motor_info[4] = createJointInfo(4,    FIXED,   90, 90,  0,   0,    0,  0);	// FIXED!!
    motor_info[5] = createJointInfo(5, ROTATION,    0,  0,  0,  0,    0, 1);
    motor_info[6] = createJointInfo(6, GRIPPER,     0,  0,  50,  0,    0, 1);

	for( int q = 0; q < motor_info.size(); q++ ){
		last_id.motor_info_id[q] = 0;
		last_id.motor_data_id[q] = 0;

		motor_info[q].id = 0;
		motor_info[q].joint_id = q;
		motor_info[q].position_min = 0.0f;
		motor_info[q].position_max = 0.0f;
		motor_info[q].limit_voltage = 0.0f;
		motor_info[q].limit_current = 0.0f;

		motor_data[q].id = 0;
		motor_data[q].joint_id = q;
		motor_data[q].position = 0.0f;
		motor_data[q].position_goal = 0.0f;
		motor_data[q].state_id = 0;
		motor_data[q].error_id = 0;

        DEBUGMSG("Initialize joint %d of type %d DH=(%f,%f,%f,%f)\n", q, motor_info[q].type, motor_info[q].dh_a, motor_info[q].dh_alpha, motor_info[q].dh_d, motor_info[q].dh_theta);
	}
	DEBUGMSG("dodatno\n");
	// nastavimo parametre roke, ki jih vrne sama roka...
	int tmp = 0;
	in_state.connection_started = 2;
	requestArmInfo();
#if defined(_WIN32) || defined(_WIN64)
			Sleep(1000);
#else
		    sleep(1);
#endif
	//DEBUGMSG("loop\n");
	// v sekundi bi morali sigurno priti že vsi podatki o servomotorjih...
	for(int q=0; q<20; q++){
		DEBUGMSG("loop: %d\n", q);
		tmp = 0;
		dataArrived();
		//DEBUGMSG("loop: %d\n", q);
		for( int w = 0; w < motor_info.size(); w++ ){
			if(motor_info[w].id != 0) ++tmp;
			motor_info[w].dh_min = motor_info[w].position_min;
			motor_info[w].dh_max = motor_info[w].position_max;
			//cout << "m: " << q << " min: " << motor_info[q].position_min << " max: " << motor_info[q].position_max << endl;
			DEBUGMSG("m: %d min: %f max: %f\n", w, motor_info[w].position_min, motor_info[w].position_max);
			//DEBUGMSG("Initialize joint %d of type %d DH=(%f,%f,%f,%f)\n", q, motor_info[q].type, motor_info[q].dh_a, motor_info[q].dh_alpha, motor_info[q].dh_d, motor_info[q].dh_theta);
		}
		//cout << tmp << endl;
		DEBUGMSG("%d\n", tmp);
		if(tmp == arm_info.joints-1) break;
	}
	DEBUGMSG("konec\n");
	return 1;
}

/*
     \return 1 success
     \return -1 device not found
     \return -2 error while opening the device
     \return -3 error while getting port parameters
     \return -4 Speed (Bauds) not recognized
     \return -5 error while writing port parameters
     \return -6 error while writing timeout parameters

	 Device : Port name (COM1, COM2, ... for Windows ) or (/dev/ttyS0, /dev/ttyACM0, /dev/ttyUSB0 ... for linux)
  */
int SerialPortRobotArm::connect(){
	if(in_state.control_started == 0){
#if defined (_WIN32) || defined( _WIN64)

		// Open serial port
        string dev = "\\\\.\\";
        dev.append(con_param.device);
        hSerial = CreateFileA(dev.c_str() ,GENERIC_READ | GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);

		if(hSerial==INVALID_HANDLE_VALUE) {
			if(GetLastError()==ERROR_FILE_NOT_FOUND)
				return -1;                                                  // Device not found
			return -2;                                                      // Error while opening the device
		}

		// Set parameters
		DCB dcbSerialParams = {0};                                          // Structure for the port parameters
		dcbSerialParams.DCBlength=sizeof(dcbSerialParams);
		if (!GetCommState(hSerial, &dcbSerialParams))                       // Get the port parameters
			return -3;                                                      // Error while getting port parameters
		switch (con_param.baud){                                            // Set the speed (Bauds)
			case 110  :     dcbSerialParams.BaudRate=CBR_110; break;
			case 300  :     dcbSerialParams.BaudRate=CBR_300; break;
			case 600  :     dcbSerialParams.BaudRate=CBR_600; break;
			case 1200 :     dcbSerialParams.BaudRate=CBR_1200; break;
			case 2400 :     dcbSerialParams.BaudRate=CBR_2400; break;
			case 4800 :     dcbSerialParams.BaudRate=CBR_4800; break;
			case 9600 :     dcbSerialParams.BaudRate=CBR_9600; break;
			case 14400 :    dcbSerialParams.BaudRate=CBR_14400; break;
			case 19200 :    dcbSerialParams.BaudRate=CBR_19200; break;
			case 38400 :    dcbSerialParams.BaudRate=CBR_38400; break;
			case 56000 :    dcbSerialParams.BaudRate=CBR_56000; break;
			case 57600 :    dcbSerialParams.BaudRate=CBR_57600; break;
			case 115200 :   dcbSerialParams.BaudRate=CBR_115200; break;
			case 128000 :   dcbSerialParams.BaudRate=CBR_128000; break;
			case 256000 :   dcbSerialParams.BaudRate=CBR_256000; break;
			default : return -4;
		}
		dcbSerialParams.ByteSize=8;                                         // 8 bit data
		dcbSerialParams.StopBits=ONESTOPBIT;                                // One stop bit
		dcbSerialParams.Parity=NOPARITY;                                    // No parity
		if(!SetCommState(hSerial, &dcbSerialParams))                        // Write the parameters
			return -5;                                                      // Error while writing

//		if(!SetCommTimeouts(hSerial, &timeouts))                            // Write the parameters
//			return -6;                                                      // Error while writting the parameters
//		return 1;                                                           // Opening successfull

//#endif
#else
	    struct termios options;				// Structure with the device's options

	    // Open device
	    fd = open(con_param.device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
	    if (fd == -1) return -2;			// If the device is not open, return -2
	    fcntl(fd, F_SETFL, FNDELAY);		// Open the device in nonblocking mode

	    // Set parameters
	    tcgetattr(fd, &options);			// Get the current options of the port
	    bzero(&options, sizeof(options));	// Clear all the options
	    speed_t         Speed;
	    switch (con_param.baud){			// Set the speed (Bauds)
	    	case 110  :     Speed=B110; break;
	    	case 300  :     Speed=B300; break;
	    	case 600  :     Speed=B600; break;
	    	case 1200 :     Speed=B1200; break;
	    	case 2400 :     Speed=B2400; break;
	    	case 4800 :     Speed=B4800; break;
	    	case 9600 :     Speed=B9600; break;
	    	case 19200 :    Speed=B19200; break;
	    	case 38400 :    Speed=B38400; break;
	    	case 57600 :    Speed=B57600; break;
	    	case 115200 :   Speed=B115200; break;
	    	default : return -4;
		}
	    cfsetispeed(&options, Speed);					// Set the baud rate at 115200 bauds
	    cfsetospeed(&options, Speed);
	    options.c_cflag |= ( CLOCAL | CREAD |  CS8);	// Configure the device : 8 bits, no parity, no control
	    options.c_iflag |= ( IGNPAR | IGNBRK );
	    options.c_cc[VTIME]=0;							// Timer unused
	    options.c_cc[VMIN]=0;							// At least on character before satisfy reading
	    tcsetattr(fd, TCSANOW, &options);				// Activate the settings
#endif

	    // send arm info command to arm and weit for response:
#if defined(_WIN32) || defined(_WIN64)
		Sleep(1000);
#else
	    sleep(1);
#endif
	    for(int q=0; q<5; q++){
			requestArmInfo(); // replace with dumy write to device
#if defined(_WIN32) || defined(_WIN64)
			Sleep(1000);
#else
		    sleep(1);
#endif
	    	if(requestArmInfo() == 1) break;
	    	if(q==4){
	    		closeConnection();
				in_state.connection_started = 0;
	    		return -7; //
	    	}
	    	//sleep(1);
	    }
	    for(int q=0; q<5; q++){
	    	dataArrived();
	    	if(in_state.connection_started == 1) break;
	    	if(q==4){
	    		closeConnection();
				in_state.connection_started = 0;
	    		return -8; // no response
	    	}
#if defined(_WIN32) || defined(_WIN64)
			Sleep(1000);
#else
			sleep(1);
#endif
	    }
		setArmParametersDynamic();
	    return 1;	// Success
	}
    return 0; // no change
}


//********************************
//		disconnect
//********************************
//   Close the connection with the current device
int SerialPortRobotArm::disconnect(){
	if(con_param.connected == 0) return 0; // no change
	stopControl();
	closeConnection();
	con_param.connected = 0;
	return 1; // success
}

int SerialPortRobotArm::closeConnection(){
	#if defined(_WIN32) || defined(_WIN64)
	CloseHandle(hSerial);
#else
    close(fd);
#endif
	return 0;
}


//********************************
//		writeData
//********************************
// ::: Read/writeData operation on bytes :::
/*!
     \brief writeData an array of data on the current serial port
     \param Buffer : array of bytes to send on the port
     \param NbBytes : number of byte to send
     \return 1 success
     \return -9 error while writting data
  */
char SerialPortRobotArm::writeData(const void *Buffer, const unsigned int NbBytes){
#if defined(_WIN32) || defined(_WIN64)
	DWORD dwBytesWritten;                                               // Number of bytes written
    if(!WriteFile(hSerial,Buffer,NbBytes,&dwBytesWritten,NULL))         // Write the char
        return -9;                                                      // Error while writing
    return 1;
#else
    if (write (fd,Buffer,NbBytes)!=(ssize_t)NbBytes)	// Write data
        return -9;										// Error while writing
    return 1;										// Write operation successfull
#endif
}

//********************************
//		ReadChar
//********************************
/*!
     \param pByte : data read on the serial device
     \return 0 success
     \return -1 error
     \return -2 error while reading the byte
  */
char SerialPortRobotArm::readChar(char *pByte){
#if defined (_WIN32) || defined(_WIN64)
    DWORD dwBytesRead = 0;                                                    // Error while writting the parameters
    if(!ReadFile(hSerial,pByte, 1, &dwBytesRead, NULL))                 // Read the byte
        return -2;                                                      // Error while reading the byte
    if (dwBytesRead==0) return -1;
    return 0;                                                           // Success
#else
	switch (read(fd,pByte,1)){	// Try to read a byte on the device
		case 1  : return 0;		// Read successfull
		case -1 : return -2;	// Error while reading
	}
	return -1;
#endif
}

//********************************
//		readData
//********************************
/*!
     \param Buffer : array of bytes read from the serial device
     \param MaxNbBytes : maximum allowed number of bytes read
     \return 0 success, return the number of bytes read
     \return -1 error
     \return -2 error while reading the byte
  */
int SerialPortRobotArm::readData (void *Buffer,unsigned int MaxNbBytes){
#if defined (_WIN32) || defined(_WIN64)
    DWORD dwBytesRead = 0;
//    timeouts.ReadTotalTimeoutConstant=(DWORD)TimeOut_ms;                // Set the TimeOut
    //if(!SetCommTimeouts(hSerial, &timeouts))                            // Write the parameters
    //    return -1;                                                      // Error while writting the parameters
    if(!ReadFile(hSerial,Buffer,(DWORD)MaxNbBytes,&dwBytesRead, NULL))  // Read the bytes from the serial device
        return -2;                                                      // Error while reading the byte
    if (dwBytesRead!=(DWORD)MaxNbBytes) return -1;
    return 0;                                                           // Success
#else
    unsigned int     NbByteRead=0;
    while (true)
    {
        unsigned char* Ptr=(unsigned char*)Buffer+NbByteRead;	// Compute the position of the current byte
        int Ret=read(fd,(void*)Ptr,MaxNbBytes-NbByteRead);		// Try to read a byte on the device
        if (Ret==-1) return -2;				// Error while reading
        if (Ret>0) {	// One or several byte(s) has been read on the device
            NbByteRead+=Ret;			// Increase the number of read bytes
            if (NbByteRead>=MaxNbBytes)	// Success : bytes has been read
                return 0;
        }
    }
    return -1;
#endif
}


// ::: Special operation :::

//********************************
//		FlushReceiver
//********************************
// Empty receiver buffer (UNIX only)
void SerialPortRobotArm::flushReceiver(){
#if defined (_WIN32) || defined(_WIN64)

#else
    tcflush(fd,TCIFLUSH);
#endif
}

//********************************
//		Peek
// Return the number of bytes in the received buffer (UNIX only)
int SerialPortRobotArm::peek(){
#if defined (_WIN32) || defined(_WIN64)
	ClearCommError(hSerial, &dw_errors, &com_stat);
	// report any error?
	if( com_stat.cbInQue ) return com_stat.cbInQue;
	return 0;
#else
    int Nbytes=0;
    ioctl(fd, FIONREAD, &Nbytes);
    return Nbytes;
#endif
}


//------------------------------------------------------------------------------
//********************************
//		dataArrived
//********************************
int SerialPortRobotArm::dataArrived(){
	// look for arrived data
	int change = 0;
	//if( Peek() > 0 ){
	//
	while( peek() >= data_length && peek() > 0 ){
		// preverimo kater ukaz smo prijeli in določimo pričakovano dolžino podatkov
		if( command == 0 && data_length == 0 ){
			// read command
			if( readChar(&command) == 0 ){
				switch( (unsigned char)command) {
					case 5:	// ping
						data_length = 2;
						break;
					case 32:	// confrm of command
						data_length = 1;
						break;
					case 33:	// arm info
						data_length = 6;
						break;
					case 34:	// arm data
						data_length = 9;
						break;
					case 35:	// Motor info
						data_length = 9;
						break;
					case 36:	// Motor data
						data_length = 7;
						break;
					default : // unknown command
						data_length = 0;
						command = 0;
						//return -3;
						break;
				}
			}
			else{
				// error
				command = 0;
				return -1;
			}
		}

		// pogledamo če smo prijeli že dovolj podatkov, če smo, jih preparsamo
		if(command != 0 && peek() >= data_length){
			int tmp_int = 0;
			int tmp_in2t = 1;
			// end off received data
			if(readData(read_buffer, data_length) == 0){
				// parse data
				change = 1;

				switch((unsigned char)command){
					case 5: // ping
						if(in_state.sended_ping == 0){ // dobimo zahtevo ping s strani naprave, potrebno odgovoriti
							write_buffer[0] = 5;
							write_buffer[1] = read_buffer[0];
							write_buffer[2] = read_buffer[1];
							writeData(write_buffer, 3);
						}
						// sicer smo dobili odgovor na naš ping
						else if( read_buffer[0] == in_state.ping_data[0] && read_buffer[1] == in_state.ping_data[1]){
							in_state.sended_ping ++;
						}
						break;

					case 32:	// confrm of command
						// tole v prihodnjosti naredi bolj kompaktno!! ->
						switch( (unsigned char)read_buffer[2] ){
							case 4: // sended callibration
								if( !additional_data_in_buffer ){
									if((unsigned char)read_buffer[0] == 0){
										if(in_state.sended_callibration > 0) in_state.sended_callibration++;
										else in_state.sended_callibration--; // dobimo potrditev, a sploh nismo poslali ukaza
									}
									else if((unsigned char)read_buffer[0] == 1){
										// zavrnjeno
										in_state.sended_callibration = -1;
									}
									if( (unsigned int)read_buffer[1] ){
										data_length = (unsigned int)read_buffer[1];
										additional_data_in_buffer = true;
									}
								}
								else{
									// dobimo še podatke kater motor smo zahtevali
									additional_data_in_buffer = false;
								}

								break;
							case 16: // sended start
								if( !additional_data_in_buffer ){
									if((unsigned char)read_buffer[0] == 0){
										if(in_state.sended_start > 0) in_state.sended_start++;
										else in_state.sended_start--; // dobimo potrditev, a sploh nismo poslali ukaza
									}
									else if((unsigned char)read_buffer[0] == 1){
										// zavrnjeno
										in_state.sended_start = -1;
									}
									if( (unsigned int)read_buffer[1] ){
										data_length = (unsigned int)read_buffer[1];
										additional_data_in_buffer = true;
									}
								}
								else{
									// do tega nebi smelo priti!
									additional_data_in_buffer = false;
								}
								break;
							case 17: // sended stop
								if( !additional_data_in_buffer ){
									if((unsigned char)read_buffer[0] == 0){
										if(in_state.sended_stop > 0) in_state.sended_stop++;
										else in_state.sended_stop--; // dobimo potrditev, a sploh nismo poslali ukaza
									}
									else if((unsigned char)read_buffer[0] == 1){
										// zavrnjeno
										in_state.sended_stop = -1;
									}
									if( (unsigned int)read_buffer[1] ){
										data_length = (unsigned int)read_buffer[1];
										additional_data_in_buffer = true;
									}
								}
								else{
									// do tega nebi smelo priti!
									additional_data_in_buffer = false;
								}
								break;
							case 18: // sended pause
								if( !additional_data_in_buffer ){
									if((unsigned char)read_buffer[0] == 0){
										if(in_state.sended_pause > 0) in_state.sended_pause++;
										else in_state.sended_pause--; // dobimo potrditev, a sploh nismo poslali ukaza
									}
									else if((unsigned char)read_buffer[0] == 1){
										// zavrnjeno
										in_state.sended_pause = -1;
									}
									if( (unsigned int)read_buffer[1] ){
										data_length = (unsigned int)read_buffer[1];
										additional_data_in_buffer = true;
									}
								}
								else{
									// dobimo še podatke kater motor smo zahtevali
									additional_data_in_buffer = false;
								}
								break;
							case 19: // sended continue
								if( !additional_data_in_buffer ){
									if((unsigned char)read_buffer[0] == 0){
										if(in_state.sended_continue > 0) in_state.sended_continue++;
										else in_state.sended_continue--; // dobimo potrditev, a sploh nismo poslali ukaza
									}
									else if((unsigned char)read_buffer[0] == 1){
										// zavrnjeno
										in_state.sended_continue = -1;
									}
									if( (unsigned int)read_buffer[1] ){
										data_length = (unsigned int)read_buffer[1];
										additional_data_in_buffer = true;
									}
								}
								else{
									// dobimo še podatke kater motor smo zahtevali
									additional_data_in_buffer = false;
								}
								break;
							case 20: // sended fold arm
								if( !additional_data_in_buffer ){
									if((unsigned char)read_buffer[0] == 0){
										if(in_state.sended_fold_arm > 0) in_state.sended_fold_arm++;
										else in_state.sended_fold_arm--; // dobimo potrditev, a sploh nismo poslali ukaza
									}
									else if((unsigned char)read_buffer[0] == 1){
										// zavrnjeno
										in_state.sended_fold_arm = -1;
									}
									if( (unsigned int)read_buffer[1] ){
										data_length = (unsigned int)read_buffer[1];
										additional_data_in_buffer = true;
									}
								}
								else{
									// dobimo še podatke kater motor smo zahtevali
									additional_data_in_buffer = false;
								}
								break;
							default : ;
						}
						break;

					case 33:	// arm info
						if(!additional_data_in_buffer){
							arm_info.id++;
							arm_info.version = 0.01f * (float)(((unsigned int)read_buffer[0] << 8) + (unsigned int)read_buffer[1]);
							arm_info.joints = (unsigned int)read_buffer[2]+1;
							arm_info.update_rate = (unsigned int)read_buffer[3];
							arm_info.update_rate_max = (unsigned int)read_buffer[4];
							if( (unsigned int)read_buffer[5] ){
								data_length = (unsigned int)read_buffer[5];
								additional_data_in_buffer = true;
							}
							in_state.valid_arm_info = true;
							if(in_state.connection_started == 0)
								in_state.connection_started = 1;
						}
						else{
							// sestavi string: !!
							additional_data_in_buffer = false;
						}
						//connection_started = 1;
						//return 1;
						break;

					case 34: // arm data
						arm_data.id++;
						//arm_data.state = (unsigned int)read_buffer[0];
						arm_data.state = (ArmState)read_buffer[0];
						switch(read_buffer[0]){
							case 1: arm_data.state = CONNECTED; break;
							case 2: arm_data.state = PASSIVE; break;
							case 3: arm_data.state = ACTIVE; break;
							case 4: arm_data.state = CALIBRATION; break;
							case 0:
							default: arm_data.state = UNKNOWN;
						}
						arm_data.error_id = (unsigned int)read_buffer[1];
						//arm_data.hand_sensor = (int)read_buffer[2];
						arm_data.input_voltage = 0.001f * (float)(((unsigned int)read_buffer[3]<<8) + (unsigned int)read_buffer[4]);
						arm_data.motors_voltage = 0.001f * (float)(((unsigned int)read_buffer[5]<<8) + (unsigned int)read_buffer[6]);
						arm_data.motors_current = 0.001f * (float)(((unsigned int)read_buffer[7]<<8) + (unsigned int)read_buffer[8]);
						in_state.valid_arm_data = true;
						//return 2;
						break;

					case 35:	// motor info
                        //if (in_state.control_started == 0) break;
                        if (in_state.connection_started != 2) break;
						tmp_int = (unsigned int)read_buffer[0];
						if(tmp_int >= FIXJ) tmp_int++;
                        //printf("Motor: %d %d %d \n", tmp_int, in_state.control_started, motor_info.size());
						motor_info[ tmp_int ].id++;
						motor_info[ tmp_int ].joint_id = tmp_int; // (unsigned int)read_buffer[0]; // hmmm...
						//motor_info[ tmp_int ].position_min = 0.01f * (float)(((short int)read_buffer[1]<<8) + (short int)read_buffer[2]);
						//motor_info[ tmp_int ].position_max = 0.01f * (float)(((short int)read_buffer[3]<<8) + (short int)read_buffer[4]);

						motor_info[ tmp_int ].position_min = 0.01f * (short int)(((short int)read_buffer[1]<<8) + (short int)read_buffer[2]);
						motor_info[ tmp_int ].position_max = 0.01f * (short int)(((short int)read_buffer[3]<<8) + (short int)read_buffer[4]);

						motor_info[ tmp_int ].limit_voltage = 0.001f * (float)(((unsigned int)read_buffer[5]<<8) + (unsigned int)read_buffer[6]);
						motor_info[ tmp_int ].limit_current = 0.001f * (float)(((unsigned int)read_buffer[7]<<8) + (unsigned int)read_buffer[8]);

						//DEBUGMSG( "motor info: %d + %d = %d = %f\n", ((unsigned int)read_buffer[5] << 8), (unsigned int)read_buffer[6], (((unsigned int)read_buffer[5] << 8) + (unsigned int)read_buffer[6]), (float)((((unsigned int)read_buffer[5]) << 8) + (unsigned int)read_buffer[6]) );
						in_state.valid_motor_info = true;
						break;

					case 36:	// motor data
                        if (in_state.control_started == 0) break;

						tmp_int = (unsigned int)read_buffer[0];
						if(tmp_int >= FIXJ) tmp_int++;
						motor_data[ tmp_int ].id++;
						motor_data[ tmp_int ].joint_id = tmp_int; //(unsigned int)read_buffer[0];
						//motor_data[ tmp_int ].position = 0.01f * (float)(((unsigned int)read_buffer[1]<<8) + (unsigned int)read_buffer[2]);

						motor_data[ tmp_int ].position = 0.01f * (short int)(((short int)read_buffer[1]<<8) + (short int)read_buffer[2]);

						//DEBUGMSG( "motor info: %d + %d = %d = %f\n", ((unsigned int)read_buffer[1] << 8), (unsigned int)read_buffer[2], (((unsigned int)read_buffer[1] << 8) + (unsigned int)read_buffer[2]), (float)((((unsigned int)read_buffer[1]) << 8) + (unsigned int)read_buffer[2]) );
						motor_data[ tmp_int ].position_goal = 0.01f * (short int)(((short int)read_buffer[3]<<8) + (short int)read_buffer[4]);
						motor_data[ tmp_int ].state_id = (unsigned int)read_buffer[5];
						motor_data[ tmp_int ].error_id = (unsigned int)read_buffer[6];
						in_state.valid_motor_data = true;
						break;

					default : ;// unknown command
						//return -3;
						//break;
				}
				// še ponastavimo parametre:
				//data_length = 0;
				//command = 0;

			}
			else{
				// error while read data
				//command = 0;
				//data_length = 0;
				//return -2;
				change = -2;
			}
			// še ponastavimo parametre:
			if( !additional_data_in_buffer ){
				data_length = 0;
				command = 0;
			}
		}
	}
	return change;// no new received command and data
}

string SerialPortRobotArm::getDevice() {
	return con_param.device;
}

int SerialPortRobotArm::getBaudRate() {
	return con_param.baud;
}

bool SerialPortRobotArm::isConnected() {
	if( con_param.connected == 1 )
		return true;
	return false;
}

//********************************
//		getData	arm info
//********************************
// popravljeno, netestirano
int SerialPortRobotArm::getArmInfo(ArmInfo &data){
	dataArrived(); // check for new data
	if( !in_state.valid_arm_info ) return -1;

	data.id = arm_info.id;
	data.version = arm_info.version;
	data.name = arm_info.name;
	data.joints = arm_info.joints;
	data.update_rate = arm_info.update_rate;
	data.update_rate_max = arm_info.update_rate_max;

	if(arm_info.id > last_id.arm_info_id){
		last_id.arm_info_id = arm_info.id;
		return 1; // change in data
	}
	return 0; // no change in data
}

//********************************
//		getData arm data
//********************************
// spremenjeno, nepreverjeno
int SerialPortRobotArm::getArmData(ArmData &data){
	dataArrived(); // check for new data
	if( !in_state.valid_arm_data ) return -1;

	data.id = arm_data.id;
	data.state = arm_data.state;
	//data.hand_sensor = arm_data.hand_sensor;
	data.input_voltage = arm_data.input_voltage;
	data.motors_voltage = arm_data.motors_voltage;
	data.motors_current = arm_data.motors_current;

	if(arm_data.id > last_id.arm_data_id){
		last_id.arm_data_id = arm_data.id;
		return 1; // change in data
	}
	return 0; // no change in data
}

int SerialPortRobotArm::getJointInfo(int joint, JointInfo &data){
	dataArrived();
	if( !in_state.valid_motor_info ) return -1;
	if( joint < 0  || joint >= (arm_info.joints)) return -1;

    data = motor_info[joint];

	if( motor_info[joint].id > last_id.motor_info_id[joint] ){
		last_id.motor_info_id[joint] = motor_info[joint].id;
		return 1;
	}

	return 0;
}

//********************************
//		getData	motor data
//********************************
//int SerialPortRobotArm::getJointData(std::vector<SerialPortRobotArm::MotorData> &data){
int SerialPortRobotArm::getJointData(int joint, JointData &data){
	dataArrived();
	if( !in_state.valid_motor_data ) return -1;
	if( joint < 0  || joint >= (arm_info.joints)) return -1;

    data = motor_data[joint];

    //data.dh_position = (((data.position - motor_info[joint].position_min) / (motor_info[joint].position_max - motor_info[joint].position_min)) * (motor_info[joint].dh_max - motor_info[joint].dh_min)) + motor_info[joint].dh_min;
    //data.dh_goal = (((data.position_goal - motor_info[joint].position_min) / (motor_info[joint].position_max - motor_info[joint].position_min)) * (motor_info[joint].dh_max - motor_info[joint].dh_min)) + motor_info[joint].dh_min;
	data.dh_position = data.position; //(((data.position - motor_info[joint].position_min) / (motor_info[joint].position_max - motor_info[joint].position_min)) * (motor_info[joint].dh_max - motor_info[joint].dh_min)) + motor_info[joint].dh_min;


	data.dh_goal = data.position_goal; //(((data.position_goal - motor_info[joint].position_min) / (motor_info[joint].position_max - motor_info[joint].position_min)) * (motor_info[joint].dh_max - motor_info[joint].dh_min)) + motor_info[joint].dh_min;

	//DEBUGMSG("Joint %d data %f g:%f\n", joint, data.dh_position, data.dh_goal);

	if( motor_info[joint].id > last_id.motor_data_id[joint] ){
		last_id.motor_data_id[joint] = motor_info[joint].id;
		return 1;
	}

	return 0;
}

int SerialPortRobotArm::move(int joint, float speed){
	return moveJoint(joint, speed);
}

int SerialPortRobotArm::moveTo(int joint, float speed, float position){
    if( joint < 0  || joint >= (arm_info.joints)) return -1;

    //position = (((position - motor_info[joint].dh_min) * (motor_info[joint].position_max - motor_info[joint].position_min)) / (motor_info[joint].dh_max - motor_info[joint].dh_min)) + motor_info[joint].position_min;

	return moveJointTo(joint, speed, position);
}

//********************************
//		sendData
//********************************
// ---- send data for move ----
// posodobljeno, nepreverjeno (1)
int SerialPortRobotArm::moveJoint(int joint, float speed){
	if(joint < 0 || joint >= arm_info.joints) return -1; // preveri veljavnost id motorjev

	write_buffer[0] = 21; // ukaz: move
	write_buffer[1] = (unsigned char)joint;

	// smer:
	if(speed < 0.0f){
		// negative value (-)
		write_buffer[2]= 1;
		if(speed < -1.0f) speed = 1.0f;
		else speed *= -1;
	}
	else{
		// positive value (+)
		write_buffer[2]=0;
		if(speed > 1.0f) speed = 1.0f;
	}
	// hitrost:
	unsigned int tmp_uint = (unsigned int)(speed * faktor_pwm);
	// we use only lower 16 bits, small end notation
	write_buffer[3]= (unsigned char)(tmp_uint>>8);
	write_buffer[4]= (unsigned char)(tmp_uint);

    DEBUGMSG("Moving joint %d with speed %f\n", joint, speed);

	return writeData(write_buffer, 5);
}

//********************************
//		sendData
//********************************
// ---- send data for move to ----
// posodobljeno, nepreverjeno (1)
int SerialPortRobotArm::moveJointTo(int joint, float speed, float pos){
	if(joint < 0 || joint >= arm_info.joints) return -1; // preveri veljavnost id motorjev

	write_buffer[0] = 22; // ukaz: move to
	if(joint >= FIXJ) write_buffer[1] = (unsigned char)joint-1;
	else write_buffer[1] = (unsigned char)joint;
	write_buffer[2] = 0; // some parameters

	// hitrost:
	if(speed < 0.0f)
		speed *= -1;
	if(speed > 1.0f)
		speed = 1.0f;
	unsigned int tmp_uint = (unsigned int)(speed * faktor_pwm);
	// we use only lower 16 bits, small end notation
	write_buffer[3] = (unsigned char)(tmp_uint>>8);
	write_buffer[4] = (unsigned char)(tmp_uint);

	// pozicija:
	if(pos < motor_info[joint].position_min)
		pos = motor_info[joint].position_min;
	else if(pos > motor_info[joint].position_max)
		pos = motor_info[joint].position_max;

	int tmp = (int)(pos*100.0f);

	write_buffer[5] = (unsigned char)(tmp>>8);
	write_buffer[6] = (unsigned char)(tmp);

	float pos1 = 0.01f * (short int)(((short int)write_buffer[5]<<8) + (short int)write_buffer[6]);

    DEBUGMSG("Moving joint %d to position %f with speed %f %f\n", joint, pos, speed, pos1);

	return writeData(write_buffer, 7);
}

//********************************
//		armInfo
//********************************
// ---- send command for arm info ----
int SerialPortRobotArm::requestArmInfo(){
	write_buffer[0] = 1;
	return writeData(write_buffer, 1);
}

//********************************
//		jointInfo
//********************************
int SerialPortRobotArm::requestJointInfo(int joint){
	if( joint >= arm_info.joints ) return -1;
	write_buffer[0] = 2;
	if(joint >= 0)
			write_buffer[1] = (unsigned char)(joint);
		else
			write_buffer[1] = 0;
	return writeData(write_buffer, 2);
}

//********************************
//		startControl
//********************************
// ---- send command for start ----
int SerialPortRobotArm::startControl(){
	in_state.control_started = 1; //drugače naj bi počakal na potrditev ukaza...!!!
	write_buffer[0] = 16;
	return writeData(write_buffer, 1);
}

//********************************
//		stopControl
//********************************
// ---- send command for stop ----
int SerialPortRobotArm::stopControl(){
	in_state.control_started = 0; //drugače naj bi počakal na potrditev ukaza...!!!
	write_buffer[0] = 17;
	return writeData(write_buffer, 1);
}

//********************************
//		lock (pause)
//********************************
int SerialPortRobotArm::lock(int joint){
	if( joint >= arm_info.joints ) return -1;
	if(in_state.control_started == 1){
		in_state.control_started = 2;
		write_buffer[0] = 18;
		if(joint >= 0)
			write_buffer[1] = (unsigned char)(joint+1);
		else
			write_buffer[1] = 0;
		return writeData(write_buffer, 2);
	}
	return 0;
}

//********************************
//		release (continue)
//********************************
int SerialPortRobotArm::release(int joint){
	if( joint >= arm_info.joints ) return -1;
	if(in_state.control_started == 1){
		in_state.control_started = 2;
		write_buffer[0] = 19;
		if(joint >= 0)
			write_buffer[1] = (unsigned char)(joint+1);
		else
			write_buffer[1] = 0;
		return writeData(write_buffer, 2);
	}
	return 0;
}

int SerialPortRobotArm::setDataRate(int rate) {
	if( rate >= 0 && rate < 51){
		write_buffer[0] = 3;
		write_buffer[1] = (unsigned char)rate;
		return writeData(write_buffer, 2);
	}
	return 0;// no change,
}

int SerialPortRobotArm::getDataRate() {
  	ArmInfo info;

    if (getArmInfo(info) == 0)
        return info.update_rate;
    else return -1;
}

//********************************
//		calibrate
//********************************
// ---- send command to start calibrate arm
int SerialPortRobotArm::calibrate(int joint){
	if( joint >= arm_info.joints ) return -1;
	write_buffer[0] = 4;
	if(joint >= 0)
			write_buffer[1] = (unsigned char)(joint+1);
		else
			write_buffer[1] = 0;
	return writeData(write_buffer, 1);
}

//********************************
//		sendPing
//********************************
//** narejeno po listu!
int SerialPortRobotArm::sendPing(){
	if( con_param.connected == 0 ) return 0;
	write_buffer[0] = 5;
	in_state.ping_data[0] = rand() % 255 +1; // vemo da ne more dobiti vrednost 0...
	in_state.ping_data[1] = rand() % 255 +1;
	write_buffer[1] = in_state.ping_data[0];
	write_buffer[2] = in_state.ping_data[1];
	in_state.sended_ping = 1;

	return writeData(write_buffer, 3);
	//return 1;
}

//********************************
//		rest (foldArm)
//********************************
int SerialPortRobotArm::rest(){
	if( con_param.connected == 0 ) return 0;
	// s tem pogojem zahtevamo, da je roka v stanju normalne uporabe, ne preverjamo pa ostalih stvari (stop,...)
	// saj  se na strani naprave staro stanje (nastavljeni cilji) povozijo
	write_buffer[0] = 20;
	// lahko dodamo še parametre??
	in_state.sended_fold_arm = 1;
	return writeData(write_buffer, 3);
}


bool SerialPortRobotArm::poll() {
    return dataArrived() >= 0;
}

int SerialPortRobotArm::size() {

    return motor_data.size();

}

std::string returnErrorDescription(int error){
	std::string des = "unknown";
	switch(error){
		case   1: des = "Success, command executed."; break;
		case   0: des = "Allready connected (no change)."; break;
		case  -1: des = "ERROR: - device not found"; break;
		case  -2: des = "ERROR: - error while opening the device"; break;
		case  -3: des = "ERROR: - error while getting port parameters"; break;
		case  -4: des = "ERROR: - Speed (Bauds) not recognised"; break;
		case  -5: des = "ERROR: - error while writing port parameters"; break;
		case  -6: des = "ERROR: - error while writing timeout parameters"; break;
		case  -7: des = "ERROR: - cant send command to arm"; break;
		case  -8: des = "ERROR: - no response from arm"; break;
		case  -9: des = "WRITE ERROR for requested command."; break;
		case -10: des = "Arm callibration in progress. Command ignored."; break;
		default : des = "ERROR: - unknown"; break;
	}

	return des;
}

