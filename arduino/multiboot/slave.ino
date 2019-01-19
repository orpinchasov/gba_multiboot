// TODO: All the labels here and duplicated, we need to get rid of that

#ifdef false

void slave_send(uint8_t low, uint8_t high)
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

void slave_receive()
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

void slave_transfer(uint8_t low, uint8_t high)
{
  cli();

  digitalWrite(SD_OUT, HIGH);
  
  slave_receive();
  slave_send(low, high);

  digitalWrite(SD_OUT, LOW);
  
  sei();
}

void slave_loop()
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

#endif
