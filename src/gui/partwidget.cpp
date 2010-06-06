/***************************************************************************
 *   Copyright (C) 2008,2010 by Volker Lanz <vl@fidra.de>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "gui/partwidget.h"

#include "util/capacity.h"

#include "core/partition.h"

#include "fs/filesystem.h"

#include <QPainter>
#include <QStyleOptionButton>

#include <kdebug.h>
#include <kglobalsettings.h>

#include <config.h>

/** Creates a new PartWidget
	@param parent pointer to the parent widget
	@param p pointer to the Partition this widget will show. must not be NULL.
*/
PartWidget::PartWidget(QWidget* parent, const Partition* p) :
	PartWidgetBase(parent),
	m_Partition(NULL),
	m_Active(false)
{
	setFont(KGlobalSettings::smallestReadableFont());

	init(p);
}

void PartWidget::init(const Partition* p)
{
	m_Partition = p;

	if (partition())
		setToolTip(partition()->deviceNode() + '\n' + partition()->fileSystem().name() + ' ' + Capacity(*partition()).toString());
	else
		setToolTip(QString());

	updateChildren();
}

/** Updates the widget's children */
void PartWidget::updateChildren()
{
	if (partition())
	{
		foreach (QWidget* w, childWidgets())
		{
			w->setVisible(false);
			w->deleteLater();
			w->setParent(NULL);
		}

		foreach(const Partition* child, partition()->children())
		{
			QWidget* w = new PartWidget(this, child);
			w->setVisible(true);
		}

		positionChildren(this, partition()->children(), childWidgets());
	}
}

void PartWidget::resizeEvent(QResizeEvent*)
{
	if (partition())
		positionChildren(this, partition()->children(), childWidgets());
}

QColor PartWidget::activeColor(const QColor& col) const
{
	return isActive() ? col.darker(130) : col;
}

void PartWidget::paintEvent(QPaintEvent*)
{
	if (partition() == NULL)
		return;

	const int usedPercentage = partition()->used() * 100 / partition()->capacity();
	const int w = (width() - 1 - (PartWidget::borderWidth() * 2)) * usedPercentage / 100;

	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing);
	drawGradient(&painter, activeColor(Config::fileSystemColorCode(partition()->fileSystem().type())), QRect(0, 0, width() - 1, height() - 1));

	if (partition()->roles().has(PartitionRole::Extended))
		return;

	if (!partition()->roles().has(PartitionRole::Unallocated))
	{
		// draw free space background
		drawGradient(&painter, activeColor(Config::availableSpaceColorCode()), QRect(PartWidget::borderWidth(), PartWidget::borderHeight(), width() - 1 - (PartWidget::borderWidth() * 2), height() - (PartWidget::borderHeight() * 2)));

		// draw used space in front of that
		drawGradient(&painter, activeColor(Config::usedSpaceColorCode()), QRect(PartWidget::borderWidth(), PartWidget::borderHeight(), w, height() - (PartWidget::borderHeight() * 2)));
	}

	// draw name and size
	QString text = partition()->deviceNode().remove("/dev/") + '\n' + Capacity(*partition()).toString();

	const QRect textRect(0, 0, width() - 1, height() - 1);
	const QRect boundingRect = painter.boundingRect(textRect, Qt::AlignVCenter | Qt::AlignHCenter, text);
	if (boundingRect.x() > PartWidgetBase::borderWidth() && boundingRect.y() > PartWidgetBase::borderHeight())
		painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignHCenter, text);
}

void PartWidget::drawGradient(QPainter* painter, const QColor& color, const QRect& rect) const
{
	if (rect.width() < 8)
		return;

	QStyleOptionButton option;
	option.initFrom(this);
	option.rect = rect;
	option.palette.setColor(QPalette::Button, color);
	option.palette.setColor(QPalette::Window, color);
	option.state |= QStyle::State_Raised;
	option.state &= ~QStyle::State_MouseOver;

	style()->drawControl(QStyle::CE_PushButtonBevel, &option, painter, this);
}
