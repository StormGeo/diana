/*
  Diana - A Free Meteorological Visualisation Tool

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

#include "diDrawingManager.h"
#include "drawingcomposite.h"
#include "drawingpolyline.h"
#include "drawingsymbol.h"
#include "drawingtext.h"
#include "drawingstylemanager.h"
#include "diPlotModule.h"
#include "diGLPainter.h"

#define MILOGGER_CATEGORY "diana.Composite"
#include <miLogger/miLogging.h>

namespace DrawingItem_Composite {

Composite::Composite(int id)
  : DrawingItemBase(id)
{
  created_ = false;
}

Composite::~Composite()
{
  deleteElements();
}

void Composite::draw(DiGLPainter* gl)
{
  if (!created_)
    createElements();

  if (points_.isEmpty() || elements_.isEmpty())
    return;

  DrawingStyleManager *styleManager = DrawingStyleManager::instance();

  QRectF bbox = boundingRect();

  QList<QPointF> points;
  points << bbox.bottomLeft() << bbox.bottomRight() << bbox.topRight() << bbox.topLeft();

  // Use the fill colour defined in the style to fill the text area.
  styleManager->beginFill(gl, this);
  styleManager->fillLoop(gl, this, points);
  styleManager->endFill(gl, this);

  foreach (DrawingItemBase *element, elements_)
    element->draw(gl);

  // Draw the outline using the border colour and line pattern defined in
  // the style.
  styleManager->beginLine(gl, this);
  styleManager->drawLines(gl, this, points);
  styleManager->endLine(gl, this);
}

QRectF Composite::boundingRect() const
{
  QRectF thisRect;

  foreach (DrawingItemBase *element, elements_) {
    QRectF rect = element->boundingRect();
    thisRect = thisRect.united(rect);
  }

  return thisRect;
}

void Composite::updateRect()
{
  QRectF rect = boundingRect();
  points_[0] = rect.center();
  points_[1] = rect.center();
}

DrawingItemBase::HitType Composite::hit(const QPointF &pos, bool selected) const
{
  QRectF box = boundingRect();
  if (box.contains(pos))
    return Area;
  else
    return None;
}

DrawingItemBase::HitType Composite::hit(const QRectF &bbox) const
{
  QRectF box = boundingRect();
  if (box.intersects(bbox))
    return Area;
  else
    return None;
}

void Composite::setProperties(const QVariantMap &properties, bool ignorePoints)
{
  DrawingItemBase::setProperties(properties, ignorePoints);

  // Special handling of properties is needed for composite items since they
  // manage internal items whose properties cannot be updated using the
  // simple mechanism used for normal items.

  writeExtraProperties();
  readExtraProperties();
  arrangeElements();
}

QDomNode Composite::toKML(const QHash<QString, QString> &extraExtData) const
{
  QHash<QString, QString> extra;
  QString output = toKMLExtraData();
  extra["children"] = output;
  return DrawingItemBase::toKML(extra.unite(extraExtData));
}

void Composite::fromKML(const QHash<QString, QString> &extraExtData)
{
  QString input = extraExtData.value("met:children");
  properties_["children"] = fromKMLExtraData(input);
  writeExtraProperties();
}

DrawingItemBase::Category Composite::category() const
{
  return DrawingItemBase::Composite;
}

/**
 * Create the elements for the composite object based on the relevant style definition.
 */
