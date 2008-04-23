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

#include <qtQuickAdmin.h>
#include <qtQuickEditOptions.h>

#include <QPushButton>
#include <QTreeWidget>
#include <QLabel>
#include <QFrame>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextEdit>
#include <QRegExp>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QVBoxLayout>


#include <up12x12.xpm>
#include <down12x12.xpm>
#include <filenew.xpm>
#include <fileopen.xpm>
#include <editcopy.xpm>
#include <editcut.xpm>
#include <editpaste.xpm>

#include <qtUtility.h>


class QuickTreeWidgetItem : public QTreeWidgetItem {
private:
  int menu;
  int item;
public:
  QuickTreeWidgetItem(QTreeWidget * parent, QStringList t, int m, int i):
    QTreeWidgetItem(parent, t), menu(m), item(i)
  {}
  QuickTreeWidgetItem(QTreeWidgetItem * parent, QStringList t, int m, int i):
    QTreeWidgetItem(parent, t) , menu(m), item(i)
  {}
//   QuickTreeWidgetItem(QTreeWidget * parent, QTreeWidgetItem * after,
// 		    QStringList t, int m, int i):
//     QTreeWidgetItem(parent, after, t), menu(m), item(i)
//   {}
//   QuickTreeWidgetItem(QTreeWidgetItem * parent, QTreeWidgetItem * after,
// 		    QStringList t, int m, int i):
//     QTreeWidgetItem(parent, after, t), menu(m), item(i)
//   {}

  int Menu() const {return menu;}
  int Item() const {return item;}
};



