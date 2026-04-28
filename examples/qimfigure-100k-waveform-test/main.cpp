#include "QImFigureWidget.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotLineItemNode.h"
#include "plot/QImPlotNode.h"

#include <QApplication>
#include <QMainWindow>
#include <QOpenGLContext>
#include <QSurfaceFormat>

#include <cmath>
#include <vector>

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
    window.setWindowTitle("QIm Figure 100K Waveform Test");

    QIM::QImFigureWidget* figure = new QIM::QImFigureWidget(&window);
    figure->setRenderMode(QIM::QImWidget::RenderOnDemand);
    figure->setSubplotGrid(1, 1);
    window.setCentralWidget(figure);

    if (QIM::QImPlotNode* plot = figure->createPlotNode()) {
        plot->setTitle("100000 Point Waveform");
        plot->x1Axis()->setLabel("time");
        plot->y1Axis()->setLabel("amplitude");
        plot->setLegendEnabled(true);

        constexpr int pointCount = 10000000;
        constexpr double sampleStep = 0.0001;
        std::vector< double > xData;
        std::vector< double > yData;
        xData.reserve(pointCount);
        yData.reserve(pointCount);

        for (int i = 0; i < pointCount; ++i) {
            const double t = static_cast< double >(i) * sampleStep;
            xData.push_back(t);
            yData.push_back(std::sin(2.0 * M_PI * 7.0 * t));
        }

        QIM::QImPlotLineItemNode* line = plot->addLine(xData, yData, "sine wave");
        line->setAdaptiveSampling(true);
        line->setDownsampleThreshold(12000);
    }

    window.resize(1280, 720);
    window.show();
    return app.exec();
}
