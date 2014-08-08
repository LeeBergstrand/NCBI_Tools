#ifndef _h_container_
#define _h_container_

#ifndef _h_itypes_
#include "itypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * SLNode
 *  singly linked node
 */
typedef struct tracedb_SLNode tracedb_SLNode;
struct tracedb_SLNode
{
    tracedb_SLNode *next;
};

/* SLNodeNext
 *  returns next node
 */
#define tracedb_SLNodeNext( n ) \
    ( n ) -> next

/* SLNodeFindNext
 *  find next element satisfying criteria
 */
tracedb_SLNode *tracedb_SLNodeFindNext ( const tracedb_SLNode *n, 
     bool ( * f ) ( const tracedb_SLNode *n ) );


/*--------------------------------------------------------------------------
 * Stack
 *  maintains a LIFO list
 */
typedef struct tracedb_Stack tracedb_Stack;
struct tracedb_Stack
{
    tracedb_SLNode *top;
};

/* StackInit
 *  initialize a stack object
 */
#define tracedb_StackInit( s ) \
    ( void ) ( ( s ) -> top = NULL )

/* StackTop
 *  returns stack top
 */
#define tracedb_StackTop( s ) \
    ( s ) -> top

/* StackPush
 *  push a single node onto stack
 */
#define tracedb_StackPush( s, n ) \
    ( void ) ( ( n ) -> next = ( s ) -> top, ( s ) -> top = ( n ) )

/* StackPop
 *  pop a single node from stack
 */
tracedb_SLNode *tracedb_StackPop ( tracedb_Stack *s );

/* StackForEach
 *  executes a function on each stack element
 */
void tracedb_StackForEach ( const tracedb_Stack *s,
    void ( * f ) ( tracedb_SLNode *n, void *data ), void *data );

/* StackDoUntil
 *  executes a function on each element
 *  until the function returns true
 */
void tracedb_StackDoUntil ( const tracedb_Stack *s,
    bool ( * f ) ( tracedb_SLNode *n, void *data ), void *data );

/* StackFindFirst
 *  find first element satisfying criteria
 */
tracedb_SLNode *tracedb_StackFindFirst ( const tracedb_Stack *s, 
     bool ( * f ) ( const tracedb_SLNode *n ) );

/* StackWhack
 *  pops elements from stack and
 *  executes a user provided destructor
 */
void tracedb_StackWhack ( tracedb_Stack *s, 
     void ( * whack ) ( tracedb_SLNode *n, void *data ), void *data );


/*--------------------------------------------------------------------------
 * tracedb_SLList
 *  singly linked list
 */
typedef struct tracedb_SLList tracedb_SLList;
struct tracedb_SLList
{
    tracedb_SLNode *head;
    tracedb_SLNode *tail;
};


/* tracedb_SLListInit
 *  initialize a singly linked list
 */
#define tracedb_SLListInit( sl ) \
    ( void ) ( ( sl ) -> head = ( sl ) -> tail = NULL )

/* tracedb_SLListHead
 *  returns list head
 */
#define tracedb_SLListHead( sl ) \
    ( sl ) -> head

/* tracedb_SLListTail
 *  returns list tail
 */
#define tracedb_SLListTail( sl ) \
    ( sl ) -> tail

/* tracedb_SLListPushHead
 *  push a single node onto head of list
 */
#define tracedb_SLListPushHead( sl, n ) \
    ( void ) ( ( ( sl ) -> tail == NULL ? \
        ( void ) ( ( sl ) -> tail = ( n ) ) : 0 ), \
      ( n ) -> next = ( sl ) -> head, ( sl ) -> head = ( n ) )

/* tracedb_SLListPushTail
 *  push a single node onto tail of list
 */
void tracedb_SLListPushTail ( tracedb_SLList *sl, tracedb_SLNode *n );

/* tracedb_SLListPopHead
 *  pop a single node from head of list
 */
tracedb_SLNode *tracedb_SLListPopHead ( tracedb_SLList *sl );

/* tracedb_SLListPopTail
 *  pop a single node from tail of list
 */
tracedb_SLNode *tracedb_SLListPopTail ( tracedb_SLList *sl );

/* tracedb_SLListUnlink
 *  removes a designated node from list
 */
void tracedb_SLListUnlink ( tracedb_SLList *sl, tracedb_SLNode *n );

/* tracedb_SLListForEach
 *  executes a function on each list element
 */
