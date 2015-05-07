/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2013 met.no

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

#ifndef DRAWINGLAYERSPANE_H
#define DRAWINGLAYERSPANE_H

#include <EditItems/layerspanebase.h>

namespace EditItems {

class LayerManager;

class DrawingLayersPane : public LayersPaneBase
{
  Q_OBJECT
public:
  DrawingLayersPane(const QString &);
  virtual void updateButtons();
private:
  virtual void addContextMenuActions(QMenu &) const;
  virtual bool handleContextMenuAction(const QAction *, const QList<LayerWidget *> &);
  void duplicateToEditable(const QSharedPointer<Layer> &);
  QToolButton *duplicateToEditableButton_;
  QAction *duplicateToEditable_act_;
private slots:
  void duplicateSelectedToEditable();
signals:
  void newEditLayerRequested(const QSharedPointer<Layer> &);
};

} // namespace

#endif // DRAWINGLAYERSPANE_H
