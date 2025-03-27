#include <Wire.h>
#include <MotorDriver.h>  // YFROBOT Motor Driver 라이브러리

#define MOTOR_TYPE YF_IIC_RZ  // I2C 기반 RZ7889 사용
MotorDriver motorDriver(MOTOR_TYPE);

// ✅ 모터 ID 정의
#define M1 1  // 좌측 모터
#define M2 2  // 우측 모터
#define M3 3  // 후방 모터

// ✅ 속도 정의
#define BASE_SPEED 600  // 기본 속도
#define CORRECTION_FACTOR 10  // 보정 값 (차이가 크면 조정)

// ✅ 엔코더 핀 정의
#define ENCODER_M1_A 2   // 좌측 모터 엔코더 A
#define ENCODER_M1_B 3   // 좌측 모터 엔코더 B
#define ENCODER_M2_A 4   // 우측 모터 엔코더 A
#define ENCODER_M2_B 5   // 우측 모터 엔코더 B

// ✅ 엔코더 카운트 변수
volatile long encoderCountM1 = 0;  // 좌측 모터 회전수
volatile long encoderCountM2 = 0;  // 우측 모터 회전수

// ✅ 장애물 감지 신호 핀
#define STOP_SIGNAL_PIN A2
#define MOVE_SIGNAL_PIN A1

bool obstacleDetected = false;  // 장애물 감지 여부
bool moving = false;  // 현재 모터가 움직이는지 여부

void encoderM1() { encoderCountM1++; }
void encoderM2() { encoderCountM2++; }

void setup() {
    Serial.begin(115200);
    Wire.begin();
   
    motorDriver.begin();
    motorDriver.setPWMFreq(100);

    pinMode(STOP_SIGNAL_PIN, INPUT);
    pinMode(MOVE_SIGNAL_PIN, INPUT);

    pinMode(ENCODER_M1_A, INPUT);
    pinMode(ENCODER_M1_B, INPUT);
    pinMode(ENCODER_M2_A, INPUT);
    pinMode(ENCODER_M2_B, INPUT);

    attachInterrupt(digitalPinToInterrupt(ENCODER_M1_A), encoderM1, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCODER_M2_A), encoderM2, RISING);

    Serial.println("🔍 아두이노: 준비 완료. 신호 대기 중...");
}

void loop() {
    int stopSignal = digitalRead(STOP_SIGNAL_PIN);
    int moveSignal = digitalRead(MOVE_SIGNAL_PIN);

    Serial.print("📡 STOP_SIGNAL_PIN (A2): ");
    Serial.print(stopSignal);
    Serial.print(" | MOVE_SIGNAL_PIN (A1): ");
    Serial.println(moveSignal);

    if (stopSignal == HIGH) {
        stopRobot();
        Serial.println("🚨 장애물 감지: 모터 정지");
        obstacleDetected = true;
        moving = false;
    }
    else if (moveSignal == HIGH && !moving) {
        Serial.println("✅ 장애물 해제: 직진 시작");
        obstacleDetected = false;
        moving = true;
        moveStraightWithCorrection(10000);  // 10초 동안 실시간 보정하며 직진
    }

    delay(100);
}

// ✅ 1초 주행 후 1초 멈추는 방식으로 직진 유지
void moveStraightWithCorrection(int totalDuration) {
    encoderCountM1 = 0;
    encoderCountM2 = 0;

    unsigned long startTime = millis();
    while (millis() - startTime < totalDuration) {
        if (digitalRead(STOP_SIGNAL_PIN) == HIGH) {
            stopRobot();
            Serial.println("🚨 이동 중 장애물 감지: 즉시 정지!");
            obstacleDetected = true;
            moving = false;
            return;
        }

        int speedLeft = BASE_SPEED;
        int speedRight = -BASE_SPEED;  // 우측 바퀴는 반대 방향으로 설정

        // ✅ 엔코더 값 비교하여 속도 자동 조절
        int error = encoderCountM1 - encoderCountM2;
        if (error > 5) {
            speedLeft -= CORRECTION_FACTOR;  // 좌측 바퀴 감속
        } else if (error < -5) {
            speedRight += CORRECTION_FACTOR;  // 우측 바퀴 감속
        }

        moveRobot(speedLeft, speedRight, 0);
        Serial.print("🚗 직진 중 | 좌측 속도: ");
        Serial.print(speedLeft);
        Serial.print(" | 우측 속도: ");
        Serial.println(speedRight);

        delay(1000);  // 1초 주행 후 정지

        // ✅ 1초 정지 후 다시 주행
        stopRobot();
        Serial.println("⏸ 일시 정지 (1초)");
        delay(1000);  // 1초 대기 후 다시 이동
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
