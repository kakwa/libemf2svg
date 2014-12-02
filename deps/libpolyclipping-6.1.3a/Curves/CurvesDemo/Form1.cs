using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Drawing;
using System.IO;
using System.Drawing.Drawing2D;
using ClipperLib;
using ClipperMultiPathsLib;

namespace Clipper_Lines_Demo
{

  using Path = List<IntPoint>;
  using Paths = List<List<IntPoint>>;
  using cInt = Int64;

  public partial class MainForm : Form
  {

    const int SUBJECT = 0, CLIP = 1;
    const double scale = 10.0; //removes the blocky-ness associated with integer rounding.
    const int btnRadius = (int)(3.0 * scale); //control button size
    bool LeftButtonPressed = false;
    int MovingButtonIdx = -1;
    MultiPathSegment MovingButtonSeg = null;
    MultiPaths allPaths = new MultiPaths(0.5 * scale);
    public Bitmap bmp = null;
    public Graphics bmpGraphics = null;
    string AppTitle;

    public MainForm()
    {
      InitializeComponent();
    }
    //------------------------------------------------------------------------------

    private CurveType GetRadiobuttonPathType()
    {
      if (rbSubjArc.Checked) return CurveType.Arc;
      else if (rbSubjCBezier.Checked) return CurveType.CubicBezier;
      else if (rbSubjQBezier.Checked) return CurveType.QuadBezier;
      else if (rbSubjEllipses.Checked) return CurveType.EllipticalArc;
      else return CurveType.Line;
    }
    //------------------------------------------------------------------------------

    private CurveType GetCurrentPathType(MultiPath mp)
    {
      if (mp.Count == 0) return CurveType.Line;
      else return mp[mp.Count - 1].curvetype;
    }
    //------------------------------------------------------------------------------

    private MultiPath GetCurrentSubjMultiPath()
    {
      for (int i = allPaths.Count - 1; i >= 0; i--)
        if (allPaths[i].RefID == SUBJECT) return allPaths[i];
      return null;
    }
    //------------------------------------------------------------------------------

    private MultiPath GetCurrentClipMultiPath()
    {
      for (int i = allPaths.Count - 1; i >= 0; i--)
        if (allPaths[i].RefID == CLIP) return allPaths[i];
      return null;
    }
    //------------------------------------------------------------------------------

