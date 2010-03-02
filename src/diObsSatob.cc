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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef METNOOBS

#include <diObsSatob.h>

using namespace::miutil;

ObsSatob::ObsSatob(const miString &file)
:satob(file){
}

void ObsSatob::init(ObsPlot *oplot){

  //It is assumed that each station has only one entry in the satob
  //file, and that fileObsTime == ObsTime

  int numStations= contents.size();

  for(int i=0; i<numStations; i++)
    if(oplot->timeOK(contents[i].desc.obsTime)){
      ObsData &d = oplot->getNextObs();
      putData(i,d);
    }

}


void
ObsSatob::putData(int i, ObsData &d){

  d.dataType = "satob";
    //Description
  d.id = contents[i].desc.kjennetegn;
  d.id.trim();
  d.xpos =  contents[i].desc.pos.longitude();
  d.ypos = contents[i].desc.pos.latitude();
  d.zone = _IDtoZone(contents[i].stationID);
  d.obsTime = contents[i].desc.obsTime;

  //Data
  if(contents[i].data.dd != undef)
    d.fdata["dd"]    = (float)contents[i].data.dd;		// grader
  if(contents[i].data.ff != undef)
    d.fdata["ff"]    = knots2ms(contents[i].data.ff);		// knop

}

#endif






