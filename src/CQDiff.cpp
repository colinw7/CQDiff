#include <CQDiff.h>
#include <CQToolBar.h>
#include <CQMenu.h>
#include <CFile.h>
#include <CCommand.h>
#include <CStrUtil.h>
#include <CStrParse.h>

#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollBar>
#include <QLabel>
#include <QStatusBar>
#include <QPainter>

#include <cmath>

#include <svg/first_diff_svg.h>
#include <svg/last_diff_svg.h>
#include <svg/next_diff_svg.h>
#include <svg/prev_diff_svg.h>
#include <svg/reload_svg.h>

CQDiff::
CQDiff() :
 CQMainWindow("CQDiff")
{
  setObjectName("diff");

  connect(this, SIGNAL(changeNumChanged()), this, SLOT(scrollToChange()));
}

CQDiff::
~CQDiff()
{
}

void
CQDiff::
setFiles(const std::string &src, const std::string &dst)
{
  addSrc(src);
  addDst(dst);

  exec();

  ledit_->update();
  redit_->update();
}

void
CQDiff::
addSrc(const std::string &src)
{
  llabel_->setText(src.c_str());

  ledit_->setFileName(src.c_str());
}

void
CQDiff::
addDst(const std::string &dst)
{
  rlabel_->setText(dst.c_str());

  redit_->setFileName(dst.c_str());
}

void
CQDiff::
exec()
{
  ledit_->reset();
  redit_->reset();

  changeNum_ = 0;

  //---

  // run diff command
  std::vector<std::string> args;

  if (isIgnoreWhiteSpace())
    args.push_back("-bw");

  args.push_back(ledit_->getFileName().toStdString());
  args.push_back(redit_->getFileName().toStdString());

  CCommand cmd("diff", "diff", args);

  std::string result;

  cmd.addStringDest(result);

  cmd.start();

  cmd.wait();

  //---

  std::vector<std::string> lines;

  CStrUtil::addLines(result, lines);

  for (const auto &line : lines) {
    uint len = line.size();

    if (len == 0 || ! isdigit(line[0]))
      continue;

    parseChange(line);
  }

  diffCombo_->load();
}

bool
CQDiff::
parseChange(const std::string &line)
{
  CStrParse parse(line);

  int lstart = 0;

  if (! parse.readInteger(&lstart))
    return false;

  int lend = lstart;

  if (parse.isChar(',')) {
    parse.skipChar();

    if (! parse.readInteger(&lend))
      return false;
  }

  char c;

  if (! parse.readChar(&c))
    return false;

  int rstart = 0;

  if (! parse.readInteger(&rstart))
    return false;

  int rend = rstart;

  if (parse.isChar(',')) {
    parse.skipChar();

    if (! parse.readInteger(&rend))
      return false;
  }

  uint num = changes_.size() + 1;

  CQDiffChange change(num, c, lstart, lend, rstart, rend);

  change.setString(line);

  changes_.push_back(change);

  ledit_->addChange(num, c         , lstart, lend);
  redit_->addChange(num, toupper(c), rstart, rend);

  return true;
}

QWidget *
CQDiff::
createCentralWidget()
{
  frame_ = new QWidget(this);

  frame_->setObjectName("frame");

  QGridLayout *layout = new QGridLayout(frame_);

  llabel_ = new QLabel;
  rlabel_ = new QLabel;

  llabel_->setAlignment(Qt::AlignHCenter);
  rlabel_->setAlignment(Qt::AlignHCenter);

  llabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  rlabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  layout->addWidget(llabel_, 0, 0);
  layout->addWidget(rlabel_, 0, 2);

  ledit_ = new CQFileEdit(this, CSIDE_TYPE_LEFT);
  vbar_  = new CQDiffBar (this);
  redit_ = new CQFileEdit(this, CSIDE_TYPE_RIGHT);

  layout->addWidget(ledit_, 1, 0);
  layout->addWidget(vbar_ , 1, 1);
  layout->addWidget(redit_, 1, 2);

  connect(vbar_, SIGNAL(valueChanged(int)), this, SLOT(scrollSlot(int)));

  return frame_;
}

