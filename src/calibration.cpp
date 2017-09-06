#include <stdio.h>
#include <unistd.h>
#include <termios.h>

#include <iostream>
#include <fstream>
#include <sys/select.h>
#include <openservo.h>
#include <vector>
#include <algorithm>
#include <yaml-cpp/yaml.h>

using namespace std;

float hardcoded_factors[] = {0.0009380807, 0.0009349769, 0.0009304994, 0.0008597145, 0.0008597145, 0.0008597145};

typedef struct motor_calibration {
	int min;
	int max;
	int center;
	float factor;
	int addr;
	int id;
	int position;
	int delta;
    vector<int> measurements;
} motor_calibration;

bool compare (const motor_calibration& i, const motor_calibration& j) { return (i.id < j.id); }

vector<motor_calibration> motors;

int wait_key() {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(STDIN_FILENO, &readSet);
    struct timeval tv = {0, 25};
    if (select(STDIN_FILENO+1, &readSet, NULL, NULL, &tv) < 0) perror("select");

    if (!FD_ISSET(STDIN_FILENO, &readSet)) return -1;
    unsigned char c;
    c = getchar();

    return c;
}

void print_motor(const motor_calibration& motor) {
	cout << "ID: " << motor.id << " Addr: " << motor.addr << " Pos: "
		<< motor.position << " Min:" << motor.min << " Max:" << motor.max << endl;
}

int update_state(OpenServo& os, vector<motor_calibration>& motors) {

	os.updateServoValuesAllServo();

	for (int i = 0; i < motors.size(); i++) {

		if (motors[i].id > -1)
			continue;

		sv status = os.getServo(motors[i].addr);

		motors[i].measurements.push_back(status.position);

		if (motors[i].measurements.size() > 20) {

			std::nth_element(motors[i].measurements.begin(), motors[i].measurements.begin() + motors[i].measurements.size()/2, motors[i].measurements.end());
            int position = motors[i].measurements[motors[i].measurements.size()/2];

			motors[i].min = std::min(motors[i].min, position);
			motors[i].max = std::max(motors[i].max, position);

			motors[i].delta += abs(motors[i].position - position);

			motors[i].position = position;
			motors[i].center = position;

			print_motor(motors[i]);

			motors[i].measurements.clear();

		}
	}

}

int main(int argc, char** argv) {

	OpenServo os;
	struct termios old_tio, new_tio;
	if (argc < 2 || !os.openPort(argv[1])) {
		cout << "Unable to connect to i2c bus" << endl;
		return -1;
	}

	cout << "Welcome to OpenServo manipulator calibration utility" << endl;

	unsigned char adrs[128];
	int n = os.scanPort(adrs);
	cout << "Found " << n << " motors\n";

  	os.scanPortAutoAddServo();

	if (argc < 3) {

		motors.resize(n);
		for (int i = 0; i < n; i++) {
			motors[i].min = 1000000;
			motors[i].max = -1000000;
			motors[i].center = 0;
			motors[i].factor = 0;
			motors[i].addr = (int) adrs[i];
			motors[i].id = -1;
		}


	    /* get the terminal settings for stdin */
	    tcgetattr(STDIN_FILENO,&old_tio);
	    /* we want to keep the old setting to restore them a the end */
	    new_tio = old_tio;
	    /* disable canonical mode (buffered i/o) and local echo */
	    new_tio.c_lflag &=(~ICANON & ~ECHO);
	    /* set the new settings immediately */
	    tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);

		int m = 0;
		while (m < n) {
			cout << "Please move motor " << m << " to both extreme positions and then to center position and press any key." << endl;

			while (wait_key() < 0) {
				update_state(os, motors);
			}

			int candidate = -1;
			int max_delta = 0;

			for (int i = 0; i < n; i++) {
				int delta = motors[i].delta;
				motors[i].delta = 0;

				if (motors[i].id > -1)
					continue;

				if (delta > max_delta) {
					max_delta = delta;
					candidate = i;
				}
			}

			motors[candidate].id = m;
			m++;
		}

	    /* restore the former settings */
	    tcsetattr(STDIN_FILENO,TCSANOW,&old_tio);


	} else {

	  YAML::Node doc = YAML::LoadFile(argv[2]);

      cout << "Loaded " << doc.size() << endl;
	  for (int i = 0; i < doc.size(); i++) {
	    motor_calibration d;

		d.id = i;
	    d.addr = doc[i]["id"].as<int>();
	    d.min = doc[i]["min"].as<int>();
	    d.max = doc[i]["max"].as<int>();
	    d.center = doc[i]["center"].as<int>();
	    d.factor = doc[i]["factor"].as<float>();

	    motors.push_back(d);
	  }

	}

	cout << "Writing data to motors" << endl;
	for (int i = 0; i < motors.size(); i++) {

		motors[i].max = min(motors[i].max, 4096 - 16);
		motors[i].min = max(motors[i].min, 16);

		cout << "Writing data to motor " << i << endl;
		os.writeEnable(motors[i].addr);
		os.setMaxSeek(motors[i].addr, motors[i].max);
		os.setMinSeek(motors[i].addr, motors[i].min);
		os.writeDisable(motors[i].addr);
		os.registerSave(motors[i].addr);

	}

	cout << "Verifying ... " << endl;
	os.updateServoValuesAllServo();

	for (int i = 0; i < motors.size(); i++) {

		sv s = os.getServo(motors[i].addr);
		cout << "Motor " << motors[i].addr << " min: " << s.minseek << " max: " << s.maxseek << endl;

	}

	std::sort(motors.begin(), motors.end(), compare);

	YAML::Node sequence;  // starts out as null

	for (int i = 0; i < n; i++) {
		YAML::Node motor;
		motor["id"] = motors[i].addr;
		motor["min"] = motors[i].min;
		motor["max"] = motors[i].max;
		motor["center"] = motors[i].center;
		motor["factor"] = hardcoded_factors[i];
		sequence.push_back(motor);
	}

	cout << sequence << endl;

}
