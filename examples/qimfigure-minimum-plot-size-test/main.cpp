#include "QImFigureWidget.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotNode.h"

#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QOpenGLContext>
#include <QSizeF>
#include <QSpinBox>
#include <QSurfaceFormat>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>
#include <vector>

namespace
{
void fillFigure(QIM::QImFigureWidget* figure, int rows, int cols)
{
    figure->setSubplotGrid(rows, cols);

    const int plotCount = rows * cols;
    for (int index = 0; index < plotCount; ++index) {
        QIM::QImPlotNode* plot = figure->createPlotNode();
        if (!plot) {
            break;
        }

        plot->setTitle(QString("Plot %1").arg(index + 1));
        plot->setLegendEnabled(false);
        plot->x1Axis()->setLabel("x");
        plot->y1Axis()->setLabel("y");

        std::vector< double > xData;
        std::vector< double > yData;
        xData.reserve(120);
        yData.reserve(120);

        const double phase = static_cast< double >(index) * 0.17;
        for (int i = 0; i < 120; ++i) {
            const double x = static_cast< double >(i) * 2.0 * M_PI / 119.0;
            xData.push_back(x);
            yData.push_back(std::sin(x + phase) * std::cos(phase * 0.5));
        }
        plot->addLine(xData, yData, "wave");
    }
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

    QMainWindow window;
    window.setWindowTitle("QIm Figure Minimum Plot Size Test");

    QWidget* central = new QWidget(&window);
    QVBoxLayout* layout = new QVBoxLayout(central);

    QWidget* controls = new QWidget(central);
    QHBoxLayout* controlsLayout = new QHBoxLayout(controls);
    controlsLayout->setContentsMargins(0, 0, 0, 0);

    QCheckBox* minimumSizeCheck = new QCheckBox("Minimum plot size", controls);
    minimumSizeCheck->setChecked(true);
    controlsLayout->addWidget(minimumSizeCheck);

    controlsLayout->addWidget(new QLabel("Width", controls));
    QSpinBox* widthSpin = new QSpinBox(controls);
    widthSpin->setRange(0, 600);
    widthSpin->setValue(220);
    widthSpin->setSuffix(" px");
    controlsLayout->addWidget(widthSpin);

    controlsLayout->addWidget(new QLabel("Height", controls));
    QSpinBox* heightSpin = new QSpinBox(controls);
    heightSpin->setRange(0, 500);
    heightSpin->setValue(160);
    heightSpin->setSuffix(" px");
    controlsLayout->addWidget(heightSpin);

    controlsLayout->addStretch();
    layout->addWidget(controls);

    QIM::QImFigureWidget* figure = new QIM::QImFigureWidget(central);
    figure->setRenderMode(QIM::QImWidget::RenderOnDemand);
    figure->setMinimumPlotSize(QSizeF(widthSpin->value(), heightSpin->value()));
    layout->addWidget(figure, 1);

    fillFigure(figure, 16, 16);

    const auto applyMinimumPlotSize = [figure, minimumSizeCheck, widthSpin, heightSpin]() {
        const QSizeF size =
            minimumSizeCheck->isChecked() ? QSizeF(widthSpin->value(), heightSpin->value()) : QSizeF(0, 0);
        figure->setMinimumPlotSize(size);
        figure->requestRender();
    };

    QObject::connect(minimumSizeCheck, &QCheckBox::toggled, figure, applyMinimumPlotSize);
    QObject::connect(widthSpin, qOverload< int >(&QSpinBox::valueChanged), figure, applyMinimumPlotSize);
    QObject::connect(heightSpin, qOverload< int >(&QSpinBox::valueChanged), figure, applyMinimumPlotSize);

    window.setCentralWidget(central);
    window.resize(1100, 760);
    window.show();
    return app.exec();
}
