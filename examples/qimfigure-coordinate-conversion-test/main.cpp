#include "QImFigureWidget.h"
#include "plot/QImPlot3DNode.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotLineItemNode.h"
#include "plot/QImPlotNode.h"
#include "plot/QImPlotScatterItemNode.h"

#include "implot.h"
#include "implot3d.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QOpenGLContext>
#include <QPushButton>
#include <QSurfaceFormat>
#include <QVBoxLayout>
#include <QWidget>

#include <array>
#include <cmath>
#include <vector>

namespace
{
void populate2DPlot(QIM::QImPlotNode* plot, int index)
{
    if (!plot) {
        return;
    }

    plot->setTitle(QString("2D Plot %1").arg(index + 1));
    plot->setLegendEnabled(true);
    plot->x1Axis()->setLabel("x");
    plot->y1Axis()->setLabel("y");

    std::vector< double > xs;
    std::vector< double > ys;
    xs.reserve(240);
    ys.reserve(240);
    for (int i = 0; i < 240; ++i) {
        const double x = static_cast< double >(i) * 4.0 * M_PI / 239.0;
        xs.push_back(x);
        ys.push_back(std::sin(x + index * 0.8));
    }

    auto* line = new QIM::QImPlotLineItemNode(plot);
    line->setLabel(QString("sin %1").arg(index + 1));
    line->setData(xs, ys);
    line->setColor(QColor::fromHsv((index * 75 + 205) % 360, 180, 210));
    line->setLineWidth(2.0f);
    line->setMarkerShape(ImPlotMarker_Circle);
    line->setMarkerSize(3.0f);

    std::vector< double > scatterX;
    std::vector< double > scatterY;
    for (int i = 0; i < 24; ++i) {
        const int sourceIndex = i * 10;
        scatterX.push_back(xs[ sourceIndex ]);
        scatterY.push_back(ys[ sourceIndex ]);
    }
    auto* scatter = new QIM::QImPlotScatterItemNode(plot);
    scatter->setLabel(QString("samples %1").arg(index + 1));
    scatter->setData(scatterX, scatterY);
    scatter->setColor(QColor::fromHsv((index * 75 + 25) % 360, 170, 230));
    scatter->setMarkerShape(ImPlotMarker_Diamond);
    scatter->setMarkerSize(5.0f);
}
}  // namespace

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
    window.setWindowTitle("QIm Coordinate Conversion Test");

    auto* central = new QWidget(&window);
    auto* layout = new QVBoxLayout(central);
    auto* controls = new QWidget(central);
    auto* controlsLayout = new QHBoxLayout(controls);
    auto* figure = new QIM::QImFigureWidget(central);
    figure->setRenderMode(QIM::QImWidget::RenderOnDemand);
    figure->setSubplotGrid(1, 3);

    std::array< QIM::QImAbstractNode*, 3 > coordinateNodes { nullptr, nullptr, nullptr };
    std::array< QPushButton*, 3 > buttons { nullptr, nullptr, nullptr };

    for (int index = 0; index < 3; ++index) {
        QIM::QImPlotNode* plot = figure->createPlotNode();
        coordinateNodes[ index ] = plot;
        populate2DPlot(plot, index);

        auto* button = new QPushButton(QString("Plot %1: 2D -> 3D").arg(index + 1), controls);
        buttons[ index ] = button;
        controlsLayout->addWidget(button);

        QObject::connect(button, &QPushButton::clicked, figure, [figure, &coordinateNodes, &buttons, index]() {
            QIM::QImAbstractNode* current = coordinateNodes[ index ];
            if (QIM::QImPlotNode* plot2D = qobject_cast< QIM::QImPlotNode* >(current)) {
                QIM::QImPlot3DNode* plot3D = figure->convertPlotNodeTo3D(plot2D);
                if (!plot3D) {
                    return;
                }
                coordinateNodes[ index ] = plot3D;
                buttons[ index ]->setText(QString("Plot %1: 3D -> 2D").arg(index + 1));
                return;
            }

            if (QIM::QImPlot3DNode* plot3D = qobject_cast< QIM::QImPlot3DNode* >(current)) {
                QIM::QImPlotNode* plot2D = figure->convertPlot3DNodeTo2D(plot3D);
                if (!plot2D) {
                    return;
                }
                coordinateNodes[ index ] = plot2D;
                buttons[ index ]->setText(QString("Plot %1: 2D -> 3D").arg(index + 1));
            }
        });
    }

    layout->addWidget(controls);
    layout->addWidget(figure, 1);
    window.setCentralWidget(central);
    window.resize(1280, 720);
    window.show();
    return app.exec();
}
