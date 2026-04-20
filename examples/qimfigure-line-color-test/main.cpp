#include <cmath>
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QOpenGLContext>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSignalBlocker>
#include <QSurfaceFormat>
#include <QVBoxLayout>
#include <QWidget>
#include "QImFigureWidget.h"
#include "implot.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotLineItemNode.h"
#include "plot/QImPlotNode.h"

namespace
{
QColor randomColor()
{
    return QColor::fromRgb(QRandomGenerator::global()->bounded(256),
                           QRandomGenerator::global()->bounded(256),
                           QRandomGenerator::global()->bounded(256));
}

class LineAppearanceTestWindow : public QMainWindow
{
public:
    LineAppearanceTestWindow()
    {
        setWindowTitle("QIm Figure Line Appearance Test");
        resize(1280, 860);

        QWidget* central = new QWidget(this);
        QVBoxLayout* root = new QVBoxLayout(central);

        QLabel* tip = new QLabel(
            "This example verifies line color, line width, marker shape, marker size, marker outline width, "
            "marker fill color, marker outline color, and line style.\n"
            "Line styles are implemented here on top of ImPlot draw lists for validation.",
            central);
        tip->setWordWrap(true);
        root->addWidget(tip);

        root->addLayout(createColorRow("Line Color",
                                       [this](const QColor& color) {
                                           if (m_line) {
                                               m_line->setColor(color);
                                               requestFigureRender();
                                           }
                                           updateSummary();
                                       },
                                       true));
        root->addLayout(createColorRow("Marker Fill",
                                       [this](const QColor& color) {
                                           if (m_line) {
                                               m_line->setMarkerFillColor(color);
                                               requestFigureRender();
                                           }
                                           updateSummary();
                                       },
                                       true));
        root->addLayout(createColorRow("Marker Outline",
                                       [this](const QColor& color) {
                                           if (m_line) {
                                               m_line->setMarkerOutlineColor(color);
                                               requestFigureRender();
                                           }
                                           updateSummary();
                                       },
                                       true));

        QHBoxLayout* styleRow = new QHBoxLayout();
        styleRow->addWidget(new QLabel("Line Width", central));
        m_lineWidthSpin = new QDoubleSpinBox(central);
        m_lineWidthSpin->setRange(0.5, 12.0);
        m_lineWidthSpin->setSingleStep(0.5);
        styleRow->addWidget(m_lineWidthSpin);

        styleRow->addWidget(new QLabel("Line Style", central));
        m_lineStyleCombo = new QComboBox(central);
        addLineStyleItem("Solid", Qt::SolidLine);
        addLineStyleItem("Dash", Qt::DashLine);
        addLineStyleItem("Dot", Qt::DotLine);
        addLineStyleItem("DashDot", Qt::DashDotLine);
        addLineStyleItem("DashDotDot", Qt::DashDotDotLine);
        addLineStyleItem("NoPen", Qt::NoPen);
        styleRow->addWidget(m_lineStyleCombo);

        styleRow->addWidget(new QLabel("Marker Shape", central));
        m_markerShapeCombo = new QComboBox(central);
        addMarkerItem("None", ImPlotMarker_None);
        addMarkerItem("Circle", ImPlotMarker_Circle);
        addMarkerItem("Square", ImPlotMarker_Square);
        addMarkerItem("Diamond", ImPlotMarker_Diamond);
        addMarkerItem("Up", ImPlotMarker_Up);
        addMarkerItem("Down", ImPlotMarker_Down);
        addMarkerItem("Left", ImPlotMarker_Left);
        addMarkerItem("Right", ImPlotMarker_Right);
        addMarkerItem("Cross", ImPlotMarker_Cross);
        addMarkerItem("Plus", ImPlotMarker_Plus);
        addMarkerItem("Asterisk", ImPlotMarker_Asterisk);
        styleRow->addWidget(m_markerShapeCombo);

        styleRow->addWidget(new QLabel("Marker Size", central));
        m_markerSizeSpin = new QDoubleSpinBox(central);
        m_markerSizeSpin->setRange(1.0, 16.0);
        m_markerSizeSpin->setSingleStep(0.5);
        styleRow->addWidget(m_markerSizeSpin);

        styleRow->addWidget(new QLabel("Marker Weight", central));
        m_markerWeightSpin = new QDoubleSpinBox(central);
        m_markerWeightSpin->setRange(0.5, 8.0);
        m_markerWeightSpin->setSingleStep(0.5);
        styleRow->addWidget(m_markerWeightSpin);

        QPushButton* resetButton = new QPushButton("Reset Appearance", central);
        styleRow->addWidget(resetButton);
        styleRow->addStretch();
        root->addLayout(styleRow);

        m_summaryLabel = new QLabel(central);
        root->addWidget(m_summaryLabel);

        m_figure = new QIM::QImFigureWidget(central);
        m_figure->setRenderMode(QIM::QImWidget::RenderOnDemand);
        m_figure->setSubplotGrid(1, 1);
        root->addWidget(m_figure, 1);

        setCentralWidget(central);
        setupPlot();
        syncControlsFromLine();
        updateSummary();

        connect(m_lineWidthSpin,
                qOverload< double >(&QDoubleSpinBox::valueChanged),
                this,
                [this](double value) {
                    if (m_line) {
                        m_line->setLineWidth(static_cast< float >(value));
                        requestFigureRender();
                    }
                    updateSummary();
                });
        connect(m_lineStyleCombo,
                qOverload< int >(&QComboBox::currentIndexChanged),
                this,
                [this](int index) {
                    if (m_line) {
                        m_line->setLineStyle(m_lineStyleCombo->itemData(index).toInt());
                        requestFigureRender();
                    }
                    updateSummary();
                });
        connect(m_markerShapeCombo,
                qOverload< int >(&QComboBox::currentIndexChanged),
                this,
                [this](int index) {
                    if (m_line) {
                        m_line->setMarkerShape(m_markerShapeCombo->itemData(index).toInt());
                        requestFigureRender();
                    }
                    updateSummary();
                });
        connect(m_markerSizeSpin,
                qOverload< double >(&QDoubleSpinBox::valueChanged),
                this,
                [this](double value) {
                    if (m_line) {
                        m_line->setMarkerSize(static_cast< float >(value));
                        requestFigureRender();
                    }
                    updateSummary();
                });
        connect(m_markerWeightSpin,
                qOverload< double >(&QDoubleSpinBox::valueChanged),
                this,
                [this](double value) {
                    if (m_line) {
                        m_line->setMarkerWeight(static_cast< float >(value));
                        requestFigureRender();
                    }
                    updateSummary();
                });
        connect(resetButton, &QPushButton::clicked, this, [this]() {
            applyDefaultAppearance();
            syncControlsFromLine();
            requestFigureRender();
            updateSummary();
        });
    }

private:
    template< typename ApplyFn >
    QHBoxLayout* createColorRow(const QString& title, ApplyFn applyFn, bool includeRandom)
    {
        QHBoxLayout* row = new QHBoxLayout();
        row->addWidget(new QLabel(title, this));

        auto addButton = [&](const QString& text, const QColor& color) {
            QPushButton* button = new QPushButton(text, this);
            connect(button, &QPushButton::clicked, this, [applyFn, color]() { applyFn(color); });
            row->addWidget(button);
        };

        addButton("Red", QColor(220, 40, 40));
        addButton("Green", QColor(40, 170, 80));
        addButton("Blue", QColor(50, 110, 220));
        addButton("Magenta", QColor(180, 60, 200));
        addButton("Orange", QColor(230, 140, 30));
        if (includeRandom) {
            QPushButton* randomButton = new QPushButton("Random", this);
            connect(randomButton, &QPushButton::clicked, this, [applyFn]() { applyFn(randomColor()); });
            row->addWidget(randomButton);
        }
        row->addStretch();
        return row;
    }

