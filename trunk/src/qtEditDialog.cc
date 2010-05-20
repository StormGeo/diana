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

#include <QMessageBox>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTabWidget>
#include <QCheckBox>
#include <QSlider>
#include <QPixmap>
#include <QImage>
#include <QInputDialog>
#include <QAction>
#include <QHBoxLayout>
#include <QFrame>
#include <QVBoxLayout>

#include <iostream>
#include <qtEditDialog.h>
#include <qtEditNewDialog.h>
#include <qtEditComment.h>
#include <qtUtility.h>
#include <qtToggleButton.h>
#include <qtTimeStepSpinbox.h>
#include <qtComplexText.h>
#include <qtEditText.h>
#include <qtAnnoText.h>
//Added by qt3to4:

#include <diSetupParser.h>
#include <diController.h>
#include <diEditManager.h>
#include <diObjectManager.h>

#include <edit_open_value.xpm>
#include <edit_lock_value.xpm>


/*********************************************/
#define HEIGHTLISTBOX 120
#define FIELD_INDEX 0
#define OBJECT_INDEX 1

#define FRONT_INDEX 0
#define SYMBOL_INDEX 1
#define AREA_INDEX 2
#define SIGMAP_INDEX 3
// #define dEditDlg
// #define DEBUGPRINT

/*********************************************/
EditDialog::EditDialog( QWidget* parent, Controller* llctrl )
: QDialog(parent), m_ctrl(llctrl), m_editm(0)
{
#ifdef dEditDlg
  cout<<"EditDialog::EditDialog called"<<endl;
#endif

  TABNAME_FIELD= tr("Field");
  TABNAME_OBJECTS= tr("Objects");
  TABNAME_COMBINE= tr("Merge");

  EditDialogInfo ll=llctrl->initEditDialog();

  //list of translations to appear in dialog:
  editTranslations["Change value"]=tr("Change value");
  editTranslations["Move"]=tr("Move");
  editTranslations["Change gradient"]=tr("Change gradient");
  editTranslations["Line, without smooth"]=tr("Line, without smooth");
  editTranslations["Line, with smooth"]=tr("Line, with smooth");
  editTranslations["Line, limited, without smooth"]=tr("Line, limited, without smooth");
  editTranslations["Line, limited, with smooth"]=tr("Line, limited, with smooth");
  editTranslations["Smooth"]=tr("Smooth");
  editTranslations["Replace undefined values"]=tr("Replace undefined values");
  editTranslations["Line"]=tr("Line");
  editTranslations["Copy value"]=tr("Copy value");
  editTranslations["Set undefined"]=tr("Set udefined");
  editTranslations["Cold front"]=tr("Cold front"); //Kaldfront
  editTranslations["Warm front"]=tr("Warm front"); //Varmfront
  editTranslations["Occlusion"]=tr("Occlusion"); //Okklusjon
  editTranslations["Cold occlusion"]=tr("Cold occlusion"); //Kald okklusjon
  editTranslations["Warm occlusion"]=tr("Warm occlusion"); //Varm okklusjon
  editTranslations["Stationary front"]=tr("Stationary front"); //Stasjon�r front
  editTranslations["Trough"]=tr("Trough"); //Tr�g
  editTranslations["Squall line"]=tr("Squall line"); //Bygelinje
  editTranslations["Significant weather"]=tr("Significant weather"); //Sig.v�r

  editTranslations["Low pressure"]=tr("Low pressure"); //Lavtrykk
  editTranslations["High pressure"]=tr("High pressure"); //H�ytrykk
  editTranslations["Cold"]=tr("Cold"); //Kald
  editTranslations["Warm"]=tr("Warm"); //Varm
  editTranslations["Fog"]=tr("Fog"); //T�ke
  editTranslations["Drizzle"]=tr("Drizzle "); //yr
  editTranslations["Freezing drizzle"]=tr("Freezing drizzle"); //Yr som fryser
  editTranslations["Freezing rain"]=tr("Freezing rain"); //Regn som fryser
  editTranslations["Showers"]=tr("Showers"); //Byger
  editTranslations["Rain showers"]=tr("Rain showers"); //Regnbyger
  editTranslations["Sleet showers"]=tr("Sleet showers"); //Sluddbyger ??
  editTranslations["Hail showers"]=tr("Hail showers"); //Haglbyger
  editTranslations["Snow showers"]=tr("Snow showers"); //Haglbyger
  editTranslations["Thunderstorm"]=tr("Thunderstorm"); //Tordenv�r
  editTranslations["Thunderstorm with hail"]=tr("Thunderstorm with hail"); //Tordenv�r m/hagl
  editTranslations["Snow"]=tr("Snow"); //Sn�(stjerne)    ??
  editTranslations["Rain"]=tr("Rain"); //Regn
  editTranslations["Sleet"]=tr("Sleet"); //Sludd

  editTranslations["Hurricane"]=tr("Hurricane"); //Tropisk orkan ??

  editTranslations["Disk"]=tr("Disk"); //
  editTranslations["Circle"]=tr("Circle"); //Sirkel
  editTranslations["Cross"]=tr("Cross");   //Kryss
  editTranslations["Text"]=tr("Text");   //Tekster
  editTranslations["EditText"]=tr("EditText");   //Tekster


  editTranslations["Precipitation"]=tr("Precipitation"); //Nedb�r ??
  editTranslations["Showers"]=tr("Showers"); //Byger
  editTranslations["Clouds"]=tr("Clouds"); //Skyer
  editTranslations["Fog"]=tr("Fog"); //T�ke
  editTranslations["Ice"]=tr("Ice"); //Is
  editTranslations["Significant weather"]=tr("Significant weather"); //Sig.v�r
  editTranslations["Reduced visibility"]=tr("Reduced visibility"); //Sig.v�r
  editTranslations["Generic area"]=tr("Generic area"); //

  // --------------------------------------------------------------------
  editAction = new QAction(this);
  editAction->setShortcut(Qt::CTRL+Qt::Key_E);
  editAction->setShortcutContext(Qt::ApplicationShortcut);
  connect(editAction, SIGNAL( triggered() ), SLOT(EditMarkedText()));
  addAction( editAction );
  // --------------------------------------------------------------------
  deleteAction = new QAction(this);
  deleteAction->setShortcut(Qt::CTRL+Qt::Key_Delete);
  deleteAction->setShortcutContext(Qt::ApplicationShortcut);
  connect(deleteAction, SIGNAL( triggered() ) , SLOT(DeleteMarkedAnnotation()));
  addAction( deleteAction );


  ConstructorCernel( ll );


}