    private void BmpUpdateNeeded()
    {
      const int textOffset = 20;
      
      if (bmpGraphics == null) return;
      FillMode fm = (mEvenOdd.Checked ? FillMode.Alternate : FillMode.Winding);
      bmpGraphics.Clear(Color.White);

      //draw the subject and clip paths ...
      Paths openPaths = new Paths();
      Paths closedPaths = new Paths();
      Paths clipPaths = new Paths();
      //sort the paths into open and closed subjects and (closed) clips ...
      foreach (MultiPath mp2 in allPaths)
        if (mp2.RefID == CLIP) clipPaths.Add(mp2.Flatten());
        else if (mp2.IsClosed) closedPaths.Add(mp2.Flatten()); 
        else openPaths.Add(mp2.Flatten());

      DrawPath(bmpGraphics, openPaths, false, 0x0, 0xFFAAAAAA, fm, 1.0);
      DrawPath(bmpGraphics, closedPaths, true, 0x0, 0xFFAAAAAA, fm, 1.0);
      DrawPath(bmpGraphics, clipPaths, true, 0x10FF6600, 0x99FF6600, fm, 1.0);

      if (cbShowCoords.Checked)
      {
        Font fnt = new Font("Arial", 8);
        SolidBrush brush = new SolidBrush(Color.Navy);
        foreach (MultiPath mp2 in allPaths)
        {
          foreach (MultiPathSegment mps in mp2)
            foreach (IntPoint ip in mps)
            {
              IntPoint ip2 = new IntPoint(ip.X / scale, ip.Y / scale);
              string coords = ip2.X.ToString() + "," + ip2.Y.ToString();
              bmpGraphics.DrawString(coords, fnt, brush, ip2.X - textOffset, ip2.Y - textOffset, null);
            }
        }
        fnt.Dispose();
        brush.Dispose();
      }

      //for the active path, draw control buttons and control lines too ...
      MultiPath activePath = GetActivePath();
      if (activePath != null && activePath.Count > 0)
      {
        foreach (MultiPathSegment mps in activePath)
        {
          CurveType pt = mps.curvetype;
          if (pt == CurveType.CubicBezier)
            DrawBezierCtrlLines(bmpGraphics, mps, 0xFFEEEEEE);
          else if (pt == CurveType.QuadBezier)
            DrawBezierCtrlLines(bmpGraphics, mps, 0xFFEEEEEE);
        }

        DrawButtons(bmpGraphics, activePath);

        //display the coords of a moving button ...
        if (MovingButtonIdx >= 0)
        {
          Font f = new Font("Arial", 8);
          SolidBrush b = new SolidBrush(Color.Navy);
          IntPoint ip = MovingButtonSeg[MovingButtonIdx];
          ip.X = (int)(ip.X / scale); ip.Y = (int)(ip.Y / scale);
          string coords = ip.X.ToString() + "," + ip.Y.ToString();
          bmpGraphics.DrawString(coords, f, b, ip.X - textOffset, ip.Y - textOffset, null);
          f.Dispose();
          b.Dispose();
        }
      }

      //if there's any clipping to be done, do it here ...
      if (!mNone.Checked && GetCurrentSubjMultiPath() != null && GetCurrentClipMultiPath() != null)
      {
        PolyFillType pft = (mEvenOdd.Checked ? PolyFillType.pftEvenOdd : PolyFillType.pftNonZero);
        ClipType ct;
        if (mUnion.Checked) ct = ClipType.ctUnion;
        else if (mDifference.Checked) ct = ClipType.ctDifference;
        else if (mXor.Checked) ct = ClipType.ctXor;
        else ct = ClipType.ctIntersection;

        //CLIPPING DONE HERE ...
        Clipper c = new Clipper();
        c.ZFillFunction = MultiPaths.ClipCallback; //set the callback function (called at intersections)
        if (openPaths.Count > 0)
          c.AddPaths(openPaths, PolyType.ptSubject, false);
        if (closedPaths.Count > 0)
          c.AddPaths(closedPaths, PolyType.ptSubject, true);
        c.AddPaths(clipPaths, PolyType.ptClip, true);
        PolyTree polytree = new PolyTree();

        Paths solution;
        c.Execute(ct, polytree, pft, pft); //EXECUTE CLIP !!!!!!!!!!!!!!!!!!!!!!
        solution = Clipper.ClosedPathsFromPolyTree(polytree);
        if (!cbReconstCurve.Checked)
          DrawPath(bmpGraphics, solution, true, 0x2033AA00, 0xFF33AA00, fm, 2.0);
        solution = Clipper.OpenPathsFromPolyTree(polytree);
        if (!cbReconstCurve.Checked)
          DrawPath(bmpGraphics, solution, false, 0x0, 0xFF33AA00, fm, 2.0);

        //now to demonstrate reconstructing beziers & arcs ...
        if (cbReconstCurve.Checked)
        {
          PolyNode pn = polytree.GetFirst();
          while (pn != null)
          {
            if (pn.IsHole || pn.Contour.Count < 2)
            {
              pn = pn.GetNext();
              continue;
            }

            if (pn.ChildCount > 0)
              throw new Exception("Sorry, this demo doesn't currently handle holes");

            //and reconstruct each curve ...
            MultiPath reconstructedMultiPath = allPaths.Reconstruct(pn.Contour);

            if (cbShowCtrls.Enabled && cbShowCtrls.Checked)
            {
              //show (small) buttons on the red reconstructed path too ...
              DrawButtons(bmpGraphics, reconstructedMultiPath, true);
            }

            //now to show how accurate these reconstructed (control) paths are,
            //we flatten them (drawing them red) so we can compare them with 
            //the original flattened paths (light gray) ...
            Paths paths = new Paths();
            paths.Add(reconstructedMultiPath.Flatten());
            DrawPath(bmpGraphics, paths, !pn.IsOpen, 0x18FF0000, 0xFFFF0000, fm, 2.0);
            
            pn = pn.GetNext();
          }
        }
        //else //shows just how many vertices there are in flattened paths ...
        //{
        //  solution = Clipper.PolyTreeToPaths(polytree);
        //  MultiPath flatMultiPath = new MultiPath(null, 0, false);
        //  foreach (Path p in solution)
        //    flatMultiPath.NewMultiPathSegment(PathType.Line, p);
        //  DrawButtons(bmpGraphics, flatMultiPath, true);
        //}
      }

      string s = "  ";
      if (mIntersection.Checked) s += "INTERSECTION";
      else if (mUnion.Checked) s += "UNION";
      else if (mDifference.Checked) s += "DIFFERENCE";
      else if (mXor.Checked) s += "XOR";
      else s += "NO CLIPPING";
      s += " with ";
      if (mEvenOdd.Checked) s += "EVENODD fill.";
      else s += "NONZERO fill.";
      toolStripStatusLabel2.Text = s;
      displayPanel.Invalidate();
    }
    //------------------------------------------------------------------------------

