// 필요한 라이브러리 포함
#include <Wire.h>               // I2C 통신용
#include <MotorDriver.h>        // YFROBOT IIC RZ 모터 드라이버용 라이브러리
#include <PID_v1.h>             // PID 제어 라이브러리
#include <EnableInterrupt.h>    // 외부 인터럽트용 라이브러리

// 모터 드라이버 객체 생성
#define MOTOR_TYPE YF_IIC_RZ
MotorDriver motorDriver(MOTOR_TYPE);

// 모터 ID 정의
#define M1 1
#define M2 2
#define M3 3
#define M4 4

// PID 제어용 기본 속도와 보정 주기
#define BASE_SPEED 600
#define CORRECTION_INTERVAL 100  // PID 보정 주기(ms)

// 엔코더 A 채널 핀
#define ENCODER_M1_A 3
#define ENCODER_M2_A 2
#define ENCODER_M3_A 7
#define ENCODER_M4_A 9

// 슬라이더용 스텝모터 핀
#define STEP_PIN 10
#define DIR_PIN 11
#define SLIDER_TRIGGER_PIN 12  // 토글 트리거 입력 (버튼 또는 신호)

// 슬라이더 이동 거리 정의
#define STEPS_PER_MM 400L               // mm당 스텝 수
#define HEIGHT_2F_MM 300                // 2층 높이(mm)
#define HEIGHT_2F_STEPS (HEIGHT_2F_MM * STEPS_PER_MM)

// 라즈베리파이에서 슬라이더 제어를 위한 GPIO 입력 핀
#define PI_SIGNAL_UP_PIN   A2  // 2층 이동 명령
#define PI_SIGNAL_DOWN_PIN A1  // 1층 이동 명령

// 엔코더 값 저장용 변수
volatile long encM1 = 0, encM2 = 0, encM3 = 0, encM4 = 0;

// 슬라이더 위치 상태 및 동작 플래그
long current_steps = 0;      // 현재 위치 (스텝 수 기준)
bool isMoving = false;       // 슬라이더 동작 중 여부
bool lastTriggerState = LOW; // 트리거 핀의 이전 상태 저장

// PID 제어 변수 (M1-M2, M3-M4 쌍으로 제어)
double diff12_input = 0, output12 = 0, setpoint12 = 0;
double diff34_input = 0, output34 = 0, setpoint34 = 0;

// PID 컨트롤러 객체 생성
PID pid12(&diff12_input, &output12, &setpoint12, 1.0, 0.5, 0.1, DIRECT);
PID pid34(&diff34_input, &output34, &setpoint34, 1.0, 0.5, 0.1, DIRECT);

// 엔코더 인터럽트 핸들러 (RISING 에지에서 카운터 증가)
void encoderM1() { encM1++; }
void encoderM2() { encM2++; }
void encoderM3() { encM3++; }
void encoderM4() { encM4++; }

void setup() {
  Serial.begin(115200);
  Wire.begin();                        // I2C 초기화
  motorDriver.begin();                // 모터 드라이버 초기화
  motorDriver.setPWMFreq(100);        // PWM 주파수 설정

  // 엔코더 핀 입력 모드 설정 및 인터럽트 등록
  pinMode(ENCODER_M1_A, INPUT);
  pinMode(ENCODER_M2_A, INPUT);
  pinMode(ENCODER_M3_A, INPUT);
  pinMode(ENCODER_M4_A, INPUT);
  enableInterrupt(ENCODER_M1_A, encoderM1, RISING);
  enableInterrupt(ENCODER_M2_A, encoderM2, RISING);
  enableInterrupt(ENCODER_M3_A, encoderM3, RISING);
  enableInterrupt(ENCODER_M4_A, encoderM4, RISING);

  // 슬라이더 핀 설정
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(SLIDER_TRIGGER_PIN, INPUT);

  // 라즈베리파이 GPIO 입력 설정
  pinMode(PI_SIGNAL_UP_PIN, INPUT);
  pinMode(PI_SIGNAL_DOWN_PIN, INPUT);

  // PID 제어기 설정
  pid12.SetMode(AUTOMATIC);
  pid34.SetMode(AUTOMATIC);
  pid12.SetOutputLimits(-100, 100);
  pid34.SetOutputLimits(-100, 100);

  delay(1000);
  Serial.println("🔍 시스템 준비 완료");
}

