#include <Wire.h>
#include <MotorDriver.h>  // YFROBOT I2C 모터드라이버용 라이브러리

// ✅ 모터드라이버 타입 지정 및 객체 생성
#define MOTOR_TYPE YF_IIC_RZ
MotorDriver motorDriver(MOTOR_TYPE);

// ✅ 4륜 모터 ID 정의 (YFROBOT 보드의 채널 번호 기준)
#define M1 1  // 앞 좌측
#define M2 2  // 앞 우측
#define M3 3  // 뒤 우측
#define M4 4  // 뒤 좌측

// ✅ 주행 속도 및 보정 값
#define BASE_SPEED 400         // 기본 주행 PWM 속도
#define CORRECTION 50          // 엔코더 오차 보정 값
#define CORRECTION_INTERVAL 100  // 보정 주기 (ms)

// ✅ 각 모터의 엔코더 A 채널 핀 번호 (인터럽트 사용)
#define ENCODER_M1_A 2
#define ENCODER_M2_A 3
#define ENCODER_M3_A 4
#define ENCODER_M4_A 5

// ✅ 엔코더 카운트 변수 (각 바퀴 회전 수)
volatile long encM1 = 0, encM2 = 0, encM3 = 0, encM4 = 0;

// ✅ 슬라이더 (리니어 액추에이터) 설정
#define STEP_PIN 10        // TB6600의 스텝 신호 입력 핀
#define DIR_PIN 11         // TB6600의 방향 제어 핀
#define BUTTON_PIN A0      // 수동 버튼 입력 (슬라이더 이동 토글용)

// 슬라이더 사양 기준: 1mm당 400스텝, 최대 스트로크 300mm까지 사용 가능
#define STEPS_PER_MM 400L
#define HEIGHT_2F_MM 100                             // 2층 높이(mm) 설정
#define HEIGHT_2F_STEPS (HEIGHT_2F_MM * STEPS_PER_MM)  // 스텝 단위로 변환

long current_steps = 0;     // 슬라이더 현재 위치 (스텝 기준)
bool lastButtonState = HIGH;   // 이전 버튼 상태 (디바운싱 용도)
bool movingTo2F = true;        // 다음 이동 방향 (true면 2층으로)
bool isSliderMoving = false;   // 현재 슬라이더가 이동 중인지 여부

// ✅ 바코드 스캐너 트랜지스터 제어 (MOSFET GATE에 D12 연결)
const int ledControl = 12;       // 스캐너 전원 제어 핀
unsigned long lastToggleTime = 0;
bool ledState = false;

void encoderM1() { encM1++; }  // 좌전
void encoderM2() { encM2++; }  // 우전
void encoderM3() { encM3++; }  // 우후
void encoderM4() { encM4++; }  // 좌후

void setup() {
  Serial.begin(115200);
  Wire.begin();  // I2C 초기화

  motorDriver.begin();          // 모터드라이버 초기화
  motorDriver.setPWMFreq(100);  // PWM 주파수 설정 (Hz)

  // 엔코더 핀 모드 및 인터럽트 연결
  pinMode(ENCODER_M1_A, INPUT);
  pinMode(ENCODER_M2_A, INPUT);
  pinMode(ENCODER_M3_A, INPUT);
  pinMode(ENCODER_M4_A, INPUT);

  attachInterrupt(digitalPinToInterrupt(ENCODER_M1_A), encoderM1, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_M2_A), encoderM2, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_M3_A), encoderM3, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_M4_A), encoderM4, RISING);

  // 슬라이더 및 바코드 핀 설정
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ledControl, OUTPUT);

  delay(1000);  // 안정화 대기
  Serial.println("🔍 시스템 준비 완료");
}

void loop() {
  moveForwardWithEncoderCorrection(CORRECTION_INTERVAL);  // 엔코더 기반 직진
  handleSliderMovement();        // 버튼 입력에 따라 슬라이더 동작
  handleBarcodeScannerToggle();  // 바코드 스캐너 깜빡임 (트랜지스터 제어)
}

