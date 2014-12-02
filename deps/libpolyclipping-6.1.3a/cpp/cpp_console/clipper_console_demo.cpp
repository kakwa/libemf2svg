#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cmath>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "clipper.hpp"

//---------------------------------------------------------------------------

using namespace std;
using namespace ClipperLib;

static string ColorToHtml(unsigned clr)
{
  stringstream ss;
  ss << '#' << hex << std::setfill('0') << setw(6) << (clr & 0xFFFFFF);
  return ss.str();
}
//------------------------------------------------------------------------------

static float GetAlphaAsFrac(unsigned clr)
{
  return ((float)(clr >> 24) / 255);
}
//------------------------------------------------------------------------------

//a simple class that builds an SVG file with any number of paths
class SVGBuilder
{

  class StyleInfo
  {
  public:
  PolyFillType pft;
  unsigned brushClr;
  unsigned penClr;
  double penWidth;
  bool showCoords;

  StyleInfo()
  {
    pft = pftNonZero;
    brushClr = 0xFFFFFFCC;
    penClr = 0xFF000000;
    penWidth = 0.8;
    showCoords = false;
  }
  };

  class PolyInfo
  {
    public:
      Paths paths;
    StyleInfo si;

      PolyInfo(Paths paths, StyleInfo style)
      {
          this->paths = paths;
          this->si = style;
      }
  };

  typedef std::vector<PolyInfo> PolyInfoList;

private:
  PolyInfoList polyInfos;
  static const std::string svg_xml_start[];
  static const std::string poly_end[];

public:
  StyleInfo style;

  void AddPaths(Paths& poly)
  {
    if (poly.size() == 0) return;
    polyInfos.push_back(PolyInfo(poly, style));
  }

  bool SaveToFile(char * filename, double scale = 1.0, int margin = 10)
  {
    //calculate the bounding rect ...
    PolyInfoList::size_type i = 0;
    Paths::size_type j;
    while (i < polyInfos.size())
    {
      j = 0;
      while (j < polyInfos[i].paths.size() &&
        polyInfos[i].paths[j].size() == 0) j++;
      if (j < polyInfos[i].paths.size()) break;
      i++;
    }
    if (i == polyInfos.size()) return false;

    IntRect rec;
    rec.left = polyInfos[i].paths[j][0].X;
    rec.right = rec.left;
    rec.top = polyInfos[i].paths[j][0].Y;
    rec.bottom = rec.top;
    for ( ; i < polyInfos.size(); ++i)
      for (Paths::size_type j = 0; j < polyInfos[i].paths.size(); ++j)
        for (Path::size_type k = 0; k < polyInfos[i].paths[j].size(); ++k)
        {
          IntPoint ip = polyInfos[i].paths[j][k];
          if (ip.X < rec.left) rec.left = ip.X;
          else if (ip.X > rec.right) rec.right = ip.X;
          if (ip.Y < rec.top) rec.top = ip.Y;
          else if (ip.Y > rec.bottom) rec.bottom = ip.Y;
        }

    if (scale == 0) scale = 1.0;
    if (margin < 0) margin = 0;
    rec.left = (long64)((double)rec.left * scale);
    rec.top = (long64)((double)rec.top * scale);
    rec.right = (long64)((double)rec.right * scale);
    rec.bottom = (long64)((double)rec.bottom * scale);
    long64 offsetX = -rec.left + margin;
    long64 offsetY = -rec.top + margin;

    ofstream file;
    file.open(filename);
    if (!file.is_open()) return false;
    file.setf(ios::fixed);
    file.precision(0);
    file << svg_xml_start[0] <<
      ((rec.right - rec.left) + margin*2) << "px" << svg_xml_start[1] <<
      ((rec.bottom - rec.top) + margin*2) << "px" << svg_xml_start[2] <<
      ((rec.right - rec.left) + margin*2) << " " <<
      ((rec.bottom - rec.top) + margin*2) << svg_xml_start[3];
    setlocale(LC_NUMERIC, "C");
    file.precision(2);

    for (PolyInfoList::size_type i = 0; i < polyInfos.size(); ++i)
  {
      file << " <path d=\"";
    for (Paths::size_type j = 0; j < polyInfos[i].paths.size(); ++j)
      {
        if (polyInfos[i].paths[j].size() < 3) continue;
        file << " M " << ((double)polyInfos[i].paths[j][0].X * scale + offsetX) <<
          " " << ((double)polyInfos[i].paths[j][0].Y * scale + offsetY);
        for (Path::size_type k = 1; k < polyInfos[i].paths[j].size(); ++k)
        {
          IntPoint ip = polyInfos[i].paths[j][k];
          double x = (double)ip.X * scale;
          double y = (double)ip.Y * scale;
          file << " L " << (x + offsetX) << " " << (y + offsetY);
        }
        file << " z";
    }
      file << poly_end[0] << ColorToHtml(polyInfos[i].si.brushClr) <<
    poly_end[1] << GetAlphaAsFrac(polyInfos[i].si.brushClr) <<
        poly_end[2] <<
        (polyInfos[i].si.pft == pftEvenOdd ? "evenodd" : "nonzero") <<
        poly_end[3] << ColorToHtml(polyInfos[i].si.penClr) <<
    poly_end[4] << GetAlphaAsFrac(polyInfos[i].si.penClr) <<
        poly_end[5] << polyInfos[i].si.penWidth << poly_end[6];

        if (polyInfos[i].si.showCoords)
        {
      file << "<g font-family=\"Verdana\" font-size=\"11\" fill=\"black\">\n\n";
      for (Paths::size_type j = 0; j < polyInfos[i].paths.size(); ++j)
      {
        if (polyInfos[i].paths[j].size() < 3) continue;
        for (Path::size_type k = 0; k < polyInfos[i].paths[j].size(); ++k)
        {
          IntPoint ip = polyInfos[i].paths[j][k];
          file << "<text x=\"" << (int)(ip.X * scale + offsetX) <<
          "\" y=\"" << (int)(ip.Y * scale + offsetY) << "\">" <<
          ip.X << "," << ip.Y << "</text>\n";
          file << "\n";
        }
      }
      file << "</g>\n";
        }
    }
    file << "</svg>\n";
    file.close();
    setlocale(LC_NUMERIC, "");
    return true;
  }
};
//------------------------------------------------------------------------------

