#include "container.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

/*--------------------------------------------------------------------------
 * SLNode
 *  singly linked node
 */

/* SLNodeFindNext
 *  find next element satisfying criteria
 */
tracedb_SLNode *tracedb_SLNodeFindNext ( const tracedb_SLNode *p, 
                         bool ( * f ) ( const tracedb_SLNode *n ) )
{
    if ( p != NULL )
    {
        tracedb_SLNode *n = p -> next;
        while ( n != NULL )
        {
            if ( ( * f ) ( n ) )
                return n;
            n = n -> next;
        }
    }
    return NULL;
}


/*--------------------------------------------------------------------------
 * Stack
 *  maintains a LIFO list
 */

/* StackPop
 *  pop a single node from stack
 */
tracedb_SLNode *tracedb_StackPop ( tracedb_Stack *s )
{
    if ( s != NULL )
    {
        tracedb_SLNode *n = s -> top;
        if ( n != NULL )
            s -> top = n -> next;
        return n;
    }
    return NULL;
}

/* StackForEach
 *  executes a function on each stack element
 */
void tracedb_StackForEach ( const tracedb_Stack *s,
    void ( * f ) ( tracedb_SLNode *n, void *data ), void *data )
{
    if ( s != NULL )
    {
        tracedb_SLNode *n = s -> top;
        while ( n != NULL )
        {
            tracedb_SLNode *next = n -> next;
            ( * f ) ( n, data );
            n = next;
        }
    }
}

/* StackDoUntil
 *  executes a function on each element
 *  until the function returns true
 */
void tracedb_StackDoUntil ( const tracedb_Stack *s,
    bool ( * f ) ( tracedb_SLNode *n, void *data ), void *data )
{
    if ( s != NULL )
    {
        tracedb_SLNode *n = s -> top;
        while ( n != NULL )
        {
            tracedb_SLNode *next = n -> next;
            if ( ( * f ) ( n, data ) )
                break;
            n = next;
        }
    }
}

/* StackFindFirst
 *  find first element satisfying criteria
 */
tracedb_SLNode *tracedb_StackFindFirst ( const tracedb_Stack *s,
     bool ( * f ) ( const tracedb_SLNode *n ) )
{
    if ( s != NULL )
    {
        tracedb_SLNode *n = s -> top;
        while ( n != NULL )
        {
            tracedb_SLNode *next = n -> next;
            if ( ( * f ) ( n ) )
                return n;
            n = next;
        }
    }
    return NULL;
}

/* StackWhack
 *  pops elements from stack and
 *  executes a user provided destructor
 */
void tracedb_StackWhack ( tracedb_Stack *s, 
     void ( * whack ) ( tracedb_SLNode *n, void *data ), void *data )
{
    if ( s != NULL )
    {
        tracedb_SLNode *n = s -> top;
        s -> top = NULL;

        if ( whack != NULL )
        {
            while ( n != NULL )
            {
                tracedb_SLNode *next = n -> next;
                ( * whack ) ( n, data );
                n = next;
            }
        }
    }
}


/*--------------------------------------------------------------------------
 * tracedb_SLList
 *  singly linked list
 */

/* tracedb_SLListPushTail
 *  push a single node onto tail of list
 */
void tracedb_SLListPushTail ( tracedb_SLList *sl, tracedb_SLNode *n )
{
    if ( sl != NULL && n != NULL )
    {
        if ( sl -> tail == NULL )
            sl -> head = sl -> tail = n;
        else
        {
            sl -> tail -> next = n;
            sl -> tail = n;
        }
        n -> next = NULL;
    }
}

/* tracedb_SLListPopHead
 *  pop a single node from head of list
 */
tracedb_SLNode *tracedb_SLListPopHead ( tracedb_SLList *sl )
{
    if ( sl != NULL )
    {
        tracedb_SLNode *n = sl -> head;
        if ( n != NULL )
        {
            sl -> head = n -> next;
            if ( n -> next == NULL )
                sl -> tail = NULL;
        }
        return n;
    }
    return NULL;
}

/* tracedb_SLListPopTail
 *  pop a single node from tail of list
 */
tracedb_SLNode *tracedb_SLListPopTail ( tracedb_SLList *sl )
{
    if ( sl != NULL )
    {
        tracedb_SLNode *n = sl -> head;
        if ( n != NULL )
        {
            tracedb_SLNode *tail = sl -> tail;
            if ( n == tail )
            {
                sl -> head = sl -> tail = NULL;
                return n;
            }
            while ( n -> next != tail )
                n = n -> next;
            sl -> tail = n;
            n -> next = NULL;
            return tail;
        }
    }
    return NULL;
}

/* tracedb_SLListUnlink
 *  removes a designated node from list
 */
void tracedb_SLListUnlink ( tracedb_SLList *sl, tracedb_SLNode *n )
{
    if ( sl != NULL && n != NULL )
    {
        tracedb_SLNode *p = sl -> head;
        if ( p == n )
        {
            sl -> head = p -> next;
            if ( p -> next == NULL )
                sl -> tail = NULL;
        }
        else while ( p != NULL )
        {
            if ( p -> next == n )
            {
                p -> next = n -> next;
                if ( n -> next == NULL )
                    sl -> tail = p;
                break;
            }
            p = p -> next;
        }
    }
}

/* tracedb_SLListForEach
 *  executes a function on each list element
 */
void tracedb_SLListForEach ( const tracedb_SLList *sl,
    void ( * f ) ( tracedb_SLNode *n, void *data ), void *data )
{
    if ( sl != NULL )
    {
        tracedb_SLNode *n = sl -> head;
        while ( n != NULL )
        {
            tracedb_SLNode *next = n -> next;
            ( * f ) ( n, data );
            n = next;
        }
    }
}

/* tracedb_SLListDoUntil
 *  executes a function on each element
 *  until the function returns true
 */
void tracedb_SLListDoUntil ( const tracedb_SLList *sl,
    bool ( * f ) ( tracedb_SLNode *n, void *data ), void *data )
{
    if ( sl )
    {
        tracedb_SLNode *n = sl -> head;
        while ( n != NULL )
        {
            tracedb_SLNode *next = n -> next;
            if ( ( * f ) ( n, data ) )
                break;
            n = next;
        }
    }
}

/* tracedb_SLListFindFirst
 *  find first element satisfying criteria
 */
tracedb_SLNode *tracedb_SLListFindFirst ( const tracedb_SLList *sl,
     bool ( * f ) ( const tracedb_SLNode *n ) )
{
    if ( sl != NULL )
    {
        tracedb_SLNode *n = sl -> head;
        while ( n != NULL )
        {
            tracedb_SLNode *next = n -> next;
            if ( ( * f ) ( n ) )
                return n;
            n = next;
        }
    }
    return NULL;
}

/* tracedb_SLListWhack
 *  pops elements from list and
 *  executes a user provided destructor
 */