void tracedb_SLListForEach ( const tracedb_SLList *sl,
    void ( * f ) ( tracedb_SLNode *n, void *data ), void *data );

/* tracedb_SLListDoUntil
 *  executes a function on each element
 *  until the function returns true
 */
void tracedb_SLListDoUntil ( const tracedb_SLList *sl,
    bool ( * f ) ( tracedb_SLNode *n, void *data ), void *data );

/* tracedb_SLListFindFirst
 *  find first element satisfying criteria
 */
tracedb_SLNode *tracedb_SLListFindFirst ( const tracedb_SLList *sl,
    bool ( * f ) ( const tracedb_SLNode *n ) );

/* tracedb_SLListWhack
 *  pops elements from list and
 *  executes a user provided destructor
 */
void tracedb_SLListWhack ( tracedb_SLList *sl,
    void ( * whack ) ( tracedb_SLNode *n, void *data ), void *data );


/*--------------------------------------------------------------------------
 * DLNode
 *  doubly linked node
 */
typedef struct tracedb_DLNode tracedb_DLNode;
struct tracedb_DLNode
{
    tracedb_DLNode *next;
    tracedb_DLNode *prev;
};

/* DLNodeNext
 *  returns next node
 */
#define tracedb_DLNodeNext( n ) \
    ( n ) -> next

/* DLNodePrev
 *  returns prev node
 */
#define tracedb_DLNodePrev( n ) \
    ( n ) -> prev

/* DLNodeFindNext
 *  find next element satisfying criteria
 */
tracedb_DLNode *tracedb_DLNodeFindNext ( const tracedb_DLNode *n,
    bool ( * f ) ( const tracedb_DLNode *n ) );

/* DLNodeFindPrev
 *  find previous element satisfying criteria
 */
tracedb_DLNode *tracedb_DLNodeFindPrev ( const tracedb_DLNode *n,
    bool ( * f ) ( const tracedb_DLNode *n ) );


/*--------------------------------------------------------------------------
 * tracedb_DLList
 *  doubly linked list
 */
typedef struct tracedb_DLList tracedb_DLList;
struct tracedb_DLList
{
    tracedb_DLNode *head;
    tracedb_DLNode *tail;
};

/* tracedb_DLListInit
 *  initialize a doubly linked list
 */
#define tracedb_DLListInit( dl ) \
    ( void ) ( ( dl ) -> head = ( dl ) -> tail = NULL )

/* tracedb_DLListHead
 *  returns list head
 */
#define tracedb_DLListHead( dl ) \
    ( dl ) -> head

/* tracedb_DLListTail
 *  returns list tail
 */
#define tracedb_DLListTail( dl ) \
    ( dl ) -> tail

/* tracedb_DLListPushHead
 *  push a single node onto the head of list
 */
void tracedb_DLListPushHead ( tracedb_DLList *dl, tracedb_DLNode *n );

/* tracedb_DLListPushTail
 *  push a single node onto the tail of list
 */
void tracedb_DLListPushTail ( tracedb_DLList *dl, tracedb_DLNode *n );

/* tracedb_DLListPopHead
 *  pop a single node from head of list
 */
tracedb_DLNode *tracedb_DLListPopHead ( tracedb_DLList *dl );

/* tracedb_DLListPopTail
 *  pop a single node from tail of list
 */
tracedb_DLNode *tracedb_DLListPopTail ( tracedb_DLList *dl );

/* tracedb_DLListPrependList
 *  pushes list contents onto the head of target
 */
void tracedb_DLListPrependList ( tracedb_DLList *dl, tracedb_DLList *l );

/* tracedb_DLListAppendList
 *  pushes list contents onto the tail of target
 */
void tracedb_DLListAppendList ( tracedb_DLList *dl, tracedb_DLList *l );

/* tracedb_DLListInsertNodeBefore
 *  inserts node "n" before "which" within list
 */
void tracedb_DLListInsertNodeBefore ( tracedb_DLList *dl,
    tracedb_DLNode *which, tracedb_DLNode *n );

/* tracedb_DLListInsertNodeAfter
 *  inserts node "n" after "which" within list
 */
void tracedb_DLListInsertNodeAfter ( tracedb_DLList *dl, 
    tracedb_DLNode *which, tracedb_DLNode *n );

/* tracedb_DLListInsertListBefore
 *  inserts list "l" before "which" within list "dl"
 */
void tracedb_DLListInsertListBefore ( tracedb_DLList *dl,
    tracedb_DLNode *which, tracedb_DLList *l );