    void addMarkerItem(const QString& text, int marker)
    {
        m_markerShapeCombo->addItem(text, marker);
    }

    void addLineStyleItem(const QString& text, int style)
    {
        m_lineStyleCombo->addItem(text, style);
    }

    void setupPlot()
    {
        QIM::QImPlotNode* plot = m_figure->createPlotNode();
        if (!plot) {
            return;
        }

        plot->setTitle("Line Appearance Update Test");
        plot->setLegendEnabled(true);
        plot->x1Axis()->setLabel("x");
        plot->y1Axis()->setLabel("y");

        std::vector< double > x(600);
        std::vector< double > y(600);
        for (int i = 0; i < static_cast< int >(x.size()); ++i) {
            x[ i ] = i * 0.02;
            y[ i ] = std::sin(x[ i ]) + 0.15 * std::cos(x[ i ] * 3.0);
        }

        m_line = plot->addLine(x, y, "appearance test line");
        applyDefaultAppearance();
    }

    void applyDefaultAppearance()
    {
        if (!m_line) {
            return;
        }
        m_line->setColor(QColor(220, 40, 40));
        m_line->setLineStyle(Qt::SolidLine);
        m_line->setLineWidth(2.5f);
        m_line->setMarkerShape(ImPlotMarker_Circle);
        m_line->setMarkerSize(6.0f);
        m_line->setMarkerWeight(1.5f);
        m_line->setMarkerFillColor(QColor(255, 215, 0));
        m_line->setMarkerOutlineColor(QColor(40, 40, 40));
    }

