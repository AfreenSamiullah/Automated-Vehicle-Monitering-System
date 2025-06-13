#define TRIG_PIN 9     // Ultrasonic sensor TRIG pin connected to Arduino pin 9
#define ECHO_PIN 8     // Ultrasonic sensor ECHO pin connected to Arduino pin 8
#define OUTPUT_PIN 7   // Output pin to ESP32 or LED

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(OUTPUT_PIN, OUTPUT);
  Serial.begin(115200);
  digitalWrite(OUTPUT_PIN, HIGH);
  delay(3000);
  Serial.println("Starting...");
}

void loop() {
  // Trigger the ultrasonic pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read the echo time
  long duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate distance in centimeters
  float distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > 0 && distance <= 100) {
    digitalWrite(OUTPUT_PIN, HIGH);  // Object detected within 1m
  } else {
    digitalWrite(OUTPUT_PIN, LOW);   // No object within 1m
  }

  delay(200);  // Small delay before next measurement
}
