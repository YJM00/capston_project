#include <Wire.h>
#include <MotorDriver.h>  // YFROBOT Motor Driver 라이브러리


#define MOTOR_TYPE YF_IIC_RZ  // IIC 기반 RZ7889 사용


// ✅ 올바르게 객체 생성
MotorDriver motorDriver(MOTOR_TYPE);


// 엔코더 핀 정의
#define ENCODER_M1_A 2   // D2 (인터럽트 사용)
#define ENCODER_M1_B 3   // D3 (인터럽트 사용)
#define ENCODER_M2_A 4   // 일반 핀
#define ENCODER_M2_B 5   // 일반 핀
#define ENCODER_M3_A 6   // 일반 핀
#define ENCODER_M3_B 7   // 일반 핀


// 모터 ID 정의
#define M1 1  // 좌측 모터
#define M2 2  // 우측 모터
#define M3 3  // 후방 모터


// ✅ 속도 조정 (부드러운 동작을 위한 균형 조정)
#define LOW_SPEED 1200  // 낮은 속도
#define MID_SPEED 1500  // 중간 속도
#define HIGH_SPEED 1800  // 높은 속도


void setup() {
    Serial.begin(115200);
    Wire.begin();
   
    // ✅ 모터 드라이버 초기화
    motorDriver.begin();
    motorDriver.setPWMFreq(100); // PWM 주파수 설정 (7.4V 기준 최적화)


    Serial.println("로봇 동작 시작...");
}


void loop() {
    // ✅ 다양한 동작을 반복 실행


    moveForward(LOW_SPEED);  // 전진
    delay(2000);
    stopRobot();
    delay(500);


    moveBackward(LOW_SPEED); // 후진
    delay(2000);
    stopRobot();
    delay(500);


    rotateClockwise(LOW_SPEED); // 시계 방향 회전
    delay(2000);
    stopRobot();
    delay(500);


    rotateCounterClockwise(LOW_SPEED); // 반시계 방향 회전
    delay(2000);
    stopRobot();
    delay(500);


    moveLeft(LOW_SPEED); // 좌측 이동 (수정됨)
    delay(2000);
    stopRobot();
    delay(500);


    moveRight(LOW_SPEED); // 우측 이동 (수정됨)
    delay(2000);
    stopRobot();
    delay(500);


    moveDiagonalLeftForward(LOW_SPEED); // 좌측 대각선 전진
    delay(2000);
    stopRobot();
    delay(500);


    moveDiagonalRightForward(LOW_SPEED); // 우측 대각선 전진
    delay(2000);
    stopRobot();
    delay(500);


    moveDiagonalLeftBackward(LOW_SPEED); // 좌측 대각선 후진
    delay(2000);
    stopRobot();
    delay(500);


    moveDiagonalRightBackward(LOW_SPEED); // 우측 대각선 후진
    delay(2000);
    stopRobot();
    delay(500);


    Serial.println("로봇 동작 반복 중...");
}


// ✅ 모터 속도 설정 함수
void moveRobot(int speedM1, int speedM2, int speedM3) {
    motorDriver.setSingleMotor(M1, speedM1);
    motorDriver.setSingleMotor(M2, speedM2);
    motorDriver.setSingleMotor(M3, speedM3);
}


// ✅ 기본 이동 동작
void moveForward(int speed) { moveRobot(speed, -speed, 0); }  // ✅ 전진
void moveBackward(int speed) { moveRobot(-speed, speed, 0); }  // ✅ 후진
void rotateClockwise(int speed) { moveRobot(speed, speed, speed); }  // ✅ 시계 방향 회전
void rotateCounterClockwise(int speed) { moveRobot(-speed, -speed, -speed); }  // ✅ 반시계 방향 회전


// ✅ 좌/우 이동 방향 수정
void moveLeft(int speed) { moveRobot(-speed, -speed, speed); }  // ✅ 좌측 이동 수정
void moveRight(int speed) { moveRobot(speed, speed, -speed); }  // ✅ 우측 이동 수정


// ✅ 대각선 이동
void moveDiagonalLeftForward(int speed) { moveRobot(0, -speed, speed); }  // 좌측 대각선 전진
void moveDiagonalRightForward(int speed) { moveRobot(speed, 0, -speed); }  // 우측 대각선 전진
void moveDiagonalLeftBackward(int speed) { moveRobot(-speed, 0, speed); }  // 좌측 대각선 후진
void moveDiagonalRightBackward(int speed) { moveRobot(0, speed, -speed); }  // 우측 대각선 후진


void stopRobot() { moveRobot(0, 0, 0); }  // 정지
