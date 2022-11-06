/*
 * This file is part of Bino, a 3D video player.
 *
 * Copyright (C) 2022
 * Martin Lambers <marlam@marlam.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>

#include "modes.hpp"
#include "bino.hpp"


class Widget : public QOpenGLWidget, protected QOpenGLExtraFunctions
{
Q_OBJECT

private:
    QSize _sizeHint;
    int _width, _height;

    OutputMode _outputMode;
    bool _openGLStereo;       // is this widget in quad-buffered stereo mode?
    int _alternatingLastView; // last view displayed in Mode_Alternating (0 or 1)

    bool _inThreeSixtyMovement;
    QPointF _threeSixtyMovementStart;
    float _threeSixtyHorizontalAngleBase;
    float _threeSixtyVerticalAngleBase;
    float _threeSixtyHorizontalAngleCurrent;
    float _threeSixtyVerticalAngleCurrent;

    unsigned int _viewTex[2];
    int _viewTexWidth[2], _viewTexHeight[2];
    unsigned int _quadVao;
    QOpenGLShaderProgram _prg;

public:
    Widget(OutputMode outputMode, QWidget* parent = nullptr);

    bool isOpenGLStereo() const;
    OutputMode outputMode() const;
    void setOutputMode(OutputMode mode);

    virtual QSize sizeHint() const override;
    virtual void initializeGL() override;
    virtual void paintGL() override;
    virtual void resizeGL(int w, int h) override;

    virtual void keyPressEvent(QKeyEvent* e) override;
    virtual void mousePressEvent(QMouseEvent* e) override;
    virtual void mouseReleaseEvent(QMouseEvent* e) override;
    virtual void mouseMoveEvent(QMouseEvent* e) override;

public slots:
    void mediaChanged(PlaylistEntry entry);

signals:
    void toggleFullscreen();
};