void tracedb_SLListWhack ( tracedb_SLList *sl,
   void ( * whack ) ( tracedb_SLNode *n, void *data ), void *data )
{
    if ( sl != NULL )
    {
        tracedb_SLNode *n = sl -> head;
        sl -> head = sl -> tail = NULL;

        if ( whack != NULL )
        {
            while ( n != NULL )
            {
                tracedb_SLNode *next = n -> next;
                ( * whack ) ( n, data );
                n = next;
            }
        }
    }
}


/*--------------------------------------------------------------------------
 * DLNode
 *  doubly linked node
 */

/* DLNodeFindNext
 *  find next element satisfying criteria
 */
tracedb_DLNode *DLNodeFindNext ( const tracedb_DLNode *p,
    bool ( * f ) ( const tracedb_DLNode *n ) )
{
    if ( p != NULL )
    {
        tracedb_DLNode *n = p -> next;
        while ( n != NULL )
        {
            if ( ( * f ) ( n ) )
                return n;
            n = n -> next;
        }
    }
    return NULL;
}

/* DLNodeFindPrev
 *  find previous element satisfying criteria
 */
tracedb_DLNode *DLNodeFindPrev ( const tracedb_DLNode *p,
    bool ( * f ) ( const tracedb_DLNode *n ) )
{
    if ( p != NULL )
    {
        tracedb_DLNode *n = p -> prev;
        while ( n != NULL )
        {
            if ( ( * f ) ( n ) )
                return n;
            n = n -> prev;
        }
    }
    return NULL;
}


/*--------------------------------------------------------------------------
 * tracedb_DLList
 *  doubly linked list
 */

/* tracedb_DLListPushHead
 *  push a single node onto the head of list
 */
void tracedb_DLListPushHead ( tracedb_DLList *dl, tracedb_DLNode *n )
{
    if ( dl != NULL && n != NULL )
    {
        n -> prev = NULL;
        n -> next = dl -> head;
        if ( dl -> head == NULL )
            dl -> head = dl -> tail = n;
        else
        {
            dl -> head -> prev = n;
            dl -> head = n;
        }
    }
}

/* tracedb_DLListPushTail
 *  push a single node onto the tail of list
 */
void tracedb_DLListPushTail ( tracedb_DLList *dl, tracedb_DLNode *n )
{
    if ( dl != NULL && n != NULL )
    {
        n -> next = NULL;
        n -> prev = dl -> tail;
        if ( dl -> tail == NULL )
            dl -> tail = dl -> head = n;
        else
        {
            dl -> tail -> next = n;
            dl -> tail = n;
        }
    }
}

/* tracedb_DLListPopHead
 *  pop a single node from head of list
 */
tracedb_DLNode *tracedb_DLListPopHead ( tracedb_DLList *dl )
{
    if ( dl != NULL )
    {
        tracedb_DLNode *n = dl -> head;
        if ( dl -> head != NULL )
        {
            dl -> head = n -> next;
            if ( n -> next == NULL )
                dl -> tail = NULL;
            else
                n -> next -> prev = NULL;
        }
        return n;
    }
    return NULL;
}

/* tracedb_DLListPopTail
 *  pop a single node from tail of list
 */
tracedb_DLNode *tracedb_DLListPopTail ( tracedb_DLList *dl )
{
    if ( dl != NULL )
    {
        tracedb_DLNode *n = dl -> tail;
        if ( dl -> tail != NULL )
        {
            dl -> tail = n -> prev;
            if ( n -> prev == NULL )
                dl -> head = NULL;
            else
                n -> prev -> next = NULL;
        }
        return n;
    }
    return NULL;
}

/* tracedb_DLListPrependList
 *  pushes list contents onto the head of target
 */
void tracedb_DLListPrependList ( tracedb_DLList *dl, tracedb_DLList *l )
{
    if ( dl != NULL && l != NULL && l -> head != NULL )
    {
        if ( dl -> tail == NULL )
            * dl = * l;
        else
        {
            dl -> head -> prev = l -> tail;
            l -> tail -> next = dl -> head;
            dl -> head = l -> head;
        }

        l -> head = l -> tail = NULL;
    }
}

/* tracedb_DLListAppendList
 *  pushes list contents onto the tail of target
 */
void tracedb_DLListAppendList ( tracedb_DLList *dl, tracedb_DLList *l )
{
    if ( dl != NULL && l != NULL && l -> head != NULL )
    {
        if ( dl -> tail == NULL )
            * dl = * l;
        else
        {
            dl -> tail -> next = l -> head;
            l -> head -> prev = dl -> tail;
            dl -> tail = l -> tail;
        }

        l -> head = l -> tail = NULL;
    }
}

/* tracedb_DLListInsertNodeBefore
 *  inserts node "n" before "which" within list
 */
void tracedb_DLListInsertNodeBefore ( tracedb_DLList *dl,
    tracedb_DLNode *which, tracedb_DLNode *n )
{
    if ( which != NULL && n != NULL )
    {
        /* take care of "n" */
        n -> next = which;
        n -> prev = which -> prev;

        /* link "which"'s prev to "n" */
        if ( which -> prev != NULL )
            which -> prev -> next = n;

        /* or if none, then perhaps head of list */
        else if ( dl != NULL && dl -> head == which )
            dl -> head = n;

        /* link "which" to "n" */
        which -> prev = n;
    }
}

/* tracedb_DLListInsertNodeAfter
 *  inserts node "n" after "which" within list
 */
void tracedb_DLListInsertNodeAfter ( tracedb_DLList *dl,
    tracedb_DLNode *which, tracedb_DLNode *n )
{
    if ( which != NULL && n != NULL )
    {
        /* take care of "n" */
        n -> prev = which;
        n -> next = which -> next;

        /* link "which"'s next to "n" */
        if ( which -> next != NULL )
            which -> next -> prev = n;

        /* or if none, then perhaps tail of list */
        else if ( dl != NULL && dl -> tail == which )
            dl -> tail = n;

        /* link "which" to "n" */
        which -> next = n;
    }
}

/* tracedb_DLListInsertListBefore
 *  inserts list "l" before "which" within list "dl"
 */
void tracedb_DLListInsertListBefore ( tracedb_DLList *dl,
    tracedb_DLNode *which, tracedb_DLList *l )
{
    if ( which != NULL && l != NULL && l -> head != NULL )
    {
        /* take care of inserting list */
        l -> tail -> next = which;
        l -> head -> prev = which -> prev;

        /* link "which"'s prev to "l -> head" */
        if ( which -> prev != NULL )
            which -> prev -> next = l -> head;

        /* or if none, then perhaps head of list */
        else if ( dl != NULL && dl -> head == which )
            dl -> head = l -> head;

        /* link "which" to "l -> tail" */
        which -> prev = l -> tail;

        /* remove items from "l" */
        l -> head = l -> tail = NULL;
    }
}

