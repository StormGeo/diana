/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2013 met.no

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

/*
 NOTES: Heavily based on old fortran code (1987-2001, A.Foss)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diVcross1Plot.h"
#include "diVcross1Options.h"
#include "diCommandParser.h"
#include "diContouring.h"

#include <qglobal.h>

#include <puTools/miSetupParser.h>
#include <diField/diMetConstants.h>

#include <GL/gl.h>
#if !defined(USE_PAINTGL)
#include <glText/glText.h>
#include <glp/glpfile.h>
#endif

#include <boost/foreach.hpp>

#include <cmath>
#include <iterator>
#include <iomanip>

#define MILOGGER_CATEGORY "diana.VcrossPlot"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

namespace {
void LOG_2D(float* data, int maxi, int n, const char* name)
{
  std::ostringstream debug;
  std::copy(data, data+std::min(n, maxi), std::ostream_iterator<float>(debug, " "));
  if (n < maxi) {
    debug << " ..... ";
    std::copy(data + std::max(0, maxi-n), data+maxi, std::ostream_iterator<float>(debug, " "));
  }
  METLIBS_LOG_DEBUG("computer " << name << " = " << debug.str());
}
}

// static
FontManager* VcrossPlot::fp = 0; // fontpack
GridConverter VcrossPlot::gc; // Projection-converter

GLPfile* VcrossPlot::psoutput = 0; // PostScript module
bool VcrossPlot::hardcopy = false; // producing postscript

map<std::string, int> VcrossPlot::vcParName; // name -> number
map<int, std::string> VcrossPlot::vcParNumber; // number -> name
multimap<std::string, vcFunction> VcrossPlot::vcFunctions;
map<std::string, vcField> VcrossPlot::vcFields;
vector<std::string> VcrossPlot::vcFieldNames; // setup/dialog sequence
map<std::string, FileContents> VcrossPlot::fileContents;

int VcrossPlot::plotw = 100;
int VcrossPlot::ploth = 100;
int VcrossPlot::numplot = 1;
int VcrossPlot::nplot = 0;

float VcrossPlot::fontsize = 0.;
float VcrossPlot::fontscale = 0.;
float VcrossPlot::chxdef = 0.;
float VcrossPlot::chydef = 0.;
float VcrossPlot::xFullWindowmin = 0.;
float VcrossPlot::xFullWindowmax = 0.;
float VcrossPlot::yFullWindowmin = 0.;
float VcrossPlot::yFullWindowmax = 0.;
float VcrossPlot::xWindowmin = 0.;
float VcrossPlot::xWindowmax = 0.;
float VcrossPlot::yWindowmin = 0.;
float VcrossPlot::yWindowmax = 0.;
float VcrossPlot::xPlotmin = 0.;
float VcrossPlot::xPlotmax = 0.;
float VcrossPlot::yPlotmin = 0.;
float VcrossPlot::yPlotmax = 0.;
Colour VcrossPlot::backColour = Colour();
Colour VcrossPlot::contrastColour = Colour();

VcrossZoomType VcrossPlot::zoomType = vczoom_standard;
int VcrossPlot::zoomSpec[4] =
{ 0, 0, 0, 0 };

miTime VcrossPlot::timeGraphReference = miTime(2002, 1, 1, 0, 0);
float VcrossPlot::timeGraphMinuteStep = 0.;

bool VcrossPlot::bottomStencil = false;

vector<VcrossText> VcrossPlot::vcText;

//#define DEBUGPRINT 1
//#define DEBUGCONTOUR 1

// Default constructor
VcrossPlot::VcrossPlot() :
      vcopt(0), timeGraph(false), horizontalPosNum(0), nPoint(0), numLev(0),
      nTotal(0)
{
  METLIBS_LOG_SCOPE();

  // data pointers (cdata1d, single level data)
  nps = nxg = nyg = nxs = nsin = ncos = npy1 = npy2 = nlat = nlon = -1;
  ngdir1 = ngdir2 = npss = npsb = ncor = nxds = ntopo = -1;

  // data pointers (cdata2d, multilevel data)
  npp = nzz = npi = nx = ny = nwork = -1;

  iundef = 1;

  vcoordPlot = vcv_none;
  verticalAxis = "none";
  v2hRatio = -1.;

  if (!fp) {
    fp = new FontManager();
    fp->parseSetup();
    fp->setFont("BITMAPFONT");
    fp->setFontFace(glText::F_NORMAL);
    fp->setScalingType(glText::S_FIXEDSIZE);
  }
}

VcrossPlot::~VcrossPlot()
{
  METLIBS_LOG_SCOPE();

  int numPar1d = cdata1d.size();
  for (int n = 0; n < numPar1d; n++)
    delete[] cdata1d[n];

  int numPar2d = cdata2d.size();
  for (int n = 0; n < numPar2d; n++)
    delete[] cdata2d[n];
}

// static function
bool VcrossPlot::parseSetup()
{
  METLIBS_LOG_SCOPE();

  if (numVcrossFunctions != vcf_no_function) {
    METLIBS_LOG_ERROR("PROGRAM ERROR: numVcrossFunctions !!!!!!!!");
    return false;
  }

  if (numVcrossPlotTypes != vcpt_no_plot) {
    METLIBS_LOG_ERROR("PROGRAM ERROR: numVcrossPlotTypes !!!!!!!!");
    return false;
  }

  std::string vcrossFunctionNames[numVcrossFunctions];
  int vcrossFunctionArguments[numVcrossFunctions];

  for (int f = 0; f < numVcrossFunctions; f++) {
    size_t k1 = vcrossFunctionDefs[f].rfind('(');
    size_t k2 = vcrossFunctionDefs[f].rfind(')');
    size_t kl = vcrossFunctionDefs[f].length();
    if (k1 == string::npos || k2 == string::npos || k2 < k1 + 2 || k2 != kl - 1) {
      METLIBS_LOG_ERROR("PROGRAM ERROR IN vcrossFunctionDefs !!!!!!!!");
      return false;
    }
    int narg = 1;
    for (size_t k = k1 + 1; k < k2; k++)
      if (vcrossFunctionDefs[f][k] == ',')
        narg++;
    vcrossFunctionNames[f] = vcrossFunctionDefs[f].substr(0, k1);
    vcrossFunctionArguments[f] = narg;
  }

  std::string vcrossPlotNames[numVcrossPlotTypes];
  int vcrossPlotArguments[numVcrossPlotTypes];

  for (int p = 0; p < numVcrossPlotTypes; p++) {
    size_t k1 = vcrossPlotDefs[p].rfind('(');
    size_t k2 = vcrossPlotDefs[p].rfind(')');
    size_t kl = vcrossPlotDefs[p].length();
    if (k1 == string::npos || k2 == string::npos || k2 < k1 + 2 || k2 != kl - 1) {
      METLIBS_LOG_ERROR("PROGRAM ERROR: vcrossPlotDefs !!!!!!!!");
      return false;
    }
    int narg = 1;
    for (size_t k = k1 + 1; k < k2; k++)
      if (vcrossPlotDefs[p][k] == ',')
        narg++;
    vcrossPlotNames[p] = vcrossPlotDefs[p].substr(0, k1);
    vcrossPlotArguments[p] = narg;
  }

  //------------------------------------------------

  const std::string section2 = "VERTICAL_CROSSECTION_PARAMETERS";
  const std::string section3 = "VERTICAL_CROSSECTION_COMPUTATIONS";
  const std::string section4 = "VERTICAL_CROSSECTION_PLOTS";

  vcParNumber.clear();
  vcParName.clear();
  vcFunctions.clear();
  vcFields.clear();
  vcFieldNames.clear();

  set<std::string> definedNames;
  vector<std::string> vstr, tokens, parts, names;
  std::string msg, str, name, func;
  int nvstr, nt, number;

  bool error = false;

  // section2 = VERTICAL_CROSSECTION_PARAMETERS

  if (SetupParser::getSection(section2, vstr)) {

    nvstr = vstr.size();

    for (int l = 0; l < nvstr; l++) {

      tokens = miutil::split(miutil::to_lower(vstr[l]), 0, " ");
      nt = tokens.size();

      for (int t = 0; t < nt; t++) {
        parts = miutil::split(tokens[t], 0, "=");
        if (parts.size() == 2) {
          if (CommandParser::isInt(parts[1])) {
            name = parts[0];
            number = atoi(parts[1].c_str());
            if (vcParName.find(name) == vcParName.end() && vcParNumber.find(
                number) == vcParNumber.end()) {
              vcParName[name] = number;
              vcParNumber[number] = name;
              definedNames.insert(name);
            } else {
              msg = "Illegal redefinition: " + tokens[t];
            }
          } else {
            msg = "Bad definition: " + tokens[t];
          }
        } else {
          msg = "Not a definition: " + tokens[t];
        }
        if (!msg.empty()) {
          SetupParser::errorMsg(section2, l, msg);
          error = true;
          msg.clear();
        }
      }
    }
    vstr.clear();

  } else {
//    METLIBS_LOG_DEBUG("Missing section " << section2 << " in setupfile.");
    //error= true;
  }

  // section3 = VERTICAL_CROSSECTION_COMPUTATIONS

  if (SetupParser::getSection(section3, vstr)) {

    nvstr = vstr.size();

    for (int l = 0; l < nvstr; l++) {

      tokens = miutil::split(miutil::to_lower(vstr[l]), 0, " ");
      nt = tokens.size();

      for (int t = 0; t < nt; t++) {
        parts = miutil::split(tokens[t], 0, "=");
        if (parts.size() == 2) {
          name = parts[0];
          definedNames.insert(name);
          size_t k1 = parts[1].rfind('(');
          size_t k2 = parts[1].rfind(')');
          if (k1 != string::npos && k2 != string::npos && k2 > k1 + 1) {
            func = parts[1].substr(0, k1);
            str = parts[1].substr(k1 + 1, k2 - k1 - 1);
            names = miutil::split(str, 0, ",");
            int j = 0;
            while (j < numVcrossFunctions && func != vcrossFunctionNames[j])
              j++;
            if (j < numVcrossFunctions && int(names.size()) == vcrossFunctionArguments[j]) {
              for (unsigned int i = 0; i < names.size(); i++) {
                if (definedNames.find(names[i]) == definedNames.end()) {
                  msg = "Input name not defined: " + names[i];
                }
              }
              vcFunction vcfunc;
              vcfunc.function = VcrossFunction(j);
              vcfunc.vars = names;
              vcFunctions.insert(pair<std::string, vcFunction> (name, vcfunc));
            } else {
              msg = "Bad function/arguments: " + tokens[t];
            }
          } else {
            msg = "Bad function: " + tokens[t];
          }
        } else {
          msg = "Not a definition: " + tokens[t];
        }
        if (!msg.empty()) {
          SetupParser::errorMsg(section3, l, msg);
          error = true;
          msg.clear();
        }
      }
    }
    vstr.clear();

  } else {
    //METLIBS_LOG_DEBUG("Missing section " << section3 << " in setupfile.");
    //error= true;
  }

  // section4 = VERTICAL_CROSSECTION_PLOTS

  if (SetupParser::getSection(section4, vstr)) {

    nvstr = vstr.size();

    for (int l = 0; l < nvstr; l++) {

      tokens = miutil::split(miutil::to_lower(vstr[l]), 0, " ");
      nt = tokens.size();

      vcField vcf;
      vcf.plotType = vcpt_no_plot;

      for (int t = 0; t < nt; t++) {
        parts = miutil::split(tokens[t], 0, "=");
        if (parts.size() == 2) {
          if (parts[0] == "name") {
            name = parts[1];
            if (vcFields.find(name) == vcFields.end()) {
              // resplit line to get mixedcase name
              vector<std::string> tokens2 = miutil::split(vstr[l], 0, " ");
              vector<std::string> parts2 = miutil::split(tokens2[t], 0, "=");
              vcf.name = parts2[1];
            } else {
              msg = "Illegal redefinition: " + name;
            }
          } else if (parts[0] == "plot") {
            size_t k1 = parts[1].rfind('(');
            size_t k2 = parts[1].rfind(')');
            if (k1 != string::npos && k2 != string::npos && k2 > k1 + 1) {
              func = parts[1].substr(0, k1);
              str = parts[1].substr(k1 + 1, k2 - k1 - 1);
              names = miutil::split(str, 0, ",");
              int j = 0;
              while (j < numVcrossPlotTypes && func != vcrossPlotNames[j])
                j++;
              if (j < numVcrossPlotTypes && int(names.size())
              == vcrossPlotArguments[j]) {
                for (unsigned int i = 0; i < names.size(); i++) {
                  if (definedNames.find(names[i]) == definedNames.end()) {
                    msg = "Input name not defined: " + names[i];
                  }
                }
                vcf.vars = names;
                vcf.plotType = VcrossPlotType(j);
              } else {
                msg = "Bad definition: " + tokens[t];
              }
            } else {
              msg = "Bad definition: " + tokens[t];
            }
          } else {
            // can only hope this is a sensible plot option...
            if (vcf.plotOpts.empty())
              vcf.plotOpts = tokens[t];
            else
              vcf.plotOpts += (" " + tokens[t]);
          }
        } else {
          // any plot options without key=value syntax ?????
          if (vcf.plotOpts.empty())
            vcf.plotOpts = tokens[t];
          else
            vcf.plotOpts += (" " + tokens[t]);
        }
        if (!msg.empty()) {
          SetupParser::errorMsg(section4, l, msg);
          error = true;
          msg.clear();
        }
      }
      if ((not vcf.name.empty()) and vcf.plotType != vcpt_no_plot) {
        name = miutil::to_lower(vcf.name);
        vcFields[name] = vcf;
        vcFieldNames.push_back(name);
      }
    }
    vstr.clear();

  } else {
    //METLIBS_LOG_DEBUG("Missing section " << section4 << " in setupfile.");
    //error= true;
  }

  return (!error);
}

// static function
void VcrossPlot::makeContents(const std::string& fileName,
    const vector<int>& iparam, int vcoord)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(fileName));

  FileContents fco;

  set<std::string> paramdef;

  int np = iparam.size();

  map<int, std::string>::iterator pn, pnend = vcParNumber.end();

  for (int i = 0; i < np; i++) {
    pn = vcParNumber.find(iparam[i]);
    if (pn == pnend) {
      // not warn if height or pressure (but keep it if defined)
      if (iparam[i] != 1 && iparam[i] != 8)
        METLIBS_LOG_INFO("VCROSS: parameter not defined in setup: " << iparam[i]);
    } else {
      paramdef.insert(pn->second);
      //##############################################################
      //      METLIBS_LOG_DEBUG(" input param: "<<iparam[i]<<"  "<<pn->second);
      //##############################################################
    }
  }

  multimap<std::string, vcFunction>::iterator vf, vfbegin, vfend;
  vfbegin = vcFunctions.begin();
  vfend = vcFunctions.end();

  set<std::string> paramtmp;
  set<std::string>::iterator pt, ptend;

  unsigned int numfound = 0;

  while (paramdef.size() > numfound) {

    numfound = paramdef.size();
    paramtmp.clear();

    for (vf = vfbegin; vf != vfend; vf++) {
      if (paramdef.find(vf->first) == paramdef.end()) {
        int n = vf->second.vars.size();
        int i = 0;
        while (i < n && paramdef.find(vf->second.vars[i]) != paramdef.end())
          i++;
        if (i == n) {
          //##############################################################
          //     METLIBS_LOG_DEBUG(" function param: "<<vf->first);
          //##############################################################
          paramtmp.insert(vf->first);
          fco.useFunctions[vf->first] = vf->second;
        }
      }
    }

    // add the latest parameters here, not above (to get the shortest
    // route between the input data and a function or plotfield)
    pt = paramtmp.begin();
    ptend = paramtmp.end();
    while (pt != ptend) {
      paramdef.insert(*pt);
      pt++;
    }
    METLIBS_LOG_DEBUG(LOGVAL(numfound));
  }
#ifdef DEBUGPRINT
  pt= paramdef.begin();
  ptend= paramdef.end();
  while (pt!=ptend) {
    METLIBS_LOG_DEBUG("      paramdef: "<<*pt);
    pt++;
  }
#endif

  map<std::string, vcField>::iterator fend = vcFields.end();
  map<std::string, vcField>::iterator f;
  set<std::string>::iterator pend = paramdef.end();

  int m = vcFieldNames.size();

  for (int j = 0; j < m; j++) {

    std::string fname = vcFieldNames[j];
    f = vcFields.find(fname);

    if (f != fend) {

      int n = f->second.vars.size();
      int i = 0;

      while (i < n && paramdef.find(f->second.vars[i]) != pend)
        i++;
      //##      if (i==n && f->second.plotType==vcpt_vt_omega &&
      //##		           (vcoord==5 || vcoord==11)) i=0;
      //##      if (i==n && f->second.plotType==vcpt_vt_w &&
      //##		           (vcoord!=5 && vcoord!=11)) i=0;
      if (i == n) {
        fco.fieldNames.push_back(f->second.name);
        //#ifdef DEBUGPRINT
        //	METLIBS_LOG_DEBUG("   field: "<<f->second.name<<"   found:");
        //	for (i=0; i<n; i++) METLIBS_LOG_DEBUG(" "<<f->second.vars[i]);
        //	METLIBS_LOG_DEBUG();
        //#endif
      }
    }
  }

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("  no. of output fields: "<<fco.fieldNames.size());
  for (int j=0; j<fco.fieldNames.size(); j++)
    METLIBS_LOG_DEBUG(setw(5)<<j<<": "<<fco.fieldNames[j]);
#endif

  fileContents[fileName] = fco;
}

// static function
void VcrossPlot::deleteContents(const std::string& fileName)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(fileName));

  fileContents.erase(fileName);
}

// static function
vector<std::string> VcrossPlot::getFieldNames(const std::string& fileName)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(fileName));

  map<std::string, FileContents>::iterator p = fileContents.find(fileName);
  if (p != fileContents.end()) {
    return p->second.fieldNames;
  } else {
    return vector<std::string>();
  }
}

// static function
map<std::string, std::string> VcrossPlot::getAllFieldOptions()
{
  METLIBS_LOG_SCOPE();

  map<std::string, std::string> fieldopts;

  Linetype defaultLinetype = Linetype::getDefaultLinetype();

  map<std::string, vcField>::iterator vcf = vcFields.begin();
  map<std::string, vcField>::iterator vcfend = vcFields.end();

  while (vcf != vcfend) {
    PlotOptions po;
    PlotOptions::parsePlotOption(vcf->second.plotOpts, po);

    if (po.linetype.name.length() == 0)
      po.linetype = defaultLinetype;
    if (po.undefLinetype.name.length() == 0)
      po.undefLinetype = defaultLinetype;

    ostringstream ostr;

    if (vcf->second.plotType == vcpt_contour) {
      bool usebase = false;
      if (po.colours.size() < 2 || po.colours.size() > 3) {
        ostr << "colour=" << po.linecolour.Name();
      } else {
        ostr << "colours=" << po.colours[0].Name();
        for (unsigned int j = 1; j < po.colours.size(); j++)
          ostr << "," << po.colours[j].Name();
        usebase = true;
      }
      if (po.linetypes.size() < 2 || po.linetypes.size() > 3) {
        ostr << " linetype=" << po.linetype.name;
      } else {
        ostr << " linetypes=" << po.linetypes[0].name;
        for (unsigned int j = 1; j < po.linetypes.size(); j++)
          ostr << "," << po.linetypes[j].name;
        usebase = true;
      }
      if (po.linewidths.size() < 2 || po.linewidths.size() > 3) {
        ostr << " linewidth=" << po.linewidth;
      } else {
        ostr << " linewidths=" << po.linewidths[0];
        for (unsigned int j = 1; j < po.linewidths.size(); j++)
          ostr << "," << po.linewidths[j];
        usebase = true;
      }
      //      if (usebase) ostr << " base=" << po.base;
      // as used in plotting :
      if (po.linevalues.size() == 0 && po.loglinevalues.size() == 0) {
        ostr << " line.interval=" << po.lineinterval;
        if (po.zeroLine >= 0)
          ostr << " zero.line=" << po.zeroLine;
      }
      ostr << " line.smooth=" << po.lineSmooth << " value.label="
          << po.valueLabel << " label.size=" << po.labelSize;
      ostr << " base=" << po.base;
      if (po.minvalue > -fieldUndef)
        ostr << " minvalue=" << po.minvalue;
      else
        ostr << " minvalue=off";
      if (po.maxvalue < fieldUndef)
        ostr << " maxvalue=" << po.maxvalue;
      else
        ostr << " maxvalue=off";
      if (!usebase) {
        int n = po.palettecolours.size();
        if (n > 0) {
          ostr << " palettecolours=" << po.palettecolours[0];
          for (int j = 1; j < n; j++)
            ostr << "," << po.palettecolours[j].Name();
        } else {
          ostr << " palettecolours=off";
        }
        ostr << " table=" << po.table;
        ostr << " repeat=" << po.repeat;
      }
    } else if (vcf->second.plotType == vcpt_wind) {
      ostr << "colour=" << po.linecolour.Name() << " linewidth="
          << po.linewidth << " density=" << po.density;
      //  } else if (po.fptype==fpt_wind_colour) {
      //    ostr << "linewidth="   << po.linewidth
      //	   << " density="    << po.density;
    } else if (vcf->second.plotType == vcpt_vector) {
      ostr << "colour=" << po.linecolour.Name() << " linewidth="
          << po.linewidth << " density=" << po.density << " vector.unit="
          << po.vectorunit;
      //  } else if (po.fptype==fpt_vector_colour) {
      //    ostr << "linewidth="   << po.linewidth
      //	   << " density="    << po.density
      //	   << " vector.unit="<< po.vectorunit
    } else if (vcf->second.plotType == vcpt_vt_omega || vcf->second.plotType == vcpt_vt_w) {
      ostr << "colour=" << po.linecolour.Name() << " linewidth="
          << po.linewidth << " density=" << po.density << " vector.unit="
          << po.vectorunit;
      //  } else if (po.fptype==fpt_direction_colour) {
      //    ostr << "linewidth="   << po.linewidth
      //	   << " density="    << po.density
      //  } else if (po.fptype==fpt_box_pattern) {
      //    ostr << "colour="      << po.linecolour.Name();
      //  } else if (po.fptype==fpt_box_alpha_shade) {
      //    ostr << "colour="      << po.linecolour.Name();
      //  } else if (po.fptype==fpt_alpha_shade) {
      //    ostr << "colour="      << po.linecolour.Name();
    } else {
      ostr << "colour=" << po.linecolour.Name();
    }

    //    std::string ucn="white";  // default WhiteC has no name!
    //    if (po.undefColour.Name().exists()) ucn= po.undefColour.Name();
    //
    //    ostr << " undef.masking="   << po.undefMasking
    //       //<< " undef.colour="    << po.undefColour.Name()
    //	 << " undef.colour="    << ucn
    //	 << " undef.linewidth=" << po.undefLinewidth
    //	 << " undef.linetype="  << po.undefLinetype.name;

    std::string opts = ostr.str();

    fieldopts[vcf->second.name] = opts;
    vcf++;
  }

  return fieldopts;
}

// static function
std::string VcrossPlot::getFieldOptions(const std::string& fieldname)
{
  METLIBS_LOG_SCOPE();

  const std::map<std::string, vcField>::const_iterator vcf = vcFields.find(miutil::to_lower(fieldname));
  if (vcf == vcFields.end())
    return std::string();
  
  return vcf->second.plotOpts;
}

// static
void VcrossPlot::startPlot(int numplots, VcrossOptions *vcoptions)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(numplots));

  numplot = numplots;
  nplot = 0;

  bottomStencil = false;

  vcText.clear();

  Colour cback(vcoptions->backgroundColour);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glClearColor(cback.fR(), cback.fG(), cback.fB(), 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
}

// static
void VcrossPlot::plotText()
{
  METLIBS_LOG_SCOPE();

  int n = vcText.size();
  if (n == 0)
    return;

  fp->setFont("BITMAPFONT");

  fontsize = 10.;
  fp->setFontSize(fontsize);

  float wspace, w, h;
  fp->getStringSize("oo", wspace, h);

  float wmod = 0., wcrs = 0., wfn = 0, wfc = 0, wtime = 0;

  vector<std::string> fctext(n);

  for (int i = 0; i < n; i++) {
    fp->getStringSize(vcText[i].modelName.c_str(), w, h);
    if (wmod < w)
      wmod = w;
    fp->getStringSize(vcText[i].crossectionName.c_str(), w, h);
    if (wcrs < w)
      wcrs = w;
    fp->getStringSize(vcText[i].fieldName.c_str(), w, h);
    if (wfn < w)
      wfn = w;
    if (!vcText[i].timeGraph) {
      ostringstream ostr;
      ostr << "(" << setiosflags(ios::showpos) << vcText[i].forecastHour << ")";
      fctext[i] = ostr.str();
      fp->getStringSize(fctext[i].c_str(), w, h);
      if (wfc < w)
        wfc = w;
      std::string ts = vcText[i].validTime.isoTime();
      fp->getStringSize(ts.c_str(), w, h);
      if (wtime < w)
        wtime = w;
    }
  }

  float xmod = xWindowmin + wspace;
  float xcrs = xmod + wmod + wspace;
  float xfn = xcrs + wcrs + wspace;
  float xfc = xfn + wfn + wspace;
  float xtime = xfc + wfc + wspace;
  float xextreme = xtime + wtime + wspace;

  float dy = chydef * 2;
  float y = yWindowmin + dy * n;

  for (int i = 0; i < n; i++) {
    glColor3ubv(vcText[i].colour.RGB());
    fp->drawStr(vcText[i].modelName.c_str(), xmod, y, 0.0);
    fp->drawStr(vcText[i].crossectionName.c_str(), xcrs, y, 0.0);
    fp->drawStr(vcText[i].fieldName.c_str(), xfn, y, 0.0);
    if (!vcText[i].timeGraph) {
      fp->drawStr(fctext[i].c_str(), xfc, y, 0.0);
      std::string ts = vcText[i].validTime.isoTime();
      fp->drawStr(ts.c_str(), xtime, y, 0.0);
    }
    fp->drawStr(vcText[i].extremeValueString.c_str(), xextreme, y, 0.0);
    y -= dy;
  }

}

bool VcrossPlot::plot(VcrossOptions *vcoptions, const std::string& fieldname, PlotOptions& poptions)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(fieldname));

  vcopt = vcoptions;

  VcrossVertical vcplot = vcv_none;
  std::string vcaxis;

  vector<std::string> vs = miutil::split(miutil::to_lower(vcopt->verticalType), 0, "/");

  if (vs.size() == 1) {
    vs.push_back("x");
  } else if (vs.size() == 0) {
    vs.push_back("standard");
    vs.push_back("x");
  }

  if (vcoord == 5) {
    vcplot = vcv_height; // no choice for ocean depths
    vcaxis = "m";
  } else if (vcoord == 11) {
    vcplot = vcv_height; // no choice for vc=11, MC2 ???????????
    if (vs[1] == "fl" || vs[1] == "ft")
      vcaxis = "Ft"; // FL correct name here ???
    else
      vcaxis = "m";
  } else if (vs[0] == "pressure" && npp >= 0) {
    vcplot = vcv_pressure;
    if (vs[1] == "fl")
      vcaxis = "FL";
    else
      vcaxis = "hPa";
  } else if (vs[0] == "height" && nzz >= 0) {
    vcplot = vcv_height;
    if (vs[1] == "fl" || vs[1] == "ft")
      vcaxis = "Ft"; // FL correct name here ???
    else
      vcaxis = "m";
  } else if (npp >= 0) {
    vcplot = vcv_exner;
    if (vs[1] == "fl")
      vcaxis = "FL";
    else
      vcaxis = "hPa";
  } else if (nzz >= 0) {
    vcplot = vcv_height;
    if (vs[1] == "fl" || vs[1] == "ft")
      vcaxis = "Ft"; // FL correct name here ???
    else
      vcaxis = "m";
  }

  if (vcplot == vcv_none)
    return false;

  verticalAxis = vcaxis;

  if (vcplot != vcoordPlot || vcopt->verHorRatio != v2hRatio) {
    vcoordPlot = vcplot;
    v2hRatio = vcopt->verHorRatio;
    prepareVertical();
  }

  if (nplot == 0) {
    computeSize();

    float w = plotw;
    float h = ploth;
    float x1 = xWindowmin;
    float x2 = xWindowmax;
    float y1 = yWindowmin;
    float y2 = yWindowmax;
    float dx = x2 - x1;
    float dy = y2 - y1;
    if (w / h > dx / dy) {
      dx = (dy * w / h - dx) * 0.5;
      x1 -= dx;
      x2 += dx;
    } else {
      dy = (dx * h / w - dy) * 0.5;
      y1 -= dy;
      y2 += dy;
    }

    xFullWindowmin = x1;
    xFullWindowmax = x2;
    yFullWindowmin = y1;
    yFullWindowmax = y2;

    glLoadIdentity();
    glOrtho(x1, x2, y1, y2, -1., 1.);

    fp->setVpSize(float(plotw), float(ploth));
    fp->setGlSize(x2 - x1, y2 - y1);
    fp->setFontSize(fontsize);
    fp->getCharSize('M', chxdef, chydef);
    // the real character height (width is ok)
    chydef *= 0.75;
    fontscale = fontsize / 0.75;
  }

  plotData(fieldname, poptions);

  nplot++;

  return true;
}

int VcrossPlot::addPar1d(int param, float *pdata)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(param));

  if (pdata == 0)
    pdata = new float[nPoint];

  int np = cdata1d.size();
  cdata1d.push_back(pdata);
  idPar1d.push_back(param);

  return np;
}

int VcrossPlot::addPar2d(int param, float *pdata)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(param));

  if (pdata == 0)
    pdata = new float[nTotal];

  int np = cdata2d.size();
  cdata2d.push_back(pdata);
  idPar2d.push_back(param);

  return np;
}

void VcrossPlot::setPlotWindow(int w, int h)
{
  plotw = w;
  ploth = h;

  if (hardcopy)
    resetPage();
}

bool VcrossPlot::startPSoutput(const printOptions& po)
{
  if (hardcopy)
    return false;
  printOptions pro = po;
  printerManager printman;

  int feedsize = 10000000;
  int print_options = 0;

  // Fit output to page
  if (pro.fittopage)
    print_options = print_options | GLP_FIT_TO_PAGE;

  // set colour mode
  if (pro.colop == d_print::greyscale) {
    print_options = print_options | GLP_GREYSCALE;

    if (pro.drawbackground)
      print_options = print_options | GLP_DRAW_BACKGROUND;

  } else if (pro.colop == d_print::blackwhite) {
    print_options = print_options | GLP_BLACKWHITE;

  } else {
    if (pro.drawbackground)
      print_options = print_options | GLP_DRAW_BACKGROUND;
  }

  // set orientation
  if (pro.orientation == d_print::ori_landscape)
    print_options = print_options | GLP_LANDSCAPE;
  else if (pro.orientation == d_print::ori_portrait)
    print_options = print_options | GLP_PORTRAIT;
  else
    print_options = print_options | GLP_AUTO_ORIENT;

  // calculate line, point (and font?) scale
  if (!pro.usecustomsize)
    pro.papersize = printman.getSize(pro.pagesize);
  d_print::PaperSize a4size;
  float scale = 1.0;
  if (abs(pro.papersize.vsize > 0))
    scale = a4size.vsize / pro.papersize.vsize;

  // check if extra output-commands
  map<string, string> extra;
  printman.checkSpecial(pro, extra);

  // make GLPfile object
  psoutput = new GLPfile(const_cast<char*> (pro.fname.c_str()), print_options,
      feedsize, &extra, pro.doEPS);

  // set line and point scale
  psoutput->setScales(0.5 * scale, 0.5 * scale);

  psoutput->StartPage();
  // set viewport
  psoutput->setViewport(pro.viewport_x0, pro.viewport_y0, pro.viewport_width, pro.viewport_height);
  hardcopy = true;

  // inform fontpack
  if (fp) {
    fp->startHardcopy(psoutput);
  }

  return true;
}

void VcrossPlot::addHCStencil(const int& size, const float* x, const float* y)
{
  if (!psoutput)
    return;
  psoutput->addStencil(size, x, y);
}

void VcrossPlot::addHCScissor(const int x0, const int y0, const int w,
    const int h)
{
  if (!psoutput)
    return;
  psoutput->addScissor(x0, y0, w, h);
}

void VcrossPlot::removeHCClipping()
{
  if (!psoutput)
    return;
  psoutput->removeClipping();
}

void VcrossPlot::UpdateOutput()
{
  // for postscript output
  if (psoutput)
    psoutput->UpdatePage(true);
}

void VcrossPlot::resetPage()
{
  // for postscript output
  if (psoutput)
    psoutput->addReset();
}

bool VcrossPlot::startPSnewpage()
{
  if (!hardcopy || !psoutput)
    return false;
  glFlush();
  if (psoutput->EndPage() != 0)
    METLIBS_LOG_ERROR("startPSnewpage: EndPage BAD!!!");
  psoutput->StartPage();
  return true;
}

bool VcrossPlot::endPSoutput()
{
  if (!hardcopy || !psoutput)
    return false;
  glFlush();
  if (psoutput->EndPage() == 0) {
    delete psoutput;
    psoutput = 0;
  }
  hardcopy = false;

  if (fp)
    fp->endHardcopy();

  return true;
}

// static
void VcrossPlot::getPlotSize(float& x1, float& y1, float& x2, float& y2,
    Colour& rubberbandColour)
{
  x1 = xWindowmin;
  y1 = yWindowmin;
  x2 = xWindowmax;
  y2 = yWindowmax;
  rubberbandColour = contrastColour;
}

// static
void VcrossPlot::decreasePart(int px1, int py1, int px2, int py2)
{
  zoomType = vczoom_decrease;
  zoomSpec[0] = px1;
  zoomSpec[1] = py1;
  zoomSpec[2] = px2;
  zoomSpec[3] = py2;
}

// static
void VcrossPlot::increasePart()
{
  zoomType = vczoom_increase;
}

// static
void VcrossPlot::movePart(int pxmove, int pymove)
{
  zoomType = vczoom_move;
  zoomSpec[0] = pxmove;
  zoomSpec[1] = pymove;
}

// static
void VcrossPlot::standardPart()
{
  zoomType = vczoom_standard;
}

bool VcrossPlot::prepareData(const std::string& fileName)
{
  METLIBS_LOG_SCOPE();

  // basic preparations after input of data,
  // may find data unplottable

  // Vertical coordinates handled:
  // vcoord:  2 = sigma (0.-1. with ps and ptop) ... Norlam
  //         10 = eta (hybrid) ... Hirlam, Ecmwf,...
  //          1 = pressure
  //          4 = isentropic surfaces (potential temp., with p(th))
  //          5 = z levels from sea model (possibly incl. sea elevation and bottom)
  //         11 = sigma height levels (MEMO,MC2)
  //         12 = sigma.MM5 (input P in all levels)


  METLIBS_LOG_DEBUG(LOGVAL(vcoord));
  if (vcoord != 2 && vcoord != 10 && vcoord != 1 && vcoord != 4 && vcoord != 5
      && vcoord != 11 && vcoord != 12)
  {
    METLIBS_LOG_ERROR("prepareData: Unknown vertical coordinate " << vcoord);
    return false;
  }

  int i, n;
  bool error = false;

  int numPar1d = idPar1d.size();
  int numPar2d = idPar2d.size();

  //##################################################################################
  //  for (n=0; n<numPar1d; n++) {
  //    float fmax=-1.e+38, fmin=+1.e+38;
  //    for (i=0; i<nPoint; i++) {
  //      if (fmin>cdata1d[n][i]) fmin=cdata1d[n][i];
  //      if (fmax<cdata1d[n][i]) fmax=cdata1d[n][i];
  //    }
  //    METLIBS_LOG_DEBUG("par1d,min,max: "<<idPar1d[n]<<"  "<<fmin<<"  "<<fmax);
  //  }
  //  for (n=0; n<numPar2d; n++) {
  //    for (k=0; k<numLev; k++) {
  //      float fmax=-1.e+38, fmin=+1.e+38;
  //      for (i=0; i<nPoint; i++) {
  //        if (fmin>cdata2d[n][k*nPoint+i]) fmin=cdata2d[n][k*nPoint+i];
  //        if (fmax<cdata2d[n][k*nPoint+i]) fmax=cdata2d[n][k*nPoint+i];
  //      }
  //      METLIBS_LOG_DEBUG("par2d,k,min,max: "<<idPar2d[n]<<" "<<k<<"  "<<fmin<<"  "<<fmax);
  //    }
  //  }
  //  METLIBS_LOG_DEBUG("vrangemin,vrangemax: "<<vrangemin<<" "<<vrangemax);
  //##################################################################################

  // locate som parameters in the data arrays
  //
  // we use the length in grid coordinates as x output
  //       ('one level' parameter -1003 or increments as param -1007)

  for (n = 0; n < numPar1d; n++) {
    // ps (vcoord=2,10,4,12)
    if (idPar1d[n] == 8)
      nps = n;
    // topography
    if (idPar1d[n] == 101)
      ntopo = n;
    // sea surface elevation (vcoord=5)
    if (idPar1d[n] == 301)
      npss = n;
    // sea bottom (vcoord=5)
    if (idPar1d[n] == 351)
      npsb = n;

    // the following are parameters computed in program vcdata

    // x_grid
    if (idPar1d[n] == -1001)
      nxg = n;
    // y_grid
    if (idPar1d[n] == -1002)
      nyg = n;
    // s_grid (in unit m) ... low scaling (ds_grid better...)
    if (idPar1d[n] == -1003)
      nxs = n;
    // coriolis parameter
    if (idPar1d[n] == -1004)
      ncor = n;
    // latitude
    if (idPar1d[n] == -1005)
      nlat = n;
    // longitude
    if (idPar1d[n] == -1006)
      nlon = n;
    // ds_grid (in unit m) .. length increments
    if (idPar1d[n] == -1007)
      nxds = n;
    // rotation of vector components to E/W and N/S
    if (idPar1d[n] == -1008)
      ngdir1 = n;
    if (idPar1d[n] == -1009)
      ngdir2 = n;
  }

  npp = -1;
  nzz = -1;

  for (n = 0; n < numPar2d; n++) {
    // isentropic surfaces, sigma height and sigma.MM5 levels: pressure
    if (idPar2d[n] == 8)
      npp = n;
    if (idPar2d[n] == 1)
      nzz = n;
  }

  // check if necessary parameters exist
  if ((vcoord == 2 || vcoord == 10) && nps < 0) {
    METLIBS_LOG_ERROR("ps (single level parameter 8) g");
    error = true;
  }
  //if (vcoord==5 && npss<0) {
  //  METLIBS_LOG_DEBUG("sea elevation (single level parameter 301) missing");
  //  error=true;
  //}
  //if (vcoord==5 && npsb<0) {
  //  METLIBS_LOG_DEBUG("sea bottom (single level parameter 351) missing");
  //  error=true;
  //}
  if (vcoord == 11 && ntopo < 0) {
    METLIBS_LOG_ERROR("zs (single level parameter 101) missing");
    error = true;
  }
  if (nxg < 0) {
    METLIBS_LOG_ERROR("x coordinate in grid (param. -1001) missing" );
    METLIBS_LOG_ERROR("             (error in preprocessing program)" );
    error = true;
  }
  if (nyg < 0) {
    METLIBS_LOG_ERROR("y coordinate in grid (param. -1002) missing" );
    METLIBS_LOG_ERROR("             (error in preprocessing program)" );
    error = true;
  }
  if (nxs < 0 && nxds < 0) {
    METLIBS_LOG_ERROR("length in meter (param. -1003 or -1007) missing" );
    METLIBS_LOG_ERROR("               (error in preprocessing program)" );
    error = true;
  }
  if (ncor < 0) {
    METLIBS_LOG_ERROR("coriolis parameter (param. -1004) missing" );
    METLIBS_LOG_ERROR("               (error in preprocessing program)" );
    error = true;
  }
  if (nlat < 0) {
    METLIBS_LOG_ERROR("latitude (param. -1005) missing" );
    METLIBS_LOG_ERROR("               (error in preprocessing program)" );
    error = true;
  }
  if (nlon < 0) {
    METLIBS_LOG_ERROR("longitude (param. -1006) missing" );
    METLIBS_LOG_ERROR("               (error in preprocessing program)" );
    error = true;
  }
  if (vcoord == 4 && npp < 0) {
    METLIBS_LOG_ERROR("pressure in isentropic surfaces (param. 8) missing" );
    error = true;
  }
  if (vcoord == 12 && npp < 0) {
    METLIBS_LOG_ERROR("pressure in sigma.MM5 surfaces (param. 8) missing" );
    error = true;
  }

  if (error) {
    METLIBS_LOG_ERROR("VcrossPlot::prepareData : Not able to plot this crossection");
    return false;
  }

  if (vcoord == 12 && nps < 0) {
    nps = addPar1d(8);
    for (i = 0; i < nPoint; i++)
      cdata1d[nps][i] = 1.5 * cdata2d[npp][i] - 0.5 * cdata2d[npp][nPoint + i];
  }

  if (nxds >= 0) {
    if (nxs < 0)
      nxs = addPar1d(-1003);
    cdata1d[nxs][0] = 0.0;
    for (i = 1; i < nPoint; i++)
      cdata1d[nxs][i] = cdata1d[nxs][i - 1] + cdata1d[nxds][i];
  }

  // constants to compute tangential and normal wind comp. etc.
  // (crossection may consist of several straight lines)

  nsin = addPar1d(-30001);
  ncos = addPar1d(-30002);

  METLIBS_LOG_DEBUG(LOGVAL(timeGraph));
  if (!timeGraph) {
    LOG_2D(cdata1d[nxg], nPoint, nPoint, "nxg");
    LOG_2D(cdata1d[nyg], nPoint, nPoint, "nyg");
    float dx = cdata1d[nxg][1] - cdata1d[nxg][0];
    float dy = cdata1d[nyg][1] - cdata1d[nyg][0];
    float ds = sqrtf(dx * dx + dy * dy);
    cdata1d[nsin][0] = dy / ds;
    cdata1d[ncos][0] = dx / ds;
    for (int i = 1; i < nPoint - 1; i++) {
      dx = cdata1d[nxg][i + 1] - cdata1d[nxg][i - 1];
      dy = cdata1d[nyg][i + 1] - cdata1d[nyg][i - 1];
      ds = sqrtf(dx * dx + dy * dy);
      cdata1d[nsin][i] = dy / ds;
      cdata1d[ncos][i] = dx / ds;
    }
    dx = cdata1d[nxg][nPoint - 1] - cdata1d[nxg][nPoint - 2];
    dy = cdata1d[nyg][nPoint - 1] - cdata1d[nyg][nPoint - 2];
    ds = sqrtf(dx * dx + dy * dy);
    cdata1d[nsin][nPoint - 1] = dy / ds;
    cdata1d[ncos][nPoint - 1] = dx / ds;
  } else {
    // time graph (one position) ..... directions rel. crossection
    float ds = sqrtf(tgdx * tgdx + tgdy * tgdy);
    float dy = tgdy / ds;
    float dx = tgdx / ds;
    for (int i = 0; i < nPoint; i++) {
      cdata1d[nsin][i] = dy;
      cdata1d[ncos][i] = dx;
    }
    tgxpos = cdata1d[nxg][0];
    tgypos = cdata1d[nyg][0];
    // time graph: good dummy values needed for plotting
    ds = 2. * horizontalLength / float(horizontalPosNum + nPoint - 2);
    for (int i = 0; i < nPoint; i++) {
      cdata1d[nxg][i] = float(i);
      cdata1d[nyg][i] = 0.0;
      cdata1d[nxs][i] = float(i) * ds; // hmmm...... good ????
    }
  }

  // space for 1d parameters ...... later when used ??????????
  if (vcoord == 2 || vcoord == 10) {
    npy1 = addPar1d(-30003);
  } else if (vcoord == 4) {
    npy1 = addPar1d(-30003);
  } else if (vcoord == 5) {
    if (npsb >= 0)
      npy1 = addPar1d(-30003);
    if (npss >= 0)
      npy2 = addPar1d(-30005);
  } else if (vcoord == 11 || vcoord == 12) {
    npy1 = addPar1d(-30003);
  }

  npi = -1;

  // space for 2d parameters ...... later when used ??????????
  if (vcoord == 2 || vcoord == 10 || vcoord == 1) {
    npp = addPar2d(8);
    npi = addPar2d(-31006);
  } else if (vcoord == 4 || vcoord == 11 || vcoord == 12) {
    if (npp >= 0)
      npi = addPar2d(-31006);
  }

  nx = addPar2d(-31001);
  ny = addPar2d(-31002);

  METLIBS_LOG_DEBUG(LOGVAL(vcoord));
  if (vcoord == 2 || vcoord == 10) {
    // pressure in all levels
    // p = alevel + blevel*ps
    // (for sigma this is the same as p = ptop + sigma*(ps-ptop))
    int n = 0;
    for (int k = 0; k < numLev; k++)
      for (int i = 0; i < nPoint; i++)
        cdata2d[npp][n++] = alevel[k] + blevel[k] * cdata1d[nps][i];
    float* ff = new float[numLev];
    std::copy(alevel.begin(), alevel.end(), ff);
    LOG_2D(ff, numLev, nPoint, "prepData alevel");
    std::copy(blevel.begin(), blevel.end(), ff);
    LOG_2D(ff, numLev, nPoint, "prepData blevel");
    delete[] ff;
    LOG_2D(cdata2d[nps], nTotal, nPoint, "prepData nps");
  } else if (vcoord == 1) {
    // pressure levels
    int n = 0;
    for (int k = 0; k < numLev; k++)
      for (int i = 0; i < nPoint; i++)
        cdata2d[npp][n++] = alevel[k];
  }

  METLIBS_LOG_DEBUG(LOGVAL(npp));
  LOG_2D(cdata2d[npp], nTotal, nPoint, "prepData npp");
  if (npp >= 0) {
    // exner function (pi) in all levels
    // pi = 1004.*(p/1000.)**(287./1004.)
    for (int n = 0; n < nTotal; n++) {
      float p = cdata2d[npp][n], pi;
      if (iundef == 0 or p != fieldUndef) {
        pi = cp * powf(p * p0inv, kappa);
      } else {
        pi = fieldUndef;
      }
      cdata2d[npi][n] = pi;
    }
  }

  // used by misc. routines (incl. contouring)
  n = 0;
  for (int k = 0; k < numLev; k++)
    for (int i = 0; i < nPoint; i++)
      cdata2d[nx][n++] = cdata1d[nxs][i];

  // location of 2d parameters (fields) by name,
  // as later used in computations and plotting

  params.clear();

  map<int, std::string>::iterator pn, pnend = vcParNumber.end();

  for (int n = 0; n < numPar2d; n++) {
    pn = vcParNumber.find(idPar2d[n]);
    if (pn != pnend)
      params[pn->second] = n;
    else
      METLIBS_LOG_DEBUG("not found: " << LOGVAL(idPar2d[n]));
  }

  // the computation functions
  map<std::string, FileContents>::iterator pfc = fileContents.find(fileName);
  if (pfc != fileContents.end())
    useFunctions = pfc->second.useFunctions;

  // initial impossible settings
  vcoordPlot = vcv_none;
  verticalAxis = "none";
  v2hRatio = -1.;

  //############################################################################
  //  for (n=0; n<idPar1d.size(); n++)
  //    METLIBS_LOG_DEBUG("VcrossPlot::prepareData n,idPar1d[n]: "<<n<<" "<<idPar1d[n]);
  //  for (n=0; n<idPar2d.size(); n++)
  //    METLIBS_LOG_DEBUG("VcrossPlot::prepareData n,idPar2d[n]: "<<n<<" "<<idPar2d[n]);
  //  METLIBS_LOG_DEBUG("VcrossPlot::prepareData ntopo,nps,nzz: "<<ntopo<<" "<<nps<<" "<<nzz);
  //############################################################################
  return true;
}

void VcrossPlot::prepareVertical()
{
  METLIBS_LOG_SCOPE();

  // preparations due to change of output vertical coordinate,
  // always called before first plot

  // Vertical coordinates handled:
  // vcoord:  2 = sigma (0.-1. with ps and ptop) ... Norlam
  //         10 = eta (hybrid) ... Hirlam, Ecmwf,...
  //          1 = pressure
  //          4 = isentropic surfaces (potential temp., with p(th))
  //          5 = z levels from sea model (possibly incl. sea elevation and bottom)
  //         11 = sigma height levels (MEMO,MC2)
  //         12 = sigma.MM5 (input P in all levels)

  int i, j, k, n;
  float p, pi, y;

  // compute window needed for all crossection data

  xDatamin = cdata1d[nxs][0];
  xDatamax = cdata1d[nxs][nPoint - 1];
  METLIBS_LOG_DEBUG(LOGVAL(nxs) << LOGVAL(nPoint) << LOGVAL(xDatamin) << LOGVAL(xDatamax));

  yconst = 0.;
  yscale = 0.;

  if (vcoord != 5 && vcoord != 11) {

    // pressure range of current crossection (all timesteps)
    pmin = vrangemin;
    pmax = vrangemax + 5.;
    // assume 10 m/hPa, then apply the vertical to horizontal ratio
    yDatamin = 0.;
    yDatamax = v2hRatio * 10. * (pmax - pmin);
    METLIBS_LOG_DEBUG(LOGVAL(pmin) << LOGVAL(pmax) << LOGVAL(yDatamin) << LOGVAL(yDatamax) << LOGVAL(v2hRatio));
    // compute output y in all data points (copy x for contouring routine)
    // exner function,   pi = 1004.*(p/1000.)**(287./1004.)
    pimax = cp * powf(pmax * p0inv, kappa);
    pimin = cp * powf(pmin * p0inv, kappa);

    int npc = -1;

    if (vcoordPlot == vcv_exner) {
      // exner function as vertical plot coordinate
      // y(pimax)=yDatamin   y(pimin)=yDatamax   y(pi)=yconst+yscale*pi
      METLIBS_LOG_DEBUG("exner" << LOGVAL(pimin) << LOGVAL(pimax));
      yscale = (yDatamax - yDatamin) / (pimin - pimax);
      yconst = yDatamin - yscale * pimax;
      npc = npi;
      LOG_2D(cdata2d[npi], nTotal, nPoint, "cdata2d[npi]");
    } else if (vcoordPlot == vcv_pressure) {
      // pressure as vertical plot coordinate
      METLIBS_LOG_DEBUG("pressure" << LOGVAL(pmin) << LOGVAL(pmax));
      yscale = (yDatamax - yDatamin) / (pmin - pmax);
      yconst = yDatamin - yscale * pmax;
      npc = npp;
    } else if (vcoordPlot == vcv_height) {
      // no "zrange" of crossections available, yet...
      float zmin, zmax;
      zmin = zmax = cdata2d[nzz][0];
      for (int n = 1; n < nTotal; n++) {
        if (zmin > cdata2d[nzz][n])
          zmin = cdata2d[nzz][n];
        if (zmax < cdata2d[nzz][n])
          zmax = cdata2d[nzz][n];
      }
      zmin -= 40.;
      zmax += 40.;
      yDatamin = v2hRatio * zmin;
      yDatamax = v2hRatio * zmax;
      yscale = v2hRatio;
      yconst = 0.;
      npc = nzz;
    }
    METLIBS_LOG_DEBUG(LOGVAL(yconst) << LOGVAL(yscale) << LOGVAL(yDatamin) << LOGVAL(yDatamax) << LOGVAL(iundef) << LOGVAL(npc));
    for (int i = 0; i < nTotal; i++) {
      if (iundef == 0 or cdata2d[npc][i] != fieldUndef)
        cdata2d[ny][i] = yconst + yscale * cdata2d[npc][i];
    }
    LOG_2D(cdata2d[ny], nTotal, nPoint, "cdata2d[ny]");

    if (npy1 >= 0) {
      if (nps >= 0 && vcoordPlot == vcv_exner) {
        for (int i = 0; i < nPoint; i++) {
          p = cdata1d[nps][i];
          pi = cp * powf(p * p0inv, kappa);
          cdata1d[npy1][i] = yconst + yscale * pi;
        }
      } else if (nps >= 0 && vcoordPlot == vcv_pressure) {
        for (int i = 0; i < nPoint; i++) {
          p = cdata1d[nps][i];
          cdata1d[npy1][i] = yconst + yscale * p;
        }
      } else if (ntopo >= 0 && vcoordPlot == vcv_height) {
        for (int i = 0; i < nPoint; i++) {
          const float z = cdata1d[ntopo][i];
          cdata1d[npy1][i] = yconst + yscale * z;
        }
      } else {
        // isentropic levels
        // approx. surface according to existing data
        // (but can't find anything below the lower level)
        for (int i = 0; i < nPoint; i++) {
          k = 0;
          while (k < numLev && cdata2d[ny][k * nPoint + i] == fieldUndef)
            k++;
          if (k < numLev) {
            float dy = 0.;
            if (k > 0 && k < numLev - 1 && cdata2d[ny][(k + 1) * nPoint + i]
                != fieldUndef)
              dy = cdata2d[ny][(k + 1) * nPoint + i]
                  - cdata2d[ny][k * nPoint + i];
            cdata1d[npy1][i] = cdata2d[ny][k * nPoint + i] - dy * 0.5;
          } else {
            cdata1d[npy1][i] = fieldUndef;
          }
        }
        // we don't want any undefined values in this array
        bool found = false;
        for (int i = 0; i < nPoint; i++) {
          if (cdata1d[npy1][i] != fieldUndef) {
            found = true;
            if (cdata1d[npy1][i] < (yDatamin + yDatamax) * 0.5)
              y = yDatamin;
            else
              y = yDatamax;
            j = i - 1;
            while (j >= 0 && cdata1d[npy1][j] == fieldUndef)
              cdata1d[npy1][j--] = y;
            j = i + 1;
          while (j < nPoint && cdata1d[npy1][j] == fieldUndef)
            cdata1d[npy1][j++] = y;
          }
        }
        if (!found) {
          y = yDatamax + yDatamax - yDatamin;
          for (int i = 0; i < nPoint; i++)
            cdata1d[npy1][i] = y;
        }
      }
    }
  } else if (vcoord == 5) {

    // z(sea): fixed heights in the alevel array
    yDatamin = v2hRatio * (vrangemin - 20.);
    yDatamax = v2hRatio * (vrangemax + 20.);
    n = 0;
    for (k = 0; k < numLev; k++)
      for (i = 0; i < nPoint; i++)
        cdata2d[ny][n++] = v2hRatio * alevel[k];

    if (npsb >= 0) { // sea bottom depth
      for (i = 0; i < nPoint; i++) {
        if (cdata1d[npsb][i] != fieldUndef)
          cdata1d[npy1][i] = v2hRatio * cdata1d[npsb][i];
        else
          cdata1d[npy1][i] = fieldUndef;
      }
    }

    if (npss >= 0) { // sea surface elevation
      for (i = 0; i < nPoint; i++) {
        if (cdata1d[npss][i] != fieldUndef)
          cdata1d[npy2][i] = v2hRatio * cdata1d[npss][i];
        else
          cdata1d[npy2][i] = 0.0; // (fieldUndef ???)
      }
    }

  } else if (vcoord == 11) {

    // sigma height
    yDatamin = v2hRatio * (vrangemin - 50.);
    yDatamax = v2hRatio * (vrangemax + 0.);
    float zsurf, zheight, ztop = alevel[0];
    n = 0;
    for (k = 0; k < numLev; k++) {
      for (i = 0; i < nPoint; i++) {
        zsurf = cdata1d[ntopo][i];
        zheight = zsurf + blevel[k] * (ztop - zsurf);
        cdata2d[ny][n++] = v2hRatio * zheight;
      }
    }
    for (i = 0; i < nPoint; i++)
      cdata1d[npy1][i] = v2hRatio * cdata1d[ntopo][i];

  }

  //####################################################################
  //METLIBS_LOG_DEBUG("xDatamin,xDatamax: "<<xDatamin<<" "<<xDatamax);
  //METLIBS_LOG_DEBUG("yDatamin,yDatamax: "<<yDatamin<<" "<<yDatamax);
  //METLIBS_LOG_DEBUG("pmin,    pmax:     "<<pmin    <<" "<<pmax);
  //METLIBS_LOG_DEBUG("pimin,   pimax:    "<<pimin   <<" "<<pimax);
  //####################################################################

  // y min,max in each level
  n = 0;
  vlimitmin.clear();
  vlimitmax.clear();
  for (int k = 0; k < numLev; k++) {
    float vmin = +fieldUndef;
    float vmax = -fieldUndef;
    for (int i = 0; i < nPoint; i++) {
      float v = cdata2d[ny][n++];
      if (v != fieldUndef) {
        if (vmin > v)
          vmin = v;
        if (vmax < v)
          vmax = v;
      }
    }
    vlimitmin.push_back(vmin);
    vlimitmax.push_back(vmax);
  }

  if (timeGraph) {
    if (nplot == 0) {
      if (xDatamax - xDatamin < yDatamax - yDatamin) {
        float ds = (yDatamax - yDatamin) / float(nPoint - 1);
        for (i = 0; i < nPoint; i++)
          cdata1d[nxs][i] = float(i) * ds;
        xDatamin = cdata1d[nxs][0];
        xDatamax = cdata1d[nxs][nPoint - 1];
        n = 0;
        for (k = 0; k < numLev; k++)
          for (i = 0; i < nPoint; i++)
            cdata2d[nx][n++] = cdata1d[nxs][i];
      }
      timeGraphReference = validTimeSeries[0];
      int minutes = miTime::minDiff(validTimeSeries[nPoint - 1],
          timeGraphReference);

      if (minutes < 0)
        METLIBS_LOG_ERROR("!!!!!!!!! minutes= " << minutes << " !!!!!!!" );
      if (minutes < 0)
        minutes = -minutes;

      timeGraphMinuteStep = (xDatamax - xDatamin) / float(minutes);
    } else {
      for (i = 0; i < nPoint; i++) {
        int minutes = miTime::minDiff(validTimeSeries[i], timeGraphReference);
        cdata1d[nxs][i] = float(minutes) * timeGraphMinuteStep;
      }
      xDatamin = cdata1d[nxs][0];
      xDatamax = cdata1d[nxs][nPoint - 1];
      n = 0;
      for (k = 0; k < numLev; k++)
        for (i = 0; i < nPoint; i++)
          cdata2d[nx][n++] = cdata1d[nxs][i];
    }
  }
}


int VcrossPlot::findParam(const std::string& var)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(var));

  const std::map<std::string, int>::const_iterator p = params.find(var);
  if (p != params.end())
    return p->second;

  const std::map<std::string, vcFunction>::const_iterator f = useFunctions.find(var);
  if (f == useFunctions.end())
    return -1;

  std::vector<int> parloc;
  BOOST_FOREACH(const std::string& v, f->second.vars) {
    const int np = findParam(v);
    if (np < 0)
      return -1;
    parloc.push_back(np);
  }

  return computer(var, f->second.function, parloc);
}

int VcrossPlot::computer(const std::string& var, VcrossFunction vcfunc, const vector<int>& parloc)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(var));

  int k, i, n, n1, n2, nu, nv, nrot1, nrot2, ntk, nth, nvn, compddz;
  float u, v, unitscale, s1;
  float *cwork1, *cwork2;

  std::string unit;

  int no = addPar2d(999);

  int compute = 0;

  bool allDefined = (iundef <= 0);

  switch (vcfunc) {

  case vcf_add:
    if (compute == 0)
      compute = 1;
  case vcf_subtract:
    if (compute == 0)
      compute = 2;
  case vcf_multiply:
    if (compute == 0)
      compute = 3;
  case vcf_divide:
    if (compute == 0)
      compute = 4;
    n1 = parloc[0];
    n2 = parloc[1];
    if (!ffunc.fieldOPERfield(compute, nPoint, numLev, cdata2d[n1],
        cdata2d[n2], cdata2d[no], allDefined, fieldUndef))
      return -1;
    break;

  case vcf_tc_from_tk:
    if (compute == 0)
      compute = 1;
  case vcf_tk_from_tc:
    if (compute == 0)
      compute = 2;
    n1 = parloc[0];
    if (!ffunc.cvtemp(compute, nPoint, numLev, cdata2d[n1], cdata2d[no],
        allDefined, fieldUndef))
      return -1;
    break;

  case vcf_tc_from_th:
    if (compute == 0){
      compute = 1;
      unit = "celsius";
    }
  case vcf_tk_from_th:
    if (compute == 0) {
      compute = 2;
      unit = "kelvin";
    }
  case vcf_th_from_tk:
    if (compute == 0)
      compute = 3;
  case vcf_thesat_from_tk:
    if (compute == 0)
      compute = 4;
  case vcf_thesat_from_th:
    if (compute == 0)
      compute = 5;
    n1 = parloc[0];
    if (npp < 0)
      return -1;
    if (!ffunc.aleveltemp(compute, nPoint, numLev, cdata2d[n1], cdata2d[npp],
        cdata2d[no], allDefined, fieldUndef, unit))
      return -1;
    break;

  case vcf_the_from_tk_q:
    if (compute == 0)
      compute = 1;
  case vcf_the_from_th_q:
    if (compute == 0)
      compute = 2;
    n1 = parloc[0];
    n2 = parloc[1];
    if (npp < 0)
      return -1;
    if (!ffunc.alevelthe(compute, nPoint, numLev, cdata2d[n1], cdata2d[n2],
        cdata2d[npp], cdata2d[no], allDefined, fieldUndef))
      return -1;
    break;

  case vcf_rh_from_tk_q:
    if (compute == 0)
      compute = 1;
  case vcf_rh_from_th_q:
    if (compute == 0)
      compute = 2;
  case vcf_q_from_tk_rh:
    if (compute == 0)
      compute = 3;
  case vcf_q_from_th_rh:
    if (compute == 0)
      compute = 4;
  case vcf_tdc_from_tk_q:
    if (compute == 0) {
      compute = 5;
      unit = "celsius";
    }
  case vcf_tdc_from_th_q:
    if (compute == 0) {
      compute = 6;
      unit = "celsius";
    }
  case vcf_tdc_from_tk_rh:
    if (compute == 0) {
      compute = 7;
      unit = "celsius";
    }
  case vcf_tdc_from_th_rh:
    if (compute == 0) {
      compute = 8;
      unit = "celsius";
    }
  case vcf_tdk_from_tk_q:
    if (compute == 0) {
      compute = 9;
      unit = "kelvin";
    }
  case vcf_tdk_from_th_q:
    if (compute == 0) {
      compute = 10;
      unit = "kelvin";
    }
  case vcf_tdk_from_tk_rh:
    if (compute == 0){
      compute = 11;
      unit = "kelvin";
    }
  case vcf_tdk_from_th_rh:
    if (compute == 0) {
      compute = 12;
      unit = "kelvin";
    }
    n1 = parloc[0];
    n2 = parloc[1];
    if (npp < 0)
      return -1;
    if (!ffunc.alevelhum(compute, nPoint, numLev, cdata2d[n1], cdata2d[n2],
        cdata2d[npp], cdata2d[no], allDefined, fieldUndef, unit))
      return -1;
    break;

  case vcf_ducting_from_tk_q:
    if (compute == 0)
      compute = 1;
  case vcf_ducting_from_th_q:
    if (compute == 0)
      compute = 2;
  case vcf_ducting_from_tk_rh:
    if (compute == 0)
      compute = 3;
  case vcf_ducting_from_th_rh:
    if (compute == 0)
      compute = 4;
  case vcf_d_ducting_dz_from_tk_q:
    if (compute == 0)
      compute = 5;
  case vcf_d_ducting_dz_from_th_q:
    if (compute == 0)
      compute = 6;
  case vcf_d_ducting_dz_from_tk_rh:
    if (compute == 0)
      compute = 7;
  case vcf_d_ducting_dz_from_th_rh:
    if (compute == 0)
      compute = 8;
    n1 = parloc[0];
    n2 = parloc[1];
    if (npp < 0)
      return -1;
    compddz = compute;
    if (compute > 4) {
      if (npi < 0)
        return -1;
      compute -= 4;
    }
    if (!ffunc.alevelducting(compute, nPoint, numLev, cdata2d[n1], cdata2d[n2],
        cdata2d[npp], cdata2d[no], allDefined, fieldUndef))
      return -1;
    if (compddz > 4) {
      //...................................................................
      //       t in unit Kelvin, p in unit hPa ???
      //       duct=77.6*(p/t)+373000.*(q*p)/(eps*t*t)
      //       q*p/eps = rh*qsat*p/eps = rh*(eps*e(t)/p)*p/eps
      //               = rh*e(t) = (e(td)/e(t))*e(t) = e(td)
      //       => duct = 77.6*(p/t)+373000.*e(td)/(t*t)
      //
      //       As i Vertical Profiles:
      //       D(ducting)/Dz =
      //       duct(k) = (duct(k+1)-duct(k))/(dz*0.001) !!!
      //...................................................................
      if (compddz == 5 || compddz == 7) {
        ntk = n1;
        float pi1, pi2, th1, th2, dz, fv1, fv2;
        for (k = 0; k < numLev - 1; k++) {
          for (i = 0, n = nPoint * k; i < nPoint; i++, n++) {
            if (cdata2d[ntk][n] != fieldUndef && cdata2d[ntk][n + nPoint]
                                                              != fieldUndef && cdata2d[no][n] != fieldUndef && cdata2d[no][n
                                                                                                                           + nPoint] != fieldUndef) {
              th1 = cp * cdata2d[ntk][n] / cdata2d[npi][n];
              th2 = cp * cdata2d[ntk][n + nPoint] / cdata2d[npi][n + nPoint];
              pi1 = cdata2d[npi][n];
              pi2 = cdata2d[npi][n + nPoint];
              dz = (th1 + th2) * 0.5 * (pi1 - pi2) * ginv;
              fv1 = cdata2d[no][n];
              fv2 = cdata2d[no][n + nPoint];
              cdata2d[no][n] = (fv2 - fv1) / (dz * 0.001);
            } else {
              cdata2d[no][n] = fieldUndef;
            }
          }
        }
      } else if (compddz == 6 || compddz == 8) {
        nth = n1;
        float pi1, pi2, th1, th2, dz, fv1, fv2;
        for (k = 0; k < numLev - 1; k++) {
          for (i = 0, n = nPoint * k; i < nPoint; i++, n++) {
            if (cdata2d[nth][n] != fieldUndef && cdata2d[nth][n + nPoint]
                                                              != fieldUndef && cdata2d[no][n] != fieldUndef && cdata2d[no][n
                                                                                                                           + nPoint] != fieldUndef) {
              th1 = cdata2d[nth][n];
              th2 = cdata2d[nth][n + nPoint];
              pi1 = cdata2d[npi][n];
              pi2 = cdata2d[npi][n + nPoint];
              dz = (th1 + th2) * 0.5 * (pi1 - pi2) * ginv;
              fv1 = cdata2d[no][n];
              fv2 = cdata2d[no][n + nPoint];
              cdata2d[no][n] = (fv2 - fv1) / (dz * 0.001);
            } else {
              cdata2d[no][n] = fieldUndef;
            }
          }
        }
      }
    }
    // upper level equal the level below
    n1 = (numLev - 1) * nPoint;
    n2 = (numLev - 2) * nPoint;
    for (i = 0; i < nPoint; i++)
      cdata2d[no][n1 + i] = cdata2d[no][n2 + i];
    break;

  case vcf_ff_total:
    nu = parloc[0];
    nv = parloc[1];
    if (allDefined) {
      for (n = 0; n < nTotal; n++) {
        u = cdata2d[nu][n];
        v = cdata2d[nv][n];
        cdata2d[no][n] = sqrtf(u * u + v * v);
      }
    } else {
      for (n = 0; n < nTotal; n++) {
        if (cdata2d[nu][n] != fieldUndef && cdata2d[nv][n] != fieldUndef) {
          u = cdata2d[nu][n];
          v = cdata2d[nv][n];
          cdata2d[no][n] = sqrtf(u * u + v * v);
        } else {
          cdata2d[no][n] = fieldUndef;
        }
      }
    }
    break;

  case vcf_ff_normal:
    if (compute == 0)
      compute = 1;
  case vcf_ff_tangential:
    if (compute == 0)
      compute = 2;
  case vcf_ff_north_south:
    if (compute == 0)
      compute = 3;
  case vcf_ff_east_west:
    if (compute == 0)
      compute = 4;
    nu = parloc[0];
    nv = parloc[1];
    if (compute == 1) {
      nrot1 = nsin;
      nrot2 = ncos;
      s1 = -1.;
    } else if (compute == 2) {
      nrot1 = ncos;
      nrot2 = nsin;
      s1 = 1.;
    } else if (compute == 3) {
      nrot1 = ngdir2;
      nrot2 = ngdir1;
      s1 = -1.;
    } else {
      nrot1 = ngdir1;
      nrot2 = ngdir2;
      s1 = 1.;
    }
  {
    METLIBS_LOG_DEBUG(LOGVAL(compute) << LOGVAL(nu) << LOGVAL(nv) << LOGVAL(nrot1) << LOGVAL(nrot2));
    LOG_2D(cdata2d[nu], nTotal, nPoint, "u");
    LOG_2D(cdata2d[nv], nTotal, nPoint, "v");
    if (nrot1 >= 0)
      LOG_2D(cdata1d[nrot1], nPoint, nPoint, "nrot1");
    if (nrot2 >= 0)
      LOG_2D(cdata1d[nrot2], nPoint, nPoint, "nrot2");
  }
    if (nrot1 < 0 || nrot2 < 0)
      return -1;
    if (allDefined) {
      for (k = 0; k < numLev; k++) {
        for (i = 0, n = nPoint * k; i < nPoint; i++, n++) {
          cdata2d[no][n] = cdata1d[nrot1][i] * cdata2d[nu][n] * s1
              + cdata1d[nrot2][i] * cdata2d[nv][n];
        }
      }
    } else {
      for (k = 0; k < numLev; k++) {
        for (i = 0, n = nPoint * k; i < nPoint; i++, n++) {
          if (cdata2d[nu][n] != fieldUndef && cdata2d[nv][n] != fieldUndef)
            cdata2d[no][n] = cdata1d[nrot1][i] * cdata2d[nu][n] * s1
            + cdata1d[nrot2][i] * cdata2d[nv][n];
          else
            cdata2d[no][n] = fieldUndef;
        }
      }
    }
    break;

  case vcf_knots_from_ms:
    n1 = parloc[0];
    unitscale = 3600. / 1852.;
    compute = 3;
    if (!ffunc.fieldOPERconstant(compute, nPoint, numLev, cdata2d[n1],
        unitscale, cdata2d[no], allDefined, fieldUndef))
      return -1;
    break;

  case vcf_momentum_vn_fs: // m = FFnormal + f*s,  f=coriolis, s=length(incl. map.ratio)
    nvn = parloc[0];
    if (ncor < 0 || nxs < 0)
      return -1;
    if (allDefined) {
      for (k = 0; k < numLev; k++) {
        for (i = 0, n = nPoint * k; i < nPoint; i++, n++) {
          cdata2d[no][n] = cdata2d[nvn][n] + cdata1d[ncor][i] * cdata1d[nxs][i];
        }
      }
    } else {
      for (k = 0; k < numLev; k++) {
        for (i = 0, n = nPoint * k; i < nPoint; i++, n++) {
          if (cdata2d[nvn][n] != fieldUndef)
            cdata2d[no][n] = cdata2d[nvn][n] + cdata1d[ncor][i]
                                                             * cdata1d[nxs][i];
          else
            cdata2d[no][n] = fieldUndef;
        }
      }
    }
    break;

  case vcf_height_above_msl_from_th:
    if (compute == 0)
      compute = 1; // height above msl
  case vcf_height_above_surface_from_th:
    if (compute == 0)
      compute = 2; // height above surface
    nth = parloc[0];
    if (vcoord != 2 && vcoord != 10)
      return -1;
    if (nps < 0)
      return -1;
    if (compute == 1 && ntopo < 0)
      return -1;
    cwork1 = new float[nPoint];
    cwork2 = new float[nPoint];
    if (compute == 1) {
      for (i = 0; i < nPoint; i++)
        cwork1[i] = cdata1d[ntopo][i];
    } else {
      for (i = 0; i < nPoint; i++)
        cwork1[i] = 0.;
    }
    // height computed in sigma1/eta_half levels (plot in sigma2/eta_full)
    float alvl1, blvl1, alvl2, blvl2, p2, pi1, pi2, dz, z1, px, pim1, pip1,
    dthdpi, pifull, thhalf;
    int km, kp;
    for (i = 0; i < nPoint; i++)
      cwork2[i] = cp * powf(cdata1d[nps][i] * p0inv, kappa);
    alvl2 = 0.;
    blvl2 = 1.;
    for (k = 0; k < numLev; k++) {
      alvl1 = alvl2;
      blvl1 = blvl2;
      alvl2 = alvl1 - (alvl1 - alevel[k]) * 2.;
      if (alvl2 < 0.)
        alvl2 = 0.;
      blvl2 = blvl1 - (blvl1 - blevel[k]) * 2.;
      if (blvl2 < 0.)
        blvl2 = 0.;
      km = (k > 0) ? k - 1 : 0;
      kp = (k < numLev - 1) ? k + 1 : numLev - 1;
      for (i = 0, n = nPoint * k; i < nPoint; i++, n++) {
        // pressure and exner function (pi) at sigma1 level above
        p2 = alvl2 + blvl2 * cdata1d[nps][i];
        pi2 = cp * powf(p2 * p0inv, kappa);
        dz = cdata2d[nth][n] * (cwork2[i] - pi2) * ginv;
        z1 = cdata2d[no][i];
        pi1 = cwork2[i];
        cwork1[i] += dz;
        cwork2[i] = pi2;

        // linear interpolation of height is not good (=> T=const. in layer),
        // so we make a first guess of a temperature profile to comp. height
        px = alevel[km] + blevel[km] * cdata1d[nps][i];
        pim1 = cp * powf(px * p0inv, kappa);
        px = alevel[kp] + blevel[kp] * cdata1d[nps][i];
        pip1 = cp * powf(px * p0inv, kappa);
        dthdpi
        = (cdata2d[nth][km * nPoint + i] - cdata2d[nth][kp * nPoint + i])
        / (pim1 - pip1);
        // get temperature at half level (bottom of layer)
        px = alevel[k] + blevel[k] * cdata1d[nps][i];
        pifull = cp * powf(px * p0inv, kappa);
        thhalf = cdata2d[nth][n] + dthdpi * (pi1 - pifull);
        // thickness from half level to full level
        dz = (thhalf + cdata2d[nth][n]) * 0.5 * (pi1 - pifull) * ginv;
        // height at full level
        cdata2d[no][n] = z1 + dz;
      }
    }
    delete[] cwork1;
    delete[] cwork2;
    break;

  default:
    METLIBS_LOG_ERROR("unknown function '" << vcfunc << "'");
    return -1;
  }

  params[var] = no;
  {
    // int n=0;
    // for (int lev=0; lev<numLev; ++lev)
    //   for (int p=0; p<nPoint; ++p)
    //     cdata2d[no][n++] = 1e-3*(lev-numLev/2)*(lev-numLev/2)*(p-nPoint/2)*(p-nPoint/2);
    LOG_2D(cdata2d[no], nTotal, nPoint, var.c_str());
  }
  return no;
}

bool VcrossPlot::computeSize()
{
  METLIBS_LOG_SCOPE();

  // Vertical coordinates handled:
  // vcoord:  2 = sigma (0.-1. with ps and ptop) ... Norlam
  //         10 = eta (hybrid) ... Hirlam, Ecmwf,...
  //          1 = pressure
  //          4 = isentropic surfaces (potential temp., with p(th))
  //          5 = z levels from sea model (possibly incl. sea elevation and bottom)
  //         11 = sigma height levels (MEMO,MC2)
  //         12 = sigma.MM5 (input P in all levels)

  float x1, x2, y1, y2, dx, dy;

  float xframe1, xframe2, yframe1, yframe2;

  // add window space for wind, pressure marks and numbers, text

  xframe1 = xframe2 = yframe1 = yframe2 = 0.;

  float chx, chy, chxt, chyt, dchx, dchy;

  float chystp = 1.7;
  chx = 1.;
  chy = 1.;
  dchx = 0.;
  dchy = 0.;

  if (vcopt->pLevelNumbers) {
    // vertical scale marks and numbers
    dchx = chx * 10;
    dchy = chy * 0.6;
  } else if (vcopt->pFrame) {
    // horizontal marks)
    dchy = chy * 0.6;
  }

  if (xframe1 < dchx)
    xframe1 = dchx;
  if (xframe2 < dchx)
    xframe2 = dchx;
  if (yframe1 < dchy)
    yframe1 = dchy;
  if (yframe2 < dchy)
    yframe2 = dchy;

  // below diagram (possibly overplotted by wind/current)
  chxt = chx*1.2;
  chyt = chy*1.2;
  if (!timeGraph) {
    // distance (from a reference position, default left end)
    if (vcopt->pDistance) {
      yframe1 += chyt * chystp * 1.2;
      if (xframe1 < chxt * 4.6)
        xframe1 = chxt * 4.6;
      if (xframe2 < chxt * 1.6)
        xframe2 = chxt * 1.6;
    }
    // x,y coordinates
    if (vcopt->pXYpos) {
      yframe1 += chyt * chystp * 2.2;
      if (xframe1 < chxt * 4.6)
        xframe1 = chxt * 4.6;
      if (xframe2 < chxt * 1.6)
        xframe2 = chxt * 1.6;
    }
    // latitude,longitude
    if (vcopt->pGeoPos) {
      yframe1 += chyt * chystp * 2.2;
      if (xframe1 < chxt * 4.6)
        xframe1 = chxt * 4.6;
      if (xframe2 < chxt * 1.6)
        xframe2 = chxt * 1.6;
    }
    // end names at top (if possible) or position names (not both)
    //if (posmarkName.size()>0) yframe2= yframe2+chy*2.;
  } else if (timeGraph) {
    // time graph: hour/date/forecast (not if text is off)
    if (vcopt->pText) {
      yframe1 += (chyt * chystp * 3.2);
      if (xframe1 < chxt * 2.5)
        xframe1 = chxt * 2.5;
      if (xframe2 < chxt * 2.5)
        xframe2 = chxt * 2.5;
    }
    // time graph: only one position (and only one line needed)
    if (vcopt->pDistance || vcopt->pXYpos || vcopt->pGeoPos) {
      yframe1 += (chy * chystp * 1.2);
    }
  }

  float yframe1s = yframe1;

  // text
  if (vcopt->pText) {
    if (!timeGraph) {
      // parameters and model name + crossection name
      yframe1 += (chy * chystp * numplot);
    } else {
      // time graph: date/time/forecast on x axis,
      //             parameters, model name + crossection name and periode
      yframe1 += (chy * chystp * numplot);
    }
  }

  if (vcopt->pPositionNames && markName.size() > 0) {
    yframe2 += (chy + 1.25 * chy * chystp);
  }

  // check if N/S/E/W symbol needed (direction for horizontal vectors)
  bool nsewsymb = false;
  float dxnsew = 0., dynsew = 0.;

  //##############???????????????????????????????????????????
  //  for (int npo=0; npo<nparout; npo++) {
  //    int np= iparout[npo];
  //    if (iparam[np][0]>=-127 && iparam[np][0]<=-123) nsewsymb= true;
  //  }
  //if (ngdir1<0 || ngdir2<0) nsewsymb= false;
  //##############???????????????????????????????????????????

  if (nsewsymb) {
    dx = (xDatamax + xframe1 - xDatamin + xframe2) * 0.2;
    dy = yframe1 - yframe1s;
    dxnsew = chy + chx * 3.;
    dynsew = chy * 3.;
    if (dxnsew > dx) {
      dynsew = dynsew * (dx / dxnsew);
      dxnsew = dx;
    }
    if (dynsew * 0.6 > dy) {
      dxnsew = dxnsew * 0.6;
      dynsew = dynsew * 0.6;
      yframe1 = yframe1s + dynsew;
    } else if (dynsew > dy) {
      dxnsew = dxnsew * dy / dynsew;
      dynsew = dy;
    }
  }

  xframe1 += 1.0;
  xframe2 += 1.0;
  yframe1 += 1.0;
  yframe2 += 1.0;

  //fontsize= 12. * vcopt->fontSize;  // ???????????????????
  fontsize = 10.;

  fp->setVpSize(float(plotw), float(ploth));
  fp->setGlSize(float(plotw), float(ploth));
  fp->setFontSize(fontsize);
  fp->getCharSize('M', chx, chy);
  chy *= 0.75;
  dx = (xframe1 + xframe2) * chx;
  dy = (yframe1 + yframe2) * chy;
  if (dx > plotw * 0.4 || dy > ploth * 0.4) {
    if (dx > plotw * 0.4)
      dx = plotw * 0.4;
    if (dy > ploth * 0.4)
      dy = ploth * 0.4;
    x1 = (fontsize / chx) * dx / (xframe1 + xframe2);
    x2 = (fontsize / chy) * dy / (yframe1 + yframe2);
    fontsize = (x1 < x2) ? x1 : x2;
    fp->setFontSize(fontsize);
    fp->getCharSize('M', chx, chy);
    chy *= 0.75;
    dx = (xframe1 + xframe2) * chx;
    dy = (yframe1 + yframe2) * chy;
  }

  float pw = float(plotw) - dx;
  float ph = float(ploth) - dy;

  bool wfree = true;
  bool hfree = true;

  if (zoomType != vczoom_keep) {
    if (zoomType == vczoom_standard) {
      xPlotmin = xDatamin;
      xPlotmax = xDatamax;
      yPlotmin = yDatamin;
      yPlotmax = yDatamax;
      if (vcopt->stdVerticalArea) {
        yPlotmin = yDatamin + (yDatamax - yDatamin)
                * float(vcopt->minVerticalArea) * 0.01;
        yPlotmax = yDatamin + (yDatamax - yDatamin)
                * float(vcopt->maxVerticalArea) * 0.01;
        hfree = false;
      }
      if (vcopt->stdHorizontalArea) {
        xPlotmin = xDatamin + (xDatamax - xDatamin)
                * float(vcopt->minHorizontalArea) * 0.01;
        xPlotmax = xDatamin + (xDatamax - xDatamin)
                * float(vcopt->maxHorizontalArea) * 0.01;
        wfree = false;
      }
    } else {
      float sx = (xFullWindowmax - xFullWindowmin) / float(plotw);
      float sy = (yFullWindowmax - yFullWindowmin) / float(ploth);
      if (zoomType == vczoom_decrease) {
        x1 = (zoomSpec[0] < zoomSpec[2]) ? zoomSpec[0] : zoomSpec[2];
        x2 = (zoomSpec[0] < zoomSpec[2]) ? zoomSpec[2] : zoomSpec[0];
        y1 = (zoomSpec[1] < zoomSpec[3]) ? zoomSpec[1] : zoomSpec[3];
        y2 = (zoomSpec[1] < zoomSpec[3]) ? zoomSpec[3] : zoomSpec[1];
        xPlotmin = xFullWindowmin + x1 * sx;
        xPlotmax = xFullWindowmin + x2 * sx;
        yPlotmin = yFullWindowmin + y1 * sy;
        yPlotmax = yFullWindowmin + y2 * sy;
      } else if (zoomType == vczoom_increase) {
        dx = (xPlotmax - xPlotmin) * 0.3 * 0.5;
        dy = (yPlotmax - yPlotmin) * 0.3 * 0.5;
        xPlotmin -= dx;
        xPlotmax += dx;
        yPlotmin -= dy;
        yPlotmax += dy;
      } else if (zoomType == vczoom_move) {
        dx = float(zoomSpec[0]) * sx;
        dy = float(zoomSpec[1]) * sy;
        xPlotmin += dx;
        xPlotmax += dx;
        yPlotmin += dy;
        yPlotmax += dy;
      }
    }
  }

  if (zoomType != vczoom_standard)
    zoomType = vczoom_keep;

  dx = xPlotmax - xPlotmin;
  dy = yPlotmax - yPlotmin;

  if (dx / dy < pw / ph && wfree) {
    dx = (dy * pw / ph - dx) * 0.5;
    xPlotmin -= dx;
    xPlotmax += dx;
  } else if (dx / dy > pw / ph && hfree) {
    dy = (dx * ph / pw - dy) * 0.5;
    yPlotmin -= dy;
    yPlotmax += dy;
  }

  dx = xPlotmax - xPlotmin;
  if (xPlotmin < xDatamin) {
    xPlotmin = xDatamin;
    xPlotmax = xDatamin + dx;
  }
  if (xPlotmax > xDatamax) {
    xPlotmax = xDatamax;
    xPlotmin = (xDatamax - dx > xDatamin) ? xDatamax - dx : xDatamin;
  }
  dy = yPlotmax - yPlotmin;
  if (yPlotmin < yDatamin) {
    yPlotmin = yDatamin;
    yPlotmax = yDatamin + dy;
  }
  if (yPlotmax > yDatamax) {
    yPlotmax = yDatamax;
    yPlotmin = (yDatamax - dy > yDatamin) ? yDatamax - dy : yDatamin;
  }

  float sx = (xPlotmax - xPlotmin) / pw;
  float sy = (yPlotmax - yPlotmin) / ph;
  float s = (sx > sy) ? sx : sy;

  // plot area
  xWindowmin = xPlotmin - xframe1 * chx * s;
  xWindowmax = xPlotmax + xframe2 * chx * s;
  yWindowmin = yPlotmin - yframe1 * chy * s;
  yWindowmax = yPlotmax + yframe2 * chy * s;

  // contrast colour
  backColour = Colour(vcopt->backgroundColour);
  int sum = backColour.R() + backColour.G() + backColour.B();
  if (sum > 255 * 3 / 2)
    contrastColour.set(0, 0, 0);
  else
    contrastColour.set(255, 255, 255);

  return true;
}

int VcrossPlot::getNearestPos(int px)
{
  METLIBS_LOG_SCOPE();
  int n = 0;

  if (nxs >= 0) {
    float sx = (xWindowmax - xWindowmin) / float(plotw);
    float x = xWindowmin + float(px) * sx;
    float d2 = fieldUndef;

    for (int i = 0; i < nPoint; i++) {
      float dx = cdata1d[nxs][i] - x;
      if (d2 > dx * dx) {
        d2 = dx * dx;
        n = i;
      }
    }
  }

  return n;
}

bool VcrossPlot::plotBackground(const vector<std::string>& labels)
{
  METLIBS_LOG_SCOPE();

  // Vertical coordinates handled:
  // vcoord:  2 = sigma (0.-1. with ps and ptop) ... Norlam
  //         10 = eta (hybrid) ... Hirlam, Ecmwf,...
  //          1 = pressure
  //          4 = isentropic surfaces (potential temp., with p(th))
  //          5 = z levels from sea model (possibly incl. sea elevation and bottom)
  //         11 = sigma height levels (MEMO,MC2)
  //         12 = sigma.MM5 (input P in all levels)


  /************************************************************************
   vector<std::string> posmarkName;
   vector<float>    posmarkLatitude;
   vector<float>    posmarkLongitude;
   float posmarkDistance= 100.;
   Area gridArea;
   ************************************************************************/

  // avoid coredump...
  if (xPlotmin >= xPlotmax || yPlotmin >= yPlotmax)
    return false;

  plotFrame();

  fp->setFont("BITMAPFONT");

  plotXLabels();

  // position names at the top
  if (vcopt->pPositionNames && markName.size() > 0 && !timeGraph && nxs >= 0) {
    plotTitle();
  }

  UpdateOutput();


  // upper level, lower level, other levels
  plotLevels();

  // surface pressure (ps)
  // or approx. surface when isentropic levels (vcoord=4)
  // or sea bottom + sea surface elevation (vcoord=5)
  // or surface height (vcoord=11)
  if (vcopt->pSurface) {
    plotSurfacePressure();
  }

  // vertical grid lines
  if (vcopt->pVerticalGridLines) {
    plotVerticalGridLines();
  }

  if (vcopt->pMarkerlines) {
    plotMarkerLines();
  }

  plotVerticalMarkerLines();
  plotAnnotations(labels);

  return true;
}