void Composite::createElements()
{
  METLIBS_LOG_SCOPE();

  if (PlotModule::instance()->getPlotSize().width() == 0)
    return;

  QVariantMap style = DrawingStyleManager::instance()->getStyle(DrawingItemBase::Composite, properties_.value("style:type").toString());
  if (style.isEmpty())
    return;

  if (points_.isEmpty()) {
    points_.append(QPointF(0, 0));
    points_.append(QPointF(0, 0));
  }

  QStringList objects = style.value("objects").toStringList();
  QStringList values = style.value("values").toStringList();
  QStringList styles = style.value("styles").toStringList();
  QString layout = style.value("layout").toString();

  if (layout == "horizontal")
    layout_ = Horizontal;
  else if (layout == "vertical")
    layout_ = Vertical;
  else if (layout == "diagonal")
    layout_ = Diagonal;
  else {
    layout_ = Horizontal;
    if (objects.size() != 1)
      METLIBS_LOG_WARN("Invalid layout given in " << properties_.value("style:type").toString().toStdString() << ": " << layout.toStdString());
  }

  // If the number of objects, styles and values do not match then warn and return.
  if ((objects.size() != values.size()) || (objects.size() != styles.size())) {
    METLIBS_LOG_WARN("Sizes of attributes do not match in " << properties_.value("style:type").toString().toStdString());
    return;
  }

  // Create the elements.

  for (int i = 0; i < objects.size(); ++i) {

    DrawingItemBase *element;

    if (objects.at(i) == "text") {
      element = newTextItem();
      element->setProperty("alignment", Qt::AlignLeft);
    } else if (objects.at(i) == "symbol")
      element = newSymbolItem();
    else if (objects.at(i) == "composite")
      element = newCompositeItem();
    else if (objects.at(i) == "line")
      element = newPolylineItem();
    else
      continue;

    element->setPoints(points_);

    // Copy the style property into the new element.
    element->setProperty("style:type", styles.at(i));

    if (objects.at(i) == "text") {
      // Copy the value into the element as text.
      QStringList lines;
      lines.append(values.at(i));
      static_cast<DrawingItem_Text::Text *>(element)->setText(lines);

    } else if (objects.at(i) == "composite") {
      DrawingItem_Composite::Composite *c = static_cast<DrawingItem_Composite::Composite *>(element);
      c->createElements();
    }

    elements_.append(element);
  }

  arrangeElements();

  // Write any properties for child elements already present in this item's
  // property map into the property maps of the children themselves.
  writeExtraProperties();

  // Read the child elements' properties back into the main children entry of
  // this item's property map.
  readExtraProperties();

  created_ = true;
}

void Composite::deleteElements()
{
  foreach (DrawingItemBase *element, elements_) {
    Composite *ce = dynamic_cast<Composite *>(element);
    if (ce)
      ce->deleteElements();
  }

  qDeleteAll(elements_);
  elements_.clear();
}

void Composite::arrangeElements()
{
  QRectF previousRect;

  // Find the maximum size of the text, symbol and composite items.
  QSizeF maxSize;
  for (int i = 0; i < elements_.size(); ++i) {
    DrawingItemBase *element = elements_.at(i);
    if (dynamic_cast<DrawingItem_PolyLine::PolyLine *>(element))
      continue;
    element->updateRect();
    QSizeF size = element->getSize();
    maxSize = maxSize.expandedTo(size);
  }

  QPointF start = points_[0] + QPointF(-maxSize.width()/2, -maxSize.height()/2);

  switch (layout_) {
  case Horizontal:
    previousRect = QRectF(start, QSizeF(0, maxSize.height()));
    break;
  case Vertical:
    previousRect = QRectF(start, QSizeF(maxSize.width(), 0));
    break;
  case Diagonal:
    previousRect = QRectF(start, QSizeF(maxSize.width(), maxSize.height()));
    break;
  default:
    ;
  }

  for (int i = 0; i < elements_.size(); ++i) {

    DrawingItemBase *element = elements_.at(i);
    QSizeF size = element->getSize();
    QPointF point;

    QList<QPointF> points;

    if (dynamic_cast<DrawingItem_PolyLine::PolyLine *>(element)) {

      // Treat lines differently. Position all the points based on the parent
      // and following elements, discarding the existing geometry of the line.
      switch (layout_) {
      case Horizontal:
        points << QPointF(previousRect.right(), previousRect.bottom());
        points << QPointF(previousRect.right(), previousRect.top());
        previousRect.translate(2, 0);
        break;
      case Vertical:
        points << QPointF(previousRect.left(), previousRect.top());
        points << QPointF(previousRect.right(), previousRect.top());
        previousRect.translate(0, -2);
        break;
      case Diagonal:
        points << QPointF(previousRect.left() + previousRect.width()/2,
                          previousRect.top() - previousRect.height()/2);
        points << QPointF(previousRect.right() + previousRect.width()/2,
                          previousRect.bottom() - previousRect.height()/2);
        previousRect.translate(2, -2);
        break;
      default:
        ;
      }
    } else {
      switch (layout_) {
      case Horizontal:
        point = QPointF(previousRect.right(), previousRect.bottom() - (previousRect.height() - size.height())/2);
        previousRect.translate(size.width(), 0);
        break;
      case Vertical:
        point = QPointF(previousRect.left() + (previousRect.size().width() - size.width())/2, previousRect.top());
        previousRect.translate(0, -size.height());
        break;
      case Diagonal:
        point = QPointF(previousRect.right(), previousRect.top());
        previousRect.translate(size.width(), -size.height());
        break;
      default:
        ;
      }

      // Determine the change in position of the element.
      QPointF offset = point - element->boundingRect().bottomLeft();

      points = element->getPoints();
      for (int j = 0; j < points.size(); ++j)
        points[j] += offset;
    }

    element->setPoints(points);
  }
}

DrawingItemBase *Composite::elementAt(int index) const
{
  return elements_.at(index);
}

