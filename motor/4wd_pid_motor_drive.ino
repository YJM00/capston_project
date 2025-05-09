#include <Wire.h>
#include <MotorDriver.h>

#define FL 1
#define FR 2
#define RL 3
#define RR 4

MotorDriver motorDriver(YF_IIC_RZ);

// PID 파라미터
float Kp = 1.2;
float Ki = 0.0;
float Kd = 0.2;

// 속도 제어 변수
float targetV_FL, targetV_FR, targetV_RL, targetV_RR;
float actualV_FL, actualV_FR, actualV_RL, actualV_RR;
float error_FL, error_FR, error_RL, error_RR;
float integral_FL, integral_FR, integral_RL, integral_RR;
float prev_error_FL, prev_error_FR, prev_error_RL, prev_error_RR;

// 엔코더 카운터
volatile long ticks_FL = 0;
volatile long ticks_FR = 0;
volatile long ticks_RL = 0;
volatile long ticks_RR = 0;

// 1초에 몇 ms마다 속도 갱신?
unsigned long lastPIDUpdate = 0;
const int PID_INTERVAL = 50;  // ms

// CPR 및 기어비 기반 회전당 tick 수
#define TICKS_PER_REV 27000.0  // 360 CPR × 75:1

void setup() {
  Serial.begin(115200);
  Wire.begin();
  motorDriver.begin();
  motorDriver.setPWMFreq(100);

  // attachInterrupt는 실제 핀 번호에 맞게 설정
  attachInterrupt(digitalPinToInterrupt(2), [] { ticks_FL++; }, RISING);
  attachInterrupt(digitalPinToInterrupt(3), [] { ticks_FR++; }, RISING);
  attachInterrupt(digitalPinToInterrupt(4), [] { ticks_RL++; }, RISING);
  attachInterrupt(digitalPinToInterrupt(5), [] { ticks_RR++; }, RISING);

  Serial.println("✅ PID 옴니휠 로봇 초기화 완료");
}

void loop() {
  // 1. 원하는 주행 명령 설정 (예: 앞으로 이동)
  float Vx = 0.0;
  float Vy = 100.0;
  float Wz = 0.0;

  setTargetSpeeds(Vx, Vy, Wz);

  // 2. PID 제어 주기마다 실제 속도 계산 및 보정
  if (millis() - lastPIDUpdate >= PID_INTERVAL) {
    lastPIDUpdate = millis();
    updateSpeedsAndPID();
  }
}

// ✅ 주행 벡터 → 바퀴 목표 속도 변환
void setTargetSpeeds(float Vx, float Vy, float Wz) {
  targetV_FL = Vy + Vx - Wz;
  targetV_FR = Vy - Vx + Wz;
  targetV_RL = Vy - Vx - Wz;
  targetV_RR = Vy + Vx + Wz;
}

// ✅ 엔코더 → 실제 속도 계산 → PID 제어
void updateSpeedsAndPID() {
  static long prevTicks_FL, prevTicks_FR, prevTicks_RL, prevTicks_RR;

  float deltaTime = PID_INTERVAL / 1000.0;

  actualV_FL = (ticks_FL - prevTicks_FL) / TICKS_PER_REV / deltaTime * 60.0;
  actualV_FR = (ticks_FR - prevTicks_FR) / TICKS_PER_REV / deltaTime * 60.0;
  actualV_RL = (ticks_RL - prevTicks_RL) / TICKS_PER_REV / deltaTime * 60.0;
  actualV_RR = (ticks_RR - prevTicks_RR) / TICKS_PER_REV / deltaTime * 60.0;

  prevTicks_FL = ticks_FL;
  prevTicks_FR = ticks_FR;
  prevTicks_RL = ticks_RL;
  prevTicks_RR = ticks_RR;

  float out_FL = pidControl(targetV_FL, actualV_FL, &error_FL, &prev_error_FL, &integral_FL);
  float out_FR = pidControl(targetV_FR, actualV_FR, &error_FR, &prev_error_FR, &integral_FR);
  float out_RL = pidControl(targetV_RL, actualV_RL, &error_RL, &prev_error_RL, &integral_RL);
  float out_RR = pidControl(targetV_RR, actualV_RR, &error_RR, &prev_error_RR, &integral_RR);

  motorDriver.setSingleMotor(FL, out_FL);
  motorDriver.setSingleMotor(FR, out_FR);
  motorDriver.setSingleMotor(RL, out_RL);
  motorDriver.setSingleMotor(RR, out_RR);
}

// ✅ PID 제어 함수
float pidControl(float target, float actual, float* e, float* e_prev, float* integral) {
  *e = target - actual;
  *integral += *e * PID_INTERVAL / 1000.0;
  float derivative = (*e - *e_prev) / (PID_INTERVAL / 1000.0);
  *e_prev = *e;

  return Kp * (*e) + Ki * (*integral) + Kd * derivative;
}
