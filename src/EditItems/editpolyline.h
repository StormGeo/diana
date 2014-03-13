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

#ifndef EDITITEM_POLYLINE_H
#define EDITITEM_POLYLINE_H

#include <QtGui> // ### include only relevant headers ... TBD
#include <diDrawingManager.h>
#include "EditItems/edititembase.h"
#include "EditItems/drawingpolyline.h"

namespace EditItem_PolyLine {

class PolyLine : public EditItemBase, public DrawingItem_PolyLine::PolyLine
{
    Q_OBJECT
    friend class SetGeometryCommand;
public:
    PolyLine();
    virtual ~PolyLine();

private:
    virtual DrawingItemBase *cloneSpecial() const;

    virtual bool hit(const QPointF &, bool) const;
    virtual bool hit(const QRectF &) const;

    virtual void mousePress(QMouseEvent *, bool &, QList<QUndoCommand *> *, QSet<QSharedPointer<DrawingItemBase> > *, const QSet<QSharedPointer<DrawingItemBase> > *, bool *);

    virtual void incompleteMousePress(QMouseEvent *, bool &, bool &, bool &);
    virtual void incompleteMouseHover(QMouseEvent *, bool &);
    virtual void incompleteKeyPress(QKeyEvent *, bool &, bool &, bool &);

    virtual QString infoString() const { return QString("%1 type=%2 npoints=%3").arg(DrawingItemBase::infoString()).arg(metaObject()->className()).arg(points_.size()); }

    virtual void drawIncomplete() const;
    virtual void drawHoverHighlighting(bool) const;

    virtual void resize(const QPointF &);
    virtual void updateControlPoints();
    virtual void setPoints(const QList<QPointF> &);

    static qreal dist2(const QPointF &, const QPointF &);
    static qreal distance2(const QPointF &, const QPointF &, const QPointF &);
    qreal distance(const QPointF &) const;

    int hitLine(const QPointF &) const;

    virtual QList<QAction *> actions(const QPoint &) const;

    QAction *addPoint_act_;
    QAction *removePoint_act_;
    mutable int hitPointIndex_;
    mutable int hitLineIndex_;
    mutable QPoint hitLinePos_;

private slots:
    void addPoint();
    void removePoint();
};

} // namespace EditItem_PolyLine

#endif // EDITITEM_POLYLINE_H
