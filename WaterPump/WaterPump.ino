#define TOUCH_PIN 4
#define PUMP_PIN  5

void setup() {
  pinMode(TOUCH_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  int touch = digitalRead(TOUCH_PIN);
  digitalWrite(PUMP_PIN, touch);
  Serial.println(touch ? "Pompe ON" : "Pompe OFF");
  delay(100);
}
