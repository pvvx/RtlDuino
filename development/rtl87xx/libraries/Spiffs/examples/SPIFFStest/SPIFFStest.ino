#include <Spiffs.h>

void setup()
{
  Serial.begin(38400);

  SPIFFS.begin();
  File tmp = SPIFFS.open("/tmp.txt", "r");
  if (!tmp) {
    printf("Format...\n");
    SPIFFS.format();
  }

  FSInfo info;
  SPIFFS.info(info);
  printf("===== SPIFFS Info ====\n");
  printf("Total: %u\nUsed: %u\nBlock: %u\nPage: %u\nMax open files: %u\nMax path len: %u\n",
         info.totalBytes,
         info.usedBytes,
         info.blockSize,
         info.pageSize,
         info.maxOpenFiles,
         info.maxPathLength
        );

  printf("======================\n");

  if (!tmp) {
    printf("Open file...\n");
    tmp = SPIFFS.open("/tmp.txt", "w");
    printf("Write data...\n");
    tmp.println("SPIFFS Test");
    printf("Close file...\n");
    tmp.close();
    //    printf("Rename file...\n");
    //    SPIFFS.rename("/tmp.txt", "/tmp1.txt");
    tmp = SPIFFS.open("/tmp.txt", "r");
  }
  if (tmp) {
    printf("Read file...\n");
    String txt = tmp.readString();
    printf("Close file...\n");
    tmp.close();
    printf("Data file: ");  Serial.println(txt);
  }
  printf("Test End.\n");
  while (1);
}


void loop()
{
}
