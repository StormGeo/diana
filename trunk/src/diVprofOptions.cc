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

#include <diVprofOptions.h>
#include <iostream>

using namespace::miutil;

// default constructor
VprofOptions::VprofOptions()
{
  setDefaults();
}


// destructor
VprofOptions::~VprofOptions()
{
}


void VprofOptions::setDefaults()
{
#ifdef DEBUGPRINT
  cerr << "VprofOptions::setDefaults" << endl;
#endif

  ptttt=    true;   // t
  ptdtd=    true;   // td
  pwind=    true;   // wind
  pvwind=   false;  // vertical wind
  prelhum=  false;  // relative humidty
  pducting= false;  // ducting
  pkindex=  true;   // Kindex
  pslwind=  false;  // numbers at significant wind levels

  dataColour.clear();
  dataColour.push_back("blue");
  dataColour.push_back("red");
  dataColour.push_back("darkGreen");
  dataColour.push_back("black");
  dataColour.push_back("magenta");
  dataColour.push_back("darkGray");
  dataColour.push_back("darkRed");
  dataColour.push_back("darkCyan");
  dataColour.push_back("orange");
  dataColour.push_back("darkMagenta");

  dataLinewidth.clear();
  windLinewidth.clear();
  for (unsigned int i=0; i<dataColour.size(); i++) {
    dataLinewidth.push_back(3.0);
    windLinewidth.push_back(1.0);
  }

  backgroundColour="white";
  diagramtype= 0; // 0=Amble  1=Exner(pi)  2=Pressure  3=ln(P)
  tangle= 45.;    // angle between the vertical and temperatur lines,
  // 0. - 80. degrees, 45 degrees on Amble diagram
  pminDiagram=  100;  // min pressure
  pmaxDiagram= 1050;  // max pressure
  tminDiagram=  -30;  // min temperature (C) at 1000 hPa
  tmaxDiagram=  +30;  // max temperature (C) at 1000 hPa
  trangeDiagram=  0;  // temperature range  0=fixed  1=fixed.max-min  2=minimum

  pplines= true;   // pressure lines
  pplinesfl= false; // false=at pressure levels  true=at flight levels
  pColour= "black";
  pLinetype= "solid";
  pLinewidth1= 1.; // thin lines
  pLinewidth2= 2.; // thick lines

  for (int p=1050; p>0; p-=50)
    plevels.push_back(p);

  // flight levels, unit 100 feet
  // Warning: pressure at flight levels found by looking at an Amble diagram!
  const int mflvl= 45;
  int   iflvl[mflvl]= {   0,  10,  20,  30,  40,  50,  60,  70,  80,  90,
      100, 110, 120, 130, 140, 150, 160, 170, 180, 190,
      200, 210, 220, 230, 240, 250, 260, 270, 280, 290,
      300, 310, 320, 330, 340, 350, 360, 370, 380, 390,
      400, 450, 500, 550, 600 };
  float pflvl[mflvl]= {1013.,978.,942.,907.,874.,843.,812.,782.,751.,722.,
      696.,670.,642.,619.,595.,572.,549.,526.,505.,485.,
      465.,445.,428.,410.,393.,378.,359.,344.,329.,314.,
      300.,288.,274.,261.,250.,239.,227.,216.,206.,197.,
      188.,147.,116., 91., 71. };

  for (int i=0; i<mflvl; i++)
    flightlevels.push_back(iflvl[i]);
  for (int i=0; i<mflvl; i++)
    pflightlevels.push_back(pflvl[i]);

  ptlines= true;   // temperature lines
  tStep= 5;     // temp. step
  tColour= "black";
  tLinetype= "solid";
  tLinewidth1= 1.; // thin lines
  tLinewidth2= 2.; // thick lines

  pdryadiabat= true;    // dry adiabats
  dryadiabatStep= 10; // temperature step (C at 1000hPa)
  dryadiabatColour= "black";
  dryadiabatLinetype= "solid";
  dryadiabatLinewidth= 1.;

  pwetadiabat= true;    // dry adiabats
  wetadiabatStep= 5; // temperature step (C at 1000hPa)
  wetadiabatColour= "darkRed";
  wetadiabatLinetype= "solid";
  wetadiabatLinewidth= 1.;
  wetadiabatPmin= 300;
  wetadiabatTmin= -50;

  pmixingratio= true;    // mixing ratio
  mixingratioSet= 1; // line set no. (0,1,2,3 available)
  mixingratioColour= "magenta";
  mixingratioLinetype= "longdash";
  mixingratioLinewidth= 1.;
  mixingratioPmin= 300;
  mixingratioTmin= -50;

  // mixing ratio line sets  (q in unit g/kg)
  const int nq0=9, nq1=11, nq2=15, nq3=28;
  float q0[nq0]= { .1,.2,.4,1.,5.,10.,20.,30.,50. };
  float q1[nq1]= { .1,.2,.4,1.,2.,5.,10.,20.,30.,40.,50. };
  float q2[nq2]= { .1,.2,.4,.6,1.,2.,3.,5.,7.,10.,14.,20.,30.,40.,50. };
  float q3[nq3]= { .1,.2,.4,.6,.8,1.,1.5,2.,2.5,3.,4.,5.,6.,7.,8.,9.,
      10.,12.,14.,16.,18.,20.,25.,30.,35.,40.,45.,50. };
  qtable.clear();
  vector<float> q;
  q.clear();
  for (int i=0; i<nq0; i++) q.push_back(q0[i]);
  qtable.push_back(q);
  q.clear();
  for (int i=0; i<nq1; i++) q.push_back(q1[i]);
  qtable.push_back(q);
  q.clear();
  for (int i=0; i<nq2; i++) q.push_back(q2[i]);
  qtable.push_back(q);
  q.clear();
  for (int i=0; i<nq3; i++) q.push_back(q3[i]);
  qtable.push_back(q);

  plabelp= true; // p labels (numbers)
  plabelt= true; // t labels (numbers)
  plabelq= true; // mixing ratio labels (numbers)
  pframe= true;  // frame
  ptext= true;   // text (station name etc.)
  frameColour= "black";
  frameLinetype= "solid";
  frameLinewidth= 2.;
  textColour= "black";

  pflevels= true; // flight levels (numbers/marks on axis only)
  flevelsColour= "black";
  flevelsLinetype= "solid";
  flevelsLinewidth1= 1.;
  flevelsLinewidth2= 2.; // thick lines
  plabelflevels= true; // labels (numbers)

  rsvaxis=   1.; // relative size vertical axis
  rstext=    1.; // relative size text
  rslabels=  1.; // relative size labels (p and t numbers)
  rswind=    1.; // relative size wind   (width of column)
  rsvwind=   1.; // relative size vertical wind (width of column)
  rsrelhum=  1.; // relative size relative humidity (width of column)
  rsducting= 1.; // relative size ducting (width of column)
  windseparate= true; // separate wind columns when multiple data
  rvwind= 0.01;    // range of vertical wind in unit hPa/s (-range to +range)
  ductingMin= -200.;  // min ducting in diagram
  ductingMax= +200.;  // max ducting in diagram

  rangeLinetype= "solid";  // for vertical wind, rel.hum. and ducting
  rangeLinewidth= 1.;

  pcotrails= true; // condensation trail lines
  // (linjer for vurdering av mulighet for
  //  kondensstriper fra fly)
  cotrailsColour= "cyan";
  cotrailsLinetype= "solid";
  cotrailsLinewidth= 3.;
  cotrailsPmin= 100;
  cotrailsPmax= 700;

  pgeotext= true; // geographic position in text
  linetext= 0;    // fixed no. of text lines for station and time,
  // 0= not fixed
  temptext= true; // false=use TEMP  true= use tempname() in text

  changed= true;
}


