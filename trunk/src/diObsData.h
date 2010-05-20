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
#ifndef diObsData_h
#define diObsData_h

#include <diField/diColour.h>


/**

  \brief Observation data
  
*/
struct ObsData
{
  //desc
  miutil::miString dataType;
  miutil::miString id;
  float xpos;
  float ypos;
  int zone;
  miutil::miTime obsTime;

  //metar
  miutil::miString metarId;
  bool CAVOK;              
  vector<miutil::miString> REww;   ///< Recent weather
  vector<miutil::miString> ww;     ///< Significant weather
  vector<miutil::miString> cloud;  ///< Clouds
  miutil::miString appendix;       ///< For whatever remains
  
  map<miutil::miString,float> fdata;

  //Hqc  
  map<miutil::miString,miutil::miString> flag; 
  map<miutil::miString,Colour> flagColour; 
};

#endif



