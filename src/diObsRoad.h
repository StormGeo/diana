/*
  Diana - A Free Meteorological Visualisation Tool

  $Id: diObsAscii.h 1 2007-09-12 08:06:42Z lisbethb $

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
#ifndef diObsRoad_h
#define diObsRoad_h

// if not defined ROADOBS, just an empty include file
//#define ROADOBS 1
#ifdef ROADOBS
#include "diStationInfo.h"
#include "diRoadObsPlot.h"
#include "diObsData.h"
#include "diObsMetaData.h"
#include "diVprofPlot.h"
#include "diCommonTypes.h"
#include "diStationInfo.h"

#include <puTools/miTime.h>

#include <map>
#include <set>
#include <string>
#include <vector>

/**
  \brief Reading road observation data
  In-house smhi.se format
*/
class ObsRoad {
private:
  std::string filename_;
  std::string databasefile_;
  std::string stationfile_;
  std::string headerfile_;
	// This defines a set of stations, eg synop,metar,ship observations
  std::vector<road::diStation> * stationlist;
  miutil::miTime filetime_;
  void readHeader(RoadObsPlot *oplot);
  void readRoadData(RoadObsPlot *oplot);
  void initRoadData(RoadObsPlot *oplot);
	bool headerRead;

	// from ObsAscii class
	bool m_needDataRead;
  std::string m_filename;

  std::vector<std::string> lines;
  std::vector<ObsData> vObsData;
  std::map< std::string, ObsData > mObsData;
  std::string separator;
  bool fileOK;
  bool knots;
  miutil::miTime plotTime;
  miutil::miTime fileTime;
  int    timeDiff;

  std::set<std::string> asciiColumnUndefined;
  typedef std::map<std::string, size_t> string_size_m;
  string_size_m asciiColumn; //column index(time, x,y,dd,ff etc)
  int  asciiSkipDataLines;
  PlotCommand_cpv labels;

  std::vector<std::string> m_columnType;
  std::vector<std::string> m_columnName;
  std::vector<std::string> m_columnTooltip;

  void readDecodeData();
  void readData(const std::string &filename);
  
  void decodeData();
  string_size_m::const_iterator getColumn(const std::string& cn, const std::vector<std::string>& cv) const;
  bool getColumnValue(const std::string& cn, const std::vector<std::string>& cv, float& value) const;
  bool getColumnValue(const std::string& cn, const std::vector<std::string>& cv, int& value) const;
  bool getColumnValue(const std::string& cn, const std::vector<std::string>& cv, std::string& value) const;

  void readHeaderInfo(const std::string& filename, const std::string& headerfile,
      const std::vector<std::string>& headerinfo);
  void decodeHeader();
  bool bracketContents(std::vector<std::string>& in_out);
  void parseHeaderBrackets(const std::string& str);

  void addStationsToUrl(std::string& filename);



public:
  ObsRoad(const std::string &filename, const std::string &databasefile,
	  const std::string &stationfile, const std::string &headerfile,
	  const miutil::miTime &filetime, RoadObsPlot *oplot, bool breadData);
  void readData(RoadObsPlot *oplot);
  void initData(RoadObsPlot *oplot);
  
  void cloud_type_string(ObsData& d, double v);
  std::string height_of_clouds_string(double height);
  // from ObsBufr
  void cloud_type(ObsData& d, double v);
  float height_of_clouds(double height);
  float ms2code4451(float v);
  float percent2oktas(float v);

	// from ObsAscii
	void yoyoPlot(const miutil::miTime &filetime, RoadObsPlot *oplot);
  void yoyoMetadata(ObsMetaData *metaData);

  bool asciiOK() const
    { return fileOK; }
  bool parameterType(const std::string& param) const
    { return asciiColumn.count(param); }
  size_t columnCount() const
    { return m_columnName.size(); }
  std::string columnName(int idx) const
    { return m_columnName.at(idx); }
  std::string columnTooltip(int idx) const
    { return m_columnTooltip.at(idx); }
    
  VprofPlot* getVprofPlot(const std::string& modelName,
	     const std::string& station,
			 const miutil::miTime& time);
       
  void getStationList(vector<stationInfo> & stations);
};
#endif // ROADOBS
#endif // diObsRoad_h
