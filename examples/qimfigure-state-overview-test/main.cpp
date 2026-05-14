#include <QApplication>
#include <QMainWindow>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include "QImFigureWidget.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotNode.h"
#include "plot/QImPlotStateOverviewItemNode.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QSurfaceFormat glFormat;
    if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL) {
        glFormat.setVersion(3, 3);
        glFormat.setProfile(QSurfaceFormat::CoreProfile);
    }
    QSurfaceFormat::setDefaultFormat(glFormat);

    QMainWindow window;
    window.setWindowTitle("QIm Figure State Overview Test");
    window.resize(1280, 260);

    auto* figure = new QIM::QImFigureWidget(&window);
    figure->setRenderMode(QIM::QImWidget::RenderOnDemand);
    window.setCentralWidget(figure);

    QIM::QImPlotNode* plot = figure->createPlotNode();
    plot->setTitle("Stability State Step Plot2");
    plot->setLegendEnabled(false);
    plot->setMouseTextEnabled(false);
    plot->setMenusEnabled(false);
    plot->setBoxSelectEnabled(false);
    plot->setSize(QSizeF(-1, 170));
    plot->x1Axis()->setLabel("System operation condition");
    plot->x1Axis()->setLimits(0.0, 16.0, QIM::QImPlotCondition::Always);
    plot->x1Axis()->setLock(true);
    plot->y1Axis()->setLabel("State");
    plot->y1Axis()->setLimits(-0.45, 1.45, QIM::QImPlotCondition::Always);
    plot->y1Axis()->setTicks({ 0.0, 1.0 });
    plot->y1Axis()->setTickLabelsEnabled(false);
    plot->y1Axis()->setLock(true);

    QIM::QImPlotStateOverviewItemNode* overview = plot->addStateOverview("stability overview");
    overview->setLineWidth(2.2f);
    overview->setStableLineStyle(Qt::SolidLine);
    overview->setUnstableLineStyle(Qt::DashLine);
    overview->setBoundaryLineStyle(Qt::DashLine);
    overview->addUnstableSegment(0.0, 0.55);
    overview->addStableSegment(0.55, 10.0);
    overview->addUnstableSegment(10.0, 10.85);
    overview->addStableSegment(10.85, 16.0);
    overview->addCaseLabel(0.0, 1.1, "01 Case1");

    window.show();
    return app.exec();
}
