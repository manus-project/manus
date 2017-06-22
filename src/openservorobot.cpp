#include <cmath>
#include <chrono>
#include <numeric>
#include <unistd.h>

#include <openservo.h>
#include "openservorobot.h"

// le začasno, za debagiranje!
void printServoRegisters(sv* servo)
{
  cout << "\t- type: " << servo->type << endl;          // OpenServo device type
  cout << "\t- subtype: " << servo->subtype << endl;        // OpenServo device subtype
  cout << "\t- version: " << servo->version << endl;        // version number of OpenServo software
  cout << "\t- flags: " << servo->flags << endl;
  cout << "\t- timer: " << servo->timer << endl;          // Timer -­ incremented each ADC sample
  cout << "\t- position: " << servo->position << endl;        // Servo position
  cout << "\t- velocity: " << servo->velocity << endl;        // Servo velocity
  cout << "\t- current: " << servo->current << endl;        // Servo current/power
  cout << "\t- pwm_cw: " << servo->pwm_cw << endl;          // PWM clockwise value
  cout << "\t- pwm_ccw: " << servo->pwm_ccw << endl;        // PWM counter­clockwise value
  //cout << "\t- speed: " << servo->speed << endl;

  // read/write (7 values)
  cout << "\t- seek_position: " << servo->seek_position << endl;      // Seek position
  cout << "\t- seek_velocity: " << servo->seek_velocity << endl;      // Speed seek position
  cout << "\t- voltage: " << servo->voltage << endl;        // Battery/adapter Voltage value
  cout << "\t- curve_delta: " << servo->curve_delta << endl;      // Curve Time delta
  cout << "\t- curve_position: " << servo->curve_position << endl;      // Curve position
  cout << "\t- curve_in_velocity: " << servo->curve_in_velocity << endl;    // Curve in velocity
  cout << "\t- curve_out_velocity: " << servo->curve_out_velocity << endl;    // Curve out velocity

  // read/write protected (9 values)
  cout << "\t- address: " << servo->address << endl;        // TWI address of servo
  cout << "\t- deadband: " << servo->deadband << endl;        // Programmable PID deadband value
  cout << "\t- pgain: " << servo->pgain << endl;          // PID proportional gain
  cout << "\t- dgain: " << servo->dgain << endl;          // PID derivative gain
  cout << "\t- igain: " << servo->igain << endl;          // PID integral gain
  cout << "\t- pwm_freq_divider: " << servo->pwm_freq_divider << endl;    // PWM frequency divider
  cout << "\t- minseek: " << servo->minseek << endl;        // Minimum seek position
  cout << "\t- maxseek: " << servo->maxseek << endl;        // Maximum seek position
  cout << "\t- reverse_seek: " << servo->reverse_seek << endl;      // reverse seek mode
}
// le začasno, za debagiranje!
void OpenServoRobot::printServos()
{
  cout << "scanning port:\n";
  unsigned char adrs[128];
  int num = open_servo.scanPort(adrs);
  cout << "\t- found " << num << " devicess\n";
  cout << "\t- adress:";
  for (int q=0; q<num; q++)
  {
    cout << " " << (unsigned int)adrs[q];
  }
  cout << endl;

  cout << "auto adding servos:\n";
  num = open_servo.scanPortAutoAddServo();
  cout << "\t- added " << num << " servos\n";
  cout << "\t- vector size: " << open_servo.getNumOfServos() << endl;
  for (int q=0, w = open_servo.getNumOfServos(); q<w; q++)
  {
    cout << "* SERVO: " << q << ", adr " << (unsigned int)adrs[q] << endl;
    sv tmp_sv = open_servo.getServo((unsigned int)adrs[q]);
    if (&tmp_sv != NULL)
      printServoRegisters(&tmp_sv);
    //printServoRegisters(&my_servo.getServo((unsigned int)adrs[q]));
  }

}

OpenServoRobot::OpenServoRobot(string path_to_i2c_port)
{
  cout << "** Open servo robot: constructor\n";
  read_rate = 30;
  _state.state = UNKNOWN;
  _description.name = "i2c Robot Manipulator";
  _description.version = 0.2f;

  if (connectTo(path_to_i2c_port))
    _state.state = ACTIVE;

  int num = open_servo.scanPortAutoAddServo();
  cout << "   num. of servos is " << num << endl;

  //printServos();

  loadRobotDescription("lalalend");

  pthread_create(&thread1, 0, startRutine, this);
  pthread_create(&thread_req, 0, startRutineReq, this);
}

