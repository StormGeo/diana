/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qtVcrossWindow.h"

#include "diController.h"
#include "diLocationPlot.h"
#include "diUtilities.h"

#include "qtUtility.h"
#include "qtToggleButton.h"
#include "qtVcrossDialog.h"
#include "qtVcrossSetupDialog.h"
#include "diEditItemManager.h"
#include "EditItems/toolbar.h"
#include "qtPrintManager.h"

#include <puTools/mi_boost_compatibility.hh>
#include <puTools/miSetupParser.h>
#include <puTools/miStringFunctions.h>

#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qfont.h>
#include <QCheckBox>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>
#include <QPixmap>
#include <QSpinBox>
#include <QAction>

#include <QtCore/QAbstractListModel>
#include <vector>

#define MILOGGER_CATEGORY "diana.VcrossWindow"
#include <miLogger/miLogging.h>

#include "forover.xpm"
#include "bakover.xpm"

using namespace vcross;

namespace /* anonymous */ {

namespace VectorModelDetail {

template<class ValueType>
struct BasicExtract {
  typedef ValueType value_t;
};

} // namespace VectorModelDetail

// ========================================================================

template<class Extract>
class VectorModel : public QAbstractListModel {
public:
  typedef typename Extract::value_t value_t;
  typedef std::vector<value_t> vector_t;

  VectorModel(const vector_t& values, const Extract& e = Extract())
    : mValues(values), mExtract(e) { }

  int rowCount(const QModelIndex&) const
    { return mValues.size(); }

  QVariant data(const QModelIndex& index, int role) const;
  
  const vector_t& values() const
    { return mValues; }

private:
  vector_t mValues;
  Extract mExtract;
};

template<class Extract>
QVariant VectorModel<Extract>::data(const QModelIndex& index, int role) const
{
  if (role == Qt::DisplayRole)
    return mExtract.text(mValues.at(index.row()));
  else if (role == Qt::ToolTipRole or role == Qt::StatusTipRole)
    return mExtract.tip(mValues.at(index.row()));
  else
    return QVariant();
}

struct MiTimeExtract : public VectorModelDetail::BasicExtract<miutil::miTime>
{
  QVariant text(const miutil::miTime& t) const
    { return QString::fromStdString(t.isoTime()); }
  QVariant tip(const miutil::miTime& t) const
    { return QVariant(); }
};
typedef VectorModel<MiTimeExtract> MiTimeModel;

} // namespace anonymous

