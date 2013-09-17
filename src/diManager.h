/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diController.cc 3685 2013-09-11 17:19:09Z davidb $

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

#ifndef MANAGER_H
#define MANAGER_H

#include <diField/diArea.h>
#include "diMapMode.h"

class PlotModule;
class QKeyEvent;
class QMouseEvent;

class Manager
{
public:
  Manager() {}
  virtual ~Manager() {}

  virtual bool parseSetup() = 0;
  virtual void sendMouseEvent(QMouseEvent* event, EventResult& res) = 0;
  virtual void sendKeyboardEvent(QKeyEvent* event, EventResult& res) = 0;

  virtual bool changeProjection(const Area& newArea) = 0;
  virtual void plot(bool under, bool over) = 0;

  virtual void setPlotModule(PlotModule *pm) = 0;

  virtual bool isEnabled() const = 0;
  virtual void setEnabled(bool enable) = 0;
};

#endif