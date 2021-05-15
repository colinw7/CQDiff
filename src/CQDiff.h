#ifndef CQDiff_H
#define CQDiff_H

#include <CSideType.h>
#include <CQMainWindow.h>

#include <QComboBox>
#include <QScrollBar>
#include <map>
#include <cassert>

class CQDiff;
class CQFileEdit;
class CQFileEditCanvas;

class CQMenu;
class CQMenuItem;
class CQToolBar;

class QScrollBar;
class QPainter;
class QLabel;

//------

class CQDiffChange {
 public:
  CQDiffChange(uint num=0, char c=0, int lstart=0, int lend=0, int rstart=0, int rend=0) :
   num_(num), c_(c), lstart_(lstart), lend_(lend), rstart_(rstart), rend_(rend) {
    llen_ = lend_ - lstart_ + 1;
    rlen_ = rend_ - rstart_ + 1;

    if      (c == 'a') { llen_ = 0; }
    else if (c == 'd') { rlen_ = 0; }
  }

  uint getNum () const { return num_; }
  char getChar() const { return c_; }

  int getStart(CSideType side) const {
    return (side == CSIDE_TYPE_LEFT ? lstart_ : rstart_);
  }

  int getEnd(CSideType side) const {
    return (side == CSIDE_TYPE_LEFT ? lend_ : rend_);
  }

  int getLen(CSideType side) const {
    return (side == CSIDE_TYPE_LEFT ? llen_ : rlen_);
  }

  int getOtherLen(CSideType side) const {
    return (side == CSIDE_TYPE_LEFT ? rlen_ : llen_);
  }

  int getMaxLen() const {
    return std::max(llen_, rlen_);
  }

  void setString(const std::string &str) { str_ = str; }

  const std::string &getString() const { return str_; }

  void setOffset(CSideType side, int offset) {
    if (side == CSIDE_TYPE_LEFT) loffset_ = offset;
    else                         roffset_ = offset;
  }

  int getOffset(CSideType side) const {
    return (side == CSIDE_TYPE_LEFT ? loffset_ : roffset_);
  }

  void resetOffset(CSideType side) {
    if (side == CSIDE_TYPE_LEFT) loffset_ = -1;
    else                         roffset_ = -1;
  }

 private:
  uint        num_ { 0 };
  char        c_ { '\0' };
  int         lstart_ { 0 }, lend_ { 0 }, llen_ { 0 };
  int         rstart_ { 0 }, rend_ { 0 }, rlen_ { 0 };
  std::string str_;
  int         loffset_ { -1 }, roffset_ { -1 };
};

//------

class CQDiffCombo : public QComboBox {
  Q_OBJECT

 public:
  CQDiffCombo(CQDiff *diff);

  void load();

 private slots:
  void changedSlot(int);
  void updateChangeSlot();

 private:
  CQDiff *diff_ { nullptr };
};

//------

class CQDiffBar : public QScrollBar {
  Q_OBJECT

 public:
  CQDiffBar(CQDiff *diff);

  void paintEvent(QPaintEvent *) override;

 private:
  CQDiff *diff_ { nullptr };
};

//------

class CQFileEditCanvas : public QWidget {
  Q_OBJECT

 public:
  CQFileEditCanvas(CQFileEdit *edit);

 private:
  void paintEvent(QPaintEvent *) override;

 private:
  CQFileEdit *edit_ { nullptr };
};

//------

class CQFileEdit : public QWidget {
  Q_OBJECT

  Q_PROPERTY(QString filename    READ getFileName   WRITE setFileName)
  Q_PROPERTY(bool    showNumbers READ isShowNumbers WRITE setShowNumbers)

 public:
  struct Change {
    uint num { 0 };
    char c { '\0' };
    int  start { 0 }, end { 0 };

    Change(uint num1=0, char c1=0, int start1=0, int end1=0) :
     num(num1), c(c1), start(start1), end(end1) {
    }
  };

 public:
  CQFileEdit(CQDiff *diff, CSideType side, const QString &fileName="");

  void setFileName(const QString &fileName);
  const QString &getFileName() const { return fileName_; }

