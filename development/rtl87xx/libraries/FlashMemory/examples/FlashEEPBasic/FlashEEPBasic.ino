/*
   This sketch demonstrates the use of flash EEP memory
*/

#include <FlashEEP.h>

#define MY_OBJ_ID 0x1234

FlashEEPClass feep;

void setup() {
  Serial.begin(38400);
  char *pbuf = new char [32];
  printf("\nRead(ObjID: %p)...", MY_OBJ_ID);
  short ret = feep.read((void *)pbuf, MY_OBJ_ID, 32);
  if (ret > 0) printf(" Size: %d, '%s'\n", ret, pbuf);
  else if (ret == 0) printf(" Size: 0 -> deleted!\n");
  else if (ret == -1) printf(" not found!\n", MY_OBJ_ID);
  else printf(" error!\n");
  printf("\nWrite(ObjID: %p, '%s') = %d\n",
         MY_OBJ_ID,
         pbuf,
         feep.write((void *)pbuf, MY_OBJ_ID, sprintf(pbuf, "Hello!")));
  delete [] pbuf;
}

void loop() {
  char *pbuf = new char [32];
  printf("\nRead(ObjID: %p)...", MY_OBJ_ID);
  short ret = feep.read((void *)pbuf, MY_OBJ_ID, 32);
  if (ret > 0) {
    printf(" Size: %d, '%s'\n", ret, pbuf);
    char c = Serial.read();
    printf("Clear Obj? [Y/N] %c\n", c);
    if (c  == 'y' || c == 'Y') {
      printf("\nClear(ObjID: %p) = %d\n", MY_OBJ_ID, feep.write((void *)0, MY_OBJ_ID, 0));
    }
  }
  else if (ret == 0) {
    printf(" Size: 0 -> deleted!\n");
    printf("\nTest end.\n");
    while (1);
  }
  else {
    printf(" error!\n");
    printf("\nTest end.\n");
    while (1);
  }
  delay(5000);
  delete [] pbuf;
}