OpenServoRobot::~OpenServoRobot()
{
  bool tmp_in_slip;
  pthread_mutex_lock(&sleep_mutex); // potrebno?
  end_thread = true;
  tmp_in_slip = in_slipe;
  pthread_mutex_unlock(&sleep_mutex);
  if (tmp_in_slip)
    pthread_cond_signal(&wake_up_condition);

  pthread_join(thread_req, 0);
  pthread_join(thread1, 0);
}

int OpenServoRobot::connectTo(string path_to_i2c_port)
{
  open_servo.openPort(path_to_i2c_port);
}

servo_info OpenServoRobot::servo_description(int id, int AD_min, int AD_max, int AD_center, float faktor)
{
  servo_info tmp;
  tmp.servo_id = id;
  tmp.AD_min = AD_min;
  tmp.AD_max = AD_max;
  tmp.AD_center = AD_center;
  tmp.faktor = faktor;
  return tmp;
}

int OpenServoRobot::loadRobotDescription(string path_to_file)
{
  int fixed_joint_id = 4;
  // basic joint data and information ... HARDCODED!
  _state.joints.resize(7);
  for (int i = 0; i < 7; i++) {
		_state.joints[i].type = IDLE;
	}

  joint_to_adr.push_back(17);
  joint_to_adr.push_back(18);
  joint_to_adr.push_back(19);
  joint_to_adr.push_back(20);
  //joint_to_adr.push_back(0);
  joint_to_adr.push_back(21);
  joint_to_adr.push_back(22);

  // ti podatki se morajo prebrati iz datoteke in preveriti če se min in max ujemajo s servotom
  //servo_description(int id, int AD_min, int AD_max, int AD_center, float faktor) (faktor in rad.)
  servo.push_back(servo_description(0, 164, 3920, 2264, 0.0009380807));
  servo.push_back(servo_description(1, 540, 3920,  880, 0.0009349769));
  servo.push_back(servo_description(2, 184, 3906, 2836, 0.0009304994));
  servo.push_back(servo_description(3, 16, 3732, 1836, 0.0008597145));
  //servo.push_back(servo_description(4, 0, 0, 0, 0));
  servo.push_back(servo_description(5,  16, 4076, 2196, 0.0008597145));
  servo.push_back(servo_description(6,  480, 2164,  940, 0.0008597145));


	//_state.state = UNKNOWN;

	min_pos.resize(7);
	max_pos.resize(7);
  _description.joints.resize(7);

  //JointDescription joint_description(JointType type, float dh_theta, float dh_alpha, float dh_d, float dh_a, float min, float max)
  // min in max se (lahko) pri rokah razlikujeta
  // Joint 0, min: -1.69, max: 1.73
  //cout << "** Joint 0, min: " << scale_servo_to_joint(servo[0], servo[0].AD_min) << ", max: " << scale_servo_to_joint(servo[0], servo[0].AD_max) << endl;
	_description.joints[0] = joint_description(ROTATION,    0, 90, 111,   0, scale_servo_to_joint(servo[0], servo[0].AD_min), scale_servo_to_joint(servo[0], servo[0].AD_max));
	_description.joints[1] = joint_description(ROTATION,   140,  0,  0, 108, scale_servo_to_joint(servo[1], servo[1].AD_min), scale_servo_to_joint(servo[1], servo[1].AD_max));
	_description.joints[2] = joint_description(ROTATION,   -80, 	0,  0, 112,scale_servo_to_joint(servo[2], servo[2].AD_min), scale_servo_to_joint(servo[2], servo[2].AD_max));
	_description.joints[3] = joint_description(ROTATION,    -50, 	0,  0,  20,scale_servo_to_joint(servo[3], servo[3].AD_min), scale_servo_to_joint(servo[3], servo[3].AD_max));
	_description.joints[4] = joint_description(FIXED,       90, 90,  0,   0, 0, 0);
	_description.joints[5] = joint_description(ROTATION,    0,  0,  0,  0,   scale_servo_to_joint(servo[4], servo[4].AD_min), scale_servo_to_joint(servo[4], servo[4].AD_max));
	_description.joints[6] = joint_description(GRIPPER,     0,  0,  50,  0,  scale_servo_to_joint(servo[5], servo[5].AD_min), scale_servo_to_joint(servo[5], servo[5].AD_max));

  return 1;
}

int OpenServoRobot::lock(int joint)
{
	//return enableMotors(joint);
  return 1;
}

int OpenServoRobot::release(int joint)
{
	//return disableMotors(joint);
  return 1;
}

int OpenServoRobot::rest() {
	//if ( _internal.connection.connected == 0 ) return 0;
	return false;
}

/*
int OpenServoRobot::init()
{

}
*/

