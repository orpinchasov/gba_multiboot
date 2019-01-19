#define SC_IN (2)
#define SD_IN (3)
#define SI_IN (4)
#define SO_IN (5)
#define SD_OUT (8)
#define SI_OUT (9)
#define SC_OUT (12)
#define LED (13)

#define BIT_TIME (42)

#define STR(x) #x
#define XSTR(s) STR(s)

byte state = 0;

volatile uint8_t received_low = 0;
volatile uint8_t received_high = 0;

volatile uint8_t send_low = 0;
volatile uint8_t send_high = 0;

void setup()
{
  Serial.begin(115200);
  
  // initialize digital pin LED_BUILTIN as an output.
  // TODO: Why is pullup necessary?
  pinMode(SC_IN, INPUT);
  pinMode(SD_IN, INPUT_PULLUP);
  pinMode(SI_IN, INPUT);
  pinMode(SO_IN, INPUT_PULLUP);
  
  pinMode(SD_OUT, OUTPUT);
  pinMode(SI_OUT, OUTPUT);
  // Disable PWM
  analogWrite(SI_OUT, 0);
  pinMode(SC_OUT, OUTPUT);
  analogWrite(SC_OUT, 0);
  pinMode(LED, OUTPUT);

  digitalWrite(SD_OUT, HIGH);
  digitalWrite(SI_OUT, HIGH);
  digitalWrite(SC_OUT, HIGH);

  state = 0;

  received_low = 0;
  received_high = 0;

  // Set a short timeout for the problem of two
  // random bytes being sent to the Arduino upon
  // initialization of port on PC.
  Serial.setTimeout(50);

  //digitalWrite(LED, LOW);
}

void communication_cycle(int a, int b)
{
  // dummy
}

void loop3()
{
  int i = 0;

  communication_cycle(255, 255);
  
  communication_cycle(0, 0);

  for (i = 0; i < 15; i++) {
    communication_cycle(2, 0x72);
  }

  Serial.write("data: ");

  for (i = 0x60; i >= 0; --i) {
    communication_cycle(2, i);

    Serial.print(reverseBits(received_low), HEX);
    Serial.write(" ");
    Serial.print(reverseBits(received_high), HEX);
    Serial.write(" \n");
  }
}

void loop4()
{
  // Receive exactly three bytes from the PC
  
  uint8_t buffer[3] = {0};

  buffer[0] = 0;
  buffer[1] = 0;
  buffer[2] = 0;
  
  while (Serial.available() > 0) {
    if (Serial.readBytes(buffer, 3) < 3) {
      digitalWrite(LED, LOW);
      return;
    }

    Serial.write(buffer[0]);
    Serial.write(buffer[1]);
    Serial.write(buffer[2]);
    Serial.flush();
  }

  if (buffer[0] == 0) {
    if ((buffer[1] == 0x34) && (buffer[2] == 0x12)) {
      digitalWrite(LED, HIGH);
    }
  }

  return;
}

unsigned long last_send = 0;

void slave_loop()
{
}

void master_loop()
{
  master_transfer(0x00, 0x62);

  Serial.print(reverseBits(received_low), HEX);
  Serial.write(" ");
  Serial.print(reverseBits(received_high), HEX);
  Serial.write(" \n");

  delay(100);
}

void loop()
{
  master_loop();
}

void loop2()
{
  char command = 0;
  uint8_t op1, op2 = 0;

  uint8_t buffer[3] = {0};

  while (Serial.available() > 0) {
    if (Serial.readBytes(buffer, 3) < 3) {
    } else {
      if (buffer[0] == 0) {
        // Update send data
        send_low = buffer[1];
        send_high = buffer[2];
      } else if (buffer[0] == 1) {
        // Clear to send
        state = 1;
        digitalWrite(LED, HIGH);
        delay(10);
      }
    }
  }

  // Add an unlock mechanism if we're stuck
  if (micros() > last_send + 1000) {
    state = 1;
  }

  if (state == 1) {
    // Received data, send it to the GB
    communication_cycle(send_low, send_high);

    Serial.write(reverseBits(received_low));
    Serial.write(reverseBits(received_high));
    Serial.flush();

    last_send = micros();
    
    // Wait for next clear to send
    state = 0;
    digitalWrite(LED, LOW);
    //delay(15);
  }
}
