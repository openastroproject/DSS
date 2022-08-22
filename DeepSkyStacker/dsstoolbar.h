#pragma once
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
#include <QToolBar>
#include <QGraphicsOpacityEffect>

class QMouseEvent;

namespace DSS
{
    class EditStars;
    class SelectRect;

    class ToolBar : public QToolBar
    {
        Q_OBJECT

        typedef QToolBar
            Inherited;

    public:
        explicit ToolBar(QWidget* parent, DSS::EditStars* ed, DSS::SelectRect* sel);

        ~ToolBar() {};

        inline void setOpacity(qreal opacity, bool upd)
        {
            opacityEffect.setOpacity(opacity);
            setGraphicsEffect(&opacityEffect);

            if (upd) update();
        };

    protected:
        inline void enterEvent([[maybe_unused]] QEvent* e) override
        {
            Inherited::enterEvent(e);
            setOpacity(1.0, true);
        };

        inline void leaveEvent([[maybe_unused]] QEvent* e) override
        {
            setOpacity(0.6, true);
            Inherited::leaveEvent(e);
        };

        QGraphicsOpacityEffect opacityEffect;

        void mousePressEvent(QMouseEvent* e) override;

    private:

        DSS::EditStars* editor;
        DSS::SelectRect* selectRect;

        QIcon selRect;
        QIcon selStars;
        QIcon selComet;
        QIcon saveButton;

        QAction* rectAction;
        QAction* starsAction;
        QAction* cometAction;
        QAction* saveAction;

        QActionGroup selectionGroup;

        QAction* onSaveAlways;
        QAction* onSaveNever;
        QAction* onAskAlways;
        QMenu* saveMenu;

        DSS::ToolBar& createActions();
        DSS::ToolBar& createMenus();

    private slots:

        void setSaveMode(int);

    };
}
