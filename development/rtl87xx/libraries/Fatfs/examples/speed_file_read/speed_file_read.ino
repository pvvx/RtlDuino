/*
  This sketch shows how to open/close file and test read speed.
*/

#include "SdFatFs.h"

void setup() {}

void test(int rd_buf_size) {
  uint16_t year   = 2016;
  uint16_t month  = 6;
  uint16_t date   = 3;
  uint16_t hour   = 16;
  uint16_t minute = 53;
  uint16_t second = 0;
  uint32_t time, file_size, size;

  char * absolute_filename = new char[rd_buf_size];
  char * buf = new char[512];
  char * fbuf = new char[rd_buf_size];
  printf("\r\nTest read file buf %d bytes:\r\n", rd_buf_size);
  SdFatFs fs;
  fs.begin();
  printf("Dir '\\*.*':\r\n");
  fs.readDir(fs.getRootPath(), buf, 512);

  char *p = buf;
  while (strlen(p) > 0) {
    int i = printf(" ") + printf(p);
    if (i < 8) printf("\t");
    if (i < 16) printf("\t");
    if (i < 32) printf("\t");
    if (i < 48) printf("\t");
    sprintf(absolute_filename, "%s%s", fs.getRootPath(), p);
    fs.getLastModTime(absolute_filename, &year, &month, &date, &hour, &minute, &second);
    printf("\t%04d/%02d/%02d %02d:%02d:%02d\t", year, month, date, hour, minute, second);
    if (fs.isDir(absolute_filename)) {
      printf("dir\r\n");
    }
    else if (fs.isFile(absolute_filename)) {
      SdFatFile file = fs.open(absolute_filename);
      time = millis();
      file_size = 0;
      while ((size = file.read(fbuf, rd_buf_size)) != 0) file_size += size;
      time = millis() - time;
      i = printf("%d bytes", file_size);
      if (i < 8) printf("\t");
      if (i < 16) printf("\t");
      printf("\t");
      i = printf("%d ms", time);
      if (i < 8) printf("\t");
      if (i < 16) printf("\t");
      if(!time) time = 1;
      printf("\t%d KB/s\r\n", file_size / time);
      file.close();
    }
    p += strlen(p) + 1;
  }
  delete[] fbuf;
  delete[] buf;
  delete[] absolute_filename;
  printf("\r\nTest end.\r\n");
  fs.end();
  sys_info();
}

void loop() {
  for(int i = 512; i < 64*1024; i <<= 1) {
    test(i);
    delay(1000);
  }
}