/*********************************************/
void EditDialog::ConstructorCernel( const EditDialogInfo mdi )
{
#ifdef dEditDlg
  cout<<"EditDialog::ConstructorCernel called"<<endl;
#endif

  m_editm= m_ctrl->getEditManager();
  m_objm=  m_ctrl->getObjectManager();

  setWindowTitle(tr("Editing"));

  inEdit= false;
  productApproved= false;
  combineAction= -1;
  fieldIndex= -1;
  fieldEditToolGroup= 0;
  numFieldEditTools=  0;
  currFieldEditToolIndex= 0;

  openValuePixmap= QPixmap(edit_open_value_xpm);
  lockValuePixmap= QPixmap(edit_lock_value_xpm);

  bgroup = new QButtonGroup( this );
  QHBoxLayout* bgroupLayout = new QHBoxLayout();
  int m_nr_buttons=3;
  b = new QPushButton*[m_nr_buttons];
  vector<miutil::miString> vstr(3);
  vstr[prodb]=tr("Product").toStdString();
  vstr[saveb]=tr("Save").toStdString();
  vstr[sendb]=tr("Send").toStdString();

  int i;

  for( i=0; i< m_nr_buttons; i++ ){
    b[i] = NormalPushButton( vstr[i].c_str(), this );
    bgroup->addButton(b[i],i);
    b[i]->setFocusPolicy(Qt::ClickFocus);
    bgroupLayout->addWidget(b[i]);
  }

  b[prodb]->setFocusPolicy(Qt::StrongFocus);
  b[saveb]->setEnabled(false);
  b[sendb]->setEnabled(false);

  // ********** TAB
  twd = new QTabWidget( this );

  FieldTab();
  FrontTab();
  CombineTab();

  twd->setEnabled(false); // initially disabled

  connect( twd, SIGNAL(currentChanged( int )),
      SLOT( tabSelected( int ) ));
  // **********

  //Spinbox for observation time step
  timelabel= new QLabel( tr("Obs. timestep:"), this );
  timestepspin= new TimeStepSpinbox(this);
  timestepspin->setMinimum(1);
  timestepspin->setValue(1);
  stepchanged(1);
  connect(timestepspin, SIGNAL(valueChanged(int)), SLOT(stepchanged(int)));

  //toggle button for comments dialog
  pausebutton = new ToggleButton( this, tr("Pause").toStdString() );
  connect(  pausebutton, SIGNAL(toggled(bool)),
      SLOT( pauseClicked(bool) ));
  pausebutton->setChecked(false);


  //toggle button for comments dialog
  commentbutton = new ToggleButton( this, tr("Comments").toStdString());
  connect(  commentbutton, SIGNAL(toggled(bool)),
      SLOT( commentClicked(bool) ));

  QHBoxLayout* h2layout = new QHBoxLayout();
  h2layout->addWidget(timelabel);
  h2layout->addWidget(timestepspin);
  h2layout->addWidget(pausebutton);
  h2layout->addWidget(commentbutton);

  editexit = NormalPushButton(tr("Exit"), this );
  connect(  editexit, SIGNAL(clicked()), SLOT( exitClicked() ));

  // qt4 fix: QButton -> QPushButton
  QPushButton* edithide = NormalPushButton(tr("Hide"), this );
  connect( edithide, SIGNAL(clicked()), SIGNAL(EditHide()));

  edithelp = NormalPushButton(tr("Help"), this );
  connect(  edithelp, SIGNAL(clicked()), SLOT( helpClicked() ));

  connect( bgroup, SIGNAL(buttonClicked(int)), SLOT(groupClicked( int ))  );

  prodlabel = new QLabel("" , this);
  prodlabel->setMaximumHeight(40);
  lStatus = new QLabel("", this);

  QVBoxLayout* lvlayout= new QVBoxLayout();
  lvlayout->addWidget(prodlabel);
  lvlayout->addWidget(lStatus);

  QHBoxLayout* hlayout = new QHBoxLayout();

  hlayout->addWidget(editexit);
  hlayout->addWidget(edithide);
  hlayout->addWidget(edithelp);

  // vlayout
  QVBoxLayout* vlayout = new QVBoxLayout( this);
  vlayout->addLayout( lvlayout,1);
  vlayout->addLayout( bgroupLayout );
  vlayout->addWidget( twd );
  vlayout->addLayout( h2layout );
  vlayout->addLayout( hlayout );

  //   vlayout->activate();
  //   vlayout->freeze();

  enew = new EditNewDialog( static_cast<QWidget*>(parent()), m_ctrl );
  enew->hide();
  connect(enew,
      SIGNAL(EditNewOk(EditProduct&, EditProductId&, miutil::miTime&)),
      SLOT(EditNewOk(EditProduct&, EditProductId&, miutil::miTime&)));
  connect(enew,
      SIGNAL(EditNewCombineOk(EditProduct&, EditProductId&, miutil::miTime&)),
      SLOT(EditNewCombineOk(EditProduct&, EditProductId&, miutil::miTime&)));
  connect(enew, SIGNAL(EditNewHelp()), SLOT(helpClicked()));
  connect(enew, SIGNAL(EditNewCancel()), SLOT(EditNewCancel()));
  connect(enew, SIGNAL(newLogin(editDBinfo&)), SLOT(newLogin(editDBinfo&)));

  ecomment = new EditComment( this, m_ctrl,true );
  connect(ecomment,SIGNAL(CommentHide()),SLOT(hideComment()));
  ecomment->hide();

  mb = new QMessageBox(tr("New analysis"),
      tr("This will delete all your edits so far.\n Do you really want them to disappear?"),
      QMessageBox::Warning,
      QMessageBox::Yes | QMessageBox::Default,
      QMessageBox::Cancel | QMessageBox::Escape,
      Qt::NoButton,
      this);
  mb->setButtonText( QMessageBox::Yes, tr("New") );
  mb->setButtonText( QMessageBox::Cancel, tr("Cancel"));


}//end constructor EditDialog



// --------------------------------------------------------------
// --------------- FieldTab methods -----------------------------
// --------------------------------------------------------------

void  EditDialog::FieldTab()
{
  int mymargin=5;
  int myspacing=5;

  fieldtab = new QWidget(twd);


  fgroup = new QButtonGroup( fieldtab );
  fbutton = new QPushButton*[maxfields];
  QHBoxLayout* hLayout = new QHBoxLayout();

  for (int i=0; i<maxfields; i++) {
    fbutton[i]= new QPushButton( "    ", fieldtab );
    int height = fbutton[i]->sizeHint().height();
    fbutton[i]->setMinimumHeight( height );
    fbutton[i]->setMaximumHeight( height );
    fgroup->addButton(fbutton[i],i);
    fbutton[i]->setEnabled(false);
    hLayout->addWidget(fbutton[i]);
  }
  numfields= 0;
  connect( fgroup, SIGNAL(buttonClicked(int)), SLOT(fgroupClicked(int)) );

  m_Fieldeditmethods = new QListWidget(fieldtab);
  m_Fieldeditmethods->setMinimumHeight(HEIGHTLISTBOX);

  connect( m_Fieldeditmethods, SIGNAL( itemClicked ( QListWidgetItem * ) ),
      SLOT( FieldEditMethods(QListWidgetItem * ) ) );

  QButtonGroup* bgroupinfluence= new QButtonGroup(fieldtab);

  rbInfluence[0]= new QRadioButton(QString(tr("Circle")),         fieldtab);
  rbInfluence[1]= new QRadioButton(QString(tr("Square")),         fieldtab);
  rbInfluence[2]= new QRadioButton(QString(tr("Ellipse(centre)")),fieldtab);
  rbInfluence[3]= new QRadioButton(QString(tr("Ellipse(focus)")), fieldtab);
  QVBoxLayout* vLayout = new QVBoxLayout();
  for (int i=0; i<4; i++) {
    bgroupinfluence->addButton(rbInfluence[i],i);
    vLayout->addWidget( rbInfluence[i] );
  }
  bgroupinfluence->setExclusive(TRUE);

  rbInfluence[1]->setEnabled(false);

  connect (bgroupinfluence, SIGNAL(buttonClicked(int)),
      SLOT(changeInfluence(int)));

  // set default (dialog and use)
  rbInfluence[0]->setChecked(true);
  changeInfluence(0); // needed as the above does not change anything


  QLabel* ellipseform = new QLabel(tr("Ellipse shape"),this);

  ellipsenumbers.clear();
  int i;
  for (i=20; i<70; i+=10) ellipsenumbers.push_back(float(i)/100.);
  for (i=70; i<85; i+=5)  ellipsenumbers.push_back(float(i)/100.);
  for (i=85; i<100; i++)  ellipsenumbers.push_back(float(i)/100.);

  ellipsenumber = new QLabel( "    ", this );
  ellipsenumber->setMinimumSize( 50, ellipsenumber->sizeHint().height() +6 );
  ellipsenumber->setMaximumSize( 50, ellipsenumber->sizeHint().height() +6 );
  ellipsenumber->setFrameStyle( QFrame::Box | QFrame::Plain);
  ellipsenumber->setLineWidth(2);
  ellipsenumber->setAlignment( Qt::AlignCenter );

  int n= ellipsenumbers.size()-1;
  int index= (n+1)/2;

  ellipsenumber->setNum( double(ellipsenumbers[index]) );

  ellipseslider  = new QSlider( Qt::Horizontal, this);
  ellipseslider->setMinimum(0);
  ellipseslider->setMaximum(n);
  ellipseslider->setPageStep(1);
  ellipseslider->setValue(index);
  ellipseslider->setMinimumHeight( 16 );
  ellipseslider->setMaximumHeight( 16 );
  ellipseslider->setEnabled( true );

  connect( ellipseslider, SIGNAL( valueChanged( int )),
      SLOT( fieldEllipseChanged( int)));

  connect( ellipseslider, SIGNAL( sliderReleased() ),
      SLOT( fieldEllipseShape()) );

  QHBoxLayout* ehbox = new QHBoxLayout();
  ehbox->setMargin( mymargin );
  ehbox->setSpacing( myspacing/2 );
  ehbox->addWidget(ellipseform);
  ehbox->addWidget(ellipsenumber);
  ehbox->addWidget(ellipseslider);
  // set default
  fieldEllipseShape();

  // enable/disable extra editing lines
  exlineCheckBox= new QCheckBox(tr("Show extra editing lines"), fieldtab);
  exlineCheckBox->setChecked( false );
  exlineCheckBox->setEnabled( true );

  connect( exlineCheckBox, SIGNAL( toggled(bool) ),
      SLOT( exlineCheckBoxToggled(bool) ) );

  // NOT USED YET....
  // QCheckBox* visible = new QCheckBox( "synlig", fieldtab );


  undoFieldButton = NormalPushButton( tr("Undo"), this);
  redoFieldButton = NormalPushButton( tr("Redo"), this);

  undoFieldButton->setEnabled( false );
  redoFieldButton->setEnabled( false );

  connect( undoFieldButton, SIGNAL(clicked()), SLOT(undofield()));
  connect( redoFieldButton, SIGNAL(clicked()), SLOT(redofield()));

  QHBoxLayout* hbox = new QHBoxLayout();
  hbox->setMargin( mymargin );
  hbox->setSpacing( myspacing );
  hbox->addWidget(undoFieldButton);
  hbox->addWidget(redoFieldButton);

  QVBoxLayout* vlayout = new QVBoxLayout( fieldtab);
  vlayout->setMargin( mymargin );
  vlayout->setSpacing( myspacing );
  vlayout->addLayout( hLayout );
  vlayout->addWidget( m_Fieldeditmethods );
  vlayout->addLayout( vLayout );
  vlayout->addLayout( ehbox );
  vlayout->addWidget( exlineCheckBox );
  vlayout->addLayout( hbox );

  twd->addTab( fieldtab, TABNAME_FIELD );
}


