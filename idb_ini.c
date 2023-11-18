#include "ff_headers.h"
#include "ff_utils.h"
#include "minIni.h"

#define sizearray(a)  (sizeof(a) / sizeof((a)[0]))

#define DEVICENAME "sd0"
#define MOUNTPOINT "/sd0"

const char inifile[] = "/sd0/test.ini";
const char inifile2[] = "/sd0/testplain.ini";

int CallbackTest(const char *section, const char *key, const char *value, void *userdata)
{
  (void)userdata; /* this parameter is not used in this example */
  printf("    [%s]\t%s=%s\n", section, key, value);
  return 1;
}

int testIni(void)
{
  char str[100];
  long n;
  int s, k;
  char section[50];

  FF_Disk_t *pxDisk = NULL;

  if (!mount(&pxDisk, DEVICENAME, MOUNTPOINT)) 
        return -1;

  /* string reading */
  n = ini_gets("first", "string", "dummy", str, sizearray(str), inifile);
  printf("String: %s, n: %d\n", str, n);
  assert(n==4 && strcmp(str,"noot")==0);
  n = ini_gets("second", "string", "dummy", str, sizearray(str), inifile);
  assert(n==4 && strcmp(str,"mies")==0);
  n = ini_gets("first", "undefined", "dummy", str, sizearray(str), inifile);
  assert(n==5 && strcmp(str,"dummy")==0);
  /* ----- */
  n = ini_gets("", "string", "dummy", str, sizearray(str), inifile2);
  assert(n==4 && strcmp(str,"noot")==0);
  n = ini_gets(NULL, "string", "dummy", str, sizearray(str), inifile2);
  assert(n==4 && strcmp(str,"noot")==0);
  /* ----- */
  printf("1. String reading tests passed\n");

  /* value reading */
  n = ini_getl("first", "val", -1, inifile);
  assert(n==1);
  n = ini_getl("second", "val", -1, inifile);
  assert(n==2);
  n = ini_getl("first", "undefined", -1, inifile);
  assert(n==-1);
  /* ----- */
  n = ini_getl(NULL, "val", -1, inifile2);
  assert(n==1);
  /* ----- */
  printf("2. Value reading tests passed\n");

  /* string writing */
  n = ini_puts("first", "alt", "flagged as \"correct\"", inifile);
  assert(n==1);
  n = ini_gets("first", "alt", "dummy", str, sizearray(str), inifile);
  assert(n==20 && strcmp(str,"flagged as \"correct\"")==0);
  /* ----- */
  n = ini_puts("second", "alt", "correct", inifile);
  assert(n==1);
  n = ini_gets("second", "alt", "dummy", str, sizearray(str), inifile);
  assert(n==7 && strcmp(str,"correct")==0);
  /* ----- */
  n = ini_puts("third", "test", "correct", inifile);
  assert(n==1);
  n = ini_gets("third", "test", "dummy", str, sizearray(str), inifile);
  assert(n==7 && strcmp(str,"correct")==0);
  /* ----- */
  n = ini_puts("second", "alt", "overwrite", inifile);
  assert(n==1);
  n = ini_gets("second", "alt", "dummy", str, sizearray(str), inifile);
  assert(n==9 && strcmp(str,"overwrite")==0);
  /* ----- */
  n = ini_puts("second", "alt", "123456789", inifile);
  assert(n==1);
  n = ini_gets("second", "alt", "dummy", str, sizearray(str), inifile);
  assert(n==9 && strcmp(str,"123456789")==0);
  /* ----- */
  n = ini_puts(NULL, "alt", "correct", inifile2);
  assert(n==1);
  n = ini_gets(NULL, "alt", "dummy", str, sizearray(str), inifile2);
  assert(n==7 && strcmp(str,"correct")==0);
  /* ----- */
  printf("3. String writing tests passed\n");

  /* section/key enumeration */
  printf("4. Section/key enumeration, file structure follows\n");
  for (s = 0; ini_getsection(s, section, sizearray(section), inifile) > 0; s++) {
    printf("    [%s]\n", section);
    for (k = 0; ini_getkey(section, k, str, sizearray(str), inifile) > 0; k++) {
      printf("\t%s\n", str);
    } /* for */
  } /* for */

  /* section/key presence check */
  assert(ini_hassection("first", inifile));
  assert(!ini_hassection("fourth", inifile));
  assert(ini_haskey("first", "val", inifile));
  assert(!ini_haskey("first", "test", inifile));
  printf("5. checking presence of sections and keys passed\n");

  /* browsing through the file */
  printf("6. browse through all settings, file field list follows\n");
  ini_browse(CallbackTest, NULL, inifile);

  /* string deletion */
  n = ini_puts("first", "alt", NULL, inifile);
  assert(n==1);
  n = ini_puts("second", "alt", NULL, inifile);
  assert(n==1);
  n = ini_puts("third", NULL, NULL, inifile);
  assert(n==1);
  /* ----- */
  n = ini_puts(NULL, "alt", NULL, inifile2);
  assert(n==1);
  printf("7. String deletion tests passed\n");

  return 0;
}