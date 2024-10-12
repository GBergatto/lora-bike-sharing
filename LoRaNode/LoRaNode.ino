void setup() {
  cli(); // pause interrupts

  // Reset counter1
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  
  // Set compare match register
  OCR1A = 65535;

  // Clear timer on compare match
  TCCR1B |= (1 << WGM12);

  // Set the prescaler to 1024
  TCCR1B |= (1 << CS12) | (1 << CS10);

  // Enable timer interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei(); // enable interrupts

  Serial.begin(9600);  // Serial communication with the laptop
  Serial1.begin(9600);  // Serial communication with the LoRa module

  pinMode(13, OUTPUT); // low=sleep, high=wake
}

bool toggle = 0;

ISR(TIMER1_COMPA_vect) { // timer1
  if (toggle){
    digitalWrite(13,LOW);
    toggle = 0;
  }
  else{
    digitalWrite(13,HIGH);
    toggle = 1;
  }
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