/* tracedb_DLListInsertListAfter
 *  inserts list "l" after "which" within list "dl"
 */
void tracedb_DLListInsertListAfter ( tracedb_DLList *dl,
    tracedb_DLNode *which, tracedb_DLList *l )
{
    if ( which != NULL && l != NULL && l -> head != NULL )
    {
        /* take care of inserting list */
        l -> head -> prev = which;
        l -> tail -> next = which -> next;

        /* link "which"'s next to "l -> tail" */
        if ( which -> next != NULL )
            which -> next -> prev = l -> tail;

        /* or if none, then perhaps tail of list */
        else if ( dl != NULL && dl -> tail == which )
            dl -> head = l -> tail;

        /* link "which" to "l -> head" */
        which -> next = l -> head;

        /* remove items from "l" */
        l -> head = l -> tail = NULL;
    }
}

/* tracedb_DLListUnlink
 *  removes a designated node from list
 */
void tracedb_DLListUnlink ( tracedb_DLList *dl, tracedb_DLNode *n )
{
    if ( n != NULL )
    {
        if ( n -> next == NULL )
        {
            if ( dl != NULL && dl -> tail == n )
            {
                if ( n -> prev == NULL )
                    dl -> head = dl -> tail = NULL;
                else
                {
                    n -> prev -> next = NULL;
                    dl -> tail = n -> prev;
                }
            }
            else
            {
                if ( n -> prev != NULL )
                    n -> prev -> next = NULL;
            }
        }
        else if ( n -> prev == NULL )
        {
            n -> next -> prev = NULL;
            if ( dl != NULL && dl -> head == n )
                dl -> head = n -> next;
        }
        else
        {
            n -> next -> prev = n -> prev;
            n -> prev -> next = n -> next;
        }
    }
}

/* tracedb_DLListForEach
 *  executes a function on each list element
 */
void tracedb_DLListForEach ( const tracedb_DLList *dl, bool reverse,
    void ( * f ) ( tracedb_DLNode *n, void *data ), void *data )
{
    if ( dl != NULL )
    {
        tracedb_DLNode *n, *next;
        if ( reverse )
        {
            n = dl -> tail;
            while ( n != NULL )
            {
                next = n -> prev;
                ( * f ) ( n, data );
                n = next;
            }
        }
        else
        {
            n = dl -> head;
            while ( n != NULL )
            {
                next = n -> next;
                ( * f ) ( n, data );
                n = next;
            }
        }
    }
}

/* tracedb_DLListDoUntil
 *  executes a function on each element
 *  until the function returns true
 */
void tracedb_DLListDoUntil ( const tracedb_DLList *dl, bool reverse,
    bool ( * f ) ( tracedb_DLNode *n, void *data ), void *data )
{
    if ( dl != NULL )
    {
        tracedb_DLNode *n, *next;
        if ( reverse )
        {
            n = dl -> tail;
            while ( n != NULL )
            {
                next = n -> prev;
                if ( ( * f ) ( n, data ) )
                    break;
                n = next;
            }
        }
        else
        {
            n = dl -> head;
            while ( n != NULL )
            {
                next = n -> next;
                if ( ( * f ) ( n, data ) )
                    break;
                n = next;
            }
        }
    }
}

/* tracedb_DLListFindFirst
 *  find first element satisfying criteria
 */
tracedb_DLNode *tracedb_DLListFindFirst ( const tracedb_DLList *dl,
    bool ( * f ) ( const tracedb_DLNode *n ) )
{
    if ( dl != NULL )
    {
        tracedb_DLNode *n = dl -> head;
        while ( n != NULL )
        {
            if ( ( * f ) ( n ) )
                return n;
            n = n -> next;
        }
    }
    return NULL;
}

/* tracedb_DLListFindLast
 *  find last element satisfying criteria
 */
tracedb_DLNode *tracedb_DLListFindLast ( const tracedb_DLList *dl,
    bool ( * f ) ( const tracedb_DLNode *n ) )
{
    if ( dl != NULL )
    {
        tracedb_DLNode *n = dl -> tail;
        while ( n != NULL )
        {
            if ( ( * f ) ( n ) )
                return n;
            n = n -> prev;
        }
    }
    return NULL;
}

/* tracedb_DLListWhack
 *  pops elements from list and
 *  executes a user provided destructor
 */
void tracedb_DLListWhack ( tracedb_DLList *dl,
    void ( * whack ) ( tracedb_DLNode *n, void *data ), void *data )
{
    if ( dl != NULL )
    {
        tracedb_DLNode *n = dl -> head;
        dl -> head = dl -> tail = NULL;

        if ( whack != NULL )
        {
            while ( n != NULL )
            {
                tracedb_DLNode *next = n -> next;
                ( * whack ) ( n, data );
                n = next;
            }
        }
    }
}


/*--------------------------------------------------------------------------
 * BTNode
 *  b-tree node
 */

#define TRACEDB_LEFT 1
#define TRACEDB_RIGHT 2

#define TRACEDB_BALANCE( node ) \
    ( ( size_t ) ( node ) -> par & 3 )
#define TRACEDB_ZERO_BALANCE( node ) \
    ( * ( size_t* ) & ( node ) -> par &= ~ ( size_t ) 3 )
#define TRACEDB_CLR_BALANCE( node, bal ) \
    ( * ( size_t* ) & ( node ) -> par ^= ( bal ) )
#define TRACEDB_SET_BALANCE( node, bal ) \
    ( * ( size_t* ) & ( node ) -> par |= ( bal ) )
#define TRACEDB_LEFT_HEAVY( node ) \
    ( ( ( size_t ) ( node ) -> par & TRACEDB_LEFT ) != 0 )
#define TRACEDB_RIGHT_HEAVY( node ) \
    ( ( ( size_t ) ( node ) -> par & TRACEDB_RIGHT ) != 0 )

#define TRACEDB_PMASK 3
#define TRACEDB_BBITS( node, bal ) ( bal )

#define TRACEDB_PBITS( node ) \
    ( ( size_t ) ( node ) -> par & TRACEDB_PMASK )
#define TRACEDB_PARENT( node ) \
    ( tracedb_BTNode* ) ( ( size_t ) ( node ) -> par & ~ ( size_t ) TRACEDB_PMASK )
#define TRACEDB_SET_PARENT( node, p ) \
    ( ( node ) -> par = ( tracedb_BTNode* ) ( ( size_t ) ( p ) | TRACEDB_PBITS ( node ) ) )
#define TRACEDB_SET_PARBAL( node, p, bal ) \
    ( ( node ) -> par = ( tracedb_BTNode* ) ( ( size_t ) ( p ) | TRACEDB_BBITS ( node, bal ) ) )


/* LeftMost
 *  returns the left-most child
 */
