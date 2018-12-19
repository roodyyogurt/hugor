/* Copyright 2015 Nikos Chantziaras
 *
 * This file is part of Hugor.
 *
 * Hugor is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Hugor is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Hugor.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Additional permission under GNU GPL version 3 section 7
 *
 * If you modify this Program, or any covered work, by linking or combining it
 * with the Hugo Engine (or a modified version of the Hugo Engine), containing
 * parts covered by the terms of the Hugo License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 * Corresponding Source for a non-source form of such a combination shall
 * include the source code for the parts of the Hugo Engine used as well as
 * that of the covered work.
 */
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QPainter>
#include <utility>

#include "hmarginwidget.h"
#include "hmainwindow.h"
#include "hugodefs.h"


HMarginWidget::HMarginWidget(QWidget* parent )
    : QWidget(parent)
    , fLayout(new QVBoxLayout)
    , fColor(hugoColorToQt(17))
{
    fLayout->setContentsMargins(0, 0, 0, 0);
    fLayout->setSpacing(0);
    setLayout(fLayout);
    setAttribute(Qt::WA_OpaquePaintEvent);
}


void
HMarginWidget::wheelEvent( QWheelEvent* e )
{
    if (e->delta() > 0) {
        hMainWin->showScrollback();
    }
    e->accept();
}


void
HMarginWidget::mouseMoveEvent( QMouseEvent* e )
{
    if (cursor().shape() == Qt::BlankCursor) {
        unsetCursor();
        setMouseTracking(false);
    }
    QWidget::mouseMoveEvent(e);
}


void
HMarginWidget::paintEvent(QPaintEvent* e)
{
    //const QMargins& m = contentsMargins();
    //if (m.isNull()) {
    //     return;
    //}
    QPainter p(this);
    p.fillRect(e->rect(), fColor);
    //p.fillRect(rect(), fColor);

    // Previous code, before introducing the fade screen opcode.
    //p.fillRect(0, 0, m.left(), height(), fColor);
    //p.fillRect(width() - m.right(), 0, m.right(), height(), fColor);
}


void
HMarginWidget::setBannerWidget( QWidget* w )
{
    // If a banner widget is already set, delete it first.
    if (fBannerWidget != nullptr) {
        fLayout->removeWidget(fBannerWidget);
        fBannerWidget->deleteLater();
    }
    fBannerWidget = w;
    if (w != nullptr) {
        w->setParent(this);
        fLayout->insertWidget(0, w);
        w->show();
    }
}


void
HMarginWidget::addWidget( QWidget* w )
{
    fLayout->addWidget(w);
}


void
HMarginWidget::removeWidget( QWidget* w )
{
    fLayout->removeWidget(w);
}


void
HMarginWidget::setColor(QColor color)
{
    fColor = std::move(color);
}
