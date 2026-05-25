#include "QImFigureWidget.h"
#include "plot/QImPlot3DLineItemNode.h"
#include "plot/QImPlot3DNode.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotLineItemNode.h"
#include "plot/QImPlotNode.h"
#include "plot/QImPlotScatterItemNode.h"

#include "implot.h"
#include "implot3d.h"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QOpenGLContext>
#include <QPushButton>
#include <QSurfaceFormat>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>
#include <vector>

namespace
{
QFrame* createDropZone(const QString& title, QWidget* parent)
{
    QFrame* frame = new QFrame(parent);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setObjectName(title == "Left Parent" ? "leftParent" : "rightParent");
    frame->setStyleSheet(
        "QFrame#leftParent, QFrame#rightParent {"
        "  background: #f6f7fb;"
        "  border: 1px solid #bfc7d5;"
        "}"
        "QLabel { color: #374151; }"
    );

    QVBoxLayout* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    QLabel* label = new QLabel(title, frame);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    return frame;
}

QWidget* createHostWindow(const QString& title, QWidget*& figureHost)
{
    QWidget* window = new QWidget();
    window->setWindowTitle(title);

    QVBoxLayout* layout = new QVBoxLayout(window);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    figureHost = createDropZone(title + " Parent", window);
    layout->addWidget(figureHost, 1);
    window->resize(760, 560);
    return window;
}

QIM::QImFigureWidget* createFigure(QWidget* parent)
{
    auto* figure = new QIM::QImFigureWidget(parent);
    figure->setRenderMode(QIM::QImWidget::RenderOnDemand);
    figure->setSubplotGrid(1, 2);
    figure->setMinimumSize(720, 420);

    if (QIM::QImPlotNode* plot = figure->createPlotNode({ 1 })) {
        plot->setTitle("2D Plot: right click here");
        plot->x1Axis()->setLabel("x");
        plot->y1Axis()->setLabel("sin(x)");
        plot->setLegendEnabled(true);
        plot->setMenusEnabled(true);

        std::vector< double > xs;
        std::vector< double > ys;
        xs.reserve(360);
        ys.reserve(360);
        for (int i = 0; i < 360; ++i) {
            const double x = static_cast< double >(i) * 2.0 * M_PI / 359.0;
            xs.push_back(x);
            ys.push_back(std::sin(x));
        }

        auto* line = new QIM::QImPlotLineItemNode(plot);
        line->setLabel("sin(x)");
        line->setData(xs, ys);
        line->setColor(QColor(0, 114, 189));
        line->setLineWidth(2.0f);

        std::vector< double > sampleX { 0.5, 1.5, 2.5, 3.5, 4.5, 5.5 };
        std::vector< double > sampleY { 0.48, 0.98, 0.59, -0.35, -0.98, -0.71 };
        auto* scatter = new QIM::QImPlotScatterItemNode(plot);
        scatter->setLabel("samples");
        scatter->setData(sampleX, sampleY);
        scatter->setMarkerShape(ImPlotMarker_Circle);
        scatter->setMarkerSize(5.0f);
        scatter->setMarkerFill(true);
        scatter->setColor(QColor(217, 83, 25));
    }

    if (QIM::QImPlot3DNode* plot = figure->createPlot3DNode({ 2 })) {
        plot->setTitle("3D Plot: check axes after reparent");
        plot->setXAxisLabel("X");
        plot->setYAxisLabel("Y");
        plot->setZAxisLabel("Z");
        plot->setLegendEnabled(true);
        plot->setMenusEnabled(true);
        plot->setEqual(true);

        std::vector< double > xs;
        std::vector< double > ys;
        std::vector< double > zs;
        xs.reserve(240);
        ys.reserve(240);
        zs.reserve(240);
        for (int i = 0; i < 240; ++i) {
            const double t = static_cast< double >(i) * 0.08;
            xs.push_back(std::cos(t));
            ys.push_back(std::sin(t));
            zs.push_back(t * 0.15);
        }

        auto* line = new QIM::QImPlot3DLineItemNode(plot);
        line->setLabel("helix");
        line->setData(xs, ys, zs);
        line->setColor(QColor(46, 160, 120));
        line->setLineWidth(2.0f);
        line->setMarkerShape(ImPlot3DMarker_Circle);
        line->setMarkerSize(3.0f);
    }

    return figure;
}
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QSurfaceFormat glFormat;
    if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL) {
        glFormat.setVersion(3, 3);
        glFormat.setProfile(QSurfaceFormat::CoreProfile);
    }
    QSurfaceFormat::setDefaultFormat(glFormat);

    QWidget* leftHost = nullptr;
    QWidget* rightHost = nullptr;
    QWidget* leftWindow = createHostWindow("QImFigureWidget Reparent Repro - Window A", leftHost);
    QWidget* rightWindow = createHostWindow("QImFigureWidget Reparent Repro - Window B", rightHost);

    QWidget* controlWindow = new QWidget();
    controlWindow->setWindowTitle("QImFigureWidget Reparent Controls");
    QVBoxLayout* rootLayout = new QVBoxLayout(controlWindow);
    rootLayout->setContentsMargins(10, 10, 10, 10);
    rootLayout->setSpacing(10);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* moveLeftButton = new QPushButton("Move to Window A Parent", controlWindow);
    QPushButton* moveRightButton = new QPushButton("Move to Window B Parent", controlWindow);
    QPushButton* toggleButton = new QPushButton("Toggle Parent Window", controlWindow);
    buttonLayout->addWidget(moveLeftButton);
    buttonLayout->addWidget(moveRightButton);
    buttonLayout->addWidget(toggleButton);
    rootLayout->addLayout(buttonLayout);

    QLabel* hint = new QLabel("The same QImFigureWidget is reparented between two independent top-level windows.", controlWindow);
    hint->setWordWrap(true);
    rootLayout->addWidget(hint);

    QIM::QImFigureWidget* figure = createFigure(leftHost);
    auto* leftLayout = qobject_cast< QVBoxLayout* >(leftHost->layout());
    auto* rightLayout = qobject_cast< QVBoxLayout* >(rightHost->layout());
    leftLayout->addWidget(figure, 1);

    QWidget* currentParent = leftHost;
    const auto moveFigure = [&currentParent, figure, leftHost, rightHost, leftLayout, rightLayout](QWidget* targetParent) {
        if (currentParent == targetParent) {
            return;
        }

        QWidget* oldParent = figure->parentWidget();
        if (oldParent) {
            oldParent->setUpdatesEnabled(false);
        }
        targetParent->setUpdatesEnabled(false);

        if (oldParent && oldParent->layout()) {
            oldParent->layout()->removeWidget(figure);
        }
        figure->setParent(targetParent);
        if (targetParent == leftHost) {
            leftLayout->addWidget(figure, 1);
        } else {
            rightLayout->addWidget(figure, 1);
        }
        currentParent = targetParent;
        figure->setVisible(true);

        if (oldParent) {
            oldParent->setUpdatesEnabled(true);
            oldParent->update();
        }
        targetParent->setUpdatesEnabled(true);
        targetParent->update();

        QTimer::singleShot(0, figure, [figure]() {
            figure->requestRender();
        });
        QTimer::singleShot(16, figure, [figure]() {
            figure->requestRender();
        });
    };

    QObject::connect(moveLeftButton, &QPushButton::clicked, figure, [moveFigure, leftHost]() mutable {
        moveFigure(leftHost);
    });
    QObject::connect(moveRightButton, &QPushButton::clicked, figure, [moveFigure, rightHost]() mutable {
        moveFigure(rightHost);
    });
    QObject::connect(toggleButton, &QPushButton::clicked, figure, [moveFigure, leftHost, rightHost, &currentParent]() mutable {
        moveFigure(currentParent == leftHost ? rightHost : leftHost);
    });

    leftWindow->move(80, 120);
    rightWindow->move(880, 120);
    controlWindow->move(420, 40);
    leftWindow->show();
    rightWindow->show();
    controlWindow->show();
    return app.exec();
}
