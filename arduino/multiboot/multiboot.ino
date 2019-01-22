#define SC_IN (2)
#define SD_IN (3)
#define SI_IN (4)
#define SO_IN (5)
#define SD_OUT (8)
// This is slave's SI (master's SO)
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
  // TODO: SC can be either pullup or not, apparently. SI
  // has to be without pullup. SD also not important. It
  // seems that all values don't matter except SI.
  // That's not true. While some things seem to work at first
  // they only work correctly in a specific configuration.
  // The following seems to be quite correct.
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

void loop()
{
  master_loop();
}