void
CQDiff::
createMenus()
{
  fileMenu_ = new CQMenu(this, "File");

  CQMenuItem *quitItem = new CQMenuItem(fileMenu_, "Quit");

  quitItem->setShortcut("Ctrl+Q");
  quitItem->setStatusTip("Quit the application");

  connect(quitItem->getAction(), SIGNAL(triggered()), this, SLOT(close()));

  //--------

  diffMenu_ = new CQMenu(this, "Diff");

  firstDiffItem_ = new CQMenuItem(diffMenu_, "First Diff");

  firstDiffItem_->setStatusTip("Move to first difference");
  firstDiffItem_->setIcon(CQPixmapCacheInst->getIcon("FIRST_DIFF"));

  firstDiffItem_->connect(this, SLOT(firstDiffSlot()));

  lastDiffItem_ = new CQMenuItem(diffMenu_, "Last Diff");

  lastDiffItem_->setStatusTip("Move to last difference");
  lastDiffItem_->setIcon(CQPixmapCacheInst->getIcon("LAST_DIFF"));

  lastDiffItem_->connect(this, SLOT(lastDiffSlot()));

  nextDiffItem_ = new CQMenuItem(diffMenu_, "Next Diff");

  nextDiffItem_->setStatusTip("Move to next difference");
  nextDiffItem_->setIcon(CQPixmapCacheInst->getIcon("NEXT_DIFF"));

  nextDiffItem_->connect(this, SLOT(nextDiffSlot()));

  prevDiffItem_ = new CQMenuItem(diffMenu_, "Prev Diff");

  prevDiffItem_->setStatusTip("Move to previous difference");
  prevDiffItem_->setIcon(CQPixmapCacheInst->getIcon("PREV_DIFF"));

  prevDiffItem_->connect(this, SLOT(prevDiffSlot()));

  whiteSpaceItem_ = new CQMenuItem(diffMenu_, "Ignore White Space", CQMenuItem::CHECKABLE);

  prevDiffItem_->setStatusTip("Ignore White Space");

  whiteSpaceItem_->connect(this, SLOT(whiteSpaceSlot(bool)));

  recompItem_ = new CQMenuItem(diffMenu_, "Recompute Diff");

  recompItem_->setStatusTip("Recompute differences");
  recompItem_->setIcon(CQPixmapCacheInst->getIcon("RELOAD"));

  recompItem_->connect(this, SLOT(recomputeSlot()));

  //--------

  viewMenu_ = new CQMenu(this, "View");

  showLineNumbersItem_ = new CQMenuItem(viewMenu_, "Show Line Numbers", CQMenuItem::CHECKED);

  showLineNumbersItem_->setStatusTip("Show Line Numbers");

  showLineNumbersItem_->connect(this, SLOT(showLineNumbersSlot(bool)));

  //----

  helpMenu_ = new CQMenu(this, "Help");

  CQMenuItem *aboutItem = new CQMenuItem(helpMenu_, "About");

  aboutItem->setStatusTip("Show the application's About box");

  aboutItem->connect(this, SLOT(aboutSlot()));
}

void
CQDiff::
createToolBars()
{
  diffToolBar_ = new CQToolBar(this, "Diff");

  diffCombo_ = new CQDiffCombo(this);

  diffToolBar_->addWidget(diffCombo_);

  diffToolBar_->addItem(firstDiffItem_);
  diffToolBar_->addItem(lastDiffItem_);
  diffToolBar_->addItem(nextDiffItem_);
  diffToolBar_->addItem(prevDiffItem_);
  diffToolBar_->addItem(recompItem_);
}

void
CQDiff::
createStatusBar()
{
  statusBar()->setObjectName("status");

  lslabel_ = new QLabel; lslabel_->setObjectName("lslabel");
  rslabel_ = new QLabel; rslabel_->setObjectName("rslabel");

  statusBar()->addWidget(lslabel_);
  statusBar()->addWidget(rslabel_);
}

void
CQDiff::
firstDiffSlot()
{
  setChangeNum(0);
}

void
CQDiff::
lastDiffSlot()
{
  setChangeNum(changes_.size() - 1);
}

void
CQDiff::
nextDiffSlot()
{
  if (changeNum_ < int(changes_.size() - 1))
    setChangeNum(changeNum_ + 1);
}

void
CQDiff::
prevDiffSlot()
{
  if (changeNum_ > 0)
    setChangeNum(changeNum_ - 1);
}

void
CQDiff::
whiteSpaceSlot(bool b)
{
  setIgnoreWhiteSpace(b);

  recomputeSlot();
}

void
CQDiff::
recomputeSlot()
{
  ledit_->setFileName(ledit_->getFileName());
  redit_->setFileName(redit_->getFileName());

  exec();

  ledit_->update();
  redit_->update();
}

