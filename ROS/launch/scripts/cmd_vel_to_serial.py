#!/usr/bin/env python3
import rospy
from geometry_msgs.msg import Twist
import serial
import time

time.sleep(1.0)

try:
    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
    time.sleep(2.0)
    rospy.loginfo("Serial port /dev/ttyACM0 opened")
except serial.SerialException as e:
    rospy.logerr(f"Failed to open serial port: {e}")
    exit(1)

last_send_time = 0
send_interval = 0.1  # 10Hz

def callback(msg):
    global last_send_time
    now = time.time()
    if now - last_send_time >= send_interval:
        vx = msg.linear.x
        vy = msg.linear.y
        omega = msg.angular.z
        data = f"{vx:.3f},{vy:.3f},{omega:.3f}\n"
        try:
            ser.write(data.encode('utf-8'))
            ser.flush()
            rospy.loginfo(f"[ROS→Arduino] Sent: {data.strip()}")
        except serial.SerialException as e:
            rospy.logerr(f"[ERROR] Serial write failed: {e}")
        last_send_time = now

def main():
    rospy.init_node('cmd_vel_to_serial')
    rospy.Subscriber("/cmd_vel", Twist, callback)
    rospy.loginfo("cmd_vel_to_serial node started")
    rospy.spin()

if __name__ == "__main__":
    main()