/* tracedb_DLListInsertListAfter
 *  inserts list "l" after "which" within list "dl"
 */
void tracedb_DLListInsertListAfter ( tracedb_DLList *dl,
    tracedb_DLNode *which, tracedb_DLList *l );

/* tracedb_DLListUnlink
 *  removes a designated node from list
 */
void tracedb_DLListUnlink ( tracedb_DLList *dl, tracedb_DLNode *n );

/* tracedb_DLListForEach
 *  executes a function on each list element
 */
void tracedb_DLListForEach ( const tracedb_DLList *dl, bool reverse,
    void ( * f ) ( tracedb_DLNode *n, void *data ), void *data );

/* tracedb_DLListDoUntil
 *  executes a function on each element
 *  until the function returns true
 */
void tracedb_DLListDoUntil ( const tracedb_DLList *dl, bool reverse,
    bool ( * f ) ( tracedb_DLNode *n, void *data ), void *data );

/* tracedb_DLListFindFirst
 *  find first element satisfying criteria
 */
tracedb_DLNode *tracedb_DLListFindFirst ( const tracedb_DLList *dl,
    bool ( * f ) ( const tracedb_DLNode *n ) );

/* tracedb_DLListFindLast
 *  find last element satisfying criteria
 */
tracedb_DLNode *tracedb_DLListFindLast ( const tracedb_DLList *dl,
    bool ( * f ) ( const tracedb_DLNode *n ) );

/* tracedb_DLListWhack
 *  pops elements from list and
 *  executes a user provided destructor
 */
void tracedb_DLListWhack ( tracedb_DLList *dl,
    void ( * whack ) ( tracedb_DLNode *n, void *data ), void *data );


/*--------------------------------------------------------------------------
 * BTNode
 *  b-tree node
 */
typedef struct tracedb_BTNode tracedb_BTNode;
struct tracedb_BTNode
{
    tracedb_BTNode *par;
    tracedb_BTNode *child [ 2 ];
};

/* BTNodeNext
 *  returns next node
 */
tracedb_BTNode *tracedb_BTNodeNext ( const tracedb_BTNode *n );

/* BTNodePrev
 *  returns prev node
 */
tracedb_BTNode *tracedb_BTNodePrev ( const tracedb_BTNode *n );

/* BTNodeParent
 *  returns a parent node if there, NULL otherwise
 */
tracedb_BTNode *tracedb_BTNodeParent ( const tracedb_BTNode *n );

/* BTNodeFindNext
 *  find next element satisfying criteria
 */
tracedb_BTNode *tracedb_BTNodeFindNext ( const tracedb_BTNode *n,
    bool ( * f ) ( const tracedb_BTNode *n ) );

/* BTNodeFindPrev
 *  find previous element satisfying criteria
 */
tracedb_BTNode *tracedb_BTNodeFindPrev ( const tracedb_BTNode *n,
    bool ( * f ) ( const tracedb_BTNode *n ) );


/*--------------------------------------------------------------------------
 * BTree
 *  b-tree
 */
typedef struct tracedb_BTree tracedb_BTree;
struct tracedb_BTree
{
    tracedb_BTNode *root;
};

/* BTreeInit
 *  initialize b-tree
 */
#define tracedb_BTreeInit( bt ) \
    ( void ) ( ( bt ) -> root = NULL )

/* BTreeDepth
 *  returns number of layers in b-tree
 *
 *  if "exact" is true, then the maximum
 *  depth is returned. otherwise, the depth of
 *  an arbitrary leaf node is returned
 */
unsigned int tracedb_BTreeDepth ( const tracedb_BTree *bt, bool exact );

/* BTreeFirst
 *  returns first node
 */
tracedb_BTNode *tracedb_BTreeFirst ( const tracedb_BTree *bt );

/* BTreeLast
 *  returns last node
 */
tracedb_BTNode *tracedb_BTreeLast ( const tracedb_BTree *bt );

/* BTreeFind
 *  find an object within tree
 *  "cmp" function returns equivalent of "item" - "n"
 */
tracedb_BTNode *tracedb_BTreeFind ( const tracedb_BTree *bt, const void *item,
    int ( * cmp ) ( const void *item, const tracedb_BTNode *n ) );

/* BTreeInsert
 *  insert an object within tree, even if duplicate
 *  "sort" function returns equivalent of "item" - "n"
 *
 *  the treatment of order for items reported as identical
 *  i.e. sort function returns zero when they are compared,
 *  is undefined.
 *
 *  the current implementation treats '<=' as '<' such
 *  that all inserts are converted to a '<' or '>' comparison,
 *  but this should not be relied upon.
 *
 *  returns 0 if insert succeeded or an OS error code otherwise.
 */
