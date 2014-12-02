namespace Clipper_Lines_Demo
{
  partial class MainForm
  {
    /// <summary>
    /// Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    /// Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing)
    {
      if (disposing && (components != null))
      {
        components.Dispose();
      }
      base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    /// <summary>
    /// Required method for Designer support - do not modify
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
      this.statusStrip1 = new System.Windows.Forms.StatusStrip();
      this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
      this.toolStripStatusLabel2 = new System.Windows.Forms.ToolStripStatusLabel();
      this.menuStrip1 = new System.Windows.Forms.MenuStrip();
      this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.mLoad = new System.Windows.Forms.ToolStripMenuItem();
      this.mSave = new System.Windows.Forms.ToolStripMenuItem();
      this.mSaveAs = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripMenuItem3 = new System.Windows.Forms.ToolStripSeparator();
      this.mExit = new System.Windows.Forms.ToolStripMenuItem();
      this.editToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.mIntersection = new System.Windows.Forms.ToolStripMenuItem();
      this.mUnion = new System.Windows.Forms.ToolStripMenuItem();
      this.mDifference = new System.Windows.Forms.ToolStripMenuItem();
      this.mXor = new System.Windows.Forms.ToolStripMenuItem();
      this.mNone = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripSeparator();
      this.mEvenOdd = new System.Windows.Forms.ToolStripMenuItem();
      this.mNonZero = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripMenuItem2 = new System.Windows.Forms.ToolStripSeparator();
      this.mNewPath = new System.Windows.Forms.ToolStripMenuItem();
      this.mUndo = new System.Windows.Forms.ToolStripMenuItem();
      this.mClear = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripMenuItem4 = new System.Windows.Forms.ToolStripSeparator();
      this.mZoomIn = new System.Windows.Forms.ToolStripMenuItem();
      this.mZoomOut = new System.Windows.Forms.ToolStripMenuItem();
      this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.quickTipsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.panel1 = new System.Windows.Forms.Panel();
      this.cbShowCoords = new System.Windows.Forms.CheckBox();
      this.cbSubjClosed = new System.Windows.Forms.CheckBox();
      this.rbClipPoly = new System.Windows.Forms.RadioButton();
      this.rbSubjEllipses = new System.Windows.Forms.RadioButton();
      this.rbSubjArc = new System.Windows.Forms.RadioButton();
      this.rbSubjQBezier = new System.Windows.Forms.RadioButton();
      this.rbSubjCBezier = new System.Windows.Forms.RadioButton();
      this.rbSubjLine = new System.Windows.Forms.RadioButton();
      this.groupBox2 = new System.Windows.Forms.GroupBox();
      this.cbShowCtrls = new System.Windows.Forms.CheckBox();
      this.cbReconstCurve = new System.Windows.Forms.CheckBox();
      this.bNewPath = new System.Windows.Forms.Button();
      this.groupBox1 = new System.Windows.Forms.GroupBox();
      this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
      this.saveFileDialog1 = new System.Windows.Forms.SaveFileDialog();
      this.displayPanel = new System.Windows.Forms.PictureBox();
      this.statusStrip1.SuspendLayout();
      this.menuStrip1.SuspendLayout();
      this.panel1.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.displayPanel)).BeginInit();
      this.SuspendLayout();
      // 
      // statusStrip1
      // 
      this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel1,
            this.toolStripStatusLabel2});
      this.statusStrip1.Location = new System.Drawing.Point(0, 443);
      this.statusStrip1.Name = "statusStrip1";
      this.statusStrip1.Size = new System.Drawing.Size(623, 22);
      this.statusStrip1.TabIndex = 2;
      this.statusStrip1.Text = "statusStrip1";
      // 
      // toolStripStatusLabel1
      // 
      this.toolStripStatusLabel1.AutoSize = false;
      this.toolStripStatusLabel1.BorderSides = ((System.Windows.Forms.ToolStripStatusLabelBorderSides)((((System.Windows.Forms.ToolStripStatusLabelBorderSides.Left | System.Windows.Forms.ToolStripStatusLabelBorderSides.Top) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Right) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Bottom)));
      this.toolStripStatusLabel1.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenOuter;
      this.toolStripStatusLabel1.Margin = new System.Windows.Forms.Padding(0, 3, 0, 0);
      this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
      this.toolStripStatusLabel1.Size = new System.Drawing.Size(110, 19);
      this.toolStripStatusLabel1.Text = "  Press F1 for Help";
      this.toolStripStatusLabel1.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // toolStripStatusLabel2
      // 
      this.toolStripStatusLabel2.AutoSize = false;
      this.toolStripStatusLabel2.BorderSides = ((System.Windows.Forms.ToolStripStatusLabelBorderSides)((((System.Windows.Forms.ToolStripStatusLabelBorderSides.Left | System.Windows.Forms.ToolStripStatusLabelBorderSides.Top) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Right) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Bottom)));
      this.toolStripStatusLabel2.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenOuter;
      this.toolStripStatusLabel2.Margin = new System.Windows.Forms.Padding(0, 3, 0, 0);
      this.toolStripStatusLabel2.Name = "toolStripStatusLabel2";
      this.toolStripStatusLabel2.Size = new System.Drawing.Size(300, 19);
      this.toolStripStatusLabel2.Text = "  Copyright 2013 - Angus Johnson";
      this.toolStripStatusLabel2.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // menuStrip1
      // 
      this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.editToolStripMenuItem,
            this.helpToolStripMenuItem});
      this.menuStrip1.Location = new System.Drawing.Point(0, 0);
      this.menuStrip1.Name = "menuStrip1";
      this.menuStrip1.Size = new System.Drawing.Size(623, 24);
      this.menuStrip1.TabIndex = 4;
      this.menuStrip1.Text = "menuStrip1";
      // 
      // fileToolStripMenuItem
      // 
      this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mLoad,
            this.mSave,
            this.mSaveAs,
            this.toolStripMenuItem3,
            this.mExit});
      this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
      this.fileToolStripMenuItem.Size = new System.Drawing.Size(35, 20);
      this.fileToolStripMenuItem.Text = "&File";
      // 
      // mLoad
      // 
      this.mLoad.Name = "mLoad";
      this.mLoad.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
      this.mLoad.Size = new System.Drawing.Size(155, 22);
      this.mLoad.Text = "&Open ...";
      this.mLoad.Click += new System.EventHandler(this.mOpen_Click);
      // 
      // mSave
      // 
      this.mSave.Name = "mSave";
      this.mSave.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
      this.mSave.Size = new System.Drawing.Size(155, 22);
      this.mSave.Text = "&Save";
      this.mSave.Click += new System.EventHandler(this.mSave_Click);
      // 
      // mSaveAs
      // 
      this.mSaveAs.Name = "mSaveAs";
      this.mSaveAs.Size = new System.Drawing.Size(155, 22);
      this.mSaveAs.Text = "Save &As ...";
      this.mSaveAs.Click += new System.EventHandler(this.mSaveAs_Click);
      // 
      // toolStripMenuItem3
      // 
      this.toolStripMenuItem3.Name = "toolStripMenuItem3";
      this.toolStripMenuItem3.Size = new System.Drawing.Size(152, 6);
      // 
      // mExit
      // 
      this.mExit.Name = "mExit";
      this.mExit.Size = new System.Drawing.Size(155, 22);
      this.mExit.Text = "E&xit  ";
      this.mExit.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.mExit.Click += new System.EventHandler(this.mExit_Click);
      // 
      // editToolStripMenuItem
      // 
      this.editToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mIntersection,
            this.mUnion,
            this.mDifference,
            this.mXor,
            this.mNone,
            this.toolStripMenuItem1,
            this.mEvenOdd,
            this.mNonZero,
            this.toolStripMenuItem2,
            this.mNewPath,
            this.mUndo,
            this.mClear,
            this.toolStripMenuItem4,
            this.mZoomIn,
            this.mZoomOut});
      this.editToolStripMenuItem.Name = "editToolStripMenuItem";
      this.editToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
      this.editToolStripMenuItem.Text = "E&dit";
      // 
      // mIntersection
      // 
      this.mIntersection.Checked = true;
      this.mIntersection.CheckState = System.Windows.Forms.CheckState.Checked;
      this.mIntersection.Name = "mIntersection";
      this.mIntersection.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.I)));
      this.mIntersection.Size = new System.Drawing.Size(202, 22);
      this.mIntersection.Text = "&Intersection";
      this.mIntersection.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.mIntersection.Click += new System.EventHandler(this.mClipType_Click);
      // 
      // mUnion
      // 
      this.mUnion.Name = "mUnion";
      this.mUnion.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.U)));
      this.mUnion.Size = new System.Drawing.Size(202, 22);
      this.mUnion.Text = "&Union";
      this.mUnion.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.mUnion.Click += new System.EventHandler(this.mClipType_Click);
      // 
      // mDifference
      // 
      this.mDifference.Name = "mDifference";
      this.mDifference.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.D)));
      this.mDifference.Size = new System.Drawing.Size(202, 22);
      this.mDifference.Text = "&Difference";
      this.mDifference.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.mDifference.Click += new System.EventHandler(this.mClipType_Click);
      // 
      // mXor
      // 
      this.mXor.Name = "mXor";
      this.mXor.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.X)));
      this.mXor.Size = new System.Drawing.Size(202, 22);
      this.mXor.Text = "&XOr";
      this.mXor.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.mXor.Click += new System.EventHandler(this.mClipType_Click);
      // 
      // mNone
      // 
      this.mNone.Name = "mNone";
      this.mNone.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Q)));
      this.mNone.Size = new System.Drawing.Size(202, 22);
      this.mNone.Text = "Non&e";
      this.mNone.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.mNone.Click += new System.EventHandler(this.mClipType_Click);
      // 
      // toolStripMenuItem1
      // 
      this.toolStripMenuItem1.Name = "toolStripMenuItem1";
      this.toolStripMenuItem1.Size = new System.Drawing.Size(199, 6);
      // 
      // mEvenOdd
      // 
      this.mEvenOdd.Checked = true;
      this.mEvenOdd.CheckState = System.Windows.Forms.CheckState.Checked;
      this.mEvenOdd.Name = "mEvenOdd";
      this.mEvenOdd.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.E)));
      this.mEvenOdd.Size = new System.Drawing.Size(202, 22);
      this.mEvenOdd.Text = "&EvenOdd";
      this.mEvenOdd.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.mEvenOdd.Click += new System.EventHandler(this.mFillType_Clicked);
      // 
      // mNonZero
      // 
      this.mNonZero.Name = "mNonZero";
      this.mNonZero.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.N)));
      this.mNonZero.Size = new System.Drawing.Size(202, 22);
      this.mNonZero.Text = "&NonZero";
      this.mNonZero.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.mNonZero.Click += new System.EventHandler(this.mFillType_Clicked);
      // 
      // toolStripMenuItem2
      // 
      this.toolStripMenuItem2.Name = "toolStripMenuItem2";
      this.toolStripMenuItem2.Size = new System.Drawing.Size(199, 6);
      // 
      // mNewPath
      // 
      this.mNewPath.Name = "mNewPath";
      this.mNewPath.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.P)));
      this.mNewPath.Size = new System.Drawing.Size(202, 22);
      this.mNewPath.Text = "New &Path";
      this.mNewPath.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.mNewPath.Click += new System.EventHandler(this.mNewPath_Click);
      // 
      // mUndo
      // 
      this.mUndo.Name = "mUndo";
      this.mUndo.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Z)));
      this.mUndo.Size = new System.Drawing.Size(202, 22);
      this.mUndo.Text = "Und&o";
      this.mUndo.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.mUndo.Click += new System.EventHandler(this.mUndo_Click);
      // 
      // mClear
      // 
      this.mClear.Name = "mClear";
      this.mClear.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.C)));
      this.mClear.Size = new System.Drawing.Size(202, 22);
      this.mClear.Text = "&Clear";
      this.mClear.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.mClear.Click += new System.EventHandler(this.mClear_Click);
      // 
      // toolStripMenuItem4
      // 
      this.toolStripMenuItem4.Name = "toolStripMenuItem4";
      this.toolStripMenuItem4.Size = new System.Drawing.Size(199, 6);
      // 
      // mZoomIn
      // 
      this.mZoomIn.Name = "mZoomIn";
      this.mZoomIn.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Oemplus)));
      this.mZoomIn.Size = new System.Drawing.Size(202, 22);
      this.mZoomIn.Text = "Zoom In";
      this.mZoomIn.Click += new System.EventHandler(this.mZoomIn_Click);
      // 
      // mZoomOut
      // 
      this.mZoomOut.Name = "mZoomOut";
      this.mZoomOut.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.OemMinus)));
      this.mZoomOut.Size = new System.Drawing.Size(202, 22);
      this.mZoomOut.Text = "Zoom Out";
      this.mZoomOut.Click += new System.EventHandler(this.mZoomOut_Click);
      // 
      // helpToolStripMenuItem
      // 
      this.helpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.quickTipsToolStripMenuItem});
      this.helpToolStripMenuItem.Name = "helpToolStripMenuItem";
      this.helpToolStripMenuItem.Size = new System.Drawing.Size(40, 20);
      this.helpToolStripMenuItem.Text = "&Help";
      // 
      // quickTipsToolStripMenuItem
      // 
      this.quickTipsToolStripMenuItem.Name = "quickTipsToolStripMenuItem";
      this.quickTipsToolStripMenuItem.ShortcutKeys = System.Windows.Forms.Keys.F1;
      this.quickTipsToolStripMenuItem.Size = new System.Drawing.Size(141, 22);
      this.quickTipsToolStripMenuItem.Text = "&Quick Tips";
      this.quickTipsToolStripMenuItem.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.quickTipsToolStripMenuItem.Click += new System.EventHandler(this.quickTipsToolStripMenuItem_Click);
      // 
      // panel1
      // 
      this.panel1.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
      this.panel1.Controls.Add(this.cbShowCoords);
      this.panel1.Controls.Add(this.cbSubjClosed);
      this.panel1.Controls.Add(this.rbClipPoly);
      this.panel1.Controls.Add(this.rbSubjEllipses);
      this.panel1.Controls.Add(this.rbSubjArc);
      this.panel1.Controls.Add(this.rbSubjQBezier);
      this.panel1.Controls.Add(this.rbSubjCBezier);
      this.panel1.Controls.Add(this.rbSubjLine);
      this.panel1.Controls.Add(this.groupBox2);
      this.panel1.Controls.Add(this.cbShowCtrls);
      this.panel1.Controls.Add(this.cbReconstCurve);
      this.panel1.Controls.Add(this.bNewPath);
      this.panel1.Controls.Add(this.groupBox1);
      this.panel1.Dock = System.Windows.Forms.DockStyle.Left;
      this.panel1.Location = new System.Drawing.Point(0, 24);
      this.panel1.Name = "panel1";
      this.panel1.Size = new System.Drawing.Size(159, 419);
      this.panel1.TabIndex = 1;
      // 
      // cbShowCoords
      // 
      this.cbShowCoords.AutoSize = true;
      this.cbShowCoords.Location = new System.Drawing.Point(12, 375);
      this.cbShowCoords.Name = "cbShowCoords";
      this.cbShowCoords.Size = new System.Drawing.Size(119, 17);
      this.cbShowCoords.TabIndex = 41;
      this.cbShowCoords.Text = "Display Coord&inates";
      this.cbShowCoords.UseVisualStyleBackColor = true;
      this.cbShowCoords.Click += new System.EventHandler(this.cbShowCoords_Click);
      // 
      // cbSubjClosed
      // 
      this.cbSubjClosed.AutoSize = true;
      this.cbSubjClosed.Location = new System.Drawing.Point(26, 40);
      this.cbSubjClosed.Name = "cbSubjClosed";
      this.cbSubjClosed.Size = new System.Drawing.Size(58, 17);
      this.cbSubjClosed.TabIndex = 39;
      this.cbSubjClosed.Text = "Cl&osed";
      this.cbSubjClosed.UseVisualStyleBackColor = true;
      this.cbSubjClosed.Click += new System.EventHandler(this.cbSubjClosed_Click);
      // 
      // rbClipPoly
      // 
      this.rbClipPoly.AutoSize = true;
      this.rbClipPoly.Location = new System.Drawing.Point(26, 218);
      this.rbClipPoly.Name = "rbClipPoly";
      this.rbClipPoly.Size = new System.Drawing.Size(85, 17);
      this.rbClipPoly.TabIndex = 20;
      this.rbClipPoly.Text = "Li&ne (closed)";
      this.rbClipPoly.UseVisualStyleBackColor = true;
      this.rbClipPoly.Click += new System.EventHandler(this.rbAdd_Click);
      // 
      // rbSubjEllipses
      // 
      this.rbSubjEllipses.AutoSize = true;
      this.rbSubjEllipses.Enabled = false;
      this.rbSubjEllipses.Location = new System.Drawing.Point(26, 151);
      this.rbSubjEllipses.Name = "rbSubjEllipses";
      this.rbSubjEllipses.Size = new System.Drawing.Size(82, 17);
      this.rbSubjEllipses.TabIndex = 19;
      this.rbSubjEllipses.Text = "&Elliptical Arc";
      this.rbSubjEllipses.UseVisualStyleBackColor = true;
      this.rbSubjEllipses.Click += new System.EventHandler(this.rbAdd_Click);
      // 
      // rbSubjArc
      // 
      this.rbSubjArc.AutoSize = true;
      this.rbSubjArc.Location = new System.Drawing.Point(26, 129);
      this.rbSubjArc.Name = "rbSubjArc";
      this.rbSubjArc.Size = new System.Drawing.Size(41, 17);
      this.rbSubjArc.TabIndex = 18;
      this.rbSubjArc.Text = "&Arc";
      this.rbSubjArc.UseVisualStyleBackColor = true;
      this.rbSubjArc.Click += new System.EventHandler(this.rbAdd_Click);
      // 
      // rbSubjQBezier
      // 
      this.rbSubjQBezier.AutoSize = true;
      this.rbSubjQBezier.Location = new System.Drawing.Point(26, 107);
      this.rbSubjQBezier.Name = "rbSubjQBezier";
      this.rbSubjQBezier.Size = new System.Drawing.Size(62, 17);
      this.rbSubjQBezier.TabIndex = 17;
      this.rbSubjQBezier.Text = "&QBezier";
      this.rbSubjQBezier.UseVisualStyleBackColor = true;
      this.rbSubjQBezier.Click += new System.EventHandler(this.rbAdd_Click);
      // 
      // rbSubjCBezier
      // 
      this.rbSubjCBezier.AutoSize = true;
      this.rbSubjCBezier.Location = new System.Drawing.Point(25, 85);
      this.rbSubjCBezier.Name = "rbSubjCBezier";
      this.rbSubjCBezier.Size = new System.Drawing.Size(61, 17);
      this.rbSubjCBezier.TabIndex = 16;
      this.rbSubjCBezier.Text = "&CBezier";
      this.rbSubjCBezier.UseVisualStyleBackColor = true;
      this.rbSubjCBezier.Click += new System.EventHandler(this.rbAdd_Click);
      // 
      // rbSubjLine
      // 
      this.rbSubjLine.AutoSize = true;
      this.rbSubjLine.Checked = true;
      this.rbSubjLine.Location = new System.Drawing.Point(25, 63);
      this.rbSubjLine.Name = "rbSubjLine";
      this.rbSubjLine.Size = new System.Drawing.Size(45, 17);
      this.rbSubjLine.TabIndex = 15;
      this.rbSubjLine.TabStop = true;
      this.rbSubjLine.Text = "&Line";
      this.rbSubjLine.UseVisualStyleBackColor = true;
      this.rbSubjLine.Click += new System.EventHandler(this.rbAdd_Click);
      // 
      // groupBox2
      // 
      this.groupBox2.Location = new System.Drawing.Point(12, 194);
      this.groupBox2.Name = "groupBox2";
      this.groupBox2.Size = new System.Drawing.Size(128, 61);
      this.groupBox2.TabIndex = 6;
      this.groupBox2.TabStop = false;
      this.groupBox2.Text = " Clip Paths ";
      // 
      // cbShowCtrls
      // 
      this.cbShowCtrls.AutoSize = true;
      this.cbShowCtrls.Enabled = false;
      this.cbShowCtrls.Location = new System.Drawing.Point(12, 337);
      this.cbShowCtrls.Name = "cbShowCtrls";
      this.cbShowCtrls.Size = new System.Drawing.Size(103, 17);
      this.cbShowCtrls.TabIndex = 36;
      this.cbShowCtrls.Text = "Show C&trl Points";
      this.cbShowCtrls.UseVisualStyleBackColor = true;
      this.cbShowCtrls.Click += new System.EventHandler(this.cbShowCtrls_Click);
      // 
      // cbReconstCurve
      // 
      this.cbReconstCurve.AutoSize = true;
      this.cbReconstCurve.Location = new System.Drawing.Point(12, 306);
      this.cbReconstCurve.Name = "cbReconstCurve";
      this.cbReconstCurve.Size = new System.Drawing.Size(132, 17);
      this.cbReconstCurve.TabIndex = 35;
      this.cbReconstCurve.Text = "Reconstructed Curves";
      this.cbReconstCurve.UseVisualStyleBackColor = true;
      this.cbReconstCurve.Click += new System.EventHandler(this.cbReconstCurve_Click);
      // 
      // bNewPath
      // 
      this.bNewPath.Enabled = false;
      this.bNewPath.Location = new System.Drawing.Point(12, 271);
      this.bNewPath.Name = "bNewPath";
      this.bNewPath.Size = new System.Drawing.Size(130, 28);
      this.bNewPath.TabIndex = 30;
      this.bNewPath.Text = "New &Path";
      this.bNewPath.UseVisualStyleBackColor = true;
      this.bNewPath.Click += new System.EventHandler(this.mNewPath_Click);
      // 
      // groupBox1
      // 
      this.groupBox1.Location = new System.Drawing.Point(12, 17);
      this.groupBox1.Name = "groupBox1";
      this.groupBox1.Size = new System.Drawing.Size(130, 167);
      this.groupBox1.TabIndex = 40;
      this.groupBox1.TabStop = false;
      this.groupBox1.Text = " Subject Paths ";
      // 
      // openFileDialog1
      // 
      this.openFileDialog1.DefaultExt = "txt";
      this.openFileDialog1.Filter = "Text Files | *.txt";
      this.openFileDialog1.Title = "Open Custom Data File ...";
      // 
      // saveFileDialog1
      // 
      this.saveFileDialog1.DefaultExt = "txt";
      this.saveFileDialog1.Filter = "Text Files | *.txt";
      this.saveFileDialog1.Title = "Save Custom Data ...";
      // 
      // displayPanel
      // 
      this.displayPanel.BackColor = System.Drawing.SystemColors.Window;
      this.displayPanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
      this.displayPanel.Dock = System.Windows.Forms.DockStyle.Fill;
      this.displayPanel.Location = new System.Drawing.Point(159, 24);
      this.displayPanel.Name = "displayPanel";
      this.displayPanel.Size = new System.Drawing.Size(464, 419);
      this.displayPanel.TabIndex = 5;
      this.displayPanel.TabStop = false;
      this.displayPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.DisplayPanel_Paint);
      this.displayPanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.DisplayPanel_MouseDown);
      this.displayPanel.MouseMove += new System.Windows.Forms.MouseEventHandler(this.DisplayPanel_MouseMove);
      this.displayPanel.MouseUp += new System.Windows.Forms.MouseEventHandler(this.DisplayPanel_MouseUp);
      // 
      // MainForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(623, 465);
      this.Controls.Add(this.displayPanel);
      this.Controls.Add(this.panel1);
      this.Controls.Add(this.menuStrip1);
      this.Controls.Add(this.statusStrip1);
      this.KeyPreview = true;
      this.Name = "MainForm";
      this.Text = "Clipper Curves  Demo";
      this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.MainForm_FormClosed);
      this.Load += new System.EventHandler(this.MainForm_Load);
      this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.MainForm_KeyDown);
      this.Resize += new System.EventHandler(this.MainForm_Resize);
      this.statusStrip1.ResumeLayout(false);
      this.statusStrip1.PerformLayout();
      this.menuStrip1.ResumeLayout(false);
      this.menuStrip1.PerformLayout();
      this.panel1.ResumeLayout(false);
      this.panel1.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.displayPanel)).EndInit();
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.StatusStrip statusStrip1;
    private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
    private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel2;
    private System.Windows.Forms.MenuStrip menuStrip1;
    private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
    private System.Windows.Forms.ToolStripMenuItem mExit;
    private System.Windows.Forms.ToolStripMenuItem editToolStripMenuItem;
    private System.Windows.Forms.ToolStripMenuItem mIntersection;
    private System.Windows.Forms.ToolStripMenuItem mUnion;
    private System.Windows.Forms.ToolStripMenuItem mDifference;
    private System.Windows.Forms.ToolStripMenuItem mXor;
    private System.Windows.Forms.ToolStripMenuItem mNone;
    private System.Windows.Forms.ToolStripSeparator toolStripMenuItem1;
    private System.Windows.Forms.ToolStripMenuItem mEvenOdd;
    private System.Windows.Forms.ToolStripMenuItem mNonZero;
    private System.Windows.Forms.ToolStripSeparator toolStripMenuItem2;
    private System.Windows.Forms.ToolStripMenuItem mNewPath;
    private System.Windows.Forms.ToolStripMenuItem mUndo;
    private System.Windows.Forms.ToolStripMenuItem mClear;
    private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
    private System.Windows.Forms.ToolStripMenuItem quickTipsToolStripMenuItem;
    private System.Windows.Forms.Panel panel1;
    private System.Windows.Forms.Button bNewPath;
    private System.Windows.Forms.CheckBox cbReconstCurve;
    private System.Windows.Forms.CheckBox cbShowCtrls;
    private System.Windows.Forms.ToolStripMenuItem mSaveAs;
    private System.Windows.Forms.ToolStripMenuItem mLoad;
    private System.Windows.Forms.ToolStripSeparator toolStripMenuItem3;
    private System.Windows.Forms.RadioButton rbSubjEllipses; 
    private System.Windows.Forms.RadioButton rbSubjArc;
    private System.Windows.Forms.RadioButton rbSubjQBezier;
    private System.Windows.Forms.RadioButton rbSubjCBezier;
    private System.Windows.Forms.RadioButton rbSubjLine;
    private System.Windows.Forms.GroupBox groupBox2;
    private System.Windows.Forms.RadioButton rbClipPoly;
    private System.Windows.Forms.CheckBox cbSubjClosed;
    private System.Windows.Forms.GroupBox groupBox1;
    private System.Windows.Forms.ToolStripSeparator toolStripMenuItem4;
    private System.Windows.Forms.ToolStripMenuItem mZoomIn;
    private System.Windows.Forms.ToolStripMenuItem mZoomOut;
    private System.Windows.Forms.OpenFileDialog openFileDialog1;
    private System.Windows.Forms.SaveFileDialog saveFileDialog1;
    private System.Windows.Forms.CheckBox cbShowCoords;
    private System.Windows.Forms.ToolStripMenuItem mSave;
    private System.Windows.Forms.PictureBox displayPanel;
  }
}