void EditDialog::fgroupClicked( int index )
{
  if (index!=fieldIndex) {
    m_editm->activateField(index);
    fieldIndex= index;
  }
}


void EditDialog::undoFieldsEnable()
{
  undoFieldButton->setEnabled(true);
  redoFieldButton->setEnabled(false);
}


void EditDialog::undoFieldsDisable()
{
  undoFieldButton->setEnabled(false);
  redoFieldButton->setEnabled(false);
}


void EditDialog::undofield()
{
  EditEvent ee;
  ee.type= edit_undo;
  ee.order= normal_event;
  ee.x= 0.;
  ee.y= 0.;
  if (!m_editm->notifyEditEvent(ee))
    undoFieldButton->setEnabled( false );
  redoFieldButton->setEnabled( true );
  if (inEdit) emit editUpdate();
}


void EditDialog::redofield()
{
  EditEvent ee;
  ee.type= edit_redo;
  ee.order= normal_event;
  ee.x= 0.;
  ee.y= 0.;
  if(!m_editm->notifyEditEvent(ee))
    redoFieldButton->setEnabled( false );
  undoFieldButton->setEnabled( true );
  if (inEdit) emit editUpdate();
}


void EditDialog::changeInfluence( int index )
{
  EditEvent ee;
  if      (index==0) ee.type= edit_circle;
  else if (index==1) ee.type= edit_square;
  else if (index==2) ee.type= edit_ellipse1;
  else if (index==3) ee.type= edit_ellipse2;
  ee.order= normal_event;
  ee.x= 0.;
  ee.y= 0.;
  m_editm->notifyEditEvent(ee);
  if (inEdit) emit editUpdate();
}


void EditDialog::fieldEllipseChanged( int index )
{
  ellipsenumber->setNum( double(ellipsenumbers[index]) );
}


void EditDialog::fieldEllipseShape()
{
  int index= ellipseslider->value();
  EditEvent ee;
  ee.type= edit_ecellipse;
  ee.order= normal_event;
  ee.x= ellipsenumbers[index];
  ee.y= 0.;
  m_editm->notifyEditEvent(ee);
  if (inEdit) emit editUpdate();
}


void EditDialog::exlineCheckBoxToggled(bool on)
{
  EditEvent ee;
  ee.type= (on) ? edit_exline_on : edit_exline_off;
  ee.order= normal_event;
  ee.x= 0.;
  ee.y= 0.;
  m_editm->notifyEditEvent(ee);
}


void EditDialog::FieldEditMethods( QListWidgetItem * item  )
{
#ifdef DEBUGREDRAW
  cerr<<"EditDialog::FieldEditMethods(index)  index= "<<m_Fieldeditmethods->currentRow()<<endl;
#endif

  if(m_Fieldeditmethods->count()==0) return;

  int index = m_Fieldeditmethods->currentRow();
  if(index<0) {
    m_Fieldeditmethods->setCurrentRow(0);
    index=0;
  }

  int numClassValues= classValues.size();

  if (index<numFieldEditTools) {

    //    currFieldEditToolIndex= index;

    currMapmode= m_EditDI.mapmodeinfo[0].mapmode;
    currEditmode = m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].editmode;
    currEdittool=  m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools[index].name;
    int tool= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools[index].index;

    m_editm->setEditMode(currMapmode,currEditmode,currEdittool);

    EditEvent ee;

    ee.type= editType(tool);
    ee.order= normal_event;
    ee.x= 0.;
    ee.y= 0.;

    m_editm->notifyEditEvent(ee);

  } else if (fieldEditToolGroup==1 &&
      index<numFieldEditTools+numClassValues) {

    //    currFieldEditToolIndex= index;

    int n= index - numFieldEditTools;
    EditEvent ee;

    ee.type= edit_class_value;
    ee.order= normal_event;
    ee.x= classValues[n];
    ee.y= 0.;

    m_editm->notifyEditEvent(ee);

  } else if (fieldEditToolGroup==1 &&
      index<numFieldEditTools+numClassValues*2) {

    int n= index - numFieldEditTools - numClassValues;
    EditEvent ee;

    m_Fieldeditmethods->blockSignals(true);

    //    m_Fieldeditmethods->clearSelection();
    //    m_Fieldeditmethods->setCurrentRow(currFieldEditToolIndex);
    //    m_Fieldeditmethods->item(currFieldEditToolIndex)->setSelected(true);

    if (classValuesLocked[n]) {
      m_Fieldeditmethods->item(index)->setIcon(QIcon(openValuePixmap));
      classValuesLocked[n]= false;
      ee.type= edit_open_value;
    } else {
      m_Fieldeditmethods->item(index)->setIcon(QIcon(lockValuePixmap));
      classValuesLocked[n]= true;
      ee.type= edit_lock_value;
    }

    m_Fieldeditmethods->blockSignals(false);

    ee.order= normal_event;
    ee.x= classValues[n];
    ee.y= 0.;

    m_editm->notifyEditEvent(ee);

  }
}



// --------------------------------------------------------------
// ------------- ObjectTab methods ------------------------------
// --------------------------------------------------------------


void  EditDialog::FrontTab()
{
  int mymargin=5;
  int myspacing=5;

  objecttab = new QWidget(twd );

  vector<miutil::miString> vstr;
  m_Frontcm = ComboBox( objecttab, vstr );
  connect( m_Frontcm, SIGNAL( activated(int) ),
      SLOT( FrontTabBox(int) ) );

  m_Fronteditmethods = new QListWidget(objecttab);

  connect( m_Fronteditmethods, SIGNAL(itemClicked(QListWidgetItem *) ),
      SLOT( FrontEditClicked() ) );
  connect( m_Fronteditmethods, SIGNAL(itemDoubleClicked(QListWidgetItem *) ),
      SLOT( FrontEditDoubleClicked() ) );


  undoFrontButton = NormalPushButton( tr("Undo"), this);
  redoFrontButton = NormalPushButton( tr("Redo"), this);

  connect( undoFrontButton, SIGNAL(clicked()), SLOT(undofront()));
  connect( redoFrontButton, SIGNAL(clicked()), SLOT(redofront()));

  QHBoxLayout* hbox = new QHBoxLayout();
  hbox->setMargin( mymargin );
  hbox->setSpacing( myspacing );
  hbox->addWidget(undoFrontButton);
  hbox->addWidget(redoFrontButton);

  autoJoin = new QCheckBox( tr("Join fronts"), objecttab);
  autoJoin->setChecked(true);
  connect(autoJoin, SIGNAL(toggled(bool)), SLOT(autoJoinToggled(bool)));

  // initialize colours and ok state
  autoJoinToggled(true);
  QVBoxLayout* vlayout = new QVBoxLayout( objecttab);

  vlayout->setMargin( mymargin );
  vlayout->setSpacing( myspacing );
  vlayout->addWidget( m_Frontcm );
  vlayout->addWidget( m_Fronteditmethods );
  vlayout->addWidget( autoJoin );
  vlayout->addLayout( hbox );

  twd->addTab( objecttab, TABNAME_OBJECTS);
}


