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

#include <iostream>

#include <diOrderClient.h>

/*
 * Protocol description:
 *
 * Sentences:
 *
 * - A sentence is a sequence of keywords and parameters ending in a
 *   newline.
 *
 * - Whitespace is not significant; leading and trailing whitespace is
 *   ignored, internal whitespace is reduced to single spaces.
 *
 * - A sentence can be followed by a comment, which starts with "#" and
 *   ends at the end of the line.
 *
 * - Blank lines (including lines consisting only of a comment) are
 *   ignored.
 *
 * Upon accepting a connection, we send the sentence "hello" and wait for
 * input from the client.
 *
 * The client sends the sentence "start order text" or "start order
 * base64", then the work order in either plaintext or base64, then the
 * sentence "end order".
 *
 * TODO how do we report the results?
 */
const QString diOrderClient::kw_hello("hello");
const QString diOrderClient::kw_error("error");
const QString diOrderClient::kw_start_order_text("start order text");
const QString diOrderClient::kw_start_order_base64("start order base64");
const QString diOrderClient::kw_end_order("end order");

diOrderClient::diOrderClient(QObject *parent, QTcpSocket *socket):
	QObject(parent), socket(socket), state(idle), base64(false)
{
	connect(socket, SIGNAL(readyRead()),
	    this, SLOT(clientReadyRead()));
	connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
	    this, SLOT(clientStateChanged(QAbstractSocket::SocketState)));
	hello();
}

diOrderClient::~diOrderClient()
{
	delete socket;
}

void
diOrderClient::clientReadyRead()
{
	if (state != idle && state != reading)
		// not expecting any input
		return;
	if (socket->canReadLine()) {
		QByteArray line = socket->readLine();
		/*
		 * Since canReadLine() was true, we know that an empty
		 * array results from an error.
		 */
		if (line.isEmpty()) {
			socket->close();
			return;
		}
		QString sline = line;
		if (sline.contains('#'))
			sline.truncate(sline.indexOf('#'));
		sline = sline.simplified();
		if (state != reading && sline.isEmpty())
			return;
		// std::cerr << "[" << sline.toStdString() << "]" << std::endl;
		if (state == idle) {
			if (sline == kw_start_order_text) {
				state = reading;
				base64 = false;
			} else if (sline == kw_start_order_base64) {
				state = reading;
				base64 = true;
			} else {
				error(QString("expected 'start order'"));
			}
		} else if (state == reading) {
			if (sline == kw_end_order) {
				QString order;
				if (base64)
					order = QByteArray::fromBase64(orderbuf);
				else
					order = orderbuf;
				std::cerr << "incoming order:" << std::endl <<
				    order.toStdString() << std::endl;
				state = pending;
				// TODO dispatch work order
			} else {
				orderbuf += sline;
			}
		} else {
			error(QString("internal error"));
		}
	}
}

void
diOrderClient::clientStateChanged(QAbstractSocket::SocketState state)
{
	if (state == 0) {
		// buh-bye
		emit connectionClosed();
	}
}

void
diOrderClient::message(const QString &kw, const QString &msg)
{
	QByteArray line;
	line += kw;
	line += " // ";
	line += msg.simplified();
	line += "\n";
	socket->write(line);
}

void
diOrderClient::hello()
{
	message(kw_hello, "bdiana"); // should include version #, parametrized?
}

void
diOrderClient::error(const QString &msg)
{
	message(kw_error, msg);
	socket->close();
}