QuickAdmin::QuickAdmin(QWidget* parent,
		       vector<quickMenu>& qm,
		       int fc, int lc)
  : QDialog(parent),
    menues(qm),firstcustom(fc), lastcustom(lc),
    activeMenu(-1),activeElement(-1),
    copyMenu(-1),copyElement(-1), autochange(true)
{
  QFont m_font= QFont( "Helvetica", 12, 75 );

  QLabel* mainlabel= new QLabel("<em><b>"+tr("Edit quickmenus")+"</b></em>", this);
  mainlabel->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

  menutree= new QTreeWidget(this);
  menutree->setRootIsDecorated(true);
  menutree->setSortingEnabled(false);
  menutree->setColumnCount(1);

  connect(menutree, SIGNAL(itemClicked(QTreeWidgetItem * ,int)),
	  this, SLOT(selectionChanged(QTreeWidgetItem * ,int)));

  // up
  QPixmap upPicture = QPixmap(up12x12_xpm);
  upButton = PixmapButton( upPicture, this, 14, 12 );
  upButton->setEnabled( false );
  upButton->setAccel(Qt::CTRL+Qt::Key_Up);
  connect( upButton, SIGNAL(clicked()), SLOT(upClicked()));
  // down
  QPixmap downPicture = QPixmap(down12x12_xpm);
  downButton = PixmapButton( downPicture, this, 14, 12 );
  downButton->setEnabled( false );
  downButton->setAccel(Qt::CTRL+Qt::Key_Down);
  connect( downButton, SIGNAL(clicked()), SLOT(downClicked()));

  // Command buttons for menu-elements
  // new
  newButton = new QPushButton(QPixmap(filenew_xpm), tr("&New"), this );
  newButton->setEnabled( false );
  connect( newButton, SIGNAL(clicked()), SLOT(newClicked()));
  
  // new from file
  newfileButton = new QPushButton(QPixmap(fileopen_xpm), tr("New from &file.."),
				  this );
  newfileButton->setEnabled( false );
  connect( newfileButton, SIGNAL(clicked()), SLOT(newfileClicked()));
  
  // rename
  renameButton = new QPushButton( tr("&Change name.."), this );
  renameButton->setEnabled( false );
  connect( renameButton, SIGNAL(clicked()), SLOT(renameClicked()));
  
  // erase
  eraseButton = new QPushButton(QPixmap(editcut_xpm), tr("Remove"), this );
  eraseButton->setEnabled( false );
  eraseButton->setAccel(Qt::CTRL+Qt::Key_X);
  connect( eraseButton, SIGNAL(clicked()), SLOT(eraseClicked()));
  
  // copy
  copyButton = new QPushButton(QPixmap(editcopy_xpm), tr("Copy"), this );
  copyButton->setEnabled( false );
  copyButton->setAccel(Qt::CTRL+Qt::Key_C);
  connect( copyButton, SIGNAL(clicked()), SLOT(copyClicked()));
  
  // paste
  pasteButton = new QPushButton(QPixmap(editpaste_xpm), tr("Paste"), this );
  pasteButton->setEnabled( false );
  pasteButton->setAccel(Qt::CTRL+Qt::Key_V);
  connect( pasteButton, SIGNAL(clicked()), SLOT(pasteClicked()));
  
  // a horizontal frame line
  QFrame* line = new QFrame( this );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  // create commands-area
  comedit= new QTextEdit(this, "comedit");
  comedit->setLineWrapMode(QTextEdit::NoWrap);
  comedit->setFont(QFont("Courier",12,QFont::Normal));
  comedit->setReadOnly(false);
  comedit->setMaximumHeight(150);
  connect(comedit, SIGNAL(textChanged()), SLOT(comChanged()));
  // qt4 fix: QFrame -> QFrame
  QFrame* comlabel= new QLabel(comedit,tr("Command field"),this,"comlabel");
  comlabel->setMinimumSize(comlabel->sizeHint());
  //comlabel->setAlignment(AlignBottom | AlignLeft);

  // edit options
  optionButton = new QPushButton(tr("&Dynamic options.."), this );
  optionButton->setEnabled( false );
  connect( optionButton, SIGNAL(clicked()), SLOT(optionClicked()));
  
  // a horizontal frame line
  QFrame* line2 = new QFrame( this );
  line2->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  // last row of buttons
  QPushButton* ok= new QPushButton( tr("&OK"), this );
  QPushButton* cancel= new QPushButton( tr("&Cancel"), this );
  //QPushButton* help= new QPushButton( tr("&Help"), this );
  connect( ok, SIGNAL(clicked()), SLOT(accept()) );
  connect( cancel, SIGNAL(clicked()), SLOT(reject()) );
  //connect( help, SIGNAL(clicked()), SLOT(helpClicked()) );


  QBoxLayout* vl1= new QVBoxLayout();
  vl1->addWidget(upButton);
  vl1->addWidget(downButton);

  QHBoxLayout* hl1= new QHBoxLayout();
  hl1->addWidget(menutree);
  hl1->addLayout(vl1);
  
  QGridLayout* gl= new QGridLayout(2,3,5);
  gl->addWidget(newButton,0,0);
  gl->addWidget(newfileButton,0,1);
  gl->addWidget(renameButton,0,2);
  gl->addWidget(eraseButton,1,0);
  gl->addWidget(copyButton,1,1);
  gl->addWidget(pasteButton,1,2);

  QHBoxLayout* hl= new QHBoxLayout(5);
  hl->addWidget(optionButton);
  hl->addStretch(1);

  QHBoxLayout* hl4= new QHBoxLayout(5);
  hl4->addStretch();
  hl4->addWidget(ok);
  hl4->addStretch();
  hl4->addWidget(cancel);
  hl4->addStretch();
  //hl4->addWidget(help);

  // top layout
  QVBoxLayout* vlayout=new QVBoxLayout(this,5,5);
  
  vlayout->addWidget(mainlabel);
  vlayout->addLayout(hl1);
  vlayout->addLayout(gl);
  vlayout->addWidget(line);
  vlayout->addWidget(comlabel);
  vlayout->addWidget(comedit);
  vlayout->addLayout(hl);
  vlayout->addWidget(line2);
  vlayout->addLayout(hl4);
  
  vlayout->activate();
  
  updateWidgets();
  resize(500,550);
}