static
tracedb_BTNode *tracedb_LeftMost ( tracedb_BTNode *q )
{
    if ( q != NULL )
    {
        tracedb_BTNode *p = q -> child [ 0 ];
        while ( p != NULL )
        {
            q = p;
            p = p -> child [ 0 ];
        }
    }
    return q;
}

/* RightMost
 *  returns the right-most child
 */
static
tracedb_BTNode *tracedb_RightMost ( tracedb_BTNode *q )
{
    if ( q != NULL )
    {
        tracedb_BTNode *p = q -> child [ 1 ];
        while ( p != NULL )
        {
            q = p;
            p = p -> child [ 1 ];
        }
    }
    return q;
}

/* FirstNode
 *  the left-most node in tree
 */
#define tracedb_FirstNode( bt ) \
    tracedb_LeftMost ( ( bt ) -> root )

/* LastNode
 *  the right-most node in tree
 */
#define tracedb_LastNode( bt ) \
    tracedb_RightMost ( ( bt ) -> root )

/* BTNodeNext
 *  returns next node
 */
tracedb_BTNode *tracedb_BTNodeNext ( const tracedb_BTNode *n )
{
    tracedb_BTNode *p = n -> child [ 1 ];
    if ( p == 0 )
    {
        tracedb_BTNode *q = ( tracedb_BTNode* ) n;
        while ( 1 )
        {
            p = TRACEDB_PARENT ( q );
            if ( p == NULL )
                return NULL;
            if ( p -> child [ 0 ] == q )
                return p;
            q = p;
        }
    }
    return tracedb_LeftMost ( p );
}

/* BTNodePrev
 *  returns prev node
 */
tracedb_BTNode *tracedb_BTNodePrev ( const tracedb_BTNode *n )
{
    tracedb_BTNode *p = n -> child [ 0 ];
    if ( p == 0 )
    {
        tracedb_BTNode *q = ( tracedb_BTNode* ) n;
        while ( 1 )
        {
            p = TRACEDB_PARENT ( q );
            if ( p == NULL )
                return NULL;
            if ( p -> child [ 1 ] == q )
                return p;
            q = p;
        }
    }
    return tracedb_RightMost ( p );
}

/* BTNodeParent
 *  returns a parent node if there, NULL otherwise
 */
tracedb_BTNode *tracedb_BTNodeParent ( const tracedb_BTNode *n )
{
    if ( n != NULL )
        return TRACEDB_PARENT ( n );
    return NULL;
}

/* BTNodeFindNext
 *  find next element satisfying criteria
 */
tracedb_BTNode *tracedb_BTNodeFindNext ( const tracedb_BTNode *p,
    bool ( * f ) ( const tracedb_BTNode *n ) )
{
    if ( p != NULL )
    {
        tracedb_BTNode *n = tracedb_BTNodeNext ( p );
        while ( n != NULL )
        {
            if ( ( * f ) ( n ) )
                return n;
            n = tracedb_BTNodeNext ( n );
        }
    }
    return NULL;
}

/* BTNodeFindPrev
 *  find previous element satisfying criteria
 */
tracedb_BTNode *tracedb_BTNodeFindPrev ( const tracedb_BTNode *p,
    bool ( * f ) ( const tracedb_BTNode *n ) )
{
    if ( p != NULL )
    {
        tracedb_BTNode *n = tracedb_BTNodePrev ( p );
        while ( n != NULL )
        {
            if ( ( * f ) ( n ) )
                return n;
            n = tracedb_BTNodePrev ( n );
        }
    }
    return NULL;
}


/*--------------------------------------------------------------------------
 * BTree
 *  b-tree
 */

/* BTreeDepth
 *  returns number of layers in b-tree
 *
 *  if "exact" is true, then the maximum
 *  depth is returned. otherwise, the depth of
 *  an arbitrary leaf node is returned
 */
unsigned int tracedb_BTreeDepth ( const tracedb_BTree *bt, bool exact )
{
    tracedb_BTNode *p;
    unsigned int depth;

    if ( bt == NULL || bt -> root == NULL )
        return 0;

    depth = 1;

    if ( exact )
    {
        for ( p = tracedb_FirstNode ( bt ); p != NULL; p = tracedb_BTNodeNext ( p ) )
        {
            tracedb_BTNode *q;
            unsigned int ndepth;

            if ( p -> child [ 0 ] != NULL || p -> child [ 1 ] != NULL )
                continue;

            for ( ndepth = 1, q = TRACEDB_PARENT ( p ); q != NULL; q = TRACEDB_PARENT ( q ) )
                ++ ndepth;

            if ( ndepth > depth )
                depth = ndepth;
        }
    }
    else
    {
        for ( p = bt -> root;; ++ depth )
        {
            if ( p -> child [ 0 ] != NULL )
                p = p -> child [ 0 ];
            else if ( p -> child [ 1 ] != NULL )
                p = p -> child [ 1 ];
            else
                break;
        }
    }

    return depth;
}

/* BTreeFirst
 *  returns first node
 */
tracedb_BTNode *tracedb_BTreeFirst ( const tracedb_BTree *bt )
{
    if ( bt == NULL )
        return NULL;
    return tracedb_FirstNode ( bt );
}

/* BTreeLast
 *  returns last node
 */
tracedb_BTNode *tracedb_BTreeLast ( const tracedb_BTree *bt )
{
    if ( bt == NULL )
        return NULL;
    return tracedb_LastNode ( bt );
}

/* BTreeFind
 *  find an object within tree
 *  "cmp" function returns equivalent of "item" - "n"
 */
tracedb_BTNode *tracedb_BTreeFind ( const tracedb_BTree *bt,
    const void *item,
    int ( * cmp ) ( const void *item, const tracedb_BTNode *n ) )
{
    if ( bt != NULL )
    {
        tracedb_BTNode *n = bt -> root;
        while ( n != NULL )
        {
            int diff = ( * cmp ) ( item, n );
            if ( diff == 0 )
                return n;
            n = n -> child [ diff > 0 ];
        }
    }
    return NULL;
}

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
 */
static
tracedb_BTNode *tracedb_RotateRightAtY ( tracedb_BTNode *y, tracedb_BTNode *x )
{
    tracedb_BTNode *w = x;
    tracedb_BTNode *z = x -> child [ 1 ];
    y -> child [ 0 ] = z;
    x -> child [ 1 ] = y;
    x -> par = TRACEDB_PARENT ( y );
    y -> par = x;

    /* patch parent link */
    if ( z != 0 )
        TRACEDB_SET_PARENT ( z, y );

    return w;
}

static
tracedb_BTNode *tracedb_RotateLeftAtY ( tracedb_BTNode *y, tracedb_BTNode *x )
{
    tracedb_BTNode *w = x;
    tracedb_BTNode *z = x -> child [ 0 ];
    y -> child [ 1 ] = z;
    x -> child [ 0 ] = y;
    x -> par = TRACEDB_PARENT ( y );
    y -> par = x;

    /* patch parent link */
    if ( z != 0 )
        TRACEDB_SET_PARENT ( z, y );

    return w;
}

