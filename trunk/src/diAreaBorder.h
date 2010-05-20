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
#ifndef AreaBorder_h
#define AreaBorder_h

#include <diObjectPlot.h>
#include <puTools/miString.h>

using namespace std;


/**

  \brief Borders between areas when merging

  The area borders are used when merging different analyses, to set the boundary lines between regions. The lines can be moved and rotated, and points can be removed in the same way as for fronts.

*/


class AreaBorder: public ObjectPlot
{
private:
  float linewidth;
  float transitionwidth;

  void drawThickLine();
 public:
  //constructor
  AreaBorder();
  AreaBorder(const AreaBorder &rhs);
  // Destructor
  ~AreaBorder();

  /// returns linewidth
  float getLineWidth(){return linewidth;}
  /// returns transitionswidth (the width of the overlap)
  float getTransitionWidth(){return transitionwidth;}
  /// sets transitionswidth (the width of the overlap)
  virtual void setTransitionWidth(float w)
  { transitionwidth=w; if (transitionwidth<1) transitionwidth=1.; }

  /// draws the border
  virtual bool plot();
  /// returns false
  virtual bool plot(const int){return false;}
  /// shows / marks node point/ points on front
  bool showLine(float x, float y);
  /// returns showLine
  bool isOnObject(float x, float y) {return showLine(x,y);}
  void increaseSize(float val);
  void setType(int ty){}
  bool setType(miutil::miString tystring){type = 0; return true;}
  /// writes a string with Object and Type
  miutil::miString writeTypeString();

};

#endif





