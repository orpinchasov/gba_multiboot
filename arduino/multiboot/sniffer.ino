uint32_t sniff()
{
  uint8_t master_low_byte = 0;
  uint8_t master_high_byte = 0;

  uint8_t slave_low_byte = 0;
  uint8_t slave_high_byte = 0;

  // Waits (m - master, s - slave):
  // wait 0 - start bit
  // wait 1 - get to middle of bit
  // wait 2 - one bit in low byte
  // wait 3 - one bit in high byte
  // wait 4 - stop bit

  cli();
  
  asm (
  "  ldi r16, 0                    \n" // low byte of result from master
  "  ldi r17, 0                    \n" // high byte of result from master
  "  ldi r19, 8                    \n" // set number of bits in low byte transmission

  // SC low signals start of transfer
  "m_wait_sc:                      \n"
  "  sbic %[pind], %[sc_pin]       \n"
  "  rjmp m_wait_sc                \n"

  "m_wait_0_loop:                  \n"
  "  sbic %[pind], %[sd_pin]       \n"
  "  rjmp m_wait_0_loop            \n"
  
  // Here we'll wait for 70 cycles
  // either 70 nops straight or a small loop.
  // for every instruction we'll need to compensate though

  "  ldi r20, 24                   \n"  
  "m_wait_1_loop:                  \n"
  "  dec r20                       \n"
  "  brne m_wait_1_loop            \n"

  // We start the loop by waiting one bit to skip the start bit

  "m_wait_2_start:                 \n"
  "  ldi r18, 40                   \n"
  "m_wait_2_loop:                  \n"
  "  dec r18                       \n"
  "  brne m_wait_2_loop            \n"

  // The way we currently work is by moving the bits all the
  // way to the left when reading. We need to reverse their
  // order after reading everything.

  // Get bit

  "  sbic %[pind], %[sd_pin]       \n"
  "  ori r16, 1                    \n"
  "  lsl r16                       \n"

  "  dec r19                       \n"
  "  brne m_wait_2_start           \n"

  // High byte

  "  ldi r19, 8                    \n"

  "m_wait_3_start:                 \n"
  "  ldi r18, 40                   \n"
  "m_wait_3_loop:                  \n"
  "  dec r18                       \n"
  "  brne m_wait_3_loop            \n"

  "  sbic %[pind], %[sd_pin]       \n"
  "  ori r17, 1                    \n"
  "  lsl r17                       \n"

  "  dec r19                       \n"
  "  brne m_wait_3_start           \n"

  "  mov %[m_low], r16             \n"
  "  mov %[m_high], r17            \n"

  "m_wait_4_start:                 \n"
  "  sbis %[pind], %[sd_pin]       \n"
  "  rjmp m_wait_4_start           \n"

  // Start slave sniffing

  "  ldi r16, 0                    \n" // low byte of result from slave
  "  ldi r17, 0                    \n" // high byte of result from slave
  "  ldi r19, 8                    \n" // set number of bits in low byte transmission

  // Wait for start bit of slave
  "s_wait_0_start:                 \n"
  "  sbic %[pind], %[sd_pin]       \n"
  "  rjmp s_wait_0_start           \n"

  "  ldi r20, 24                   \n"  
  "s_wait_1_loop:                  \n"
  "  dec r20                       \n"
  "  brne s_wait_1_loop            \n"

  "s_wait_2_start:                 \n"
  "  ldi r18, 40                   \n"
  "s_wait_2_loop:                  \n"
  "  dec r18                       \n"
  "  brne s_wait_2_loop            \n"

  // The way we currently work is by moving the bits all the
  // way to the left when reading. We need to reverse their
  // order after reading everything.

  // Get bit

  "  sbic %[pind], %[sd_pin]       \n"
  "  ori r16, 1                    \n"
  "  lsl r16                       \n"

  "  dec r19                       \n"
  "  brne s_wait_2_start           \n"

  // High byte

  "  ldi r19, 8                    \n"

  "s_wait_3_start:                 \n"
  "  ldi r18, 40                   \n"
  "s_wait_3_loop:                  \n"
  "  dec r18                       \n"
  "  brne s_wait_3_loop            \n"

  "  sbic %[pind], %[sd_pin]       \n"
  "  ori r17, 1                    \n"
  "  lsl r17                       \n"

  "  dec r19                       \n"
  "  brne s_wait_3_start           \n"

  "  mov %[s_low], r16             \n"
  "  mov %[s_high], r17            \n"

    : [m_low] "=r" (master_low_byte), [m_high] "=r" (master_high_byte),
      [s_low] "=r" (slave_low_byte), [s_high] "=r" (slave_high_byte)
    : [pind] "I" (_SFR_IO_ADDR(PIND)), [sc_pin] "I" (PORTD2), [sd_pin] "I" (PORTD3), [si_pin] "I" (PORTD4), [so_pin] "I" (PORTD5)
    : "r16", "r17", "r18", "r19", "r20"
  );

  sei();

  return ((uint32_t)reverseBits(slave_high_byte)) << 24 | \
         ((uint32_t)reverseBits(slave_low_byte)) << 16 | \
         reverseBits(master_high_byte) << 8 | \
         reverseBits(master_low_byte);
}

void sniffer_loop()
{
  uint32_t value = 0;
  
  pinMode(SC_IN, INPUT);
  pinMode(SD_IN, INPUT);
  pinMode(SI_IN, INPUT);
  pinMode(SO_IN, INPUT);

  while (1) {
    value = sniff();
    Serial.write((byte *)&value, sizeof(value));
    Serial.flush();
  }
}
