// ✅ EnableInterrupt 라이브러리 사용한 코드
#include <Wire.h>
#include <MotorDriver.h>
#include <PID_v1.h>
#include <EnableInterrupt.h>   // ✅ 추가

#define MOTOR_TYPE YF_IIC_RZ
MotorDriver motorDriver(MOTOR_TYPE);

// 모터 ID
#define M1 1
#define M2 2
#define M3 3
#define M4 4

#define BASE_SPEED 700
#define CORRECTION_INTERVAL 100

// 엔코더 A채널 핀 (M1~M4)
#define ENCODER_M1_A 3  // D3
#define ENCODER_M2_A 2  // D2
#define ENCODER_M3_A 7  // D7
#define ENCODER_M4_A 9  // D9

volatile long encM1 = 0, encM2 = 0, encM3 = 0, encM4 = 0;

// 슬라이더 핀
#define STEP_PIN 10
#define DIR_PIN 11
#define SLIDER_TRIGGER_PIN 12

#define STEPS_PER_MM 400L
#define HEIGHT_2F_MM 300
#define HEIGHT_2F_STEPS (HEIGHT_2F_MM * STEPS_PER_MM)

long current_steps = 0;
bool movingTo2F = true;
bool isMoving = false;
bool lastTriggerState = LOW;

// PID 변수
double diff12_input = 0, output12 = 0, setpoint12 = 0;
double diff34_input = 0, output34 = 0, setpoint34 = 0;

PID pid12(&diff12_input, &output12, &setpoint12, 1.0, 0.5, 0.1, DIRECT);
PID pid34(&diff34_input, &output34, &setpoint34, 1.0, 0.5, 0.1, DIRECT);

void encoderM1() { encM1++; }
void encoderM2() { encM2++; }
void encoderM3() { encM3++; }
void encoderM4() { encM4++; }

void setup() {
  Serial.begin(115200);
  Wire.begin();
  motorDriver.begin();
  motorDriver.setPWMFreq(100);

  pinMode(ENCODER_M1_A, INPUT);
  pinMode(ENCODER_M2_A, INPUT);
  pinMode(ENCODER_M3_A, INPUT);
  pinMode(ENCODER_M4_A, INPUT);

  enableInterrupt(ENCODER_M1_A, encoderM1, RISING);
  enableInterrupt(ENCODER_M2_A, encoderM2, RISING);
  enableInterrupt(ENCODER_M3_A, encoderM3, RISING);
  enableInterrupt(ENCODER_M4_A, encoderM4, RISING);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(SLIDER_TRIGGER_PIN, INPUT);

  pid12.SetMode(AUTOMATIC);
  pid34.SetMode(AUTOMATIC);
  pid12.SetOutputLimits(-100, 100);
  pid34.SetOutputLimits(-100, 100);

  delay(1000);
  Serial.println("🔍 시스템 준비 완료");
}

void loop() {
  bool triggerState = digitalRead(SLIDER_TRIGGER_PIN);

  if (!isMoving && triggerState == HIGH && lastTriggerState == LOW) {
    Serial.println("📥 슬라이더 트리거 신호 감지됨 → 슬라이더 동작 시작");
    stopRobot();

    if (movingTo2F) {
      moveToWithLog("1층", "2층", HEIGHT_2F_STEPS);
    } else {
      moveToWithLog("2층", "1층", 0);
    }

    movingTo2F = !movingTo2F;
    delay(500);
  }

  lastTriggerState = triggerState;

  if (!isMoving) {
    moveForwardWithPID(CORRECTION_INTERVAL);
  } else {
    stopRobot();
  }
}

void moveToWithLog(String from, String to, long target_steps) {
  isMoving = true;
  long steps_to_move = target_steps - current_steps;
  int dir;

  Serial.println("====================================");
  Serial.print("📍 From: "); Serial.println(from);
  Serial.print("🎯 To: "); Serial.println(to);
  Serial.print("📏 current_steps: "); Serial.println(current_steps);
  Serial.print("📌 target_steps: "); Serial.println(target_steps);
  Serial.print("➡ 이동 스텝 수: "); Serial.println(steps_to_move);

  if (steps_to_move > 0) {
    dir = HIGH;
    Serial.println("↑ 위로 이동");
  } else {
    dir = LOW;
    Serial.println("↓ 아래로 이동");
    steps_to_move = -steps_to_move;
  }

  digitalWrite(DIR_PIN, dir);
  Serial.println("🚀 슬라이더 이동 시작");

  for (long i = 0; i < steps_to_move; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(100);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(100);
  }

  current_steps = target_steps;
  isMoving = false;
  Serial.print("✅ 이동 완료 → current_steps = ");
  Serial.println(current_steps);
}

void moveForwardWithPID(int interval) {
  encM1 = encM2 = encM3 = encM4 = 0;

  int s1 = -BASE_SPEED;
  int s2 = -BASE_SPEED;
  int s3 = BASE_SPEED + 80;
  int s4 = BASE_SPEED + 80;

  diff12_input = encM1 - encM2;
  diff34_input = encM3 - encM4;

  pid12.Compute();
  pid34.Compute();

  s1 += output12;
  s2 -= output12;
  s3 += output34;
  s4 -= output34;

  motorDriver.setSingleMotor(M1, s1);
  motorDriver.setSingleMotor(M2, s2);
  motorDriver.setSingleMotor(M3, s3);
  motorDriver.setSingleMotor(M4, s4);

  Serial.print("▶ PID 직진 | Enc M1="); Serial.print(encM1);
  Serial.print(" M2="); Serial.print(encM2);
  Serial.print(" M3="); Serial.print(encM3);
  Serial.print(" M4="); Serial.print(encM4);
  Serial.print(" | PID12="); Serial.print(output12);
  Serial.print(" | PID34="); Serial.println(output34);

  delay(interval);
}

void stopRobot() {
  motorDriver.setSingleMotor(M1, 0);
  motorDriver.setSingleMotor(M2, 0);
  motorDriver.setSingleMotor(M3, 0);
  motorDriver.setSingleMotor(M4, 0);
  Serial.println("⏹ 정지 완료");
}