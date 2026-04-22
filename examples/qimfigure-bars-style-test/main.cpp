#include <cmath>
#include <vector>

#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QOpenGLContext>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSurfaceFormat>
#include <QVBoxLayout>
#include <QWidget>

#include "QImFigureWidget.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotBarsItemNode.h"
#include "plot/QImPlotNode.h"

namespace
{
class BarsAppearanceTestWindow : public QMainWindow
{
public:
    BarsAppearanceTestWindow()
    {
        setWindowTitle("QIm Figure Bars Style Test");
        resize(1320, 860);

        m_positions = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
        m_values = {3.8, 5.1, 4.4, 6.2, 4.9, 7.1};

        QWidget* central = new QWidget(this);
        QVBoxLayout* root = new QVBoxLayout(central);

        QLabel* tip = new QLabel(
            "This example verifies QImPlotBarsItemNode appearance controls. "
            "The left plot is interactive and exercises fill color, border color, border width, "
            "fill visibility, border visibility, bar width, horizontal mode, and the legacy setColor API. "
            "The right plot shows several preset combinations for quick visual regression.",
            central);
        tip->setWordWrap(true);
        root->addWidget(tip);

        root->addLayout(createStylePresetRow());
        root->addLayout(createColorRow("Fill Color", [this](const QColor& color) {
            if (!m_mainBars) {
                return;
            }
            m_mainBars->setFillColor(color);
            requestRender();
            updateSummary();
        }));
        root->addLayout(createColorRow("Border Color", [this](const QColor& color) {
            if (!m_mainBars) {
                return;
            }
            m_mainBars->setBorderColor(color);
            requestRender();
            updateSummary();
        }));

        QHBoxLayout* controlRow = new QHBoxLayout();
        m_fillVisibleCheck = new QCheckBox("Fill Visible", central);
        m_borderVisibleCheck = new QCheckBox("Border Visible", central);
        m_horizontalCheck = new QCheckBox("Horizontal", central);
        controlRow->addWidget(m_fillVisibleCheck);
        controlRow->addWidget(m_borderVisibleCheck);
        controlRow->addWidget(m_horizontalCheck);

        controlRow->addWidget(new QLabel("Bar Width", central));
        m_barWidthSpin = new QDoubleSpinBox(central);
        m_barWidthSpin->setRange(0.2, 1.6);
        m_barWidthSpin->setSingleStep(0.05);
        controlRow->addWidget(m_barWidthSpin);

        controlRow->addWidget(new QLabel("Border Width", central));
        m_borderWidthSpin = new QDoubleSpinBox(central);
        m_borderWidthSpin->setRange(0.0, 8.0);
        m_borderWidthSpin->setSingleStep(0.25);
        controlRow->addWidget(m_borderWidthSpin);

        QPushButton* randomizeValuesButton = new QPushButton("Randomize Values", central);
        QPushButton* resetButton = new QPushButton("Reset Interactive Style", central);
        controlRow->addWidget(randomizeValuesButton);
        controlRow->addWidget(resetButton);
        controlRow->addStretch();
        root->addLayout(controlRow);

        m_summaryLabel = new QLabel(central);
        root->addWidget(m_summaryLabel);

        m_figure = new QIM::QImFigureWidget(central);
        m_figure->setRenderMode(QIM::QImWidget::RenderOnDemand);
        m_figure->setSubplotGrid(1, 2);
        root->addWidget(m_figure, 1);

        setCentralWidget(central);

        setupPlots();
        applyDefaultInteractiveStyle();
        syncControlsFromBars();
        updateSummary();

        connect(m_fillVisibleCheck, &QCheckBox::toggled, this, [this](bool checked) {
            if (!m_mainBars) {
                return;
            }
            m_mainBars->setFillVisible(checked);
            requestRender();
            updateSummary();
        });
        connect(m_borderVisibleCheck, &QCheckBox::toggled, this, [this](bool checked) {
            if (!m_mainBars) {
                return;
            }
            m_mainBars->setBorderVisible(checked);
            requestRender();
            updateSummary();
        });
        connect(m_horizontalCheck, &QCheckBox::toggled, this, [this](bool checked) {
            if (!m_mainBars) {
                return;
            }
            m_mainBars->setHorizontal(checked);
            refreshMainBarsData();
            requestRender();
            updateSummary();
        });
        connect(m_barWidthSpin, qOverload< double >(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            if (!m_mainBars) {
                return;
            }
            m_mainBars->setBarWidth(value);
            requestRender();
            updateSummary();
        });
        connect(m_borderWidthSpin, qOverload< double >(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            if (!m_mainBars) {
                return;
            }
            m_mainBars->setBorderWidth(static_cast< float >(value));
            requestRender();
            updateSummary();
        });
        connect(randomizeValuesButton, &QPushButton::clicked, this, [this]() {
            for (std::size_t i = 0; i < m_values.size(); ++i) {
                const double phase = static_cast< double >(i) * 0.7 + m_randomSeed;
                m_values[ i ] = 3.2 + std::abs(std::sin(phase)) * 4.0 + 0.35 * static_cast< double >(i);
            }
            m_randomSeed += 0.8;
            refreshMainBarsData();
            requestRender();
            updateSummary();
        });
        connect(resetButton, &QPushButton::clicked, this, [this]() {
            applyDefaultInteractiveStyle();
            syncControlsFromBars();
            requestRender();
            updateSummary();
        });
    }

private:
    template< typename ApplyFn >
    QHBoxLayout* createColorRow(const QString& title, ApplyFn apply)
    {
        QHBoxLayout* row = new QHBoxLayout();
        row->addWidget(new QLabel(title, this));

        const struct ColorEntry
        {
            const char* label;
            QColor color;
        } entries[] = {
            {"Forest", QColor(80, 170, 90)},
            {"Orange", QColor(217, 119, 6)},
            {"Blue", QColor(59, 130, 246)},
            {"Crimson", QColor(220, 38, 38)},
            {"Slate", QColor(71, 85, 105)},
            {"Transparent", QColor(0, 0, 0, 0)},
        };

        for (const auto& entry : entries) {
            QPushButton* button = new QPushButton(QString::fromUtf8(entry.label), this);
            connect(button, &QPushButton::clicked, this, [apply, entry]() { apply(entry.color); });
            row->addWidget(button);
        }
        row->addStretch();
        return row;
    }

