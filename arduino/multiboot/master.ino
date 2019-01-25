void master_send(uint8_t low, uint8_t high)
{
  asm (
  "  mov r16, %[input_low]     \n"
  "  mov r17, %[input_high]    \n"

  // Verify SD is HIGH (everyone ready)
  "wait_for_sd:                \n"
  "  sbis %[pind], %[pin_sd]   \n"
  "  rjmp wait_for_sd          \n"

  // Set SC to LOW
  "  cbi %[portd], %[pin_sc]    \n"

  // Switch SD to output (as it was configured with
  // pullup resistors it will be configured with
  // HIGH)
  "  sbi %[ddrd], %[pin_sd_direction]     \n"

  // Send start bit (with debug)
  "  sbi %[portb], %[debug_pin]     \n"
  "  cbi %[portd], %[pin_sd]    \n"
  "  ldi r18, " XSTR(BIT_TIME) "\n"
  "send_wait_bit:              \n"
  "  dec r18                   \n"
  "  brne send_wait_bit        \n"

  // Send low byte
  "  ldi r19, 8                \n"
  "send_one_bit_from_low_byte: \n"
  
  // Debug
  "  cbi %[portb], %[debug_pin]     \n"
  
  "  mov r18, r16              \n"
  // NOTE: I used the wrong opcode :(
  // "and" instead of "andi". It probably
  // takes some register and uses its value.
  "  andi r18, 1                \n"
  "  sbrs r18, 0               \n" //skip the next instruction if it's set
  "  rjmp set_to_zero_low      \n"
  "  sbi %[portd], %[pin_sd]    \n"
  "  sbi %[portb], %[led_pin]    \n"
  "  rjmp continue_low         \n"
  "set_to_zero_low:            \n"
  "  cbi %[portd], %[pin_sd]    \n"
  "  cbi %[portb], %[led_pin]    \n"
  "continue_low:               \n"
  "  lsr r16                   \n"

  // Debug while bit is transmitted
  "  sbi %[portb], %[debug_pin]     \n"

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
  // Debug
  "  cbi %[portb], %[debug_pin]     \n"
  
  "  mov r18, r17              \n"
  "  andi r18, 1                \n"
  "  sbrs r18, 0               \n" //skip the next instruction if it's set
  "  rjmp set_to_zero_high     \n"
  "  sbi %[portd], %[pin_sd]    \n"
  "  sbi %[portb], %[led_pin]    \n"
  "  rjmp continue_high        \n"
  "set_to_zero_high:           \n"
  "  cbi %[portd], %[pin_sd]    \n"
  "  cbi %[portb], %[led_pin]    \n"
  "continue_high:              \n"
  "  lsr r17                   \n"

  // Debug while bit is transmitted
  "  sbi %[portb], %[debug_pin]     \n"

  // Wait bit time
  "  ldi r18, " XSTR(BIT_TIME) "\n"
  "send_wait_bit_from_high:    \n"
  "  dec r18                   \n"
  "  brne send_wait_bit_from_high        \n"
  
  "  dec r19                   \n"
  "  brne send_one_bit_from_high_byte  \n"

  // Set stop bit 
  "  sbi %[portd], %[pin_sd]    \n"

  // Clear debug while stop bit is transmitted
  "  cbi %[portb], %[debug_pin]     \n"

  // Wait stop bit time
  "  ldi r18, 100\n"
  "wait_stop_bit:    \n"
  "  dec r18                   \n"
  "  brne wait_stop_bit        \n"

  // Return SD to input (as it was configured with
  // HIGH, it'll have the pullup resistors configured)
  "  cbi %[ddrd], %[pin_sd_direction]     \n"

  // Set SO to LOW so slave can begin transfer
  "  cbi %[portd], %[pin_so]    \n"

    :
    : [debug_pin] "I" (PORTB4), [led_pin] "I" (PORTB5),
      [portb] "I" (_SFR_IO_ADDR(PORTB)), [portd] "I" (_SFR_IO_ADDR(PORTD)),
      [pind] "I" (_SFR_IO_ADDR(PIND)), [pin_sc] "I" (PORTD2), [pin_sd] "I" (PORTD3), [pin_so] "I" (PORTD4),
      [ddrd] "I" (_SFR_IO_ADDR(DDRD)), [pin_sd_direction] "I" (DDD5),
      [input_low] "r" (low), [input_high] "r" (high)
    : "r16", "r17", "r18", "r19", "r20", "r21", "r22"
    );
}