int tracedb_BTreeInsert ( tracedb_BTree *bt, tracedb_BTNode *item,
    int ( * sort ) ( const tracedb_BTNode *item, const tracedb_BTNode *n ) );

/* BTreeInsertUnique
 *  insert an object within tree, but only if unique.
 *  "sort" function returns equivalent of "item" - "n"
 *
 *  returns 0 if insertion succeeded. or an OS error code otherwise.
 *  if error code is EEXIST, the existing object is returned in "exist".
 */
int tracedb_BTreeInsertUnique ( tracedb_BTree *bt, 
    tracedb_BTNode *item, tracedb_BTNode **exist,
    int ( * sort ) ( const tracedb_BTNode *item, const tracedb_BTNode *n ) );

/* BTreeResort
 *  an optimized removal and re-insertion of
 *  all contained elements using another function
 *
 *  the treatment of order for items reported as identical
 *  i.e. sort function returns zero when they are compared,
 *  is undefined.
 *
 *  the current implementation treats '<=' as '<' such
 *  that all inserts are converted to a '<' or '>' comparison,
 *  but this should not be relied upon.
 */
void tracedb_BTreeResort ( tracedb_BTree *bt,
    int ( * resort ) ( const tracedb_BTNode *item, const tracedb_BTNode *n ) );

/* BTreeUnlink
 *  removes a node from tree
 *
 *  returns true if node was removed from tree
 *  false if it could not be removed, e.g. was not in tree
 */
bool tracedb_BTreeUnlink ( tracedb_BTree *bt, tracedb_BTNode *n );

/* BTreeForEach
 *  executes a function on each tree element
 */
void tracedb_BTreeForEach ( const tracedb_BTree *bt, bool reverse,
    void ( * f ) ( tracedb_BTNode *n, void *data ), void *data );

/* BTreeDoUntil
 *  executes a function on each element
 *  until the function returns true
 *
 *  return values:
 *    false unless the function returns true
 */
bool tracedb_BTreeDoUntil ( const tracedb_BTree *bt, bool reverse,
    bool ( * f ) ( tracedb_BTNode *n, void *data ), void *data );

/* BTreeWhack
 *  removes nodes from tree and
 *  executes a user provided destructor
 */
void tracedb_BTreeWhack ( tracedb_BTree *bt,
    void ( * whack ) ( tracedb_BTNode *n, void *data ), void *data );


/*--------------------------------------------------------------------------
 * SparseArray
 *  sparse array, supporting arbitrary signed integer addressing
 */
typedef struct tracedb_SparseArray tracedb_SparseArray;
struct tracedb_SparseArray
{
    tracedb_BTree bt;
    unsigned int block_len;
};

/* SparseArrayInit
 *  initialize b-tree
 */
void tracedb_SparseArrayInit ( tracedb_SparseArray *sa, unsigned int block_len );

/* SparseArrayGet
 *  returns an indexed element
 */
void *tracedb_SparseArrayGet ( const tracedb_SparseArray *sa, int i );

/* SparseArraySet
 *  sets an indexed element
 *  returns 0 on success, error code ( ENOMEM ) otherwise
 */
int tracedb_SparseArraySet ( tracedb_SparseArray *sa, int i, void *item );

/* SparseArrayUnset
 *  clears an indexed slot
 */
void tracedb_SparseArrayUnset ( tracedb_SparseArray *sa, int i,
    void ( * whack ) ( void *item, int i, void *data ), void *data );

/* SparseArrayForEach
 *  executes a function on each non-NULL element
 */
void tracedb_SparseArrayForEach ( const tracedb_SparseArray *sa, bool reverse,
    void ( * f ) ( void *item, int i, void *data ), void *data );

/* SparseArrayDoUntil
 *  executes a function on each non-NULL element
 *  until the function returns true
 */
void tracedb_SparseArrayDoUntil ( const tracedb_SparseArray *sa, bool reverse,
    bool ( * f ) ( void *item, int i, void *data ), void *data );

/* SparseArrayWhack
 *  executes a user provided destructor
 */
void tracedb_SparseArrayWhack ( tracedb_SparseArray *sa,
    void ( * whack ) ( void *item, int i, void *data ), void *data );


#ifdef __cplusplus
}
#endif

#endif /* _h_container_ */
