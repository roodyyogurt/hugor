// This is copyrighted software. More information is at the end of this file.
#include "hugohandlers.h"

#include <QDebug>
#include <QFileDialog>
#include <QTextCodec>
#include <QTextLayout>
#include <QTextStream>
#include <QWindow>
#include <algorithm>
#include <cstdio>

#include "hframe.h"
#include "hmainwindow.h"
#include "hugorfile.h"
#include "settings.h"
#include "videoplayer.h"

void HugoHandlers::calcFontDimensions()
{
    const QFontMetrics& curMetr = hFrame->currentFontMetrics();
    const QFontMetrics fixedMetr(hApp->settings()->fixed_font);

    FIXEDCHARWIDTH = fixedMetr.averageCharWidth();
    FIXEDLINEHEIGHT = fixedMetr.lineSpacing();

    ::charwidth = curMetr.averageCharWidth();
    lineheight = curMetr.lineSpacing();
}

void HugoHandlers::getfilename(char* a, char* b)
{
    Q_ASSERT(a != nullptr and b != nullptr);

    QString fname;
    QString filter;
    // Fallback message in case we won't recognize the 'a' string.
    QString caption("Select a file");
    // Assume save mode. We do this in order to get a confirmation dialog on existing files, in case
    // we won't recognize the 'a' string.
    bool saveMode = true;

    if (QString::fromLatin1(a).endsWith("to save")) {
        filter = QObject::tr("Hugo Saved Games") + QString::fromLatin1(" (*.sav)");
        caption = "Save current game position";
    } else if (QString::fromLatin1(a).endsWith("to restore")) {
        filter = QObject::tr("Hugo Saved Games") + QString::fromLatin1(" (*.sav)");
        caption = "Restore a saved game position";
        saveMode = false;
    } else if (QString::fromLatin1(a).endsWith("for command recording")) {
        filter = QObject::tr("Hugo recording files") + QString::fromLatin1(" (*.rec)");
        caption = "Record commands to a file";
    } else if (QString::fromLatin1(a).endsWith("for command playback")) {
        filter = QObject::tr("Hugo recording files") + QString::fromLatin1(" (*.rec)");
        caption = "Play recorded commands from a file";
        saveMode = false;
    } else if (QString::fromLatin1(a).endsWith("transcription (or printer name)")) {
        filter = QObject::tr("Transcription files") + QString::fromLatin1(" (*.txt)");
        caption = "Save transcript to a file";
    }

    hFrame->setPreventAutoMinimize(true);
    if (saveMode) {
        fname = QFileDialog::getSaveFileName(hMainWin, caption, b, filter);
    } else {
        fname = QFileDialog::getOpenFileName(hMainWin, caption, b, filter);
    }
    hFrame->setPreventAutoMinimize(false);
    qstrcpy(line, fname.toLocal8Bit().constData());
}

void HugoHandlers::startGetline(char* p)
{
    // Print the prompt in normal text colors.
    settextcolor(fcolor);
    setbackcolor(bgcolor);
    print(p);

    // Switch to input color.
    settextcolor(icolor);

    hFrame->setCursorVisible(true);
    hFrame->moveCursorPos(QPoint(current_text_x, current_text_y));
    hFrame->startInput(current_text_x, current_text_y);
}

void HugoHandlers::endGetline()
{
    hFrame->setCursorVisible(false);
    char newline[] = "\r\n";
    print(newline);
    hFrame->updateGameScreen(false);
}

void HugoHandlers::clearfullscreen()
{
    hFrame->clearRegion(0, 0, 0, 0);
}

void HugoHandlers::clearwindow()
{
    hFrame->setBgColor(bgcolor);
    hFrame->clearRegion(physical_windowleft, physical_windowtop, physical_windowright,
                        physical_windowbottom);
}

void HugoHandlers::settextmode()
{
    calcFontDimensions();
    SCREENWIDTH = hFrame->width();
    SCREENHEIGHT = hFrame->height();

    /* Must be set: */
    settextwindow(1, 1, SCREENWIDTH / FIXEDCHARWIDTH, SCREENHEIGHT / FIXEDLINEHEIGHT);
}

void HugoHandlers::settextwindow(int left, int top, int right, int bottom)
{
    // qDebug() << "settextwindow" << left << top << right << bottom;
    /* Must be set: */
    physical_windowleft = (left - 1) * FIXEDCHARWIDTH;
    physical_windowtop = (top - 1) * FIXEDLINEHEIGHT;
    physical_windowright = right * FIXEDCHARWIDTH - 1;
    physical_windowbottom = bottom * FIXEDLINEHEIGHT - 1;

    // Correct for full-width windows where the right border would otherwise be clipped to a
    // multiple of charwidth, leaving a sliver of the former window at the righthand side.
    if (right >= SCREENWIDTH / FIXEDCHARWIDTH) {
        physical_windowright = hFrame->width() - 1;
    }
    if (bottom >= SCREENHEIGHT / FIXEDLINEHEIGHT) {
        physical_windowbottom = hFrame->height() - 1;
    }

    physical_windowwidth = physical_windowright - physical_windowleft + 1;
    physical_windowheight = physical_windowbottom - physical_windowtop + 1;

    hFrame->setFgColor(fcolor);
    hFrame->setBgColor(bgcolor);
    hFrame->setFontType(currentfont);
}

void HugoHandlers::printFatalError(char* a)
{
    print(a);
}

