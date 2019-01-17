#define SC_IN (2)
#define SD_IN (3)
#define SI_IN (4)
#define SO_IN (5)
#define SD_OUT (8)
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
  pinMode(SC_IN, INPUT_PULLUP);
  pinMode(SD_IN, INPUT_PULLUP);
  pinMode(SI_IN, INPUT);
  pinMode(SO_IN, INPUT_PULLUP);
  
  pinMode(SD_OUT, OUTPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(SD_OUT, HIGH);

  state = 0;

  received_low = 0;
  received_high = 0;
}

uint8_t reverseBits(uint8_t num) 
{ 
    uint8_t NO_OF_BITS = 8;
    uint8_t reverse_num = 0;
    int i; 
    
    for (i = 0; i < NO_OF_BITS; i++) 
    { 
        if((num & (1 << i))) 
           reverse_num |= 1 << ((NO_OF_BITS - 1) - i);   
   } 
    return reverse_num; 
} 

void send_data(uint8_t low, uint8_t high)
{
  asm (
  "  mov r16, %[input_low]     \n"
  "  mov r17, %[input_high]    \n"

  // Wait for master to signal us to start sending
  "wait_for_so:                \n"
  "  sbic %[pind], %[so_pin]   \n"
  "  rjmp wait_for_so          \n"

  // Send start bit
  "  cbi %[portb], %[sd_pin_out]    \n"
  "  ldi r18, 40               \n"
  "send_wait_bit:              \n"
  "  dec r18                   \n"
  "  brne send_wait_bit        \n"

  // Send low byte
  "  ldi r19, 8                \n"
  "send_one_bit_from_low_byte: \n"
  "  mov r18, r16              \n"
  // NOTE: I used the wrong opcode :(
  // "and" instead of "andi". It probably
  // takes some register and uses its value.
  "  andi r18, 1                \n"
  "  sbrs r18, 0               \n" //skip the next instruction if it's set
  "  rjmp set_to_zero_low      \n"
  "  sbi %[portb], %[sd_pin_out]    \n"
  "  sbi %[portb], %[led_pin]    \n"
  "  rjmp continue_low         \n"
  "set_to_zero_low:            \n"
  "  cbi %[portb], %[sd_pin_out]    \n"
  "  cbi %[portb], %[led_pin]    \n"
  "continue_low:               \n"
  "  lsr r16                   \n"

  // Wait bit time
  "  ldi r18, " XSTR(BIT_TIME) "\n"
  "send_wait_bit_from_low:     \n"
  "  dec r18                   \n"
  "  brne send_wait_bit_from_low        \n"
  
  "  dec r19                   \n"
  "  brne send_one_bit_from_low_byte  \n"

  // Send high byte
  "  ldi r19, 8                \n"
  "send_one_bit_from_high_byte: \n"
  "  mov r18, r17              \n"
  "  andi r18, 1                \n"
  "  sbrs r18, 0               \n" //skip the next instruction if it's set
  "  rjmp set_to_zero_high     \n"
  "  sbi %[portb], %[sd_pin_out]    \n"
  "  sbi %[portb], %[led_pin]    \n"
  "  rjmp continue_high        \n"
  "set_to_zero_high:           \n"
  "  cbi %[portb], %[sd_pin_out]    \n"
  "  cbi %[portb], %[led_pin]    \n"
  "continue_high:              \n"
  "  lsr r17                   \n"

  // Wait bit time
  "  ldi r18, " XSTR(BIT_TIME) "\n"
  "send_wait_bit_from_high:    \n"
  "  dec r18                   \n"
  "  brne send_wait_bit_from_high        \n"
  
  "  dec r19                   \n"
  "  brne send_one_bit_from_high_byte  \n"

  // Set stop bit
  "  sbi %[portb], %[sd_pin_out]    \n"

  "wait_for_clock_high:         \n"
  "  sbis %[pind], %[sc_pin]               \n"
  "  rjmp wait_for_clock_high    \n"
  
    :
    : [portb] "I" (_SFR_IO_ADDR(PORTB)), [sd_pin_out] "I" (PORTB0), [led_pin] "I" (PORTB5),
      [pind] "I" (_SFR_IO_ADDR(PIND)), [sc_pin] "I" (PORTD2), [sd_pin] "I" (PORTD3), [so_pin] "I" (PORTD5),
      [input_low] "r" (low), [input_high] "r" (high)
    : "r16", "r17", "r18", "r19", "r20", "r21", "r22"
    );
}

