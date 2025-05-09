#include <Wire.h>
#include <MotorDriver.h>

#define MOTOR_TYPE YF_IIC_RZ
MotorDriver motorDriver(MOTOR_TYPE);

// 모터 ID
#define M1 1  // 좌측
#define M2 2  // 우측
#define M3 3  // 후방

// PID 파라미터 (필요시 튜닝)
float Kp = 1.2;
float Ki = 0.0;
float Kd = 0.3;

// 제어 주기
const int PID_INTERVAL = 50;
unsigned long lastUpdate = 0;

// 엔코더 관련
#define TICKS_PER_REV 27000.0  // 360CPR × 75:1 기어비
volatile long ticks_M1 = 0, ticks_M2 = 0, ticks_M3 = 0;
long prevTicks_M1 = 0, prevTicks_M2 = 0, prevTicks_M3 = 0;

// PID 계산용 변수
float targetRPM_M1 = 0, targetRPM_M2 = 0, targetRPM_M3 = 0;
float error_M1 = 0, error_M2 = 0, error_M3 = 0;
float prevError_M1 = 0, prevError_M2 = 0, prevError_M3 = 0;
float integral_M1 = 0, integral_M2 = 0, integral_M3 = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  motorDriver.begin();
  motorDriver.setPWMFreq(100);

  // 엔코더 인터럽트 설정 (핀은 실제 연결에 따라 변경)
  attachInterrupt(digitalPinToInterrupt(2), [] { ticks_M1++; }, RISING);
  attachInterrupt(digitalPinToInterrupt(4), [] { ticks_M2++; }, RISING);
  attachInterrupt(digitalPinToInterrupt(6), [] { ticks_M3++; }, RISING);

  Serial.println("✅ 3륜 PID 제어 시작됨");
}

void loop() {
  // 목표 속도 설정 (RPM 단위)
  targetRPM_M1 = 60;
  targetRPM_M2 = -60;
  targetRPM_M3 = 0;

  if (millis() - lastUpdate >= PID_INTERVAL) {
    lastUpdate = millis();
    updatePID();
  }
}

// ✅ PID 제어 및 속도 보정
void updatePID() {
  float dt = PID_INTERVAL / 1000.0;

  // 실제 속도 계산 (ticks → RPM)
  float rpm_M1 = (ticks_M1 - prevTicks_M1) / TICKS_PER_REV / dt * 60.0;
  float rpm_M2 = (ticks_M2 - prevTicks_M2) / TICKS_PER_REV / dt * 60.0;
  float rpm_M3 = (ticks_M3 - prevTicks_M3) / TICKS_PER_REV / dt * 60.0;
  prevTicks_M1 = ticks_M1;
  prevTicks_M2 = ticks_M2;
  prevTicks_M3 = ticks_M3;

  // PID 계산
  float pwm_M1 = pidControl(targetRPM_M1, rpm_M1, &error_M1, &prevError_M1, &integral_M1);
  float pwm_M2 = pidControl(targetRPM_M2, rpm_M2, &error_M2, &prevError_M2, &integral_M2);
  float pwm_M3 = pidControl(targetRPM_M3, rpm_M3, &error_M3, &prevError_M3, &integral_M3);

  motorDriver.setSingleMotor(M1, pwm_M1);
  motorDriver.setSingleMotor(M2, pwm_M2);
  motorDriver.setSingleMotor(M3, pwm_M3);

  Serial.print("RPM: M1=");
  Serial.print(rpm_M1);
  Serial.print(" M2=");
  Serial.print(rpm_M2);
  Serial.print(" M3=");
  Serial.println(rpm_M3);
}

// ✅ PID 제어 함수
float pidControl(float target, float actual, float* e, float* e_prev, float* integral) {
  *e = target - actual;
  *integral += *e * PID_INTERVAL / 1000.0;
  float derivative = (*e - *e_prev) / (PID_INTERVAL / 1000.0);
  *e_prev = *e;

  return Kp * (*e) + Ki * (*integral) + Kd * derivative;
}
