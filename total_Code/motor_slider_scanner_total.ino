#include <Wire.h>
#include <MotorDriver.h>  // YFROBOT Motor Driver 라이브러리

// ✅ 모터 설정 (로봇 주행용)
#define MOTOR_TYPE YF_IIC_RZ
MotorDriver motorDriver(MOTOR_TYPE);

#define M1 1  // 좌측 모터
#define M2 2  // 우측 모터
#define M3 3  // 후방 모터

#define BASE_SPEED 400
#define CORRECTION_FACTOR 10

#define ENCODER_M1_A 2
#define ENCODER_M1_B 3
#define ENCODER_M2_A 4
#define ENCODER_M2_B 5

volatile long encoderCountM1 = 0;
volatile long encoderCountM2 = 0;

#define STOP_SIGNAL_PIN A2
#define MOVE_SIGNAL_PIN A1

bool obstacleDetected = false;
bool movingRobot = false;

// ✅ 슬라이더 설정 (리니어 슬라이더용)
#define STEP_PIN 10        // TB6600 PUL+
#define DIR_PIN 11         // TB6600 DIR+
#define BUTTON_PIN A0      // 슬라이더 조작 버튼

#define STEPS_PER_MM 400L
#define HEIGHT_2F_MM 100
#define HEIGHT_2F_STEPS (HEIGHT_2F_MM * STEPS_PER_MM)

long current_steps = 0;
bool lastButtonState = HIGH;
bool movingTo2F = true;
bool isSliderMoving = false;

// ✅ 바코드 스캐너 트랜지스터 제어 (D12 사용)
const int ledControl = 12;
unsigned long lastToggleTime = 0;
bool ledState = false;

void encoderM1() { encoderCountM1++; }
void encoderM2() { encoderCountM2++; }

void setup() {
  Serial.begin(115200);
  Wire.begin();

  motorDriver.begin();
  motorDriver.setPWMFreq(100);

  // 주행 관련 핀 설정
  pinMode(STOP_SIGNAL_PIN, INPUT);
  pinMode(MOVE_SIGNAL_PIN, INPUT);
  pinMode(ENCODER_M1_A, INPUT);
  pinMode(ENCODER_M1_B, INPUT);
  pinMode(ENCODER_M2_A, INPUT);
  pinMode(ENCODER_M2_B, INPUT);

  attachInterrupt(digitalPinToInterrupt(ENCODER_M1_A), encoderM1, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_M2_A), encoderM2, RISING);

  // 슬라이더 관련 핀 설정
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 바코드 스캐너 제어 핀 설정
  pinMode(ledControl, OUTPUT);

  delay(1000);
  Serial.println("🔍 시스템 준비 완료");
}

void loop() {
  handleRobotMovement();
  handleSliderMovement();
  handleBarcodeScannerToggle();  // 바코드 스캐너 제어
}

// ✅ 바코드 스캐너 ON/OFF 반복
void handleBarcodeScannerToggle() {
  unsigned long currentTime = millis();
  if (currentTime - lastToggleTime >= 100) {
    ledState = !ledState;
    digitalWrite(ledControl, ledState ? HIGH : LOW);
    lastToggleTime = currentTime;
  }
}

// ✅ 로봇 주행 처리
void handleRobotMovement() {
  int stopSignal = digitalRead(STOP_SIGNAL_PIN);
  int moveSignal = digitalRead(MOVE_SIGNAL_PIN);

  if (stopSignal == HIGH) {
    stopRobot();
    if (!obstacleDetected) {
      Serial.println("🚨 장애물 감지: 모터 정지");
      obstacleDetected = true;
      movingRobot = false;
    }
  } else if (moveSignal == HIGH && !movingRobot) {
    Serial.println("✅ 장애물 해제: 직진 시작");
    obstacleDetected = false;
    movingRobot = true;
    moveStraightWithCorrection(100);
  }
}

// ✅ 슬라이더 버튼 처리
void handleSliderMovement() {
  bool buttonState = digitalRead(BUTTON_PIN);

  if (!isSliderMoving && lastButtonState == HIGH && buttonState == LOW) {
    Serial.println("🛎 슬라이더 버튼 눌림 감지");

    if (movingTo2F) {
      moveToWithLog("맨 아래", "2층 (10cm)", HEIGHT_2F_STEPS);
    } else {
      moveToWithLog("2층 (10cm)", "맨 아래", 0);
    }

    movingTo2F = !movingTo2F;
    delay(500); // 디바운스
  }

  lastButtonState = buttonState;
}

// ✅ 슬라이더 이동 함수
void moveToWithLog(String from, String to, long target_steps) {
  isSliderMoving = true;

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
  Serial.println("🚀 이동 시작");

  for (long i = 0; i < steps_to_move; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(300);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(300);
  }

  current_steps = target_steps;
  isSliderMoving = false;

  Serial.print("✅ 슬라이더 이동 완료 → current_steps 갱신됨: ");
  Serial.println(current_steps);
  Serial.println();
}

// ✅ 실시간 직진 보정
void moveStraightWithCorrection(int correctionInterval) {
  encoderCountM1 = 0;
  encoderCountM2 = 0;

  int speedLeft = BASE_SPEED;
  int speedRight = -BASE_SPEED;

  int error = encoderCountM1 - encoderCountM2;
  if (error > 5) {
    speedLeft -= CORRECTION_FACTOR;
  } else if (error < -5) {
    speedRight += CORRECTION_FACTOR;
  }

  moveRobot(speedLeft, speedRight, 0);
  delay(correctionInterval);
}

// ✅ 개별 모터 속도 제어
void moveRobot(int speedM1, int speedM2, int speedM3) {
  motorDriver.setSingleMotor(M1, speedM1);
  motorDriver.setSingleMotor(M2, speedM2);
  motorDriver.setSingleMotor(M3, speedM3);
}

// ✅ 전체 정지
void stopRobot() {
  moveRobot(0, 0, 0);
}