    QHBoxLayout* createStylePresetRow()
    {
        QHBoxLayout* row = new QHBoxLayout();
        row->addWidget(new QLabel("Interactive Presets", this));

        auto addButton = [this, row](const QString& title, const std::function< void() >& apply) {
            QPushButton* button = new QPushButton(title, this);
            connect(button, &QPushButton::clicked, this, [this, apply]() {
                if (!m_mainBars) {
                    return;
                }
                apply();
                syncControlsFromBars();
                requestRender();
                updateSummary();
            });
            row->addWidget(button);
        };

        addButton("Legacy setColor", [this]() {
            m_mainBars->setColor(QColor(46, 204, 113));
            m_mainBars->setBorderWidth(1.0f);
            m_mainBars->setFillVisible(true);
            m_mainBars->setBorderVisible(true);
        });
        addButton("Fill Only", [this]() {
            m_mainBars->setFillColor(QColor(59, 130, 246, 180));
            m_mainBars->setBorderColor(QColor(59, 130, 246, 180));
            m_mainBars->setBorderWidth(0.0f);
            m_mainBars->setFillVisible(true);
            m_mainBars->setBorderVisible(false);
        });
        addButton("Outline Only", [this]() {
            m_mainBars->setFillColor(QColor(0, 0, 0, 0));
            m_mainBars->setBorderColor(QColor(220, 38, 38));
            m_mainBars->setBorderWidth(2.0f);
            m_mainBars->setFillVisible(false);
            m_mainBars->setBorderVisible(true);
        });
        addButton("Bold Border", [this]() {
            m_mainBars->setFillColor(QColor(251, 191, 36, 150));
            m_mainBars->setBorderColor(QColor(120, 53, 15));
            m_mainBars->setBorderWidth(4.0f);
            m_mainBars->setFillVisible(true);
            m_mainBars->setBorderVisible(true);
        });

        row->addStretch();
        return row;
    }

    void setupPlots()
    {
        QIM::QImPlotNode* interactivePlot = m_figure->createPlotNode();
        if (interactivePlot) {
            interactivePlot->setTitle("Interactive Bars");
            interactivePlot->x1Axis()->setLabel("Category / Value");
            interactivePlot->y1Axis()->setLabel("Value / Category");
            interactivePlot->setLegendEnabled(true);

            m_mainBars = new QIM::QImPlotBarsItemNode(interactivePlot);
            m_mainBars->setLabel("interactive");
        }

        QIM::QImPlotNode* presetPlot = m_figure->createPlotNode();
        if (presetPlot) {
            presetPlot->setTitle("Preset Comparison");
            presetPlot->x1Axis()->setLabel("Grouped Categories");
            presetPlot->y1Axis()->setLabel("Value");
            presetPlot->setLegendEnabled(true);

            auto* defaultBars = new QIM::QImPlotBarsItemNode(presetPlot);
            defaultBars->setLabel("default");
            defaultBars->setData(std::vector< double > {1.0, 2.0, 3.0}, std::vector< double > {2.8, 4.1, 3.6});
            defaultBars->setBarWidth(0.55);

            auto* fillOnlyBars = new QIM::QImPlotBarsItemNode(presetPlot);
            fillOnlyBars->setLabel("fill only");
            fillOnlyBars->setData(std::vector< double > {5.0, 6.0, 7.0}, std::vector< double > {3.2, 5.0, 4.2});
            fillOnlyBars->setFillColor(QColor(59, 130, 246, 170));
            fillOnlyBars->setBorderColor(QColor(59, 130, 246, 170));
            fillOnlyBars->setBorderVisible(false);
            fillOnlyBars->setBarWidth(0.55);

            auto* outlineOnlyBars = new QIM::QImPlotBarsItemNode(presetPlot);
            outlineOnlyBars->setLabel("outline only");
            outlineOnlyBars->setData(std::vector< double > {9.0, 10.0, 11.0}, std::vector< double > {2.7, 4.7, 5.4});
            outlineOnlyBars->setFillVisible(false);
            outlineOnlyBars->setBorderColor(QColor(220, 38, 38));
            outlineOnlyBars->setBorderWidth(2.5f);
            outlineOnlyBars->setBarWidth(0.55);

            auto* boldBorderBars = new QIM::QImPlotBarsItemNode(presetPlot);
            boldBorderBars->setLabel("bold border");
            boldBorderBars->setData(std::vector< double > {13.0, 14.0, 15.0}, std::vector< double > {4.2, 3.8, 6.1});
            boldBorderBars->setFillColor(QColor(251, 191, 36, 140));
            boldBorderBars->setBorderColor(QColor(120, 53, 15));
            boldBorderBars->setBorderWidth(4.0f);
            boldBorderBars->setBarWidth(0.55);
        }

        refreshMainBarsData();
    }