void VprofOptions::checkValues()
{
#ifdef DEBUGPRINT
  cerr << "VprofOptions::checkValues" << endl;
#endif

  if (diagramtype<0 || diagramtype>3) diagramtype= 0;
  if (tangle< 1.) tangle= 0.;
  if (tangle>80.) tangle=80.;

  if (tmaxDiagram-tminDiagram<5) tminDiagram= tmaxDiagram-5;
  if (trangeDiagram<0 || trangeDiagram>2) trangeDiagram=0;

  if (rsvaxis<0.01)   rsvaxis=  0.01;
  if (rsvaxis>100.)   rsvaxis=  100.;
  if (rstext<0.01)    rstext=   0.01;
  if (rstext>100.)    rstext=   100.;
  if (rslabels<0.01)  rslabels= 0.01;
  if (rslabels>100.)  rslabels= 100.;
  if (rswind<0.01)    rswind=   0.01;
  if (rswind>100.)    rswind=   100.;
  if (rsvwind<0.01)   rsvwind=  0.01;
  if (rsvwind>100.)   rsvwind=  100.;
  if (rsrelhum<0.01)  rsrelhum= 0.01;
  if (rsrelhum>100.)  rsrelhum= 100.;
  if (rsducting<0.01) rsducting=0.01;
  if (rsducting>100.) rsducting=100.;
}