void  EditDialog::FrontTabBox( int index )
{
  // called when an item in objects combo box selected
  if (index!=m_FrontcmIndex || m_Fronteditmethods->count()==0){
    m_FrontcmIndex=index;
    ListWidgetData( m_Fronteditmethods, 1, index);
    m_FronteditIndex=-1;
    m_Fronteditmethods->item(0)->setSelected(true);
  } else if (m_FronteditIndex < m_Fronteditmethods->count()-1){
    m_Fronteditmethods->item(m_FronteditIndex)->setSelected(true);
  }
  currEditmode= miutil::miString(m_Frontcm->itemText(m_FrontcmIndex).toStdString());
  FrontEditClicked();
  return;
}



void EditDialog::FrontEditClicked()
{
  //  cerr << "FrontEditClicked "  << endl;
  //called when an item in the objects list box clicked
  if (!inEdit || m_Fronteditmethods->count()==0) return;

  int index = m_Fronteditmethods->currentRow();
  if(index<0) {
    m_Fronteditmethods->setCurrentRow(0);
    index=0;
  }

  if (int(m_FronteditList.size())>index) currEdittool= m_FronteditList[index];
  m_editm->setEditMode(currMapmode, currEditmode, currEdittool);
  if (index!=m_FronteditIndex){
    m_FronteditIndex=index;
    if (m_objm->inTextMode()){
      miutil::miString text = m_objm->getCurrentText();
      Colour::ColourInfo colour= m_objm->getCurrentColour();
      if (text.empty()){
        if (getText(text,colour)){
          m_objm->setCurrentText(text);
          m_objm->setCurrentColour(colour);
        }
      }
    }
    else if (m_objm->inComplexTextMode()){
      vector <miutil::miString> symbolText,xText;
      m_objm->initCurrentComplexText();
      m_objm->getCurrentComplexText(symbolText,xText);
      if (getComplexText(symbolText,xText)){
        m_objm->setCurrentComplexText(symbolText,xText);
      }
    } 
    else if (m_objm->inComplexTextColorMode()){
      vector <miutil::miString> symbolText,xText;
      m_objm->initCurrentComplexText();
      m_objm->getCurrentComplexText(symbolText,xText);
      Colour::ColourInfo colour=m_objm->getCurrentColour();
      if (getComplexColoredText(symbolText,xText,colour)){
        m_objm->setCurrentComplexText(symbolText,xText);
        m_objm->setCurrentColour(colour);
      }
  
    }
    else if (m_objm->inEditTextMode()){
      vector <miutil::miString> symbolText,xText;
      m_objm->initCurrentComplexText();
      m_objm->getCurrentComplexText(symbolText,xText);
      if (getEditText(symbolText)){
        m_objm->setCurrentComplexText(symbolText,xText);
      }
    }
  }
  m_objm->createNewObject();
}



void EditDialog::FrontEditDoubleClicked()
{
  //called when am item in the objects list box doubleclicked
  if (m_objm->inTextMode()){
    miutil::miString text = m_objm->getCurrentText();
    Colour::ColourInfo colour=m_objm->getCurrentColour();
    if (getText(text,colour)){
      //change objectmanagers current text !
      m_objm->setCurrentText(text);
      m_objm->setCurrentColour(colour);
    }
  } else if (m_objm->inComplexTextMode()){
    vector <miutil::miString> symbolText,xText;
    m_objm->getCurrentComplexText(symbolText,xText);
    if (getComplexText(symbolText,xText)){
      m_objm->setCurrentComplexText(symbolText,xText);
    }
  
  } else if (m_objm->inComplexTextColorMode()){
    vector <miutil::miString> symbolText,xText;
    m_objm->getCurrentComplexText(symbolText,xText);
    Colour::ColourInfo colour=m_objm->getCurrentColour();
    if (getComplexColoredText(symbolText,xText,colour)){
      m_objm->setCurrentComplexText(symbolText,xText);
      m_objm->setCurrentColour(colour);
    }
  
  } else if (m_objm->inEditTextMode()){
    vector <miutil::miString> symbolText,xText;
    m_objm->getCurrentComplexText(symbolText,xText);
    if (getEditText(symbolText)){
      m_objm->setCurrentComplexText(symbolText,xText);
    }
  }
  //create new object
  if (inEdit)
    m_objm->createNewObject();
}


void EditDialog::undoFrontsEnable()
{
  undoFrontButton->setEnabled(true);
  redoFrontButton->setEnabled(false);
}


void EditDialog::undoFrontsDisable()
{
  undoFrontButton->setEnabled(false);
  redoFrontButton->setEnabled(false);
}


void EditDialog::undofront()
{
  m_editm->showAllObjects();
  if (!m_objm->undofront())
    undoFrontButton->setEnabled( false );
  redoFrontButton->setEnabled( true );
  if (inEdit) emit editUpdate();
}


void EditDialog::redofront()
{
  m_editm->showAllObjects();
  if (!m_objm->redofront())
    redoFrontButton->setEnabled( false );
  undoFrontButton->setEnabled( true );
  if (inEdit) emit editUpdate();
}



void EditDialog::autoJoinToggled(bool on)
{
  m_objm->autoJoinToggled(on);
}



void EditDialog::EditMarkedText()
{
  //called from shortcut ctrl-e
  //changes all marked texts and objectmanagers current text !
  vector <miutil::miString> symbolText,xText,eText;
  miutil::miString text = m_objm->getMarkedText();
  if (!text.empty()){
    //get new text from inputdialog box
    Colour::ColourInfo colour=m_objm->getMarkedColour();
    if (getText(text,colour)){
      m_objm->changeMarkedText(text);
      m_objm->changeMarkedColour(colour);
      m_objm->setCurrentText(text);
      m_objm->setCurrentColour(colour);
    }
  }
  //text from annotations
  text = m_ctrl->getMarkedAnnotation();
  if (!text.empty()){
    eText.push_back(text);
    AnnoText * aText =new AnnoText(this,m_ctrl,m_editm->getProductName(), eText,xText);
    connect(aText,SIGNAL(editUpdate()),SIGNAL(editUpdate()));
    m_ctrl->startEditAnnotation();
    aText->exec();
    delete aText;
  }
  m_objm->getMarkedComplexText(symbolText,xText);
  if (getComplexText(symbolText,xText))
    m_objm->changeMarkedComplexText(symbolText,xText);
}

void EditDialog::DeleteMarkedAnnotation()
{
  m_ctrl->DeleteMarkedAnnotation();
}


bool EditDialog::getText(miutil::miString & text, Colour::ColourInfo & colour)
{
//   cerr << "EditDialog::getText called"  << endl;
  bool ok = false;

  vector <miutil::miString> symbolText,xText;
  symbolText.push_back(text);
  set <miutil::miString> textList=m_objm->getTextList();
  ComplexText * cText =new ComplexText(this,m_ctrl, symbolText,xText,
      textList,true);
  cText->setColour(colour);
  if (cText->exec()){
    cText->getComplexText(symbolText,xText);
    cText->getColour(colour);
    if (symbolText.size())
      text=symbolText[0];
    ok=true;
  }
  delete cText;

  return ok;
}