int OpenServoRobot::get_file_descriptor()
{
	return -1;
}

bool OpenServoRobot::handle_input()
{
	return true;
}

bool OpenServoRobot::handle_output()
{
	return true;
}

void OpenServoRobot::disconnect()
{

	cout << "Disconnect" << endl;

}


int OpenServoRobot::size() {

	return _description.joints.size();

}


int OpenServoRobot::move(int joint, float position, float speed)
{
  //cout << " move joint " << joint << " pos: " << position << endl;
  // dodamo ukaz v vrsto za pošiljanje...
  bool tmp_in_slip;
  buff_data tmp;
  tmp.action_type = MOVE;
  tmp.joint = joint;
  tmp.speed = speed;
  tmp.position = position;

  pthread_mutex_lock (&q_mutex);
  q_out.push(tmp);
  pthread_mutex_unlock (&q_mutex);

  pthread_mutex_lock(&sleep_mutex);
  tmp_in_slip = in_slipe;
  pthread_mutex_unlock(&sleep_mutex);
  if (tmp_in_slip)
    pthread_cond_signal(&wake_up_condition);

  return 1;
}

void OpenServoRobot::sendMove(int joint, float speed, float position)
{
  //
  int tmp_mot = joint_to_motor(joint);
  if (tmp_mot < 0)
    return;
  sv tmp_sv = open_servo.getServo(joint_to_adr[tmp_mot]);
  if (&tmp_sv == NULL)
    return;
  float pos = ::round(scale_joint_to_servo(tmp_sv, servo[tmp_mot], position));
  // preveri, če je slučajno pos prevelik!!

  //cout << " send move joint " << joint << " motor: " << tmp_mot << " pos: " << (int)pos << endl;
  open_servo.setSeekPossition(joint_to_adr[tmp_mot], (int)pos);

}

ManipulatorDescription OpenServoRobot::describe()
{
	return _description;
}


ManipulatorState OpenServoRobot::state()
{
  //cout << "1";
  /*
  bool izpis = false;
  static int count = 0;
  if ((++count % 500) == 0){
    cout << "** state(): count = " << count << endl;
    izpis = true;
  }
  */
  //posodobi podatke v _state in vrni
  // le ta del je kritičen pri branju v thredu
  int tmp_mot = -1;
  //  for(int q=0; q<_state.joints.size()+1; q++)
  for(int q=0; q<_description.joints.size(); q++)
  {
    tmp_mot = joint_to_motor(q);
    if( tmp_mot < 0)
      continue;
    pthread_mutex_lock(&read_servo_mutex);
    sv tmp_sv = open_servo.getServo(joint_to_adr[tmp_mot]); // če tega servota ni, vrne null
    pthread_mutex_unlock(&read_servo_mutex);
    if (&tmp_sv != NULL)
    {
      _state.joints[q].position = scale_servo_to_joint(servo[tmp_mot], (float)tmp_sv.position);
      _state.joints[q].goal = scale_servo_to_joint(servo[tmp_mot], (float)tmp_sv.seek_position);
      _state.joints[q].speed = 1;
      /*
      if(izpis){
        cout << "** joint: " << q << ", motor: " << tmp_mot << endl;
        cout << "    position = " << _state.joints[q].position << ", ro: " << tmp_sv.position << endl;
        cout << "    goal = " << _state.joints[q].goal << ", ro: " << tmp_sv.seek_position << endl;
        cout << "    goal = " << scale_servo_to_joint(servo[tmp_mot], (float)tmp_sv.seek_position) << endl;
        cout << "    speed = " << _state.joints[q].speed << ", ro: " << tmp_sv.velocity << endl;
      }
      */
    }

  }
	return _state;
}

float OpenServoRobot::scale_servo_to_joint(servo_info si, float ad_pos)
{
  float pos_deg = (ad_pos - si.AD_center) * si.faktor;
  return round(pos_deg * 100.0) / 100.0;
}

float OpenServoRobot::scale_joint_to_servo(sv tmp_sv, servo_info si, float pos)
{
  float pos_ad = (pos/si.faktor) + si.AD_center;
  return pos_ad;
}

int OpenServoRobot::joint_to_motor(int j)
{
	if (j < 0 || j >= _description.joints.size()) return -1;

	if (_description.joints[j].type == FIXED) return -1;

	int m = 0;
	for (int i = 0; i < j; i++) {
		if (_description.joints[i].type != FIXED)
			m++;
	}

	return m;

}

int OpenServoRobot::motor_to_joint(int m)
{
	int mt = m;
	int j = 0;
	while (m > 0) {
		j++; m--;
		if (_description.joints[j].type == FIXED)
			j++;
	}

	return j;

}