static
tracedb_BTNode *tracedb_RotateLeftAtXRightAtY ( tracedb_BTNode *y, tracedb_BTNode *x )
{
    tracedb_BTNode *w = x -> child [ 1 ];
    tracedb_BTNode *z = w -> child [ 0 ];
    x -> child [ 1 ] = z;
    if ( z != 0 )
        TRACEDB_SET_PARENT ( z, x );
    z = w -> child [ 1 ];
    w -> child [ 0 ] = x;
    y -> child [ 0 ] = z;
    w -> child [ 1 ] = y;
        
    switch ( TRACEDB_BALANCE ( w ) )
    {
    case 0:
        w -> par = TRACEDB_PARENT ( y );
        x -> par = w;
        y -> par = w;
        break;
    case TRACEDB_LEFT:
        w -> par = TRACEDB_PARENT ( y );
        x -> par = w;
        TRACEDB_SET_PARBAL ( y, w, TRACEDB_RIGHT );
        break;
    case TRACEDB_RIGHT:
        w -> par = TRACEDB_PARENT ( y );
        TRACEDB_SET_PARBAL ( x, w, TRACEDB_LEFT );
        y -> par = w;
        break;
    }

    /* patch parent link */
    if ( z != 0 )
        TRACEDB_SET_PARENT ( z, y );

    return w;
}

static
tracedb_BTNode *tracedb_RotateRightAtXLeftAtY ( tracedb_BTNode *y, tracedb_BTNode *x )
{
    tracedb_BTNode *w = x -> child [ 0 ];
    tracedb_BTNode *z = w -> child [ 1 ];
    x -> child [ 0 ] = z;
    if ( z != 0 )
        TRACEDB_SET_PARENT ( z, x );
    z = w -> child [ 0 ];
    w -> child [ 1 ] = x;
    y -> child [ 1 ] = z;
    w -> child [ 0 ] = y;
        
    switch ( TRACEDB_BALANCE ( w ) )
    {
    case 0:
        w -> par = TRACEDB_PARENT ( y );
        x -> par = w;
        y -> par = w;
        break;
    case TRACEDB_LEFT:
        w -> par = TRACEDB_PARENT ( y );
        TRACEDB_SET_PARBAL ( x, w, TRACEDB_RIGHT );
        y -> par = w;
        break;
    case TRACEDB_RIGHT:
        w -> par = TRACEDB_PARENT ( y );
        x -> par = w;
        TRACEDB_SET_PARBAL ( y, w, TRACEDB_LEFT );
        break;
    }

    /* patch parent link */
    if ( z != 0 )
        TRACEDB_SET_PARENT ( z, y );

    return w;
}

static
tracedb_BTNode *tracedb_RebalanceLeft ( tracedb_BTNode *y, tracedb_BTNode *x )
{
    /* detect child balance */
    if ( TRACEDB_LEFT_HEAVY ( x ) )
        return tracedb_RotateRightAtY ( y, x );

    /* child is right heavy */
    return tracedb_RotateLeftAtXRightAtY ( y, x );
}

static
tracedb_BTNode *tracedb_RebalanceRight ( tracedb_BTNode *y, tracedb_BTNode *x )
{
    /* detect child balance */
    if ( TRACEDB_RIGHT_HEAVY ( x ) )
        return tracedb_RotateLeftAtY ( y, x );

    /* left heavy */
    return tracedb_RotateRightAtXLeftAtY ( y, x );
}


static
void tracedb_RebalanceAfterInsert ( tracedb_BTNode **root,
    tracedb_BTNode *y, tracedb_BTNode *x )
{
    tracedb_BTNode *w, *z;

    /* detect left insertion */
    if ( y -> child [ 0 ] == x )
    {
        /* if y was right-heavy, done */
        if ( TRACEDB_RIGHT_HEAVY ( y ) )
        {
            TRACEDB_CLR_BALANCE ( y, TRACEDB_RIGHT );
            return;
        }

        /* rebalance left insertion */
        w = tracedb_RebalanceLeft ( y, x );
    }

    /* right insertion */
    else
    {
        /* if y was left-heavy, done */
        if ( TRACEDB_LEFT_HEAVY ( y ) )
        {
            TRACEDB_CLR_BALANCE ( y, TRACEDB_LEFT );
            return;
        }

        /* rebalance right insertion */
        w = tracedb_RebalanceRight ( y, x );
    }

    /* fix parent to child */
    assert ( TRACEDB_BALANCE ( w ) == 0 );
    z = w -> par;
    if ( z == 0 )
        * root = w;
    else
        z -> child [ z -> child [ 1 ] == y ] = w;
}

int tracedb_BTreeInsert ( tracedb_BTree *bt, tracedb_BTNode *n,
    int ( * sort ) ( const tracedb_BTNode *n, const tracedb_BTNode *p ) )
{
    if ( bt != NULL && n != NULL )
    {
        int diff;

        tracedb_BTNode *p = bt -> root;
        tracedb_BTNode *q = NULL;
        tracedb_BTNode *y = NULL;

        while ( p != NULL )
        {
            diff = ( * sort ) ( n, p );
            q = p;
            if ( TRACEDB_BALANCE ( p ) != 0 )
                y = p;
            p = p -> child [ diff > 0 ];
        }

        n -> par = q;
        n -> child [ 0 ] = n -> child [ 1 ] = NULL;

        if ( q == NULL )
            bt -> root = n;
        else
        {
            q -> child [ diff > 0 ] = n;

            /* run a trace-back */
            for ( p = n; q != y; )
            {
                /* this is safe because q has 0 balance */
                tracedb_BTNode *z = q -> par;
                if ( q -> child [ 0 ] == p )
                    TRACEDB_SET_BALANCE ( q, TRACEDB_LEFT );
                else
                    TRACEDB_SET_BALANCE ( q, TRACEDB_RIGHT );

                p = q;
                q = z;
            }

            /* rebalance */
            if ( q != NULL )
                tracedb_RebalanceAfterInsert ( & bt -> root, q, p );
        }
    }

    /* never fails in this implementation */
    return 0;
}

/* BTreeInsertUnique
 *  insert an object within tree, but only if unique
 *  "sort" function returns equivalent of "item" - "n"
 *  returns non-NULL "n" upon match or NULL on success
 */
