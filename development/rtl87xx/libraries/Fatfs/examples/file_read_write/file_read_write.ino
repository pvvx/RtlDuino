/*
 This sketch shows how to open/close file and perform read/write to it.
 */

#include "SdFatFs.h"

extern "C" {
void UserPreInit(void)
{
   Init_CPU_CLK_UART(7,38400); // 83.3 MHz
   // 0 - 166666666 Hz, 1 - 83333333 Hz, 2 - 41666666 Hz, 3 - 20833333 Hz, 4 - 10416666 Hz, 5 - 4000000 Hz
   // 6 - 200000000 Hz, 7 - 10000000 Hz, 8 - 50000000 Hz, 9 - 25000000 Hz, 10 - 12500000 Hz, 11 - 4000000 Hz
}  
} // extern "C"

char filename[] = "test.txt";
char write_content[] = "hello world!";

SdFatFs fs;

void setup() {
  char buf[128];
  char absolute_filename[128];

  fs.begin();

  printf("write something to \"%s\"\r\n", filename);
  sprintf(absolute_filename, "%s%s", fs.getRootPath(), filename);
  SdFatFile file = fs.open(absolute_filename);

  file.println(write_content);

  file.close();
  printf("write finish\r\n\r\n");


  printf("read back from \"%s\"\r\n", filename);
  file = fs.open(absolute_filename);

  memset(buf, 0, sizeof(buf));
  file.read(buf, sizeof(buf));

  file.close();
  printf("==== content ====\r\n");
  printf("%s", buf);
  printf("====   end   ====\r\n");

  fs.end();
}

void loop() {

}