#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QOpenGLContext>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSurfaceFormat>
#include <QVBoxLayout>
#include <QWidget>
#include "QImFigureWidget.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotLineItemNode.h"
#include "plot/QImPlotNode.h"

namespace
{
class LineColorTestWindow : public QMainWindow
{
public:
    LineColorTestWindow()
    {
        setWindowTitle("QIm Figure Line Color Test");
        resize(1200, 800);

        QWidget* central = new QWidget(this);
        QVBoxLayout* root = new QVBoxLayout(central);
        QHBoxLayout* controls = new QHBoxLayout();

        QLabel* tip = new QLabel("Click a button to change the line color.", central);
        controls->addWidget(tip);

        auto* redButton = new QPushButton("Red", central);
        auto* greenButton = new QPushButton("Green", central);
        auto* blueButton = new QPushButton("Blue", central);
        auto* magentaButton = new QPushButton("Magenta", central);
        auto* randomButton = new QPushButton("Random", central);
        m_colorLabel = new QLabel(central);

        controls->addWidget(redButton);
        controls->addWidget(greenButton);
        controls->addWidget(blueButton);
        controls->addWidget(magentaButton);
        controls->addWidget(randomButton);
        controls->addStretch();
        controls->addWidget(m_colorLabel);

        m_figure = new QIM::QImFigureWidget(central);
        m_figure->setRenderMode(QIM::QImWidget::RenderOnDemand);
        m_figure->setSubplotGrid(1, 1);

        root->addLayout(controls);
        root->addWidget(m_figure, 1);
        setCentralWidget(central);

        setupPlot();

        connect(redButton, &QPushButton::clicked, this, [this]() { applyColor(QColor(220, 40, 40)); });
        connect(greenButton, &QPushButton::clicked, this, [this]() { applyColor(QColor(40, 170, 80)); });
        connect(blueButton, &QPushButton::clicked, this, [this]() { applyColor(QColor(50, 110, 220)); });
        connect(magentaButton, &QPushButton::clicked, this, [this]() { applyColor(QColor(180, 60, 200)); });
        connect(randomButton, &QPushButton::clicked, this, [this]() {
            applyColor(QColor::fromRgb(
                QRandomGenerator::global()->bounded(256),
                QRandomGenerator::global()->bounded(256),
                QRandomGenerator::global()->bounded(256)));
        });
    }

private:
    void setupPlot()
    {
        QIM::QImPlotNode* plot = m_figure->createPlotNode();
        if (!plot) {
            return;
        }

        plot->setTitle("Line Color Update Test");
        plot->setLegendEnabled(true);
        plot->x1Axis()->setLabel("x");
        plot->y1Axis()->setLabel("y");

        std::vector< double > x(600);
        std::vector< double > y(600);
        for (int i = 0; i < static_cast< int >(x.size()); ++i) {
            x[ i ] = i * 0.02;
            y[ i ] = std::sin(x[ i ]) + 0.15 * std::cos(x[ i ] * 3.0);
        }

        m_line = plot->addLine(x, y, "test line");
        applyColor(QColor(220, 40, 40));
    }

    void applyColor(const QColor& color)
    {
        if (!m_line) {
            return;
        }
        m_line->setColor(color);
        m_colorLabel->setText(QString("Current: %1").arg(color.name(QColor::HexRgb)));
        m_figure->requestRender();
    }

    QIM::QImFigureWidget* m_figure { nullptr };
    QIM::QImPlotLineItemNode* m_line { nullptr };
    QLabel* m_colorLabel { nullptr };
};
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

    LineColorTestWindow window;
    window.show();
    return app.exec();
}