vector<miString> VprofOptions::writeOptions()
{
#ifdef DEBUGPRINT
  cerr << "VprofOptions::writeOptions" << endl;
#endif

  vector<miString> vstr;
  miString str;

  str= "tttt=" + miString(ptttt ? "on" : "off");
  vstr.push_back(str);
  str= "tdtd=" + miString(ptdtd ? "on" : "off");
  vstr.push_back(str);
  str= "wind=" + miString(pwind ? "on" : "off");
  vstr.push_back(str);
  str= "vwind=" + miString(pvwind ? "on" : "off");
  vstr.push_back(str);
  str= "relhum=" + miString(prelhum ? "on" : "off");
  vstr.push_back(str);
  str= "ducting=" + miString(pducting ? "on" : "off");
  vstr.push_back(str);
  str= "kindex=" + miString(pkindex ? "on" : "off");
  vstr.push_back(str);
  str= "slwind=" + miString(pslwind ? "on" : "off");
  vstr.push_back(str);

  if (dataColour.size()) {
    str= "dataColour=";
    for (unsigned int i=0; i<dataColour.size(); i++) {
      if (i>0) str+=",";
      str+=dataColour[i];
    }
    vstr.push_back(str);
  }
  if (dataLinewidth.size()) {
    str= "dataLinewidth=";
    for (unsigned int i=1; i<dataLinewidth.size(); i++) {
      if (i>0) str+=",";
      str+= miString(dataLinewidth[i]);
    }
    vstr.push_back(str);
  }
  if (windLinewidth.size()) {
    str= "windLinewidth=";
    for (unsigned int i=1; i<windLinewidth.size(); i++) {
      if (i>0) str+=",";
      str+= miString(windLinewidth[i]);
    }
    vstr.push_back(str);
  }

  str= "windseparate=" + miString(windseparate ? "on" : "off");
  str+= " text=" + miString(ptext ? "on" : "off");
  str+= " geotext=" + miString(pgeotext ? "on" : "off");
  str+= " temptext=" + miString(temptext ? "on" : "off");
  vstr.push_back(str);

  str= "rvwind=" + miString(rvwind);
  str+= (" ductingMin=" + miString(ductingMin));
  str+= (" ductingMax=" + miString(ductingMax));
  str+= (" linetext=" + miString(linetext));
  vstr.push_back(str);

  str= "backgroundColour=" + backgroundColour;
  vstr.push_back(str);

  str= "diagramtype=" + miString(diagramtype);
  str+= (" tangle=" + miString(tangle));
  vstr.push_back(str);

  str=   "pminDiagram=" + miString(pminDiagram)
  + " pmaxDiagram=" + miString(pmaxDiagram);
  vstr.push_back(str);
  str=   "tminDiagram=" + miString(tminDiagram)
  + " tmaxDiagram=" + miString(tmaxDiagram)
  + " trangeDiagram=" + miString(trangeDiagram);
  vstr.push_back(str);

  str=  "plinesfl=" + miString(pplinesfl ? "on" : "off");
  vstr.push_back(str);

  str=  "plines=" + miString(pplines ? "on" : "off");
  str+= (" pColour=" + pColour);
  str+= (" pLinetype=" + pLinetype);
  str+= (" pLinewidth1=" + miString(pLinewidth1));
  str+= (" pLinewidth2=" + miString(pLinewidth2));
  vstr.push_back(str);

  str=  "tlines=" + miString(ptlines ? "on" : "off");
  str+= (" tStep=" + miString(tStep));
  str+= (" tColour=" + tColour);
  str+= (" tLinetype=" + tLinetype);
  str+= (" tLinewidth1=" + miString(tLinewidth1));
  str+= (" tLinewidth2=" + miString(tLinewidth2));
  vstr.push_back(str);

  str=  "dryadiabat=" + miString(pdryadiabat ? "on" : "off");
  str+= (" dryadiabatStep=" + miString(dryadiabatStep));
  str+= (" dryadiabatColour=" + dryadiabatColour);
  str+= (" dryadiabatLinetype=" + dryadiabatLinetype);
  str+= (" dryadiabatLinewidth=" + miString(dryadiabatLinewidth));
  vstr.push_back(str);

  str=  "wetadiabat=" + miString(pwetadiabat ? "on" : "off");
  str+= (" wetadiabatStep=" + miString(wetadiabatStep));
  str+= (" wetadiabatColour=" + wetadiabatColour);
  str+= (" wetadiabatLinetype=" + wetadiabatLinetype);
  str+= (" wetadiabatLinewidth=" + miString(wetadiabatLinewidth));
  str+= (" wetadiabatPmin=" + miString(wetadiabatPmin));
  str+= (" wetadiabatTmin=" + miString(wetadiabatTmin));
  vstr.push_back(str);

  str=  "mixingratio=" + miString(pmixingratio ? "on" : "off");
  str+= (" mixingratioSet=" + miString(mixingratioSet));
  str+= (" mixingratioColour=" + mixingratioColour);
  str+= (" mixingratioLinetype=" + mixingratioLinetype);
  str+= (" mixingratioLinewidth=" + miString(mixingratioLinewidth));
  str+= (" mixingratioPmin=" + miString(mixingratioPmin));
  str+= (" mixingratioTmin=" + miString(mixingratioTmin));
  vstr.push_back(str);

  str=  "labelp=" + miString(plabelp ? "on" : "off");
  str+= " labelt=" + miString(plabelt ? "on" : "off");
  str+= " labelq=" + miString(plabelq ? "on" : "off");
  vstr.push_back(str);

  str=  "frame="  + miString(pframe ? "on" : "off");
  str+= (" frameColour=" + frameColour);
  str+= (" frameLinetype=" + frameLinetype);
  str+= (" frameLinewidth=" + miString(frameLinewidth));
  vstr.push_back(str);

  str= ("textColour=" + textColour);
  vstr.push_back(str);

  str=  "flevels=" + miString(pflevels ? "on" : "off");
  str+= (" flevelsColour=" + flevelsColour);
  str+= (" flevelsLinetype=" + flevelsLinetype);
  str+= (" flevelsLinewidth1=" + miString(flevelsLinewidth1));
  str+= (" flevelsLinewidth2=" + miString(flevelsLinewidth2));
  vstr.push_back(str);

  str=  "labelflevels=" + miString(plabelflevels ? "on" : "off");
  vstr.push_back(str);

  str=   "rsvaxis="   + miString(rsvaxis);
  str+= " rstext="    + miString(rstext);
  str+= " rslabels="  + miString(rslabels);
  str+= " rswind="    + miString(rswind);
  str+= " rsvwind="   + miString(rsvwind);
  str+= " rsrelhum="  + miString(rsrelhum);
  str+= " rsducting=" + miString(rsducting);
  vstr.push_back(str);

  str=   "rangeLinetype=" + rangeLinetype;
  str+= (" rangeLinewidth=" + miString(rangeLinewidth));
  vstr.push_back(str);

  str=  "cotrails="  + miString(pcotrails ? "on" : "off");
  str+= (" cotrailsColour=" + cotrailsColour);
  str+= (" cotrailsLinetype=" + cotrailsLinetype);
  str+= (" cotrailsLinewidth=" + miString(cotrailsLinewidth));
  str+= (" cotrailsPmin=" + miString(cotrailsPmin));
  str+= (" cotrailsPmax=" + miString(cotrailsPmax));
  vstr.push_back(str);

  return vstr;
}


