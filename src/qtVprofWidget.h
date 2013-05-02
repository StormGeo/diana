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
#ifndef VPROFWIDGET_H
#define VPROFWIDGET_H

#include <qglobal.h>

#include "diVprofDiagram.h"
#include "diVprofData.h"
#include "diVprofPlot.h"
#include <puTools/miString.h>
#include <map>

#if !defined(USE_PAINTGL)
#include <qgl.h>
#else
#include "PaintGL/paintgl.h"
#include <QWidget>
#define QGLWidget PaintGLWidget
#endif
#include <QKeyEvent>

using namespace std;

class VprofManager;

/**
   \brief The OpenGL widget for Vertical Profiles (soundings)

   Handles widget paint/redraw events.
   Receives mouse and keybord events and initiates actions.
*/
class VprofWidget : public QGLWidget
{
  Q_OBJECT

public:
#if !defined(USE_PAINTGL)
  VprofWidget(VprofManager *vpm, const QGLFormat fmt,
             QWidget* parent = 0);
#else
  VprofWidget(VprofManager *vpm, QWidget* parent = 0);
#endif

  bool saveRasterImage(const miutil::miString fname,
  		       const miutil::miString format,
		       const int quality = -1);

protected:

  void initializeGL();
  void paintGL();
  void resizeGL( int w, int h );

private:
  VprofManager *vprofm;

  void keyPressEvent(QKeyEvent *me);

signals:
  void timeChanged(int);
  void stationChanged(int);

};


#endif
