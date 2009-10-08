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
#ifndef SPECTRUMOPTIONS_H
#define SPECTRUMOPTIONS_H

#include <puTools/miString.h>
#include <vector>

using namespace std;

/**
  \brief Wave Spectrum diagram settings

   Contains diagram layout settings and defaults.
*/
class SpectrumOptions
{
public:
  SpectrumOptions();
  ~SpectrumOptions();
  void setDefaults();

  // log and setup
  vector<miString> writeOptions();
  void readOptions(const vector<miString>& vstr);

private:
  friend class SpectrumSetupDialog;
  friend class SpectrumPlot;
  friend class SpectrumManager;

  bool changed;

  bool     pText;
  miString textColour;

  bool     pFixedText;
  miString fixedTextColour;

  bool     pFrame;
  miString frameColour;
  float    frameLinewidth;

  bool     pSpectrumLines;
  miString spectrumLineColour;
  float    spectrumLinewidth;

  bool     pSpectrumColoured;

  bool     pEnergyLine;
  miString energyLineColour;
  float    energyLinewidth;

  bool     pEnergyColoured;
  miString energyFillColour;

  bool     pWind;
  miString windColour;
  float    windLinewidth;

  bool     pPeakDirection;
  miString peakDirectionColour;
  float    peakDirectionLinewidth;

  float    freqMax;

  miString backgroundColour;

};

#endif

