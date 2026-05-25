#include "QImFigureWidget.h"
#include "plot/QImPlot3DLineItemNode.h"
#include "plot/QImPlot3DNode.h"
#include "plot/QImPlot3DScatterItemNode.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotBarsItemNode.h"
#include "plot/QImPlotLineItemNode.h"
#include "plot/QImPlotNode.h"
#include "plot/QImPlotScatterItemNode.h"

#include "implot.h"
#include "implot3d.h"

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
    window.setWindowTitle("QIm Unified 2D + 3D Figure Example");

    auto* figure = new QIM::QImFigureWidget(&window);
    figure->setRenderMode(QIM::QImWidget::RenderOnDemand);

    // 2D 和 3D 共用同一个 QImFigureWidget 和同一份 subplot 网格配置。
    // 这里演示两个 2D 图分别占用上方 {1} 和 {2}，3D 图占用下方 {3, 4} 两个网格。
    figure->setSubplotGrid(2, 2);
    window.setCentralWidget(figure);

    if (QIM::QImPlotNode* plot = figure->createPlotNode({ 1 })) {
        plot->setTitle("2D Sine");
        plot->x1Axis()->setLabel("x");
        plot->y1Axis()->setLabel("sin(x)");
        plot->setLegendEnabled(true);

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
    }

    if (QIM::QImPlotNode* plot = figure->createPlotNode({ 2 })) {
        plot->setTitle("2D Bars + Scatter");
        plot->x1Axis()->setLabel("index");
        plot->y1Axis()->setLabel("value");
        plot->setLegendEnabled(true);

        std::vector< double > x { 1.0, 2.0, 3.0, 4.0, 5.0 };
        std::vector< double > barsY { 3.0, 5.0, 4.0, 6.0, 5.5 };
        auto* bars = new QIM::QImPlotBarsItemNode(plot);
        bars->setLabel("bars");
        bars->setData(x, barsY);
        bars->setBarWidth(0.45);
        bars->setColor(QColor(80, 170, 90));

        std::vector< double > scatterY { 3.4, 4.7, 4.5, 5.7, 6.1 };
        auto* scatter = new QIM::QImPlotScatterItemNode(plot);
        scatter->setLabel("samples");
        scatter->setData(x, scatterY);
        scatter->setMarkerSize(5.0f);
        scatter->setMarkerShape(ImPlotMarker_Circle);
        scatter->setMarkerFill(true);
        scatter->setColor(QColor(217, 83, 25));
    }

    if (QIM::QImPlot3DNode* plot = figure->createPlot3DNode({ 3, 4 })) {
        plot->setTitle("3D Helix");
        plot->setXAxisLabel("X");
        plot->setYAxisLabel("Y");
        plot->setZAxisLabel("Z");
        plot->setLegendEnabled(true);
        plot->setEqual(true);

        std::vector< double > xs;
        std::vector< double > ys;
        std::vector< double > zs;
        std::vector< double > xsOuter;
        std::vector< double > ysOuter;
        std::vector< double > zsOuter;
        std::vector< double > xsWave;
        std::vector< double > ysWave;
        std::vector< double > zsWave;
        xs.reserve(240);
        ys.reserve(240);
        zs.reserve(240);
        xsOuter.reserve(240);
        ysOuter.reserve(240);
        zsOuter.reserve(240);
        xsWave.reserve(240);
        ysWave.reserve(240);
        zsWave.reserve(240);
        for (int i = 0; i < 240; ++i) {
            const double t = static_cast< double >(i) * 0.08;
            xs.push_back(std::cos(t));
            ys.push_back(std::sin(t));
            zs.push_back(t * 0.2);

            xsOuter.push_back(1.35 * std::cos(t + 0.45));
            ysOuter.push_back(1.35 * std::sin(t + 0.45));
            zsOuter.push_back(t * 0.2 + 0.25);

            xsWave.push_back(0.65 * std::cos(t));
            ysWave.push_back(0.65 * std::sin(t));
            zsWave.push_back(t * 0.2 + 0.35 * std::sin(t * 1.7));
        }

        auto* line = new QIM::QImPlot3DLineItemNode(plot);
        line->setLabel("helix");
        line->setData(xs, ys, zs);
        line->setColor(QColor(33, 150, 243));
        line->setLineWidth(2.0f);

        auto* dashedLine = new QIM::QImPlot3DLineItemNode(plot);
        dashedLine->setLabel("offset dashed");
        dashedLine->setData(xsOuter, ysOuter, zsOuter);
        dashedLine->setColor(QColor(230, 126, 34));
        dashedLine->setLineStyle(Qt::DashLine);
        dashedLine->setLineWidth(2.5f);
        dashedLine->setMarkerShape(ImPlot3DMarker_Square);
        dashedLine->setMarkerSize(3.5f);
        dashedLine->setMarkerWeight(1.2f);
        dashedLine->setMarkerFillColor(QColor(255, 205, 120));
        dashedLine->setMarkerOutlineColor(QColor(180, 82, 20));

        auto* dashDotLine = new QIM::QImPlot3DLineItemNode(plot);
        dashDotLine->setLabel("wavy dash-dot");
        dashDotLine->setData(xsWave, ysWave, zsWave);
        dashDotLine->setColor(QColor(46, 160, 120));
        dashDotLine->setLineStyle(Qt::DashDotLine);
        dashDotLine->setLineWidth(2.0f);
        dashDotLine->setMarkerShape(ImPlot3DMarker_Diamond);
        dashDotLine->setMarkerSize(4.0f);
        dashDotLine->setMarkerWeight(1.4f);
        dashDotLine->setMarkerFillColor(QColor(150, 230, 190));
        dashDotLine->setMarkerOutlineColor(QColor(20, 110, 82));
    }

    window.resize(1280, 900);
    window.show();
    return app.exec();
}
