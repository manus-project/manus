
import echolib, random
from manipulator_msgs import ManipulatorDescriptionSubscriber, ManipulatorStateSubscriber, PlanSegment, JointCommand, Plan, PlanPublisher

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

move = PlanPublisher(client, "manipulator0.plan")

while loop.wait(1000):
	plan = Plan()
	plan.segments.append(PlanSegment())
	for j in description.joints:
		c = JointCommand()
		c.speed = 4.0
		c.goal = random.uniform(j.dh_min, j.dh_max)
		print c.goal
		plan.segments[0].joints.append(c)
	print "Sending"
	move.send(plan)