    void applyDefaultInteractiveStyle()
    {
        if (!m_mainBars) {
            return;
        }

        m_values = {3.8, 5.1, 4.4, 6.2, 4.9, 7.1};
        m_mainBars->setHorizontal(false);
        m_mainBars->setFillColor(QColor(80, 170, 90, 190));
        m_mainBars->setBorderColor(QColor(28, 93, 42));
        m_mainBars->setBorderWidth(1.5f);
        m_mainBars->setFillVisible(true);
        m_mainBars->setBorderVisible(true);
        m_mainBars->setBarWidth(0.62);
        refreshMainBarsData();
    }

    void refreshMainBarsData()
    {
        if (!m_mainBars) {
            return;
        }

        if (m_mainBars->isHorizontal()) {
            m_mainBars->setData(m_values, m_positions);
        } else {
            m_mainBars->setData(m_positions, m_values);
        }
    }

    void syncControlsFromBars()
    {
        if (!m_mainBars) {
            return;
        }

        const QSignalBlocker fillBlocker(m_fillVisibleCheck);
        const QSignalBlocker borderBlocker(m_borderVisibleCheck);
        const QSignalBlocker horizontalBlocker(m_horizontalCheck);
        const QSignalBlocker barWidthBlocker(m_barWidthSpin);
        const QSignalBlocker borderWidthBlocker(m_borderWidthSpin);

        m_fillVisibleCheck->setChecked(m_mainBars->isFillVisible());
        m_borderVisibleCheck->setChecked(m_mainBars->isBorderVisible());
        m_horizontalCheck->setChecked(m_mainBars->isHorizontal());
        m_barWidthSpin->setValue(m_mainBars->barWidth());
        m_borderWidthSpin->setValue(m_mainBars->borderWidth());
    }

    void updateSummary()
    {
        if (!m_mainBars) {
            return;
        }

        const QString summary = QString(
                                    "Interactive bars: fill=%1, border=%2, borderWidth=%3, barWidth=%4, horizontal=%5, "
                                    "fillColor=%6, borderColor=%7")
                                    .arg(m_mainBars->isFillVisible() ? "on" : "off")
                                    .arg(m_mainBars->isBorderVisible() ? "on" : "off")
                                    .arg(m_mainBars->borderWidth(), 0, 'f', 2)
                                    .arg(m_mainBars->barWidth(), 0, 'f', 2)
                                    .arg(m_mainBars->isHorizontal() ? "true" : "false")
                                    .arg(m_mainBars->fillColor().name(QColor::HexArgb))
                                    .arg(m_mainBars->borderColor().name(QColor::HexArgb));
        m_summaryLabel->setText(summary);
    }

    void requestRender()
    {
        if (m_figure) {
            m_figure->requestRender();
        }
    }

private:
    QIM::QImFigureWidget* m_figure { nullptr };
    QIM::QImPlotBarsItemNode* m_mainBars { nullptr };
    QLabel* m_summaryLabel { nullptr };
    QCheckBox* m_fillVisibleCheck { nullptr };
    QCheckBox* m_borderVisibleCheck { nullptr };
    QCheckBox* m_horizontalCheck { nullptr };
    QDoubleSpinBox* m_barWidthSpin { nullptr };
    QDoubleSpinBox* m_borderWidthSpin { nullptr };
    std::vector< double > m_positions;
    std::vector< double > m_values;
    double m_randomSeed { 0.0 };
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

    BarsAppearanceTestWindow window;
    window.show();
    return app.exec();
}
