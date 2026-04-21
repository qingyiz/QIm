#include "QImFigureWidget.h"
#include "QImPlotTheme.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotNode.h"

#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QVBoxLayout>
#include <QWidget>

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
    window.setWindowTitle("QIm Figure Borderless Test");

    QWidget* central = new QWidget(&window);
    QVBoxLayout* layout = new QVBoxLayout(central);

    QCheckBox* hideWidgetBorderCheck = new QCheckBox("Hide Widget Border", central);
    hideWidgetBorderCheck->setChecked(true);
    layout->addWidget(hideWidgetBorderCheck);

    QCheckBox* hidePlotBorderCheck = new QCheckBox("Hide Plot Border", central);
    hidePlotBorderCheck->setChecked(true);
    layout->addWidget(hidePlotBorderCheck);

    QCheckBox* zeroMarginsCheck = new QCheckBox("Zero Host Margins", central);
    zeroMarginsCheck->setChecked(false);
    layout->addWidget(zeroMarginsCheck);

    QIM::QImFigureWidget* figure = new QIM::QImFigureWidget(central);
    figure->setRenderMode(QIM::QImWidget::RenderOnDemand);
    figure->setSubplotGrid(1, 1);
    layout->addWidget(figure, 1);

    QIM::QImPlotNode* plot = figure->createPlotNode();
    if (plot) {
        plot->setTitle("Border Toggle Plot");
        plot->x1Axis()->setLabel("x");
        plot->y1Axis()->setLabel("sin(x)");
        plot->setLegendEnabled(false);

        std::vector< double > x;
        std::vector< double > y;
        x.reserve(300);
        y.reserve(300);
        for (int i = 0; i < 300; ++i) {
            const double value = i * 2.0 * M_PI / 299.0;
            x.push_back(value);
            y.push_back(std::sin(value));
        }
        plot->addLine(x, y, "sin(x)");
    }

    const auto applyPlotBorderVisible = [figure, plot](bool hideBorder) {
        QIM::QImPlotTheme theme = figure->plotTheme();
        theme.setPlotBorderColor(hideBorder ? Qt::transparent : QColor(80, 80, 80, 180));
        figure->setPlotTheme(theme);
        if (plot) {
            plot->setFrameEnabled(!hideBorder);
        }
        figure->requestRender();
    };

    const auto applyWidgetBorderVisible = [figure](bool hideBorder) {
        figure->setHostWindowBorderVisible(!hideBorder);
    };

    QObject::connect(hideWidgetBorderCheck, &QCheckBox::toggled, figure, applyWidgetBorderVisible);
    QObject::connect(hidePlotBorderCheck, &QCheckBox::toggled, figure, applyPlotBorderVisible);
    applyWidgetBorderVisible(hideWidgetBorderCheck->isChecked());
    applyPlotBorderVisible(hidePlotBorderCheck->isChecked());

    const auto applyHostMargins = [layout, figure](bool zeroMargins) {
        layout->setContentsMargins(zeroMargins ? 0 : 9, zeroMargins ? 0 : 9, zeroMargins ? 0 : 9, zeroMargins ? 0 : 9);
        layout->setSpacing(zeroMargins ? 0 : 6);
        figure->requestRender();
    };
    QObject::connect(zeroMarginsCheck, &QCheckBox::toggled, figure, applyHostMargins);
    applyHostMargins(zeroMarginsCheck->isChecked());

    window.setCentralWidget(central);
    window.resize(960, 640);
    window.show();
    return app.exec();
}