void QuickAdmin::selectionChanged(QTreeWidgetItem *p ,int i)
{
//   cerr <<"selectionChanged()"<<endl;
//   QTreeWidgetItem * p = menutree->currentItem();
//   cerr <<"columnCount:"<<p->columnCount()<<endl;
//   if(p->columnCount()>0) cerr <<p->text(0).toStdString()<<endl;
//   QList<QTreeWidgetItem *> lq = menutree->selectedItems();
//   for(int i=0;i<lq.count();i++)
//     cerr <<i<<"  "<<lq[i]->text(0).toStdString()<<endl;
 if (p){
    QuickTreeWidgetItem* qp= (QuickTreeWidgetItem*)(p);
    activeMenu= qp->Menu();
    activeElement= qp->Item();

    if (activeElement==-1){
      newButton->setText(tr("&New menu.."));
      copyButton->setText(tr("Copy menu"));
      eraseButton->setText(tr("Remove menu.."));
      copyButton->setAccel(Qt::CTRL+Qt::Key_C);
      eraseButton->setAccel(Qt::CTRL+Qt::Key_X);
    } else {
      newButton->setText(tr("&New plot.."));
      copyButton->setText(tr("Copy plot"));
      eraseButton->setText(tr("Remove plot"));
      copyButton->setAccel(Qt::CTRL+Qt::Key_C);
      eraseButton->setAccel(Qt::CTRL+Qt::Key_X);
    }
    updateCommand();

    int nitems= menues[activeMenu].menuitems.size();

    if (activeMenu >= firstcustom && activeMenu <= lastcustom){
      // up
      if ((activeElement>0) ||
	  (activeElement==-1 && activeMenu>firstcustom))
	upButton->setEnabled(true);
      else
	upButton->setEnabled(false);
      // down
      if ((activeElement>=0 && activeElement <nitems-1) ||
	  (activeElement==-1 && activeMenu<lastcustom))	  
	downButton->setEnabled(true);
      else
	downButton->setEnabled(false);
      // new, erase, copy, rename
      newButton->setEnabled(true);
      newfileButton->setEnabled(true);
      renameButton->setEnabled(true);
      eraseButton->setEnabled(true);
      copyButton->setEnabled(true);
      // paste
      if (copyMenu!=-1)
	pasteButton->setEnabled(true);
      else
	pasteButton->setEnabled(false);
      // edit options
      optionButton->setEnabled(true);
      
    } else if (activeMenu == 0){ // history
      // up
      if (activeElement>0)
	upButton->setEnabled(true);
      else 
	upButton->setEnabled(false);
      // down
      if (activeElement>=0 && activeElement<nitems-1)
	downButton->setEnabled(true);
      else
	downButton->setEnabled(false);
      // new
      if (activeElement==-1)
	newButton->setEnabled(true);
      else
	newButton->setEnabled(false);
      // new from file
      newfileButton->setEnabled(true);
      // rename
      renameButton->setEnabled(false);
      // erase
      if (activeElement!=-1)
	eraseButton->setEnabled(true);
      else
	eraseButton->setEnabled(false);
      // copy
      copyButton->setEnabled(true);
      // paste
      if (copyMenu!=-1 && activeElement==-1)
	pasteButton->setEnabled(true);
      else
	pasteButton->setEnabled(false);
      // edit options
      optionButton->setEnabled(false);

    } else { // standard menues
      upButton->setEnabled(false);
      downButton->setEnabled(false);
      newButton->setEnabled(false);
      newfileButton->setEnabled(false);
      renameButton->setEnabled(false);
      eraseButton->setEnabled(false);
      copyButton->setEnabled(true);
      pasteButton->setEnabled(false);
      // edit options
      optionButton->setEnabled(false);
    }
  }
}


void QuickAdmin::updateWidgets()
{

  menutree->clear();
  
  int n= menues.size();

  QTreeWidgetItem *tmp, *active= 0;
  
  for (int i=0; i<n; i++){
    QString mname(menues[i].name.cStr());
    QuickTreeWidgetItem* pp= new QuickTreeWidgetItem( menutree,
						      QStringList(mname),
						      i,-1);
    if (activeMenu==i && activeElement==-1)
      active= pp;
    int m= menues[i].menuitems.size();
    for (int j=0; j<m; j++){
      QString qstr= menues[i].menuitems[j].name.cStr();
      qstr.replace(QRegExp("</*font[^>]*>"), "" );
	tmp= new QuickTreeWidgetItem( pp, QStringList(qstr), i, j);
      if (activeMenu==i && activeElement==j)
	active= tmp;
    }
  }

   if (active) {
     active->setSelected(true);
     menutree->scrollToItem(active,QAbstractItemView::EnsureVisible);
   }
}


vector<quickMenu> QuickAdmin::getMenues() const
{
  return menues;
}


void QuickAdmin::helpClicked(){
  emit help("quickadmin");
}