void
CQDiff::
showLineNumbersSlot(bool b)
{
  ledit_->setShowNumbers(b);
  redit_->setShowNumbers(b);

  ledit_->update();
  redit_->update();
}

void
CQDiff::
aboutSlot()
{
}

void
CQDiff::
setChangeNum(int changeNum)
{
  changeNum_ = changeNum;

  emit changeNumChanged();
}

void
CQDiff::
scrollToChange()
{
  if (changeNum_ < 0 || changeNum_ >= getNumChanges())
    return;

  const CQDiffChange &change = getChange(changeNum_);

  int loffset = change.getOffset(CSIDE_TYPE_LEFT);
  int roffset = change.getOffset(CSIDE_TYPE_RIGHT);

  int offset = 0;

  if      (loffset >= 0 && roffset >= 0)
    offset = (loffset + roffset)/2;
  else if (loffset >= 0)
    offset = loffset;
  else
    offset = roffset;

  offset -= vbar_->pageStep()/3;

  //CQFileEdit *ledit = getEdit(CSIDE_TYPE_LEFT);
  //CQFileEdit *redit = getEdit(CSIDE_TYPE_RIGHT);

  //ledit->getVBar()->setValue(offset);
  //redit->getVBar()->setValue(offset);

  vbar_->setValue(offset);

  firstDiffItem_->setEnabled(changeNum_ > 0);
  lastDiffItem_ ->setEnabled(changeNum_ < int(changes_.size()) - 1);
  nextDiffItem_ ->setEnabled(changeNum_ < int(changes_.size()) - 1);
  prevDiffItem_ ->setEnabled(changeNum_ > 0);

  lslabel_->setText(change.getString().c_str());
}

void
CQDiff::
setDataHeight(int dataHeight)
{
  int scrollHeight = vbar_->height();

  if (dataHeight == dataHeight_ && scrollHeight == scrollHeight_)
    return;

  scrollHeight_ = scrollHeight;
  dataHeight_   = dataHeight;

  updateVBar();
}

void
CQDiff::
updateVBar()
{
  int dy = std::max(0, dataHeight_ - scrollHeight_);

  vbar_->setPageStep(scrollHeight_);

  vbar_->setMinimum(0);
  vbar_->setMaximum(dy);

  vbar_->setSingleStep(scrollHeight_/10);

  vbar_->setValue(0);

  ledit_->updateScrollbars(dataHeight_);
  redit_->updateScrollbars(dataHeight_);
}

void
CQDiff::
scrollSlot(int y)
{
  ledit_->getVBar()->setValue(y);
  redit_->getVBar()->setValue(y);
}

QSize
CQDiff::
sizeHint() const
{
  QFontMetrics fm(font());

  int tw = fm.width("X");
  int th = fm.height();

  return QSize(160*tw, 70*th);
}

//-------

CQFileEdit::
CQFileEdit(CQDiff *diff, CSideType side, const QString &fileName) :
 diff_(diff), side_(side), fileName_(fileName), x_offset_(0), y_offset_(0)
{
  setObjectName("edit");

  QGridLayout *grid = new QGridLayout(this);
  grid->setMargin(0); grid->setSpacing(0);

  canvas_ = new CQFileEditCanvas(this);

  hbar_ = new QScrollBar(Qt::Horizontal); hbar_->setObjectName("hbar");
  vbar_ = new QScrollBar(Qt::Vertical  ); vbar_->setObjectName("vbar");

  if (side == CSIDE_TYPE_LEFT) {
    grid->addWidget(canvas_, 0, 1);
    grid->addWidget(vbar_  , 0, 0);
    grid->addWidget(hbar_  , 1, 1);
  }
  else {
    grid->addWidget(canvas_, 0, 0);
    grid->addWidget(vbar_  , 0, 1);
    grid->addWidget(hbar_  , 1, 0);
  }

  connect(vbar_, SIGNAL(valueChanged(int)), this, SLOT(vscrollSlot(int)));
  connect(hbar_, SIGNAL(valueChanged(int)), this, SLOT(hscrollSlot(int)));
}

void
CQFileEdit::
setFileName(const QString &fileName)
{
  fileName_ = fileName;

  lines_.clear();

  CFile file(fileName_.toStdString());

  if (! file.isRegular()) return;

  file.toLines(lines_);
}

