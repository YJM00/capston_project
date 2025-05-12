#include <Wire.h>
#include <MotorDriver.h>

#define MOTOR_TYPE YF_IIC_RZ
MotorDriver motorDriver(MOTOR_TYPE);

// 모터 ID
#define M1 1  // 좌전
#define M2 2  // 우전
#define M3 3  // 우후
#define M4 4  // 좌후

#define BASE_SPEED 400
#define CORRECTION 50
#define LOOP_DELAY 200

// 엔코더 핀 정의
#define ENCODER_M1_A 2
#define ENCODER_M2_A 3
#define ENCODER_M3_A 4
#define ENCODER_M4_A 5

volatile long encM1 = 0, encM2 = 0, encM3 = 0, encM4 = 0;

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

  Serial.println("✅ 4륜 완전 보정 시작");
}

void loop() {
  moveWithCorrection("forward");
  delay(500);
 /* moveWithCorrection("backward");
  delay(500);
  moveWithCorrection("left");
  delay(500);
  moveWithCorrection("right");
  delay(500);
  moveWithCorrection("rotate_cw");
  delay(500);
  moveWithCorrection("rotate_ccw");
  delay(500);
  stopRobot();
  delay(1000);*/
}

void moveWithCorrection(String dir) {
  encM1 = encM2 = encM3 = encM4 = 0;
  delay(LOOP_DELAY);

  int s1 = 0, s2 = 0, s3 = 0, s4 = 0;

  if (dir == "forward") {
    s1 = -BASE_SPEED; s2 = -BASE_SPEED; s3 = BASE_SPEED; s4 = BASE_SPEED;
  } else if (dir == "backward") {
    s1 = BASE_SPEED; s2 = BASE_SPEED; s3 = -BASE_SPEED; s4 = -BASE_SPEED;
  } else if (dir == "left") {
    s1 = BASE_SPEED; s2 = -BASE_SPEED; s3 = -BASE_SPEED; s4 = BASE_SPEED;
  } else if (dir == "right") {
    s1 = -BASE_SPEED; s2 = BASE_SPEED; s3 = BASE_SPEED; s4 = -BASE_SPEED;
  } else if (dir == "rotate_cw") {
    s1 = BASE_SPEED; s2 = BASE_SPEED; s3 = BASE_SPEED; s4 = BASE_SPEED;
  } else if (dir == "rotate_ccw") {
    s1 = -BASE_SPEED; s2 = -BASE_SPEED; s3 = -BASE_SPEED; s4 = -BASE_SPEED;
  }

  // 쌍으로 보정 (M1-M2, M3-M4)
  long diff12 = encM1 - encM2;
  if (diff12 > 5) s1 += CORRECTION;
  else if (diff12 < -5) s2 -= CORRECTION;

  long diff34 = encM3 - encM4;
  if (diff34 > 5) s3 += CORRECTION;
  else if (diff34 < -5) s4 -= CORRECTION;

  motorDriver.setSingleMotor(M1, s1);
  motorDriver.setSingleMotor(M2, s2);
  motorDriver.setSingleMotor(M3, s3);
  motorDriver.setSingleMotor(M4, s4);

  Serial.print("▶ "); Serial.print(dir);
  Serial.print(" | Enc M1="); Serial.print(encM1);
  Serial.print(" M2="); Serial.print(encM2);
  Serial.print(" M3="); Serial.print(encM3);
  Serial.print(" M4="); Serial.print(encM4);
  Serial.print(" | PWM M1="); Serial.print(s1);
  Serial.print(" M2="); Serial.print(s2);
  Serial.print(" M3="); Serial.print(s3);
  Serial.print(" M4="); Serial.println(s4);
}

void stopRobot() {
  motorDriver.setSingleMotor(M1, 0);
  motorDriver.setSingleMotor(M2, 0);
  motorDriver.setSingleMotor(M3, 0);
  motorDriver.setSingleMotor(M4, 0);
  Serial.println("⏹ 정지");
}