void VcrossPlot::plotTitle()
{
  float chystp = 1.5;
  float chx = chxdef;
  float chy = chydef;
//  float dchy = chy * 0.6;
  float chxt, chyt;

  int nn = markName.size();
  float y = yWindowmax - 1.25 * chy * chystp;
  fp->setFontSize(fontscale);
  chxt = 1.25 * chx;
  chyt = 1.25 * chy;
  Colour c(vcopt->positionNamesColour);
  if (c == backColour)
    c = contrastColour;
  glColor3ubv(c.RGB());
  float xm, dxh;
  float xlen = 0., xmin = xWindowmax, xmax = xWindowmin;
  vector<int> vpos;
  vector<float> vdx, vxm, vx1, vx2;

  for (int n = 0; n < nn; n++) {
    float x = markNamePosMin[n];
    int i = int(x);
    if (i < 0)
      i = 0;
    if (i > nPoint - 2)
      i = nPoint - 2;
    float x1 = cdata1d[nxs][i] + (cdata1d[nxs][i + 1] - cdata1d[nxs][i]) * (x - float(i));
    x = markNamePosMax[n];
    i = int(x);
    if (i < 0)
      i = 0;
    if (i > nPoint - 2)
      i = nPoint - 2;
    float x2 = cdata1d[nxs][i] + (cdata1d[nxs][i + 1] - cdata1d[nxs][i]) * (x - float(i));
    if (x2 > xDatamin && x1 < xDatamax) {
      float dx,dy;
      fp->getStringSize(markName[n].c_str(), dx, dy);
      float dxh = dx * 0.5 + chxt;
      xm = (x1 + x2) * 0.5;
      if (x1 < xWindowmin + dxh)
        x1 = xWindowmin + dxh;
      if (x2 > xWindowmax - dxh)
        x2 = xWindowmax - dxh;
      if (x1 <= x2) {
        if (xm < x1)
          xm = x1;
        if (xm > x2)
          xm = x2;
        vpos.push_back(n);
        vdx.push_back(dx);
        vxm.push_back(xm);
        vx1.push_back(x1);
        vx2.push_back(x2);
        xlen += dxh * 2.;
        if (xmin > x1)
          xmin = x1;
        if (xmax < x2)
          xmax = x2;
      }
    }
  }

  nn = vpos.size();

  if (nn > 0) {

    if (xlen > xmax - xmin) {
      float s = (xmax - xmin) / xlen;
      if (s < 0.75)
        s = 0.75;
      chxt = chxt * s;
      chyt = chyt * s;
      fp->setFontSize(fontscale * s);
      for (int n = 0; n < nn; n++) {
        float dx, dy;
        fp->getStringSize(markName[n].c_str(), dx, dy);
        vdx[n] = dx;
      }
    }

    for (int n = 0; n < nn; n++) {
      float x = vxm[n];
      dxh = vdx[n] * 0.5 + chxt;
      xmin = vx1[n] - dxh;
      xmax = vx2[n] + dxh;
      int k1 = -1;
      int k2 = -1;
      for (int i = 0; i < nn; i++) {
        if (i != n) {
          float x1 = vxm[i] - vdx[i] * 0.5 - chxt;
          float x2 = vxm[i] + vdx[i] * 0.5 + chxt;
          if (vxm[i] < x && xmin < x2) {
            xmin = x2;
            k1 = i;
          }
          if (vxm[i] > x && xmax > x1) {
            xmax = x1;
            k2 = i;
          }
        }
      }
      float x1 = x - dxh;
      float x2 = x + dxh;
      if (xmin > x1 && k1 >= 0) {
        if (k1 < n)
          x1 = xmin;
      }
      if (xmax < x2 && k2 >= 0) {
        if (k2 < n)
          x2 = xmax;
      }
      if (x1 < vx1[n] - dxh)
        x1 = vx1[n] - dxh;
      if (x2 > vx2[n] + dxh)
        x2 = vx2[n] + dxh;
      if (x2 - x1 > dxh * 1.99) {
        x = (x1 + x2) * 0.5;
        vxm[n] = x;
        x -= vdx[n] * 0.5;
        fp->drawStr(markName[vpos[n]].c_str(), x, y, 0.0);
      }
    }
  }


}