    private void DisplayPanel_Paint(object sender, PaintEventArgs e)
    {
      if (bmp == null) return;
      var g = e.Graphics;
      g.DrawImage(bmp,0,0);
    }
    //------------------------------------------------------------------------------

    private void mExit_Click(object sender, EventArgs e)
    {
      Close();
    }
    //------------------------------------------------------------------------------

    private static Color MakeColor(uint color)
    {
      return Color.FromArgb((int)color);
    }
    //------------------------------------------------------------------------------

    private static void IntPointsToGraphicsPath(Path path, GraphicsPath gp, bool closed)
    {
      if (path.Count == 0) return;
      PointF[] pts = new PointF[path.Count];
      int j = -1;
      for (int i = 0; i < path.Count; ++i)
      {
        if (i > 0 && path[i] == path[j]) continue;
        j++;
        pts[j] = PathToPointF(path[i]);
      }
      j++;
      if (j < path.Count) Array.Resize(ref pts, j);
      if (closed && j > 2)
      {
        gp.AddPolygon(pts);
      }
      else
      {
        gp.StartFigure();
        gp.AddLines(pts);
      }
    }
    //------------------------------------------------------------------------------

    private static void DoublePointsToGraphicsPath(List<DoublePoint> path, GraphicsPath gp, bool closed)
    {
      if (closed && path.Count < 2) return;
      else if (!closed && path.Count < 3) return;

      PointF[] pts = new PointF[path.Count];
      for (int i = 0; i < path.Count; ++i)
      {
        pts[i] = new PointF((float)(path[i].X / scale), (float)(path[i].Y / scale));
      }
      if (closed)
        gp.AddPolygon(pts);
      else
      {
        gp.StartFigure();
        gp.AddLines(pts);
      }
    }
    //------------------------------------------------------------------------------

    private static PointF[] MakeButton(IntPoint ip, bool small) 
    {
      float d1 = (small ? 3 : 4), d2 = (small ? 1.0f : 1.5f);
      PointF[] btnPts = new PointF[8];
      Path p = new Path(8);
      float x = (float)(ip.X / scale);
      float y = (float)(ip.Y / scale);
      btnPts[0] = new PointF(x - d2, y - d1);
      btnPts[1] = new PointF(x + d2, y - d1);
      btnPts[2] = new PointF(x + d1, y - d2);
      btnPts[3] = new PointF(x + d1, y + d2);
      btnPts[4] = new PointF(x + d2, y + d1);
      btnPts[5] = new PointF(x - d2, y + d1);
      btnPts[6] = new PointF(x - d1, y + d2);
      btnPts[7] = new PointF(x - d1, y - d2);
      return btnPts;
    }
    //------------------------------------------------------------------------------

