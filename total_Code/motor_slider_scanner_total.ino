#include <Wire.h>
#include <MotorDriver.h>
#include <PID_v1.h>

#define MOTOR_TYPE YF_IIC_RZ
MotorDriver motorDriver(MOTOR_TYPE);

// 모터 ID 정의
#define M1 1
#define M2 2
#define M3 3
#define M4 4

#define BASE_SPEED 700
#define CORRECTION_INTERVAL 100

// 엔코더 핀
#define ENCODER_M1_A 2
#define ENCODER_M2_A 3
#define ENCODER_M3_A 4
#define ENCODER_M4_A 5
volatile long encM1 = 0, encM2 = 0, encM3 = 0, encM4 = 0;

// 슬라이더 설정
#define STEP_PIN 10
#define DIR_PIN 11
#define BUTTON_PIN A0

#define STEPS_PER_MM 400L
#define HEIGHT_2F_MM 300
#define HEIGHT_2F_STEPS (HEIGHT_2F_MM * STEPS_PER_MM)

long current_steps = 0;
bool lastButtonState = HIGH;
bool movingTo2F = true;
bool isMoving = false;  // 슬라이더 이동 중 여부

// 바코드 스캐너 제어
const int ledControl = 12;
unsigned long lastToggleTime = 0;
bool ledState = false;

// PID 변수
double diff12_input = 0, output12 = 0, setpoint12 = 0;
double diff34_input = 0, output34 = 0, setpoint34 = 0;

PID pid12(&diff12_input, &output12, &setpoint12, 1.0, 0.5, 0.1, DIRECT);
PID pid34(&diff34_input, &output34, &setpoint34, 1.0, 0.5, 0.1, DIRECT);

// 엔코더 인터럽트
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

  attachInterrupt(digitalPinToInterrupt(ENCODER_M1_A), encoderM1, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_M2_A), encoderM2, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_M3_A), encoderM3, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_M4_A), encoderM4, RISING);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ledControl, OUTPUT);

  pid12.SetMode(AUTOMATIC);
  pid34.SetMode(AUTOMATIC);
  pid12.SetOutputLimits(-100, 100);
  pid34.SetOutputLimits(-100, 100);

  delay(1000);
  Serial.println("🔍 시스템 준비 완료");
}

void loop() {
  if (!isMoving) {
    moveForwardWithPID(CORRECTION_INTERVAL);
  } else {
    stopRobot();
  }

  handleSliderMovement();
  handleBarcodeScannerToggle();
}

// 🔦 바코드 스캐너 토글
void handleBarcodeScannerToggle() {
  unsigned long currentTime = millis();
  if (currentTime - lastToggleTime >= 100) {
    ledState = !ledState;
    digitalWrite(ledControl, ledState ? HIGH : LOW);
    lastToggleTime = currentTime;

    Serial.print("🔦 스캐너 제어 상태 (ledState): ");
    Serial.println(ledState ? "ON (HIGH)" : "OFF (LOW)");

    int actualPinState = digitalRead(ledControl);
    Serial.print("📟 실제 D12 핀 상태: ");
    Serial.println(actualPinState == HIGH ? "HIGH" : "LOW");
  }
}

// 🛎 슬라이더 버튼 처리
void handleSliderMovement() {
  bool buttonState = digitalRead(BUTTON_PIN);

  if (!isMoving && lastButtonState == HIGH && buttonState == LOW) {
    Serial.println("🛎 슬라이더 버튼 누름 감지");

    if (movingTo2F) {
      moveToWithLog("맨 아래", "2층 (30cm)", HEIGHT_2F_STEPS);
    } else {
      moveToWithLog("2층 (30cm)", "맨 아래", 0);
    }

    movingTo2F = !movingTo2F;
    delay(500);  // 디바운싱
  }

  lastButtonState = buttonState;
}

void moveToWithLog(String from, String to, long target_steps) {
  isMoving = true;

  stopRobot();  // ✅ 추가된 코드

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
    Serial.println("↑ 방향: 위로 이동");
  } else {
    dir = LOW;
    Serial.println("↓ 방향: 아래로 이동");
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

  Serial.print("✅ 이동 완료 → current_steps 갱신됨: ");
  Serial.println(current_steps);
}


// ▶ PID 보정 직진
void moveForwardWithPID(int interval) {
  encM1 = encM2 = encM3 = encM4 = 0;

  int s1 = -BASE_SPEED;
  int s2 = -BASE_SPEED;
  int s3 = BASE_SPEED + 110;
  int s4 = BASE_SPEED + 110;

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
  Serial.print(" | PID12 Out="); Serial.print(output12);
  Serial.print(" | PID34 Out="); Serial.println(output34);

  delay(interval);
}

// ⏹ 전체 정지
void stopRobot() {
  motorDriver.setSingleMotor(M1, 0);
  motorDriver.setSingleMotor(M2, 0);
  motorDriver.setSingleMotor(M3, 0);
  motorDriver.setSingleMotor(M4, 0);
  Serial.println("⏹ 정지 완료");
}