void VcrossPlot::plotXLabels()
{
  // horizontal part of total crossection
  int ipd1 = 0;
  int ipd2 = 1;
  for (int i = 0; i < nPoint - 1; i++) {
    if (cdata1d[nxs][i] < xPlotmin)
      ipd1 = i;
    if (cdata1d[nxs][i] < xPlotmax)
      ipd2 = i + 1;
  }

  int ip1 = (cdata1d[nxs][ipd1] < xPlotmin) ? ipd1 + 1 : ipd1;
  int ip2 = (cdata1d[nxs][ipd2] > xPlotmax) ? ipd2 - 1 : ipd2;

  float chystp = 1.5;
  float chx = chxdef;
  float chy = chydef;
  float dchy = chy * 0.6;
  float chxt, chyt;

  Colour c;


  chxt = chx;
  chyt = chy;

  // colour etc. for frame etc.
  glLineWidth(vcopt->frameLinewidth);


  float y = yPlotmin - dchy * 0.5;

  if (!timeGraph) {

    // Distance from reference position (default left end)
    if (vcopt->pDistance) {
      c = Colour(vcopt->distanceColour);
      if (c == backColour)
        c = contrastColour;
      glColor3ubv(c.RGB());
      chxt = chx * 0.75;
      chyt = chy * 0.75;
      fp->setFontSize(fontscale * chyt / chy);
      float y1 = y - chyt * chystp;
      y = y - chyt * chystp * 1.2;
      float xlen = chxt * 6.75;
      float xcut = xPlotmax - xlen;
      float x = xPlotmin - xlen * 2.;
      int i = int(refPosition);
      if (i < 0)
        i = 0;
      if (i > nPoint - 2)
        i = nPoint - 2;
      float rpos = cdata1d[nxs][i] + (cdata1d[nxs][i + 1] - cdata1d[nxs][i])
              * (refPosition - float(i));
      float unit;
      std::string uname;
      if (miutil::to_lower(vcopt->distanceUnit) == "nm") {
        unit = 1852.; // Nautical mile
        uname = "nm";
      } else {
        unit = 1000.;
        uname = "km";
      }

      if (vcopt->distanceStep == "grid") { //distance at gridpoints
        for (i = ip1; i <= ip2; i++) {
          if ((cdata1d[nxs][i] > x + xlen && cdata1d[nxs][i] < xcut) || i == ip2) {
            ostringstream xostr;
            xostr << setprecision(1) << setiosflags(ios::fixed) << fabsf(
                (cdata1d[nxs][i] - rpos) / unit);
            std::string xstr = xostr.str() + uname;
            float dx, dy;
            fp->getStringSize(xstr.c_str(), dx, dy);
            x = cdata1d[nxs][i];
            fp->drawStr(xstr.c_str(), x + chxt * 1.5 - dx, y1, 0.0);
            //draw tickmarks
            glBegin(GL_LINES);
            glVertex2f(x, yPlotmin);
            glVertex2f(x, yPlotmin - dchy);
            glVertex2f(x, yPlotmax);
            glVertex2f(x, yPlotmax + dchy);
            glEnd();
          }
        }

      } else { //distance in "step" km or nm
        int step = miutil::to_int(vcopt->distanceStep);
        int i = ip1;
        //pos first possible label
        while (i <= ip2 && (cdata1d[nxs][i] < x + xlen || cdata1d[nxs][i] > xcut))
          i++;
        float p1 = ((cdata1d[nxs][i] - rpos) / unit);
        x = cdata1d[nxs][i];
        //pos nex possible label
        while (i <= ip2 && (cdata1d[nxs][i] < x + xlen || cdata1d[nxs][i] > xcut))
          i++;
        float p2 = ((cdata1d[nxs][i] - rpos) / unit);
        //step
        int minStep = abs(int(p1 - p2) / step * step);
        if (minStep > step)
          step = minStep;
        //pos first label
        int xLabel = int(p1 / step) * step;
        for (i = ip1; i <= ip2; i++) {
          float gridPointDist = ((cdata1d[nxs][i] - rpos) / unit);
          if (gridPointDist > xLabel) {
            //find position of xLabel
            float rpos1 = ((cdata1d[nxs][i] - rpos) / unit);
            float rpos2 = ((cdata1d[nxs][i + 1] - rpos) / unit);
            x = (cdata1d[nxs][i + 1] - (rpos2 - xLabel) / (rpos2 - rpos1)
                * (cdata1d[nxs][i + 1] - cdata1d[nxs][i]));
            //print string
            ostringstream xostr;
            xostr << abs(xLabel);
            std::string xstr = xostr.str() + uname;
            float dx, dy;
            fp->getStringSize(xstr.c_str(), dx, dy);
            fp->drawStr(xstr.c_str(), x + chxt * 1.5 - dx, y1, 0.0);
            //draw tickmarks
            glBegin(GL_LINES);
            glVertex2f(x, yPlotmin);
            glVertex2f(x, yPlotmin - dchy);
            glVertex2f(x, yPlotmax);
            glVertex2f(x, yPlotmax + dchy);
            glEnd();
            xLabel += step;
          }
        }
      }
    }

    // x,y coordinates
    if (vcopt->pXYpos) {
      c = Colour(vcopt->xyposColour);
      if (c == backColour)
        c = contrastColour;
      glColor3ubv(c.RGB());
      chxt = chx;
      chyt = chy;
      fp->setFontSize(fontscale * 0.75);
      float y1 = y - chyt * chystp;
      float y2 = y - chyt * chystp * 2.;
      y = y - chyt * chystp * 2.2;
      float xlen = chxt * 6.75;
      float xcut = xPlotmax - xlen;
      float x = xPlotmin - xlen * 2.;
      for (int i = ip1; i <= ip2; i++) {
        if ((cdata1d[nxs][i] > x + xlen && cdata1d[nxs][i] < xcut) || i == ip2) {
          ostringstream xostr, yostr;
          xostr << setw(6) << setprecision(1) << setiosflags(ios::fixed)
                  << cdata1d[nxg][i];
          yostr << setw(6) << setprecision(1) << setiosflags(ios::fixed)
                  << cdata1d[nyg][i];
          string xstr = xostr.str();
          string ystr = yostr.str();
          float xdx, ydx, dy;
          fp->getStringSize(xstr.c_str(), xdx, dy);
          fp->getStringSize(ystr.c_str(), ydx, dy);
          x = cdata1d[nxs][i];
          fp->drawStr(xstr.c_str(), x + chxt * 1.5 - xdx, y1, 0.0);
          fp->drawStr(ystr.c_str(), x + chxt * 1.5 - ydx, y2, 0.0);
        }
      }
    }

    // latitude,longitude
    if (vcopt->pGeoPos) {
      c = Colour(vcopt->geoposColour);
      if (c == backColour)
        c = contrastColour;
      glColor3ubv(c.RGB());
      chxt = chx;
      chyt = chy;
      fp->setFontSize(fontscale * 0.75);
      float y1 = y - chyt * chystp;
      float y2 = y - chyt * chystp * 2.;
      y = y - chyt * chystp * 2.2;
      float xlen = chxt * 6.75;
      float xcut = xPlotmax - xlen;
      float x = xPlotmin - xlen * 2.;
      for (int i = ip1; i <= ip2; i++) {
        if ((cdata1d[nxs][i] > x + xlen && cdata1d[nxs][i] < xcut) || i == ip2) {
          float glat = cdata1d[nlat][i];
          float glon = cdata1d[nlon][i];
          ostringstream xostr, yostr;
          xostr << setw(5) << setprecision(1) << setiosflags(ios::fixed)
                  << fabsf(glat);
          if (glat < 0.)
            xostr << 'S';
          else
            xostr << 'N';
          yostr << setw(5) << setprecision(1) << setiosflags(ios::fixed)
                  << fabsf(glon);
          if (glon < 0.)
            yostr << 'W';
          else
            yostr << 'E';
          string xstr = xostr.str();
          string ystr = yostr.str();
          float xdx, ydx,dy;
          fp->getStringSize(xstr.c_str(), xdx, dy);
          fp->getStringSize(ystr.c_str(), ydx, dy);
          x = cdata1d[nxs][i];
          fp->drawStr(xstr.c_str(), x + chxt * 1.5 - xdx, y1, 0.0);
          fp->drawStr(ystr.c_str(), x + chxt * 1.5 - ydx, y2, 0.0);
        }
      }
    }

  } else if (timeGraph) {

    // time graph
    if (vcopt->pText) {
      c = Colour(vcopt->textColour);
      if (c == backColour)
        c = contrastColour;
      glColor3ubv(c.RGB());
      // hour,day,month,hour_forecast along x axis (year in text below)
      vector<string> fctext(nPoint);
      vector<bool> showday(nPoint, false);
      unsigned int lmax = 0;
      for (int i = ip1; i <= ip2; i++) {
        ostringstream ostr;
        ostr << setiosflags(ios::showpos) << forecastHourSeries[i];
        string str = ostr.str();
        if (lmax < str.length())
          lmax = str.length();
        fctext[i] = str;
      }
      int ihrmin = validTimeSeries[ip1].hour();
      int ittmin = ip1;
      int ittday = ip1;
      float dxday = cdata1d[nxs][ip2] - cdata1d[nxs][ip1];
      float dxhr = cdata1d[nxs][ip2] - cdata1d[nxs][ip1];
      for (int i = ip1 + 1; i <= ip2; i++) {
        if (validTimeSeries[i].hour() < ihrmin) {
          ihrmin = validTimeSeries[i].hour();
          ittmin = i;
        }
        if (validTimeSeries[i].date() != validTimeSeries[ittday].date()) {
          float dx = cdata1d[nxs][i] - cdata1d[nxs][ittday];
          if (dxday > dx)
            dxday = dx;
          ittday = i;
          showday[i] = true;
        }
        float dx = cdata1d[nxs][i] - cdata1d[nxs][i - 1];
        if (dxhr > dx)
          dxhr = dx;
      }
      chxt = chx * 0.75;
      if (chxt * 3. > dxhr)
        chxt = dxhr / 3.;
      if (chxt * 6. > dxday)
        chxt = dxday / 6.;
      if (chxt * (lmax + 1) > dxday)
        chxt = dxday / (lmax + 1);
      if (ittmin == ip1)
        showday[ip1] = true;
      else if (cdata1d[nxs][ittmin] - cdata1d[nxs][ip1] > chxt * (lmax + 1))
        showday[ip1] = true;
      chyt = chxt * chy / chx;
      float y1 = y - chyt * chystp;
      float y2 = y - chyt * chystp * 2.;
      float y3 = y - chyt * chystp * 3.;
      y = y - chyt * chystp * 3.2;
      fp->setFontSize(fontscale * chyt / chy);
      for (int i = ip1; i <= ip2; i++) {
        float x = cdata1d[nxs][i];
        ostringstream ostr;
        ostr << setw(2) << setfill('0') << validTimeSeries[i].hour();
        string str = ostr.str();
        float dx, dy;
        fp->getStringSize(str.c_str(), dx, dy);
        fp->drawStr(str.c_str(), x - dx * 0.5, y1, 0.0);
        if (showday[i]) {
          ostringstream ostr2;
          ostr2 << validTimeSeries[i].day() << "/"
              << validTimeSeries[i].month();
          str = ostr2.str();
          fp->drawStr(str.c_str(), x - chxt, y2, 0.0);
        }
        fp->drawStr(fctext[i].c_str(), x - chxt, y3, 0.0);
      }
    }
    if (vcopt->pDistance || vcopt->pXYpos || vcopt->pGeoPos) {
      y -= (chy * chystp);
      vector<std::string> vpstr, vpcol;
      int l = 0;
      if (vcopt->pDistance) {
        //----------------------------------------------------------
        int i = int(refPosition);
        if (i < 0)
          i = 0;
        if (i > nPoint - 2)
          i = nPoint - 2;
        float rpos = cdata1d[nxs][i] + (cdata1d[nxs][i + 1] - cdata1d[nxs][i])
                * (refPosition - float(i));
        //----------------------------------------------------------
        ostringstream xostr;
        xostr << "Distance=" << setprecision(1) << setiosflags(ios::fixed)
                << (cdata1d[nxs][i] - rpos) / 1000.;
        std::string xstr = xostr.str();
        miutil::trim(xstr);
        xstr += "km";
        l += xstr.length();
        vpstr.push_back(xstr);
        vpcol.push_back(vcopt->distanceColour);
      }
      // x,y coordinate
      if (vcopt->pXYpos) {
        ostringstream ostr;
        ostr << "X=" << setprecision(1) << setiosflags(ios::fixed) << tgxpos
            << "  Y=" << setprecision(1) << setiosflags(ios::fixed) << tgypos;
        std::string xystr = ostr.str();
        l += xystr.length();
        vpstr.push_back(xystr);
        vpcol.push_back(vcopt->xyposColour);
      }
      // latitude,longitude
      if (vcopt->pGeoPos) {
        float glat = cdata1d[nlat][0];
        float glon = cdata1d[nlon][0];
        ostringstream xostr, yostr;
        xostr << "Lat=" << setprecision(1) << setiosflags(ios::fixed) << fabsf(glat);
        if (glat < 0.)
          xostr << 'S';
        else
          xostr << 'N';
        yostr << "  Long=" << setprecision(1) << setiosflags(ios::fixed) << fabsf(glon);
        if (glon < 0.)
          yostr << 'W';
        else
          yostr << 'E';
        std::string str = xostr.str() + yostr.str();
        l += str.length();
        vpstr.push_back(str);
        vpcol.push_back(vcopt->geoposColour);
      }
      l += (vpstr.size() - 1) * 2;
      chxt = chx;
      if (l * chx > cdata1d[nxs][ipd2] - cdata1d[nxs][ipd1])
        chxt = (cdata1d[nxs][ipd2] - cdata1d[nxs][ipd1]) / float(l);
      chyt = chxt * chy / chx;
      fp->setFontSize(fontscale * chyt / chy);
      float x = xPlotmin;
      for (unsigned int n = 0; n < vpstr.size(); n++) {
        c = Colour(vpcol[n]);
        if (c == backColour)
          c = contrastColour;
        glColor3ubv(c.RGB());
        fp->drawStr(vpstr[n].c_str(), x, y, 0.0);
        x += ((vpstr[n].length() + 2) * chxt);
      }
    }
  }
}


