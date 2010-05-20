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
#ifndef diAnnotationPlot_h
#define diAnnotationPlot_h

#include <diPlot.h>
#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <vector>
#include <map>

using namespace std;

class LegendPlot;

/**

 \brief Plotting text, legends etc on the map

 Includes: text, symbol, image, input, legend, arrow, box

 */
class AnnotationPlot: public Plot {

  enum annoType {
    anno_data, anno_text,
  };

  enum elementType {
    text, symbol, image, input, table, arrow, box
  };

  struct Border {
    float x1, x2, y1, y2;
  };

  struct element {
    vector<element> subelement;
    elementType eType;
    miutil::miString eText;
    int eCharacter;
    miutil::miString eFont;
    miutil::miString eFace;
    miutil::miString textcolour;
    miutil::miString eImage;
    LegendPlot* classplot;
    float arrowLength;
    bool arrowFeather;
    miutil::miString eName;
    Alignment eHalign; //where it makes sense
    float eSize;
    float x1, y1, x2, y2;//dimensions and size of element
    bool isInside, inEdit;
    int itsCursor, itsSel1, itsSel2;
    int eAlpha;
    float width, height;
    bool horizontal;
    polyStyle polystyle;
  };

public:
  /**

   \brief Annotation text, format, alignment etc

   */
  struct Annotation {
    miutil::miString str;
    vector<miutil::miString> vstr;
    Colour col;
    Alignment hAlign;
    polyStyle polystyle;
    Colour bordercolour;
    vector<element> annoElements;
    bool spaceLine;
    float wid, hei;
    Rectangle rect;
    bool oldFormat;
  };

private:

  vector<Annotation> annotations;
  vector<Annotation> orig_annotations;
  annoType atype;
  vector<Border> borderline;

  //for comparing input text
  map<miutil::miString, miutil::miString> inputText;

  // OKstring variables
  float cmargin;
  float cspacing;
  float cxoffset;
  float cyoffset;
  int clinewidth; //line width of border
  float cxratio; //ratio between frame and annotation box in x
  float cyratio; //ratio between frame and annotation box in x
  bool plotRequested;//annotations aligned rel. to frame (not window)
  bool nothingToDo;
  //
  miutil::miString labelstrings; //fixed part of okstrings
  miutil::miString productname;
  bool editable;

  Rectangle bbox;
  bool scaleAnno, plotAnno;
  bool isMarked;
  //float xbox,ybox;
  float fontsizeToPlot;
  float scaleFactor;
  float border;
  float spacing;
  float maxwid;

  bool useAnaTime;
  vector<miutil::miTime> fieldAnaTime;
  Colour currentColour;

  //called from constructor
  void init();
  // insert time in text string
  const miutil::miString insertTime(const miutil::miString&, const miutil::miTime&);
  // expand string-variables
  const vector<miutil::miString> expanded(const vector<miutil::miString>&);
  // decode string, put into elements
  void splitAnnotations();
  bool putElements();
  void addElement2Vector(vector<element>& v_e, const element& e, int index);
  bool decodeElement(miutil::miString elementstring, element& el);
  //get size of annotation line
  void getAnnoSize(vector<element>& annoEl, float& wid, float& hei,
      bool horizontal = true);
  void getXYBox();
  void getXYBoxScaled(Rectangle& window);
  bool plotElements(vector<element>& annoEl, float& x, float& y,
      float annoHeight, bool horizontal = true);
  float plotArrow(float x, float y, float l, bool feather = false);
  void plotBorders();
  vector<miutil::miString> split(const miutil::miString, const char, const char);
  miutil::miString writeElement(element& annoEl);

public:
  // Constructors
  AnnotationPlot();
  AnnotationPlot(const miutil::miString&);

  bool plot();
  bool plot(const int)
  {
    return false;
  }
  ///decode plot info strings
  bool prepare(const miutil::miString&);
  ///set data annotations
  bool setData(const vector<Annotation>& a,
      const vector<miutil::miTime>& fieldAnalysisTime);
  void setfillcolour(miutil::miString colname);
  /// mark editable annotationPlot if x,y inside plot
  bool markAnnotationPlot(int, int);
  /// get text of marked and editable annotationPlot
  miutil::miString getMarkedAnnotation();
  /// change text of marked and editable annotationplot
  void changeMarkedAnnotation(miutil::miString text, int cursor = 0, int sel1 = 0,
      int sel2 = 0);
  /// delete marked and editable annotation
  void DeleteMarkedAnnotation();
  /// start editing annotations
  void startEditAnnotation();
  /// stop editing annotations
  void stopEditAnnotation();
  /// go to next element in annotation being edited
  void editNextAnnoElement();
  /// go to last element in annotation being edited
  void editLastAnnoElement();
  /// put info from saved edit labels into new annotation
  void updateInputLabels(const AnnotationPlot * oldAnno, bool newProduct);
  /// return vector miutil::miStrings with edited annotation for product prodname
  miutil::miString writeAnnotation(miutil::miString prodname);
  void setProductName(miutil::miString prodname)
  {
    productname = prodname;
  }
  //get annotations, change them somewhere else, and put them back
  vector<vector<miutil::miString> > getAnnotationStrings();
  ///replace annotations
  bool setAnnotationStrings(vector<vector<miutil::miString> >& vstr);
};

#endif