    private static void DrawButtons(Graphics graphics, MultiPath mp, bool small = false)
    {
      if (mp == null || mp.Count == 0) return;
      GraphicsPath gpath = new GraphicsPath(FillMode.Alternate);
      SolidBrush midBrush, startBrush, endBrush;
      Pen pen;
      if (small)
      {
        midBrush = new SolidBrush(MakeColor(0xFFFFAAAA));
        startBrush = new SolidBrush(MakeColor(0xFFFFAAAA));
        endBrush = new SolidBrush(MakeColor(0xFFFFAAAA));
        pen = new Pen(MakeColor(0xFF660000), 1.0f);
      }
      else 
      {
        midBrush = new SolidBrush(MakeColor(0x20808080));
        startBrush = new SolidBrush(MakeColor(0x9980FF80));
        endBrush = new SolidBrush(MakeColor(0x99FA8072));
        pen = new Pen(Color.Black, 1.0f);
      }
      foreach (MultiPathSegment mps in mp)
      {
        int len = mps.Count;
        if (len == 0) continue;

        for (int j = 0; j < len; ++j)
        {
          PointF[] btnPts = MakeButton(mps[j], small);
          gpath.AddPolygon(btnPts);
        }
        graphics.FillPath(midBrush, gpath);
        graphics.DrawPath(pen, gpath);
        gpath.Reset();
      }

      //draw the start button a shade of green ...
      if (mp.Count > 0 && mp[0].Count > 0)
      {
        gpath.AddPolygon(MakeButton(mp[0][0], small));
        graphics.FillPath(startBrush, gpath);

        MultiPathSegment mps = mp[mp.Count - 1];
        //draw the end button a shade of red ...
        if (mps.index > 0 || mps.Count > 1)
        {
          gpath.Reset();
          gpath.AddPolygon(MakeButton(mps[mps.Count -1], small));
          graphics.FillPath(endBrush, gpath);
        }
      }

      //clean-up
      midBrush.Dispose();
      startBrush.Dispose();
      endBrush.Dispose();
      pen.Dispose();
      gpath.Dispose();
    }
    //------------------------------------------------------------------------------

    private static PointF PathToPointF(IntPoint ip)
    {
      PointF result = new PointF((float)(ip.X / scale), (float)(ip.Y / scale));
      return result;
    }
    //------------------------------------------------------------------------------

    private static void DrawBezierCtrlLines(Graphics graphics,
      MultiPathSegment mps, uint color)
    {
      int cnt = mps.Count;
      if (cnt < 2) return;
      Pen pen = new Pen(MakeColor(color));
      GraphicsPath gpath = new GraphicsPath();
      PointF[] pts = new PointF[2];
      pts[0] = PathToPointF(mps[0]);
      pts[1] = PathToPointF(mps[1]);
      gpath.StartFigure();
      gpath.AddLines(pts);

      if (mps.IsValid()) 
        if (mps.curvetype == CurveType.CubicBezier)
        {
          pts[0] = PathToPointF(mps[2]);
          pts[1] = PathToPointF(mps[3]);
          gpath.StartFigure();
          gpath.AddLines(pts);
        }
        else
        {
          pts[0] = PathToPointF(mps[2]);
          gpath.StartFigure();
          gpath.AddLines(pts);
        }

      graphics.DrawPath(pen, gpath);
      pen.Dispose();
      gpath.Dispose();
    }
    //------------------------------------------------------------------------------

    private static void DrawPath(Graphics graphics, Paths paths, bool closed, 
      uint brushClr, uint penClr,
      FillMode fillMode = FillMode.Alternate, double penWidth = 1.0)
    {
      if (paths.Count == 0) return;

      SolidBrush brush = new SolidBrush (MakeColor(brushClr));
      Pen pen = new Pen(MakeColor(penClr), (float)penWidth);
      GraphicsPath gpath = new GraphicsPath(fillMode);
      try
      {
        for (int i = 0; i < paths.Count; ++i)
          IntPointsToGraphicsPath(paths[i], gpath, closed);
        if (closed) graphics.FillPath(brush, gpath);
        graphics.DrawPath(pen, gpath);
      }
      finally
      {
        brush.Dispose();
        pen.Dispose();
        gpath.Dispose();
      }
    }
    //------------------------------------------------------------------------------

    private MultiPath GetActivePath()
    {
      if (rbClipPoly.Checked)
      {
        MultiPath mp = GetCurrentClipMultiPath();
        if (mp == null) return allPaths.NewMultiPath(CLIP, true);
        else return mp;
      }
      else
      {
        MultiPath mp = GetCurrentSubjMultiPath();
        if (mp == null) return allPaths.NewMultiPath(SUBJECT, false);
        else return mp;
      }
    }
    //------------------------------------------------------------------------------

