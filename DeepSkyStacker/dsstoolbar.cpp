/****************************************************************************
**
** Copyright (C) 2020 David C. Partridge
* **
** BSD License Usage
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of DeepSkyStacker nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
**
****************************************************************************/
#include "stdafx.h"
#include <QMenu>
#include <QMouseEvent>

#include "dsseditstars.h"
#include "dssselectrect.h"
#include "dsstoolbar.h"
#include "SaveEditChanges.h"

#include "ZTrace.h"

namespace DSS
{
    ToolBar::ToolBar(QWidget* parent, EditStars* ed, SelectRect* sel) :
        QToolBar(parent),
        editor{ ed },
        selectRect{ sel },
        opacityEffect(this),
        selectionGroup(this)
    {
        constexpr int iconpixels = 48;
        constexpr int radius = 27;

        setOpacity(0.6, false);

        QSize iconSize(iconpixels, iconpixels);

        selRect.addFile(":/toolbar/ButtonSelect_Up.png", iconSize, QIcon::Normal);
        selRect.addFile(":/toolbarButtonSelect_Hot.png", iconSize, QIcon::Active);
        selRect.addFile(":/toolbar/ButtonSelect_Down.png", iconSize, QIcon::Normal, QIcon::On);

        selStars.addFile(":/toolbar/ButtonStar_Up.png", iconSize, QIcon::Normal);
        selStars.addFile(":/toolbar/ButtonStar_Hot.png", iconSize, QIcon::Active);
        selStars.addFile(":/toolbar/ButtonStar_Down.png", iconSize, QIcon::Normal, QIcon::On);

        selComet.addFile(":/toolbar/ButtonComet_Up.png", iconSize, QIcon::Normal);
        selComet.addFile(":/toolbar/ButtonComet_Hot.png", iconSize, QIcon::Active);
        selComet.addFile(":/toolbar/ButtonComet_Down.png", iconSize, QIcon::Normal, QIcon::On);

        saveButton.addFile(":/toolbar/ButtonSave_Up.png", iconSize, QIcon::Normal);
        saveButton.addFile(":/toolbar/ButtonSave_Hot.png", iconSize, QIcon::Active);
        saveButton.addFile(":/toolbar/ButtonSave_Down.png", iconSize, QIcon::Selected);

        setOrientation(Qt::Vertical);

        rectAction = addAction(selRect, "");
        rectAction->setToolTip(tr(
            "Custom Rectangle Mode:\n"
            "This mode allows you to create or modify a Custom Rectangle\n"
            "defining the part of the images you wish to stack.",
            "IDS_TOOLTIP_SELECTRECT"
        ));
        connect(rectAction, &QAction::triggered, editor, &EditStars::rectButtonChecked);
        connect(rectAction, &QAction::triggered, selectRect, &SelectRect::rectButtonChecked);

        starsAction = addAction(selStars, "");
        starsAction->setToolTip(tr(
            "Edit Stars Mode:\n"
            "This mode shows the stars that have been detected in the image.\n"
            "You can add additional stars or remove incorrectly detected stars.",
            "IDS_TOOLTIP_STAR"
        ));
        connect(starsAction, &QAction::triggered, editor, &EditStars::starsButtonChecked);
        connect(starsAction, &QAction::triggered, selectRect, &SelectRect::starsButtonChecked);

        cometAction = addAction(selComet, "");
        cometAction->setToolTip(tr(
            "Edit Comet Mode:\n"
            "This mode allows you to select and edit the location\n"
            "of the comet's nucleus in the image.",
            "IDS_TOOLTIP_COMET"
        ));
        connect(cometAction, &QAction::triggered, editor, &EditStars::cometButtonChecked);
        connect(cometAction, &QAction::triggered, selectRect, &SelectRect::cometButtonChecked);

        saveAction = addAction(saveButton, "");
        saveAction->setToolTip(tr(
            "Save changes:\n"
            "Saves the changes made to the current image\n"
            "in Edit Stars and Edit Comet modes.\n\n"
            "Right Click to change behaviour.",
            "IDS_TOOLTIP_SAVE"
        ));
        connect(saveAction, &QAction::triggered, editor, &EditStars::saveButtonPressed);
        connect(saveAction, &QAction::triggered, selectRect, &SelectRect::saveButtonPressed);

        rectAction->setCheckable(true);
        starsAction->setCheckable(true);
        cometAction->setCheckable(true);
        selectionGroup.addAction(rectAction);
        selectionGroup.addAction(starsAction);
        selectionGroup.addAction(cometAction);
        selectionGroup.setExclusive(true);
        rectAction->setChecked(true);
        rectAction->activate(QAction::Trigger);

        setIconSize(iconSize);
        adjustSize();
        QString styleSheet(QString(
            "QToolBar "
            "{background-color: rgba(255,255,255,205); "
            "border-radius: %1px;}; "
        )
            .arg(radius)
        );
        setStyleSheet(styleSheet);

        createActions().createMenus();

    }


    ToolBar& ToolBar::createActions()
    {
        onSaveAlways = new QAction(tr("Save without asking", "ID_SAVECONTEXT_SAVEWITHOUTASKING"), this);
        connect(onSaveAlways, &QAction::triggered, this,
            [=]() { this->setSaveMode(static_cast<int>(EditSaveMode::SECM_SAVEDONTASK)); });

        onSaveNever = new QAction(tr("Don't save", "ID_SAVECONTEXT_DONTSAVEWITHOUTASKING"), this);
        connect(onSaveNever, &QAction::triggered, this,
            [=]() { this->setSaveMode(static_cast<int>(EditSaveMode::SECM_DONTSAVEDONTASK)); });

        onAskAlways = new QAction(tr("Ask always", "ID_SAVECONTEXT_ASKALWAYS"), this);
        connect(onAskAlways, &QAction::triggered, this,
            [=]() { this->setSaveMode(static_cast<int>(EditSaveMode::SECM_DONTSAVEDONTASK)); });

        return *this;
    }

    ToolBar& ToolBar::createMenus()
    {
        QMenu* menu = new QMenu(this);
        menu->addAction(onSaveAlways);
        menu->addAction(onSaveNever);
        menu->addAction(onAskAlways);

        saveMenu = menu;

        return *this;
    }

    void ToolBar::setSaveMode(int mode)
    {
        SetSaveEditMode(static_cast<EditSaveMode>(mode));
    }

    void ToolBar::mousePressEvent(QMouseEvent* event)
    {
        //
        // If this is a right mouse button press and the mouse is over the
        // save Button, then show the save Button menu
        // 
        const QPoint globalMouseLocation(QCursor::pos());
        const QPoint mouseLocation(mapFromGlobal(globalMouseLocation));
        if (Qt::RightButton == event->buttons() && saveAction == actionAt(mouseLocation))
        {
            //
            // Show the popup menu 
            //
            saveMenu->exec(QCursor::pos());
        }
        Inherited::mousePressEvent(event);
    }
}