void receive_data()
{
  asm (
  "  ldi r16, 0                \n" // low byte of result
  "  ldi r17, 0                \n" // high byte of result
  "  ldi r19, 8                \n" // set number of bits in low byte transmission

  "wait_for_start_bit:         \n"
  "  sbic %[pind], %[sd_pin]   \n"
  "  rjmp wait_for_start_bit   \n"
  
  /* Here we'll wait for 70 cycles
     either 70 nops straight or a small loop.
     for every instruction we'll need to compensate though
  */

  "  ldi r20, 24               \n"  
  "get_to_middle_of_bit:       \n"
  "  dec r20                   \n"
  "  brne get_to_middle_of_bit \n"

  // We start the loop by waiting one bit to skip the start bit

  "start_wait_bit:             \n"
  "  ldi r18, 40               \n"
  "get_to_middle_of_bit_in_low_byte: \n"
  "  dec r18                   \n"
  "  brne get_to_middle_of_bit_in_low_byte \n"

  // The way we currently work is by moving the bits all the
  // way to the left when reading. We need to reverse their
  // order after reading everything.

  // Get bit

  "  sbic %[pind], %[sd_pin]   \n"
  "  ori r16, 1                \n"
  "  lsl r16                   \n"

  "  dec r19                   \n"
  "  brne start_wait_bit       \n"

  // High bit

  "  ldi r19, 8                \n"

  // We start the loop by waiting one bit to skip the start bit

  "start_wait_bit_high:             \n"
  "  ldi r18, 40               \n"
  "get_to_middle_of_bit_in_high_byte: \n"
  "  dec r18                   \n"
  "  brne get_to_middle_of_bit_in_high_byte \n"

  // The way we currently work is by moving the bits all the
  // way to the left when reading. We need to reverse their
  // order after reading everything.

  "get_bit_from_high_byte:      \n"
  "  sbic %[pind], %[sd_pin]   \n"
  "  ori r17, 1                \n"
  "  lsl r17                   \n"

  "  dec r19                   \n"
  "  brne start_wait_bit_high       \n"

  "  mov %[output_low], r16    \n"
  "  mov %[output_high], r17    \n"

    : [output_low] "=r" (received_low), [output_high] "=r" (received_high) 
    : [portb] "I" (_SFR_IO_ADDR(PORTB)), [sd_pin_out] "I" (PORTB0),
      [pind] "I" (_SFR_IO_ADDR(PIND)), [sc_pin] "I" (PORTD2), [sd_pin] "I" (PORTD3), [so_pin] "I" (PORTD5)
    : "r16", "r17", "r18", "r19", "r20"
  );
}

void communication_cycle(uint8_t low, uint8_t high)
{
  cli();

  digitalWrite(SD_OUT, HIGH);
  
  receive_data();
  send_data(low, high);

  digitalWrite(SD_OUT, LOW);
  
  sei();
}

void loop2()
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

void loop()
{
  if (state == 0) {
    // Wait for data to be sent
    if (Serial.available() >= 2) {
      send_low = Serial.read();
      send_high = Serial.read();
    
      state = 1;
    }
  } else if (state == 1) {
    // Received data, send it to the GB
    communication_cycle(send_low, send_high);
    Serial.write(reverseBits(received_high));
    Serial.write(reverseBits(received_low));
    Serial.flush();
    
    // Wait for more data
    state = 0;
  } else {
  }
}
