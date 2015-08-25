#ifdef __cplusplus
extern "C" {
#endif
#include "emf2svg_private.h"

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
    POINT_D st;
    POINT_D en;
    POINT_D c1;
    POINT_D c2;
    segmentType type;
} clipSegment;

typedef struct clipsegmentlist {
    clipSegment seg;
    struct clipsegmentlist * next;
    struct clipsegmentlist * prev;
} clipSegmentList;


clipSegmentList *bezierToLine(clipSegmentList *seg, uint32_t depth);
clipSegmentList *arcToLine(clipSegmentList *seg, uint32_t depth);
clipSegmentList merge(clipSegmentList form1, clipSegmentList form2, operation op);

#ifdef __cplusplus
}
#endif