bool EditDialog::getComplexColoredText(vector <miutil::miString> & symbolText,
    vector <miutil::miString> & xText,Colour::ColourInfo & colour)
{
  cerr << "EditDialog::getComplexColoredText called"  << endl;
  bool ok=false;
  if (symbolText.size()||xText.size()){
    set <miutil::miString> complexList = m_ctrl->getComplexList();
    ComplexText * cText =new ComplexText(this,m_ctrl, symbolText,xText,
        complexList,true);
    cText->setColour(colour);
    if (cText->exec()){
      cText->getComplexText(symbolText,xText);
      cText->getColour(colour);
      ok=true;
    }
    delete cText;
  }
  return ok;
}

bool EditDialog::getComplexText(vector <miutil::miString> & symbolText,
    vector <miutil::miString> & xText)
{
//   cerr << "EditDialog::getComplexText called"  << endl;
  bool ok=false;
  if (symbolText.size()||xText.size()){
    set <miutil::miString> complexList = m_ctrl->getComplexList();
    ComplexText * cText =new ComplexText(this,m_ctrl, symbolText,xText,
        complexList);
    if (cText->exec()){
      cText->getComplexText(symbolText,xText);
      ok=true;
    }
    delete cText;
  }
  return ok;
}

bool EditDialog::getEditText(vector <miutil::miString> & editText)
{
//   cerr << "EditDialog::getEditText called"  << endl;
  bool ok=false;
  if (editText.size()) {
     set <miutil::miString> complexList = m_ctrl->getComplexList();
     //set <miutil::miString> textList=m_objm->getTextList();
     EditText * eText =new EditText(this,m_ctrl, editText, complexList,true);
     if (eText->exec()){
      eText->getEditText(editText);
      ok=true;
    }
    delete eText;
  } 
  return ok;
}


// --------------------------------------------------------------
// ------------- CombineTab methods -----------------------------
// --------------------------------------------------------------

void  EditDialog::CombineTab()
{
  const int mymargin= 5;
  const int myspacing= 5;

  combinetab = new QWidget(twd );

  QButtonGroup* bg= new QButtonGroup(combinetab);
  QRadioButton* rb1= new QRadioButton(tr("Change borders"),this);
  QRadioButton* rb2= new QRadioButton(tr("Set data sources"),this);
  bg->addButton(rb1,0);
  bg->addButton(rb2,1);
  QVBoxLayout* bgLayout = new QVBoxLayout();
  bgLayout->addWidget(rb1);
  bgLayout->addWidget(rb2);
  rb1->setChecked(true);
  connect(bg, SIGNAL(buttonClicked(int)), SLOT(combine_action(int)));

  m_SelectAreas = new QListWidget(combinetab);//listBox( group, 150, 75, false );
  m_SelectAreas->setMinimumHeight(100);

  connect( m_SelectAreas, SIGNAL( itemClicked ( QListWidgetItem *  ) ),
      SLOT( selectAreas(QListWidgetItem * ) ));

  //   QHBoxLayout* hbox = new QHBoxLayout(combinetab);
  //   hbox->setMargin( mymargin );
  //   hbox->setSpacing( myspacing );
  //   combinetab->setStretchFactor(hbox, 20);

  stopCombineButton = new QPushButton( tr("Exit merge"), combinetab);
  connect(stopCombineButton, SIGNAL(clicked()), SLOT(stopCombine()));

  QVBoxLayout* vlayout = new QVBoxLayout( combinetab);
  vlayout->setMargin( mymargin );
  vlayout->setSpacing( myspacing );
  vlayout->addLayout( bgLayout );
  vlayout->addWidget( m_SelectAreas );
  vlayout->addWidget( stopCombineButton );

  twd->addTab( combinetab, TABNAME_COMBINE );
}

void EditDialog::stopCombine()
{
//   cerr << "EditDialog::stopCombine called" << endl;

  twd->setTabEnabled(0, true);
  twd->setTabEnabled(1, true);
  twd->setCurrentIndex(0);
  twd->setTabEnabled(2, false);
  //not possible to save or send until combine stopped
  b[saveb]->setEnabled(true);
#ifdef METNOPRODDB
  b[sendb]->setEnabled(true);
#endif
  m_editm->stopCombine();
}

void EditDialog::combine_action(int idx)
{
  if (idx == combineAction) return;
  combineAction= idx;
  CombineEditMethods();
}

void EditDialog::selectAreas(QListWidgetItem * item )
{
  int index = m_SelectAreas->currentRow();
  miutil::miString tmp= miutil::miString( m_SelectAreas->item(index)->text().toStdString());
  if (tmp !=currEdittool){
    currEdittool= tmp;
    if (inEdit) m_editm->setEditMode(currMapmode, currEditmode, currEdittool);
  }
  if (inEdit) m_objm->createNewObject();
}

void EditDialog::CombineEditMethods()
{

  if (combineAction<0){ // first time
    currEditmode= m_EditDI.mapmodeinfo[2].editmodeinfo[0].editmode;
    currEdittool= m_EditDI.mapmodeinfo[2].editmodeinfo[0].edittools[0].name;
    m_SelectAreas->setEnabled(false);
  } else if (combineAction==0){ // region border editing
    m_SelectAreas->setEnabled(false);
    currEditmode= m_EditDI.mapmodeinfo[2].editmodeinfo[0].editmode;
    currEdittool= m_EditDI.mapmodeinfo[2].editmodeinfo[0].edittools[0].name;
    if (inEdit) m_objm->setAllPassive();
  } else if (combineAction==1){ // region selections
    if(m_SelectAreas->count() > 0) {
      m_SelectAreas->setEnabled(true);
      currEditmode= m_EditDI.mapmodeinfo[2].editmodeinfo[1].editmode;
      if(m_SelectAreas->currentRow()<0) {
        m_SelectAreas->setCurrentRow(0);
      }
      currEdittool= miutil::miString( m_SelectAreas->currentItem()->text().toStdString());
      if (inEdit) m_objm->createNewObject();
    }
  } else {
    cerr << "EditDialog::CombineEditMethods    unknown combineAction:"
    << combineAction << endl;
    return;
  }
  if (inEdit) {
    m_editm->setEditMode(currMapmode, currEditmode, currEdittool);
  }

}

// --------------------------------------------------------------
// ------------- Common methods ---------------------------------
// --------------------------------------------------------------

void EditDialog::tabSelected( int tabindex)
{
#ifdef DEBUGREDRAW
  cerr<<"EditDialog::tabSelected:"<<tabindex<<endl;
#endif
  QString tabname = twd->tabText(tabindex);

  if (tabname == TABNAME_FIELD) {
    //unmark all objects when changing mapMode
    m_objm->editNotMarked();
#ifdef DEBUGREDRAW
    if (!inEdit) cerr<<"EditDialog::tabSelected emit editUpdate()...(1)"<<endl;
#endif
    if (!inEdit) emit editUpdate();
    if (m_EditDI.mapmodeinfo.size()>0){
      currMapmode= m_EditDI.mapmodeinfo[0].mapmode;
      FieldEditMethods(m_Fieldeditmethods->currentItem());
    }
  } else if (tabname == TABNAME_OBJECTS) {
    if (m_EditDI.mapmodeinfo.size()>1){
      currMapmode= m_EditDI.mapmodeinfo[1].mapmode;
      m_Frontcm->setCurrentIndex(m_FrontcmIndex);
      FrontTabBox(m_FrontcmIndex);
    }
  } else if (tabname == TABNAME_COMBINE) {
    if (m_EditDI.mapmodeinfo.size()>2){
      currMapmode= m_EditDI.mapmodeinfo[2].mapmode;
      CombineEditMethods();
    }
  }
  // do a complete redraw - with underlay saving
#ifdef DEBUGREDRAW
  if (inEdit) cerr<<"EditDialog::tabSelected emit editUpdate()...(2)"<<endl;
#endif
  if (inEdit) emit editUpdate();
}