    private int GetButtonIndex(IntPoint mousePt, out MultiPathSegment mps)
    {
      MultiPath mp = GetActivePath();
      mps = null;
      if (mp.Count == 0) return -1;
      for (int i = 0; i < mp.Count; i++)
        for (int j = 0; j < mp[i].Count; j++)
          if (Math.Abs(mp[i][j].X - mousePt.X) <= btnRadius &&
          Math.Abs(mp[i][j].Y - mousePt.Y) <= btnRadius)
          {
            mps = mp[i];
            return j;
          }
      return -1;
    }
    //------------------------------------------------------------------------------

    private void DisplayPanel_MouseDown(object sender, MouseEventArgs e)
    {
      if (e.Button == MouseButtons.Right)
      {
        mNewPath_Click(sender, e);
        MovingButtonIdx = -1;
      }
      else if (displayPanel.Cursor == Cursors.Hand)
      {
        MovingButtonIdx = GetButtonIndex(new IntPoint(e.X * scale, e.Y * scale), out MovingButtonSeg);
        BmpUpdateNeeded();
      }
      else
      {
        //Add a new control point ...

        CurveType rbPathType = GetRadiobuttonPathType();
        MultiPath mp = GetActivePath();
        if (mp.Count == 0)
          mp.NewMultiPathSegment(rbPathType, new Path());
        else if (rbPathType != GetCurrentPathType(mp))
        {
          if (rbPathType != GetCurrentPathType(mp))
          {
            Path tmp = new Path();
            if (!mp.IsValid()) 
            {
              MultiPathSegment mps = mp[mp.Count - 1];
              foreach (IntPoint ip in mps) tmp.Add(ip);
              mp.RemoveLast();
            }
            mp.NewMultiPathSegment(rbPathType, tmp);
          }
        }
        if (!mp[mp.Count - 1].Add(new IntPoint(e.X * scale, e.Y * scale)))
        {
          mp.NewMultiPathSegment(rbPathType, new Path());
          mp[mp.Count - 1].Add(new IntPoint(e.X * scale, e.Y * scale));
        }
        
        UpdateBtnAndMenuState();
        BmpUpdateNeeded();
        MovingButtonIdx = -1;
      }
      LeftButtonPressed = (e.Button == MouseButtons.Left);
    }
    //------------------------------------------------------------------------------

    private void DisplayPanel_MouseMove(object sender, MouseEventArgs e)
    {
      if (LeftButtonPressed)
      { 
        if (MovingButtonIdx < 0) return;
        MultiPath mp = GetActivePath();
        if (MovingButtonIdx >= MovingButtonSeg.Count) return;
        MovingButtonSeg.Move(MovingButtonIdx, new IntPoint(e.X * scale, e.Y * scale));
        BmpUpdateNeeded();
      }
      else
      {
        int i = GetButtonIndex(new IntPoint(e.X * scale, e.Y * scale), out MovingButtonSeg);
        displayPanel.Cursor = (i >= 0 ? Cursors.Hand : Cursors.Default);
      }
    }
    //------------------------------------------------------------------------------

    private void DisplayPanel_MouseUp(object sender, MouseEventArgs e)
    {
      if (e.Button == MouseButtons.Left) LeftButtonPressed = false;
      if (MovingButtonIdx >= 0)
      {
        MovingButtonIdx = -1;
        BmpUpdateNeeded();
      }
    }
    //------------------------------------------------------------------------------

    private void MainForm_KeyDown(object sender, KeyEventArgs e)
    {
      if (e.KeyValue == (int)Keys.Escape) Close();
      else if (e.KeyValue == (int)Keys.Z && e.Control) mUndo_Click(sender, e);
    }
    //------------------------------------------------------------------------------

