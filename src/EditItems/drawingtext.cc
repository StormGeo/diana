/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2014 met.no

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

#include "GL/gl.h"
#include "drawingtext.h"
#include "EditItems/drawingstylemanager.h"
#include "diFontManager.h"

namespace DrawingItem_Text {

Text::Text()
{
  margin_ = 4;
  spacing_ = 0.5;
}

Text::~Text()
{
}

void Text::draw()
{
  if (points_.isEmpty() || lines_.isEmpty())
    return;

  DrawingStyleManager *styleManager = DrawingStyleManager::instance();
  styleManager->beginText(this);

  GLfloat s = qMax(pwidth/maprect.width(), pheight/maprect.height());
  fp->set(poptions.fontname, poptions.fontface, poptions.fontsize * s);

  float x = points_.at(0).x();
  float y = points_.at(0).y();

  foreach (QString text, lines_) {
    QSizeF size = getStringSize(text);
    size.setHeight(qMax(size.height(), qreal(poptions.fontsize)));
    fp->drawStr(text.toStdString().c_str(), x, y - size.height(), 0);
    y -= size.height() * (1.0 + spacing_);
  }

  styleManager->endText(this);
}

QDomNode Text::toKML() const
{
  return DrawingItemBase::toKML(); // call base implementation for now
}

QSizeF Text::getStringSize(const QString &text, int index) const
{
  if (index == -1)
    index = text.size();

  float width, height;
  GLfloat s = qMax(pwidth/maprect.width(), pheight/maprect.height());
  fp->set(poptions.fontname, poptions.fontface, poptions.fontsize * s);
  fp->getStringSize(text.left(index).toStdString().c_str(), width, height);

  height = qMax(height, poptions.fontsize);

  return QSizeF(width, height);
}

} // namespace