void  EditDialog::ListWidgetData( QListWidget* list, int mindex, int index)
{

  list->clear();
  vector<miutil::miString> vstr;
  int n= m_EditDI.mapmodeinfo[mindex].editmodeinfo[index].edittools.size();
  list->setViewMode(QListView::ListMode);
  for ( int i=0; i<n; i++){
    miutil::miString etool=m_EditDI.mapmodeinfo[mindex].editmodeinfo[index].edittools[i].name;
#ifdef DEBUGPRINT
  if (inEdit) cerr<<"ListWidgetData etool = "<< etool <<endl;
#endif
    vstr.push_back(etool);
    QString dialog_etool;
    //find translation
    if (editTranslations.count(etool))
      dialog_etool =editTranslations[etool];
    else
      dialog_etool = etool.cStr();
    list->addItem(QString(dialog_etool));
  }

  list->setCurrentItem(0);

  if (mindex==OBJECT_INDEX)
    m_FronteditList=vstr; //list of edit tools

  if (mindex==OBJECT_INDEX && index==SIGMAP_INDEX){
    //for now, only sigmap symbols have images...
    list->clear();
    list->setViewMode(QListView::IconMode);
    SetupParser sp;
    for ( int i=0; i<n; i++){
      miutil::miString path = sp.basicValue("imagepath");
      miutil::miString filename = path+ m_FronteditList[i] + ".png";
      QPixmap pmap(filename.c_str());
      if(!pmap.isNull()){
        QListWidgetItem* item = new QListWidgetItem(QIcon(pmap),QString());
        list->addItem(item);
      } else {
        list->addItem(QString(m_FronteditList[i].c_str()));
      }
    }
  }

  return;
}


void EditDialog::ComboBoxData(QComboBox* box, int mindex)
{
  int n= m_EditDI.mapmodeinfo[mindex].editmodeinfo.size();
  vector<miutil::miString> vstr;
  m_Frontcm->clear();
  for( int i=0; i<n; i++ ){
    if (m_EditDI.mapmodeinfo[mindex].editmodeinfo[i].edittools.size()){
      m_Frontcm->addItem(QString(m_EditDI.mapmodeinfo[1].editmodeinfo[i].editmode.cStr()));
    }
  }

}


bool EditDialog::saveEverything(bool send)
{
  bool approved= false;

  if (send){
    switch(QMessageBox::information(this, tr("Send product"),
        tr("Start distribution of product to all regions.\n Use \"Approve produkt\" to give product official status\n as approved and ready."),
        tr("&Distribution only"), tr("&Approve product"), tr("&Cancel"),
        0,      // Enter == button 0
        2 ) ) { // Escape == button 2


    case 0: // "Kun distribusjon" clicked, Enter pressed.
      approved= false;
      break;
    case 1: // "Godkjenn produkt" clicked
      approved= true;
      break;
    case 2: // Cancel clicked, Escape pressed
      return false;
      break;
    }
  }

  ecomment->saveComment();
  miutil::miString message;
  bool res = m_editm->writeEditProduct(message,true,true,send,approved);

  if (!res){
    message= miutil::miString(tr("Problem saving/sending product\n").toStdString()) +
    miutil::miString(tr("Message from server:\n").toStdString())
    + message;
    QMessageBox::warning( this, tr("Save error:"),
        message.c_str());

    return false;
  }

  miutil::miTime t= miutil::miTime::nowTime();
  QString lcs(send ? " <font color=\"darkgreen\">"+tr("Saved") +"</font> "
      : " <font color=\"black\">"+tr("saved")+"</font> ");
  QString tcs= QString("<font color=\"black\">")+
  QString(t.isoTime().cStr()) + QString("</font> ");

  QString qs= lcs + tcs;

  if (send && approved){
    productApproved= true;
    qs += " <font color=\"darkgreen\">"+ tr("and approved") +"</font> ";
  } else if (productApproved){
    qs += " <font color=\"red\">"+ tr("(approved)") + "</font> ";
  }

  lStatus->setText(qs);

  return true;
}


void  EditDialog::groupClicked( int id )
{
  switch (id){
  case 0:
    // start new product
    if (m_editm->unsavedEditChanges()){
      switch( mb->exec() ) { // ask if discard changes
      case QMessageBox::Cancel:
        return;
        break;
      }
    }
    // get productdefinitions etc. from Controller
    //if new-product-dialog already active, do nothing
    if (!enew->newActive)      {
      enew->load(dbi);
      // show start-new-product dialog
      enew->show();
    }
    break;
  case 1:
    // Save all
    saveEverything(false);
    // show all objects if any hidden
    if (m_editm->showAllObjects()) emit editUpdate();
    break;

  case 2:
    // Send all
    saveEverything(true);
    // show all objects if any hidden
    if (m_editm->showAllObjects()) emit editUpdate();
    break;
  };
}


void  EditDialog::stepchanged(int step)
{
  m_ctrl->obsStepChanged(step);
}

void  EditDialog::commentClicked( bool on )
{
  if (inEdit){
    if (on){
      ecomment->show();
    }
    else{
      ecomment->hide();
    }
  }
  else
    //comment button should not be shown
    commentbutton->setChecked(false);
}

void  EditDialog::pauseClicked( bool on )
{
  if (inEdit){
    m_editm->setEditPause(on);
    emit editMode(!on);
  }
}



void EditDialog::showAll()
{
  if (inEdit){
    //show this dialog
    this->show();
    if(commentbutton ->isChecked() )
      ecomment->show();
  } else{
    //load and start EditNewDialog
    enew->load(dbi);
    enew->show();
  }
}


void EditDialog::hideAll()
{
  this->hide();
  enew->hide();
  if( commentbutton->isChecked() )
    ecomment->hide();
}


bool EditDialog::okToExit()
{
  //save comments to plotm->editObjects struct
  ecomment->saveComment();
  if (m_editm->unsavedEditChanges()){
    raise(); //put dialog on top

    switch(QMessageBox::information(this,tr("Exit editing"),
        tr("You have unsaved edits.\n Save before exiting?"),
        tr("&Save"), tr("&Don't save"), tr("&Cancel"),
        0,2)){


    case 0: // save clicked
      saveEverything(false);
      break;
    case 1: // don't save, but exit
      break;
    case 2:
      return false; // cancel operation
      break;
    }
  }
  if(m_editm->unsentEditChanges()){
    raise(); //put dialog on top
    switch(QMessageBox::information(this,tr("Send analysis"),
        tr("Send last saved analysis to the database?"),
        tr("&Send"), tr("&Don't send"),0,1)){
    case 0: // send clicked
      saveEverything(true);
      break;
    case 1: // don't send, but exit
      break;
    }
  }
  return true;
}


bool EditDialog::cleanupForExit()
{
  if (!okToExit()) return false;
  ecomment->stopComment();
  m_editm->stopEdit();
  m_editm->logoutDatabase(dbi);

  return true;
}


void EditDialog::exitClicked()
{
#ifdef DEBUGREDRAW
  cerr<<"EditDialog::exitClicked...................."<<endl;
#endif
  if (!cleanupForExit()) return;
  commentbutton->setChecked(false);
  // update field dialog
  emit emitFieldEditUpdate("");
  m_editm->setEditMode("normal_mode","","");
  twd->setEnabled(false); // disable tab-widget
  b[saveb]->setEnabled(false);
#ifdef METNOPRODDB
  b[sendb]->setEnabled(false);
#endif
  emit EditHide();
  emit editApply();
  // empty timeslider producttime
  vector<miutil::miTime> noTimes;
  emit emitTimes("product",noTimes);
  inEdit= false;
  productApproved= false;
  enew->newActive=false;
  // set labels
  updateLabels();
  lStatus->setText(" ");
  emit editMode(false);
}


void EditDialog::helpClicked()
{
  emit showsource("ug_editdialogue.html");
}


void EditDialog::updateLabels()
{
  // update top-labels etc.
  miutil::miString s;
  if (inEdit)
    s= miutil::miString("<font color=\"darkgreen\">") +
    currprod.name + miutil::miString("</font>") +
    miutil::miString("<font color=\"blue\"> ") +
    currid.name + miutil::miString("</font>") +
    miutil::miString(" ") + prodtime.format("%D %H:%M");
  else
    s= "";

  prodlabel->setText(s.cStr());
}

void EditDialog::newLogin(editDBinfo& d)
{
  dbi= d;
  updateLabels();
}


