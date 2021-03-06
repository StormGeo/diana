/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2016 met.no

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

#include "MovieMaker.h"
#include "util/subprocess.h"
#include <QImage>

#include <cmath>
#include <cstdlib>

#define MILOGGER_CATEGORY "diana.MovieMaker"
#include <miLogger/miLogging.h>

using namespace std;

namespace {

const QString framePattern("frame_%1.png");
const int framePatternWidth = 3;
const QString framePatternFF = framePattern.arg("%0" + QString::number(framePatternWidth) + "d");

const QString format_series = "png_series";
const QString format_animated = "animated_gif";

} // namespace

MovieMaker::MovieMaker(const QString &filename, const QString &format,
    double framerate, const QSize& frameSize)
  : mOutputFile(filename)
  , mOutputFormat(format)
  , mFrameRate(framerate)
  , mFrameSize(frameSize)
  , mFrameCount(0)
{
  if (!isImageSeries())
    mOutputDir.create();
}

MovieMaker::~MovieMaker()
{
}

bool MovieMaker::isImageSeries() const
{
  return mOutputFormat == format_series;
}

QString MovieMaker::frameFile(int frameNumber) const
{
  return framePattern.arg(frameNumber, framePatternWidth, 10, QLatin1Char('0'));
}

QString MovieMaker::framePath(int frameNumber) const
{
  if (isImageSeries())
    return mOutputFile.arg(frameNumber);
  else
    return mOutputDir.filePath(frameFile(frameNumber));
}

bool MovieMaker::addImage(const QImage &image)
{
  if (!isImageSeries() && !mOutputDir.exists())
    return false;

  const QImage::Format FORMAT = QImage::Format_RGB32;

  QImage imageScaled;
  if (image.size() == mFrameSize)
    imageScaled = image;
  else
    imageScaled = image.scaled(mFrameSize);

  if (imageScaled.format() != FORMAT)
    imageScaled = imageScaled.convertToFormat(FORMAT);

  mFrameCount += 1;
  QString imagefilename = framePath(mFrameCount);
  if (imageScaled.save(imagefilename)) {
    METLIBS_LOG_DEBUG("saved frame " << mFrameCount << " to '" << imagefilename.toStdString() << "'");
    mOutputFiles << imagefilename;
    return true;
  } else {
    METLIBS_LOG_ERROR("could not save frame " << mFrameCount << " to '" << imagefilename.toStdString() << "'");
    return false;
  }
}

bool MovieMaker::finish()
{
  METLIBS_LOG_SCOPE();
  if (mFrameCount == 0) {
    METLIBS_LOG_WARN("no video frames in '" << mOutputFile.toStdString() << "'");
    return false;
  }
  if (isImageSeries())
    return true;
  if (!mOutputDir.exists())
    return false;
  if (mOutputFormat == format_animated)
    return createAnimatedGif();
  else
    return createVideo();
}

bool MovieMaker::createVideo()
{
  METLIBS_LOG_SCOPE();
  int framerate_d = 10;
  int framerate_n = static_cast<int>(round(mFrameRate*framerate_d));

  QString encoder;
  if (mOutputFormat == "mp4")
    encoder = "libx264";
  else if (mOutputFormat == "avi")
    encoder = "msmpeg4v2";
  else
    encoder = "mpeg2video";

  const QString converter = "avconv";
  QStringList args;
  args << "-y" // overwrite output file
       << "-framerate" << QString("%1/%2").arg(framerate_n).arg(framerate_d)
       << "-i" << mOutputDir.filePath(framePatternFF)
       << "-c:v" << encoder
       << "-r" << "30"
       << "-pix_fmt" << "yuv420p"
       << mOutputFile;
  METLIBS_LOG_INFO("running: '" << converter.toStdString() << "' '" << args.join("' '").toStdString() << "'");
  const int code = diutil::execute(converter, args);
  if (code == 0) {
    METLIBS_LOG_INFO("video created in '" << args.back().toStdString() << "'");
    mOutputFiles.clear();
    mOutputFiles << mOutputFile;
  } else {
    METLIBS_LOG_ERROR("problem creating video, code=" << code);
  }
  return (code == 0);
}

bool MovieMaker::createAnimatedGif()
{
  METLIBS_LOG_SCOPE();
  const int delay = std::max(1, int(100 / mFrameRate));

  const QString converter = "convert";
  QStringList args;
  args << "-delay" << QString::number(delay)
       << mOutputFiles
       << mOutputFile;
  METLIBS_LOG_INFO("running: '" << converter.toStdString() << "' '" << args.join("' '").toStdString() << "'");
  const int code = diutil::execute(converter, args);
  if (code == 0) {
    METLIBS_LOG_INFO("animation created in '" << args.back().toStdString() << "'");
    mOutputFiles.clear();
    mOutputFiles << mOutputFile;
  } else {
    METLIBS_LOG_ERROR("problem creating animation, code=" << code);
  }
  return (code == 0);
}