void OpenServoRobot::threadRutine()
{
  cout << "** Thread rutine: IO openservo\n";
  sv* servo;
  int action_type;
  buff_data tmp_data;

  bool end_loop = false;

  //while(!end_loop)
  while(true)
  {
    //cout << "** IO thread loop\n";
    pthread_mutex_lock(&q_mutex);
    bool empty = q_out.empty();
    pthread_mutex_unlock(&q_mutex);

    while(!empty)
    {
      pthread_mutex_lock(&q_mutex);
      action_type = q_out.front().action_type;
      pthread_mutex_unlock(&q_mutex);
      switch(action_type)
      {
        case MOVE:
          pthread_mutex_lock(&q_mutex);
          tmp_data = q_out.front();
          q_out.pop();
          pthread_mutex_unlock(&q_mutex);
          //cout << "** Thread IO, MOVE joint: " << tmp_data.joint << endl;
          sendMove(tmp_data.joint, tmp_data.speed, tmp_data.position);
        break;
        case UPDATE_JOINTS:
          pthread_mutex_lock(&q_mutex);
          q_out.pop();
          pthread_mutex_unlock(&q_mutex);

          pthread_mutex_lock(&read_servo_mutex);
          open_servo.updateBasicValuesAllServo();
          pthread_mutex_unlock(&read_servo_mutex);
        break;
      }

      pthread_mutex_lock(&q_mutex);
      empty = q_out.empty();
      pthread_mutex_unlock(&q_mutex);
    }

    // varneje bi blo prej še poslati/počakati oz. sprazniti vrsto
    if(end_loop)
      break;
    //cout << "** IO thread loop -> sleep\n";
    // go to sleep
    pthread_mutex_lock(&sleep_mutex);
    in_slipe = true;
    pthread_mutex_unlock(&sleep_mutex);

    // waiting for signal
    pthread_mutex_lock(&sleep_mutex); // potrebno?
    pthread_cond_wait(&wake_up_condition, &sleep_mutex);
    end_loop = end_thread;
    pthread_mutex_unlock(&sleep_mutex);

    pthread_mutex_lock(&sleep_mutex);
    in_slipe = false;
    pthread_mutex_unlock(&sleep_mutex);
    // tudi če je end_loop true moramo prej sprazniti vrsto
  }

  // je potrebno sprostiti še kakšne vire?
  pthread_exit(NULL);
}

void OpenServoRobot::threadRutineReq()
{
  cout << "** Thread rutine: Requset\n";
  chrono::steady_clock::time_point begin;
  chrono::microseconds interval_micro(1000000/read_rate);
  //cout << "request, micro: " << chrono::duration_cast<chrono::microseconds>(interval_micro).count() << "us" << endl;
  bool end_loop = false;
  //auto zamik = chrono::duration_cast<chrono::microseconds>(begin - chrono::steady_clock::now()).count();
  begin = chrono::steady_clock::now();
  while(true)
  {
    //cout << " Req. thread loop -> request\n";
    updateJoints();
    //cout << " Req. thread loop -> request in que\n";
    pthread_mutex_lock(&sleep_mutex);
    end_loop = end_thread;
    pthread_mutex_unlock(&sleep_mutex);
    if(end_loop)
      break;
    //cout << " Req. thread loop -> sleep\n";
    begin += interval_micro;
    usleep(chrono::duration_cast<chrono::microseconds>(begin - chrono::steady_clock::now()).count());
  }

  pthread_exit(NULL);
}

void OpenServoRobot::updateJoints()
{
  bool tmp_in_slip;

  buff_data tmp;
  tmp.action_type = UPDATE_JOINTS;

  pthread_mutex_lock (&q_mutex);
  q_out.push(tmp);
  pthread_mutex_unlock (&q_mutex);

  pthread_mutex_lock(&sleep_mutex);
  tmp_in_slip = in_slipe;
  pthread_mutex_unlock(&sleep_mutex);
  if (tmp_in_slip)
    pthread_cond_signal(&wake_up_condition);
}

//----------------------------------------------------------------------
//    MAIN
//----------------------------------------------------------------------
int main(int argc, char** argv) {

	SharedClient client = echolib::connect();

	shared_ptr<OpenServoRobot> manipulator = shared_ptr<OpenServoRobot>(new OpenServoRobot("/dev/i2c-1"));

	//echolib::default_loop()->add_handler(manipulator);

	ManipulatorManager manager(client, manipulator);

	while (echolib::wait(50)) {
		manager.update();
	}

	exit(0);
}
