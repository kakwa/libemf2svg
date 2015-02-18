#include "polyclipping/clipper.hpp"
#include "clipperWrapper.h"

using namespace ClipperLib;

extern "C" {

clipSegmentList * bezierToLine(clipSegmentList *seg, uint32_t depth){

    if (depth == 0){
        return seg;
    }

    clipSegmentList *newPt1 = (clipSegmentList *)malloc(sizeof(clipSegmentList));
    clipSegmentList *newPt2 = (clipSegmentList *)malloc(sizeof(clipSegmentList));

    // some math is still needed
    newPt1->seg.st.x = 0;
    newPt1->seg.en.x = 0;
    newPt1->seg.c1.x = 0;
    newPt1->seg.c2.x = 0;

    newPt1->seg.st.y = 0;
    newPt1->seg.en.y = 0;
    newPt1->seg.c1.y = 0;
    newPt1->seg.c2.y = 0;

    newPt1->seg.type = LINE;

    newPt2->seg.st.x = 0;
    newPt2->seg.en.x = 0;
    newPt2->seg.c1.x = 0;
    newPt2->seg.c2.x = 0;

    newPt2->seg.st.y = 0;
    newPt2->seg.en.y = 0;
    newPt2->seg.c1.y = 0;
    newPt2->seg.c2.y = 0;

    newPt2->seg.type = LINE;

    free(seg);

    clipSegmentList *newSegStart1 = bezierToLine(newPt1, depth - 1);
    clipSegmentList *newSegStart2 = bezierToLine(newPt2, depth - 1);

    clipSegmentList *newSegEnd1 = newSegStart1->prev;
    clipSegmentList *newSegEnd2 = newSegStart2->prev;

    newSegStart1->prev = newSegEnd2;
    newSegStart2->prev = newSegEnd1;
    newSegEnd2->next   = newSegStart1;
    newSegEnd1->next   = newSegStart2;

    return newSegStart1;
}


clipSegmentList *arcToLine(clipSegmentList *seg, uint32_t depth){
    clipSegmentList *out = NULL;
    return out;
}

clipSegmentList merge(clipSegmentList form1, clipSegmentList form2, operation op){
    //Paths subj(2), clip(1), solution;

    ////define outer blue 'subject' polygon
    //subj[0] << 
    //    IntPoint(180,200) << IntPoint(260,200) <<
    //    IntPoint(260,150) << IntPoint(180,150);

    ////define subject's inner triangular 'hole' (with reverse orientation)
    //subj[1] << 
    //    IntPoint(215,160) << IntPoint(230,190) << IntPoint(200,190);

    ////define orange 'clipping' polygon
    //clip[0] << 
    //    IntPoint(190,210) << IntPoint(240,210) << 
    //    IntPoint(240,130) << IntPoint(190,130);

    ////draw input polygons with user-defined routine ... 

    ////perform intersection ...
    //Clipper c;
    //c.AddPaths(subj, ptSubject, true);
    //c.AddPaths(clip, ptClip, true);
    //c.Execute(ctIntersection, solution, pftNonZero, pftNonZero);
    
    clipSegmentList out;
    return out;
}
}
