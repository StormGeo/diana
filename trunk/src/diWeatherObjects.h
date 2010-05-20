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
#ifndef _diWeatherObjects_h
#define _diWeatherObjects_h
#include <vector>
#include <diField/diArea.h>
#include <diObjectPlot.h>
#include <diAreaBorder.h>
#include <diUndoFront.h>
#include <diMapMode.h>
#include <diField/diGridConverter.h>

using namespace std; 


/**

  \brief WeatherObjects to be edited or displayed


*/

class WeatherObjects{
public:

  WeatherObjects();
  virtual ~WeatherObjects(){}

  /// the weather objects to plot
  vector<ObjectPlot*> objects;
  /// clears all variables
  void clear();
  virtual void init(){}
  /// returns true of no objects or labels
  bool empty();
  /// plot all weather objects
  void plot();
 /// check if objectplots are enabled
  bool isEnabled();
 /// enable/disable objectplots
  void enable(const bool b);
  /// set prefix for object files
  void setPrefix(miutil::miString p){prefix=p;}
  /// sets the object time 
 void setTime(miutil::miTime t){itsTime=t;}
  /// gets the object time
  miutil::miTime getTime(){if (itsTime.undef()) return ztime; else return itsTime;}
  /// returns number of object plots
  int getSize(){return objects.size();}
  /// return objects area
  Area getArea(){return itsArea;}
  /// set objects area
  void setArea(const Area & area){itsArea=area;};
  /// updates the bound box
  void updateObjects();
  /// change projection to newAreas projection
  bool changeProjection(const Area& newArea);
  /// read file with comments
  bool readEditCommentFile(const miutil::miString fn);
  /// returns objects' old comments
  miutil::miString readComments();

  ///return oldLabels from object file
  vector <miutil::miString> getObjectLabels();
  /// return new edited labels 
  vector <miutil::miString> getEditLabels(); 

  /// read file with weather objects and change projection to newArea
  bool readEditDrawFile(const miutil::miString ,const Area& newArea );
  /// read  string with weather objects and change projection to newArea
  bool readEditDrawString(const miutil::miString ,const Area& newArea, bool replace=false);
  /// read file with area borders (for combining analyses)
  bool readAreaBorders(const miutil::miString ,const Area& );
  /// write file with area borders (for combining analyses)
  bool writeAreaBorders(const miutil::miString);
  /// writes string with edited objects
  miutil::miString writeEditDrawString(const miutil::miTime& );
  /// sets which object types should be plotted
  void setSelectedObjectTypes(miutil::miString t){useobject = decodeTypeString(t);}
  /// returns number of object of this type
  int objectCount(int type );
  /// add an object 
  void addObject(ObjectPlot * object, bool replace=false);
  /// remove an object
  vector<ObjectPlot*>::iterator removeObject(vector<ObjectPlot*>::iterator );
 /// the file the objects are read from
  miutil::miString filename;     
  /// decode string with types of objects to plot
  static map <miutil::miString,bool> decodeTypeString(miutil::miString);
  /// x,y for copied objects
  float xcopy,ycopy; 

private:

  map<miutil::miString,bool> useobject;
  static miutil::miTime ztime;
  GridConverter gc;              // gridconverter class
  bool enabled;


protected:

  miutil::miString prefix;               //VA,VV,VNN...   
  Area itsArea;                  // current object area
  Area geoArea;



  miutil::miTime itsTime;                //plot time 
  miutil::miString itsOldComments;       // the comment string to edit
  vector <miutil::miString> itsLabels;            //edited labels
  vector <miutil::miString> itsOldLabels;         //labels read in from object file

  //static members
  static miutil::miTime timeFromString(miutil::miString timeString);
  static miutil::miString stringFromTime(const miutil::miTime& t,bool addMinutes);

};


#endif
