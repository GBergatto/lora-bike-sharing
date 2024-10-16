void setup() {
  Serial.begin(115200);  // Serial communication with the laptop
  Serial1.begin(9600);  // Serial communication with the LoRa module

  pinMode(13, OUTPUT);  // low=sleep, high=wake
  digitalWrite(13, HIGH);
}

void loop() {
  // If data is available from the LoRa module (Serial1), send it to the Serial Monitor
  if (Serial1.available()) {
    char data = Serial1.read();
    Serial.write(data);  // Relay to Serial Monitor
  }

  // If data is available from the Serial Monitor (USB-C), send it to the LoRa module
  if (Serial.available()) {
    char data = Serial.read();
    Serial1.write(data);  // Relay to LoRa module
  }
}
