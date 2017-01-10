// version: 0.6

#include "serial.h"
#include "debug.h"
#include <stdio.h>
#include <cmath>

#include <err.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "manipulator.h"

SerialManipulator::SerialManipulator(const string& device, int rate) {

	_internal.connection.device = device.empty() ? string("/dev/ttyACM0") : device;
	_internal.connection.baud = rate;

	_description.version = 0.1f;
	_description.name = "USB Robot Manipulator";
	_internal.update_rate = 0;
	_internal.update_rate_max = 0;

	_state.state = UNKNOWN;
	//manipulator_data.hand_sensor = -1;
	_internal.input_voltage = -1.0f;
	_internal.motors_voltage = -1.0f;
	_internal.motors_current = -1.0f;
	_internal.error_id = 0;


	// in_state
	_internal.configured = 0;
	_internal.valid_manipulator_info = 0;
	_internal.valid_manipulator_data = 0;
	_internal.valid_motor_info = 0;
	_internal.valid_motor_data = 0;

	_internal.synced = 0;

	//factor_pwm = (1<<16)-1;
	factor_pwm = 65535;

	_internal.connection.device = device;
	_internal.connection.baud = rate;


	read_buffer_position = 0;
	read_buffer_available = 0;
	read_buffer_length = BUFFER_SIZE;

	write_buffer_position = 0;
	write_buffer_available = 0;
	write_buffer_length = BUFFER_SIZE;

	serial_connect();

}

SerialManipulator::~SerialManipulator()
{
	serial_disconnect();
}

