#include "tracedata.h"

#include <stdlib.h>

/* exercises the linker to ensure all code is in library */
int main ( int argc, char *argv [] )
{
    if ( argc == 0 && argv == NULL )
    {
        size_t mlim;
        unsigned int flim;

        char buffer [ 128 ];
        size_t num_read, remaining;

        TraceData *td;

        TraceDB db;
        TraceDBInit ( & db, 0, 0 );

        TraceDBGetSoftFileLimit ( & db, & flim );
        TraceDBSetSoftFileLimit ( & db, flim );

        TraceDBGetSoftMemLimit ( &db, & mlim );
        TraceDBSetSoftMemLimit ( & db, mlim );

        TraceDBRunGCTasks ( & db, 0 );

        TraceDBRead ( & db, "path", 1, buffer, sizeof buffer,
            0, & num_read, & remaining );

        TraceDBInitDecompression ( & db );

        TraceDBGetTraceData ( & db, "path", 1,
            td_basecall | td_qualscore | td_peakindex |
            td_comment | td_extended, & td );
        TraceDataGetBasecall ( td, & num_read );
        TraceDataGetQualscore ( td, & num_read );
        TraceDataGetPeakindex ( td, & num_read );
        TraceDataGetComment ( td, & num_read );
        TraceDataGetExtended ( td, & num_read );
        TraceDataWhack ( td );

        TraceDBWhackDecompression ( & db );

        TraceDBWhack ( & db, 0 );
    }

    return 0;
}
