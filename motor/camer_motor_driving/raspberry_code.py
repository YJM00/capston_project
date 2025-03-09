import torch
import cv2
import numpy as np
from gpiozero import OutputDevice
from time import sleep
from ultralytics import YOLO  # YOLOv5 라이브러리 사용

# ✅ 모터드라이버의 A12(A2) → 장애물 감지 신호
STOP_SIGNAL_PIN = OutputDevice(12)  # HIGH 시 정지

# ✅ 모터드라이버의 A11(A1) → 장애물 해제 신호
MOVE_SIGNAL_PIN = OutputDevice(11)  # HIGH 시 이동

# ✅ YOLOv5 모델 로드
model = YOLO("yolov5s.pt")  # 사전 학습된 YOLOv5s 모델 사용

# ✅ 웹캠 초기화
camera = cv2.VideoCapture(0)
obstacle_detected = False  # 장애물 감지 여부

def detect_obstacle():
    global obstacle_detected
    while True:
        ret, frame = camera.read()
        if not ret:
            continue

        # YOLOv5를 사용하여 객체 감지
        results = model(frame)

        detected_obstacle = False  # 현재 프레임에서 장애물이 감지되었는지 여부

        for result in results:
            boxes = result.boxes.xyxy.cpu().numpy()  # 감지된 객체의 박스 좌표
            classes = result.boxes.cls.cpu().numpy()  # 감지된 객체의 클래스 ID
            confs = result.boxes.conf.cpu().numpy()  # 신뢰도 값

            for box, class_id, conf in zip(boxes, classes, confs):
                class_id = int(class_id)  # 클래스 ID를 정수형으로 변환
                if class_id == 0 and conf > 0.5:  # ✅ "사람(person)" 클래스만 감지
                    print(f"🚨 사람 감지됨! (신뢰도: {conf:.2f})")
                    detected_obstacle = True
                    x1, y1, x2, y2 = map(int, box)
                    cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 0, 255), 2)  # 빨간색 박스 표시

        if detected_obstacle and not obstacle_detected:
            STOP_SIGNAL_PIN.on()
            MOVE_SIGNAL_PIN.off()
            print("🚨 사람 감지됨: 모터 정지")
            obstacle_detected = True
        elif not detected_obstacle and obstacle_detected:
            STOP_SIGNAL_PIN.off()
            MOVE_SIGNAL_PIN.on()
            print("✅ 사람 없음: 모터 이동")
            obstacle_detected = False

        cv2.imshow("Camera", frame)
        if cv2.waitKey(1) == ord('q'):
            break

    camera.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    detect_obstacle()