// ✅ 바코드 스캐너 트랜지스터 제어: 100ms 주기로 ON/OFF
void handleBarcodeScannerToggle() {
  unsigned long currentTime = millis();
  if (currentTime - lastToggleTime >= 100) {
    ledState = !ledState;
    digitalWrite(ledControl, ledState ? HIGH : LOW);
    lastToggleTime = currentTime;
  }
}

// ✅ 슬라이더 버튼 감지 및 토글 이동
void handleSliderMovement() {
  bool buttonState = digitalRead(BUTTON_PIN);

  // 버튼이 새로 눌렸을 때만 동작
  if (!isSliderMoving && lastButtonState == HIGH && buttonState == LOW) {
    Serial.println("🛎 슬라이더 버튼 눌림 감지");

    if (movingTo2F) {
      moveToWithLog("맨 아래", "2층 (10cm)", HEIGHT_2F_STEPS);
    } else {
      moveToWithLog("2층 (10cm)", "맨 아래", 0);
    }

    movingTo2F = !movingTo2F;  // 방향 토글
    delay(500);                // 디바운싱
  }

  lastButtonState = buttonState;
}

// ✅ 슬라이더 실제 이동 함수 (스텝 수 기준)
void moveToWithLog(String from, String to, long target_steps) {
  isSliderMoving = true;

  long steps_to_move = target_steps - current_steps;
  int dir = steps_to_move > 0 ? HIGH : LOW;
  steps_to_move = abs(steps_to_move);

  Serial.println("====================================");
  Serial.print("📍 From: "); Serial.println(from);
  Serial.print("🎯 To: "); Serial.println(to);
  Serial.print("➡ 이동 스텝 수: "); Serial.println(steps_to_move);
  Serial.println(dir == HIGH ? "↑ 위로 이동" : "↓ 아래로 이동");

  digitalWrite(DIR_PIN, dir);

  for (long i = 0; i < steps_to_move; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(300);  // 모터 속도
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(300);
  }

  current_steps = target_steps;
  isSliderMoving = false;

  Serial.print("✅ 슬라이더 이동 완료 → 위치: ");
  Serial.println(current_steps);
}

// ✅ 4륜 로봇 직진: 엔코더 기반 실시간 보정
void moveForwardWithEncoderCorrection(int interval) {
  encM1 = encM2 = encM3 = encM4 = 0;  // 엔코더 카운트 초기화

  // 기본 속도 세팅
  int s1 = -BASE_SPEED, s2 = -BASE_SPEED;
  int s3 = BASE_SPEED, s4 = BASE_SPEED;

  // 앞쪽 바퀴(M1, M2) 보정
  long diff12 = encM1 - encM2;
  if (diff12 > 5) s1 += CORRECTION;
  else if (diff12 < -5) s2 -= CORRECTION;

  // 뒷쪽 바퀴(M3, M4) 보정
  long diff34 = encM3 - encM4;
  if (diff34 > 5) s3 += CORRECTION;
  else if (diff34 < -5) s4 -= CORRECTION;

  // 모터 속도 설정
  motorDriver.setSingleMotor(M1, s1);
  motorDriver.setSingleMotor(M2, s2);
  motorDriver.setSingleMotor(M3, s3);
  motorDriver.setSingleMotor(M4, s4);

  // 디버깅 로그 출력
  Serial.print("▶ 직진 | Enc M1="); Serial.print(encM1);
  Serial.print(" M2="); Serial.print(encM2);
  Serial.print(" M3="); Serial.print(encM3);
  Serial.print(" M4="); Serial.println(encM4);

  delay(interval);  // 다음 보정까지 대기
}

// ✅ 로봇 정지
void stopRobot() {
  motorDriver.setSingleMotor(M1, 0);
  motorDriver.setSingleMotor(M2, 0);
  motorDriver.setSingleMotor(M3, 0);
  motorDriver.setSingleMotor(M4, 0);
  Serial.println("⏹ 정지 완료");
}
