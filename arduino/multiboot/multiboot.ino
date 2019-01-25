#define PIN_SC (2)
#define PIN_SD (3)
#define PIN_SO (4) // Arduino as master SO - GBA as slave SI

// Used in conjunction with logic analyzer to accurately
// verify our timing.
#define DEBUG (12)
#define LED (13)

#define BIT_TIME (40)

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

  pinMode(PIN_SC, OUTPUT);
  pinMode(PIN_SD, INPUT);
  pinMode(PIN_SO, OUTPUT);

  pinMode(DEBUG, OUTPUT);
  pinMode(LED, OUTPUT);
  
  // Disable PWM
  analogWrite(PIN_SC, 0);

  digitalWrite(PIN_SC, HIGH);
  digitalWrite(PIN_SO, HIGH);

  digitalWrite(DEBUG, HIGH);

  state = 0;

  received_low = 0;
  received_high = 0;

  // Set a short timeout for the problem of two
  // random bytes being sent to the Arduino upon
  // initialization of port on PC.
  // TODO: Is this still relevant?
  Serial.setTimeout(50);

  //digitalWrite(LED, LOW);
}

void loop()
{
  master_loop();
}
