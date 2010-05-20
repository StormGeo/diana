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
#ifndef diVprofPilot_h
#define diVprofPilot_h

#include <robs/pilot.h>
#include <puTools/miTime.h>

using namespace std;

class VprofPlot;

/**

  \brief Reading sounding from met.no obs files (pilot)
  
   using the robs library 

*/
class VprofPilot : public pilot {
public:
  // Constructors
  VprofPilot();
  VprofPilot(const miutil::miString& file,
	     const vector<miutil::miString>& stationList);
  VprofPilot(const miutil::miString& file,
	     float latitude, float longitude,
	     float deltalat, float deltalong);
  // Destructor
  ~VprofPilot();

  miutil::miTime getFileObsTime();

  VprofPlot* getStation(const miutil::miString& station,
			const miutil::miTime& time);
};

#endif