void QuickAdmin::upClicked()
{
  if (activeElement==-1){
    quickMenu tmp= menues[activeMenu-1];
    menues[activeMenu-1]= menues[activeMenu];
    menues[activeMenu]= tmp;
    activeMenu--;
  } else {
    quickMenuItem tmp= menues[activeMenu].menuitems[activeElement-1];
    menues[activeMenu].menuitems[activeElement-1]=
      menues[activeMenu].menuitems[activeElement];
    menues[activeMenu].menuitems[activeElement]= tmp;
    activeElement--;
  }
  updateWidgets();
}

void QuickAdmin::downClicked()
{
  if (activeElement==-1){
    quickMenu tmp= menues[activeMenu+1];
    menues[activeMenu+1]= menues[activeMenu];
    menues[activeMenu]= tmp;
    activeMenu++;
  } else {
    quickMenuItem tmp= menues[activeMenu].menuitems[activeElement+1];
    menues[activeMenu].menuitems[activeElement+1]=
      menues[activeMenu].menuitems[activeElement];
    menues[activeMenu].menuitems[activeElement]= tmp;
    activeElement++;
  }
  updateWidgets();
}

void QuickAdmin::newClicked()
{
  bool ok = FALSE;
  if (activeElement==-1){
    QString text = QInputDialog::getText(tr("Make new menu"),
					 tr("Make new menu with name:"),
					 QLineEdit::Normal,
					 QString::null, &ok, this );
    if ( ok && !text.isEmpty() ){
      //       cerr << "Making a new MENU after menu:" << activeMenu << endl;
      quickMenu tmp;
      tmp.name= text.toStdString();
      tmp.name.trim();
      tmp.name.replace(","," ");
      tmp.filename= tmp.name + ".quick";
      tmp.filename.replace(" ","_");
      tmp.plotindex= 0;

      menues.insert(menues.begin()+activeMenu+1,tmp);
      if (firstcustom<0){
	firstcustom=1;
	lastcustom=0;
      }
      lastcustom++;
    }
    
  } else {
    QString text = QInputDialog::getText(tr("Make new plot"),
					 tr("Make new plot with name:"),
					 QLineEdit::Normal,
					 QString::null, &ok, this );
    if ( ok && !text.isEmpty() ){
      //       cerr << "Making a new ITEM in menu:" << activeMenu
      // 	   << " after item:" << activeElement << endl;
      quickMenuItem tmp;
      tmp.name= text.toStdString();
      menues[activeMenu].menuitems.insert(menues[activeMenu].menuitems.begin()
					  +activeElement, tmp);
    }
  }
  if (ok) updateWidgets();
}

void QuickAdmin::newfileClicked()
{
  QString filter= tr("Menus (*.quick);;All (*.*)");
  QString s(QFileDialog::getOpenFileName(this,
					 tr("Add new menu from file"),
					 "./",
					 filter ));
  if ( s.isEmpty() )
    return;
  
  quickMenu tmp;
  tmp.filename= s.toStdString();
  tmp.plotindex= 0;
  if (readQuickMenu(tmp)){
    menues.insert(menues.begin()+activeMenu+1,tmp);
    if (firstcustom<0){
      firstcustom=1;
      lastcustom=0;
    }
    lastcustom++;
  }
  updateWidgets();
}

void QuickAdmin::renameClicked()
{
  bool ok = FALSE;
  if (activeElement==-1){
    QString text = QInputDialog::getText(tr("Change menu name"),tr("New name:"),
					 QLineEdit::Normal,
					 menues[activeMenu].name.cStr(),
					 &ok, this );
    if ( ok && !text.isEmpty() )
      menues[activeMenu].name= text.toStdString();
      menues[activeMenu].name.trim();
      menues[activeMenu].name.replace(","," ");
      menues[activeMenu].filename= menues[activeMenu].name + ".quick";
      menues[activeMenu].filename.replace(" ","_");

  } else {
    QString text = QInputDialog::getText(tr("Change plot name"),tr("New name:"),
					 QLineEdit::Normal,
					 menues[activeMenu].menuitems[activeElement].name.cStr(),
					 &ok, this );
    if ( ok && !text.isEmpty() )
      menues[activeMenu].menuitems[activeElement].name= text.toStdString();
  }
  if (ok) updateWidgets();
}