int tracedb_BTreeInsertUnique ( tracedb_BTree *bt,
    tracedb_BTNode *n, tracedb_BTNode **exist,
    int ( * sort ) ( const tracedb_BTNode *n, const tracedb_BTNode *p ) )
{
    if ( bt != NULL && n != NULL )
    {
        int diff;

        tracedb_BTNode *p = bt -> root;
        tracedb_BTNode *q = NULL;
        tracedb_BTNode *y = NULL;

        while ( p != NULL )
        {
            diff = ( * sort ) ( n, p );

            if ( diff == 0 )
            {
                /* fail to insert */
                if ( exist != NULL )
                    * exist = p;
                return EEXIST;
            }

            q = p;
            if ( TRACEDB_BALANCE ( p ) != 0 )
                y = p;
            p = p -> child [ diff > 0 ];
        }

        n -> par = q;
        n -> child [ 0 ] = n -> child [ 1 ] = NULL;

        if ( q == NULL )
            bt -> root = n;
        else
        {
            q -> child [ diff > 0 ] = n;

            /* run a trace-back */
            for ( p = n; q != y; )
            {
                /* this is safe because q has 0 balance */
                tracedb_BTNode *z = q -> par;
                if ( q -> child [ 0 ] == p )
                    TRACEDB_SET_BALANCE ( q, TRACEDB_LEFT );
                else
                    TRACEDB_SET_BALANCE ( q, TRACEDB_RIGHT );

                p = q;
                q = z;
            }

            /* rebalance */
            if ( q != NULL )
                tracedb_RebalanceAfterInsert ( & bt -> root, q, p );
        }
    }

    /* only fails on existing item in this implementation */
    return 0;
}

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
    int ( * resort ) ( const tracedb_BTNode *item, const tracedb_BTNode *n ) )
{
    if ( bt != NULL )
    {
        tracedb_BTNode *p = bt -> root;
        bt -> root = NULL;

        while ( p != NULL )
        {
            tracedb_BTNode *q = p -> child [ 0 ];
            if ( q == 0 )
            {
                q = p -> child [ 1 ];
                tracedb_BTreeInsert ( bt, p, resort );
            }
            else
            {
                p -> child [ 0 ] = q -> child [ 1 ];
                q -> child [ 1 ] = p;
            }
            p = q;
        }
    }
}

/* BTreeUnlink
 *  removes a node from tree
 */
static
void tracedb_RebalanceAfterUnlink ( tracedb_BTNode **root, 
    tracedb_BTNode *q, bool dir )
{
    while ( q != 0 )
    {
        tracedb_BTNode *w, *x, *y = q;
        q = TRACEDB_PARENT ( q );

        if ( ! dir )
        {
            if ( q && q -> child [ 1 ] == y )
                dir = true;

            /* simulate an increment of balance */
            switch ( TRACEDB_BALANCE ( y ) )
            {
            case 0:
                TRACEDB_SET_BALANCE ( y, TRACEDB_RIGHT );
                return;
            case TRACEDB_LEFT:
                TRACEDB_CLR_BALANCE ( y, TRACEDB_LEFT );
                break;
            case TRACEDB_RIGHT:
                /* y has just become ++ */
                x = y -> child [ 1 ];
                if ( TRACEDB_LEFT_HEAVY ( x ) )
                {
                    w = tracedb_RotateRightAtXLeftAtY ( y, x );
                    if ( q == 0 )
                        * root = w;
                    else
                        q -> child [ dir ] = w;
                }
                else
                {
                    w = y -> child [ 1 ] = x -> child [ 0 ];
                    x -> child [ 0 ] = y;
                    TRACEDB_SET_PARENT ( x, q );
                    TRACEDB_SET_PARENT ( y, x );
                    if ( w != 0 )
                        TRACEDB_SET_PARENT ( w, y );
                    if ( q == 0 )
                        * root = x;
                    else
                        q -> child [ dir ] = x;
                    if ( TRACEDB_BALANCE ( x ) == 0 )
                    {
                        TRACEDB_SET_BALANCE ( x, TRACEDB_LEFT );
                        TRACEDB_SET_PARBAL ( y, x, TRACEDB_RIGHT );
                        return;
                    }
                    TRACEDB_ZERO_BALANCE ( x );
                    TRACEDB_ZERO_BALANCE ( y );
                    /* y = x; */
                }
                break;
            }
        }

        /* symmetric case */
        else
        {
            if ( q && q -> child [ 0 ] == y )
                dir = false;

            switch ( TRACEDB_BALANCE ( y ) )
            {
            case 0:
                TRACEDB_SET_BALANCE ( y, TRACEDB_LEFT );
                return;
            case TRACEDB_LEFT:
                /* y has just become -- */
                x = y -> child [ 0 ];
                if ( TRACEDB_RIGHT_HEAVY ( x ) )
                {
                    w = tracedb_RotateLeftAtXRightAtY ( y, x );
                    if ( q == 0 )
                        * root = w;
                    else
                        q -> child [ dir ] = w;
                }
                else
                {
                    w = x -> child [ 1 ];
                    y -> child [ 0 ] = w;
                    x -> child [ 1 ] = y;
                    TRACEDB_SET_PARENT ( x, q );
                    TRACEDB_SET_PARENT ( y, x );
                    if ( w != 0 )
                        TRACEDB_SET_PARENT ( w, y );
                    if ( q == 0 )
                        * root = x;
                    else
                        q -> child [ dir ] = x;
                    if ( TRACEDB_BALANCE ( x ) == 0 )
                    {
                        TRACEDB_SET_BALANCE ( x, TRACEDB_RIGHT );
                        TRACEDB_SET_PARBAL ( y, x, TRACEDB_LEFT );
                        return;
                    }
                    TRACEDB_ZERO_BALANCE ( x );
                    TRACEDB_ZERO_BALANCE ( y );
                    /* y = x; */
                }
                break;
            case TRACEDB_RIGHT:
                TRACEDB_CLR_BALANCE ( y, TRACEDB_RIGHT );
                break;
            }
        }
    }
}

static
void tracedb_BTUnlink ( tracedb_BTNode **root,
    tracedb_BTNode *p, bool dir )
{
    tracedb_BTNode *q = TRACEDB_PARENT ( p );
    tracedb_BTNode *l, *r = p -> child [ 1 ];
    if ( r == 0 )
    {
        // no right child - simple unlink
        l = p -> child [ 0 ];
        if ( q == 0 )
            * root = l;
        else
            q -> child [ dir ] = l;
        if ( l != 0 )
            TRACEDB_SET_PARENT ( l, q );
    }
    else
    {
        // have a right child - check its left
        l = r -> child [ 0 ];
        if ( l == 0 )
        {
            l = p -> child [ 0 ];
            r -> child [ 0 ] = l;

            // take not only p's parent ( q )
            // but its balance as well
            r -> par = p -> par;

            if ( q == 0 )
                * root = r;
            else
                q -> child [ dir ] = r;

            if ( l != 0 )
                TRACEDB_SET_PARENT ( l, r );

            // artificially reset for following
            q = r;
            dir = true;
        }

        // involves some work
        else
        {
            // find smallest subsequent item
            r = l -> child [ 0 ];
            while ( r != 0 )
            {
                l = r;
                r = l -> child [ 0 ];
            }

            // unlink it
            r = TRACEDB_PARENT ( l );
            r -> child [ 0 ] = l -> child [ 1 ];

            // take over doomed node
            l -> child [ 0 ] = p -> child [ 0 ];
            l -> child [ 1 ] = p -> child [ 1 ];

            // take not only p's parent ( q )
            // but its balance as well
            l -> par = p -> par;

            // new king pin
            if ( q == 0 )
                * root = l;
            else
                q -> child [ dir ] = l;

            // update parent links
            q = l -> child [ 0 ];
            if ( q != 0 )
                TRACEDB_SET_PARENT ( q, l );
            q = l -> child [ 1 ];
            TRACEDB_SET_PARENT ( q, l );
            q = r -> child [ 0 ];
            if ( q != 0 )
                TRACEDB_SET_PARENT ( q, r );

            q = r;
            dir = false;
        }
    }

    // now - rebalance what we've undone
    if ( q != 0 )
        tracedb_RebalanceAfterUnlink ( root, q, dir );
}