void master_receive()
{
  asm (
  "  ldi r16, 0                \n" // low byte of result
  "  ldi r17, 0                \n" // high byte of result
  "  ldi r19, 8                \n" // set number of bits in low byte transmission

  "wait_for_start_bit:         \n"
  "  sbic %[pind], %[pin_sd]   \n"
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
  "  ldi r18, " XSTR(BIT_TIME) "\n"
  "get_to_middle_of_bit_in_low_byte: \n"
  "  dec r18                   \n"
  "  brne get_to_middle_of_bit_in_low_byte \n"

  // The way we currently work is by moving the bits all the
  // way to the left when reading. We need to reverse their
  // order after reading everything.

  // Get bit

  "  sbic %[pind], %[pin_sd]   \n"
  "  ori r16, 1                \n"
  "  lsl r16                   \n"

  "  dec r19                   \n"
  "  brne start_wait_bit       \n"

  // High bit

  "  ldi r19, 8                \n"

  // We start the loop by waiting one bit to skip the start bit

  "start_wait_bit_high:             \n"
  "  ldi r18, " XSTR(BIT_TIME) "\n"
  "get_to_middle_of_bit_in_high_byte: \n"
  "  dec r18                   \n"
  "  brne get_to_middle_of_bit_in_high_byte \n"

  // The way we currently work is by moving the bits all the
  // way to the left when reading. We need to reverse their
  // order after reading everything.

  "get_bit_from_high_byte:      \n"
  "  sbic %[pind], %[pin_sd]   \n"
  "  ori r17, 1                \n"
  "  lsl r17                   \n"

  "  dec r19                   \n"
  "  brne start_wait_bit_high       \n"

  // Wait for the stop bit to end
  "  ldi r20, 24               \n"  
  "get_to_middle_of_last_bit:  \n"
  "  dec r20                   \n"
  "  brne get_to_middle_of_last_bit \n"

  // TODO: It's possible that we'd like to move this
  // right before setting SC to high.
  // Set SO to HIGH to signal end of slave transfer
  "  sbi %[portd], %[pin_so]    \n"

  // TODO: This has to be checked (if I reduced this to 40 though it
  // was too short and the transfers failed). Apparently, it's really
  // important. If I increase it from 100 to 150 I get much better
  // results in quick sends.
  // Wait for timeout to end transfer
  "  ldi r20, 200              \n"
  "wait_for_timeout:           \n"
  "  dec r20                   \n"
  "  brne wait_for_timeout     \n"
  
  // Set SC to HIGH to end transfer
  "  sbi %[portd], %[pin_sc]    \n"

  "  mov %[output_low], r16    \n"
  "  mov %[output_high], r17   \n"

    : [output_low] "=r" (received_low), [output_high] "=r" (received_high) 
    : [portb] "I" (_SFR_IO_ADDR(PORTB)), [portd] "I" (_SFR_IO_ADDR(PORTD)),
      [pin_sc] "I" (PORTD2), [pin_sd] "I" (PORTD3), [pin_so] "I" (PORTD4),
      [pind] "I" (_SFR_IO_ADDR(PIND))
    : "r16", "r17", "r18", "r19", "r20"
  );
}

void master_transfer(uint8_t low, uint8_t high)
{
  cli();

  master_send(low, high);
  master_receive();
  
  sei();
}

void master_loop()
{
  char send_low = 0;
  char send_high = 0;

  /* TODO: This was supposed to remove corrupted commands
   *  from the serial queue but it seems to introduct quite
   *  a big lag.
   *
  if (Serial.available() != 2) {
    delay(50);
    int amount = 0;
    if (amount = Serial.available() != 2) {
      for (int i=0; i < amount; ++i) {
        Serial.read();
        Serial.write(0);
      }
    }
  }
  */

  if (Serial.available() >= 2) {
    // NOTE: Here I had something really terrible.
    // I declared these variables here and used them
    // outside of the scope of this 'if' statement.
    // There was no warning whatsoever and they
    // were simply replaced by 0s. Took me a couple of
    // hours to debug this thing :/
    send_low = Serial.read();
    send_high = Serial.read();
  } else {
    return;
  }

  master_transfer(send_low, send_high);

  Serial.write(reverseBits(received_low));
  Serial.write(reverseBits(received_high));
  Serial.flush();

  //delay(60);

  return;
}