void VcrossPlot::plotLevels()
{
  // horizontal part of total crossection
  int ipd1 = 0;
  int ipd2 = 1;
  for (int i = 0; i < nPoint - 1; i++) {
    if (cdata1d[nxs][i] < xPlotmin)
      ipd1 = i;
    if (cdata1d[nxs][i] < xPlotmax)
      ipd2 = i + 1;
  }

  int npd = ipd2 - ipd1 + 1;

  // find lower and upper level inside current window
  // (contour plot: ilev1 - ilev2    wind plot: iwlev1 - iwlev2)
  int ilev1 = 0;
  int ilev2 = 1;
  for (int k = 0; k < numLev - 1; k++) {
    if (vlimitmax[k] < yPlotmin)
      ilev1 = k;
    if (vlimitmin[k] < yPlotmax)
      ilev2 = k + 1;
  }

  float xylim[4] =
  { xPlotmin, xPlotmax, yPlotmin, yPlotmax };

  Colour c;
  int k1,k2;

  for (int loop = 0; loop < 3; loop++) {
    float lwidth;
    std::string ltype;
    if (loop == 0 && vcopt->pUpperLevel) {
      k1 = k2 = numLev - 1;
      c = Colour(vcopt->upperLevelColour);
      lwidth = vcopt->upperLevelLinewidth;
      ltype = vcopt->upperLevelLinetype;
    } else if (loop == 1 && vcopt->pLowerLevel) {
      k1 = k2 = 0;
      c = Colour(vcopt->lowerLevelColour);
      lwidth = vcopt->lowerLevelLinewidth;
      ltype = vcopt->lowerLevelLinetype;
    } else if (loop == 2 && vcopt->pOtherLevels) {
      k1 = 1;
      k2 = numLev - 2;
      c = Colour(vcopt->otherLevelsColour);
      lwidth = vcopt->otherLevelsLinewidth;
      ltype = vcopt->otherLevelsLinetype;
    } else {
      k1 = k2 = -1;
    }
    if (k1 >= 0) {
      if (k1 < ilev1)
        k1 = ilev1;
      if (k2 > ilev2)
        k2 = ilev2;
      k2++;
    }
    if (k1 < k2) {
      if (c == backColour)
        c = contrastColour;
      glColor3ubv(c.RGB());
      glLineWidth(lwidth);
      Linetype linetype(ltype);
      if (linetype.stipple) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(linetype.factor, linetype.bmap);
      }
      if (vcoord != 4) {
        for (int k = k1; k < k2; k++)
          xyclip(npd, &cdata2d[nx][k * nPoint + ipd1], &cdata2d[ny][k * nPoint + ipd1], xylim);
      } else {
        // theta levels
        for (int k = k1; k < k2; k++) {
          int i = ipd1 - 1;
          while (i < ipd2) {
            i++;
            while (i <= ipd2 && cdata2d[ny][k * nPoint + i] == fieldUndef)
              i++;
            int ibgn = i;
            while (i <= ipd2 && cdata2d[ny][k * nPoint + i] != fieldUndef)
              i++;
            int iend = i;
            if (ibgn < iend - 1)
              xyclip(iend - ibgn, &cdata2d[nx][k * nPoint + ibgn],
                  &cdata2d[ny][k * nPoint + ibgn], xylim);
          }
        }
      }
      UpdateOutput();
      glDisable(GL_LINE_STIPPLE);
    }
  }

}