const std::string SVGBuilder::svg_xml_start [] =
  {"<?xml version=\"1.0\" standalone=\"no\"?>\n"
   "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\"\n"
   "\"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n\n"
   "<svg width=\"",
   "\" height=\"",
   "\" viewBox=\"0 0 ",
   "\" version=\"1.0\" xmlns=\"http://www.w3.org/2000/svg\">\n\n"
  };
const std::string SVGBuilder::poly_end [] =
  {"\"\n style=\"fill:",
   "; fill-opacity:",
   "; fill-rule:",
   "; stroke:",
   "; stroke-opacity:",
   "; stroke-width:",
   ";\"/>\n\n"
  };

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

inline long64 Round(double val)
{
  if ((val < 0)) return (long64)(val - 0.5); else return (long64)(val + 0.5);
}
//------------------------------------------------------------------------------

bool LoadFromFile(Paths &ppg, char * filename, double scale= 1,
  int xOffset = 0, int yOffset = 0)
{
  ppg.clear();

  FILE *f = fopen(filename, "r");
  if (!f) return false;
  int polyCnt, vertCnt;
  char junk [80];
  double X, Y;
  if (fscanf(f, "%d", &polyCnt) == 1 && polyCnt > 0)
  {
    ppg.resize(polyCnt);
    for (int i = 0; i < polyCnt; i++) {
      if (fscanf(f, "%d", &vertCnt) != 1 || vertCnt <= 0) break;
      ppg[i].resize(vertCnt);
      for (int j = 0; j < vertCnt; j++) {
        if (fscanf(f, "%lf%*[, ]%lf", &X, &Y) != 2) break;
        ppg[i][j].X = Round((X + xOffset) * scale);
        ppg[i][j].Y = Round((Y + yOffset) * scale);
        fgets(junk, 80, f);
      }
    }
  }
  fclose(f);
  return true;
}
//------------------------------------------------------------------------------

void SaveToConsole(const string name, const Paths &pp, double scale = 1.0)
{
  cout << '\n' << name << ":\n"
    << pp.size() << '\n';
  for (unsigned i = 0; i < pp.size(); ++i)
  {
    cout << pp[i].size() << '\n';
    for (unsigned j = 0; j < pp[i].size(); ++j)
      cout << pp[i][j].X /scale << ", " << pp[i][j].Y /scale << ",\n";
  }
  cout << "\n";
}
//---------------------------------------------------------------------------

