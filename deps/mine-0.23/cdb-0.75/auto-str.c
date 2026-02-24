#include <stdio.h>

#define puts(s) fputs(s, stdout)

main(argc,argv)
int argc;
char **argv;
{
  char *name;
  char *value;
  unsigned char ch;
  char octal[4];

  name = argv[1];
  if (!name) _exit(100);
  value = argv[2];
  if (!value) _exit(100);

  puts("char ");
  puts(name);
  puts("[] = \"\\\n");

  while (ch = *value++) {
    puts("\\");
    octal[3] = 0;
    octal[2] = '0' + (ch & 7); ch >>= 3;
    octal[1] = '0' + (ch & 7); ch >>= 3;
    octal[0] = '0' + (ch & 7);
    puts(octal);
  }

  puts("\\\n\";\n");
  return 0;
}
