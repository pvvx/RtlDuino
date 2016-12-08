/*
  This sketch shows how to open/close file and test read speed.
*/

#include "SdFatFs.h"

void setup() {
  debug_on();
  for (int i = 512; i < 64 * 1024; i <<= 1) {
    test(i);
    delay(1000);
  }
}

void test(int rd_buf_size) {
  uint16_t year   = 2016;
  uint16_t month  = 6;
  uint16_t date   = 3;
  uint16_t hour   = 16;
  uint16_t minute = 53;
  uint16_t second = 0;
  uint32_t time, file_size, size, svn;
  uint8_t attr;

  char * absolute_filename = new char[rd_buf_size];
  char * buf = new char[512];
  char * fbuf = new char[rd_buf_size];
  printf("\nTest read file buf %d bytes:\n", rd_buf_size);
  SdFatFs fs;
  fs.begin();
  if (fs.getCSD((unsigned char *)buf) == 0) {
    printf("\nSD CSD: ");
    for (int i = 0; i < 16; i++)
      printf("%02x", buf[i]);
    printf("\n");
  }
  buf[0] = 0;
  if (fs.getLabel(fs.getRootPath(), buf, &svn) == 0)
    printf("Disk Label: '%s', Serial Number: %p\n", buf, svn);
  printf("\nDir: '%s*.*'\n", fs.getRootPath());
  buf[0] = 0;
  fs.readDir(fs.getRootPath(), buf, 512);

  char *p = buf;
  while (strlen(p) > 0) {
    int i = printf(" ") + printf(p);
    while (i < 40) i += printf(" ");
    sprintf(absolute_filename, "%s%s", fs.getRootPath(), p);
    fs.getLastModTime(absolute_filename, &year, &month, &date, &hour, &minute, &second);
    i += printf(" %04d/%02d/%02d %02d:%02d:%02d", year, month, date, hour, minute, second);
    if (fs.getFsize(absolute_filename, &file_size, &attr) == 0) {
      if (attr & ATTR_ARC) {
        i += printf(" %d bytes", file_size);
        if (file_size > rd_buf_size) {
          while (i < 80) i += printf(" ");
          SdFatFile file = fs.open(absolute_filename);
          file_size = 0;
          time = millis();
          while ((size = file.read(fbuf, rd_buf_size)) != 0) file_size += size;
          time = millis() - time;
          i += printf("read %d bytes %d ms", file_size, time);
          while (i < 120) i += printf(" ");
          if (!time) time = 1;
          printf(" %d KB/s", file_size / time);
          file.close();
        }
      }
      else if (ATTR_DIR) printf(" dir");
      printf("\n");
    }
    p += strlen(p) + 1;
  }

  delete[] fbuf;
  delete[] buf;
  delete[] absolute_filename;
  printf("\nTest end.\n");
  fs.end();
  sys_info();
}

void loop() {

}
