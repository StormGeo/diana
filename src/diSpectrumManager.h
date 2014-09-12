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
#ifndef SPECTRUMMANAGER_H
#define SPECTRUMMANAGER_H

#include <diCommonTypes.h>
#include <diPrintOptions.h>

#include <puTools/miTime.h>

#include <vector>
#include <map>
#include <set>

class SpectrumOptions;
class SpectrumFile;
class SpectrumPlot;

/**
   \brief Managing Wave Spectrum data sources and plotting
*/
class SpectrumManager
{
private:

  struct StationPos {
    float latitude;
    float longitude;
    std::string obs;
  };

  // map<model,filename>
  std::map<std::string,std::string> filenames;

  // for use in dialog (unique lists in setup order)
  std::vector<std::string> dialogModelNames;
  std::vector<std::string> dialogFileNames;

  SpectrumOptions *spopt;
  std::vector<SpectrumFile*> spfile;
  bool asField;

  std::vector<std::string> nameList;
  std::vector<float> latitudeList;
  std::vector<float> longitudeList;
  std::vector<miutil::miTime>   timeList;

  std::vector<std::string> fieldModels;
  std::vector<std::string> selectedModels;
  std::vector<std::string> selectedFiles;
  std::set<std::string> usemodels;

  int plotw, ploth;

  std::string plotStation;
  std::string lastStation;
  miutil::miTime   plotTime;
  miutil::miTime   ztime;

  bool dataChange;
  std::vector<SpectrumPlot*> spectrumplots;

  bool hardcopy;
  printOptions printoptions;
  bool hardcopystarted;

  std::map<std::string,std::string> menuConst;

  std::string getDefaultModel();
  bool initSpectrumFile(std::string file,std::string model);
  void initStations();
  void initTimes();
  void preparePlot();

public:
  // constructor
  SpectrumManager();
  // destructor
  ~SpectrumManager();

  void parseSetup();
  SpectrumOptions* getOptions() { return spopt; }
  void setPlotWindow(int w, int h);

  //routines from controller
  std::vector<std::string> getLineThickness();

  void setModel();
  void setStation(const std::string& station);
  void setTime(const miutil::miTime& time);
  std::string setStation(int step);
  miutil::miTime setTime(int step);

  const miutil::miTime getTime(){return plotTime;}
  const std::string getStation() { return plotStation; }
  const std::string getLastStation() { return lastStation; }
  const std::vector<std::string>& getStationList() { return nameList; }
  const std::vector<float>& getLatitudes() { return latitudeList; }
  const std::vector<float>& getLongitudes() { return longitudeList; }
  const std::vector<miutil::miTime>& getTimeList() { return timeList; }

  std::vector<std::string> getModelNames();
  std::vector<std::string> getModelFiles();
  void setFieldModels(const std::vector<std::string>& fieldmodels);
  void setSelectedModels(const std::vector<std::string>& models, bool field);
  void setSelectedFiles(const std::vector<std::string>& files, bool field);

  std::vector<std::string> getSelectedModels();

  bool plot();
  void startHardcopy(const printOptions& po);
  void endHardcopy();
  void mainWindowTimeChanged(const miutil::miTime& time);
  std::string getAnnotationString();

  void setMenuConst(std::map<std::string,std::string> mc)
    { menuConst = mc;}

  std::vector<std::string> writeLog();
  void readLog(const std::vector<std::string>& vstr,
      const std::string& thisVersion, const std::string& logVersion);
};

#endif