void VcrossPlot::plotSurfacePressure()
{
  // horizontal part of total crossection
  int ipd1 = 0;
  int ipd2 = 1;
  for (int i = 0; i < nPoint - 1; i++) {
    if (cdata1d[nxs][i] < xPlotmin)
      ipd1 = i;
    if (cdata1d[nxs][i] < xPlotmax)
      ipd2 = i + 1;
  }

//  int ip1 = (cdata1d[nxs][ipd1] < xPlotmin) ? ipd1 + 1 : ipd1;
//  int ip2 = (cdata1d[nxs][ipd2] > xPlotmax) ? ipd2 - 1 : ipd2;
  int npd = ipd2 - ipd1 + 1;

  float xylim[4] =
  { xPlotmin, xPlotmax, yPlotmin, yPlotmax };


  Colour c(vcopt->surfaceColour);
  if (c == backColour)
    c = contrastColour;
  glColor3ubv(c.RGB());
  glLineWidth(vcopt->surfaceLinewidth);
  Linetype linetype(vcopt->surfaceLinetype);
  if (linetype.stipple) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(linetype.factor, linetype.bmap);
  }

  if (vcopt->pSurface && npy1 >= 0) {

    if (vcoord != 5) {
      xyclip(npd, &cdata1d[nxs][ipd1], &cdata1d[npy1][ipd1], xylim);
    } else if (vcoord == 5) {
      // sea bottom: coast drawn between the horizontal points
      int i = ipd1 - 1;
      while (i < ipd2) {
        i++;
        while (i <= ipd2 && cdata1d[npy1][i] == fieldUndef)
          i++;
        int ibgn = i;
        while (i <= ipd2 && cdata1d[npy1][i] != fieldUndef)
          i++;
        int iend = i - 1;
        if (ibgn <= ipd2) {
          if (ibgn > ipd1) {
            float x = (cdata1d[nxs][ibgn - 1] + cdata1d[nxs][ibgn]) * 0.5;
            float xline[3] =
            { x, x, cdata1d[nxs][ibgn] };
            float yline[3] =
            { yPlotmax, 0., cdata1d[npy1][ibgn] };
            xyclip(3, xline, yline, xylim);
          }
          xyclip(iend - ibgn + 1, &cdata1d[nxs][ibgn], &cdata1d[npy1][ibgn], xylim);
          if (iend < ipd2) {
            float x = (cdata1d[nxs][iend] + cdata1d[nxs][iend + 1]) * 0.5;
            float xline[3] =
            { cdata1d[nxs][iend], x, x };
            float yline[3] =
            { cdata1d[npy1][iend], 0., yPlotmax };
            xyclip(3, xline, yline, xylim);
          }
        }
      }
    }
  }

  if (npy2 >= 0) {
    xyclip(npd, &cdata1d[nxs][ipd1], &cdata1d[npy2][ipd1], xylim);
  }

  UpdateOutput();
  glDisable(GL_LINE_STIPPLE);

}

void VcrossPlot::plotVerticalGridLines()
{
  // horizontal part of total crossection
  int ipd1 = 0;
  int ipd2 = 1;
  for (int i = 0; i < nPoint - 1; i++) {
    if (cdata1d[nxs][i] < xPlotmin)
      ipd1 = i;
    if (cdata1d[nxs][i] < xPlotmax)
      ipd2 = i + 1;
  }

  int ip1 = (cdata1d[nxs][ipd1] < xPlotmin) ? ipd1 + 1 : ipd1;
  int ip2 = (cdata1d[nxs][ipd2] > xPlotmax) ? ipd2 - 1 : ipd2;

  Colour c(vcopt->vergridColour);
  if (c == backColour)
    c = contrastColour;
  glColor3ubv(c.RGB());
  glLineWidth(vcopt->vergridLinewidth);
  Linetype linetype(vcopt->vergridLinetype);
  if (linetype.stipple) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(linetype.factor, linetype.bmap);
  }
  glBegin(GL_LINES);
  for (int i = ip1; i <= ip2; i++) {
    glVertex2f(cdata1d[nxs][i], yPlotmin);
    glVertex2f(cdata1d[nxs][i], yPlotmax);
  }
  glEnd();
  UpdateOutput();
  glDisable(GL_LINE_STIPPLE);
}

void VcrossPlot::plotMarkerLines()
{
  // horizontal part of total crossection
  int ipd1 = 0;
  int ipd2 = 1;
  for (int i = 0; i < nPoint - 1; i++) {
    if (cdata1d[nxs][i] < xPlotmin)
      ipd1 = i;
    if (cdata1d[nxs][i] < xPlotmax)
      ipd2 = i + 1;
  }

  float xylim[4] =
  { xPlotmin, xPlotmax, yPlotmin, yPlotmax };

  int numPar1d = cdata1d.size();
  vector<int> mlines;
  if (vcoordPlot == vcv_height) {
    for (int n = 0; n < numPar1d; n++) {
      if (idPar1d[n] == 907)
        mlines.push_back(n); // inflight height above sealevel
    }
  } else if (vcoordPlot == vcv_exner || vcoordPlot == vcv_pressure) {
    for (int n = 0; n < numPar1d; n++) {
      if (idPar1d[n] == 908)
        mlines.push_back(n); // inflight pressure
    }
  }
  if (mlines.size() > 0) {
    Colour c(vcopt->markerlinesColour);
    if (c == backColour)
      c = contrastColour;
    glColor3ubv(c.RGB());
    glLineWidth(vcopt->markerlinesLinewidth);
    Linetype linetype(vcopt->markerlinesLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    float *y1d = new float[nPoint];
    for (unsigned int m = 0; m < mlines.size(); m++) {
      int n = mlines[m];
      int i = ipd1 - 1;
      while (i < ipd2) {
        i++;
        while (i <= ipd2 && cdata1d[n][i] == fieldUndef)
          i++;
        int ibgn = i;
        while (i <= ipd2 && cdata1d[n][i] != fieldUndef)
          i++;
        int iend = i;
        if (ibgn < iend - 1) {
          if (vcoordPlot == vcv_exner) {
            for (int j = ibgn; j < iend; j++) {
              float p = cdata1d[n][j];
              float pi = cp * powf(p * p0inv, kappa);
              y1d[j] = yconst + yscale * pi;
            }
          } else {
            for (int j = ibgn; j < iend; j++) {
              y1d[j] = yconst + yscale * cdata1d[n][j];
            }
          }
          xyclip(iend - ibgn, &cdata1d[nxs][ibgn], &y1d[ibgn], xylim);
        }
      }
    }
    delete[] y1d;
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
  }
}


void VcrossPlot::plotVerticalMarkerLines()
{
  // horizontal part of total crossection
  int ipd1 = 0;
  int ipd2 = 1;
  for (int i = 0; i < nPoint - 1; i++) {
    if (cdata1d[nxs][i] < xPlotmin)
      ipd1 = i;
    if (cdata1d[nxs][i] < xPlotmax)
      ipd2 = i + 1;
  }

  int ip1 = (cdata1d[nxs][ipd1] < xPlotmin) ? ipd1 + 1 : ipd1;
  int ip2 = (cdata1d[nxs][ipd2] > xPlotmax) ? ipd2 - 1 : ipd2;

  // vertical marker lines
  if (vcopt->pVerticalMarker) {
    if (ip2 - ip1 > 3) {
      Colour c(vcopt->verticalMarkerColour);
      if (c == backColour)
        c = contrastColour;
      glColor3ubv(c.RGB());
      glLineWidth(vcopt->verticalMarkerLinewidth);
      Linetype linetype(vcopt->verticalMarkerLinetype);
      if (linetype.stipple) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(linetype.factor, linetype.bmap);
      }
      glBegin(GL_LINES);
      float delta1 = cdata1d[nxs][ip1 + 1] - cdata1d[nxs][ip1];
      for (int i = ip1 + 1; i < ip2; i++) {
        float delta2 = cdata1d[nxs][i + 1] - cdata1d[nxs][i];
        if (fabs(delta1 - delta2) > 1.0) {
          delta1 = delta2;
          glVertex2f(cdata1d[nxs][i], yPlotmin);
          glVertex2f(cdata1d[nxs][i], yPlotmax);
        }
      }
      glEnd();
      UpdateOutput();
      glDisable(GL_LINE_STIPPLE);
    }
  }
}


void VcrossPlot::plotAnnotations(const vector<std::string>& labels)
{
  //Annotations
  float xoffset = (xPlotmax - xPlotmin) / 50;
  float yoffset = (yPlotmax - yPlotmin) / 50;
  bool left, top;
  int nlabels = labels.size();

  for (int i = 0; i < nlabels; i++) {
    left = top = true;
    vector<std::string> tokens = miutil::split_protected(labels[i], '"', '"');
    int ntokens = tokens.size();
    std::string text;
    float unit = 1.0, xfac = 1.0, yfac = 1.0;
    bool arrow = false;
    vector<float> arrow_x, arrow_y;
    Colour tcolour, fcolour, bcolour;
    float yoffsetfac = 1.0, xoffsetfac = 1.0;
    for (int j = 0; j < ntokens; j++) {
      vector<std::string> stokens = split(tokens[j], '<', '>');
      int nstokens = stokens.size();
      if (nstokens > 0) {
        for (int k = 0; k < nstokens; k++) {
          vector<std::string> sstokens = miutil::split_protected(stokens[k], '\"', '\"', ",", true);
          int nsstokens = sstokens.size();
          for (int l = 0; l < nsstokens; l++) {
            vector<std::string> ssstokens = miutil::split_protected(sstokens[l], '\"', '\"', "=", true);
            if (ssstokens.size() == 2) {
              if (miutil::to_upper(ssstokens[0]) == "TEXT") {
                text = ssstokens[1];
              } else if (miutil::to_upper(ssstokens[0]) == "ARROW") {
                unit = miutil::to_double(ssstokens[1]);;
                arrow = true;
              } else if (miutil::to_upper(ssstokens[0]) == "XFAC") {
                xfac = miutil::to_double(ssstokens[1]);
              } else if (miutil::to_upper(ssstokens[0]) == "YFAC") {
                yfac = miutil::to_double(ssstokens[1]);;
              }
            }
          }
        }
      } else {
        vector<std::string> stokens = miutil::split(tokens[j], "=");
        if (stokens.size() == 2) {
          if (miutil::to_upper(stokens[0]) == "TCOLOUR") {
            tcolour = Colour(stokens[1]);
          } else if (miutil::to_upper(stokens[0]) == "FCOLOUR") {
            fcolour = Colour(stokens[1]);
          } else if (miutil::to_upper(stokens[0]) == "BCOLOUR") {
            bcolour = Colour(stokens[1]);
          } else if (miutil::to_upper(stokens[0]) == "VALIGN" && miutil::to_upper(stokens[1])
              == "BOTTOM") {
            top = false;
          } else if (miutil::to_upper(stokens[0]) == "HALIGN" && miutil::to_upper(stokens[1])
              == "RIGHT") {
            left = false;
          } else if (miutil::to_upper(stokens[0]) == "XOFFSET") {
            xoffsetfac = miutil::to_double(stokens[1]);
          } else if (miutil::to_upper(stokens[0]) == "YOFFSET") {
            yoffsetfac = miutil::to_double(stokens[1]);
          }
        }
      }
    }

    xoffset *= xoffsetfac;
    yoffset *= yoffsetfac;
    float ddy = 0., ddx = 0.;
    if (arrow) {
      ddy = 3600 * unit * v2hRatio * yfac;
      ddx = 3600 * unit * xfac;
    }
    float dx,dy;
    fp->setFontSize(fontscale);
    fp->getStringSize(text.c_str(), dx, dy);
    dy = dy > ddy ? dy : ddy;
    dx += ddx;

    float xpos, ypos;
    if (left) {
      xpos = xPlotmin + xoffset;
    } else {
      xpos = xPlotmax - xoffset - dx;
    }

    if (top) {
      ypos = yPlotmax - yoffset;
    } else {
      ypos = yPlotmin + yoffset + dy * 2;
    }

    //textbox
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3ubv(fcolour.RGBA());
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1);
    glBegin(GL_POLYGON);
    glVertex2f(xpos, ypos);
    glVertex2f(xpos + xoffset * 0.6 + dx, ypos);
    glVertex2f(xpos + xoffset * 0.6 + dx, ypos - dy * 2);
    glVertex2f(xpos, ypos - dy * 2);
    glEnd();
    glDisable(GL_BLEND);

    glColor3ubv(bcolour.RGB());
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_POLYGON);
    glVertex2f(xpos, ypos);
    glVertex2f(xpos + xoffset * 0.6 + dx, ypos);
    glVertex2f(xpos + xoffset * 0.6 + dx, ypos - dy * 2);
    glVertex2f(xpos, ypos - dy * 2);
    glEnd();

    glColor3ubv(tcolour.RGB());

    if (arrow) {
      plotArrow(xpos + xoffset * 0.3, ypos - dy * 1.4, ddx, ddy, true);
    }
    if (not text.empty()) {
      miutil::remove(text, '"');
      text = validTime.format(text);
      miutil::trim(text);
      fp->drawStr(text.c_str(), xpos + xoffset * 0.3 + ddx * 2,
          ypos - dy * 1.4, 0.0);
    }
  }
  //end annotations
}


