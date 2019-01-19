void master_send(uint8_t low, uint8_t high)
{
  asm (
  "  mov r16, %[input_low]     \n"
  "  mov r17, %[input_high]    \n"

  // Verify SD is HIGH (everyone ready)
  "wait_for_sd:                \n"
  "  sbis %[pind], %[sd_pin]   \n"
  "  rjmp wait_for_sd          \n"

  // Set SC to LOW
  "  cbi %[portb], %[sc_pin_out]    \n"

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

  // Wait stop bit time
  "  ldi r18, " XSTR(BIT_TIME) "\n"
  "wait_stop_bit:    \n"
  "  dec r18                   \n"
  "  brne wait_stop_bit        \n"

  // Set SO to LOW so slave can begin transfer
  "  cbi %[portb], %[si_pin_out]    \n"

    :
    : [portb] "I" (_SFR_IO_ADDR(PORTB)), [sd_pin_out] "I" (PORTB0), [si_pin_out] "I" (PORTB1), [sc_pin_out] "I" (PORTB4), [led_pin] "I" (PORTB5),
      [pind] "I" (_SFR_IO_ADDR(PIND)), [sc_pin] "I" (PORTD2), [sd_pin] "I" (PORTD3), [so_pin] "I" (PORTD5),
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

  // Wait for the stop bit to end
  "  ldi r20, 24               \n"  
  "get_to_middle_of_last_bit:  \n"
  "  dec r20                   \n"
  "  brne get_to_middle_of_last_bit \n"

  // Set SO to HIGH to signal end of slave transfer
  "  sbi %[portb], %[si_pin_out]    \n"

  // Wait for timeout to end transfer
  "  ldi r20, 100              \n"
  "wait_for_timeout:           \n"
  "  dec r20                   \n"
  "  brne wait_for_timeout     \n"
  
  // Set SC to HIGH to end transfer
  "  sbi %[portb], %[sc_pin_out]    \n"

  "  mov %[output_low], r16    \n"
  "  mov %[output_high], r17   \n"

    : [output_low] "=r" (received_low), [output_high] "=r" (received_high) 
    : [portb] "I" (_SFR_IO_ADDR(PORTB)), [sd_pin_out] "I" (PORTB0), [si_pin_out] "I" (PORTB1), [sc_pin_out] "I" (PORTB4),
      [pind] "I" (_SFR_IO_ADDR(PIND)), [sc_pin] "I" (PORTD2), [sd_pin] "I" (PORTD3), [so_pin] "I" (PORTD5)
    : "r16", "r17", "r18", "r19", "r20"
  );
}

void master_transfer(uint8_t low, uint8_t high)
{
  cli();

  // TODO: I don't know at the moment why adding these
  // causes everything to collapse???
  //digitalWrite(SD_OUT, HIGH);

  master_send(low, high);
  master_receive();

  //digitalWrite(SD_OUT, LOW);
  
  sei();
}
