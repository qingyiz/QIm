#include <cmath>
#include <QApplication>
#include <QCheckBox>
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
#include "plot/QImPlotMarkerAnnotationItemNode.h"
#include "plot/QImPlotNode.h"

namespace
{

QColor randomColor()
{
    return QColor::fromRgb(QRandomGenerator::global()->bounded(256),
                           QRandomGenerator::global()->bounded(256),
                           QRandomGenerator::global()->bounded(256));
}

QVector< QPointF > buildPeakAnnotations()
{
    QVector< QPointF > points;
    points << QPointF(1.2, std::sin(1.2) + 0.25 * std::cos(3.0 * 1.2)) << QPointF(3.1, std::sin(3.1) + 0.25 * std::cos(9.3))
           << QPointF(5.5, std::sin(5.5) + 0.25 * std::cos(16.5)) << QPointF(7.9, std::sin(7.9) + 0.25 * std::cos(23.7));
    return points;
}

QVector< QPointF > buildRandomAnnotations()
{
    QVector< QPointF > points;
    for (int i = 0; i < 6; ++i) {
        const double x = 0.5 + QRandomGenerator::global()->generateDouble() * 9.0;
        const double y = std::sin(x) + 0.25 * std::cos(3.0 * x);
        points << QPointF(x, y);
    }
    return points;
}

QStringList buildAnnotationTexts(int count)
{
    QStringList texts;
    for (int i = 0; i < count; ++i) {
        texts << QString("P%1").arg(i + 1);
    }
    return texts;
}

class MarkerAnnotationWindow : public QMainWindow
{
public:
    MarkerAnnotationWindow()
    {
        setWindowTitle("QIm Figure Marker Annotation Test");
        resize(1240, 820);

        QWidget* central = new QWidget(this);
        QVBoxLayout* root = new QVBoxLayout(central);

        QLabel* tip = new QLabel(
            "This example tests multi-point marker annotations. Style controls and color controls are independent.",
            central);
        tip->setWordWrap(true);
        root->addWidget(tip);

        QHBoxLayout* styleRow = new QHBoxLayout();
        styleRow->addWidget(new QLabel("Marker Shape", central));
        m_shapeCombo = new QComboBox(central);
        m_shapeCombo->addItem("Cross", ImPlotMarker_Cross);
        m_shapeCombo->addItem("Circle", ImPlotMarker_Circle);
        m_shapeCombo->addItem("Square", ImPlotMarker_Square);
        m_shapeCombo->addItem("Diamond", ImPlotMarker_Diamond);
        m_shapeCombo->addItem("Up Triangle", ImPlotMarker_Up);
        m_shapeCombo->addItem("Asterisk", ImPlotMarker_Asterisk);
        styleRow->addWidget(m_shapeCombo);

        styleRow->addWidget(new QLabel("Size", central));
        m_sizeSpin = new QDoubleSpinBox(central);
        m_sizeSpin->setRange(2.0, 20.0);
        m_sizeSpin->setSingleStep(0.5);
        styleRow->addWidget(m_sizeSpin);

        styleRow->addWidget(new QLabel("Weight", central));
        m_weightSpin = new QDoubleSpinBox(central);
        m_weightSpin->setRange(0.5, 8.0);
        m_weightSpin->setSingleStep(0.5);
        styleRow->addWidget(m_weightSpin);

        m_filledCheck = new QCheckBox("Filled", central);
        styleRow->addWidget(m_filledCheck);

        QPushButton* randomPoints = new QPushButton("Randomize Points", central);
        QPushButton* resetStyle   = new QPushButton("Reset Style", central);
        styleRow->addWidget(randomPoints);
        styleRow->addWidget(resetStyle);
        styleRow->addStretch();
        root->addLayout(styleRow);

        QHBoxLayout* textRow = new QHBoxLayout();
        m_showTextCheck = new QCheckBox("Show Text", central);
        textRow->addWidget(m_showTextCheck);

        textRow->addWidget(new QLabel("Text Offset X", central));
        m_textOffsetXSpin = new QDoubleSpinBox(central);
        m_textOffsetXSpin->setRange(-40.0, 40.0);
        m_textOffsetXSpin->setSingleStep(1.0);
        textRow->addWidget(m_textOffsetXSpin);

        textRow->addWidget(new QLabel("Text Offset Y", central));
        m_textOffsetYSpin = new QDoubleSpinBox(central);
        m_textOffsetYSpin->setRange(-40.0, 40.0);
        m_textOffsetYSpin->setSingleStep(1.0);
        textRow->addWidget(m_textOffsetYSpin);
        textRow->addStretch();
        root->addLayout(textRow);

        root->addLayout(createColorRow("Primary Color", [this](const QColor& color) {
            if (!m_annotations) {
                return;
            }
            m_annotations->setColor(color);
            requestRender();
            updateSummary();
        }));
        root->addLayout(createColorRow("Fill Color", [this](const QColor& color) {
            if (!m_annotations) {
                return;
            }
            m_annotations->setMarkerFillColor(color);
            requestRender();
            updateSummary();
        }));
        root->addLayout(createColorRow("Outline Color", [this](const QColor& color) {
            if (!m_annotations) {
                return;
            }
            m_annotations->setMarkerOutlineColor(color);
            requestRender();
            updateSummary();
        }));
        root->addLayout(createColorRow("Text Color", [this](const QColor& color) {
            if (!m_annotations) {
                return;
            }
            m_annotations->setTextColor(color);
            requestRender();
            updateSummary();
        }));

        m_summaryLabel = new QLabel(central);
        root->addWidget(m_summaryLabel);

        m_figure = new QIM::QImFigureWidget(central);
        m_figure->setRenderMode(QIM::QImWidget::RenderOnDemand);
        root->addWidget(m_figure, 1);
        setCentralWidget(central);

        setupPlot();
        syncControls();
        updateSummary();

        connect(m_shapeCombo, qOverload< int >(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!m_annotations) {
                return;
            }
            m_annotations->setMarkerShape(m_shapeCombo->itemData(index).toInt());
            requestRender();
            updateSummary();
        });
        connect(m_sizeSpin, qOverload< double >(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            if (!m_annotations) {
                return;
            }
            m_annotations->setMarkerSize(static_cast< float >(value));
            requestRender();
            updateSummary();
        });
        connect(m_weightSpin, qOverload< double >(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            if (!m_annotations) {
                return;
            }
            m_annotations->setMarkerWeight(static_cast< float >(value));
            requestRender();
            updateSummary();
        });
        connect(m_filledCheck, &QCheckBox::toggled, this, [this](bool checked) {
            if (!m_annotations) {
                return;
            }
            m_annotations->setMarkerFilled(checked);
            requestRender();
            updateSummary();
        });
        connect(m_showTextCheck, &QCheckBox::toggled, this, [this](bool checked) {
            if (!m_annotations) {
                return;
            }
            m_annotations->setTextVisible(checked);
            requestRender();
            updateSummary();
        });
        connect(m_textOffsetXSpin, qOverload< double >(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            if (!m_annotations) {
                return;
            }
            const QPointF oldOffset = m_annotations->textPixelOffset();
            m_annotations->setTextPixelOffset(QPointF(value, oldOffset.y()));
            requestRender();
            updateSummary();
        });
        connect(m_textOffsetYSpin, qOverload< double >(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            if (!m_annotations) {
                return;
            }
            const QPointF oldOffset = m_annotations->textPixelOffset();
            m_annotations->setTextPixelOffset(QPointF(oldOffset.x(), value));
            requestRender();
            updateSummary();
        });
        connect(randomPoints, &QPushButton::clicked, this, [this]() {
            if (!m_annotations) {
                return;
            }
            const QVector< QPointF > points = buildRandomAnnotations();
            m_annotations->setPoints(points);
            m_annotations->setTexts(buildAnnotationTexts(points.size()));
            requestRender();
            updateSummary();
        });
        connect(resetStyle, &QPushButton::clicked, this, [this]() {
            applyDefaultAppearance();
            syncControls();
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

        auto addColorButton = [&](const QString& text, const QColor& color) {
            QPushButton* button = new QPushButton(text, this);
            connect(button, &QPushButton::clicked, this, [apply, color]() { apply(color); });
            row->addWidget(button);
        };

        addColorButton("Red", QColor(220, 45, 45));
        addColorButton("Blue", QColor(50, 110, 220));
        addColorButton("Green", QColor(40, 170, 80));
        addColorButton("Gold", QColor(220, 160, 30));

        QPushButton* randomButton = new QPushButton("Random", this);
        connect(randomButton, &QPushButton::clicked, this, [apply]() { apply(randomColor()); });
        row->addWidget(randomButton);
        row->addStretch();
        return row;
    }

    void setupPlot()
    {
        QIM::QImPlotNode* plot = m_figure->createPlotNode();
        if (!plot) {
            return;
        }

        plot->setTitle("Marker Annotation Demo");
        plot->setLegendEnabled(true);
        plot->x1Axis()->setLabel("x");
        plot->y1Axis()->setLabel("y");

        std::vector< double > xs(600);
        std::vector< double > ys(600);
        for (int i = 0; i < static_cast< int >(xs.size()); ++i) {
            xs[ i ] = i * 0.016;
            ys[ i ] = std::sin(xs[ i ]) + 0.25 * std::cos(3.0 * xs[ i ]);
        }
        auto* line = plot->addLine(xs, ys, "reference curve");
        line->setColor(QColor(35, 35, 35));
        line->setLineWidth(2.0f);

        m_annotations = plot->addMarkerAnnotations(buildPeakAnnotations(), "marker annotations");
        applyDefaultAppearance();
    }

    void applyDefaultAppearance()
    {
        if (!m_annotations) {
            return;
        }
        m_annotations->setMarkerShape(ImPlotMarker_Cross);
        m_annotations->setColor(QColor(220, 45, 45));
        m_annotations->setMarkerFilled(false);
        m_annotations->setMarkerSize(10.0f);
        m_annotations->setMarkerWeight(2.5f);
        m_annotations->setMarkerFillColor(QColor(220, 45, 45));
        m_annotations->setMarkerOutlineColor(QColor(220, 45, 45));
        m_annotations->setTexts(buildAnnotationTexts(m_annotations->data() ? m_annotations->data()->size() : 0));
        m_annotations->setTextVisible(true);
        m_annotations->setTextPixelOffset(QPointF(8.0, -12.0));
        m_annotations->setTextColor(QColor(80, 80, 80));
    }

    void syncControls()
    {
        if (!m_annotations) {
            return;
        }

        QSignalBlocker blockShape(m_shapeCombo);
        QSignalBlocker blockSize(m_sizeSpin);
        QSignalBlocker blockWeight(m_weightSpin);
        QSignalBlocker blockFilled(m_filledCheck);
        QSignalBlocker blockShowText(m_showTextCheck);
        QSignalBlocker blockTextOffsetX(m_textOffsetXSpin);
        QSignalBlocker blockTextOffsetY(m_textOffsetYSpin);

        for (int i = 0; i < m_shapeCombo->count(); ++i) {
            if (m_shapeCombo->itemData(i).toInt() == m_annotations->markerShape()) {
                m_shapeCombo->setCurrentIndex(i);
                break;
            }
        }
        m_sizeSpin->setValue(m_annotations->markerSize());
        m_weightSpin->setValue(m_annotations->markerWeight());
        m_filledCheck->setChecked(m_annotations->isMarkerFilled());
        m_showTextCheck->setChecked(m_annotations->isTextVisible());
        m_textOffsetXSpin->setValue(m_annotations->textPixelOffset().x());
        m_textOffsetYSpin->setValue(m_annotations->textPixelOffset().y());
    }

    void updateSummary()
    {
        if (!m_annotations || !m_summaryLabel) {
            return;
        }
        m_summaryLabel->setText(
            QString("Style: shape=%1 | size=%2 | weight=%3 | filled=%4. "
                    "Colors: primary=%5 | fill=%6 | outline=%7 | text=%8. "
                    "Text: visible=%9 | count=%10 | offset=(%11,%12)")
                .arg(m_shapeCombo ? m_shapeCombo->currentText() : QString())
                .arg(m_annotations->markerSize(), 0, 'f', 1)
                .arg(m_annotations->markerWeight(), 0, 'f', 1)
                .arg(m_annotations->isMarkerFilled() ? "true" : "false")
                .arg(m_annotations->color().name(QColor::HexRgb))
                .arg(m_annotations->markerFillColor().name(QColor::HexRgb))
                .arg(m_annotations->markerOutlineColor().name(QColor::HexRgb))
                .arg(m_annotations->textColor().name(QColor::HexRgb))
                .arg(m_annotations->isTextVisible() ? "true" : "false")
                .arg(m_annotations->texts().size())
                .arg(m_annotations->textPixelOffset().x(), 0, 'f', 0)
                .arg(m_annotations->textPixelOffset().y(), 0, 'f', 0));
    }

    void requestRender()
    {
        if (m_figure) {
            m_figure->requestRender();
        }
    }

    QIM::QImFigureWidget* m_figure { nullptr };
    QIM::QImPlotMarkerAnnotationItemNode* m_annotations { nullptr };
    QComboBox* m_shapeCombo { nullptr };
    QDoubleSpinBox* m_sizeSpin { nullptr };
    QDoubleSpinBox* m_weightSpin { nullptr };
    QCheckBox* m_filledCheck { nullptr };
    QCheckBox* m_showTextCheck { nullptr };
    QDoubleSpinBox* m_textOffsetXSpin { nullptr };
    QDoubleSpinBox* m_textOffsetYSpin { nullptr };
    QLabel* m_summaryLabel { nullptr };
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

    MarkerAnnotationWindow window;
    window.show();
    return app.exec();
}
