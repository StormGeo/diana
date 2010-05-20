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
#ifndef _qtLoginDialog_h
#define _qtLoginDialog_h

#include <QDialog>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <diEditSpec.h>

/**
   \brief Login dialog
   
   A login dialog for use with database connections

*/

class QPushButton;
class QLineEdit;

class LoginDialog : public QDialog 
{
  Q_OBJECT
public:
  LoginDialog(editDBinfo& d,  QWidget* parent);

  editDBinfo getDbInfo();

private:
  editDBinfo dbi;

  QLineEdit* dbserver;
  QLineEdit* username;
  QLineEdit* passwd;

  QPushButton *okb;
  QPushButton *quitb;
};

#endif