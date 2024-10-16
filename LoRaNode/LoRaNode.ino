#include "LoRaNode.h"

LoRaNode node;
Payload buffer;

bool canSleep = 0;  // TODO: use this to prevent the module from falling asleep while sending
bool awake = 1;
bool canSend = 0;

void setup() {
  // Hardcode GPS data based on Google Earth 
  // Bike 1
  uint16_t id = 1;
  uint8_t vehicle_type = VEHICLE_TYPE::BICYCLE;
  float latitude = cvtDeg2Rad(cvt2Decimal(51.0f, 26.0f, 52.0f, 'N'));
  float longitude = cvtDeg2Rad(cvt2Decimal(5.0f, 29.0f, 3.0f, 'E'));

  // // Bike 2
  // uint16_t id = 2;
  // uint8_t vehicle_type = VEHICLE_TYPE::BICYCLE;
  // float latitude = cvtDeg2Rad(cvt2Decimal(51.0f, 26.0f, 49.0f, 'N'));
  // float longitude = cvtDeg2Rad(cvt2Decimal(5.0f, 29.0f, 30.0f, 'E'));

  // // E Bike 3
  // uint16_t id = 3;
  // uint8_t vehicle_type = VEHICLE_TYPE::ELECTRIC_BICYCLE;
  // float latitude = cvtDeg2Rad(cvt2Decimal(51.0f, 27.0f, 0.0f, 'N'));
  // float longitude = cvtDeg2Rad(cvt2Decimal(5.0f, 29.0f, 28.0f, 'E'));

  // // E bike 4
  // uint16_t id = 4;
  // uint8_t vehicle_type = VEHICLE_TYPE::ELECTRIC_BICYCLE;
  // float latitude = cvtDeg2Rad(cvt2Decimal(51.0f, 26.0f, 53.0f, 'N'));
  // float longitude = cvtDeg2Rad(cvt2Decimal(5.0f, 29.0f, 43.0f, 'E'));

  // // Cargo bike 5
  // uint16_t id = 5;
  // uint8_t vehicle_type = VEHICLE_TYPE::CARGO_BICYCLE;
  // float latitude = cvtDeg2Rad(cvt2Decimal(51.0f, 26.0f, 48.0f, 'N'));
  // float longitude = cvtDeg2Rad(cvt2Decimal(5.0f, 29.0f, 32.0f, 'E'));

  // // Motorbike 6
  // uint16_t id = 6;
  // uint8_t vehicle_type = VEHICLE_TYPE::MOTORCYCLE;
  // float latitude = cvtDeg2Rad(cvt2Decimal(51.0f, 26.0f, 51.0f, 'N'));
  // float longitude = cvtDeg2Rad(cvt2Decimal(5.0f, 28.0f, 38.0f, 'E'));

  node.data = { id, 0, 0, longitude, latitude, 85, 
                vehicle_type, 31, UPDATE };

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