void VcrossPlot::plotFrame()
{
  const int nzsteps = 10;
  float zsteps[nzsteps] =
  { 5., 10., 25., 50., 100., 250., 500., 1000., 2500., 5000. };

  const int nflsteps = 8;
  float flsteps[nflsteps] =
  { 1., 2., 5., 10., 50., 100., 200., 500. };

  const int npfixed1 = 17;
  float pfixed1[npfixed1] =
  { 1000., 925., 850., 700., 600., 500., 400., 300., 250., 200., 150., 100.,
      70., 50., 30., 10., 5. };
  const int npfixed2 = 22;
  float pfixed2[npfixed2] =
  { 1000., 950., 900., 850., 800., 750., 700., 650., 600., 550., 500., 450.,
      400., 350., 300., 250., 200., 150., 100., 50., 10., 5. };
  // P -> FlightLevels (used for remapping fields from P to FL)
  const int mfl = 16;
  const int plevels[mfl] =
  { 1000, 925, 850, 700, 600, 500, 400, 300, 250, 200, 150, 100, 70, 50, 30,
      10 };
  const int flevels[mfl] =
  { 0, 25, 50, 100, 140, 180, 240, 300, 340, 390, 450, 530, 600, 700, 800,
      999 };

  // Warning: pressure at flight levels found by looking at an Amble diagram!
  // (used for Vertical Profiles)
  const int mflvl = 45;
  int iflvl[mflvl] =
  { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160,
      170, 180, 190, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300,
      310, 320, 330, 340, 350, 360, 370, 380, 390, 400, 450, 500, 550, 600 };
  float pflvl[mflvl] =
  { 1013., 978., 942., 907., 874., 843., 812., 782., 751., 722., 696., 670.,
      642., 619., 595., 572., 549., 526., 505., 485., 465., 445., 428., 410.,
      393., 378., 359., 344., 329., 314., 300., 288., 274., 261., 250., 239.,
      227., 216., 206., 197., 188., 147., 116., 91., 71. };

  const float fl2m = 1. / 3.2808399; // flightlevel (100 feet unit) to meter


  int npfixed = 0;
  float *pfixed = 0;
  float *ypfixed = 0;

  if (vcoord != 5 && vcoord != 11) {
    // fixed pressure levels possibly shown..
    if (vcoordPlot == vcv_exner) {
      if (verticalAxis == "hPa") {
        npfixed = npfixed1;
        pfixed = new float[npfixed];
        ypfixed = new float[npfixed];
        for (int k = 0; k < npfixed; k++) {
          float p =pfixed[k] = pfixed1[k];
          float pi =cp * powf(p * p0inv, kappa);
          ypfixed[k] = yconst + yscale * pi;
        }
      } else if (verticalAxis == "FL") {
        npfixed = mfl;
        pfixed = new float[npfixed];
        ypfixed = new float[npfixed];
        for (int k = 0; k < npfixed; k++) {
          pfixed[k] = flevels[k];
          float p =plevels[k];
          float pi =cp * powf(p * p0inv, kappa);
          ypfixed[k] = yconst + yscale * pi;
        }
      }
    } else if (vcoordPlot == vcv_pressure) {
      if (verticalAxis == "hPa") {
        npfixed = npfixed2;
        pfixed = new float[npfixed];
        ypfixed = new float[npfixed];
        for (int k = 0; k < npfixed; k++) {
          float p =pfixed[k] = pfixed2[k];
          ypfixed[k] = yconst + yscale * p;
        }
      } else if (verticalAxis == "FL") {
        npfixed = mflvl;
        pfixed = new float[npfixed];
        ypfixed = new float[npfixed];
        for (int k = 0; k < npfixed; k++) {
          pfixed[k] = iflvl[k];
          float p =pflvl[k];
          ypfixed[k] = yconst + yscale * p;
        }
      }
    } else if (vcoordPlot == vcv_height) {
      float zmin = (yPlotmin - yconst) / yscale;
      float zmax = (yPlotmax - yconst) / yscale;
      float dz = zmax - zmin;
      if (verticalAxis == "m") {
        int n = 1;
        while (n < nzsteps && zsteps[n] * 10. < dz)
          n++;
        n--;
        npfixed = int(dz / zsteps[n]) + 1;
        int k = int(zmin / zsteps[n]) + 1;
        if (zmin < 0.)
          k--;
        pfixed = new float[npfixed];
        ypfixed = new float[npfixed];
        for (int i = 0; i < npfixed; i++) {
          pfixed[i] = zsteps[n] * float(k + i);
          ypfixed[i] = yconst + yscale * zsteps[n] * float(k + i);
        }
      } else if (verticalAxis == "Ft") {
        zmin /= fl2m;
        zmax /= fl2m;
        dz /= fl2m;
        int n = 1;
        while (n < nflsteps && flsteps[n] * 10. < dz)
          n++;
        n--;
        npfixed = int(dz / flsteps[n]) + 1;
        int k = int(zmin / flsteps[n]) + 1;
        if (zmin < 0.)
          k--;
        pfixed = new float[npfixed];
        ypfixed = new float[npfixed];
        for (int i = 0; i < npfixed; i++) {
          pfixed[i] = flsteps[n] * float(k + i);
          ypfixed[i] = (yconst + yscale * flsteps[n] * float(k + i)) * fl2m;
        }
      }
    }
  }

  // frame

  Linetype linetype;
  float y1,y2;
  int k1,k2;
  float chx = chxdef;
  float chy = chydef;
  float chxt, chyt;
  float dchy = chy * 0.6;

  fp->setFont("BITMAPFONT");

  // horizontal part of total crossection
  int ipd1 = 0;
  int ipd2 = 1;
  for (int i = 0; i < nPoint - 1; i++) {
    if (cdata1d[nxs][i] < xPlotmin)
      ipd1 = i;
    if (cdata1d[nxs][i] < xPlotmax)
      ipd2 = i + 1;
  }
  int ip1 = (cdata1d[nxs][ipd1] < xPlotmin) ? ipd1 + 1 : ipd1;
  int ip2 = (cdata1d[nxs][ipd2] > xPlotmax) ? ipd2 - 1 : ipd2;


  Colour c;

  //  float xylim[4] =
  //    { xPlotmin, xPlotmax, yPlotmin, yPlotmax };

  chxt = chx;
  chyt = chy;

  // colour etc. for frame etc.
  c = Colour(vcopt->frameColour);
  if (c == backColour)
    c = contrastColour;
  glColor3ubv(c.RGB());
  glLineWidth(vcopt->frameLinewidth);

  if (vcopt->pFrame) {
    linetype = Linetype(vcopt->frameLinetype);
    if (linetype.stipple) {
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(linetype.factor, linetype.bmap);
    }
    glBegin(GL_LINE_LOOP);
    glVertex2f(xPlotmin, yPlotmin);
    glVertex2f(xPlotmax, yPlotmin);
    glVertex2f(xPlotmax, yPlotmax);
    glVertex2f(xPlotmin, yPlotmax);
    glEnd();
    UpdateOutput();
    glDisable(GL_LINE_STIPPLE);
    if (vcopt->pLevelNumbers) {
      if ((vcoord != 5 && vcoord != 11) && npfixed > 0) {
        y1 = yPlotmin - (yPlotmax - yPlotmin) * 0.005;
        y2 = yPlotmax + (yPlotmax - yPlotmin) * 0.005;
        k1 = -1;
        k2 = -2;
        for (int k = 0; k < npfixed; k++) {
          if (ypfixed[k] > y1 && ypfixed[k] < y2) {
            if (k1 < 0)
              k1 = k;
            k2 = k;
          }
        }
        glBegin(GL_LINES);
        for (int k = k1; k <= k2; k++) {
          glVertex2f(xPlotmin - chx, ypfixed[k]);
          glVertex2f(xPlotmin, ypfixed[k]);
          glVertex2f(xPlotmax, ypfixed[k]);
          glVertex2f(xPlotmax + chx, ypfixed[k]);
        }
        glEnd();

        if (vcopt->pLevelNumbers && k1 >= 0) {
          chyt = chy;
          chxt = chyt * chx / chy;
          fp->setFontSize(fontscale * chyt / chy);
          float x1 = xPlotmin - chx - chxt * 0.5;
          float x2 = xPlotmax + chx + chxt * 0.5;
          float y1 = yPlotmin - chy * 2.;
          for (int k = k1; k <= k2; k++) {
            float y = ypfixed[k] - chyt * 0.5;
            if (y > y1) {
              ostringstream ostr;
              string str;
              //        if (verticalAxis=="FL") {
              //          ostr << setw(3) << setfill('0') << int(pfixed[k]);
              //          str= verticalAxis + ostr.str();
              //        } else {
              ostr << setw(4) << int(pfixed[k]);
              str = ostr.str() + verticalAxis;
              //        }
              float dx,dy;
              fp->getStringSize(str.c_str(), dx, dy);
              fp->drawStr(str.c_str(), x1 - dx, y, 0.0);
              fp->drawStr(str.c_str(), x2, y, 0.0);
              y1 = y + chyt * 2;
            }
          }
          // needed after drawStr, otherwise colour change may not work
          glShadeModel(GL_FLAT);
        }
      } else if (vcoord == 5) {
        // sea depth
        float y1 = yPlotmin - (yPlotmax - yPlotmin) * 0.005;
        float y2 = yPlotmax + (yPlotmax - yPlotmin) * 0.005;
        k1 = -1;
        k2 = -2;
        for (int k = 0; k < numLev; k++) {
          float y = v2hRatio * alevel[k];
          if (y > y1 && y < y2) {
            if (k1 < 0)
              k1 = k;
            k2 = k;
          }
        }
        glBegin(GL_LINES);
        for (int k = k1; k <= k2; k++) {
          float y = v2hRatio * alevel[k];
          glVertex2f(xPlotmin - chx, y);
          glVertex2f(xPlotmin, y);
          glVertex2f(xPlotmax, y);
          glVertex2f(xPlotmax + chx, y);
        }
        glEnd();
        if (vcopt->pLevelNumbers && k1 >= 0) {
          chyt = chy;
          chxt = chyt * chx / chy;
          fp->setFontSize(fontscale * chyt / chy);
          float x1 = xPlotmin - chx - chxt * 4.5;
          float x2 = xPlotmax + chx + chxt * 0.5;
          float y1 = yPlotmin - chy * 2.;
          for (int k = k1; k <= k2; k++) {
            float y = v2hRatio * alevel[k] - chyt * 0.5;
            if (y > y1) {
              ostringstream ostr;
              ostr << setw(4) << -int(alevel[k] - 0.5);
              string str = ostr.str();
              fp->drawStr(str.c_str(), x1, y, 0.0);
              fp->drawStr(str.c_str(), x2, y, 0.0);
              y1 = y + chyt * 2;
            }
          }
          // needed after drawStr, otherwise colour change may not work
          glShadeModel(GL_FLAT);
        }
      } else if (vcoord == 11) {
        // sigma height
        int izmin, izmax, izstp, kz1, kz2;
        float y1 = yPlotmin - (yPlotmax - yPlotmin) * 0.005;
        float y2 = yPlotmax + (yPlotmax - yPlotmin) * 0.005;
        izmin = int(yDatamin / v2hRatio + 0.5);
        izmax = int(yDatamax / v2hRatio + 0.5);
        izstp = 500;
        if (izmax - izmin > 6000)
          izstp = 1000;
        kz1 = (izmin + izstp - 1) / izstp;
        kz2 = izmax / izstp;
        k1 = -1;
        k2 = -2;
        for (int k = kz1; k <= kz2; k++) {
          float y = v2hRatio * float(izstp * k);
          if (y > y1 && y < y2) {
            if (k1 < 0)
              k1 = k;
            k2 = k;
          }
        }
        glBegin(GL_LINES);
        for (int k = k1; k <= k2; k++) {
          float y = v2hRatio * float(izstp * k);
          glVertex2f(xPlotmin - chx, y);
          glVertex2f(xPlotmin, y);
          glVertex2f(xPlotmax, y);
          glVertex2f(xPlotmax + chx, y);
        }
        glEnd();
        if (vcopt->pLevelNumbers && k1 >= 0) {
          chyt = chy;
          chxt = chyt * chx / chy;
          fp->setFontSize(fontscale * chyt / chy);
          float x1 = xPlotmin - chx - chxt * 5.5;
          float x2 = xPlotmax + chx + chxt * 0.5;
          float y1 = yPlotmin - chy * 2.;
          for (int k = k1; k <= k2; k++) {
            float y = v2hRatio * float(izstp * k) - chyt * 0.5;
            if (y > y1) {
              ostringstream ostr;
              ostr << setw(5) << izstp * k;
              string str = ostr.str();
              fp->drawStr(str.c_str(), x1, y, 0.0);
              fp->drawStr(str.c_str(), x2, y, 0.0);
              y1 = y + chyt * 2;
            }
          }
          // needed after drawStr, otherwise colour change may not work
          glShadeModel(GL_FLAT);
        }
      }
      // horizontal grid resolution
      glBegin(GL_LINES);
      for (int i = ip1; i <= ip2; i++) {
        glVertex2f(cdata1d[nxs][i], yPlotmin);
        glVertex2f(cdata1d[nxs][i], yPlotmin - dchy * 0.5);
        glVertex2f(cdata1d[nxs][i], yPlotmax);
        glVertex2f(cdata1d[nxs][i], yPlotmax + dchy * 0.5);
      }
      glEnd();
      UpdateOutput();
    }
  }
  if (pfixed)
    delete[] pfixed;
  if (ypfixed)
    delete[] ypfixed;

}

