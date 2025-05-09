#include <Wire.h>
#include <MotorDriver.h>  // YFROBOT Motor Driver 라이브러리

#define MOTOR_TYPE YF_IIC_RZ
MotorDriver motorDriver(MOTOR_TYPE);

// 모터 ID
#define M1 1  // 좌측
#define M2 2  // 우측
#define M3 3  // 후방

// 속도 및 보정 설정
#define BASE_SPEED 500
#define CORRECTION 50
#define LOOP_DELAY 200  // 각 동작 유지 시간 (ms)

// 엔코더 핀 (A채널만 사용)
#define ENCODER_M1_A 2
#define ENCODER_M2_A 3

volatile long encoderCountM1 = 0;
volatile long encoderCountM2 = 0;

// 엔코더 인터럽트 핸들러
void encoderM1() { encoderCountM1++; }
void encoderM2() { encoderCountM2++; }

void setup() {
  Serial.begin(115200);
  Wire.begin();

  motorDriver.begin();
  motorDriver.setPWMFreq(100);

  pinMode(ENCODER_M1_A, INPUT);
  pinMode(ENCODER_M2_A, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_M1_A), encoderM1, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_M2_A), encoderM2, RISING);

  Serial.println("✅ 전방향 엔코더 보정 주행 시작");
}

void loop() {
  moveWithCorrection("forward", BASE_SPEED);
  delay(500);
  moveWithCorrection("backward", BASE_SPEED);
  delay(500);
  moveWithCorrection("left", BASE_SPEED);     // ← 방향 수정 적용됨
  delay(500);
  moveWithCorrection("right", BASE_SPEED);    // → 방향 수정 적용됨
  delay(500);
  moveWithCorrection("rotate_cw", BASE_SPEED);
  delay(500);
  moveWithCorrection("rotate_ccw", BASE_SPEED);
  delay(500);
  stopRobot();
  delay(1000);
}

// ✅ 보정 주행 함수
void moveWithCorrection(String direction, int baseSpeed) {
  encoderCountM1 = 0;
  encoderCountM2 = 0;
  delay(LOOP_DELAY);

  int speedM1 = 0, speedM2 = 0, speedM3 = 0;

  if (direction == "forward") {
    speedM1 = -baseSpeed;
    speedM2 = +baseSpeed;
    speedM3 = 0;
  } else if (direction == "backward") {
    speedM1 = +baseSpeed;
    speedM2 = -baseSpeed;
    speedM3 = 0;
  } else if (direction == "left") {
    // ✅ 방향 반전 적용됨
    speedM1 = -baseSpeed;
    speedM2 = -baseSpeed;
    speedM3 = +baseSpeed;
  } else if (direction == "right") {
    // ✅ 방향 반전 적용됨
    speedM1 = +baseSpeed;
    speedM2 = +baseSpeed;
    speedM3 = -baseSpeed;
  } else if (direction == "rotate_cw") {
  speedM1 = +baseSpeed;
  speedM2 = +baseSpeed;
  speedM3 = +baseSpeed;
  } else if (direction == "rotate_ccw") {
  speedM1 = -baseSpeed;
  speedM2 = -baseSpeed;
  speedM3 = -baseSpeed;
  }

  // 좌우 보정 적용 (M1 vs M2)
  long diff = encoderCountM1 - encoderCountM2;

  if (diff > 5) {
    speedM1 += CORRECTION;
  } else if (diff < -5) {
    speedM2 -= CORRECTION;
  }

  motorDriver.setSingleMotor(M1, speedM1);
  motorDriver.setSingleMotor(M2, speedM2);
  motorDriver.setSingleMotor(M3, speedM3);

  Serial.print("▶ ");
  Serial.print(direction);
  Serial.print(" | Enc M1=");
  Serial.print(encoderCountM1);
  Serial.print(" | M2=");
  Serial.print(encoderCountM2);
  Serial.print(" | Speed M1=");
  Serial.print(speedM1);
  Serial.print(" | M2=");
  Serial.print(speedM2);
  Serial.print(" | M3=");
  Serial.println(speedM3);
}

// 정지 함수
void stopRobot() {
  motorDriver.setSingleMotor(M1, 0);
  motorDriver.setSingleMotor(M2, 0);
  motorDriver.setSingleMotor(M3, 0);
  Serial.println("⏹ 정지");
}