void HugoHandlers::print(char* a)
{
    uint len = qstrlen(a);
    QString ac;

    for (uint i = 0; i < len; ++i) {
        // If we've passed the bottom of the window, align to the bottom edge.
        if (current_text_y > physical_windowbottom - lineheight) {
            int temp_lh = lineheight;
            lineheight = current_text_y - physical_windowbottom + lineheight;
            current_text_y -= lineheight;
            if (inwindow) {
                --lineheight;
            }
            hFrame->scrollUp(physical_windowleft, physical_windowtop, physical_windowright,
                             physical_windowbottom, lineheight);
            TB_Scroll();
            lineheight = temp_lh;
        }

        switch (a[i]) {
        case '\n':
            hFrame->flushText();
            current_text_y += lineheight;
            // last_was_italic = false;
            break;

        case '\r':
            hFrame->flushText();
            if (!inwindow) {
                current_text_x = physical_windowleft - FIXEDCHARWIDTH;
            } else {
                current_text_x = 0;
            }
            current_text_x = physical_windowleft;
            // last_was_italic = false;
            break;

        default: {
            ac += hApp->hugoCodec()->toUnicode(a + i, 1);
        }
        }
    }

    hFrame->printText(ac, current_text_x, current_text_y);
    current_text_x += hFrame->currentFontMetrics().width(ac);

    // Check again after printing.
    if (current_text_y > physical_windowbottom - lineheight) {
        int temp_lh = lineheight;
        lineheight = current_text_y - physical_windowbottom + lineheight;
        current_text_y -= lineheight;
        if (inwindow) {
            --lineheight;
        }
        hFrame->scrollUp(physical_windowleft, physical_windowtop, physical_windowright,
                         physical_windowbottom, lineheight);
        TB_Scroll();
        lineheight = temp_lh;
    }
}

void HugoHandlers::font(int f)
{
    hFrame->setFontType(f);
    ::charwidth = hFrame->currentFontMetrics().averageCharWidth();
    lineheight = hFrame->currentFontMetrics().lineSpacing();
}

void HugoHandlers::settextcolor(int c)
{
    hFrame->setFgColor(c);
}

void HugoHandlers::setbackcolor(int c)
{
    hFrame->setBgColor(c);
}

// FIXME: Check for errors when loading images.
void HugoHandlers::displaypicture(HUGO_FILE infile, long len, int* result)
{
    // Open it as a QFile.
    long pos = ftell(infile->get());
    QFile file;
    file.open(infile->get(), QIODevice::ReadOnly);
    file.seek(pos);

    // Load the data into a byte array.
    const QByteArray& data = file.read(len);

    // Create the image from the data.
    // FIXME: Allow only JPEG images. By default, QImage supports all image formats recognized by
    // Qt.
    QImage img;
    img.loadFromData(data);
    const auto dpr = hMainWin->windowHandle()->devicePixelRatio();
    img.setDevicePixelRatio(dpr);

    // Scale the image, if needed.
    QSize imgSize(img.size());
    if (img.width() > physical_windowwidth) {
        imgSize.setWidth(physical_windowwidth);
    }
    if (img.height() > physical_windowheight) {
        imgSize.setHeight(physical_windowheight);
    }
    // Make sure to keep the aspect ratio (don't stretch.)
    if (imgSize != img.size()) {
        img = img.scaled(imgSize * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imgSize = img.size() / dpr;
    } else if (not qFuzzyCompare(dpr, 1.0)) {
        img = img.scaled(img.size() * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    // The image should be displayed centered.
    int x = (physical_windowwidth - imgSize.width()) / 2 + physical_windowleft;
    int y = (physical_windowheight - imgSize.height()) / 2 + physical_windowtop;
    hFrame->printImage(img, x, y);
    *result = true;
}

#ifndef DISABLE_VIDEO

static VideoPlayer* vid_player = nullptr;

void HugoHandlers::stopvideo()
{
    if (vid_player != nullptr) {
        vid_player->stop();
    }
}

void HugoHandlers::playvideo(HUGO_FILE infile, long len, char loop, char bg, int vol, int* result)
{
    if (not hApp->settings()->enable_video or hApp->settings()->video_sys_error) {
        *result = false;
        return;
    }
    stopvideo();
    if (vid_player == nullptr) {
        vid_player = new VideoPlayer(hFrame);
        if (vid_player == nullptr) {
            *result = false;
            return;
        }
    }
    if (not vid_player->loadVideo(infile, len, loop)) {
        *result = false;
        return;
    }
    vid_player->setVolume(vol);
    vid_player->setMaximumSize(QSize(physical_windowwidth, physical_windowheight));
    vid_player->setGeometry(physical_windowleft, physical_windowtop, physical_windowwidth,
                             physical_windowheight);
    if (not bg) {
        QEventLoop idleLoop;
        QObject::connect(vid_player, &VideoPlayer::videoFinished, &idleLoop, &QEventLoop::quit);
        QObject::connect(vid_player, &VideoPlayer::errorOccurred, &idleLoop, &QEventLoop::quit);
        QObject::connect(hApp, &HApplication::gameQuitting, &idleLoop, &QEventLoop::quit);
        QObject::connect(hFrame, &HFrame::escKeyPressed, &idleLoop, &QEventLoop::quit);
        vid_player->play();
        idleLoop.exec();
    } else {
        vid_player->play();
    }
    *result = true;
}

#endif // !DISABLE_VIDEO

/* Copyright (C) 2011-2019 Nikos Chantziaras
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
 */