int SerialManipulator::dynamic_configuration() {

	if (_internal.connection.connected == 0) return 0;

	if (_internal.configured)
		return 0;

	// basic joint data and information ... HARDCODED!
	_state.joints.resize(7);
	_description.joints.resize(7);

	_state.state = UNKNOWN;

	min_pos.resize(7);
	max_pos.resize(7);

	_description.joints[0] = joint_description(ROTATION,    0, 90, 111,   0, -135, 135);
	_description.joints[1] = joint_description(ROTATION,   140,  0,  0, 108,    0, 180);
	_description.joints[2] = joint_description(ROTATION,   -80, 	0,  0, 112, -150, 150);
	_description.joints[3] = joint_description(ROTATION,    -50, 	0,  0,  20,  -60, 60);
	_description.joints[4] = joint_description(FIXED,       90, 90,  0,   0,    0,  0);
	_description.joints[5] = joint_description(ROTATION,    0,  0,  0,  0,    -90, 90);
	_description.joints[6] = joint_description(GRIPPER,     0,  0,  50,  0,    0, 1);

	for (int i = 0; i < 7; i++) {
		if (_description.joints[i].type != FIXED)
			_state.joints[i].goal = -1000;
		_state.joints[i].type = IDLE;
	}

	write_char(1);  // Request manipulator description
	write_char(16);
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
  */
int SerialManipulator::serial_connect() {

	if (_internal.connection.connected) return 0;

	struct termios options;				// Structure with the device's options
	fd = open(_internal.connection.device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd == -1) {
		perror("open");
		return -2;			// If the device is not open, return -2
	}
	//fcntl(fd, F_SETFL, O_NONBLOCK);		// Open the device in nonblocking mode
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
	// Set parameters
	tcgetattr(fd, &options);			// Get the current options of the port
	bzero(&options, sizeof(options));	// Clear all the options
	speed_t         Speed;
	switch (_internal.connection.baud) {			// Set the speed (Bauds)
	case 110  :     Speed = B110; break;
	case 300  :     Speed = B300; break;
	case 600  :     Speed = B600; break;
	case 1200 :     Speed = B1200; break;
	case 2400 :     Speed = B2400; break;
	case 4800 :     Speed = B4800; break;
	case 9600 :     Speed = B9600; break;
	case 19200 :    Speed = B19200; break;
	case 38400 :    Speed = B38400; break;
	case 57600 :    Speed = B57600; break;
	case 115200 :   Speed = B115200; break;
	default : return -4;
	}
	cfsetispeed(&options, Speed);					// Set the baud rate at 115200 bauds
	cfsetospeed(&options, Speed);
	options.c_cflag |= ( CLOCAL | CREAD |  CS8);	// Configure the device : 8 bits, no parity, no control
	options.c_iflag |= ( IGNPAR | IGNBRK );
	options.c_cc[VTIME] = 0;						// Timer unused
	options.c_cc[VMIN] = 0;							// At least on character before satisfy reading
	tcsetattr(fd, TCSANOW, &options);				// Activate the settings

	_internal.connection.connected = 1;

	dynamic_configuration();

	return 1;	// Success

}

int SerialManipulator::serial_disconnect() {
	if (_internal.connection.connected == 0 || fd < 1) return 0; // no change
	close(fd);
	fd = 0;
	_internal.connection.connected = 0;
	return 1; // success
}

int SerialManipulator::joint_to_motor(int j) {

	if (!_internal.configured) return -2;

	if (j < 0 || j >= _description.joints.size()) return -1;

	if (_description.joints[j].type == FIXED) return -1;

	int m = 0;
	for (int i = 0; i < j; i++) {
		if (_description.joints[i].type != FIXED)
			m++;
	}

	return m;

}

int SerialManipulator::motor_to_joint(int m) {

	if (!_internal.configured) return -2;

	int mt = m;
	int j = 0;
	while (m > 0) {
		j++; m--;
		if (_description.joints[j].type == FIXED)
			j++;
	}

	return j;

}

inline float read_float(const char* buffer, size_t* position) {

	unsigned int a = (unsigned int)buffer[position[0]++];
	unsigned int b = (unsigned int)buffer[position[0]++];

	return 0.001f * (float)((a << 8) + b);

}

inline float read_float2(const char* buffer, size_t* position) {

	short int a = (short int)buffer[position[0]++];
	short int b = (short int)buffer[position[0]++];

	return 0.01f * (short int)((a << 8) + b);

}

float SerialManipulator::scale_joint_to_motor(int j, float o) {

	// Round to two decimals
	o = ::round(o * 100.0) / 100.0;

	float x = (o - _description.joints[j].dh_min) / (_description.joints[j].dh_max - _description.joints[j].dh_min);

	return (x * (max_pos[j] - min_pos[j])) + min_pos[j];
}

float SerialManipulator::scale_motor_to_joint(int j, float o) {

	float x = (o - min_pos[j]) / (max_pos[j] - min_pos[j]);

	float r =  (x * (_description.joints[j].dh_max - _description.joints[j].dh_min)) +  _description.joints[j].dh_min;

	return round(r * 100.0) / 100.0;

}


int SerialManipulator::read_data(const char* buffer, size_t buffer_length) {

	size_t position = 0;
	int command;
	size_t message_length = 0;

	while ( true ) {
		if (position + 2 >= buffer_length) {
			position++;
			break;
		}

		// check which instructions has been received and set expected data length
		if ( (int) buffer[position] != 0 || (int) buffer[position+1] != 1) {
			position+=2;
			break;
		} 

		command = (int) buffer[position+2];
		switch (command) {
		case 33:	// manipulator info
			message_length = 6;
			break;
		case 34:	// manipulator data
			message_length = 9;
			break;
		case 35:	// motor info
			message_length = 9;
			break;
		case 36:	// motor data
			message_length = 7;
			break;
		default :   // unknown command
			message_length = 0;
			command = 0;
		}

		position += 3;

		//cout << "Command: " << command << " Len: " << message_length << endl;

		if (command == 0) {
			position++;
			break;
		}

		// check if the remaining data buffer contains the entire message
		if (buffer_length - position >= message_length) {
			_internal.synced  = 1;

			switch ((unsigned char)command) {
			case 33: {	// manipulator info
				unsigned int v1 = (unsigned int)buffer[position++];
				unsigned int v2 = (unsigned int)buffer[position++];

				_description.version = 0.01f * (float)((v1 << 8) + (unsigned int)v2);
				//_description.joints = (unsigned int)buffer[position++] + 1;
				//manipulator_info.update_rate = (unsigned int)buffer[position++];
				//manipulator_info.update_rate_max = (unsigned int)buffer[position++];
				position += 4;
				//position++;
				_internal.valid_manipulator_info = true;
				_internal.configured = 1;

				break;

			} case 34: { // manipulator data
				int state = buffer[position++];
				/*switch (state) {
				case 1: _state.state = CONNECTED; break;
				case 2: _state.state = PASSIVE; break;
				case 3: _state.state = ACTIVE; break;
				case 4: _state.state = CALIBRATION; break;
				default: _state.state = UNKNOWN;
				}*/

				int error_id = (unsigned int) buffer[position++];
				//manipulator_data.hand_sensor = (int)read_buffer[2];
				_internal.input_voltage = read_float(buffer, &position);
				_internal.motors_voltage = read_float(buffer, &position);
				_internal.motors_current = read_float(buffer, &position);
				_internal.valid_manipulator_data = true;
				break;

			} case 35: { // motor description
				int j = motor_to_joint ((unsigned int)buffer[position++]);

				float minp = read_float2(buffer, &position);
				float maxp = read_float2(buffer, &position);

				min_pos[j] = minp;
				max_pos[j] = maxp;

				if (_description.joints[j].type != GRIPPER) {
					_description.joints[j].dh_min = (minp / 180.0) * M_PI;
					_description.joints[j].dh_max = (maxp / 180.0) * M_PI;
					cout << "Setting joint " << j << " limits to " << _description.joints[j].dh_min << " to " << _description.joints[j].dh_max << endl; 
				}

				float limit_voltage = read_float(buffer, &position);
				float limit_current = read_float(buffer, &position);
				_internal.valid_motor_info = true;

				if (j == _description.joints.size() - 1) {
					_internal.configured = 1;
					//cout << "Configure" << endl;
				}

				break;

			} case 36: { // motor state
				int j = motor_to_joint ((unsigned int)buffer[position++]);

				float pos = read_float2(buffer, &position);
				float goal = read_float2(buffer, &position);

				_state.joints[j].position = scale_motor_to_joint(j, pos);
				//_state.joints[j].goal = scale_motor_to_joint(j, goal);
				/*
				if (_state.joints[j].goal != goal) {

				}*/

				if (_state.joints[j].goal < -999) {
					cout << "Setting joint goal " << j << " to " << scale_motor_to_joint(j, goal)<< endl;
					_state.joints[j].goal = scale_motor_to_joint(j, goal);

					if (isinf(_state.joints[j].goal))
						_state.joints[j].goal = _state.joints[j].position;
				}

				/*
				if (_state.joints[j].goal < _description.joints[j].dh_min || _description.joints[j].dh_max < _state.joints[j].goal)
				{
					cout << "Illegal joint " << j << " goal:" << _state.joints[j].goal << " - " << goal << " : " << min_pos[j] << " " << max_pos[j] << endl;
				}*/

				position += 2;
				//motor_data[ tmp_int ].state_id = (unsigned int) buffer[5];
				//motor_data[ tmp_int ].error_id = (unsigned int) read_buffer[6];
				_internal.valid_motor_data = true;

				if (j == _description.joints.size() - 1 && _internal.configured) {
					_state.state = ACTIVE;
				}

				break;
			}
			}

		} else {
			position-=2;
			break;
		}
	}

	return position-1;
}

bool SerialManipulator::is_connected() {
	if ( _internal.connection.connected == 1 )
		return true;
	return false;
}

int SerialManipulator::move(int joint, float pos, float speed) {
	//cout << "Move " << joint << " to " << pos << endl;

	if (joint < 0 || joint >= _description.joints.size()) return -1;

	int m = joint_to_motor(joint);
	if (m < 0) return -2;

	write_char(22); // MoveTo instruction

	write_char((unsigned char)m);
	write_char(0); // unused

	if (speed < 0.0f)
		speed *= -1;
	if (speed > 1.0f)
		speed = 1.0f;
	unsigned int tmp_uint = (unsigned int)(speed * factor_pwm);
	// we use only lower 16 bits, small end notation
	write_char((unsigned char)(tmp_uint >> 8));
	write_char((unsigned char)(tmp_uint));

	pos = scale_joint_to_motor(joint, pos);

	if (pos < min_pos[joint])
		pos = min_pos[joint];
	else if (pos > max_pos[joint])
		pos = max_pos[joint];

	_state.joints[joint].goal = scale_motor_to_joint(joint, pos);


	int tmp = (int)(pos * 100.0f);

	write_char((unsigned char)(tmp >> 8));
	write_char((unsigned char)(tmp));

	return true;
}

int SerialManipulator::start_control() {
	if ( _internal.connection.connected == 0 ) return 0;
	write_char(16);

	return 1;
}

int SerialManipulator::stop_control() {
	if ( _internal.connection.connected == 0 ) return 0;
	write_char(17);
	return 1;
}

int SerialManipulator::lock(int joint) {
	if (joint < 0 || joint >= _description.joints.size() ) return 0;

	int m = joint_to_motor(joint);
	if (m < 0) return 0;

	write_char(18);
	if (joint >= 0)
		write_char((unsigned char)(m));
	else
		write_char(0);
	return 1;
}

int SerialManipulator::release(int joint) {
	if (joint < 0 || joint >= _description.joints.size() ) return 0;
	int m = joint_to_motor(joint);
	if (m < 0) return 0;

	write_char(19);
	if (joint >= 0)
		write_char((unsigned char)(m));
	else
		write_char(0);
	return 1;
}

int SerialManipulator::rest() {
	if ( _internal.connection.connected == 0 ) return 0;

	write_char(20);
	write_char(0);
	write_char(0);

	return true;
}

int SerialManipulator::write_char(unsigned char c) {
	if (write_buffer_available >= write_buffer_length) {
		// TODO: error
	} else {

		write_buffer[write_buffer_available++] = c;

	}
}


bool SerialManipulator::handle() {
	int error = 0;
	ssize_t count = 0;
	while (true) {
		if (read_buffer_available >= read_buffer_length) {

			if (read_buffer_position > 0) {
				for (int i = read_buffer_position; i < read_buffer_available; i++) {
					read_buffer[i-read_buffer_position] = read_buffer[i];
				}

				read_buffer_available -= read_buffer_position;
				read_buffer_position = 0;

			}

		}

		/* Buffer position is increased in process_message() */
		if (read_buffer_available < read_buffer_length) {
			count = ::read(fd, &(read_buffer[read_buffer_available]), read_buffer_length - read_buffer_available);
			if (count == -1) {
				/* If errno == EAGAIN, that means we have read all
				data. So go back to the main loop. */
				if (errno != EAGAIN) {
					error = -1;
				}
				break;
			} else if (count == 0) {
				/* End of file. The remote has closed the
				  connection. */
				//error = -2;
				break;
			}

			read_buffer_available += count;
		}
/*
		for (int i = read_buffer_position; i < read_buffer_available; i++) {
			cout << ((unsigned short) (read_buffer[i]) & 0xFF) << ",";
		}
		cout << endl;
*/
		count = read_data(&(read_buffer[read_buffer_position]), read_buffer_available - read_buffer_position);

		if (count >= 0) {
			read_buffer_position += count;
			//cout << "Progress: " << count << endl;

			if (read_buffer_position == read_buffer_available) {
				read_buffer_position = 0;
				read_buffer_available = 0;
			}
			break;
		}
		else {
			/*if (!_internal.synced && count == -2 && read_buffer_position < read_buffer_available) {
				read_buffer_position++;
			} else {*/
				error = -3;
				break;
			//}
		}

	}

	if (!_internal.configured) {
		dynamic_configuration();
	}

	while (true) {
		count = 0;

		if (write_buffer_position < write_buffer_available) {
			errno = 0;
			count = ::write(fd, &(write_buffer[write_buffer_position]), write_buffer_available - write_buffer_position);

		/*for (int i = write_buffer_position; i < write_buffer_position + count; i++) {
			cout << ((unsigned short) (write_buffer[i]) & 0xFF) << ",";
		}
		cout << endl;*/

			if (count == -1) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					break;
				} else {
					perror("send");
					error = -1;
				}
				tcflush(fd, TCIFLUSH);
				break;
			} else if (count == 0) {
				/* End of file. The remote has closed the
				  connection. */
				error = -2;
				break;
			} else {
				write_buffer_position += count;
				//cout << "Written " << count << endl;
				if (write_buffer_position == write_buffer_available) {
					write_buffer_position = 0;
					write_buffer_available = 0;
				}

				continue;
			}

		} else break;

	}

	return error == 0;

}

int SerialManipulator::get_file_descriptor() {
	return fd;
}

int SerialManipulator::size() {

	return _description.joints.size();

}

ManipulatorDescription SerialManipulator::describe() {
	return _description;
}


ManipulatorState SerialManipulator::state() {
	return _state;
}

void SerialManipulator::disconnect() {

	cout << "Disconnect" << endl;

}


int main(int argc, char** argv) {

	IOLoop loop;

	SharedClient client = connect(loop);

	shared_ptr<SerialManipulator> manipulator = shared_ptr<SerialManipulator>(new SerialManipulator("/dev/ttyACM0"));

	loop.add_handler(manipulator);

	ManipulatorManager manager(client, manipulator);

	while (loop.wait(50)) {
		manager.update();
	}

	exit(0);
}