VcrossWindow::VcrossWindow(Controller *co)
  : QWidget(0)
  , dynEditManagerConnected(false)
{
  METLIBS_LOG_SCOPE();
  
  vcrossm =  miutil::make_shared<QtManager>();

  setWindowTitle( tr("Diana Vertical Crossections") );

  //central widget
  vcrossw = new QtWidget(vcrossm,this);
  connect(vcrossw, SIGNAL(timeChanged(int)),SLOT(timeChangedSlot(int)));
  connect(vcrossw, SIGNAL(crossectionChanged(int)),SLOT(crossectionChangedSlot(int)));

  //button for model/field dialog-starts new dialog
  dataButton = new ToggleButton(this, tr("Model/field"));
  connect( dataButton, SIGNAL( toggled(bool)), SLOT( dataClicked( bool) ));

  //button for setup - starts setupdialog
  setupButton = new ToggleButton(this, tr("Settings"));
  connect( setupButton, SIGNAL( toggled(bool)), SLOT( setupClicked( bool) ));

  //button for timeGraph
  timeGraphButton = new ToggleButton(this, tr("TimeGraph"));
  connect( timeGraphButton, SIGNAL( toggled(bool)), SLOT( timeGraphClicked( bool) ));

  //button to print - starts print dialog
  QPushButton* printButton = NormalPushButton(tr("Print"),this);
  connect( printButton, SIGNAL(clicked()), SLOT( printClicked() ));

  //button to save - starts save dialog
  QPushButton* saveButton = NormalPushButton(tr("Save"),this);
  connect( saveButton, SIGNAL(clicked()), SLOT( saveClicked() ));

  //button for quit
  QPushButton * quitButton = NormalPushButton(tr("Quit"),this);
  connect( quitButton, SIGNAL(clicked()), SLOT(quitClicked()) );

  //button for help - pushbutton
  QPushButton * helpButton = NormalPushButton(tr("Help"),this);
  connect( helpButton, SIGNAL(clicked()), SLOT(helpClicked()) );

  dynEditManager = new QCheckBox(tr("Draw/Edit"), this);
  dynEditManager->setEnabled(false);
  connect(dynEditManager, SIGNAL(stateChanged(int)),
      SLOT(dynCrossEditManagerEnabled(int)));

  QAbstractButton *leftCrossectionButton= new QToolButton(this);
  leftCrossectionButton->setIcon(QPixmap(bakover_xpm));
  connect(leftCrossectionButton, SIGNAL(clicked()), SLOT(leftCrossectionClicked()) );
  leftCrossectionButton->setAutoRepeat(true);

  //combobox to select crossection
  std::vector<std::string> dummycross;
  dummycross.push_back("                        ");
  crossectionBox = ComboBox(this, dummycross, true, 0);
  connect( crossectionBox, SIGNAL( activated(int) ),
      SLOT( crossectionBoxActivated(int) ) );

  QAbstractButton *rightCrossectionButton= new QToolButton(this);
  rightCrossectionButton->setIcon(QPixmap(forward_xpm));
  connect(rightCrossectionButton, SIGNAL(clicked()), SLOT(rightCrossectionClicked()) );
  rightCrossectionButton->setAutoRepeat(true);

  QAbstractButton *leftTimeButton = new QToolButton(this);
  leftTimeButton->setIcon(QPixmap(bakover_xpm));
  connect(leftTimeButton, SIGNAL(clicked()), SLOT(leftTimeClicked()) );
  leftTimeButton->setAutoRepeat(true);

  //combobox to select time
  timeBox = new QComboBox(this);
  timeBox->setModel(new MiTimeModel(std::vector<miutil::miTime>()));
  timeBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  connect(timeBox, SIGNAL(activated(int)), SLOT(timeBoxActivated(int)));

  QAbstractButton *rightTimeButton = new QToolButton(this);
  rightTimeButton->setIcon(QPixmap(forward_xpm));
  connect(rightTimeButton, SIGNAL(clicked()), SLOT(rightTimeClicked()) );
  rightTimeButton->setAutoRepeat(true);

  timeSpinBox = new QSpinBox(this);
  timeSpinBox->setMinimum(1);
  timeSpinBox->setValue(1);
  timeSpinBox->setToolTip(tr("Number of times to step forward/backward when using the time arrow buttons"));

  QVBoxLayout* vlayout = new QVBoxLayout;
  vlayout->setSpacing(2);
  vlayout->setMargin(2);

  QHBoxLayout* vclayout = new QHBoxLayout;
  vclayout->setSpacing(2);
  vclayout->addWidget(dataButton);
  vclayout->addWidget(setupButton);
  vclayout->addWidget(timeGraphButton);
  vclayout->addWidget(printButton);
  vclayout->addWidget(saveButton);
  vclayout->addWidget(quitButton);
  vclayout->addWidget(helpButton);
  vclayout->addWidget(dynEditManager);
  vclayout->addStretch();
  vlayout->addLayout(vclayout);

  QHBoxLayout* tslayout = new QHBoxLayout;
  tslayout->setSpacing(2);
  tslayout->addWidget(leftCrossectionButton);
  tslayout->addWidget(crossectionBox);
  tslayout->addWidget(rightCrossectionButton);
  tslayout->addWidget(leftTimeButton);
  tslayout->addWidget(timeBox);
  tslayout->addWidget(rightTimeButton);
  tslayout->addWidget(timeSpinBox);
  tslayout->addStretch();
  vlayout->addLayout(tslayout);

  vlayout->addWidget(vcrossw);

  setLayout(vlayout);

  //connected dialogboxes

  vcDialog = new VcrossDialog(this,vcrossm);
  connect(vcDialog, SIGNAL(VcrossDialogApply(bool)),SLOT(changeFields(bool)));
  connect(vcDialog, SIGNAL(VcrossDialogHide()),SLOT(hideDialog()));
  connect(vcDialog, SIGNAL(showsource(const std::string&, const std::string&)),
      SIGNAL(showsource(const std::string&, const std::string&)));


  vcSetupDialog = new VcrossSetupDialog(this,vcrossm);
  connect(vcSetupDialog, SIGNAL(SetupApply()),SLOT(changeSetup()));
  connect(vcSetupDialog, SIGNAL(SetupHide()),SLOT(hideSetup()));
  connect(vcSetupDialog, SIGNAL(showsource(const std::string&, const std::string&)),
      SIGNAL(showsource(const std::string&, const std::string&)));

  // --------------------------------------------------------------------
  showPrevPlotAction = new QAction( tr("P&revious plot"), this );
  showPrevPlotAction->setShortcut(Qt::Key_F10);
  connect( showPrevPlotAction, SIGNAL( triggered() ) ,  SIGNAL( prevHVcrossPlot() ) );
  addAction( showPrevPlotAction );
  // --------------------------------------------------------------------
  showNextPlotAction = new QAction( tr("&Next plot"), this );
  showNextPlotAction->setShortcut(Qt::Key_F11);
  connect( showNextPlotAction, SIGNAL( triggered() ) ,  SIGNAL( nextHVcrossPlot() ) );
  addAction( showNextPlotAction );
  // --------------------------------------------------------------------

  //inialize everything in startUp
  firstTime = true;
  active = false;
}

