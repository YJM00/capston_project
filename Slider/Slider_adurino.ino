#define STEP_PIN 10        // TB6600 PUL+
#define DIR_PIN 11         // TB6600 DIR+
#define BUTTON_PIN A0      // 스위치 연결 핀

#define STEPS_PER_MM 400L  // 1mm당 400스텝
#define HEIGHT_2F_MM 100   // 2층 높이 (10cm = 100mm)
#define HEIGHT_2F_STEPS (HEIGHT_2F_MM * STEPS_PER_MM)

long current_steps = 0;             // 초기 위치: 맨 아래 (0)
bool lastButtonState = HIGH;        // 스위치 이전 상태 저장
bool movingTo2F = true;             // 다음 이동 방향 기억 (true = 2층으로)
bool isMoving = false;              // 현재 이동 중인지 상태 플래그

void setup() {
  Serial.begin(9600);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // 버튼 풀업 설정
  delay(1000);

  Serial.println("초기 위치: 맨 아래 (0)");
}

void loop() {
  bool buttonState = digitalRead(BUTTON_PIN);

  // 이동 중이 아닐 때만 버튼 입력 감지
  if (!isMoving && lastButtonState == HIGH && buttonState == LOW) {
    Serial.println("🛎 버튼 누름 감지 (이동 시작)");

    if (movingTo2F) {
      moveToWithLog("맨 아래", "2층 (10cm)", HEIGHT_2F_STEPS);
    } else {
      moveToWithLog("2층 (10cm)", "맨 아래", 0);
    }

    movingTo2F = !movingTo2F; // 다음 이동 방향 반전
    delay(500); // 디바운스
  }

  lastButtonState = buttonState;
}

void moveToWithLog(String from, String to, long target_steps) {
  isMoving = true;  // 이동 시작 표시

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
    delayMicroseconds(300);  // 속도 빠르게 설정
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(300);
  }

  current_steps = target_steps; // 최종 위치 갱신
  isMoving = false;             // 이동 완료 표시

  Serial.print("✅ 이동 완료 → current_steps 갱신됨: ");
  Serial.println(current_steps);
  Serial.println();
}
