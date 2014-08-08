#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <byteswap.h>

struct teststr {
  union {
    uint16_t all;
    struct {
      uint16_t ext_keys  : 1;
      uint16_t backtrace : 1;
      uint16_t id_coding : 3;
      uint16_t unused    : 11;
    } s;
    struct {
      uint16_t unused    : 11;
      uint16_t id_coding : 3;
      uint16_t backtrace : 1;
      uint16_t ext_keys  : 1;
    } sw;
  } u;
};



int main(int ac, char **av)
{
  struct teststr s;
  FILE *fp;
  int i;

  if (ac > 2 && !strcmp(av[1], "write")) {

    fp = fopen("/tmp/foobar", "w");

    if (!fp) {
      fprintf(stderr, "Can't open /tmp/foobar for write.\n");
      return 0;
    }
    
    bzero(&s, sizeof(s));
    s.u.s.ext_keys = 1;
    fwrite(&s, sizeof(s), 1, fp);
    
    bzero(&s, sizeof(s));
    s.u.s.backtrace = 1;
    fwrite(&s, sizeof(s), 1, fp);
    
    bzero(&s, sizeof(s));
    s.u.s.id_coding = 3;
    fwrite(&s, sizeof(s), 1, fp);

    bzero(&s, sizeof(s));
    s.u.s.unused = 1436;
    fwrite(&s, sizeof(s), 1, fp);

    fclose(fp);
  }

  fp = fopen("/tmp/foobar", "r");

  if (!fp) {
    fprintf(stderr, "Can't open /tmp/foobar.\n");
    return 0;
  }

  if (ac> 2 && !strcmp(av[2], "swap")) {
    for (i=0; i<4; i++) {
      fread(&s, sizeof(s), 1, fp);
      s.u.all = bswap_16(s.u.all);
      printf("Ext keys: %d\n", s.u.sw.ext_keys);
      printf("Backtrace: %d\n", s.u.sw.backtrace);
      printf("Id coding: %d\n", s.u.sw.id_coding);
      printf("Unused: %d\n", s.u.sw.unused);
    }
  } else {
    for (i=0; i<4; i++) {
      fread(&s, sizeof(s), 1, fp);
      printf("Ext keys: %d\n", s.u.s.ext_keys);
      printf("Backtrace: %d\n", s.u.s.backtrace);
      printf("Id coding: %d\n", s.u.s.id_coding);
      printf("Unused: %d\n", s.u.s.unused);
    }
  }

}
