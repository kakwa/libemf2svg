#ifdef __cplusplus
extern "C" {
#endif
#include "EMFSVG_private.h"

enum segmentType{
    LINE,
    BEZIER,
    CUBIC
};

enum operation{
    INT,
    UNION,
    DIFF,
    XOR
};

typedef struct{
    //start
    POINT_D s;
    POINT_D e;
    POINT_D c1;
    POINT_D c2;
    segmentType type;
} clipSegment;

typedef struct clipsegmentlist {
    clipSegment seg;
    struct clipsegmentlist * next;
} clipSegmentList;


clipSegmentList bezierToLine(clipSegment seg);
clipSegmentList  arcToLine(clipSegment seg);

#ifdef __cplusplus
}
#endif
