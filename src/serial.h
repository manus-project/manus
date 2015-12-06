// version: 0.6

#ifndef __SERIALPORT_ARM_H
#define __SERIALPORT_ARM_H

// Include for windows
#if defined (_WIN32) || defined(_WIN64)
    // Accessing to the serial port under Windows
    #include <windows.h>
#else
    // Used for TimeOut operations
    #include <sys/time.h>
    #include <stdlib.h>
    #include <sys/types.h>
    #include <sys/shm.h>
    #include <termios.h>
    #include <string.h>
    #include <iostream>
    // File control definitions
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
#endif

#include <string>
#include <vector>

#include "arm.h"

class SerialPortRobotArm : public RobotArm {
public:
    SerialPortRobotArm(const string& device, int rate = 19200);
    ~SerialPortRobotArm();
	
	virtual int connect();						// connect to device
    virtual int disconnect();					// Close the current device
	virtual bool poll();
	virtual bool isConnected();
	string getDevice();
    int getBaudRate();

    int setDataRate(int rate);
    int getDataRate();

	virtual int calibrate(int joint = -1);		// start calibrate the arm
	virtual int startControl();					// start controling arm
	virtual int stopControl();					// stop controling arm
	virtual int lock(int joint = -1);			// pause controling
	virtual int release(int joint = -1);		// coninue controling
	virtual int rest();							// fold arm on preeset (safe) position
	
	virtual int move(int joint, float speed);
	virtual int moveTo(int joint, float speed, float position);

    virtual int size();

	virtual int getArmInfo(ArmInfo &data);					// get info abaut arm
	virtual int getArmData(ArmData &data);					// get data abaut arm
	virtual int getJointInfo(int joint, JointInfo &data);	// get info abaut joint
	virtual int getJointData(int joint, JointData &data);	// get data abaut joint
	
	std::string returnErrorDescription(int error);
	
private:
	//__________________________________
    // ::: Read and write operations :::
    char readChar(char *pByte);									// Read a character
    char writeData(const void *Buffer, const unsigned int NbBytes);	// Write an array of bytes
    int readData (void *Buffer,unsigned int MaxNbBytes); 			// Read an array of byte (with timeout)
	
	//__________________________
    // ::: Special operation :::
    void flushReceiver();	// Empty the received buffer
    int peek();				// Return the number of bytes in the received buffer
	int closeConnection();
    
    // ::: Branje in dekodiranje prijetih podatkov :::
	int dataArrived();  // prevri ali so prispeli novi podatki
	
	// ::: Ostale privatne metode :::
	int moveJoint(int joint, float speed);
	int moveJointTo(int joint, float speed, float position);
	int requestArmInfo();
	int requestJointInfo(int joint);
	int sendPing();
	int setDefaultParameters();
	int setArmParametersDynamic();	//
	
    //________________________
    // ::: private varible :::
    // zapomnimo si id zadnje poslane informacije uporabniku
    struct CheckId{
    	unsigned int arm_info_id;
    	unsigned int arm_data_id;
		std::vector<unsigned int> motor_info_id;
		std::vector<unsigned int> motor_data_id;
    } last_id;
	
	struct InternalState{
		bool valid_arm_info;
		bool valid_arm_data;
		bool valid_motor_info;
		bool valid_motor_data;
		int sended_start;		// -1->rejected, 0->false, 1->true, 2->true & confirmed
		int sended_stop;		// -1->rejected, 0->false, 1->true, 2->true & confirmed
		int sended_pause;		// -1->rejected, 0->false, 1->true, 2->true & confirmed
		int sended_continue;	// -1->rejected, 0->false, 1->true, 2->true & confirmed
		int sended_fold_arm;	// -1->rejected, 0->false, 1->true, 2->true & confirmed
		int sended_callibration;	// -1->rejected, 0->false, 1->true, 2->true & confirmed
		int sended_ping;			// -1->rejected, 0->false, 1->true, 2->true & confirmed
		char ping_data[2];
		
		int connection_started;
		int control_started;	// 0->false, 1->true, 2->pause
		int device_state;		// stanje zunanje naprave z naše strani...
	} in_state;
	
	// parametri povezave
	struct ConnectionParam{
		bool new_parameters;
		int connected;		// 0->false, 1->true; 2->new parameters, reconnect
		string device;
		unsigned int baud; // 19200
		//optional
		/*					// defaults param.
		char dataLength; 	// 8bits
		bool parity; 		// false (no parity)
		char stopBit; 		// 1 stop bit
		char comProcedure; 	// full duplex
		*/
	} con_param;
	
	// privatne spremenljivke struktur, v njih hranimo zadnje prejete podatke
    //ConnectionParam con_param;
	ArmInfo arm_info;
	ArmData arm_data;
	vector<JointInfo> motor_info;
	vector<JointData> motor_data;
	//confrmedCommand conf_command;
    
    unsigned char write_buffer[64];	// send data to dev.
    unsigned char read_buffer[128];	// read data from dev.
    
	bool additional_data_in_buffer;
    char command;
	int data_length;
	
	unsigned int faktor_pwm; // pošiljamo 16 bitne podatke.... -> vrednosti: 0 do 2^16-1
	
#if defined(_WIN32) || defined(_WIN64)
	HANDLE hSerial;
	COMSTAT com_stat;
	DWORD dw_errors;
	//COMMTIMEOUTS    timeouts;
	class TimeOut {
	public:

		// Constructor
		TimeOut();

		// Init the timer
		void                InitTimer();

		// Return the elapsed time since initialization
		unsigned long int   ElapsedTime_ms();

	private:    
		struct timeval      PreviousTime;
	};
#else
    int fd;
#endif
};


#endif

