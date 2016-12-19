#include <FlashMemory.h>

void setup() {
  unsigned int value;
  printf("\nChipID: 0x%02X, CLK: %d Hz\n", HalGetChipId(), HalGetCpuClk());
  printf("FlashID: 0x%06X, FlashSize: %d bytes\n", GetFlashId(), GetFlashSize());
  /* request flash size 0x4000 from 0x7C000 */
  FlashMemory.begin(0x7C000, 0x4000);

  /* read one word (32-bit) from 0x7C000 plus offset 0x3F00 */
  value = FlashMemory.readWord(0x3F00);

  printf("value is 0x%08X\r\n", value);

  if (value == 0xFFFFFFFF) {
    value = 0;
  } else {
    value++;
  }

  /* write one word (32-bit) to 0x7C000 plus offset 0x3F00 */
  FlashMemory.writeWord(0x3F00, value);
}

void loop() {
  // put your main code here, to run repeatedly:

}