void Composite::setPoints(const QList<QPointF> &points)
{
  QPointF offset;

  // Calculate the offset for each of the points using the first point passed.
  if (points_.isEmpty()) {
    DrawingItemBase::setPoints(points);
    offset = QPointF(0, 0);
  } else
    offset = points.at(0) - points_.at(0);

  // Adjust the child elements using the offset.
  foreach (DrawingItemBase *element, elements_) {

    QList<QPointF> newPoints;
    foreach (QPointF point, element->getPoints())
      newPoints.append(point + offset);

    element->setPoints(newPoints);
  }

  // Update the points to contain the child elements inside the object.
  updateRect();
}

/**
 * Read the properties of the child elements, storing them in a list in this
 * item's property map so that they can be stored in undo/redo commands.
 */
void Composite::readExtraProperties()
{
  // Examine the child elements of this item and their children, incorporating their
  // properties into the properties of this item.
  QVariantList extraProperties;

  foreach (DrawingItemBase *element, elements_) {
    // For child elements that are composite items, ensure that they contain
    // up-to-date information about their children.
    Composite *c = dynamic_cast<Composite *>(element);
    if (c)
      c->readExtraProperties();

    extraProperties.append(element->properties());
  }

  properties_["children"] = extraProperties;
}

/**
 * Write the properties of the child elements, retrieving them from a list in
 * this item's property map.
 */
void Composite::writeExtraProperties()
{
  METLIBS_LOG_SCOPE();
  QVariantList childList = properties_["children"].toList();
  if (childList.isEmpty())
    return;

  if (childList.size() != elements_.size()) {
    METLIBS_LOG_WARN("Number of elements " << elements_.size()
      << " does not match the number of properties "
      << childList.size() << " for style"
      << properties_["style:type"].toString().toStdString());
    return;
  }

  for (int i = 0; i < childList.size(); ++i) {
    DrawingItemBase *element = elements_.at(i);
    QVariantMap newProps = childList.at(i).toMap();
    QVariantMap props = element->properties();

    // Override the properties of the element with those from the map.
    foreach (const QString &key, newProps.keys())
      props[key] = newProps.value(key);

    element->setProperties(props);

    // Update any child elements that are composite items using their updated
    // properties.
    Composite *c = dynamic_cast<Composite *>(element);
    if (c)
      c->writeExtraProperties();
  }
}

DrawingItemBase *Composite::newCompositeItem() const
{
  return new DrawingItem_Composite::Composite();
}

DrawingItemBase *Composite::newPolylineItem() const
{
  return new DrawingItem_PolyLine::PolyLine();
}

DrawingItemBase *Composite::newSymbolItem() const
{
  return new DrawingItem_Symbol::Symbol();
}

DrawingItemBase *Composite::newTextItem() const
{
  return new DrawingItem_Text::Text();
}

/**
 * Converts the QVariantMap objects corresponding to elements in a composite
 * item to strings for storage in a KML file.
 */
QString Composite::toKMLExtraData() const
{
  QString output;

  foreach (DrawingItemBase *element, elements_) {

    DrawingItem_Text::Text *te = dynamic_cast<DrawingItem_Text::Text *>(element);
    if (te) {
      QStringList lines = te->property("text").toStringList();
      QString text = lines.join("\n");
      text = text.replace("\\", "\\\\").replace("\n", "\\n");
      output += text + "\n";

    } else {
      Composite *ce = dynamic_cast<Composite *>(element);
      if (ce)
        output += ce->toKMLExtraData();
    }
  }

  return output;
}

/**
 * Read XML elements from a stream, converting them to properties in a
 * QVariantMap and append each map to a QVariantList corresponding to
 * each drawing element. Returns the list of maps corresponding to the
 * elements held by a composite item.
 */
QVariantList Composite::fromKMLExtraData(QString &input)
{
  QVariantList childList;

  foreach (DrawingItemBase *element, elements_) {

    DrawingItem_Text::Text *te = dynamic_cast<DrawingItem_Text::Text *>(element);
    if (te) {
      // Read a line of text from the met:children element.
      int end = input.indexOf("\n");
      if (end == -1) end = input.size();
      QString text = input.mid(0, end);

      text = text.replace("\\n", "\n").replace("\\\\", "\\");
      QVariantMap childMap;
      childMap["text"] = text.split("\n");
      childList.append(childMap);

      // Remove the handled text from the input string.
      input.remove(0, end);

    } else {
      Composite *ce = dynamic_cast<Composite *>(element);
      if (ce)
        childList.append(ce->fromKMLExtraData(input));
    }
  }

  return childList;
}

} // namespace
