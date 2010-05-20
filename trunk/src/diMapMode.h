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
#ifndef _diMapMode_h
#define _diMapMode_h

#include <diKeys.h>

/// The state of the applications main work area
enum mapMode{
  normal_mode,  // normal map-zoom, -pan etc.
  fedit_mode,   // edit fields
  draw_mode,    // drawing of symbols
  combine_mode  // combine products
};


enum field_modes{
  field_manip   // field manipulation tools
};

enum object_modes{
  front_drawing,  // draw fronts
  symbol_drawing, // draw symbols, free text etc
  area_drawing    // draw areas
};

enum combine_modes{
  set_region,     // select region
  set_borders     // edit border
};

/// types of mouse events
enum mouseEventType{
  mousepress,
  mouserelease,
  mousemove,
  mousedoubleclick,
  mouseunknown
};

/// the mouse buttons
enum mouseButton{
  noButton,
  leftButton,
  midButton,
  rightButton
};

/// types of keyboard events
enum keyboardEventType{
  keypress,
  keyrelease,
  keytype_unknown
};

/// single mouse event from GUI
struct mouseEvent{
  mouseButton button;
  mouseEventType type;
  int x;  // pointer position
  int y;
  int globalX;
  int globalY;
  KeyType modifier;
  mouseEvent() :
    button(noButton), type(mouseunknown), x(0), y(0), globalX(0), globalY(0),
        modifier(key_unknown)
  {
  }
};

/// single keyboard event from GUI
struct keyboardEvent{
  keyboardEventType type;
  KeyType key;
  KeyType modifier;
  keyboardEvent() :
    type(keytype_unknown), key(key_unknown), modifier(key_unknown)
  {
  }
};

/// suggestive use of cursors
enum cursortype{
  keep_it,
  normal_cursor,
  edit_cursor,
  edit_move_cursor,
  edit_value_cursor,
  draw_cursor,
  paint_select_cursor,
  paint_move_cursor,
  paint_draw_cursor,
  paint_add_crusor,
  paint_remove_crusor,
  paint_forbidden_crusor
};

enum actiontype {
  no_action,
  pointclick,
  rightclick,
  browsing,
  quick_browsing,
  objects_changed,
  fields_changed,
  keypressed,
  grid_area_changed
};

/// returned to GUI after sent keyboard/mouse-events
struct EventResult{
  bool repaint;          // do a repaint
  bool background;       // paint background as well
  bool savebackground;   // start saving background to pixelbuffer
  cursortype newcursor;  // set to new cursor or 'keep_it'
  actiontype action;     // should the event trigger a GUI-action
  EventResult() :
    repaint(false), background(false), savebackground(false),
        newcursor(keep_it), action(no_action)
  {
  }
};

#endif