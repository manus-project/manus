
import echolib, random, math
from manipulator_msgs import ManipulatorDescriptionSubscriber, ManipulatorStateSubscriber, TrajectorySegment, JointCommand, Trajectory, TrajectoryPublisher, Point, Rotation

description = None

def description_callback(obj):
	global description
	description = obj

def description_state(obj):
	pass

loop = echolib.IOLoop()

client = echolib.Client(loop, "/tmp/echo.sock")

a = ManipulatorDescriptionSubscriber(client, "manipulator0.description", description_callback)
b = ManipulatorStateSubscriber(client, "manipulator0.state", description_state)

move = TrajectoryPublisher(client, "manipulator0.trajectory")

while loop.wait(3000):
	trajectory = Trajectory()
	trajectory.segments.append(TrajectorySegment())
	trajectory.segments[0].location = Point()
	trajectory.segments[0].location.x = random.uniform(10, 300)
	trajectory.segments[0].location.y = random.uniform(-300, 300)
	trajectory.segments[0].location.z = random.uniform(0, 100)

	print trajectory.segments[0].location.x, trajectory.segments[0].location.y, trajectory.segments[0].location.z

	trajectory.segments[0].rotation = Rotation()
	trajectory.segments[0].rotation.x = math.pi
	trajectory.segments[0].rotation.y = 0.0
	trajectory.segments[0].rotation.z = 0.0

	trajectory.segments[0].required = True
	trajectory.segments[0].gripper = 0.5
	trajectory.segments[0].speed = 3.0

	print "Sending"
	move.send(trajectory)