void VprofOptions::readOptions(const vector<miString>& vstr)
{
#ifdef DEBUGPRINT
  cerr << "VprofOptions::readOptions" << endl;
#endif

  vector<miString> vs,tokens;
  miString key,value;

  int n= vstr.size();

  for (int i=0; i<n; i++) {

    vs= vstr[i].split(' ');

    int m= vs.size();

    for (int j=0; j<m; j++) {

      tokens= vs[j].split('=');

      if (tokens.size()==2) {

        //	key=   tokens[0].downcase();
        key=   tokens[0];
        value= tokens[1];

        if      (key=="tttt")       ptttt=    (value.downcase()=="on");
        else if (key=="tdtd")       ptdtd=    (value.downcase()=="on");
        else if (key=="wind")       pwind=    (value.downcase()=="on");
        else if (key=="vwind")      pvwind=   (value.downcase()=="on");
        else if (key=="relhum")     prelhum=  (value.downcase()=="on");
        else if (key=="ducting")    pducting= (value.downcase()=="on");
        else if (key=="kindex")     pkindex=  (value.downcase()=="on");
        else if (key=="slwind")     pslwind=  (value.downcase()=="on");

        else if (key=="windseparate") windseparate= (value.downcase()=="on");
        else if (key=="text")         ptext=        (value.downcase()=="on");
        else if (key=="geotext")      pgeotext=     (value.downcase()=="on");
        else if (key=="temptext")     temptext=     (value.downcase()=="on");

        else if (key=="rvwind")     rvwind=     atof(value.cStr());
        else if (key=="ductingMin") ductingMin= atof(value.cStr());
        else if (key=="ductingMax") ductingMax= atof(value.cStr());
        else if (key=="linetext")   linetext=   atoi(value.cStr());

        else if (key=="backgroundColour") backgroundColour= value;

        else if (key=="diagramtype") diagramtype= atoi(value.cStr());
        else if (key=="tangle")      tangle=      atof(value.cStr());

        else if (key=="pminDiagram")   pminDiagram= atoi(value.cStr());
        else if (key=="pmaxDiagram")   pmaxDiagram= atoi(value.cStr());
        else if (key=="tminDiagram")   tminDiagram= atoi(value.cStr());
        else if (key=="tmaxDiagram")   tmaxDiagram= atoi(value.cStr());
        else if (key=="trangeDiagram") trangeDiagram= atoi(value.cStr());

        else if (key=="plinesfl")  pplinesfl= (value.downcase()=="on");

        else if (key=="plines")      pplines= (value.downcase()=="on");
        else if (key=="pColour")     pColour= value;
        else if (key=="pLinetype")   pLinetype= value;
        else if (key=="pLinewidth1") pLinewidth2= atof(value.cStr());
        else if (key=="pLinewidth2") pLinewidth2= atof(value.cStr());

        else if (key=="tlines")      ptlines=     (value.downcase()=="on");
        else if (key=="tStep")       tStep=       atoi(value.cStr());
        else if (key=="tColour")     tColour=     value;
        else if (key=="tLinetype")   tLinetype=   value;
        else if (key=="tLinewidth1") tLinewidth2= atof(value.cStr());
        else if (key=="tLinewidth2") tLinewidth2= atof(value.cStr());

        else if (key=="dryadiabat")          pdryadiabat=    (value.downcase()=="on");
        else if (key=="dryadiabatStep")      dryadiabatStep=      atoi(value.cStr());
        else if (key=="dryadiabatColour")    dryadiabatColour=    value;
        else if (key=="dryadiabatLinetype")  dryadiabatLinetype=  value;
        else if (key=="dryadiabatLinewidth") dryadiabatLinewidth= atof(value.cStr());

        else if (key=="wetadiabat")          pwetadiabat=    (value.downcase()=="on");
        else if (key=="wetadiabatStep")      wetadiabatStep=      atoi(value.cStr());
        else if (key=="wetadiabatColour")    wetadiabatColour=    value;
        else if (key=="wetadiabatLinetype")  wetadiabatLinetype=  value;
        else if (key=="wetadiabatLinewidth") wetadiabatLinewidth= atof(value.cStr());
        else if (key=="wetadiabatPmin")      wetadiabatPmin=      atoi(value.cStr());
        else if (key=="wetadiabatTmin")      wetadiabatTmin=      atoi(value.cStr());

        else if (key=="mixingratio")          pmixingratio=    (value.downcase()=="on");
        else if (key=="mixingratioSet")       mixingratioSet=   atoi(value.cStr());
        else if (key=="mixingratioColour")    mixingratioColour=    value;
        else if (key=="mixingratioLinetype")  mixingratioLinetype=  value;
        else if (key=="mixingratioLinewidth") mixingratioLinewidth= atof(value.cStr());
        else if (key=="mixingratioPmin")      mixingratioPmin=      atoi(value.cStr());
        else if (key=="mixingratioTmin")      mixingratioTmin=      atoi(value.cStr());

        else if (key=="labelp") plabelp= (value.downcase()=="on");
        else if (key=="labelt") plabelt= (value.downcase()=="on");
        else if (key=="labelq") plabelq= (value.downcase()=="on");

        else if (key=="frame")          pframe= (value.downcase()=="on");
        else if (key=="frameColour")    frameColour= value;
        else if (key=="frameLinetype")  frameLinetype= value;
        else if (key=="frameLinewidth") frameLinewidth= atof(value.cStr());

        else if (key=="textColour") textColour= value;

        else if (key=="flevels")           pflevels= (value.downcase()=="on");
        else if (key=="flevelsColour")     flevelsColour= value;
        else if (key=="flevelsLinetype")   flevelsLinetype= value;
        else if (key=="flevelsLinewidth1") flevelsLinewidth1= atof(value.cStr());
        else if (key=="flevelsLinewidth2") flevelsLinewidth2= atof(value.cStr());

        else if (key=="labelflevels") plabelflevels= (value.downcase()=="on");

        else if (key=="rsvaxis")   rsvaxis=   atof(value.cStr());
        else if (key=="rstext")    rstext=    atof(value.cStr());
        else if (key=="rslabels")  rslabels=  atof(value.cStr());
        else if (key=="rswind")    rswind=    atof(value.cStr());
        else if (key=="rsvwind")   rsvwind=   atof(value.cStr());
        else if (key=="rsrelhum")  rsrelhum=  atof(value.cStr());
        else if (key=="rsducting") rsducting= atof(value.cStr());

        else if (key=="rangeLinetype")  rangeLinetype= value;
        else if (key=="rangeLinewidth") rangeLinewidth= atof(value.cStr());

        else if (key=="cotrails")          pcotrails=    (value.downcase()=="on");
        else if (key=="cotrailsColour")    cotrailsColour=    value;
        else if (key=="cotrailsLinetype")  cotrailsLinetype=  value;
        else if (key=="cotrailsLinewidth") cotrailsLinewidth= atof(value.cStr());
        else if (key=="cotrailsPmin")      cotrailsPmin=      atoi(value.cStr());
        else if (key=="cotrailsPmax")      cotrailsPmax=      atoi(value.cStr());

      }

      if (tokens.size()>=2) {

        //	key=   tokens[0].downcase();
        key=   tokens[0];
        value= tokens[1];
        vector<miString> vs= value.split(',');
        int nv= vs.size();

        if (nv>0) {
          if (key=="dataColour") {
            if (nv>int(dataColour.size())) dataColour.resize(nv);
            for (int k=0; k<nv; k++)
              dataColour[k]= vs[k];
          } else if (key=="dataLinewidth") {
            if (nv>int(dataLinewidth.size())) dataLinewidth.resize(nv);
            for (int k=0; k<nv; k++)
              dataLinewidth[k]= atof(vs[k].cStr());
          } else if (key=="windLinewidth") {
            if (nv>int(windLinewidth.size())) windLinewidth.resize(nv);
            for (int k=0; k<nv; k++)
              windLinewidth[k]= atof(vs[k].cStr());
          }
        }
      }

    }
  }

  changed= true;
}