/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

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
#ifndef _qtShowSatValues_h
#define _qtShowSatValues_h

#include <qwidget.h>
//Added by qt3to4:
#include <QLabel>
#include <puTools/miString.h>
#include <diCommonTypes.h>

class QLabel;
class QComboBox;

using namespace std; 
/**

  \brief Geo image pixel value in status bar
   

*/
class ShowSatValues : public QWidget {
  Q_OBJECT
public:
  ShowSatValues(QWidget* parent = 0);

private slots:
  void channelChanged(int index);

public slots:
  /// set channels in combobox
  void SetChannels(const vector<miutil::miString>& channel);
  /// show channel and value in status bar
  void ShowValues(const vector<SatValues>& satval);

// signals:
//   void calibChannel(miutil::miString);

private:
  vector<miutil::miString> tooltip;
  QLabel *sylabel;
  QLabel *chlabel;
  QComboBox *channelbox;
};


#endif