bool VcrossPlot::plotData(const std::string& fieldname, PlotOptions& poptions)
{
  METLIBS_LOG_SCOPE();

  // Vertical coordinates handled:
  // vcoord:  2 = sigma (0.-1. with ps and ptop) ... Norlam
  //         10 = eta (hybrid) ... Hirlam, Ecmwf,...
  //          1 = pressure
  //          4 = isentropic surfaces (potential temp., with p(th))
  //          5 = z levels from sea model (possibly incl. sea elevation and bottom)
  //         11 = sigma height levels (MEMO,MC2)
  //         12 = sigma.MM5 (input P in all levels)


  const int nbitwd = sizeof(int) * 8;

  // avoid coredump...
  if (xPlotmin >= xPlotmax || yPlotmin >= yPlotmax || numLev<=0) {
    METLIBS_LOG_DEBUG(LOGVAL(xPlotmin) << LOGVAL(xPlotmax) << LOGVAL(yPlotmin) << LOGVAL(yPlotmax) << LOGVAL(numLev));
    return false;
  }

  int i, i1, i2, j, k, n, nn;
  float p, pi, x, y, dx;
  float xline[5], yline[5];

  const int npfixed1 = 16;
  float pfixed1[npfixed1] =
  { 1000., 925., 850., 700., 600., 500., 400., 300., 250., 200., 150., 100.,
      70., 50., 30., 10. };
  const int npfixed2 = 21;
  float pfixed2[npfixed2] =
  { 1000., 950., 900., 850., 800., 750., 700., 650., 600., 550., 500., 450.,
      400., 350., 300., 250., 200., 150., 100., 50., 10. };
  int npfixed = 0;
  float *pfixed = 0;
  float *ypfixed = 0;

  if (vcoord != 5 && vcoord != 11) {
    // fixed pressure levels possibly shown..
    if (vcoordPlot == vcv_exner) {
      npfixed = npfixed1;
      pfixed = pfixed1;
      ypfixed = new float[npfixed];
      for (k = 0; k < npfixed; k++) {
        p =pfixed[k];
        pi =cp * powf(p * p0inv, kappa);
        ypfixed[k] = yconst + yscale * pi;
      }
    } else if (vcoordPlot == vcv_pressure) {
      npfixed = npfixed2;
      pfixed = pfixed2;
      ypfixed = new float[npfixed];
      for (k = 0; k < npfixed; k++) {
        p =pfixed[k];
        ypfixed[k] = yconst + yscale * p;
      }
    }
  }

  bool *windlevel = new bool[numLev];
  // wind plotted in all levels .... how to change ???
  for (k = 0; k < numLev; k++)
    windlevel[k] = true;

  int ipd1, ipd2, ilev1, ilev2, iwpd1, iwpd2, iwlev1, iwlev2;
  float xylim[4];

  // horizontal part of total crossection
  ipd1 = 0;
  ipd2 = 1;
  iwpd1 = 0;
  iwpd2 = 1;
  for (i = 0; i < nPoint - 1; i++) {
    if (cdata1d[nxs][i] < xPlotmin)
      ipd1 = i;
    if (cdata1d[nxs][i] < xPlotmax)
      ipd2 = i + 1;
    if (cdata1d[nxs][i] < xPlotmin)
      iwpd1 = i + 1;
    if (cdata1d[nxs][i] < xPlotmax)
      iwpd2 = i;
  }
  if (cdata1d[nxs][nPoint - 1] <= xPlotmax)
    iwpd2 = nPoint - 1;

  // find lower and upper level inside current window
  // (contour plot: ilev1 - ilev2    wind plot: iwlev1 - iwlev2)
  ilev1 = 0;
  ilev2 = 1;
  iwlev1 = 0;
  iwlev2 = 1;
  for (k = 0; k < numLev - 1; k++) {
    if (vlimitmax[k] < yPlotmin)
      ilev1 = k;
    if (vlimitmin[k] < yPlotmax)
      ilev2 = k + 1;
    if (vlimitmax[k] < yPlotmin)
      iwlev1 = k + 1;
    if (vlimitmin[k] < yPlotmax)
      iwlev2 = k;
  }
  if (vlimitmin[numLev - 1] <= yPlotmax)
    iwlev2 = numLev - 1;

  // length of wind/vector arrows if Auto density
  float pdw, rw1, rw2, rwindAuto;
  int hstepAuto, npdw;
  pdw = float(ipd2 - ipd1) / 30.;
  hstepAuto = int(pdw + 0.5);
  if (hstepAuto < 1)
    hstepAuto = 1;
  pdw = float(ipd2 - ipd1) / float(hstepAuto);
  npdw = int(pdw + 0.5);
  if (npdw < 1)
    npdw = 1;
  rw1 = (xPlotmax - xPlotmin) / float(npdw);
  rw2 = (yPlotmax - yPlotmin) / float(ilev2 - ilev1);
  rwindAuto = (rw1 < rw2) ? rw1 : rw2;

  int iwpdm = (iwpd1 + iwpd2) / 2;
  int iwpd[5] =
  { iwpd1, (iwpd1 + iwpdm) / 2, iwpdm, (iwpdm + iwpd2) / 2, iwpd2 };
  int kprev = iwlev1;
  for (k = iwlev1 + 1; k <= iwlev2; k++) {
    float dy = 0.;
    for (int i = 0; i < 5; i++)
      dy += (cdata2d[ny][k * nPoint + iwpd[i]] - cdata2d[ny][kprev * nPoint
                                                             + iwpd[i]]);
    dy /= 5.;
    if (dy > rwindAuto * 0.8)
      kprev = k;
    else
      windlevel[k] = false;
  }

  Colour c;

  // extrapolation and line removal needs existing bottom
  bool bottomext = (vcoord == 5 && vcopt->extrapolateToBottom && npy1 >= 0);

  // if bottom extrapolation, then lines below bottom are hidden
  // by an OpenGL stencil (all scalar fields)

  float ylim[2];

  // for line contouring etc.
  int part[4] =
  { ipd1, ipd2, ilev1, ilev2 };
  int partwind[4] =
  { iwpd1, iwpd2, iwlev1, iwlev2 };
  xylim[0] = xPlotmin;
  xylim[1] = xPlotmax;
  xylim[2] = yPlotmin;
  xylim[3] = yPlotmax;

  ylim[0] = yPlotmin;
  ylim[1] = yPlotmax;

  std::string fname = miutil::to_lower(fieldname);

  map<std::string, vcField>::iterator vcf = vcFields.find(fname);

  if (vcf == vcFields.end())
    return false;

  vector<int> index2d;

  n = vcf->second.vars.size();

  for (i = 0; i < n; i++) {
    j = findParam(vcf->second.vars[i]);
    METLIBS_LOG_DEBUG(LOGVAL(vcf->second.vars[i]) << LOGVAL(j));
    if (j < 0)
      return false;
    index2d.push_back(j);
  }

  int no1 = index2d[0];
  int no2 = -1;
  if (n > 1)
    no2 = index2d[1];

  Colour linecolour = poptions.linecolour;
  if (poptions.linecolour == backColour)
    poptions.linecolour = contrastColour;

  bool ok = true;

  if (vcf->second.plotType == vcpt_contour) {

    float chxlab, chylab;
    int labfmt[3] =
    { -1, 0, 0 };
    if (poptions.valueLabel == 0)
      labfmt[0] = 0;
    else
      labfmt[0] = -1;
    labfmt[1] = 0;
    labfmt[2] = 0;

    if (labfmt[0] != 0) {
      float fontsize = 10. * poptions.labelSize;
      fp->set(poptions.fontname, poptions.fontface, fontsize);
      fp->getCharSize('0', chxlab, chylab);
      // the real height for numbers 0-9 (width is ok)
      chylab *= 0.75;
    } else {
      chxlab = chylab = 1.0;
    }

    int ibmap;
    int nxbmap, nybmap, lbmap;
    float rbmap[4];
    int *bmap = 0;

    bool stencil = false;
    bool scissor = false;

    if (bottomext) {
      if (nwork < 0)
        nwork = addPar2d(-8888);
      for (i = 0; i < nTotal; i++)
        cdata2d[nwork][i] = cdata2d[no1][i];
      no1 = nwork;
      //######bottomExtrapolate(ipo);
      replaceUndefinedValues(nPoint, numLev, cdata2d[no1], true);
    }

    if (bottomext && labfmt[0] != 0 && npy1>=0) {

      ibmap = 1;
      // contour masking to prevent partial labels shown near bottom on plot
      dx = chxlab * 0.5;
      nxbmap = int((xPlotmax - xPlotmin) / dx + 2.);
      nybmap = int((yPlotmax - yPlotmin) / dx + 2.);
      lbmap = (nxbmap * nybmap + nbitwd - 1) / nbitwd;
      // It is very dagnerous to allocate a vector of size 0!
      if (lbmap != 0)
        bmap = new int[lbmap];
      rbmap[0] = xPlotmin;
      rbmap[1] = yPlotmin;
      rbmap[2] = dx;
      rbmap[3] = dx;
      // clear bitmap
      for (i = 0; i < lbmap; i++)
        bmap[i] = 0;
      // mark area below bottom in the bitmap
      for (i = ipd1; i < ipd2; i++) {
        if (cdata1d[npy1][i] != fieldUndef && cdata1d[npy1][i + 1]
                                                            != fieldUndef) {
          xline[0] = cdata1d[nxs][i];
          xline[1] = cdata1d[nxs][i + 1];
          yline[0] = cdata1d[npy1][i];
          yline[1] = cdata1d[npy1][i + 1];
          nn = 1;
        } else {
          xline[0] = cdata1d[nxs][i];
          xline[1] = 0.5 * (cdata1d[nxs][i] + cdata1d[nxs][i + 1]);
          xline[2] = cdata1d[nxs][i + 1];
          yline[0] = cdata1d[npy1][i];
          yline[1] = 0.;
          yline[2] = cdata1d[npy1][i + 1];
          if (yline[0] == fieldUndef)
            yline[0] = 0.;
          if (yline[2] == fieldUndef)
            yline[2] = 0.;
          //##          if (yline[0]==fieldUndef) yline[0]=yPlotmax;
          //##          if (yline[2]==fieldUndef) yline[2]=yPlotmax;
          nn = 2;
        }
        for (n = 0; n < nn; n++) {
          //        i1= int((xline[n]  -rbmap[0])/rbmap[2])+1;
          //        i2= int((xline[n+1]-rbmap[0])/rbmap[2])+1;
          i1 = int((xline[n] - rbmap[0]) / rbmap[2]);
          i2 = int((xline[n + 1] - rbmap[0]) / rbmap[2]);
          //          if (i>ipd1 || n>1) i1++;
          if (n > 0)
            i1++;
          if (i1 < 0)
            i1 = 0;
          if (i2 >= nxbmap)
            i2 = nxbmap - 1;
          for (int ixb = i1; ixb <= i2; ixb++) {
            x = rbmap[0] + rbmap[2] * (ixb - 0.5);
            y = yline[n] + (yline[n + 1] - yline[n]) * (x - xline[n])
                    / (xline[n + 1] - xline[n]);
            int iyb = int((y - rbmap[1]) / rbmap[3]) + 1;
            if (iyb >= nybmap)
              iyb = nybmap - 1;
            for (j = 0; j < iyb; j++) {
              int ibit = j * nxbmap + ixb;
              int iwrd = ibit / nbitwd;
              ibit = ibit % nbitwd;
              bmap[iwrd] = (bmap[iwrd] | (1 << ibit));
            }
          }
        }
      }

    } else {

      ibmap = 0;
      lbmap = 0;
      nxbmap = 0;
      nybmap = 0;
      rbmap[0] = 0.;
      rbmap[1] = 0.;
      rbmap[2] = 0.;
      rbmap[3] = 0.;

    }

    /*************************************************************************/
    if (bottomext && !bottomStencil && npy1>=0) {

      // hide lines below ocean bottom (only labels hidden above)
      glEnable(GL_STENCIL_TEST);
      glClearStencil(0);
      glClear(GL_STENCIL_BUFFER_BIT);
      glStencilFunc(GL_ALWAYS, 1, 1);
      glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

      // not draw this in color buffer (only marked in stencil buffer)
      //???? BUT MAYBE FASTER IF WE DRAW IN COLORBUFFER TOO ... => first in plotBackground...
      glColorMask(GL_FALSE,GL_FALSE, GL_FALSE,GL_FALSE);
      glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
      glDisable(GL_BLEND);
      glShadeModel(GL_FLAT);
      glColor3ubv(backColour.RGB());
      //glColor4f(1.0f,0.0f,0.0f,0.5f);
      float yupper = 0.0f;
      float ylower = yDatamin;
      bool prevundef = false;
      glBegin(GL_QUAD_STRIP);
      if (cdata1d[npy1][ipd1] == fieldUndef) {
        glVertex2f(cdata1d[nxs][ipd1], yupper);
        glVertex2f(cdata1d[nxs][ipd1], ylower);
        prevundef = true;
      }
      for (i = ipd1; i <= ipd2; i++) {
        if (cdata1d[npy1][i] != fieldUndef) {
          if (prevundef) {
            x = (cdata1d[nxs][i - 1] + cdata1d[nxs][i]) * 0.5;
            glVertex2f(x, yupper);
            glVertex2f(x, ylower);
            prevundef = false;
          }
          glVertex2f(cdata1d[nxs][i], cdata1d[npy1][i]);
          glVertex2f(cdata1d[nxs][i], ylower);
        } else if (!prevundef) {
          x = (cdata1d[nxs][i - 1] + cdata1d[nxs][i]) * 0.5;
          glVertex2f(x, yupper);
          glVertex2f(x, ylower);
          prevundef = true;
        }
      }
      if (cdata1d[npy1][ipd2] == fieldUndef) {
        glVertex2f(cdata1d[nxs][ipd2], yupper);
        glVertex2f(cdata1d[nxs][ipd2], ylower);
      }
      glEnd();
      // finished making stencil
      glColorMask(GL_TRUE,GL_TRUE, GL_TRUE,GL_TRUE);

      bottomStencil = true;

      glStencilFunc(GL_NOTEQUAL, 1, 1);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    } else if (bottomext) {

      glEnable(GL_STENCIL_TEST);

    }

    if (bottomext && hardcopy && npy1>=0) {

      n = (ipd2 - ipd1 + 1) * 2 + 5;
      float *xhcstencil = new float[n];
      float *yhcstencil = new float[n];
      n = 0;
      float yupper = 0.0f;
      bool prevundef = false;

      xhcstencil[n] = cdata1d[nxs][ipd1];
      yhcstencil[n++] = yupper;
      xhcstencil[n] = cdata1d[nxs][ipd2];
      yhcstencil[n++] = yupper;

      if (cdata1d[npy1][ipd2] == fieldUndef) {
        xhcstencil[n] = cdata1d[nxs][ipd2];
        yhcstencil[n++] = yupper;
        prevundef = true;
      }
      for (i = ipd2; i >= ipd1; i--) {
        if (cdata1d[npy1][i] != fieldUndef) {
          if (prevundef) {
            x = (cdata1d[nxs][i] + cdata1d[nxs][i + 1]) * 0.5;
            xhcstencil[n] = x;
            yhcstencil[n++] = yupper;
            prevundef = false;
          }
          xhcstencil[n] = cdata1d[nxs][i];
          yhcstencil[n++] = cdata1d[npy1][i];
        } else if (!prevundef) {
          x = (cdata1d[nxs][i] + cdata1d[nxs][i + 1]) * 0.5;
          xhcstencil[n] = x;
          yhcstencil[n++] = yupper;
          prevundef = true;
        }
      }
      if (cdata1d[npy1][ipd1] == fieldUndef) {
        xhcstencil[n] = cdata1d[nxs][ipd1];
        yhcstencil[n++] = yupper;
      }

      addHCStencil(n, xhcstencil, yhcstencil);

      delete[] xhcstencil;
      delete[] yhcstencil;

      stencil = true;
    }
    /*************************************************************************/

    int iconv = 2;
    float conv[6];

    Area dummyArea;

    const int mmm = 2;
    const int mmmUsed = 100;

    int ismooth, ibcol;
    int idraw, nlines, nlim;
    int ncol, icol[mmmUsed], ntyp, ityp[mmmUsed], nwid, iwid[mmmUsed];
    float zrange[2], zstep, zoff, rlines[mmmUsed], rlim[mmm];
    int idraw2 = 0, nlines2, nlim2;
    int ncol2, icol2[mmm], ntyp2, ityp2[mmm], nwid2, iwid2[mmm];
    float zrange2[2], zstep2 = 0.0, zoff2 = 0.0, rlines2[mmm], rlim2[mmm];

    bool res = true;

    ibcol = -1;
    zstep = poptions.lineinterval;
    zoff = poptions.base;

    if (poptions.linevalues.size() > 0) {
      nlines = poptions.linevalues.size();
      for (int ii = 0; ii < nlines; ii++) {
        rlines[ii] = poptions.linevalues[ii];
      }
      idraw = 3;
    } else if (poptions.loglinevalues.size() > 0) {
      nlines = poptions.loglinevalues.size();
      for (int ii = 0; ii < nlines; ii++) {
        rlines[ii] = poptions.loglinevalues[ii];
      }
      idraw = 4;
    } else {
      nlines = 0;
      idraw = 1;
      if (poptions.zeroLine == 0) {
        idraw = 2;
        zoff = 0.;
      }
    }

    zrange[0] = +1.;
    zrange[1] = -1.;
    zrange2[0] = +1.;
    zrange2[1] = -1.;

    if ( poptions.minvalue > -fieldUndef ||
        poptions.maxvalue < fieldUndef ) {
      zrange[0] = poptions.minvalue;
      zrange[1] = poptions.maxvalue;
    }

    ncol = 1;
    icol[0] = -1; // -1: set colour below
    // otherwise index in poptions.colours[]
    ntyp = 1;
    ityp[0] = -1;
    nwid = 1;
    iwid[0] = -1;
    nlim = 0;
    rlim[0] = 0.;

    nlines2 = 0;
    ncol2 = 1;
    icol2[0] = -1;
    ntyp2 = 1;
    ityp2[0] = -1;
    nwid2 = 1;
    iwid2[0] = -1;
    nlim2 = 0;
    rlim2[0] = 0.;

    ismooth = poptions.lineSmooth;
    if (ismooth < 0)
      ismooth = 0;

    // enable clipping at data area (excluding space for text etc.)
    if (iwpd1 > 0 || iwpd2 < nPoint - 1 || iwlev1 > 0 || iwlev2 < numLev - 1) {
      GLint viewport[4];
      glGetIntegerv(GL_VIEWPORT, viewport);
      float sx = float(plotw) / (xFullWindowmax - xFullWindowmin);
      float sy = float(ploth) / (yFullWindowmax - yFullWindowmin);
      GLint px = viewport[0] + int((xPlotmin - xFullWindowmin) * sx);
      GLint py = viewport[1] + int((yPlotmin - yFullWindowmin) * sy);
      GLsizei pw = int((xPlotmax - xPlotmin) * sx) + 1;
      GLsizei ph = int((yPlotmax - yPlotmin) * sy) + 1;
      glEnable(GL_SCISSOR_TEST);
      glScissor(px, py, pw, ph);
      // add a hardcopy scissoring
      if (hardcopy) {
        addHCScissor(px, py, pw, ph);
        scissor = true;
      }
    }

    if (poptions.contourShading == 0 && !poptions.options_1)
      idraw = 0;

    //Plot colour shading
    if (poptions.contourShading != 0) {

      int idraw2 = 0;

#ifdef DEBUGCONTOUR
      /*
       bool contour(int nx, int ny, float z[], float xz[], float yz[],
       int ipart[], int icxy, float cxy[], float xylim[],
       int idraw, float zrange[], float zstep, float zoff,
       int nlines, float rlines[],
       int ncol, int icol[], int ntyp, int ityp[],
       int nwid, int iwid[], int nlim, float rlim[],
       int idraw2, float zrange2[], float zstep2, float zoff2,
       int nlines2, float rlines2[],
       int ncol2, int icol2[], int ntyp2, int ityp2[],
       int nwid2, int iwid2[], int nlim2, float rlim2[],
       int ismooth, int labfmt[], float chxlab, float chylab,
       int ibcol,
       int ibmap, int lbmap, int kbmap[],
       int nxbmap, int nybmap, float rbmap[],
       FontManager* fp, const PlotOptions& poptions, GLPfile* psoutput,
       const Area& fieldArea, const float& fieldUndef)
       */
      METLIBS_LOG_DEBUG("contour: " << nPoint << "," << numLev << "," << "cdata2d[" << no1 << "]," << "cdata2d[" << nx << "]," << "cdata2d[" << ny << "],");
      METLIBS_LOG_DEBUG("part[" << part[0] << "," << part[1] << "," << part[2] << "," << part[3] << "]," << iconv << ",conv[" << conv[0] << "," << conv[1] << "," << conv[2] << "," << conv[3] << "," << conv[4]<< "," << conv[5] << "],xylim[" << xylim[0] << "," << xylim[1] << "," << xylim[2] << "," << xylim[3] << "],");
      METLIBS_LOG_DEBUG(idraw << ",zrange[" << zrange[0] << "," << zrange[1] << "]," << zstep << "," << zoff << ",");
      METLIBS_LOG_DEBUG(nlines << ",rlines[100](" << rlines[0] <<"),");
      METLIBS_LOG_DEBUG(ncol << ",icol[100](" << icol[0] <<")," << ntyp << ",ityp[100](" << ityp[0] <<"),");
      METLIBS_LOG_DEBUG(nwid << ",iwid[100](" << iwid[0] <<")," << nlim << ",rlim[" << rlim[0] << "," << rlim[1] <<"],");
      METLIBS_LOG_DEBUG(idraw2 << ",zrange2[" << zrange2[0] << "," << zrange2[1] << "]," << zstep2 <<"," << zoff2 << ",");
      METLIBS_LOG_DEBUG(nlines2 << ",rlines2[" << rlines2[0] << "," << rlines2[1] << "],");
      METLIBS_LOG_DEBUG(ncol2 << ",icol2[" << icol2[0] <<"," << icol2[1] << "],"<< ntyp2 << ",ityp2[" << ityp2[0]<<"," << ityp2[1] << "],");
      METLIBS_LOG_DEBUG(nwid2 << ",iwid2[" << iwid2[0]<<","<<iwid2[1] <<"]," << nlim2 << ",rlim2[" << rlim2[0] << "," << rlim2[1] <<"],");
      METLIBS_LOG_DEBUG(ismooth << ",labfmt["<< labfmt[0] <<"," <<labfmt[1] << ","<< labfmt[2]<<"]," << chxlab << "," << chylab << ",");
      METLIBS_LOG_DEBUG(ibcol << ",");
      if (bmap != NULL)
        METLIBS_LOG_DEBUG(ibmap << "," << lbmap << ",bmap[lbmap](" << bmap[0] << ")");
      else
        METLIBS_LOG_DEBUG(ibmap << "," << lbmap << ",bmap[lbmap](NULL),");
      METLIBS_LOG_DEBUG(nxbmap << "," << nybmap << ",rbmap[" << rbmap[0] << "," << rbmap[1] << "," << rbmap[2] << "," << rbmap[3] << "],");
      METLIBS_LOG_DEBUG("fp, poptions, psoutput,");
      METLIBS_LOG_DEBUG("dummyArea," << fieldUndef);
#endif

      res = contour(nPoint, numLev, cdata2d[no1], cdata2d[nx], cdata2d[ny],
          part, iconv, conv, xylim, idraw, zrange, zstep, zoff, nlines, rlines,
          ncol, icol, ntyp, ityp, nwid, iwid, nlim, rlim, idraw2, zrange2,
          zstep2, zoff2, nlines2, rlines2, ncol2, icol2, ntyp2, ityp2, nwid2,
          iwid2, nlim2, rlim2, ismooth, labfmt, chxlab, chylab, ibcol, ibmap,
          lbmap, bmap, nxbmap, nybmap, rbmap, fp, poptions, psoutput,
          dummyArea, fieldUndef);
#ifdef DEBUGCONTOUR
      METLIBS_LOG_DEBUG("contour: poptions.contourShading!=0 res: " << res);
#endif

    }
    //Plot contour lines
    if (idraw > 0 || idraw2 > 0) {

      zstep2 = poptions.lineinterval_2;
      zoff2 = poptions.base_2;

      if (!poptions.options_2)
        idraw2 = 0;
      else
        idraw2 = 1;

      if (poptions.colours.size() > 1) {
        if (idraw > 0 && idraw2 > 0) {
          icol[0] = 0;
          icol2[0] = 1;
        } else {
          ncol = poptions.colours.size();
          for (int i = 0; i < ncol; ++i)
            icol[i] = i;
        }
      } else if (idraw > 0) {
        glColor3ubv(poptions.linecolour.RGB());
      } else {
        glColor3ubv(poptions.linecolour_2.RGB());
      }

      if ( poptions.minvalue_2 > -fieldUndef ||
          poptions.maxvalue_2 < fieldUndef ) {
        zrange2[0] = poptions.minvalue_2;
        zrange2[1] = poptions.maxvalue_2;
      }

      if (poptions.linewidths.size() == 1) {
        glLineWidth(poptions.linewidth);
      } else {
        if (idraw2 > 0) { // two set of plot options
          iwid[0] = 0;
          iwid2[0] = 1;
        } else { // one set of plot options, different lines
          nwid = poptions.linewidths.size();
          for (int i = 0; i < nwid; ++i)
            iwid[i] = i;
        }
      }

      if (poptions.linetypes.size() == 1 && poptions.linetype.stipple) {
        glLineStipple(poptions.linetype.factor, poptions.linetype.bmap);
        glEnable(GL_LINE_STIPPLE);
      } else {
        if (idraw2 > 0) { // two set of plot options
          ityp[0] = 0;
          ityp2[0] = 1;
        } else { // one set of plot options, different lines
          ntyp = poptions.linetypes.size();
          for (int i = 0; i < ntyp; ++i)
            ityp[i] = i;
        }
      }

      if (!poptions.options_1)
        idraw = 0;

      //turn off contour shading
      bool contourShading = poptions.contourShading;
      poptions.contourShading = 0;

#ifdef DEBUGCONTOUR
      /*
       bool contour(int nx, int ny, float z[], float xz[], float yz[],
       int ipart[], int icxy, float cxy[], float xylim[],
       int idraw, float zrange[], float zstep, float zoff,
       int nlines, float rlines[],
       int ncol, int icol[], int ntyp, int ityp[],
       int nwid, int iwid[], int nlim, float rlim[],
       int idraw2, float zrange2[], float zstep2, float zoff2,
       int nlines2, float rlines2[],
       int ncol2, int icol2[], int ntyp2, int ityp2[],
       int nwid2, int iwid2[], int nlim2, float rlim2[],
       int ismooth, int labfmt[], float chxlab, float chylab,
       int ibcol,
       int ibmap, int lbmap, int kbmap[],
       int nxbmap, int nybmap, float rbmap[],
       FontManager* fp, const PlotOptions& poptions, GLPfile* psoutput,
       const Area& fieldArea, const float& fieldUndef)
       */
      METLIBS_LOG_DEBUG("contour: " << nPoint << "," << numLev << "," << "cdata2d[" << no1 << "]," << "cdata2d[" << nx << "]," << "cdata2d[" << ny << "],");
      METLIBS_LOG_DEBUG("part[" << part[0] << "," << part[1] << "," << part[2] << "," << part[3] << "]," << iconv << ",conv[" << conv[0] << "," << conv[1] << "," << conv[2] << "," << conv[3] << "," << conv[4]<< "," << conv[5] << "],xylim[" << xylim[0] << "," << xylim[1] << "," << xylim[2] << "," << xylim[3] << "],");
      METLIBS_LOG_DEBUG(idraw << ",zrange[" << zrange[0] << "," << zrange[1] << "]," << zstep << "," << zoff << ",");
      METLIBS_LOG_DEBUG(nlines << ",rlines[100](" << rlines[0] <<"),");
      METLIBS_LOG_DEBUG(ncol << ",icol[100](" << icol[0] <<")," << ntyp << ",ityp[100](" << ityp[0] <<"),");
      METLIBS_LOG_DEBUG(nwid << ",iwid[100](" << iwid[0] <<")," << nlim << ",rlim[" << rlim[0] << "," << rlim[1] <<"],");
      METLIBS_LOG_DEBUG(idraw2 << ",zrange2[" << zrange2[0] << "," << zrange2[1] << "]," << zstep2 <<"," << zoff2 << ",");
      METLIBS_LOG_DEBUG(nlines2 << ",rlines2[" << rlines2[0] << "," << rlines2[1] << "],");
      METLIBS_LOG_DEBUG(ncol2 << ",icol2[" << icol2[0] <<"," << icol2[1] << "],"<< ntyp2 << ",ityp2[" << ityp2[0]<<"," << ityp2[1] << "],");
      METLIBS_LOG_DEBUG(nwid2 << ",iwid2[" << iwid2[0]<<","<<iwid2[1] <<"]," << nlim2 << ",rlim2[" << rlim2[0] << "," << rlim2[1] <<"],");
      METLIBS_LOG_DEBUG(ismooth << ",labfmt["<< labfmt[0] <<"," <<labfmt[1] << ","<< labfmt[2]<<"]," << chxlab << "," << chylab << ",");
      METLIBS_LOG_DEBUG(ibcol << ",");
      if (bmap != NULL)
        METLIBS_LOG_DEBUG(ibmap << "," << lbmap << ",bmap[lbmap](" << bmap[0] << ")");
      else
        METLIBS_LOG_DEBUG(ibmap << "," << lbmap << ",bmap[lbmap](NULL),");
      METLIBS_LOG_DEBUG(nxbmap << "," << nybmap << ",rbmap[" << rbmap[0] << "," << rbmap[1] << "," << rbmap[2] << "," << rbmap[3] << "],");
      METLIBS_LOG_DEBUG("fp, poptions, psoutput,");
      METLIBS_LOG_DEBUG("dummyArea," << fieldUndef);
#endif

      res = contour(nPoint, numLev, cdata2d[no1], cdata2d[nx], cdata2d[ny],
          part, iconv, conv, xylim, idraw, zrange, zstep, zoff, nlines, rlines,
          ncol, icol, ntyp, ityp, nwid, iwid, nlim, rlim, idraw2, zrange2,
          zstep2, zoff2, nlines2, rlines2, ncol2, icol2, ntyp2, ityp2, nwid2,
          iwid2, nlim2, rlim2, ismooth, labfmt, chxlab, chylab, ibcol, ibmap,
          lbmap, bmap, nxbmap, nybmap, rbmap, fp, poptions, psoutput,
          dummyArea, fieldUndef);
#ifdef DEBUGCONTOUR
      METLIBS_LOG_DEBUG("contour: poptions.contourShading = 0); res: " << res;
#endif

      //reset contour shading
      poptions.contourShading = contourShading;
    }

    if (!res)
      METLIBS_LOG_ERROR("VcrossPlot::plotData  Contour error");

    UpdateOutput();

    glDisable(GL_LINE_STIPPLE);

    // disable clipping
    glDisable(GL_SCISSOR_TEST);
    if (hardcopy && scissor)
      removeHCClipping();

    if (bottomext) {
      glDisable(GL_STENCIL_TEST);
      if (hardcopy && stencil)
        removeHCClipping();
    }

    if (bmap)
      delete[] bmap;

  } else if (vcf->second.plotType == vcpt_wind) {

    plotWind(cdata2d[no1], cdata2d[no2], cdata2d[nx], cdata2d[ny], partwind,
        ylim, rwindAuto, hstepAuto, windlevel, poptions);

  } else if (vcf->second.plotType == vcpt_vt_omega && (vcoordPlot == vcv_exner || vcoordPlot == vcv_pressure)) {

    float hours = poptions.vectorunit;

    vcMovement(cdata2d[no1], cdata2d[no2], cdata2d[npp], cdata2d[nx],
        cdata2d[ny], partwind, ylim, hours, hstepAuto, windlevel, poptions);

  } else if (vcf->second.plotType == vcpt_vt_w ) {

    float hours = poptions.vectorunit;

    vcMovement(cdata2d[no1], cdata2d[no2], NULL,
        cdata2d[nx], cdata2d[ny], partwind, ylim, hours, hstepAuto, windlevel,
        poptions);

  } else if (vcf->second.plotType == vcpt_vector) {

    plotVector(cdata2d[no1], cdata2d[no2], cdata2d[nx], cdata2d[ny], partwind,
        ylim, rwindAuto, hstepAuto, windlevel, poptions);

  } else {
    METLIBS_LOG_WARN("cannot plot" << LOGVAL(vcf->second.plotType));
    ok = false;
  }

  UpdateOutput();

  if (ok) {
    n = vcText.size();
    VcrossText vct;
    vct.timeGraph = timeGraph;
    vct.colour = poptions.linecolour;
    vct.modelName = modelName;
    vct.crossectionName = crossectionName;
    vct.fieldName = fieldname;
    if (!timeGraph) {
      vct.forecastHour = forecastHour;
      vct.validTime = validTime;
    }

    if(poptions.extremeType=="Value" && no2 == -1) { //no2>-1 -> vectordata

      //find min/max
      float minValue = fieldUndef;
      float maxValue = -fieldUndef;
      float maxPressure = fieldUndef;
      float minPressure = -fieldUndef;
      if ( poptions.extremeLimits.size() > 0 ) {
        maxPressure = poptions.extremeLimits[0];
        if ( poptions.extremeLimits.size() > 1 ) {
          minPressure = poptions.extremeLimits[1];
        }
      }
      int ii_min=-1, ii_max=-1;
      for( j= partwind[2]; j<partwind[3]-1; ++j ) {
        for( k= partwind[0]; k<partwind[1]-1; ++k ) {
          int i = j*nPoint + k;
          if(cdata2d[no1][i] > maxValue && cdata2d[npp][i] > minPressure && cdata2d[npp][i] < maxPressure ) {
            ii_max = i;
            maxValue = cdata2d[no1][i];
          }
          if(cdata2d[no1][i] < minValue && cdata2d[npp][i] > minPressure && cdata2d[npp][i] < maxPressure ) {
            ii_min=i;
            minValue = cdata2d[no1][i];
          }
        }
      }
      ostringstream ost;
      ost << setprecision(3);
      if ( ii_min > 1 ) {
        ost <<"Min: "<<cdata2d[no1][ii_min]<<"("<<cdata1d[nlat][ii_min%nPoint];
        if( cdata1d[nlat][ii_min%nPoint] > 0 ) ost <<"N, ";
        else ost <<"S, ";
        ost <<cdata1d[nlon][ii_min%nPoint];
        if( cdata1d[nlon][ii_min%nPoint] > 0 ) ost<<"E, ";
        else ost <<"W, ";
        ost <<cdata2d[npp][ii_min]<<"hPa"<<")";
      }
      if ( ii_max > 1 ) {
        ost <<" Max: "<<cdata2d[no1][ii_max]<<"("<<cdata1d[nlat][ii_max%nPoint];
        if( cdata1d[nlat][ii_max%nPoint] > 0 ) ost <<"N, ";
        else ost <<"S, ";
        ost <<cdata1d[nlon][ii_max%nPoint];
        if( cdata1d[nlon][ii_max%nPoint] > 0 ) ost<<"E, ";
        else ost <<"W, ";
        ost <<cdata2d[npp][ii_max]<<"hPa"<<")";
      }
      vct.extremeValueString = ost.str();

      float scale = (xylim[1] - xylim[0])/100 * poptions.extremeSize;
      plotMin(cdata2d[nx][ii_min],cdata2d[ny][ii_min],scale);
      plotMax(cdata2d[nx][ii_max],cdata2d[ny][ii_max],scale);
    }
    vcText.push_back(vct);
  }
  // reset in case contrast used
  poptions.linecolour = linecolour;

  delete[] ypfixed;
  delete[] windlevel;

  return true;
}

void VcrossPlot::plotMin(float x, float y, float scale)
{
  glLineWidth(3);

  glBegin(GL_LINES);
  glVertex2f(x, y);
  glVertex2f(x+scale, y+scale);
  glVertex2f(x, y);
  glVertex2f(x-scale, y+scale);
  glEnd();
}


void VcrossPlot::plotMax(float x, float y, float scale)
{
  glLineWidth(3);

  glBegin(GL_LINES);
  glVertex2f(x-scale, y-scale);
  glVertex2f(x, y);
  glVertex2f(x+scale, y-scale);
  glVertex2f(x, y);
  glEnd();
}


bool VcrossPlot::plotWind(float *u, float *v, float *x, float *y, int *part,
    float *ylim, float size, int hstepAuto, bool *uselevel,
    PlotOptions& poptions)
{
  METLIBS_LOG_SCOPE();

  const int ix1 = part[0];
  const int ix2 = part[1] + 1;
  const int iy1 = part[2];
  const int iy2 = part[3] + 1;

  const float ymin = ylim[0];
  const float ymax = ylim[1];

  int xstep = poptions.density;
  const int step = 1;

  if (xstep < 1) {
    xstep = hstepAuto;
  } else if (xstep == 1) {
    for (int k = 0; k < numLev; k++)
      uselevel[k] = true;
  }
  const bool colourwind = false;
  const float* colourdata = 0;

  float* limits=0;
  const GLfloat* rgb=0;
  int nlim = poptions.limits.size();
  int ncol = poptions.colours.size();

  const int n_rgb_default = 4;
  const GLfloat rgb_default[n_rgb_default * 3] = {0.5,0.5,0.5,  0,0,0,  0,1,1,  1,0,0};

  if (colourwind) {
    if (ncol>=2) {
      GLfloat* rgb_tmp = new GLfloat[ncol*3];
      for (int i=0; i<ncol; i++) {
        rgb_tmp[i*3+0] = poptions.colours[i].fR();
        rgb_tmp[i*3+1] = poptions.colours[i].fG();
        rgb_tmp[i*3+2] = poptions.colours[i].fB();
      }
      rgb = rgb_tmp;
    } else {
      ncol = n_rgb_default;
      rgb = rgb_default;
    }

    if (nlim>=1 ) {

      if (nlim>ncol-1) nlim= ncol-1;
      if (ncol>nlim+1) ncol= nlim+1;
      limits= new float[nlim];
      for (int i=0; i<nlim; i++) {
        limits[i]= poptions.limits[i];
      }

    } else {

      // default, should be handled when reading setup, if allowed...
      const int maxdef= 4;
      nlim= maxdef-1;

      float fmin=fieldUndef, fmax=-fieldUndef;
      for (int i=0; i<nx*ny; ++i) {
        const float cv = colourdata[i];
        if (cv != fieldUndef) {
          if (fmin > cv) fmin = cv;
          if (fmax < cv) fmax = cv;
        }
      }
      if (fmin>fmax)
        return false;

      limits= new float[nlim];
      float dlim= (fmax-fmin)/float(ncol);

      for (int i=0; i<nlim; i++) {
        limits[i]= fmin + dlim*float(i+1);
      }
    }
  }

  const int nx = nPoint;
  const float unitlength = 1;
  const float flagl = size * 0.85 / unitlength;
  const float flagstep = flagl / 10.;
  const float flagw = flagl * 0.35;
  const float hflagw = 0.6;
  // for arrow tip
  const float afac = -1.5, sfac = afac * 0.5;

  vector<float> vx, vy; // keep vertices for 50-knot flags
  vector<int>   vc;    // keep the colour too

  glLineWidth(poptions.linewidth + 0.1); // +0.1 to avoid MesaGL coredump
  glColor3ubv(poptions.linecolour.RGB());

  glBegin(GL_LINES);

  for (int iy = iy1; iy < iy2; iy += step) {
    if (uselevel[iy]) {
      for (int ix = ix1; ix < ix2; ix += xstep) {
        const int i = iy * nx + ix;
        const int sign = 1;
        float gx = x[i];
        float gy = y[i];
        if (u[i] != fieldUndef && v[i] != fieldUndef && gy >= ymin && gy <= ymax) {
          float ff = sqrtf(u[i] * u[i] + v[i] * v[i]);
          if (ff>0.00001 && (!colourwind || colourdata[i]!=fieldUndef) ){

            const float gu = u[i] / ff;
            const float gv = v[i] / ff;

            ff *= 3600.0 / 1852.0;

            // find no. of 50,10 and 5 knot flags
            int n50, n10, n05;
            if (ff < 182.49) {
              n05 = int(ff * 0.2 + 0.5);
              n50 = n05 / 10;
              n05 -= n50 * 10;
              n10 = n05 / 2;
              n05 -= n10 * 2;
            } else if (ff < 190.) {
              n50 = 3;
              n10 = 3;
              n05 = 0;
            } else if (ff < 205.) {
              n50 = 4;
              n10 = 0;
              n05 = 0;
            } else if (ff < 225.) {
              n50 = 4;
              n10 = 1;
              n05 = 0;
            } else {
              n50 = 5;
              n10 = 0;
              n05 = 0;
            }

            int l = 0;
            if (colourwind) {
              while (l<nlim && colourdata[i]>limits[l])
                l++;
              glColor3fv(&rgb[l*3]);
            }

            const float dx = flagstep * gu;
            const float dy = flagstep * gv;
            const float dxf = -sign*flagw * gv - dx;
            const float dyf = sign*flagw * gu - dy;

            if (poptions.arrowstyle == arrow_wind_arrow) {
              // arrow (drawn as two lines)
              vx.push_back(gx);
              vy.push_back(gy);
              vx.push_back(gx + afac*dx + sfac*dy);
              vy.push_back(gy + afac*dy - sfac*dx);
              vx.push_back(gx + afac*dx - sfac*dy);
              vy.push_back(gy + afac*dy + sfac*dx);
            }

            // direction
            glVertex2f(gx, gy);
            gx = gx - flagl * gu;
            gy = gy - flagl * gv;
            glVertex2f(gx, gy);

            // 50-knot flags, store for plot below
            if (n50 > 0) {
              for (int n = 0; n < n50; n++) {
                if (colourwind)
                  vc.push_back(l);
                vx.push_back(gx);
                vy.push_back(gy);
                gx += dx * 2.;
                gy += dy * 2.;
                vx.push_back(gx + dxf);
                vy.push_back(gy + dyf);
                vx.push_back(gx);
                vy.push_back(gy);
              }
              gx += dx;
              gy += dy;
            }
            // 10-knot flags
            for (int n = 0; n < n10; n++) {
              glVertex2f(gx, gy);
              glVertex2f(gx + dxf, gy + dyf);
              gx += dx;
              gy += dy;
            }
            // 5-knot flag
            if (n05 > 0) {
              if (n50 + n10 == 0) {
                gx += dx;
                gy += dy;
              }
              glVertex2f(gx, gy);
              glVertex2f(gx + hflagw * dxf, gy + hflagw * dyf);
            }
          }
        }
      }
    }
  }
  glEnd();

  UpdateOutput();

  // draw 50-knot flags
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  const int vi = vx.size();
  if (vi >= 3) {
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < vi; i += 3) {
      if (colourwind)
        glColor3fv(&rgb[vc[i]]);
      
      glVertex2f(vx[i], vy[i]);
      glVertex2f(vx[i + 1], vy[i + 1]);
      glVertex2f(vx[i + 2], vy[i + 2]);
    }
    glEnd();
    UpdateOutput();
  }
  
  if (rgb != rgb_default)
    delete[] rgb;
  delete[] limits;

  //glDisable(GL_LINE_STIPPLE);

  return true;
}

