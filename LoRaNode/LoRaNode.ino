#include "application.h"

#define DELTA 4688  // time to send a packet = 300ms * 16MHz / 1024

NodeManager node;
Payload receivedData;

bool canSleep = 0;  // TODO: use this to prevent the module from falling asleep while sending
bool awake = 0;
bool canSend = 0;

void setup() {
  node.data = { 0x4433, 0, 0, 1234.5678, 1234.5678, 85, 2, 31, 0 };

  cli();  // pause interrupts
  // Set Timer/Counter Control Registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12);               // clear timer on compare match
  TCCR1B |= (1 << CS12) | (1 << CS10);  // set the prescaler to 1024
  // Set Output Compare Register
  OCR1A = 0xFFFF;  // (T = 4.194s)
  // Reset Timer/Counter
  TCNT1 = 0;
  // Enable timer interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();  // enable interrupts

  randomSeed(analogRead(0));
  Serial.begin(115200);  // laptop
  Serial1.begin(9600);   // LoRa module
  delay(200);

  pinMode(13, OUTPUT);  // low=sleep, high=wake
  digitalWrite(13, HIGH);

  // Ensure TDT mode
  delay(200);
  Serial1.print("+++\r\n");
  delay(200);
  Serial1.print("AT+EXIT\r\n");
  delay(200);

  // read OKs
  while (Serial1.available() > 0) {
    char data = Serial1.read();
    Serial.write(data);
  }

  Serial.println("Start sync sequence...");
  bool received = 0;
  unsigned long startMillis = millis();
  while (millis() - startMillis < 10000 && !received) {
    if (Serial1.available() > 0) {
      int out = Serial1.readBytes((uint8_t*)&receivedData, sizeof(Payload));
      if (receivedData.id != 0) {  // ignore garbage
        TCNT1 = receivedData.timer;
        received = 1;

        Serial.print("Received SYNC from ID: ");
        Serial.println(receivedData.id, HEX);
      }
    }
  }

  node.data.seq_num = 0;
  if (received) {  // Follower: wait random time, then broadcast
    Serial.println("Follower.");
    unsigned long startMillis = millis();
    long interval = random(0, 1000);
    while (millis() - startMillis < interval)
      ;
  } else { // Synchronizer
    Serial.println("Synchronizer.");
  }
  // Broadcast your clock
  node.data.timer = TCNT1 + DELTA;
  Serial1.write((uint8_t*)&node.data, sizeof(Payload));
  Serial.println("Sync sequence complete...");
  canSleep = 1;
}

ISR(TIMER1_COMPA_vect) {  // timer1
  if (canSleep) {
    if (awake) {
      digitalWrite(13, LOW);
      awake = 0;
    } else {
      digitalWrite(13, HIGH);
      while (Serial1.available() > 0) {
        char t = Serial1.read();
      }
      awake = 1;
      canSend = 1;
    }
  }
}

void loop() {
  if (canSend) {  // every time you wake up
    delay(10);

    Serial.println("Sending...");
    node.data.seq_num++;
    node.data.timer = TCNT1 + DELTA;
    Serial1.write((uint8_t*)&node.data, sizeof(Payload));
    canSend = 0;
  }

  if (Serial1.available() > 0) {
    int out = Serial1.readBytes((uint8_t*)&receivedData, sizeof(Payload));
    if (receivedData.id != 0) {  // ignore garbage
      Serial.print("ID: ");
      Serial.println(receivedData.id, HEX);

      bool distance_check = node.checkDistance(receivedData);
      bool sequence_check = node.checkSequence(receivedData);

      if (receivedData.command == Payload::COMMAND::UPDATE) {
        // Check both sequence and distance before update
        if (distance_check && sequence_check) {
          node.update(receivedData);
          // TODO: Flooding received data
        }
      }

      if (receivedData.command == Payload::COMMAND::PURGE) {
        // Purge data regardless of distance
        if (node.checkRecord(receivedData.id)) {
          // If record exist, purge and flood received data
          node.purge(receivedData.id);
          // TODO: Flooding received data
        }
      }
    }
  }
}