void SaveToFile(char *filename, Paths &pp, double scale = 1)
{
  FILE *f = fopen(filename, "w");
  if (!f) return;
  fprintf(f, "%d\n", pp.size());
  for (unsigned i = 0; i < pp.size(); ++i)
  {
    fprintf(f, "%d\n", pp[i].size());
    if (scale > 1.01 || scale < 0.99) {
      for (unsigned j = 0; j < pp[i].size(); ++j)
        fprintf(f, "%.6lf, %.6lf,\n",
          (double)pp[i][j].X /scale, (double)pp[i][j].Y /scale);
    }
    else
    {
      for (unsigned j = 0; j < pp[i].size(); ++j)
        fprintf(f, "%lld, %lld,\n", pp[i][j].X, pp[i][j].Y );
    }
  }
  fclose(f);
}
//---------------------------------------------------------------------------

void MakeRandomPoly(int edgeCount, int width, int height, Paths & poly)
{
  poly.resize(1);
  poly[0].resize(edgeCount);
  for (int i = 0; i < edgeCount; i++){
    poly[0][i].X = rand() % width;
    poly[0][i].Y = rand() % height;
  }
}
//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
  ////quick test with random paths ...
  //Paths ss, cc, sss;
  //PolyFillType pft = pftNonZero;//pftEvenOdd;//
  //srand((int)time(0));
  //MakeRandomPoly(100, 400, 400, ss);
  //MakeRandomPoly(100, 400, 400, cc);
  //Clipper cpr;
  //cpr.AddPaths(ss, ptSubject, true);
  //cpr.AddPaths(cc, ptClip, true);
  //cpr.Execute(ctIntersection, sss, pft, pft);
  ////sss = Clipper.OffsetPolygons(sss, -5.0 * scale, JoinType.jtMiter, 4);
  //SVGBuilder svg1;
  //svg1.style.pft = pft;
  //svg1.style.brushClr = 0x2000009c;
  //svg1.style.penClr = 0xFFd3d3da;
  //svg1.AddPaths(ss);
  //svg1.style.brushClr = 0x209c0000;
  //svg1.style.penClr = 0xFFFFa07a;
  //svg1.AddPaths(cc);
  //svg1.style.brushClr = 0xAA80ff9c;
  //svg1.style.penClr = 0xFF003300;
  //svg1.AddPaths(sss);
  //svg1.SaveToFile("solution.svg", 1.0);
  //return 0;

  if (argc > 1 &&
    (strcmp(argv[1], "-b") == 0 || strcmp(argv[1], "--benchmark") == 0))
  {
    //do a benchmark test that creates a subject and a clip polygon both with
    //100 vertices randomly placed in a 400 * 400 space. Then perform an
    //intersection operation based on even-odd filling. Repeat all this X times.
    int loop_cnt = 100;
    char * dummy;
    if (argc > 2) loop_cnt = strtol(argv[2], &dummy, 10);
    if (loop_cnt == 0) loop_cnt = 100;
    cout << "\nPerforming " << loop_cnt << " random intersection operations ... ";
    srand((int)time(0));
    int error_cnt = 0;
    Paths subject, clip, solution;
    Clipper clpr;
    time_t time_start = clock();
    for (int i = 0; i < loop_cnt; i++) {
      MakeRandomPoly(100, 400, 400, subject);
      MakeRandomPoly(100, 400, 400, clip);
      clpr.Clear();
      clpr.AddPaths(subject, ptSubject, true);
      clpr.AddPaths(clip, ptClip, true);
      if (!clpr.Execute(ctIntersection, solution, pftEvenOdd, pftEvenOdd))
        error_cnt++;
    }
    double time_elapsed = double(clock() - time_start)/CLOCKS_PER_SEC;
    cout << "\nFinished in " << time_elapsed << " secs with ";
    cout << error_cnt << " errors.\n\n";
    //let's save the very last result ...
    SaveToFile("Subject.txt", subject);
    SaveToFile("Clip.txt", clip);
    SaveToFile("Solution.txt", solution);
    //and see it as an image too ...
    SVGBuilder svg;
    svg.style.penWidth = 0.8;
    svg.style.pft = pftEvenOdd;
    svg.style.brushClr = 0x1200009C;
    svg.style.penClr = 0xCCD3D3DA;
    svg.AddPaths(subject);
    svg.style.brushClr = 0x129C0000;
    svg.style.penClr = 0xCCFFA07A;
    svg.AddPaths(clip);
    svg.style.brushClr = 0x6080ff9C;
    svg.style.penClr = 0xFF003300;
    svg.style.pft = pftNonZero;
    svg.AddPaths(solution);
    svg.SaveToFile("solution.svg");
    return 0;
  }

  if (argc < 3)
  {
    cout << "\nUSAGE:\n"
      << "clipper --benchmark [LOOP_COUNT (default = 100)]\n"
      << "or\n"
      << "clipper sub_file clp_file CLIPTYPE [SUB_FILL CLP_FILL] [PRECISION] [SVG_SCALE]\n"
      << "where ...\n"
      << "  CLIPTYPE  = INTERSECTION or UNION or DIFFERENCE or XOR, and\n"
      << "  ???_FILL  = EVENODD or NONZERO (default = NONZERO)\n"
      << "  PRECISION = in decimal places (default = 0)\n"
      << "  SVG_SCALE = SVG output scale (default = 1.0)\n\n";
    cout << "\nINPUT AND OUTPUT FILE FORMAT ([optional] {comments}):\n"
      << "Polygon Count\n"
      << "Vertex Count {first polygon}\n"
      << "X, Y[,] {first vertex}\n"
      << "X, Y[,] {next vertex}\n"
      << "{etc.}\n"
      << "Vertex Count {second polygon, if there is one}\n"
      << "X, Y[,] {first vertex of second polygon}\n"
      << "{etc.}\n\n";
    return 1;
  }

  int scale_log10 = 0;
  char * dummy;
  if (argc > 6) scale_log10 = strtol(argv[6], &dummy, 10);
  double scale = std::pow(double(10), scale_log10);

  double svg_scale = 1.0;
  if (argc > 7) svg_scale = strtod(argv[7], &dummy);
  svg_scale /= scale;

  Paths subject, clip;

  if (!LoadFromFile(subject, argv[1], scale))
  {
    cerr << "\nCan't open the file " << argv[1]
      << " or the file format is invalid.\n";
    return 1;
  }
  if (!LoadFromFile(clip, argv[2], scale))
  {
    cerr << "\nCan't open the file " << argv[2]
      << " or the file format is invalid.\n";
    return 1;
  }

  ClipType clipType = ctIntersection;
  const string sClipType[] = {"INTERSECTION", "UNION", "DIFFERENCE", "XOR"};

  if (argc > 3)
  {
    if (_stricmp(argv[3], "XOR") == 0) clipType = ctXor;
    else if (_stricmp(argv[3], "UNION") == 0) clipType = ctUnion;
    else if (_stricmp(argv[3], "DIFFERENCE") == 0) clipType = ctDifference;
    else clipType = ctIntersection;
  }

  PolyFillType subj_pft = pftNonZero, clip_pft = pftNonZero;
  if (argc > 5)
  {
    if (_stricmp(argv[4], "EVENODD") == 0) subj_pft = pftEvenOdd;
    if (_stricmp(argv[5], "EVENODD") == 0) clip_pft = pftEvenOdd;
  }

  Clipper c;
  c.AddPaths(subject, ptSubject, true);
  c.AddPaths(clip, ptClip, true);
  Paths solution;

  bool succeeded = c.Execute(clipType, solution, subj_pft, clip_pft);
  string s = "Subjects (";
  s += (subj_pft == pftEvenOdd ? "EVENODD)" : "NONZERO)");

  //ie don't change the paths back to the original size if we've
  //just down-sized them to a manageable (all-in-one-screen) size ...
  //if (scale < 1) scale = 1;

  SaveToConsole(s, subject, scale);
  s = "Clips (";
  s += (clip_pft == pftEvenOdd ? "EVENODD)" : "NONZERO)");
  SaveToConsole(s, clip, scale);
  if (succeeded) {
    s = "Solution (using " + sClipType[clipType] + ")";
    //SaveToConsole(s, solution, scale);
    SaveToFile("solution.txt", solution, scale);

    //OffsetPaths(solution, solution, -5.0 *scale, jtRound, etClosed);

    //let's see the result too ...
    SVGBuilder svg;
    svg.style.penWidth = 0.8;
    svg.style.brushClr = 0x1200009C;
    svg.style.penClr = 0xCCD3D3DA;
    svg.style.pft = subj_pft;
    svg.AddPaths(subject);
    svg.style.brushClr = 0x129C0000;
    svg.style.penClr = 0xCCFFA07A;
    svg.style.pft = clip_pft;
    svg.AddPaths(clip);
    svg.style.brushClr = 0x6080ff9C;
    svg.style.penClr = 0xFF003300;
    svg.style.pft = pftNonZero;
    svg.AddPaths(solution);
    svg.SaveToFile("solution.svg", svg_scale);
  } else
      cout << (sClipType[clipType] + " failed!\n\n");

  return 0;
}
//---------------------------------------------------------------------------
