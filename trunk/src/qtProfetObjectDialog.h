/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006 met.no

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
#ifndef QTPROFETOBJECTDIALOG_H_
#define QTPROFETOBJECTDIALOG_H_

#include <QDialog>
#include <QString>
#include <QPushButton>
//#include <QTableWidget>
//#include <QRadioButton>
//#include <QLineEdit>
//Added by qt3to4:
#include <QCloseEvent>
#include <profet/fetSession.h>
#include <profet/fetParameter.h>
#include <profet/fetObject.h>
#include <profet/fetBaseObject.h>
#include <profet/fetDynamicGui.h>
#include "qtPolygonBookmarkDialog.h"

#include <vector>
#include <map>

class QGroupBox;
class QLabel;
class QTextEdit;
class QComboBox;
class QStackedWidget;

/// map of baseComboBox index and dynamic-gui widget
typedef map<int,fetDynamicGui*> DynamicGuiMap;
/// map of baseObject name and description
typedef map<QString,QString> DescriptionMap;

class ProfetObjectDialog: public QDialog{
  Q_OBJECT
public:
  enum AreaStatus {AREA_NOT_SELECTED,AREA_OK,AREA_NOT_VALID};
  enum OperationMode {NEW_OBJECT_MODE,EDIT_OBJECT_MODE,VIEW_OBJECT_MODE};
private:
  QLabel         *parameterLabel;
  QLabel         *sessionLabel;
  QLabel         *algDescriptionLabel;
  QLabel         *statisticLabel;
  QComboBox      *baseComboBox;
  QPushButton    *databaseAreaButton;
  QPushButton    *fileAreaButton;
  QLabel         *areaInfoLabel;
  QTextEdit     *reasonText;
  QPushButton    *saveObjectButton;
  QPushButton    *cancelObjectButton;
  QStackedWidget *widgetStack;
  QGroupBox     *algGroupBox;
  QGroupBox     *areaGroupBox;
  QGroupBox     *stackGroupBox;
  QGroupBox     *reasonGroupBox;
  QGroupBox     *statGroupBox;
  DynamicGuiMap  dynamicGuiMap;
  DescriptionMap descriptionMap;
  miutil::miString       selectedBaseObject;
  miutil::miString       lastSavedPolygonName;
  OperationMode  mode;

  void           initGui();
  void           connectSignals();
  void           setAllEnabled(bool enable);
  void           logSizeAndPos();
  QString        getAreaStatusString(AreaStatus);

protected:
  void closeEvent( QCloseEvent* );

public:
  ProfetObjectDialog(QWidget* parent, OperationMode = NEW_OBJECT_MODE);

  void addDymanicGui(vector<fetDynamicGui::GuiComponent> components);
  void setSession(const miutil::miTime & session);
  void setParameter(const miutil::miString & parameter);
  void setBaseObjects(const vector<fetBaseObject> & o);
  void setAreaStatus(AreaStatus);
  void setStatistics(map<miutil::miString,float>&);
  miutil::miString getSelectedBaseObject();
  miutil::miString getReason();
  void selectDefault();
  vector<fetDynamicGui::GuiComponent> getCurrentGuiComponents();
  void newObjectMode();
  void showObject(const fetObject & obj,
      vector<fetDynamicGui::GuiComponent> components);
  void editObjectMode(const fetObject & obj,
      vector<fetDynamicGui::GuiComponent> components);
  bool showingNewObject(){ return (mode==NEW_OBJECT_MODE);}
  void startBookmarkDialog(vector<miutil::miString>& bookm);
  void setLastSavedPolygonName(miutil::miString pn) {lastSavedPolygonName=pn;}
private slots:
  void baseObjectChanged(const QString&);
  void quitBookmarks();
  void quitDialog();
  void saveDialog();

signals:
  void baseObjectSelected(miutil::miString name);
  void dynamicGuiChanged();
  void saveObjectClicked();
  void cancelObjectDialog();

  void copyPolygon(miutil::miString,miutil::miString,bool);
  void selectPolygon(miutil::miString);
  void requestPolygonList();





};

#endif /*QTPROFETOBJECTDIALOG_H_*/