bool VcrossPlot::vector2fixedLevel(float *u, float *v, float *x, float *y,
    int ix1, int ix2, float *ylim, int hstep, const vector<float> yfixed,
    int &kf1, int &kf2, float *uint, float *vint, float *xint, float *yint)
{
  METLIBS_LOG_SCOPE();

  int nlev = yfixed.size();
  kf1 = kf2 = -1;
  for (int kf = 0; kf < nlev; kf++) {
    if (yfixed[kf] >= ylim[0] && yfixed[kf] <= ylim[1]) {
      if (kf1 < 0)
        kf1 = kf;
      kf2 = kf;
    }
  }

  for (int ix = ix1; ix <= ix2; ix += hstep) {
    int k = 0;

    for (int kf = kf1; kf <= kf2; kf++) {
      int nf = kf * nPoint + ix;

      while (k < numLev && y[k * nPoint + ix] <= yfixed[kf])
        k++;

      int n1 = (k - 1) * nPoint + ix;
      int n2 = k * nPoint + ix;
      xint[nf] = x[n1];
      yint[nf] = yfixed[kf];

      if (k > 0 && k < numLev) {
        float c1 = (y[n2] - yfixed[kf]) / (y[n2] - y[n1]);
        float c2 = (yfixed[kf] - y[n1]) / (y[n2] - y[n1]);
        // interpolation of direction and speed
        float u1 = u[n1];
        float v1 = v[n1];
        float u2 = u[n2];
        float v2 = v[n2];
        float ff = c1 * sqrtf(u1 * u1 + v1 * v1) + c2
            * sqrtf(u2 * u2 + v2 * v2);
        float upi = c1 * u1 + c2 * u2;
        float vpi = c1 * v1 + c2 * v2;
        float ffi = sqrtf(upi * upi + vpi * vpi);
        uint[nf] = upi * (ff / ffi);
        vint[nf] = vpi * (ff / ffi);
      } else if (vcopt->extrapolateFixedLevels) {
        if (k == 0) {
          uint[nf] = u[n2];
          vint[nf] = v[n2];
        } else {
          uint[nf] = u[n1];
          vint[nf] = v[n1];
        }
      } else {
        uint[nf] = fieldUndef;
        vint[nf] = fieldUndef;
      }
    }
  }

  return true;
}

bool VcrossPlot::vcMovement(float *vt, float *wom, float *p, float *x,
    float *y, int *part, float *ylim, float hours, int hstepAuto,
    bool *uselevel, PlotOptions& poptions)
{
  //
  //   plot of horizontal/vertical movement.
  //
  //   vertical is exner or pressure : the 'wom' field is omega
  //   vertical is height            : the 'wom' field is w
  //
  //   vt: wind tangential to crossection (unit m/s)
  //   om: omega, vertical 'velocity'     (unit hPa/s)
  //    x: x coordinate on plot
  //    y: y coordinate on plot
  //    p: pressure
  //   hours: time of movement shown


  int ix1 = part[0];
  int ix2 = part[1] + 1;
  int iy1 = part[2];
  int iy2 = part[3] + 1;

  float ymin = ylim[0];
  float ymax = ylim[1];

  int hstep = poptions.density;

  if (hstep < 1) {
    hstep = hstepAuto;
  } else if (hstep == 1) {
    for (int k = 0; k < numLev; k++)
      uselevel[k] = true;
  }

  float dt = 3600. * hours;

  float x0, y0, p2, pi2, yend, dx, dy;

  glDisable(GL_BLEND);

  glColor3ubv(poptions.linecolour.RGB());
  glLineWidth(1);

  dy = 0.;

  for (int iy = iy1; iy < iy2; iy++) {
    if (uselevel[iy]) {
      for (int ix = ix1; ix < ix2; ix += hstep) {
        int n = iy * nPoint + ix;

        x0 = x[n];
        y0 = y[n];

        if (y0 >= ymin && y0 <= ymax && vt[n] != fieldUndef && wom[n] != fieldUndef && (vt[n] != 0. || wom[n] != 0.)) {
          // vt+w
          if (p==NULL) {
            if (vcoordPlot == vcv_height) {
              dy = v2hRatio * dt * wom[n];
            } else {
              dy = v2hRatio * dt * 0.1 * wom[n];
            }
            dy = v2hRatio * dt * wom[n];
            // vt+omega
          } else if (vcoordPlot == vcv_exner) {
            // exner function as output vertical coordinate
            p2 = p[n] + dt * wom[n];
            pi2 = cp * powf(p2 * p0inv, kappa);
            yend = yconst + yscale * pi2;
            // vertical component
            dy = yend - y0;
          } else if (vcoordPlot == vcv_pressure) {
            // pressure as output vertical coordinate
            p2 = p[n] + dt * wom[n];
            yend = yconst + yscale * p2;
            // vertical component
            dy = yend - y0;
          }

          // horizontal component
          dx = dt * vt[n];

          plotArrow(x0, y0, dx, dy, vcopt->thinArrows);
        }
      }
    }
  }

  UpdateOutput();

  return true;
}

bool VcrossPlot::plotVector(float *u, float *v, float *x, float *y, int *part,
    float *ylim, float size, int hstepAuto, bool *uselevel,
    PlotOptions& poptions)
{
  METLIBS_LOG_SCOPE();

  int ix1 = part[0];
  int ix2 = part[1] + 1;
  int iy1 = part[2];
  int iy2 = part[3] + 1;

  float ymin = ylim[0];
  float ymax = ylim[1];

  int hstep = poptions.density;

  if (hstep < 1) {
    hstep = hstepAuto;
  } else if (hstep == 1) {
    for (int k = 0; k < numLev; k++)
      uselevel[k] = true;
  }

  float unitlength = poptions.vectorunit;

  // length if abs(vector) = unitlength
  float arrowlength = size * hstep;

  float scale = arrowlength / unitlength;

  // for arrow tip
  const float afac = -0.333333;
  const float sfac = afac * 0.5;

  float ff, gx, gy, dx, dy;
  int i;

  glLineWidth(poptions.linewidth + 0.1); // +0.1 to avoid MesaGL coredump
  glColor3ubv(poptions.linecolour.RGB());

  glBegin(GL_LINES);

  for (int iy = iy1; iy < iy2; iy++) {
    if (uselevel[iy]) {
      for (int ix = ix1; ix < ix2; ix += hstep) {
        i = iy * nPoint + ix;
        gx = x[i];
        gy = y[i];
        if (u[i] != fieldUndef && v[i] != fieldUndef && y[i] >= ymin && y[i]
                                                                          <= ymax) {
          ff = sqrtf(u[i] * u[i] + v[i] * v[i]);
          if (ff > 0.00001) {
            dx = scale * u[i];
            dy = scale * v[i];

            // direction
            glVertex2f(gx, gy);
            gx = gx + dx;
            gy = gy + dy;
            glVertex2f(gx, gy);

            // arrow (drawn as two lines)
            glVertex2f(gx, gy);
            glVertex2f(gx + afac * dx + sfac * dy, gy + afac * dy - sfac * dx);
            glVertex2f(gx, gy);
            glVertex2f(gx + afac * dx - sfac * dy, gy + afac * dy + sfac * dx);
          }
        }
      }
    }
  }
  glEnd();

  UpdateOutput();

  return true;
}

//############### bottomExtrapolate ...
void VcrossPlot::replaceUndefinedValues(int nx, int ny, float *f, bool fillAll)
{
  const int nloop = 20;

  float *ftmp = new float[nx * ny];

  for (int ij = 0; ij < nx * ny; ij++) {
    ftmp[ij] = f[ij];
    if (f[ij] == fieldUndef) {
      float sum = 0.;
      int num = 0;
      int i = ij % nx;
      int j = ij / nx;

      float sum2 = 0.;
      int num2 = 0;
      if (i > 0 && f[ij - 1] != fieldUndef) {
        sum += f[ij - 1];
        num++;
        if (i > 1 && f[ij - 2] != fieldUndef) {
          sum2 += f[ij - 1] - f[ij - 2];
          num2++;
        }
      }
      if (i < nx - 1 && f[ij + 1] != fieldUndef) {
        sum += f[ij + 1];
        num++;
        if (i < nx - 2 && f[ij + 2] != fieldUndef) {
          sum2 += f[ij + 1] - f[ij + 2];
          num2++;
        }
      }
      if (j > 0 && f[ij - nx] != fieldUndef) {
        sum += f[ij - nx];
        num++;
        if (j > 1 && f[ij - 2 * nx] != fieldUndef) {
          sum2 += f[ij - nx] - f[ij - 2 * nx];
          num2++;
        }
      }
      if (j < ny - 1 && f[ij + nx] != fieldUndef) {
        sum += f[ij + nx];
        num++;
        if (j < ny - 2 && f[ij + 2 * nx] != fieldUndef) {
          sum2 += f[ij + nx] - f[ij + 2 * nx];
          num2++;
        }
      }
      if (i > 0 && j > 0 && f[ij - nx - 1] != fieldUndef) {
        sum += f[ij - nx - 1];
        num++;
        if (i > 1 && j > 1 && f[ij - 2 * nx - 2] != fieldUndef) {
          //	  sum2+= 0.5*(f[ij-nx-1]-f[ij-2*nx-2]);
          sum2 += f[ij - nx - 1] - f[ij - 2 * nx - 2];
          num2++;
        }
      }
      if (i > 0 && j < ny - 1 && f[ij + nx - 1] != fieldUndef) {
        sum += f[ij + nx - 1];
        num++;
        if (i > 1 && j < ny - 2 && f[ij + 2 * nx - 2] != fieldUndef) {
          //	  sum2+= 0.5*(f[ij+nx-1]-f[ij+2*nx-2]);
          sum2 += f[ij + nx - 1] - f[ij + 2 * nx - 2];
          num2++;
        }
      }
      if (i < nx - 1 && j > 0 && f[ij - nx + 1] != fieldUndef) {
        sum += f[ij - nx + 1];
        num++;
        if (i < nx - 2 && j > 1 && f[ij - 2 * nx + 2] != fieldUndef) {
          //	  sum2+= 0.5*(f[ij-nx+1]-f[ij-2*nx+2]);
          sum2 += f[ij - nx + 1] - f[ij - 2 * nx + 2];
          num2++;
        }
      }
      if (i < nx - 1 && j < ny - 1 && f[ij + nx + 1] != fieldUndef) {
        sum += f[ij + nx + 1];
        num++;
        if (i < nx - 2 && j < ny - 2 && f[ij + 2 * nx + 2] != fieldUndef) {
          //	  sum2+= 0.5*(f[ij+nx+1]-f[ij+2*nx+2]);
          sum2 += f[ij + nx + 1] - f[ij + 2 * nx + 2];
          num2++;
        }
      }
      if (num2 > 1)
        sum += sum2;
      if (num > 0)
        ftmp[ij] = sum / float(num);
    }
  }
  for (int ij = 0; ij < nx * ny; ij++)
    f[ij] = ftmp[ij];
  delete[] ftmp;
  if (!fillAll)
    return;
  //-----------------------------------------------------------

  int nundef = 0;
  float avg = 0.0;
  for (int i = 0; i < nx * ny; i++) {
    if (f[i] != fieldUndef)
      avg += f[i];
    else
      nundef++;
  }
  if (nundef > 0 && nundef < nx * ny) {
    avg /= float(nx * ny - nundef);
    int *indx = new int[nundef];
    int n1a, n1b, n2a, n2b, n3a, n3b, n4a, n4b, n5a, n5b;
    int i, j, ij, n;
    n = 0;
    n1a = n;
    for (j = 1; j < ny - 1; j++)
      for (i = 1; i < nx - 1; i++)
        if (f[j * nx + i] == fieldUndef)
          indx[n++] = j * nx + i;
    n1b = n;
    n2a = n;
    i = 0;
    for (j = 1; j < ny - 1; j++)
      if (f[j * nx + i] == fieldUndef)
        indx[n++] = j * nx + i;
    n2b = n;
    n3a = n;
    i = nx - 1;
    for (j = 1; j < ny - 1; j++)
      if (f[j * nx + i] == fieldUndef)
        indx[n++] = j * nx + i;
    n3b = n;
    n4a = n;
    j = 0;
    for (i = 0; i < nx; i++)
      if (f[j * nx + i] == fieldUndef)
        indx[n++] = j * nx + i;
    n4b = n;
    n5a = n;
    j = ny - 1;
    for (i = 0; i < nx; i++)
      if (f[j * nx + i] == fieldUndef)
        indx[n++] = j * nx + i;
    n5b = n;

    for (i = 0; i < nx * ny; i++)
      if (f[i] == fieldUndef)
        f[i] = avg;

    float cor = 1.6;
    float error;

    // much faster than in the DNMI ecom3d ocean model,
    // where method was found to fill undefined values with
    // rather sensible values...

    for (int loop = 0; loop < nloop; loop++) {
      for (n = n1a; n < n1b; n++) {
        ij = indx[n];
        error = (f[ij - nx] + f[ij - 1] + f[ij + 1] + f[ij + nx]) * 0.25
            - f[ij];
        f[ij] += (error * cor);
      }
      for (n = n2a; n < n2b; n++) {
        ij = indx[n];
        f[ij] = f[ij + 1];
      }
      for (n = n3a; n < n3b; n++) {
        ij = indx[n];
        f[ij] = f[ij - 1];
      }
      for (n = n4a; n < n4b; n++) {
        ij = indx[n];
        f[ij] = f[ij + nx];
      }
      for (n = n5a; n < n5b; n++) {
        ij = indx[n];
        f[ij] = f[ij - nx];
      }
    }

    delete[] indx;
  }
}

void VcrossPlot::xyclip(int npos, float *x, float *y, float xylim[4])
{
  //  plotter del(er) av sammenhengende linje som er innenfor gitt
  //  omraade, ogsaa linjestykker mellom 'nabopunkt' som begge er
  //  utenfor omraadet.
  //  (farge, linje-type og -tykkelse maa vaere satt paa forhaand)
  //
  //  grafikk: OpenGL
  //
  //  input:
  //  ------
  //  x(npos),y(npos): linje med 'npos' punkt (npos>1)
  //  xylim(1-4):      x1,x2,y1,y2 for aktuelt omraade

  int nint, nc, n, i, k1, k2;
  float xa, xb, ya, yb, xint = 0.0, yint = 0.0, x1, x2, y1, y2;
  float xc[4], yc[4];

  if (npos < 2)
    return;

  xa = xylim[0];
  xb = xylim[1];
  ya = xylim[2];
  yb = xylim[3];

  if (x[0] < xa || x[0] > xb || y[0] < ya || y[0] > yb) {
    k2 = 0;
  } else {
    k2 = 1;
    nint = 0;
    xint = x[0];
    yint = y[0];
  }

  for (n = 1; n < npos; ++n) {
    k1 = k2;
    k2 = 1;

    if (x[n] < xa || x[n] > xb || y[n] < ya || y[n] > yb)
      k2 = 0;

    // sjekk om 'n' og 'n-1' er innenfor
    if (k1 + k2 == 2)
      continue;

    // k1+k2=1: punkt 'n' eller 'n-1' er utenfor
    // k1+k2=0: sjekker om 2 nabopunkt som begge er utenfor omraadet
    //          likevel har en del av linja innenfor.

    x1 = x[n - 1];
    y1 = y[n - 1];
    x2 = x[n];
    y2 = y[n];

    // sjekker om 'n-1' og 'n' er utenfor paa samme side
    if (k1 + k2 == 0 && ((x1 < xa && x2 < xa) || (x1 > xb && x2 > xb) || (y1
        < ya && y2 < ya) || (y1 > yb && y2 > yb)))
      continue;

    // sjekker alle skjaerings-muligheter
    nc = -1;
    if (x1 != x2) {
      nc++;
      xc[nc] = xa;
      yc[nc] = y1 + (y2 - y1) * (xa - x1) / (x2 - x1);
      if (yc[nc] < ya || yc[nc] > yb || (xa - x1) * (xa - x2) > 0.)
        nc--;
      nc++;
      xc[nc] = xb;
      yc[nc] = y1 + (y2 - y1) * (xb - x1) / (x2 - x1);
      if (yc[nc] < ya || yc[nc] > yb || (xb - x1) * (xb - x2) > 0.)
        nc--;
    }
    if (y1 != y2) {
      nc++;
      yc[nc] = ya;
      xc[nc] = x1 + (x2 - x1) * (ya - y1) / (y2 - y1);
      if (xc[nc] < xa || xc[nc] > xb || (ya - y1) * (ya - y2) > 0.)
        nc--;
      nc++;
      yc[nc] = yb;
      xc[nc] = x1 + (x2 - x1) * (yb - y1) / (y2 - y1);
      if (xc[nc] < xa || xc[nc] > xb || (yb - y1) * (yb - y2) > 0.)
        nc--;
    }

    if (k2 == 1) {
      // foerste punkt paa linjestykke innenfor
      nint = n - 1;
      xint = xc[0];
      yint = yc[0];
    } else if (k1 == 1) {
      // siste punkt paa linjestykke innenfor
      glBegin(GL_LINE_STRIP);
      glVertex2f(xint, yint);
      for (i = nint + 1; i < n; i++)
        glVertex2f(x[i], y[i]);
      glVertex2f(xc[0], yc[0]);
      glEnd();
    } else if (nc > 0) {
      // to 'nabopunkt' utenfor, men del av linja innenfor
      glBegin(GL_LINE_STRIP);
      glVertex2f(xc[0], yc[0]);
      glVertex2f(xc[1], yc[1]);
      glEnd();
    }
  }

  if (k2 == 1) {
    // siste punkt er innenfor omraadet
    glBegin(GL_LINE_STRIP);
    glVertex2f(xint, yint);
    for (i = nint + 1; i < npos; i++)
      glVertex2f(x[i], y[i]);
    glEnd();
  }
}

//copy from diAnnotationPlot, move to std::string?

vector<std::string> VcrossPlot::split(const std::string eString, const char s1, const char s2)
{
  /*finds entries delimited by s1 and s2
   (f.ex. s1=<,s2=>) <"this is the entry">
   anything inside " " is ignored as delimiters
   f.ex. <"this is also > an entry">
   */
  int stop, start, stop2, start2, len;
  vector<std::string> vec;

  if (eString.empty())
    return vec;

  len = eString.length();
  start = eString.find_first_of(s1, 0) + 1;
  while (start >= 1 && start < len) {
    start2 = eString.find_first_of('"', start);
    stop = eString.find_first_of(s2, start);
    while (start2 > -1 && start2 < stop) {
      // search for second ", which should come before >
      stop2 = eString.find_first_of('"', start2 + 1);
      if (stop2 > -1) {
        start2 = eString.find_first_of('"', stop2 + 1);
        stop = eString.find_first_of(s2, stop2 + 1);
      } else
        start2 = -1;
    }

    //if maching s2 found, add entry
    if (stop > 0 && stop < len)
      vec.push_back(eString.substr(start, stop - start));

    //next s1
    if (stop < len)
      start = eString.find_first_of(s1, stop) + 1;
    else
      start = len;
  }
  return vec;
}

void VcrossPlot::plotArrow(const float& x0, const float& y0, const float& dx,
    const float& dy, bool thinArrows)
{

  // arrow layout
  const float afac = -1. / 3.;
  const float sfac = -afac * 0.57735;
  const float wfac = 1. / 38.;

  float x1 = x0 + dx;
  float y1 = y0 + dy;

  if (!thinArrows) {

    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glShadeModel(GL_FLAT);
    // each arrow splitted into three triangles
    glBegin(GL_TRIANGLES);
    // arrow head
    glVertex2f(x1, y1);
    glVertex2f(x1 + afac * dx - sfac * dy, y1 + afac * dy + sfac * dx);
    glVertex2f(x1 + afac * dx + sfac * dy, y1 + afac * dy - sfac * dx);
    // the rest as two triangles (1)
    glVertex2f(x0 - wfac * dy, y0 + wfac * dx);
    glVertex2f(x1 + afac * dx - wfac * dy, y1 + afac * dy + wfac * dx);
    glVertex2f(x1 + afac * dx + wfac * dy, y1 + afac * dy - wfac * dx);
    // the rest as two triangles (2)
    glVertex2f(x1 + afac * dx + wfac * dy, y1 + afac * dy - wfac * dx);
    glVertex2f(x0 + wfac * dy, y0 - wfac * dx);
    glVertex2f(x0 - wfac * dy, y0 + wfac * dx);
    glEnd();

  } else {

    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x1 + afac * dx - sfac * dy, y1 + afac * dy + sfac * dx);
    glVertex2f(x1, y1);
    glVertex2f(x1 + afac * dx + sfac * dy, y1 + afac * dy - sfac * dx);
    glVertex2f(x1, y1);
    glVertex2f(x0, y0);
    glEnd();

  }
}