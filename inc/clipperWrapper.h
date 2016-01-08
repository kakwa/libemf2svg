#ifdef __cplusplus
extern "C" {
#endif
#include "emf2svg_private.h"

typedef enum { LINE, BEZIER, CUBIC } segmentType;

typedef enum { INT, UNION, DIFF, XOR } operation;

typedef struct {
    // start
    POINT_D st;
    POINT_D en;
    POINT_D c1;
    POINT_D c2;
    segmentType type;
} clipSegment;

typedef struct clipsegmentlist {
    clipSegment seg;
    struct clipsegmentlist *next;
} clipSegmentList;

typedef struct tuplesegment {
    clipSegmentList *st;
    clipSegmentList *en;
} tupleSegments;

clipSegmentList *bezierToLine(clipSegmentList *seg, uint32_t depth);
clipSegmentList *arcToLine(clipSegmentList *seg, uint32_t depth);
clipSegmentList merge(clipSegmentList form1, clipSegmentList form2,
                      operation op);

#ifdef __cplusplus
}
#endif
