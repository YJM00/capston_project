#!/usr/bin/env python3
import rospy
from nav_msgs.msg import Odometry
from geometry_msgs.msg import Quaternion
import tf
import serial
import math

# === 설정 ===
PORT = "/dev/ttyACM0"
BAUD = 115200
WHEEL_RADIUS = 0.04
TICKS_PER_REV = 360
DIST_PER_TICK = 2 * math.pi * WHEEL_RADIUS / TICKS_PER_REV

# 시리얼 포트 열기
ser = serial.Serial(PORT, BAUD, timeout=1)

# ROS 노드 초기화
rospy.init_node("odom_from_serial")
rospy.loginfo("/odom 퍼블리셔 시작")

# 퍼블리셔 및 TF 브로드캐스터
pub = rospy.Publisher("/odom", Odometry, queue_size=50)
br = tf.TransformBroadcaster()

# 상태 변수 초기화
x, y, theta = 0.0, 0.0, 0.0
last_time = rospy.Time.now()
prev_enc = [0, 0, 0]

# 시리얼 데이터 파싱
def parse_encoder(line):
    if not line.startswith("ENC,"):
        return None
    try:
        parts = line.strip().split(",")
        return [int(parts[1]), int(parts[2]), int(parts[3])]
    except:
        return None

rate = rospy.Rate(30)

# === 메인 루프 ===
while not rospy.is_shutdown():
    now = rospy.Time.now()
    stable_time = now - rospy.Duration(0.05)
    line = ser.readline().decode("utf-8", errors="ignore").strip()
    enc = parse_encoder(line)

    if enc is None:
        continue

    dt = (now - last_time).to_sec()
    last_time = now

    d1 = (enc[0] - prev_enc[0]) * DIST_PER_TICK
    d2 = (enc[1] - prev_enc[1]) * DIST_PER_TICK
    d3 = (enc[2] - prev_enc[2]) * DIST_PER_TICK
    dx = (d1 + d2 + d3) / 3.0
    vx = dx / dt if dt > 0 else 0.0
    vth = 0.0

    x += dx * math.cos(theta)
    y += dx * math.sin(theta)
    prev_enc = enc

    quat = tf.transformations.quaternion_from_euler(0, 0, theta)

    # TF 전송
    br.sendTransform((x, y, 0), quat, stable_time, "base_link", "odom")

    # Odometry 메시지 생성
    odom = Odometry()
    odom.header.stamp = stable_time
    odom.header.frame_id = "odom"
    odom.child_frame_id = "base_link"
    odom.pose.pose.position.x = x
    odom.pose.pose.position.y = y
    odom.pose.pose.orientation = Quaternion(*quat)
    odom.twist.twist.linear.x = vx
    odom.twist.twist.angular.z = vth

    pub.publish(odom)
    rospy.loginfo(f"[ODOM] x={x:.2f}, y={y:.2f}, vx={vx:.2f}, t={stable_time.to_sec():.2f}")

    rate.sleep()