void
CQFileEdit::
reset()
{
  changeMap_.clear();
}

void
CQFileEdit::
addChange(uint num, char c, int start, int end)
{
  changeMap_[start] = Change(num, c, start, end);
}

void
CQFileEdit::
hscrollSlot(int x)
{
  x_offset_ = -x;

  canvas_->update();
}

void
CQFileEdit::
vscrollSlot(int y)
{
  y_offset_ = -y;

  canvas_->update();
}

void
CQFileEdit::
draw(QPainter *p)
{
  QFont font = canvas_->font();

  QFontMetrics fm(font);

  int width  = canvas_->width ();
  int height = canvas_->height();

  charWidth_  = fm.averageCharWidth();
  charHeight_ = fm.height();
  charAscent_ = fm.ascent();

  int y1 = y_offset_;
  int y2 = y1 + charHeight_;

  char   change_c   = '\0';
  int    change_end = 0;
  int    change_len = 0;
  QColor change_bg;

  int num_lines = lines_.size();

  int         lfw = 0;
  std::string lfmt;

  if (isShowNumbers()) {
    int lw = log10(num_lines) + 1;

    lfmt = "%" + CStrUtil::toString(lw) + "d";

    lfw = lw*charWidth_ + 8;
  }

  int iw = charWidth_ + 8;

  for (int line_num = 1; line_num <= num_lines; ++line_num, y1 = y2, y2 = y1 + charHeight_) {
    // check if line if visible
    bool draw = (y2 >= 0 && y1 < height);

    //-----

    int x = x_offset_;

    if (draw) {
      // clear background
      p->fillRect(0, y1, width - x_offset_, charHeight_, QBrush(diff_->bgColor()));

      // draw line number if needed
      if (isShowNumbers()) {
        p->setPen(diff_->fgColor());

        std::string lstr = CStrUtil::strprintf(&lfmt, line_num);

        p->drawText(x, y1 + charAscent_, lstr.c_str());
      }
    }

    x += lfw;

    int pad = 0;

    // check for new change
    if (! change_c) {
      // find change for line
      auto pc = changeMap_.find(line_num);

      if (pc != changeMap_.end()) {
        const Change &change = (*pc).second;

        bool selected = (diff_->getChangeNum() == int(change.num - 1));

        // get delta change data (both files)
        CQDiffChange &dchange = diff_->getChange(change.num - 1);

        change_c   = dchange.getChar();
        change_end = dchange.getEnd(side_);
        change_len = dchange.getOtherLen(side_) - dchange.getLen(side_);
        change_bg  = diff_->getChangeColor(side_, change_c);

        if (selected && side_ == CSIDE_TYPE_LEFT)
          change_bg = diff_->selectedColor();

        if (change_len > 0)
          pad = change_len;

        // check this line part of change
        if (dchange.getLen(side_) > 0) {
          // fill background for change color
          if (draw)
            p->fillRect(x, y1, width - x_offset_, charHeight_, QBrush(change_bg));
        }
        else {
          change_c   = '\0';
          change_end = 0;
        }

        // update change y position
        dchange.setOffset(side_, y1 - y_offset_);
      }
    }
    // continuation of change
    else {
      // reset if done
      if (line_num > change_end) {
        change_c   = '\0';
        change_end = 0;
      }
      // fill background for change color
      else {
        if (draw)
          p->fillRect(x, y1, width - x_offset_, charHeight_, QBrush(change_bg));
      }
    }

    //---

    // draw change character
    if (draw) {
      if (change_c)
        p->drawText(x, y1 + charAscent_, QString(change_c));
      else
        p->drawText(x, y1 + charAscent_, " ");
    }

    x += iw;

    //---

    // draw line
    if (draw) {
      const std::string &line = lines_[line_num - 1];

      p->drawText(x, y1 + charAscent_, line.c_str());
    }

    //---

    for (int j = 0; j < pad; ++j) {
      x = x_offset_;

      y1 = y2;
      y2 = y1 + charHeight_;

      bool draw = (y2 >= 0 && y1 < height);

      if (draw) {
        p->fillRect(x + lfw, y1, width - x_offset_, charHeight_, QBrush(change_bg));

        if (change_c)
          p->drawText(x + lfw, y1 + charAscent_, QString(change_c));
        else
          p->drawText(x + lfw, y1 + charAscent_, " ");
      }
    }
  }

  //---

  // draw border lines
  p->setPen(diff_->borderColor());

  int x = x_offset_ + lfw - 4;

  p->drawLine(x, 0, x, height - 1);

  x = x_offset_ + lfw + iw - 4;

  p->drawLine(x, 0, x, height - 1);

  //---

  if (side_ == CSIDE_TYPE_LEFT)
    diff_->setDataHeight(y2 - y_offset_);
}

