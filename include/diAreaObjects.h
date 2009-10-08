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
#ifndef _diAreaObjects_h
#define _diAreaObjects_h
#include <vector>
#include <diField/diArea.h>
#include <diObjectPlot.h>
#include <diWeatherObjects.h>
#include <puTools/miString.h>
#include <diCommonTypes.h>


using namespace std; 

/**

  \brief WeatherObjects with special commands

 The AreaObjects class is designed to hold a vector of WeatherArea objects. Each instance of the class willhave an icon, and specific commands like"show" and "select" can be applied. It is also possible to find which areas a point is inside, with the findAreas function.
  

*/


class AreaObjects:public WeatherObjects{
public:

  AreaObjects();
  ~AreaObjects(){}
  /// assign name,icon and id, and read areastring to make WeatherArea objects  
  void makeAreas(const miString& name, const miString& icon, 
		 const miString& areastring, int id, const Area& );
  /// apply a command
  bool areaCommand(const miString& command,const miString& data);
  /// returns a vector of selectArea which x and y are inside
  vector <selectArea> findAreas(float x, float y, bool newArea=false);
  /// returns id
  int getId(){return itsId;}
  /// returns name
  miString getName(){return itsName;}
  /// gets name
  void getPlotName(miString &name){name=itsName;}
  /// gets bounding box of area with name name
  Rectangle getBoundBox(const miString& name);
  /// returns autozoom
  bool autoZoom(){return autozoom;}
  /// sets icon   
  void setIcon(miString icon){iconName = icon;}
  /// gets icon 
  miString getIcon(){return iconName;}

private:
  int itsId;
  miString itsName;
  miString iconName;
  bool clickSelect;
  bool autozoom;
  ObjectPlot* currentArea;
};
#endif




