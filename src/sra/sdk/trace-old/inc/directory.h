#ifndef _h_directory_
#define _h_directory_

#ifndef _h_fdio_
#include "fdio.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*--------------------------------------------------------------------------
 * Directory
 */

/* DirectoryCreate
 */
#define tracedb_DirectoryCreate( path, mode, parents ) \
    tracedb_FSCreateDir ( stdfs, path, mode, parents )


/*--------------------------------------------------------------------------
 * DirList
 *  a listing of entries in the directory
 *  ignores "." and ".."
 */
struct tracedb_DirList
{
    tracedb_fs_t fs;
    unsigned int idx;
    unsigned int len;
    unsigned int last;
    const char **entries;
    void *data;
};

/* DirListInit
 *  creates a directory list via stdfs
 */
#define tracedb_DirListInit( dl, path ) \
    tracedb_FSListDir ( stdfs, dl, path )

/* DirListHead
 *  access the head of the list
 *  sets index to head ( default )
 */
#define tracedb_DirListHead( dl ) \
    ( ( dl ) -> len == 0 ? NULL : ( dl ) -> entries [ ( dl ) -> idx = 0 ] )

/* DirListTail
 *  access the tail of the list
 *  sets index to tail
 */
#define tracedb_DirListTail( dl ) \
    ( ( dl ) -> len == 0 ? NULL : \
      ( dl ) -> entries [ ( dl ) -> idx = ( dl ) -> last ] )

/* DirListNext
 *  access the next item in list
 */
#define tracedb_DirListNext( dl ) \
    ( ( dl ) -> idx >= ( dl ) -> last ? NULL : \
      ( dl ) -> entries [ ++ ( dl ) -> idx ] )

/* DirListPrev
 *  access previous item in list
 */
#define tracedb_DirListPrev( dl ) \
    ( ( dl ) -> idx == 0 ? NULL : ( dl ) -> entries [ -- ( dl ) -> idx ] )

/* DirListWhack
 *  destroy dirlist
 */
int tracedb_DirListWhack ( tracedb_DirList *dl );

#ifdef __cplusplus
}
#endif


#endif /* _h_directory_ */