static
bool tracedb_BTreeContains ( const tracedb_BTNode *root,
    const tracedb_BTNode *n )
{
    while ( n != NULL )
    {
        if ( n == root )
            return true;
        n = TRACEDB_PARENT ( n );
    }
    return false;
}

bool tracedb_BTreeUnlink ( tracedb_BTree *bt, tracedb_BTNode *n )
{
    if ( bt != NULL && tracedb_BTreeContains ( bt -> root, n ) )
    {
        int dir = false;
        tracedb_BTNode *q = TRACEDB_PARENT ( n );
        if ( q != 0 )
        {
            assert ( q -> child [ 0 ] == n || q -> child [ 1 ] == n );
            dir = q -> child [ 1 ] == n;
        }
        tracedb_BTUnlink ( & bt -> root, n, ( bool ) dir );
        return true;
    }
    return false;
}

/* BTreeForEach
 *  executes a function on each tree element
 */
void tracedb_BTreeForEach ( const tracedb_BTree *bt, bool reverse,
    void ( * f ) ( tracedb_BTNode *n, void *data ), void *data )
{
    if ( bt != NULL )
    {
        tracedb_BTNode *n, *next;
        if ( reverse )
        {
            n = tracedb_LastNode ( bt );
            while ( n != NULL )
            {
                next = tracedb_BTNodePrev ( n );
                ( * f ) ( n, data );
                n = next;
            }
        }
        else
        {
            n = tracedb_FirstNode ( bt );
            while ( n != NULL )
            {
                next = tracedb_BTNodeNext ( n );
                ( * f ) ( n, data );
                n = next;
            }
        }
    }
}

/* BTreeDoUntil
 *  executes a function on each element
 *  until the function returns true
 */
bool tracedb_BTreeDoUntil ( const tracedb_BTree *bt, bool reverse,
    bool ( * f ) ( tracedb_BTNode *n, void *data ), void *data )
{
    if ( bt != NULL )
    {
        tracedb_BTNode *n, *next;
        if ( reverse )
        {
            n = tracedb_LastNode ( bt );
            while ( n != NULL )
            {
                next = tracedb_BTNodePrev ( n );
                if ( ( * f ) ( n, data ) )
                    return true;
                n = next;
            }
        }
        else
        {
            n = tracedb_FirstNode ( bt );
            while ( n != NULL )
            {
                next = tracedb_BTNodeNext ( n );
                if ( ( * f ) ( n, data ) )
                    return true;
                n = next;
            }
        }
    }
    return false;
}

/* BTreeWhack
 *  removes nodes from tree and
 *  executes a user provided destructor
 */
void tracedb_BTreeWhack ( tracedb_BTree *bt,
    void ( * whack ) ( tracedb_BTNode *n, void *data ), void *data )
{
    if ( bt != NULL )
    {
        tracedb_BTNode *p = bt -> root;
        bt -> root = NULL;

        if ( whack != NULL )
        {
            while ( p != NULL )
            {
                tracedb_BTNode *q = p -> child [ 0 ];
                if ( q == 0 )
                {
                    q = p -> child [ 1 ];
                    ( * whack ) ( p, data );
                }
                else
                {
                    p -> child [ 0 ] = q -> child [ 1 ];
                    q -> child [ 1 ] = p;
                }
                p = q;
            }
        }
    }
}


/*--------------------------------------------------------------------------
 * SparseArrayBlock
 */
typedef struct tracedb_SparseArrayBlock tracedb_SparseArrayBlock;
struct tracedb_SparseArrayBlock
{
    tracedb_BTNode n;
    uint32_t count;
    int32_t first;
    void *elem [ 1 ];
};

/* SparseArrayBlockMake
 */
static
int tracedb_SparseArrayBlockMake ( tracedb_SparseArrayBlock **bp,
    unsigned int block_len, int first )
{
    size_t array_size = sizeof ( * bp ) -> elem [ 0 ] * block_len;
    tracedb_SparseArrayBlock *b = malloc ( sizeof * b - sizeof b -> elem + array_size );
    if ( b == NULL )
        return errno;

    b -> count = 0;
    b -> first = first;
    memset ( b -> elem, 0, array_size );

    * bp = b;
    return 0;
}

/* SparseArrayBlockFind
 */
static
int tracedb_SparseArrayBlockFind ( const void *item, const tracedb_BTNode *n )
{
#define a ( * ( const int* ) item )
#define b ( ( const tracedb_SparseArrayBlock* ) n )

    return a - b -> first;

#undef a
#undef b
}

/* SparseArrayBlockSort
 */
static
int tracedb_SparseArrayBlockSort ( const tracedb_BTNode *item,
    const tracedb_BTNode *n )
{
#define a ( ( const tracedb_SparseArrayBlock* ) item )
#define b ( ( const tracedb_SparseArrayBlock* ) n )

    return a -> first - b -> first;

#undef a
#undef b
}

/* SparseArrayBlockForEach
 */
typedef struct tracedb_SparseArrayBlockForEachData tracedb_SparseArrayBlockForEachData;
struct tracedb_SparseArrayBlockForEachData
{
    tracedb_BTree *bt;
    void ( * f ) ( void *item, int i, void *data );
    void *data;
    int block_len;
    bool reverse;
};

static
void tracedb_SparseArrayBlockForEach ( tracedb_BTNode *n, void *data )
{
    int i;
    void *elem;

    const tracedb_SparseArrayBlockForEachData *pb = ( const void* ) data;
    const tracedb_SparseArrayBlock *b = ( const tracedb_SparseArrayBlock* ) n;

    if ( pb -> reverse ) for ( i = pb -> block_len; -- i >= 0; )
    {
        elem = b -> elem [ i ];
        if ( elem != NULL )
            ( * pb -> f ) ( elem, i + b -> first, pb -> data );
    }
    else for ( i = 0; i < pb -> block_len; ++ i )
    {
        elem = b -> elem [ i ];
        if ( elem != NULL )
            ( * pb -> f ) ( elem, i + b -> first, pb -> data );
    }
}