  void addChange(uint num, char c, int start, int end);

  bool isShowNumbers() const { return showNumbers_; }
  void setShowNumbers(bool b) { showNumbers_ = b; }

  int charWidth () const { return charWidth_ ; }
  int charHeight() const { return charHeight_; }
  int charAscent() const { return charAscent_; }

  void reset();

  void draw(QPainter *p);

  void updateScrollbars(int height);

  QScrollBar *getVBar() const { return vbar_; }

 private slots:
  void hscrollSlot(int x);
  void vscrollSlot(int y);

 private:
  typedef std::map<int,Change> ChangeMap;

  CQDiff                   *diff_        { nullptr };
  CSideType                 side_        { CSIDE_TYPE_LEFT };
  QString                   fileName_;
  std::vector<std::string>  lines_;
  ChangeMap                 changeMap_;
  int                       x_offset_    { 0 };
  int                       y_offset_    { 0 };
  CQFileEditCanvas         *canvas_      { nullptr };
  QScrollBar               *vbar_        { nullptr };
  QScrollBar               *hbar_        { nullptr };
  bool                      showNumbers_ { true };
  int                       charWidth_   { 0 };
  int                       charHeight_  { 0 };
  int                       charAscent_  { 0 };
};

//------

class CQDiff : public CQMainWindow {
  Q_OBJECT

  Q_PROPERTY(QColor bgColor          READ bgColor          WRITE setBgColor)
  Q_PROPERTY(QColor fgColor          READ fgColor          WRITE setFgColor)
  Q_PROPERTY(QColor borderColor      READ borderColor      WRITE setBorderColor)
  Q_PROPERTY(QColor leftAddColor     READ leftAddColor     WRITE setLeftAddColor)
  Q_PROPERTY(QColor leftChangeColor  READ leftChangeColor  WRITE setLeftChangeColor)
  Q_PROPERTY(QColor leftDeleteColor  READ leftDeleteColor  WRITE setLeftDeleteColor)
  Q_PROPERTY(QColor rightAddColor    READ rightAddColor    WRITE setRightAddColor)
  Q_PROPERTY(QColor rightChangeColor READ rightChangeColor WRITE setRightChangeColor)
  Q_PROPERTY(QColor rightDeleteColor READ rightDeleteColor WRITE setRightDeleteColor)
  Q_PROPERTY(QColor selectedColor    READ selectedColor    WRITE setSelectedColor)

 public:
  typedef std::vector<CQDiffChange> ChangeArray;

 public:
  CQDiff();
 ~CQDiff();

  const QColor &bgColor() const { return bgColor_; }
  void setBgColor(const QColor &v) { bgColor_ = v; }

  const QColor &fgColor() const { return fgColor_; }
  void setFgColor(const QColor &v) { fgColor_ = v; }

  const QColor &borderColor() const { return borderColor_; }
  void setBorderColor(const QColor &v) { borderColor_ = v; }

  const QColor &leftAddColor() const { return leftAddColor_; }
  void setLeftAddColor(const QColor &v) { leftAddColor_ = v; }

  const QColor &leftChangeColor() const { return leftChangeColor_; }
  void setLeftChangeColor(const QColor &v) { leftChangeColor_ = v; }

  const QColor &leftDeleteColor() const { return leftDeleteColor_; }
  void setLeftDeleteColor(const QColor &v) { leftDeleteColor_ = v; }

  const QColor &rightAddColor() const { return rightAddColor_; }
  void setRightAddColor(const QColor &v) { rightAddColor_ = v; }

  const QColor &rightChangeColor() const { return rightChangeColor_; }
  void setRightChangeColor(const QColor &v) { rightChangeColor_ = v; }

  const QColor &rightDeleteColor() const { return rightDeleteColor_; }
  void setRightDeleteColor(const QColor &v) { rightDeleteColor_ = v; }

  const QColor &selectedColor() const { return selectedColor_; }
  void setSelectedColor(const QColor &v) { selectedColor_ = v; }

  void setFiles(const std::string &src, const std::string &dst);

  void addSrc(const std::string &src);
  void addDst(const std::string &dst);

