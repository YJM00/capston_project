#define STEP_PIN 10        // TB6600 PUL+
#define DIR_PIN 11         // TB6600 DIR+

#define STEPS_PER_MM 400L   // pitch 1mm 기준, 실제 측정값
#define HEIGHT_1F 70       // 1층 높이(mm)
#define HEIGHT_2F 290      // 2층 높이(mm)

long current_steps = 0;    // 슬라이더 위치를 스텝으로 저장

void setup() {
  Serial.begin(9600);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  delay(1000);  // 안정화 대기

  moveToWithLog("맨 아래", "1층", HEIGHT_1F * STEPS_PER_MM);
  delay(2000);

  moveToWithLog("1층", "2층", HEIGHT_2F * STEPS_PER_MM);
  delay(2000);

  moveToWithLog("2층", "1층", HEIGHT_1F * STEPS_PER_MM);
  delay(2000);

  moveToWithLog("1층", "맨 아래", 0);
}

void loop() {
  // 반복 없음
}

void moveToWithLog(String from, String to, long target_steps) {
  long steps_to_move = target_steps - current_steps;

  Serial.println("====================================");
  Serial.print("📍 From: "); Serial.println(from);
  Serial.print("🎯 To: "); Serial.println(to);
  Serial.print("📏 current_steps: "); Serial.println(current_steps);
  Serial.print("📌 target_steps: "); Serial.println(target_steps);
  Serial.print("➡ 이동 스텝 수: "); Serial.println(steps_to_move);

  if (steps_to_move > 0) {
    digitalWrite(DIR_PIN, HIGH);
    Serial.println("↑ 방향: 위로 이동");
  } else {
    digitalWrite(DIR_PIN, LOW);
    Serial.println("↓ 방향: 아래로 이동");
    steps_to_move = -steps_to_move; // 절댓값 처리
  }

  Serial.println("🚀 이동 시작");

  for (long i = 0; i < steps_to_move; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(500);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(500);
  }

  current_steps = target_steps;  // 위치 갱신

  Serial.print("✅ 이동 완료 → current_steps 갱신됨: ");
  Serial.println(current_steps);
  Serial.println();
}
