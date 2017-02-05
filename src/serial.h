
#ifndef __SERIALPORT_MANIPULATOR_H
#define __SERIALPORT_MANIPULATOR_H

#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <termios.h>
#include <string.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <string>
#include <vector>

#include "manipulator.h"

#define BUFFER_SIZE 128

class SerialManipulator : public Manipulator, public IOBase {
public:
	SerialManipulator(const string& device, int rate = 19200);
	~SerialManipulator();

	virtual int lock(int joint = -1);
	virtual int release(int joint = -1);
	virtual int rest();
	
    virtual int size();
	virtual int move(int joint, float speed, float position);	

	virtual ManipulatorDescription describe();
	virtual ManipulatorState state();

	virtual int get_file_descriptor();
	virtual bool handle_output();
	virtual bool handle_input();
	virtual void disconnect();

private:

	//virtual int calibrate(int joint = -1);		// start calibrate the manipulator
	virtual int start_control();					// start controling manipulator
	virtual int stop_control();					// stop controling manipulator

	int serial_connect();						// connect to device
	int serial_disconnect();					// Close the current device
	bool is_connected();

	int write_char(unsigned char data);
	int read_data (const char *buffer, size_t buffer_length);

	int closeConnection();

	int request_motor_info(int motor);
	int dynamic_configuration();	//

	int joint_to_motor(int j);
	int motor_to_joint(int m);

	float scale_joint_to_motor(int j, float o);
	float scale_motor_to_joint(int j, float o);

	struct InternalState {

		bool valid_manipulator_info;
		bool valid_manipulator_data;
		bool valid_motor_info;
		bool valid_motor_data;

		int connection_started;
		int configured;	
		int device_state;		// perceived stare of the device

		int update_rate;
		int update_rate_max;

		float input_voltage;
		float motors_voltage;
		float motors_current;

		int error_id;

		int synced;

		// parametri povezave
		struct ConnectionParam {
			bool new_parameters;
			int connected;		// 0->false, 1->true; 2->new parameters, reconnect
			string device;
			unsigned int baud; // 19200
		} connection;

	} _internal;

	ManipulatorDescription _description;
    ManipulatorState _state;
    std::vector<float> min_pos;
    std::vector<float> max_pos;


    int write_buffer_position;
    int read_buffer_position;

    int write_buffer_available;
    int read_buffer_available;

    int write_buffer_length;
    int read_buffer_length;

	char write_buffer[BUFFER_SIZE];	// send data to dev.
	char read_buffer[BUFFER_SIZE];	// read data from dev.

	unsigned int factor_pwm; // sending 16 bit data -> values from 0 to 2^16-1
	int fd;
};


#endif