/* SparseArrayBlockWhack
 */
static
void tracedb_SparseArrayBlockWhack ( tracedb_BTNode *n, void *data )
{
    /* whack contents */
    tracedb_SparseArrayBlockForEach ( n, data );

    /* whack node */
    free ( n );
}

/* SparseArrayBlockDoUntil
 */
typedef struct tracedb_SparseArrayBlockDoUntilData tracedb_SparseArrayBlockDoUntilData;
struct tracedb_SparseArrayBlockDoUntilData
{
    bool ( * f ) ( void *item, int i, void *data );
    void *data;
    int block_len;
    bool reverse;
};

static
bool tracedb_SparseArrayBlockDoUntil ( tracedb_BTNode *n, void *data )
{
    int i;
    void *elem;

    const tracedb_SparseArrayBlockDoUntilData *pb = ( const void* ) data;
    const tracedb_SparseArrayBlock *b = ( const tracedb_SparseArrayBlock* ) n;

    if ( pb -> reverse ) for ( i = pb -> block_len; -- i >= 0; )
    {
        elem = b -> elem [ i ];
        if ( elem != NULL )
        {
            if ( ( * pb -> f ) ( elem, i + b -> first, pb -> data ) )
                return true;
        }
    }
    else for ( i = 0; i < pb -> block_len; ++ i )
    {
        elem = b -> elem [ i ];
        if ( elem != NULL )
        {
            if ( ( * pb -> f ) ( elem, i + b -> first, pb -> data ) )
                return true;
        }
    }

    return false;
}


/*--------------------------------------------------------------------------
 * SparseArray
 *  sparse array, supporting arbitrary signed integer addressing
 */

/* SparseArrayInit
 *  initialize b-tree
 */
void tracedb_SparseArrayInit ( tracedb_SparseArray *sa, unsigned int block_len )
{
    if ( sa != NULL )
    {
        /* block_len needs to be power of 2 */
        if ( ( block_len & ( block_len - 1 ) ) != 0 )
        {
            unsigned int bits = 1;
            while ( bits < block_len )
                bits <<= 1;
            block_len = bits;
        }

        tracedb_BTreeInit ( & sa -> bt );
        sa -> block_len = ( int ) block_len;
    }
}

/* SparseArrayGet
 *  returns an indexed element
 */
void *tracedb_SparseArrayGet ( const tracedb_SparseArray *sa, int idx )
{
    if ( sa != NULL )
    {
        int i = idx & ( sa -> block_len - 1 );
        int first = idx - i;
        const tracedb_SparseArrayBlock *b = ( const tracedb_SparseArrayBlock* )
            tracedb_BTreeFind ( & sa -> bt, & first, tracedb_SparseArrayBlockFind );
        if ( b != NULL )
            return b -> elem [ i ];
    }

    return NULL;
}

/* SparseArraySet
 *  sets an indexed element
 *  returns 0 on success, error code ( ENOMEM ) otherwise
 */
int tracedb_SparseArraySet ( tracedb_SparseArray *sa, int idx, void *item )
{
    int i, first;
    tracedb_SparseArrayBlock *b;

    if ( sa == NULL )
        return EINVAL;

    i = idx & ( sa -> block_len - 1 );
    first = idx - i;

    b = ( tracedb_SparseArrayBlock* )
        tracedb_BTreeFind ( & sa -> bt, & first, tracedb_SparseArrayBlockFind );

    if ( b == NULL )
    {
        int status = tracedb_SparseArrayBlockMake ( & b, sa -> block_len, first );
        if ( status != 0 )
            return status;
        status = tracedb_BTreeInsert ( & sa -> bt, & b -> n, tracedb_SparseArrayBlockSort );
        if ( status != 0 )
        {
            free ( b );
            return status;
        }
    }

    if ( b -> elem [ i ] != NULL )
    {
        assert ( b -> count > 0 );
        if ( item == NULL )
            -- b -> count;
    }
    else if ( item != NULL )
    {
        ++ b -> count;
    }

    b -> elem [ i ] = item;
    return 0;
}

/* SparseArrayUnset
 *  clears an indexed slot
 */
void tracedb_SparseArrayUnset ( tracedb_SparseArray *sa, int idx,
    void ( * whack ) ( void *item, int i, void *data ), void *data )
{
    if ( sa != NULL )
    {
        int i = idx & ( sa -> block_len - 1 );
        int first = idx - i;
        tracedb_SparseArrayBlock *b = ( tracedb_SparseArrayBlock* )
            tracedb_BTreeFind ( & sa -> bt, & first, tracedb_SparseArrayBlockFind );
        if ( b != NULL )
        {
            void *item = b -> elem [ i ];
            if ( item != NULL )
            {
                b -> elem [ i ] = NULL;

                if ( whack != NULL )
                    ( * whack ) ( item, idx, data );

                assert ( b -> count > 0 );
                if ( -- b -> count == 0 )
                {
                    tracedb_BTreeUnlink ( & sa -> bt, & b -> n );
                    free ( b );
                }
            }
        }
    }
}

/* SparseArrayForEach
 *  executes a function on each non-NULL element
 */
void tracedb_SparseArrayForEach ( const tracedb_SparseArray *sa, bool reverse,
    void ( * f ) ( void *item, int i, void *data ), void *data )
{
    if ( sa != NULL )
    {
        tracedb_SparseArrayBlockForEachData pb;
        pb . f = f;
        pb . data = data;
        pb . block_len = sa -> block_len;
        pb . reverse = reverse;

        tracedb_BTreeForEach ( & sa -> bt, reverse, 
                               tracedb_SparseArrayBlockForEach, & pb );
    }
}

/* SparseArrayDoUntil
 *  executes a function on each non-NULL element
 *  until the function returns true
 */
void tracedb_SparseArrayDoUntil ( const tracedb_SparseArray *sa, bool reverse,
    bool ( * f ) ( void *item, int i, void *data ), void *data )
{
    if ( sa != NULL )
    {
        tracedb_SparseArrayBlockDoUntilData pb;
        pb . f = f;
        pb . data = data;
        pb . block_len = sa -> block_len;
        pb . reverse = reverse;

        tracedb_BTreeDoUntil ( & sa -> bt, reverse,
                               tracedb_SparseArrayBlockDoUntil, & pb );
    }
}

/* SparseArrayWhack
 *  executes a user provided destructor
 */
void tracedb_SparseArrayWhack ( tracedb_SparseArray *sa,
    void ( * whack ) ( void *item, int i, void *data ), void *data )
{
    if ( sa != NULL )
    {
        tracedb_SparseArrayBlockForEachData pb;
        pb . bt = & sa -> bt;
        pb . f = whack;
        pb . data = data;
        pb . block_len = sa -> block_len;
        pb . reverse = false;

        tracedb_BTreeWhack ( & sa -> bt, tracedb_SparseArrayBlockWhack, & pb );
    }
}