    void syncControlsFromLine()
    {
        if (!m_line) {
            return;
        }

        QSignalBlocker blockWidth(m_lineWidthSpin);
        QSignalBlocker blockLineStyle(m_lineStyleCombo);
        QSignalBlocker blockShape(m_markerShapeCombo);
        QSignalBlocker blockSize(m_markerSizeSpin);
        QSignalBlocker blockWeight(m_markerWeightSpin);

        m_lineWidthSpin->setValue(m_line->lineWidth());
        m_markerSizeSpin->setValue(m_line->markerSize());
        m_markerWeightSpin->setValue(m_line->markerWeight());

        const int marker = m_line->markerShape();
        for (int i = 0; i < m_markerShapeCombo->count(); ++i) {
            if (m_markerShapeCombo->itemData(i).toInt() == marker) {
                m_markerShapeCombo->setCurrentIndex(i);
                break;
            }
        }
        const int lineStyle = m_line->lineStyle();
        for (int i = 0; i < m_lineStyleCombo->count(); ++i) {
            if (m_lineStyleCombo->itemData(i).toInt() == lineStyle) {
                m_lineStyleCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    void requestFigureRender()
    {
        if (m_figure) {
            m_figure->requestRender();
        }
    }

    void updateSummary()
    {
        if (!m_line) {
            m_summaryLabel->clear();
            return;
        }

        m_summaryLabel->setText(
            QString("Line color: %1 | style: %2 | width: %3 | marker: %4 | size: %5 | weight: %6 | fill: %7 | outline: %8")
                .arg(m_line->color().name(QColor::HexRgb))
                .arg(m_lineStyleCombo->currentText())
                .arg(m_line->lineWidth(), 0, 'f', 1)
                .arg(m_markerShapeCombo->currentText())
                .arg(m_line->markerSize(), 0, 'f', 1)
                .arg(m_line->markerWeight(), 0, 'f', 1)
                .arg(m_line->markerFillColor().name(QColor::HexRgb))
                .arg(m_line->markerOutlineColor().name(QColor::HexRgb)));
    }

    QIM::QImFigureWidget* m_figure { nullptr };
    QIM::QImPlotLineItemNode* m_line { nullptr };
    QLabel* m_summaryLabel { nullptr };
    QDoubleSpinBox* m_lineWidthSpin { nullptr };
    QComboBox* m_lineStyleCombo { nullptr };
    QComboBox* m_markerShapeCombo { nullptr };
    QDoubleSpinBox* m_markerSizeSpin { nullptr };
    QDoubleSpinBox* m_markerWeightSpin { nullptr };
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

    LineAppearanceTestWindow window;
    window.show();
    return app.exec();
}
