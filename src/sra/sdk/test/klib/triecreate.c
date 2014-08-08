/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#include <kapp/main.h>
#include <klib/log.h>
#include <klib/rc.h>

#include <klib/trie.h>
#include <klib/ptrie.h>
#include <klib/vector.h>
#include <klib/impl.h>
#include <kfs/directory.h>
#include <kfs/file.h>
#include <klib/printf.h>
#include <os-native.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Version  EXTERN
 *  return 4-part version code: 0xMMmmrrrr, where
 *      MM = major release
 *      mm = minor release
 *    rrrr = bug-fix release
 */
uint32_t KAppVersion ( void )
{
    return 0;
}


typedef struct MyKVPair {
  TNode tnode;
  int value;
  char key[1];
} MyKVPair;


Trie t;

static
rc_t MyAux ( void *param, const void *node, size_t *num_writ, PTWriteFunc write, void *write_param ) 
{
  char buffer[1024];
  MyKVPair *pair = (MyKVPair *)node;
  int len;

  len = snprintf(buffer, sizeof buffer, "%d %s", pair->value, pair->key);

  if (write != NULL) {
    return (*write)(write_param, buffer, len, num_writ);
  }

  *num_writ = len;
  return 0;
}

static
rc_t MyWrite ( void *param, const void *buffer, size_t bytes, size_t *num_writ )
{
  FILE *fp = param;
  * num_writ = fwrite(buffer, 1, bytes, fp);
  if ( * num_writ != bytes )
    return -1;
  return 0;
}

rc_t Persist(const char *filename) {
  rc_t rc;
  size_t num_writ;
  char buf[48];
  
  FILE *fp = fopen(filename, "w");

  /* it looks like you used some of the routines from inside Trie code
     the "Null*" functions are ones that are not intended to write, but
     are just used as dummies to calculate byte offsets.

     you, on the other hand, want real functions, such as
     the ones you provided! */
  rc = TriePersist( &t, &num_writ, false, MyWrite, fp, MyAux, NULL);

  fclose(fp);

  if (rc != 0) {
    LOGERR ( klogInt, rc, "persist" );    
  }
  string_printf(buf, sizeof(buf), NULL, "Number of bytes needed: %zu\n",
                num_writ);
  fputs(buf, stderr);
  return 0;
}

void TrieRead(bool swap)
{
  struct stat sbuf;
  FILE *fp;
  PTrie *pt;
  size_t siz;
  char *fbuf;
  char buf[1024];
  String key;
  int len;
  rc_t rc;
  PTNode ptn;
  uint32_t id;
  size_t count;

  if (stat("/tmp/foo", &sbuf)) {
    return;
  }
  siz = sbuf.st_size;
  fbuf = malloc(siz+1);
  fp = fopen("/tmp/foo", "r");
  count = fread(fbuf, 1, siz, fp);
  if (count != siz) {
    fprintf(stderr, "Only read %ld of %ld\n", count, siz);
    return;
  }
  fbuf[siz] = '\0';
  rc = PTrieMake(&pt, fbuf, siz, swap);
  if (rc != 0 || pt == NULL) {
    fprintf(stderr, "PTrieMake failed.\n");
    return;
  }
  
  while (NULL != fgets(buf, 1024, stdin)) {
    len = strlen(buf);
    buf[--len] = '\0';
    StringInitCString( &key, buf );

    id = PTrieFind(pt, &key, &ptn, NULL, NULL);
    if ( id == 0 )
      fprintf(stderr, "not found\n");
    else
      fprintf(stderr, "id found: %d - '%.*s'\n", id, (int) ptn.data.size, ptn.data.addr);
  }
}  

rc_t KMain ( int argc, char *argv [] )
{
  rc_t rc;
  MyKVPair *pair;
  char buf[1024];
  int counter = 0;
  String key;
  int len;

  if (argc > 1 && !strcmp(argv[1], "-read")) {
    TrieRead(false);
    return 0;
  }

  if (argc > 1 && !strcmp(argv[1], "-swapread")) {
    TrieRead(true);
    return 0;
  }

  /* the documentation claims to return Unix status codes
     EINVAL or ENOMEM. however, it's more likely we just
     ran out of energy to update the comments...

     this creates a Trie with an initial character set of
     0-9, but which expands to accept new characters as seen
     thanks to the "true" value for "cs_expand"

     a standard Trie would encode the entire string into
     the prefix-tree, while this version only encodes as much
     of the strings initial characters as necessary before
     depositing the nodes into a BSTree at the leaves. you have
     told the code that it can accept up to 512 nodes in the
     BSTree until it has to split them up. to get more standard
     Trie behavior, choose a lower number, e.g. 1 */
  rc = TrieInit(&t, "a-z", 1, true);
  if (rc != 0) 
    LOGERR ( klogInt, rc, "triecreate" );

  /* you might want to get out after such an error... */

  /* another comment for you - using "buf, sizeof buf, stdin"
     will be safer ( I know this is just for experimentation,
     but since I'm reviewing it, I want you to get your money's worth */
  while (NULL != fgets(buf, 1024, stdin)) {
    len = strlen(buf);
    buf[--len] = '\0';
    /* printf("%d\n", len); */
    if (len <= 0)
      break;

    /* in C++, the cast to MyKVPair* is required, whereas
       in C it's not - which is part of its beauty. I prefer
       to avoid the cast unless we're planning on porting to C++,
       because the cast prevents you from changing the type of "pair"
       up above. same goes for "sizeof" - I prefer to let the compiler
       choose the right size for the allocation as "sizeof *pair"
       which will always be correct, even if I were to retype "pair" */
    pair = (MyKVPair *)malloc(sizeof(MyKVPair) + len + 1);
    if (pair == NULL) {
      fprintf(stderr, "Error in malloc\n");
      return -1;
    }
    strcpy(pair->key, buf);
    StringInitCString( &pair->tnode.key, (const char *)&pair->key );
    pair->value = counter++;
    /* fprintf(stderr, "At trieinsert\n"); */

    /* here I'd prefer "&pair->tnode" rather than the typecast
       ( do you get the idea I worry about typecasts? ) */
    rc = TrieInsert( &t, (TNode *)pair );
    if (rc != 0) 
      LOGERR ( klogInt, rc, "triecreate" );    
  }
  while (NULL != fgets(buf, 1024, stdin)) {
    len = strlen(buf);
    buf[--len] = '\0';
    StringInitCString( &key, buf );

    /* the first cast is required, although would normally
       want to be a "const MyKVPair*" so that you are sure
       not to modify it. the second cast is unnecessary */
    pair = (MyKVPair *)TrieFind(&t, (const String *)&key);
    if (pair != NULL) {
      printf("%s %d\n", buf, pair->value);
    }
  }
  Persist("/tmp/foo");
  return 0;
}
  