  void exec();

  bool parseChange(const std::string &line);

  QWidget *createCentralWidget() override;

  void createMenus() override;
  void createToolBars() override;
  void createStatusBar() override;

  void scrollToChange(int change);

  void setDataHeight(int dataHeight);

  CQFileEdit *getEdit(CSideType side) const {
    return (side == CSIDE_TYPE_LEFT ? ledit_ : redit_);
  }

  const ChangeArray &getChanges() const { return changes_; }

  int getNumChanges() const { return changes_.size(); }

  int  getChangeNum() const { return changeNum_; }
  void setChangeNum(int changeNum);

  const CQDiffChange &getChange(int i) const {
    assert(i >= 0 && i < int(changes_.size()));

    return changes_[i];
  }

  CQDiffChange &getChange(int i) {
    assert(i >= 0 && i < int(changes_.size()));

    return changes_[i];
  }

  bool isIgnoreWhiteSpace() const { return ignoreWhiteSpace_; }
  void setIgnoreWhiteSpace(bool b) { ignoreWhiteSpace_ = b; }

  QColor getChangeColor(CSideType side, char c) const {
    if (side == CSIDE_TYPE_LEFT) {
      switch (c) {
        case 'a': return leftAddColor();
        case 'c': return leftChangeColor();
        case 'd': return leftDeleteColor();
        default : return bgColor();
      }
    }
    else {
      switch (c) {
        case 'a': return rightAddColor();
        case 'c': return rightChangeColor();
        case 'd': return rightDeleteColor();
        default : return bgColor();
      }
    }
  }

  QSize sizeHint() const override;

 signals:
  void changeNumChanged();

 private slots:
  void scrollSlot(int y);

  void firstDiffSlot();
  void lastDiffSlot();
  void prevDiffSlot();
  void nextDiffSlot();

  void recomputeSlot();

  void whiteSpaceSlot(bool);
  void showLineNumbersSlot(bool);

  void aboutSlot();

  void scrollToChange();

 private:
  void updateVBar();

 private:
  QColor       bgColor_             { 255, 255, 255 };
  QColor       fgColor_             { 0, 0, 0 };
  QColor       borderColor_         { 200, 200, 200 };
  QColor       leftAddColor_        { 255, 150, 150 };
  QColor       leftChangeColor_     { 150, 255, 150 };
  QColor       leftDeleteColor_     { 150, 150, 255 };
  QColor       rightAddColor_       { 255, 100, 100 };
  QColor       rightChangeColor_    { 100, 255, 100 };
  QColor       rightDeleteColor_    { 100, 100, 255 };
  QColor       selectedColor_       { 240, 230, 140 };
  QWidget     *frame_               { nullptr };
  QLabel      *llabel_              { nullptr };
  QLabel      *rlabel_              { nullptr };
  CQFileEdit  *ledit_               { nullptr };
  CQDiffBar   *vbar_                { nullptr };
  CQFileEdit  *redit_               { nullptr };
  CQMenu      *fileMenu_            { nullptr };
  CQMenu      *diffMenu_            { nullptr };
  CQMenuItem  *firstDiffItem_       { nullptr };
  CQMenuItem  *lastDiffItem_        { nullptr };
  CQMenuItem  *nextDiffItem_        { nullptr };
  CQMenuItem  *prevDiffItem_        { nullptr };
  CQMenuItem  *whiteSpaceItem_      { nullptr };
  CQMenuItem  *recompItem_          { nullptr };
  CQMenuItem  *showLineNumbersItem_ { nullptr };
  CQMenu      *viewMenu_            { nullptr };
  CQMenu      *helpMenu_            { nullptr };
  CQToolBar   *diffToolBar_         { nullptr };
  CQDiffCombo *diffCombo_           { nullptr };
  QLabel      *lslabel_             { nullptr };
  QLabel      *rslabel_             { nullptr };
  ChangeArray  changes_;
  int          changeNum_           { 0 };
  int          dataHeight_          { 0 };
  int          scrollHeight_        { 0 };
  bool         ignoreWhiteSpace_    { false };
};

#endif