void loop() {
  // 2층 이동 명령 수신
  if (!isMoving && digitalRead(PI_SIGNAL_UP_PIN) == HIGH) {
    Serial.println("📡 라즈베리파이 → 2층 이동 명령 수신");
    stopRobot();  // 주행 중지
    moveToWithLog("1층", "2층", HEIGHT_2F_STEPS);
    return;
  }

  // 1층 이동 명령 수신
  if (!isMoving && digitalRead(PI_SIGNAL_DOWN_PIN) == HIGH) {
    Serial.println("📡 라즈베리파이 → 1층 이동 명령 수신");
    stopRobot();  // 주행 중지
    moveToWithLog("2층", "1층", 0);
    return;
  }

  // 슬라이더 트리거 버튼 처리 (HIGH 에지 감지)
  bool triggerState = digitalRead(SLIDER_TRIGGER_PIN);
  if (!isMoving && triggerState == HIGH && lastTriggerState == LOW) {
    Serial.println("📥 슬라이더 트리거 감지됨 → 현재 방식 유지");
    stopRobot();  // 주행 중지
    delay(500);   // 디바운싱
  }
  lastTriggerState = triggerState;

  // 슬라이더 동작 중이 아니면 PID 주행
  if (!isMoving) {
    moveForwardWithPID(CORRECTION_INTERVAL);
  } else {
    stopRobot();
  }
}

// 슬라이더 이동 함수 + 로그 출력
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
    dir = HIGH;  // 위로 이동
    Serial.println("↑ 위로 이동");
  } else {
    dir = LOW;   // 아래로 이동
    Serial.println("↓ 아래로 이동");
    steps_to_move = -steps_to_move;
  }

  digitalWrite(DIR_PIN, dir);
  Serial.println("🚀 슬라이더 이동 시작");

  // 슬라이더모터 구동 루프
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

// PID 기반 직진 주행
void moveForwardWithPID(int interval) {
  // 엔코더 값 초기화
  encM1 = encM2 = encM3 = encM4 = 0;

  // 초기 속도 설정
  int s1 = -BASE_SPEED;
  int s2 = -BASE_SPEED;
  int s3 = BASE_SPEED + 110;
  int s4 = BASE_SPEED + 110;

  // PID 입력값 계산
  diff12_input = encM1 - encM2;
  diff34_input = encM3 - encM4;

  // PID 보정 수행
  pid12.Compute();
  pid34.Compute();

  // 보정값 적용
  s1 += output12;
  s2 -= output12;
  s3 += output34;
  s4 -= output34;

  // 모터 속도 설정
  motorDriver.setSingleMotor(M1, s1);
  motorDriver.setSingleMotor(M2, s2);
  motorDriver.setSingleMotor(M3, s3);
  motorDriver.setSingleMotor(M4, s4);

  // 디버깅 출력
  Serial.print("▶ PID 직진 | Enc M1="); Serial.print(encM1);
  Serial.print(" M2="); Serial.print(encM2);
  Serial.print(" M3="); Serial.print(encM3);
  Serial.print(" M4="); Serial.print(encM4);
  Serial.print(" | PID12="); Serial.print(output12);
  Serial.print(" | PID34="); Serial.println(output34);

  delay(interval);
}

// 모든 모터 정지
void stopRobot() {
  motorDriver.setSingleMotor(M1, 0);
  motorDriver.setSingleMotor(M2, 0);
  motorDriver.setSingleMotor(M3, 0);
  motorDriver.setSingleMotor(M4, 0);
  Serial.println("⏹ 정지 완료");
}