void EditDialog::EditNewOk(EditProduct& ep,
    EditProductId& ci,
    miutil::miTime& time)
{
//   cerr << "EditDialog::EditNewOk called................" << endl;

  emit editMode(true);

  // Turn off Undo-buttons
  undoFrontsEnable();
  undoFieldsDisable();
  // stop current edit
  m_editm->stopEdit();
  inEdit= false;
  productApproved= false;
  updateLabels();

  // the fields to edit
  numfields= ep.fields.size();
  for (int i=0; i<numfields; i++) {
    fbutton[i]->setText(ep.fields[i].name.c_str());
    fbutton[i]->setEnabled(true);
  }
  for (int i=numfields; i<maxfields; i++) {
    fbutton[i]->setText("    ");
    fbutton[i]->setEnabled(false);
  }
  // force field activate if last selected field was
  // the first one..
  if (fieldIndex==0) fieldIndex= -1;
  fgroupClicked( 0 );

  // update field dialog
#ifdef DEBUGREDRAW
  cerr<<"EditDialog::EditNewOk emit emitFieldEditUpdate(empty)"<<endl;
#endif
  emit emitFieldEditUpdate("");

  if (!m_editm->startEdit(ep,ci,time)) {
    cerr << "Error starting edit" << endl;
    emit editApply();
    return;
  }

  currprod= ep;
  currid= ci;
  prodtime= time;
  inEdit= true;
  // update Product and Id label..
  updateLabels();

  m_EditDI=m_editm->getEditDialogInfo();

  fieldEditToolGroup= 0;
  if (currprod.fields.size()>0) {
    // BUG YET: we only use the tool for the first field if more than one field!!!
    //  to be fixed later...
    if      (currprod.fields[0].editTools[0]=="standard") fieldEditToolGroup=0;
    else if (currprod.fields[0].editTools[0]=="classes")  fieldEditToolGroup=1;
    else if (currprod.fields[0].editTools[0]=="numbers")  fieldEditToolGroup=2;
  }

  if (fieldEditToolGroup==0) {
    rbInfluence[0]->setEnabled(true);
    rbInfluence[1]->setEnabled(false);
    rbInfluence[2]->setEnabled(true);
    rbInfluence[3]->setEnabled(true);
  } else if (fieldEditToolGroup==1) {
    rbInfluence[0]->setEnabled(true);
    rbInfluence[1]->setEnabled(true);
    rbInfluence[2]->setEnabled(false);
    rbInfluence[3]->setEnabled(false);
  } else if (fieldEditToolGroup==2) {
    rbInfluence[0]->setEnabled(true);
    rbInfluence[1]->setEnabled(false);
    rbInfluence[2]->setEnabled(true);
    rbInfluence[3]->setEnabled(true);
  }

  numFieldEditTools= 0;  // don't know yet!
  classNames.clear();
  classValues.clear();
  classValuesLocked.clear();

  if (fieldEditToolGroup!=1) {

    //Fill field edit listbox
    ListWidgetData( m_Fieldeditmethods, 0, fieldEditToolGroup);

    numFieldEditTools= m_Fieldeditmethods->count();

  } else if (fieldEditToolGroup==1) {

    m_Fieldeditmethods->blockSignals(true);

    m_Fieldeditmethods->clear();

    int n= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools.size();

    for (int i=0; i<n; i++) {
      miutil::miString ts= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools[i].name;
      m_Fieldeditmethods->addItem(QString(ts.cStr()));
    }

    numFieldEditTools= n;

    miutil::miString str= m_ctrl->getFieldClassSpecifications(currprod.fields[0].name);

    vector<miutil::miString> vclass= str.split(',');
    for (unsigned int i=0; i<vclass.size(); i++) {
      vector<miutil::miString> vs= vclass[i].split(":");
      if (vs.size()>=2) {
        classNames.push_back(vs[1]);
        classValues.push_back(atof(vs[0].cStr()));
        classValuesLocked.push_back(false);
      }
    }
    classNames.push_back(tr("Undefined").toStdString());
    classValues.push_back(1.e+35);        // the fieldUndef value
    classValuesLocked.push_back(false);

    for (unsigned int i=0; i<classNames.size(); i++) {
      miutil::miString estr= tr("New value:").toStdString() +  classNames[i];
      m_Fieldeditmethods->addItem(QString(estr.cStr()));
    }

    for (unsigned int i=0; i<classNames.size(); i++) {
      QListWidgetItem* item
      = new QListWidgetItem(QIcon(openValuePixmap),QString(classNames[i].cStr()));
      m_Fieldeditmethods->addItem(item);
    }

    m_Fieldeditmethods->blockSignals(false);
    //     currFieldEditToolIndex=0;
    //     m_Fieldeditmethods->setCurrentRow(currFieldEditToolIndex);

    //    m_FieldA//OBSeditmethods->triggerUpdate(true);
  }

  //########################################################################
  EditEvent ee;
  if (fieldEditToolGroup==2)
    ee.type= edit_show_numbers_on;
  else
    ee.type= edit_show_numbers_off;
  ee.order= normal_event;
  ee.x= 0.;
  ee.y= 0.;
  m_editm->notifyEditEvent(ee);
  //########################################################################

  //Fill object edit combobox
  ComboBoxData(m_Frontcm,1);
  //Clear object edit listbox and set indices to zero
  m_Fronteditmethods->clear();
  m_FrontcmIndex=0;
  m_FronteditIndex=-1;

  //find out which tabs to enable and current tab
  int mm;
  if (numfields >0)
    mm=0;
  else
    mm=1;
  twd->setEnabled(true); // enable tab-widget
  if (mm==0){
    twd->setTabEnabled(0, true);
  } else{
    twd->setTabEnabled(0, false);
    FrontTabBox(0);
  }
  twd->setTabEnabled(1, currprod.objectsFilenamePart.exists() );

  twd->setTabEnabled(2, false);
  if (twd->currentIndex()!=mm) twd->setCurrentIndex(mm);

  b[saveb]->setEnabled(true);
#ifdef METNOPRODDB
  b[sendb]->setEnabled(true);
#endif

  commentbutton->setEnabled(currprod.commentFilenamePart.exists() );

  lStatus->setText(tr("Not saved"));
  // set timeslider producttime
  miutil::miTime t;
  m_editm->getProductTime(t);
  vector<miutil::miTime> Times;
  Times.push_back(t);
#ifdef DEBUGREDRAW
  cerr<<"EditDialog::EditNewOk emit emitTimes(product): "<<Times[0]<<endl;
#endif
  emit emitTimes("product",Times);

  // update field dialog
  for (unsigned int i=0; i<currprod.fields.size(); i++){
    if (currprod.fields[i].fromfield){
      // this will remove the original field in the field dialog
#ifdef DEBUGREDRAW
      cerr<<"EditDialog::EditNewOk emit emitFieldEditUpdate"<<endl;
#endif
      emit emitFieldEditUpdate(currprod.fields[i].fromfname);
    } else {
      // add a new selected field in the field dialog
#ifdef DEBUGREDRAW
      cerr<<"EditDialog::EditNewOk emit emitFieldEditUpdate...new"<<endl;
#endif
      emit emitFieldEditUpdate(currprod.fields[i].name);
    }
  }

  this->show();
  //qt4 fix
  tabSelected(twd->currentIndex());

  ecomment->stopComment();
  ecomment->startComment();
  pausebutton->setChecked(false);

  if (ep.OKstrings.size()){
#ifdef DEBUGREDRAW
    cerr<<"EditDialog::EditNewOk emit Apply(ep.OKstrings)"<<endl;
#endif
    //apply commands for this EditProduct (probably MAP)
    m_ctrl->keepCurrentArea(false); // unset area conservatism
    emit Apply(ep.OKstrings,false);
    m_ctrl->keepCurrentArea(true); // reset area conservatism
  } else {
    //  m_ctrl->keepCurrentArea(true); // reset area conservatism
#ifdef DEBUGREDRAW
    cerr<<"EditDialog::EditNewOk emit editApply()"<<endl;
#endif
    emit editApply();
    //  m_ctrl->keepCurrentArea(false); // reset area conservatism
  }

#ifdef DEBUGREDRAW
  cerr<<"REMOVED EditDialog::EditNewOk emit editUpdate()"<<endl;
#endif
  //emit editUpdate();

#ifdef DEBUGREDRAW
  cerr << "EditDialog::EditNewOk finished...................." << endl;
#endif
}


