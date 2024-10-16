#include "LoRaNode.h"

LoRaNode node;
Payload buffer;

bool canSleep = 0;  // TODO: use this to prevent the module from falling asleep while sending
bool awake = 1;
bool canSend = 0;

void setup() {
  node.data = { 0x0100, 0, 0, 51.4484378, 5.494187, 85, 2, 31, UPDATE };

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
      int out = Serial1.readBytes((uint8_t*)&buffer, sizeof(Payload));
      if (buffer.id != 0) {  // ignore garbage
        TCNT1 = buffer.timer + DELTA;
        received = 1;

        Serial.print("Received SYNC from ID: ");
        Serial.println(buffer.id, HEX);
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
  } else {  // Synchronizer
    Serial.println("Synchronizer.");
  }
  // Broadcast your clock
  node.data.timer = TCNT1;
  Serial1.write((uint8_t*)&(node.data), sizeof(Payload));
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

    Serial.println("Broadcasting...");
    node.data.seq_num++;
    node.data.timer = TCNT1;
    Serial1.write((uint8_t*)&(node.data), sizeof(Payload));
    canSend = 0;
  }

  if (awake) {
    int out = node.dequeue(&buffer);
    if (out != -1) {
      Serial.print("Relaying packet ");
      Serial.print(buffer.seq_num);
      Serial.print(" from ");
      Serial.print(buffer.id, HEX);
      Serial.print("... (");
      Serial.print(out);
      Serial.println(") left in the queue");

      buffer.timer = TCNT1;
      Serial1.write((uint8_t*)&(buffer), sizeof(Payload));
    }
  }

  if (Serial1.available() > 0) {
    int out = Serial1.readBytes((uint8_t*)&buffer, sizeof(Payload));
    if (buffer.id != 0) {  // ignore garbage
      Serial.print("ID: ");
      Serial.println(buffer.id, HEX);

      if (buffer.id != node.data.id && node.checkSequence(buffer.id, buffer.seq_num)) {  // ignore maessage about yourself
        node.handleMessage(buffer);
        Serial.print("Table size = ");
        Serial.println(node.tableTail);

        int out = node.enqueue(buffer);
        if (out == -1) {
          Serial.println("Queue full! Message dropped.");
        } else if (out == -2) {
          Serial.println("Bike was too far away to be relayed");
        } else {
          Serial.println("Added to queue");
        }
      }
    }
  }
}
