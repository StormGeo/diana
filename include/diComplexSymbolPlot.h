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
#ifndef ComplexSymbolPlot_h
#define ComplexSymbolPlot_h

#include <diPlot.h>
#include <set>

/**

  \brief Draw complex sigmap symbols

The symbols are a combination of font symbols and text the user can enter

*/


class ComplexSymbolPlot: public Plot
{
public:
 // Constructor
  ComplexSymbolPlot();
  ComplexSymbolPlot(int drawIndex);
  /// draw complex symbol with index drawindex centered at x,y fontsize size and rotation rot 
  void draw(int drawIndex, float x,float y,int size, float rot);
  /// hides/shows the white box behind symbol
  void hideBox();
  /// check if white box to be drawn
  int hasWhiteBox();
  /// hides/shows the white box behind symbol
  void setWhiteBox(int on);
  /// reads complex text to be plotted from a string (written by readComplexText)
  void readComplexText(miString complexString);
  /// gets miString vectors with symboltext and xText
  void getComplexText(vector <miString> & symbolText, vector <miString> & xText);
  /// change text to be drawn
  void changeComplexText(const vector <miString> & symbolText, const vector <miString> & xText);
  /// writes complex text to a string (read by readComplexText)
  miString writeComplexText();
  /// get the the size and x,y of center of bounding box
  void getComplexBoundingBox(int index, float& sw, float & sh, float & x, float & y);
  void setBorderColour(const miString& colstring){borderColour= Colour(colstring);}
  /// returns true if symbol is complex text
  static bool isComplexText(int drawIndex);
  /// gets current complex text (used in text dialog)
  static void getCurrentComplexText(vector <miString> & symbolText, 
			     vector <miString> & xText);
  /// sets current complex text (used in text dialog)
  static void setCurrentComplexText(const vector <miString> & symbolText,
			     const vector <miString> & xText); 
  /// Initial values of current strings (used in text dialog)
  static void initCurrentStrings(int drawIndex);
  /// sets list of complex texts (used in text dialog)
  static void initComplexList();
  /// get list of complex texts (used in text dialog)
  static set<miString> getComplexList();


private:
  void initStrings(int drawIndex);
  void drawSigString(float x,float y,bool whitebox=true);
  void drawSigText(float x,float y, bool whitebox=true);
  void drawDoubleSigText(float x,float y, bool whitebox=true);
  void drawDoubleSigTextAndSymbol(int symbol,float x,float y);
  void drawSig1(float x,float y);
  void drawSig2(float x,float y);
  void drawSig3(float x,float y);
  void drawSig4(float x,float y);
  void drawSig5(float x,float y);
  void drawSig6(float x,float y);
  void drawSig7(float x,float y);
  void drawSig8(float x,float y);
  void drawSig9(float x,float y);
  void drawSig10(float x,float y);
  void drawSig11(float x,float y);
  void drawSig12(float x,float y);
  void drawSig13(float x,float y);
  void drawSig14(float x,float y);
  void drawSig15(float x,float y);
  void drawSig16(float x,float y);
  void drawSig17(float x,float y);
  void drawSig18(float x,float y, int metSymbol);
  void drawSig22(float x,float y);
  void drawSig27(float x,float y);
  void drawSig28(float x,float y);
  void drawSig29(float x,float y);
  void drawSig30(float x,float y);
  void drawSig31(float x,float y);
  void drawSig32(float x,float y);
  void drawSig33(float x,float y);
  void drawSig34(float x,float y);
  void drawSig35(float x,float y, int drawIndex);
  void drawSig36(float x,float y);
  void drawBox(int index,float x, float y, bool fill=true);
  void drawBigBox(int index,float x, float y, int size);
  void drawCircle(int index,float x, float y, bool circle=false);
  void drawDiamond(int index,float x, float y);
  void drawFlag(int index,float x, float y, bool fill=false);
  void drawNuclear(float x,float y);
  void drawPrecipitation(float x,float y);
  void getComplexSize(int index, float& sw, float & sh);
  void drawSymbol(int index,float x,float y);

  //text used in new complex symbols
  static vector <miString> currentSymbolStrings; 
  static vector <miString> currentXStrings; //new sigstrings  (x/x)
  static set <miString> clist;
  vector <miString> symbolStrings; //text used in this complex symbols
  vector <miString> xstrings; // sigstrings  (x/x)
  bool xvisible; //used when editing strings
  int nstringsvisible; //used when editing strings 
  int symbolSizeToPlot;

  miString sigString;
  bool whiteBox;
  static float textShrink;
  float diffx,diffy;

  Colour borderColour;
};

#endif