void EditDialog::EditNewCombineOk(EditProduct& ep,
    EditProductId& ci,
    miutil::miTime& time)
{
//   cerr << "EditNewCombineOK" << endl;
  // Turn off Undo-buttons
  undoFrontsEnable();
  undoFieldsDisable();
  // stop current edit
  m_editm->stopEdit();
  inEdit= false;
  productApproved= false;
  updateLabels();

  // start combine+edit without autojoin
  autoJoin->setChecked(false);
  autoJoinToggled(false);

  // the fields to edit when ending combinations
  numfields= ep.fields.size();
  for (int i=0; i<numfields; i++) {
    fbutton[i]->setText(ep.fields[i].name.c_str());
    fbutton[i]->setEnabled(true);
  }
  for (int i=numfields; i<maxfields; i++) {
    fbutton[i]->setText("    ");
    fbutton[i]->setEnabled(false);
  }
  // force field activate if last selected field was
  // the first one..
  if (fieldIndex==0) fieldIndex= -1;
  fgroupClicked( 0 );

  // update field dialog
  emit emitFieldEditUpdate("");

  vector<miutil::miString> combids;
  // try to start combine
  if (!m_editm->startCombineEdit(ep,ci,time,combids)){
    cerr << "Error starting combine" << endl;
    emit editApply();
    return;
  }

  // put combids into select-areas listbox
  m_SelectAreas->clear();
  for (unsigned int i=0; i<combids.size(); i++){
    m_SelectAreas->addItem(QString(combids[i].cStr()));
  }
  if(combids.size()) m_SelectAreas->setCurrentItem(0);

  inEdit= true;
  currprod= ep;
  currid= ci;
  prodtime= time;
  // update label etc..
  updateLabels();

  m_EditDI=m_editm->getEditDialogInfo();

  fieldEditToolGroup= 0;
  if (currprod.fields.size()>0) {
    // BUG YET: we only use the tool for the first field if more than one field!!!
    //  to be fixed later...
    if      (currprod.fields[0].editTools[0]=="standard") fieldEditToolGroup=0;
    else if (currprod.fields[0].editTools[0]=="classes")  fieldEditToolGroup=1;
    else if (currprod.fields[0].editTools[0]=="numbers")  fieldEditToolGroup=2;
  }

  //ListBoxData( m_Fieldeditmethods, 0, fieldEditToolGroup);

  if (fieldEditToolGroup==0) {
    rbInfluence[0]->setEnabled(true);
    rbInfluence[1]->setEnabled(false);
    rbInfluence[2]->setEnabled(true);
    rbInfluence[3]->setEnabled(true);
  } else if (fieldEditToolGroup==1) {
    rbInfluence[0]->setEnabled(true);
    rbInfluence[1]->setEnabled(true);
    rbInfluence[2]->setEnabled(false);
    rbInfluence[3]->setEnabled(false);
  } else if (fieldEditToolGroup==2) {
    rbInfluence[0]->setEnabled(true);
    rbInfluence[1]->setEnabled(false);
    rbInfluence[2]->setEnabled(true);
    rbInfluence[3]->setEnabled(true);
  }

  numFieldEditTools= 0;  // don't know yet!
  classNames.clear();
  classValues.clear();
  classValuesLocked.clear();

  if (fieldEditToolGroup!=1) {

    //Fill field edit listbox
    ListWidgetData( m_Fieldeditmethods, 0, fieldEditToolGroup);

    numFieldEditTools= m_Fieldeditmethods->count();

  } else if (fieldEditToolGroup==1) {

    m_Fieldeditmethods->blockSignals(true);

    m_Fieldeditmethods->clear();

    int n= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools.size();

    for (int i=0; i<n; i++) {
      miutil::miString ts= m_EditDI.mapmodeinfo[0].editmodeinfo[fieldEditToolGroup].edittools[i].name;
      m_Fieldeditmethods->addItem(QString(ts.cStr()));
    }

    numFieldEditTools= n;

    miutil::miString str= m_ctrl->getFieldClassSpecifications(currprod.fields[0].name);

    vector<miutil::miString> vclass= str.split(',');
    for (unsigned int i=0; i<vclass.size(); i++) {
      vector<miutil::miString> vs= vclass[i].split(":");
      if (vs.size()>=2) {
        classNames.push_back(vs[1]);
        classValues.push_back(atof(vs[0].cStr()));
        classValuesLocked.push_back(false);
      }
    }
    classNames.push_back(tr("Undefined").toStdString());
    classValues.push_back(1.e+35);        // the fieldUndef value
    classValuesLocked.push_back(false);

    for (unsigned int i=0; i<classNames.size(); i++) {
      miutil::miString estr= tr("New value:").toStdString() +  classNames[i];
      m_Fieldeditmethods->addItem(QString(estr.cStr()));
    }

    for (unsigned int i=0; i<classNames.size(); i++) {
      QListWidgetItem* item
      = new QListWidgetItem(QIcon(openValuePixmap),QString(classNames[i].cStr()));
      m_Fieldeditmethods->addItem(item);
    }

    m_Fieldeditmethods->blockSignals(false);
    //     currFieldEditToolIndex=0;
    //     m_Fieldeditmethods->setCurrentRow(currFieldEditToolIndex);

    //OBS    m_Fieldeditmethods->triggerUpdate(true);
  }


  //Fill object edit combobox
  ComboBoxData(m_Frontcm,1);
  //Clear object edit listbox and set indices to zero
  m_Fronteditmethods->clear();
  m_FrontcmIndex=0;
  m_FronteditIndex=-1;

  twd->setEnabled(true); // enable tab-widget
  twd->setTabEnabled(0, false);
  twd->setTabEnabled(1, false);
  twd->setTabEnabled(2, true);
  // switch to combine tab - will automatically set correct
  // currEditmode, currEdittool
  if (twd->currentIndex()!=2) twd->setCurrentIndex(2);

  m_editm->setEditMode(currMapmode, currEditmode, currEdittool);

  lStatus->setText(tr("Not saved"));
  // set timeslider producttime
  miutil::miTime t;
  if (m_editm->getProductTime(t)){
    vector<miutil::miTime> Times;
    Times.push_back(t);
    emit emitTimes("product",Times);
    // update field dialog
    for (unsigned int i=0; i<currprod.fields.size(); i++){
      // add a new selected field in the field dialog
      emit emitFieldEditUpdate(currprod.fields[i].name);
    }
  } else {
    cerr << "Controller returned no producttime" << endl;
  }

  m_editm->editCombine();

  this->show();
  //qt4 fix
  tabSelected(twd->currentIndex());

  pausebutton->setChecked(false);
  ecomment->stopComment();
  ecomment->startComment();
  if (ep.OKstrings.size())
    emit Apply(ep.OKstrings,false);
  else
    emit editApply();
  //emit editUpdate();
}

void EditDialog::EditNewCancel()
{
}


void EditDialog::closeEvent( QCloseEvent* e)
{
  emit EditHide();
}

void EditDialog::hideComment()
{
  commentbutton->setChecked(false);
  ecomment->hide();
}


void EditDialog::undoEdit()
{
  //called from shortcut ctrl-z in main window
  if (!inEdit) return;

  if (currMapmode == m_EditDI.mapmodeinfo[0].mapmode){
    //in field edit
    undofield();
  } else if (currMapmode == m_EditDI.mapmodeinfo[1].mapmode){
    //in objects edit
    undofront();
  } else if (currMapmode == m_EditDI.mapmodeinfo[2].mapmode){
    //in combine edit, not possible to undo
  }
}


void EditDialog::redoEdit()
{
  //called from shortcut ctrl-y in main window
  if (!inEdit) return;

  if (currMapmode == m_EditDI.mapmodeinfo[0].mapmode){
    //in field edit
    redofield();
  } else if (currMapmode == m_EditDI.mapmodeinfo[1].mapmode){
    //in objects edit
    redofront();
  } else if (currMapmode == m_EditDI.mapmodeinfo[2].mapmode){
    //in combine edit, not possible to do
  }
}


void EditDialog::saveEdit()
{
  //called from shortcut ctrl-s in main window
  if (inEdit) saveEverything(false);
}

bool EditDialog::inedit()
{
  return (inEdit && !m_editm->getEditPause());
}