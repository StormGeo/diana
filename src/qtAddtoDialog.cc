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
/*
  Input for adding new choices to menus
*/

#include <qtAddtoDialog.h>

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QString>

#include <fstream>
#include <miString.h>
#include <iostream>
#include <diController.h>
#include <diObjectManager.h>




/*********************************************/
AddtoDialog::AddtoDialog( QWidget* parent, Controller* llctrl)
  : QDialog(parent), m_ctrl(llctrl), m_objm(0)
{
#ifdef DEBUGPRINT 
  cout<<"AddtoDialog::AddtoDialog called"<<endl;
#endif
     
  m_objm= m_ctrl->getObjectManager();

  setWindowTitle(tr("Add to objectdialog"));
  setModal(true);

  QGridLayout* glayout = new QGridLayout();

  QLabel* namelabel= new QLabel(tr("Dialog name:"), this); 
  QLabel* filelabel = new QLabel(tr("File name:"),   this);

  name  = new QLineEdit(this);
  name->setMinimumWidth(420);
  file  = new QLineEdit(this);
  file->setMinimumWidth(420);

  glayout->addWidget(namelabel, 1,1);
  glayout->addWidget(name, 1,2);
  glayout->addWidget(filelabel, 2,1);
  glayout->addWidget(file,2,2);

  QPushButton * okb= new QPushButton(tr("OK"),this);
  connect(okb, SIGNAL(clicked()), SLOT(accept()));
  QPushButton* quitb= new QPushButton(tr("Cancel"),this);
  connect(quitb, SIGNAL(clicked()), SLOT(reject()));

  int width  = quitb->sizeHint().width();
  int height = quitb->sizeHint().height();
  //set button size
  okb->setMinimumSize( width, height );
  okb->setMaximumSize( width, height );  
  quitb->setMinimumSize( width, height );
  quitb->setMaximumSize( width, height );

  // buttons layout
  QHBoxLayout * hlayout = new QHBoxLayout();
  hlayout->addWidget(okb, 10);
  hlayout->addWidget(quitb, 10);

  //now create a vertical layout to put all the other layouts in
  QVBoxLayout * vlayout = new QVBoxLayout( this );                            
  vlayout->addLayout(glayout, 0);
  vlayout->addLayout(hlayout,0);
}


miString AddtoDialog::getName(){
  return name->text().toStdString(); 
}


miString AddtoDialog::getFile(){
  return file->text().toStdString(); 
}


void AddtoDialog::putName(const miString & namestring){
  name->setText(namestring.c_str());
}


void AddtoDialog::putFile(const miString & filestring){
  file->setText(filestring.c_str());
}