    private void UpdateBtnAndMenuState()
    {
      mClear.Enabled = allPaths.Count > 0;
      MultiPath mp = GetActivePath();
      int cnt  = allPaths.Count;
      MultiPath subjMp = GetCurrentSubjMultiPath();

      cbSubjClosed.Checked = (subjMp != null && subjMp.IsClosed);
      if (mp.Count == 0)
      {
        mUndo.Enabled = mp.owner.Count > 0;
        mNewPath.Enabled = false;
        bNewPath.Enabled = false;
        return;
      }
      int j = mp[mp.Count - 1].Count;
      mUndo.Enabled = (mp.Count > 1 || j > 0);
      bNewPath.Enabled = mp.IsValid();
      mNewPath.Enabled = bNewPath.Enabled;
    }
    //------------------------------------------------------------------------------

    private void mClipType_Click(object sender, EventArgs e)
    {
      mIntersection.Checked = (sender == mIntersection);
      mUnion.Checked = (sender == mUnion);
      mDifference.Checked = (sender == mDifference);
      mXor.Checked = (sender == mXor);
      mNone.Checked = (sender == mNone);
      BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void mFillType_Clicked(object sender, EventArgs e)
    {
      mEvenOdd.Checked = (sender == mEvenOdd);
      mNonZero.Checked = (sender == mNonZero);
      BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void mUndo_Click(object sender, EventArgs e)
    {
      MultiPath mp = GetActivePath();
      if (mp.Count == 0)
      {
        if (mp.owner.Count == 1) return;
        else mp.owner.RemoveAt(mp.owner.Count -1);
      }
      else
      {
        MultiPathSegment mps = mp[mp.Count - 1];
        if (!mps.RemoveLast()) mp.RemoveLast();
      }
      UpdateBtnAndMenuState();
      BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void mClear_Click(object sender, EventArgs e)
    {
      allPaths.Clear();
      UpdateBtnAndMenuState();
      BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void mNewPath_Click(object sender, EventArgs e)
    {
      MultiPath mp = GetActivePath();
      if (!mp.IsValid()) return;
      int refID = (rbClipPoly.Checked ? CLIP : SUBJECT);
      mp.owner.NewMultiPath((UInt16)refID, mp.IsClosed);
      UpdateBtnAndMenuState();
      BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void rbAdd_Click(object sender, EventArgs e)
    {
      UpdateBtnAndMenuState();
      BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void OnLoadResize(bool isLoading)
    {
      if (bmp != null) bmp.Dispose();
      if (bmpGraphics != null) bmpGraphics.Dispose();
      Rectangle r = displayPanel.ClientRectangle;
      bmp = new Bitmap(r.Right, r.Height);
      bmpGraphics = Graphics.FromImage(bmp);
      bmpGraphics.SmoothingMode = SmoothingMode.AntiAlias;
      bmpGraphics.InterpolationMode = InterpolationMode.HighQualityBicubic;

      toolStripStatusLabel2.Width = statusStrip1.ClientSize.Width -
        toolStripStatusLabel1.Width - statusStrip1.ClientSize.Height;

      if (!isLoading) BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void MainForm_Resize(object sender, EventArgs e)
    {
      OnLoadResize(false);
    }
    //------------------------------------------------------------------------------

    private void MainForm_Load(object sender, EventArgs e)
    {
      OnLoadResize(true);
      cbReconstCurve.Text = "Redraw &Reconstructed\nOpen Curves ( bold red )";
      AppTitle = this.Text + " - ";
    }
    //------------------------------------------------------------------------------

    private void MainForm_FormClosed(object sender, FormClosedEventArgs e)
    {
      if (bmp != null) bmp.Dispose();
      if (bmpGraphics != null) bmpGraphics.Dispose();
    }
    //------------------------------------------------------------------------------

    private void quickTipsToolStripMenuItem_Click(object sender, EventArgs e)
    {
      MessageBox.Show(this, 
			  "Ctrl+I - for Intersection operations\n" +
        "Ctrl+U - for Union operations       \n" +
        "Ctrl+D - for Difference operations  \n" +
        "Ctrl+X - for XOR operations         \n" +
        "Ctrl+Q - clipping off  \n" + 
			  "------------------------------  \n" +
        "Ctrl+E - for EvenOdd fills          \n" +
        "Ctrl+N - for NonZero fills          \n" +
        "------------------------------  \n" +
        "Ctrl+P or RightClick - start new path \n" +
        "Ctrl+Z - Undo last Button entry \n" +
        "Ctrl+C - Clear \n" +
        "------------------------------  \n" +
        "F1 - to see these tips again   \n" +
			  "Esc - to quit                  \n",
        this.Text);
    }
    //------------------------------------------------------------------------------

    private void cbReconstCurve_Click(object sender, EventArgs e)
    {
      cbShowCtrls.Enabled = cbReconstCurve.Checked;
      if (allPaths.Count > 0 && GetCurrentSubjMultiPath() != null && 
        GetCurrentClipMultiPath() != null) 
          BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void cbShowCtrls_Click(object sender, EventArgs e)
    {
      BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void mSave_Click(object sender, EventArgs e)
    {
      if (openFileDialog1.FileName == "")
      {
        mSaveAs_Click(sender, e);
        return;
      }
      StreamWriter writer = new StreamWriter(openFileDialog1.FileName);
      if (writer == null) return;
      writer.Write(allPaths.ToSvgString());
      writer.Close();
    }
    //------------------------------------------------------------------------------

    private void mSaveAs_Click(object sender, EventArgs e)
    {
      if (allPaths.Count == 0 || (allPaths.Count == 1 && allPaths[0].Count < 2))
        return;
      if (openFileDialog1.FileName != "")
        saveFileDialog1.FileName = openFileDialog1.FileName;
      if (saveFileDialog1.ShowDialog() != DialogResult.OK) return;
      openFileDialog1.FileName = saveFileDialog1.FileName;
      this.Text = AppTitle + System.IO.Path.GetFileName(openFileDialog1.FileName);
      StreamWriter writer = new StreamWriter(saveFileDialog1.FileName);
      if (writer == null) return;
      writer.Write(allPaths.ToSvgString());
      writer.Close();
    }
    //------------------------------------------------------------------------------

    private void mOpen_Click(object sender, EventArgs e)
    {
      if (openFileDialog1.ShowDialog() != DialogResult.OK) return;
      allPaths.Clear();
      this.Text = AppTitle + System.IO.Path.GetFileName(openFileDialog1.FileName);
      StreamReader sr = new StreamReader(openFileDialog1.FileName);
      allPaths.FromSvgString(sr.ReadToEnd());
      sr.Close();
      UpdateBtnAndMenuState();
      BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void cbSubjClosed_Click(object sender, EventArgs e)
    {
      MultiPath mp = GetCurrentSubjMultiPath();
      if (mp == null) allPaths.NewMultiPath(SUBJECT, false);
      mp.IsClosed = cbSubjClosed.Checked;
      if (rbClipPoly.Checked) 
      {
        if (mp.Count == 0) rbSubjLine.Checked = true;
        else
          switch (mp[mp.Count - 1].curvetype)
          {
            case CurveType.Line: rbSubjLine.Checked = true; break;
            case CurveType.Arc: rbSubjArc.Checked = true; break;
            case CurveType.CubicBezier: rbSubjCBezier.Checked = true; break;
            case CurveType.QuadBezier: rbSubjQBezier.Checked = true; break;
            //case CurveType.EllipticalArc: rbSubjEllipses.Checked = true; break;
          }
      }
      BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void ScaleMultiPaths(MultiPaths multiP, double scale)
    {
      foreach (MultiPath mp in multiP)
        foreach (MultiPathSegment mps in mp)
          for (int i = 0; i < mps.Count; i++)
          {
            if (i == 0 && mps.index > 0) continue;
            IntPoint ip = new IntPoint(mps[i].X * scale, mps[i].Y * scale, mps[i].Z);
            mps.Move(i, ip);
          }
    }
    //------------------------------------------------------------------------------

    private void mZoomIn_Click(object sender, EventArgs e)
    {
      ScaleMultiPaths(allPaths, 2.0);
      UpdateBtnAndMenuState();
      BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void mZoomOut_Click(object sender, EventArgs e)
    {
      ScaleMultiPaths(allPaths, 0.5);
      UpdateBtnAndMenuState();
      BmpUpdateNeeded();
    }
    //------------------------------------------------------------------------------

    private void cbShowCoords_Click(object sender, EventArgs e)
    {
      BmpUpdateNeeded();
    }

  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------


}