void QuickAdmin::eraseClicked()
{
  copyClicked();
  if (activeElement==-1){
    menues.erase(menues.begin()+activeMenu);
    lastcustom--;
    if (lastcustom < firstcustom){
      firstcustom= -1;
    }
    
  } else {
    menues[activeMenu].menuitems.erase(menues[activeMenu].menuitems.begin()
				       + activeElement);
    if (activeElement == menues[activeMenu].menuitems.size()){
      if (activeElement==0)
	activeElement= -1;
      else
	activeElement--;
    }
  }
  updateWidgets();
}

void QuickAdmin::copyClicked()
{

  copyMenu= activeMenu;
  copyElement= activeElement;

  if (copyElement==-1) {
    //     cerr << "Copy menu:" << copyMenu << endl;
    MenuCopy= menues[copyMenu];
  } else {
    //     cerr << "Copy item:" << copyElement
    // 	 << " from menu:" << copyMenu <<endl;
    MenuItemCopy= menues[copyMenu].menuitems[copyElement];
  }

  if (copyElement==-1)
    pasteButton->setText(tr("Paste menu"));
  else
    pasteButton->setText(tr("Paste plot"));
  
  pasteButton->setAccel(Qt::CTRL+Qt::Key_V);

  if ((activeMenu >= firstcustom && activeMenu<= lastcustom)
      || (activeMenu==0 && activeElement==-1) )
    pasteButton->setEnabled(true);
}

void QuickAdmin::pasteClicked()
{
  if (copyElement==-1){
    //     cerr << "Paste menu:" << copyMenu << " after menu:" << activeMenu << endl;
    quickMenu tmp= MenuCopy;
    tmp.name.trim();
    tmp.name.replace(","," ");
    tmp.filename= tmp.name + "2.quick";
    tmp.filename.replace(" ","_");
    tmp.plotindex= 0;

    menues.insert(menues.begin()+activeMenu+1,tmp);
    if (firstcustom<0){
      firstcustom=1;
      lastcustom=0;
    }
    lastcustom++;
    activeMenu++;
    activeElement= -1; // move to start of menu
  } else {
    //     cerr << "Paste item:" << copyElement
    // 	 << " from menu:" << copyMenu
    // 	 << " to item:" << activeElement << ", menu:" << activeMenu << endl;
    quickMenuItem tmp= MenuItemCopy;
    int pos= (activeElement!=-1 ? activeElement+1 : 0);
    menues[activeMenu].menuitems.insert(menues[activeMenu].menuitems.begin()
					+pos, tmp);
    activeElement= pos;
  }
  copyMenu= -1;
  copyElement= -1;

  pasteButton->setText(tr("Paste"));
  pasteButton->setEnabled(false);
  pasteButton->setAccel(Qt::CTRL+Qt::Key_V);
  updateWidgets();
}

void QuickAdmin::updateCommand()
{
  autochange= true;
  if (activeMenu >=0 && activeElement>=0){
    miString ts;
    int n= menues[activeMenu].menuitems[activeElement].command.size();
    for (int i=0; i<n; i++){
      ts += menues[activeMenu].menuitems[activeElement].command[i];
      ts+= miString("\n");
    }
    // set command into command-edit
    comedit->setText(ts.cStr());
  } else
    comedit->clear();
  autochange= false;
}


void QuickAdmin::comChanged(){
  if (autochange) return;
  if (activeElement<0 || activeMenu<0) return;
//   int ni= comedit->numLines();
//   int ni= comedit->paragraphs();
//   vector<miString> s;
//   for (int i=0; i<ni; i++){
// //     miString str= comedit->textLine(i).toStdString();
//     miString str= comedit->text(i).toStdString();
//     str.trim();
//     if (str.contains("\n"))
//       str.erase(str.end()-1);
//     if (str.exists()) s.push_back(str);
//   }

//Qt4
  vector<miString> s;
  miString str= comedit->text().toStdString();
  if (str.exists()) s.push_back(str);
  menues[activeMenu].menuitems[activeElement].command= s;
}


void QuickAdmin::optionClicked()
{
  QuickEditOptions* qeo= new QuickEditOptions(this,
					      menues[activeMenu].opt);
  //connect(qeo, SIGNAL(help(const char*)), this, SIGNAL(help(const char*)));
  if (qeo->exec() ){
    menues[activeMenu].opt= qeo->getOptions();
  }
}
