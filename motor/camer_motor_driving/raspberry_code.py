from gpiozero import OutputDevice
import cv2
import time


# ✅ 모터드라이버의 A2(A12) → 사람 감지 시 정지 신호
STOP_SIGNAL_PIN = OutputDevice(12)  # HIGH 시 정지


# ✅ 모터드라이버의 A1(A11) → 사람 없을 때 이동 신호
MOVE_SIGNAL_PIN = OutputDevice(11)  # HIGH 시 이동


# ✅ COCO 데이터셋에서 사람(person)만 감지
class_names = {
    1: "person"
}


# ✅ OpenCV DNN 모델 로드
model = cv2.dnn.readNetFromTensorflow(
    '/home/pi/models/frozen_inference_graph.pb',
    '/home/pi/models/ssd_mobilenet_v2_coco_2018_03_29.pbtxt'
)


# ✅ 웹캠 초기화
camera = cv2.VideoCapture(0)
obstacle_detected = False  # 사람 감지 여부


def detect_obstacle():
    global obstacle_detected
    while True:
        ret, frame = camera.read()
        if not ret:
            continue
        
        image_height, image_width, _ = frame.shape
        model.setInput(cv2.dnn.blobFromImage(frame, size=(300, 300), swapRB=True))
        output = model.forward()


        detected_obstacle = False  # 사람이 감지되었는지 여부


        for detection in output[0, 0, :, :]:
            confidence = detection[2]
            class_id = int(detection[1])


            # ✅ **사람(person, class_id=1)만 감지 + 신뢰도 50% 이상일 때만 반응**
            if class_id == 1 and confidence > 0.5:  
                print(f"🔍 감지된 객체: {class_names.get(class_id, 'Unknown')} (신뢰도: {confidence:.2f})")
                detected_obstacle = True


        # ✅ **사람이 감지되었을 때 모터 정지**
        if detected_obstacle and not obstacle_detected:
            STOP_SIGNAL_PIN.on()
            MOVE_SIGNAL_PIN.off()
            print("🚨 사람 감지: 모터 정지")
            obstacle_detected = True


        # ✅ **사람이 사라졌을 때 다시 이동**
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
