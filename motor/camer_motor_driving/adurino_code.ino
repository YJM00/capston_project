#include <Wire.h>
#include <MotorDriver.h>  // YFROBOT Motor Driver 라이브러리

#define MOTOR_TYPE YF_IIC_RZ  // I2C 기반 RZ7889 사용
MotorDriver motorDriver(MOTOR_TYPE);

// ✅ 모터 ID 정의
#define M1 1  // 좌측 모터
#define M2 2  // 우측 모터
#define M3 3  // 후방 모터

// ✅ 속도 조정 (왼쪽 쏠림 보정)
#define BASE_SPEED 900  // 기본 속도
#define LEFT_CORRECTION -50  // 왼쪽 모터 보정값 (왼쪽이 더 빨리 돌도록 설정)

// ✅ 장애물 감지 신호 핀 (라즈베리파이와 연결된 핀)
#define STOP_SIGNAL_PIN A2
#define MOVE_SIGNAL_PIN A1

bool obstacleDetected = false;  // 장애물 감지 여부
bool moving = false;  // 현재 모터가 움직이는지 여부

void setup() {
    Serial.begin(115200);  // ✅ 시리얼 모니터 활성화
    Wire.begin();
   
    // ✅ 모터 드라이버 초기화
    motorDriver.begin();
    motorDriver.setPWMFreq(100);

    // ✅ 신호 핀을 입력 모드로 설정
    pinMode(STOP_SIGNAL_PIN, INPUT);
    pinMode(MOVE_SIGNAL_PIN, INPUT);

    Serial.println("🔍 아두이노: 준비 완료. 신호 대기 중...");
}

void loop() {
    int stopSignal = digitalRead(STOP_SIGNAL_PIN);
    int moveSignal = digitalRead(MOVE_SIGNAL_PIN);

    // ✅ 시리얼 모니터 출력
    Serial.print("📡 STOP_SIGNAL_PIN (A2): ");
    Serial.print(stopSignal);
    Serial.print(" | MOVE_SIGNAL_PIN (A1): ");
    Serial.println(moveSignal);

    // ✅ 장애물 감지 시 즉시 정지
    if (stopSignal == HIGH) {
        stopRobot();
        Serial.println("🚨 장애물 감지: 모터 정지");
        obstacleDetected = true;
        moving = false;
    }
    // ✅ 장애물 해제 시 이동 시작
    else if (moveSignal == HIGH && !moving) {
        Serial.println("✅ 장애물 해제: 직진 시작");
        obstacleDetected = false;
        moving = true;
        moveStraightWithPause(BASE_SPEED, 10000);  // 10초 직진, 1초마다 멈춤
    }

    delay(100);  // 신호 확인 주기 (0.1초)
}

// ✅ 10초 동안 직진, 1초마다 멈추는 함수 (왼쪽 쏠림 보정)
void moveStraightWithPause(int speed, int totalDuration) {
    unsigned long startTime = millis();
    while (millis() - startTime < totalDuration) {
        if (digitalRead(STOP_SIGNAL_PIN) == HIGH) {
            stopRobot();
            Serial.println("🚨 이동 중 장애물 감지: 즉시 정지!");
            obstacleDetected = true;
            moving = false;
            return;
        }

        // ✅ 왼쪽 쏠림을 보정하기 위해 좌측 모터 속도를 증가
        moveRobot(speed + LEFT_CORRECTION, -speed, 0);
        Serial.println("🚗 전진 중 (보정 적용)...");

        delay(1000);  // 1초 이동

        stopRobot();
        Serial.println("🛑 잠시 정지...");
        delay(1000);  // 1초 정지
    }

    stopRobot();
    moving = false;
}

// ✅ 모터 속도 설정 함수
void moveRobot(int speedM1, int speedM2, int speedM3) {
    motorDriver.setSingleMotor(M1, speedM1);
    motorDriver.setSingleMotor(M2, speedM2);
    motorDriver.setSingleMotor(M3, speedM3);
}

// ✅ 모터 정지
void stopRobot() {
    moveRobot(0, 0, 0);
}