void
CQFileEdit::
updateScrollbars(int height)
{
  int xsize = canvas_->width ();
  int ysize = canvas_->height();

  QFont font = canvas_->font();

  QFontMetrics fm(font);

  charWidth_ = fm.averageCharWidth();

  uint num_lines = lines_.size();

  int width = 0;

  for (const auto &line : lines_)
    width = std::max(width, fm.width(line.c_str()));

  int lw = log10(num_lines) + 1;

  int lfw = lw*charWidth_ + 8;
  int iw  = charWidth_ + 8;

  width += lfw + iw;

  int dx = std::max(0, width  - xsize);
  int dy = std::max(0, height - ysize);

  hbar_->setPageStep(xsize);

  hbar_->setMinimum(0);
  hbar_->setMaximum(dx);

  hbar_->setSingleStep(charWidth_);

  hbar_->setValue(-x_offset_);

  vbar_->setPageStep(ysize);

  vbar_->setMinimum(0);
  vbar_->setMaximum(dy);

  vbar_->setSingleStep(fm.height());

  vbar_->setValue(-y_offset_);
}

//------

CQFileEditCanvas::
CQFileEditCanvas(CQFileEdit *edit) :
 QWidget(nullptr), edit_(edit)
{
  setObjectName("canvas");

  setFocusPolicy(Qt::StrongFocus);

  setFont(QFont("Courier", 14));
}

void
CQFileEditCanvas::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  edit_->draw(&painter);
}

//------

CQDiffCombo::
CQDiffCombo(CQDiff *diff) :
 QComboBox(nullptr), diff_(diff)
{
  setObjectName("diffCombo");

  connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(changedSlot(int)));

  connect(diff_, SIGNAL(changeNumChanged()), this, SLOT(updateChangeSlot()));
}

void
CQDiffCombo::
changedSlot(int ind)
{
  diff_->setChangeNum(ind);
}

void
CQDiffCombo::
updateChangeSlot()
{
  int changeNum = diff_->getChangeNum();

  if (changeNum != currentIndex())
    setCurrentIndex(changeNum);
}

void
CQDiffCombo::
load()
{
  QComboBox::clear();

  for (const auto &change : diff_->getChanges()) {
    std::string str = CStrUtil::strprintf("%d: %s", change.getNum(), change.getString().c_str());

    addItem(str.c_str());
  }
}

//-------

CQDiffBar::
CQDiffBar(CQDiff *diff) :
 QScrollBar(Qt::Vertical), diff_(diff)
{
  setObjectName("diffBar");
}

void
CQDiffBar::
paintEvent(QPaintEvent *e)
{
  QScrollBar::paintEvent(e);

  QStyleOptionSlider opt;

  initStyleOption(&opt);

  opt.subControls = QStyle::SC_All;

  QRect ulrect = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                         QStyle::SC_ScrollBarAddLine, this);
  QRect dlrect = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                         QStyle::SC_ScrollBarSubLine, this);
//QRect uprect = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
//                                       QStyle::SC_ScrollBarAddPage, this);
//QRect dprect = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
//                                       QStyle::SC_ScrollBarSubPage, this);
//QRect srect  = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
//                                       QStyle::SC_ScrollBarSlider, this);

//int sm = style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarSpacing, &opt, this);
  int sm = 2;

  int us = ulrect.height();
  int ds = dlrect.height();

  QPainter painter(this);

  CSideType side = CSIDE_TYPE_LEFT;

  CQFileEdit *edit = diff_->getEdit(side);

  int sheight = height() - us - ds;
  int smax    = maximum() + pageStep();

  double scale = (smax > 0 ? (1.0*sheight)/smax : 1);

  int w = width();

  for (const auto &change : diff_->getChanges()) {
    double y1 = scale*(us + change.getOffset(side));
    double y2 = y1 + scale*change.getMaxLen()*edit->charHeight();

    QColor c = diff_->getChangeColor(side, change.getChar());

    painter.fillRect(QRectF(QPointF(sm, y1), QSizeF(w - 2*sm, y2 - y1 + 1)), c);
  }
}