/***************************************************************************/

VcrossWindow::~VcrossWindow()
{
}

/***************************************************************************/

void VcrossWindow::dataClicked( bool on ){
  //called when the model button is clicked
  if( on ){
    METLIBS_LOG_DEBUG("Model/field button clicked on");
    vcDialog->show();
  } else {
    METLIBS_LOG_DEBUG("Model/field button clicked off");
    vcDialog->hide();
  }
}

/***************************************************************************/

void VcrossWindow::leftCrossectionClicked()
{
  //called when the left Crossection button is clicked
  const std::string s = vcrossm->setCrossection(-1);
  crossectionChangedSlot(-1);
  vcrossw->update();
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::rightCrossectionClicked()
{
  //called when the right Crossection button is clicked
  const std::string s= vcrossm->setCrossection(+1);
  crossectionChangedSlot(+1);
  vcrossw->update();
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::stepTime(int direction)
{
  const int step = std::max(timeSpinBox->value(), 1)
      * (direction < 0 ? -1 : 1);
  vcrossm->setTime(step);
  timeChangedSlot(step);
  vcrossw->update();
}

/***************************************************************************/

void VcrossWindow::leftTimeClicked()
{
  stepTime(-1);
}

/***************************************************************************/

void VcrossWindow::rightTimeClicked()
{
  stepTime(+1);
}

/***************************************************************************/

bool VcrossWindow::timeChangedSlot(int diff)
{
  // called if signal timeChanged is emitted from graphics window (qtVcrossWidget)
  METLIBS_LOG_SCOPE();

  const int count = timeBox->count();
  METLIBS_LOG_DEBUG(LOGVAL(count) << LOGVAL(diff));
  if (count <= 0)
    return false;

  if (diff != 0) {
    int index = timeBox->currentIndex();
    index += diff;
    while (index < 0)
      index += count;
    index %= count;
    timeBox->setCurrentIndex(index);
  }

  const miutil::miTime& vct = vcrossm->getTime();
  const MiTimeModel* tim = static_cast<MiTimeModel*>(timeBox->model());
  const miutil::miTime& tbt = tim->values().at(timeBox->currentIndex());

  bool ok = (vct == tbt);

  // search timeList
  if (not ok) {
    for (int i = 0; i<count; i++) {
      if (vct == tim->values().at(i)) {
        timeBox->setCurrentIndex(i);
        METLIBS_LOG_DEBUG(LOGVAL(i));
        ok = true;
        break;
      }
    }
  }

  if (not ok) {
    METLIBS_LOG_DEBUG(LOGVAL(vct) << "not found");
    return false;
  }

  /*emit*/ setTime("vcross", vct);
  return true;
}

/***************************************************************************/

bool VcrossWindow::crossectionChangedSlot(int diff)
{
  METLIBS_LOG_SCOPE();

  const int count = crossectionBox->count();
  METLIBS_LOG_DEBUG(LOGVAL(count) << LOGVAL(diff));
  if (count <= 0)
    return false;

  if (diff != 0) {
    int index = crossectionBox->currentIndex();
    index += diff;
    while (index < 0)
      index += count;
    index %= count;
    crossectionBox->setCurrentIndex(index);
  }

  //get current crossection
  std::string s = vcrossm->getCrossection();
  //if no current crossection, use last crossection plotted
  if (s.empty())
    s = ""; // FIXME vcrossm->getLastCrossection();
  std::string sbs = crossectionBox->currentText().toStdString();
  if (sbs != s){
    const int n = crossectionBox->count();
    for(int i = 0;i<n;i++){
      if (s==crossectionBox->itemText(i).toStdString()) {
        crossectionBox->setCurrentIndex(i);
        sbs = crossectionBox->currentText().toStdString();
        break;
      }
    }
  }
  QString sq = QString::fromStdString(s);
  if (sbs == s) {
    /*emit*/ crossectionChanged(sq); //name of current crossection (to mainWindow)
    return true;
  } else {
    //    METLIBS_LOG_WARN("WARNING! crossectionChangedSlot  crossection from vcrossm ="
    // 	 << s    <<" not equal to crossectionBox text = " << sbs);
    //current or last crossection plotted is not in the list, insert it...
    crossectionBox->addItem(sq,0);
    crossectionBox->setCurrentIndex(0);
    return false;
  }
}


/***************************************************************************/

void VcrossWindow::printClicked()
{
  printerManager pman;
  std::string command = pman.printCommand();

  QPrinter qprt;
  fromPrintOption(qprt, priop);

  QPrintDialog printerDialog(&qprt, this);
  if (printerDialog.exec()) {
    // fill printOption from qprinter-selections
    toPrintOption(qprt, priop);

    diutil::OverrideCursor waitCursor;
    vcrossw->print(qprt);
  }
}

/***************************************************************************/

void VcrossWindow::saveClicked()
{
  QString filename = QFileDialog::getSaveFileName(this,
      tr("Save plot as image"),
      mRasterFilename,
      tr("Images (*.png *.xpm *.bmp);;All (*.*)"));
  
  if (not filename.isNull()) {// got a filename
    mRasterFilename = filename;
    if (not vcrossw->saveRasterImage(filename))
      QMessageBox::warning(this, tr("Save image failed"),
          tr("Saveing the vertical cross section plot as '%1' failed. Sorry.").arg(filename));
  }
}


void VcrossWindow::makeEPS(const std::string& filename)
{
  diutil::OverrideCursor waitCursor;
  printOptions priop;
  priop.fname= filename;
  priop.colop= d_print::incolour;
  priop.orientation= d_print::ori_automatic;
  priop.pagesize= d_print::A4;
  priop.numcopies= 1;
  priop.usecustomsize= false;
  priop.fittopage= false;
  priop.drawbackground= true;
  priop.doEPS= true;

//  vcrossw->print(priop);
}

/***************************************************************************/

void VcrossWindow::setupClicked(bool on){
  //called when the setup button is clicked
  if( on ){
    vcSetupDialog->start();
    vcSetupDialog->show();
  } else {
    vcSetupDialog->hide();
  }
}

/***************************************************************************/

void VcrossWindow::timeGraphClicked(bool on)
{
  // called when the timeGraph button is clicked
  METLIBS_LOG_SCOPE("on=" << on);

  if (on && vcrossm->timeGraphOK()) {
    vcrossw->enableTimeGraph(true);
  } else if (on) {
    timeGraphButton->setChecked(false);
  } else {
    vcrossm->disableTimeGraph();
    vcrossw->enableTimeGraph(false);
    vcrossw->update();
  }
}

/***************************************************************************/

void VcrossWindow::quitClicked()
{
  //called when the quit button is clicked
  METLIBS_LOG_SCOPE();

  dataButton->setChecked(false);
  setupButton->setChecked(false);

  // cleanup selections in dialog and data in memory
  vcDialog->cleanup();
  vcrossm->cleanup();

  crossectionBox->clear();
  timeBox->setModel(new MiTimeModel(std::vector<miutil::miTime>()));

  active = false;
  /*emit*/ updateCrossSectionPos(false);
  /*emit*/ VcrossHide();
  std::vector<miutil::miTime> t;
  /*emit*/ emitTimes("vcross", t);
}

/***************************************************************************/

void VcrossWindow::helpClicked()
{
  //called when the help button in Vcrosswindow is clicked
  METLIBS_LOG_SCOPE();
  /*emit*/ showsource("ug_verticalcrosssections.html");
}

void VcrossWindow::dynCrossEditManagerEnabled(int state)
{
  if (state == Qt::Checked) {
    EditItems::ToolBar::instance()->setCreatePolyLineAction("Cross section");
    EditItemManager::instance()->setEditing(true);
    EditItems::ToolBar::instance()->show();
    dynCrossEditManagerEnableSignals();
  } else {
    EditItemManager::instance()->setEditing(false);
  }
}

void VcrossWindow::dynCrossEditManagerEnableSignals()
{
  if (not dynEditManagerConnected) {
    dynEditManagerConnected = true;

    EditItemManager::instance()->enableItemChangeNotification();
    EditItemManager::instance()->setItemChangeFilter("Cross section");
    connect(EditItemManager::instance(), SIGNAL(itemChanged(const QVariantMap &)),
        this, SLOT(dynCrossEditManagerChange(const QVariantMap &)), Qt::UniqueConnection);
    connect(EditItemManager::instance(), SIGNAL(itemRemoved(int)),
        this, SLOT(dynCrossEditManagerRemoval(int)), Qt::UniqueConnection);
    connect(EditItemManager::instance(), SIGNAL(editing(bool)),
        this, SLOT(slotCheckEditmode(bool)), Qt::UniqueConnection);
  }
}

void VcrossWindow::slotCheckEditmode(bool editing)
{
  if (vcrossm->supportsDynamicCrossections())
    dynEditManager->setChecked(editing);
}

void VcrossWindow::dynCrossEditManagerChange(const QVariantMap &props)
{
  METLIBS_LOG_SCOPE();

  const char key_points[] = "latLonPoints", key_id[] = "id";
  if (not (props.contains(key_points) and props.contains(key_id)))
    return;

  std::string label = QString("dyn_%1").arg(props.value(key_id).toInt()).toStdString();

  vcross::LonLat_v points;
  foreach (QVariant v, props.value(key_points).toList()) {
    const QPointF p = v.toPointF();
    const float lat = p.x(), lon = p.y(); // FIXME swpa x <-> y
    points.push_back(LonLat::fromDegrees(lon, lat));
  }
  if (points.size() < 2)
    return;

  vcrossm->setDynamicCrossection(label, points);

  Q_EMIT crossectionSetChanged();
  updateCrossectionBox();

  vcrossm->setCrossection(label);

  vcrossw->update();
}

void VcrossWindow::dynCrossEditManagerRemoval(int id)
{
  METLIBS_LOG_SCOPE();

  std::string label = QString("dyn_%1").arg(id).toStdString();
  vcrossm->setDynamicCrossection(label, vcross::LonLat_v()); // empty points => remove
  Q_EMIT crossectionSetChanged();
  updateCrossectionBox();

  vcrossw->update();
}

/***************************************************************************/

void VcrossWindow::emitQmenuStrings()
{
  const std::string plotname = "<font color=\"#005566\">" + vcDialog->getShortname() + " " + vcrossm->getCrossection() + "</font>";
  const std::vector<std::string> qm_string = vcrossm->getQuickMenuStrings();
  /*emit*/ quickMenuStrings(plotname, qm_string);
}

/***************************************************************************/

void VcrossWindow::changeFields(bool modelChanged)
{
  //called when the apply button from model/field dialog is clicked
  //... or field is changed ?
  METLIBS_LOG_SCOPE();

  if (vcrossm->supportsDynamicCrossections()) {
    dynEditManager->setEnabled(true);
    if (EditItemManager::instance()) {
      typedef std::set<std::string> string_s;
      const string_s& csPredefined = vcrossm->getCrossectionPredefinitions();
      if (not csPredefined.empty()) {
        for (string_s::const_iterator it = csPredefined.begin(); it != csPredefined.end(); ++it)
          EditItemManager::instance()->emitLoadFile(QString::fromStdString(*it));

#if 0
        const QSet<QSharedPointer<DrawingItemBase> > items
            = EditItemManager::instance()->getLayerManager()->itemsInSelectedLayers(false);
        foreach (const QSharedPointer<DrawingItemBase> item, items) {
          if (item->property("style:type").toString() == "Cross section") {
            // TODO add name to vcross manager without actually creating a cross-section
          }
        }
#endif
        dynCrossEditManagerEnableSignals();
      } else {
        dynEditManager->setChecked(true);
      }
    }
  } else {
    dynEditManager->setChecked(false);
    dynEditManager->setEnabled(false);
  }

  if (modelChanged) {

    //emit to MainWindow (updates crossectionPlot)
    Q_EMIT crossectionSetChanged();

    //update combobox lists of crossections and time
    updateCrossectionBox();
    updateTimeBox();

    //get correct selection in comboboxes
    crossectionChangedSlot(0);
    timeChangedSlot(0);
  }

  vcrossw->update();
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::changeSetup()
{
  //called when the apply from setup dialog is clicked
  METLIBS_LOG_SCOPE();

  //###if (mapOptionsChanged) {
  // emit to MainWindow
  // (updates crossectionPlot colour etc., not data, name etc.)
  /*emit*/ crossectionSetUpdate();
  //###}

  vcrossw->update();
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::hideDialog()
{
  //called when the hide button (from model dialog) is clicked
  METLIBS_LOG_SCOPE();

  vcDialog->hide();
  dataButton->setChecked(false);
}

/***************************************************************************/

void VcrossWindow::hideSetup()
{
  //called when the hide button (from setup dialog) is clicked
  METLIBS_LOG_SCOPE();

  vcSetupDialog->hide();
  setupButton->setChecked(false);
}

/***************************************************************************/

void VcrossWindow::getCrossections(LocationData& locationdata)
{
  METLIBS_LOG_SCOPE();

  vcrossm->getCrossections(locationdata);
}

/***************************************************************************/

void VcrossWindow::updateCrossectionBox()
{
  //update list of crossections in crossectionBox
  METLIBS_LOG_SCOPE();

  crossectionBox->clear();
  const std::vector<std::string>& crossections = vcrossm->getCrossectionList();

  for (size_t i=0; i<crossections.size(); i++)
    crossectionBox->addItem(QString::fromStdString(crossections[i]));
}

/***************************************************************************/

void VcrossWindow::updateTimeBox()
{
  METLIBS_LOG_SCOPE();

  const std::vector<miutil::miTime>& times = vcrossm->getTimeList();
  timeBox->setModel(new MiTimeModel(times));
  /*emit*/ emitTimes("vcross",times);
}

/***************************************************************************/

void VcrossWindow::crossectionBoxActivated(int index)
{
  const QString& sq = crossectionBox->currentText();
  vcrossm->setCrossection(sq.toStdString());
  vcrossw->update();
  /*emit*/ crossectionChanged(sq); //name of current crossection (to mainWindow)
  emitQmenuStrings();
}

/***************************************************************************/

void VcrossWindow::timeBoxActivated(int index)
{
  const std::vector<miutil::miTime>& times = vcrossm->getTimeList();

  if (index>=0 && index < int(times.size())) {
    vcrossm->setTime(times[index]);
    vcrossw->update();
    /*emit*/ setTime("vcross", times[index]);
  }
}

/***************************************************************************/

bool VcrossWindow::changeCrossection(const std::string& crossection)
{
  METLIBS_LOG_SCOPE();

  vcrossm->setCrossection(crossection); //HK ??? should check if crossection exists ?
  vcrossw->update();
  raise();
  emitQmenuStrings();

  if (crossectionChangedSlot(0))
    return true;
  else
    return false;
}

/***************************************************************************/

void VcrossWindow::mainWindowTimeChanged(const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE();
  if (!active)
    return;
  METLIBS_LOG_DEBUG("time = " << t);

  vcrossm->mainWindowTimeChanged(t);
  //get correct selection in comboboxes
  crossectionChangedSlot(0);
  timeChangedSlot(0);
  vcrossw->update();
}


/***************************************************************************/

void VcrossWindow::startUp(const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE("t= " << t);

  active = true;
  //do something first time we start Vertical crossections
  if (firstTime){
    //vector<miutil::miString> models;
    //define models for dialogs, comboboxes and crossectionplot
    //vcrossm->setSelectedModels(models, true,false);
    //vcDialog->setSelection();
    firstTime=false;
    // show default diagram without any data
    vcrossw->update();
  }
  //changeModel();
  mainWindowTimeChanged(t);
}

/*
 * Set the position clicked on the map in the current VcrossPlot.
 */
void VcrossWindow::mapPos(float lat, float lon)
{
  METLIBS_LOG_SCOPE(LOGVAL(lat) << LOGVAL(lon));

//  if(vcrossm->setCrossection(lat,lon)) {
//    // If the return is true (field) update the crossection box and
//    // tell mainWindow to reread the crossections.
//    updateCrossectionBox();
//    /*emit*/ crossectionSetChanged();
//  }
  emitQmenuStrings();
}

void VcrossWindow::parseQuickMenuStrings(const std::vector<std::string>& qm_string)
{
  vcrossm->parseQuickMenuStrings(qm_string);
  vcDialog->putOKString(qm_string);
  /*emit*/ crossectionSetChanged();

  //update combobox lists of crossections and time
  updateCrossectionBox();
  updateTimeBox();

  crossectionChangedSlot(0);
  vcrossw->update();
}

/***************************************************************************/
void VcrossWindow::parseSetup()
{
  METLIBS_LOG_SCOPE();
  string_v sources;
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_FILES", sources);
  string_v computations;
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_COMPUTATIONS", computations);
  string_v plots;
  miutil::SetupParser::getSection("VERTICAL_CROSSECTION_PLOTS", plots);
  vcrossm->parseSetup(sources,computations,plots);
}

std::vector<std::string> VcrossWindow::writeLog(const std::string& logpart)
{
  std::vector<std::string> vstr;

  if (logpart=="window") {
    std::string str;
    str= "VcrossWindow.size " + miutil::from_number(width()) + " "
        + miutil::from_number(height());
    vstr.push_back(str);
    str= "VcrossWindow.pos " + miutil::from_number(x()) + " "
        + miutil::from_number(y());
    vstr.push_back(str);
    str= "VcrossDialog.pos "  + miutil::from_number(vcDialog->x()) + " "
        + miutil::from_number(vcDialog->y());
    vstr.push_back(str);
    str= "VcrossSetupDialog.pos " + miutil::from_number(vcSetupDialog->x()) + " "
        + miutil::from_number(vcSetupDialog->y());
    vstr.push_back(str);

    // printer name & options...
    if (not priop.printer.empty()){
      str= "PRINTER " + priop.printer;
      vstr.push_back(str);
      if (priop.orientation==d_print::ori_portrait)
        str= "PRINTORIENTATION portrait";
      else
        str= "PRINTORIENTATION landscape";
      vstr.push_back(str);
    }

  } else if (logpart=="setup") {
    vstr = vcrossm->writeLog();
  } else if (logpart=="field") {
    vstr = vcDialog->writeLog();
  }

  return vstr;
}

void VcrossWindow::readLog(const std::string& logpart, const std::vector<std::string>& vstr,
    const std::string& thisVersion, const std::string& logVersion,
    int displayWidth, int displayHeight)
{
  if (logpart=="window") {

    const int n = vstr.size();

    for (int i=0; i<n; i++) {
      const std::vector<std::string> tokens = miutil::split(vstr[i], 0, " ");
      if (tokens.size()==3) {

        int x= atoi(tokens[1].c_str());
        int y= atoi(tokens[2].c_str());
        if (x>20 && y>20 && x<=displayWidth && y<=displayHeight) {
          if (tokens[0]=="VcrossWindow.size") this->resize(x,y);
        }
        if (x>=0 && y>=0 && x<displayWidth-20 && y<displayHeight-20) {
          if      (tokens[0]=="VcrossWindow.pos")      this->move(x,y);
          else if (tokens[0]=="VcrossDialog.pos")      vcDialog->move(x,y);
          else if (tokens[0]=="VcrossSetupDialog.pos") vcSetupDialog->move(x,y);
        }

      } else if (tokens.size()>=2) {

        if (tokens[0]=="PRINTER") {
          priop.printer=tokens[1];
        } else if (tokens[0]=="PRINTORIENTATION") {
          if (tokens[1]=="portrait")
            priop.orientation=d_print::ori_portrait;
          else
            priop.orientation=d_print::ori_landscape;
        }

      }
    }

  } else if (logpart=="setup") {
    vcrossm->readLog(vstr, thisVersion, logVersion);
  } else if (logpart=="field") {
    vcDialog->readLog(vstr,thisVersion,logVersion);
  }
}

/***************************************************************************/

void VcrossWindow::closeEvent(QCloseEvent * e)
{
  quitClicked();
}
