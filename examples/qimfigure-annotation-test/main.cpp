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
#include <QSurfaceFormat>
#include <QVBoxLayout>
#include <QWidget>
#include "QImFigureWidget.h"
#include "plot/QImPlotAnnotationItemNode.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotNode.h"

namespace
{

class AnnotationWindow : public QMainWindow
{
public:
    AnnotationWindow()
    {
        setWindowTitle("QIm Figure Annotation Test");
        resize(1240, 820);

        QWidget* central = new QWidget(this);
        QVBoxLayout* root = new QVBoxLayout(central);

        QLabel* tip = new QLabel(
            "Drag the blue control points in the plot. Shape center points move the whole annotation; edge points edit size or endpoints.",
            central);
        tip->setWordWrap(true);
        root->addWidget(tip);

        QHBoxLayout* controls = new QHBoxLayout();
        m_editableCheck = new QCheckBox("Editable", central);
        m_editableCheck->setChecked(true);
        controls->addWidget(m_editableCheck);

        m_filledCheck = new QCheckBox("Filled shapes", central);
        controls->addWidget(m_filledCheck);

        controls->addWidget(new QLabel("Line Style", central));
        m_lineStyleCombo = new QComboBox(central);
        m_lineStyleCombo->addItem("Solid", static_cast< int >(Qt::SolidLine));
        m_lineStyleCombo->addItem("Dash", static_cast< int >(Qt::DashLine));
        m_lineStyleCombo->addItem("Dot", static_cast< int >(Qt::DotLine));
        m_lineStyleCombo->addItem("Dash Dot", static_cast< int >(Qt::DashDotLine));
        m_lineStyleCombo->addItem("Dash Dot Dot", static_cast< int >(Qt::DashDotDotLine));
        controls->addWidget(m_lineStyleCombo);

        controls->addWidget(new QLabel("Line Width", central));
        m_lineWidthSpin = new QDoubleSpinBox(central);
        m_lineWidthSpin->setRange(0.5, 8.0);
        m_lineWidthSpin->setSingleStep(0.5);
        m_lineWidthSpin->setValue(2.0);
        controls->addWidget(m_lineWidthSpin);

        QPushButton* resetButton = new QPushButton("Reset Annotations", central);
        controls->addWidget(resetButton);
        controls->addStretch();
        root->addLayout(controls);

        m_summaryLabel = new QLabel(central);
        root->addWidget(m_summaryLabel);

        m_figure = new QIM::QImFigureWidget(central);
        m_figure->setRenderMode(QIM::QImWidget::RenderOnDemand);
        root->addWidget(m_figure, 1);
        setCentralWidget(central);

        setupPlot();
        updateSummary();

        connect(m_editableCheck, &QCheckBox::toggled, this, [this](bool checked) {
            m_annotations->setEditable(checked);
            requestRender();
        });
        connect(m_filledCheck, &QCheckBox::toggled, this, [this](bool checked) {
            m_annotations->setFilled(checked);
            requestRender();
        });
        connect(m_lineStyleCombo, qOverload< int >(&QComboBox::currentIndexChanged), this, [this](int index) {
            m_annotations->setLineStyle(m_lineStyleCombo->itemData(index).toInt());
            updateSummary();
            requestRender();
        });
        connect(m_lineWidthSpin, qOverload< double >(&QDoubleSpinBox::valueChanged), this, [this](double value) {
            m_annotations->setLineWidth(static_cast< float >(value));
            requestRender();
        });
        connect(resetButton, &QPushButton::clicked, this, [this]() {
            populateAnnotations();
            requestRender();
            updateSummary();
        });
        connect(m_annotations, &QIM::QImPlotAnnotationItemNode::annotationsChanged, this, [this]() {
            updateSummary();
            requestRender();
        });
        connect(m_annotations, &QIM::QImPlotAnnotationItemNode::selectedAnnotationChanged, this, [this]() {
            updateSummary();
            requestRender();
        });
    }

private:
    void setupPlot()
    {
        QIM::QImPlotNode* plot = m_figure->createPlotNode();
        plot->setTitle("Editable Annotation Demo");
        plot->x1Axis()->setLabel("x");
        plot->y1Axis()->setLabel("y");
        plot->setLegendEnabled(true);

        std::vector< double > xs(500);
        std::vector< double > ys(500);
        for (int i = 0; i < static_cast< int >(xs.size()); ++i) {
            xs[ i ] = -1.0 + i * 0.024;
            ys[ i ] = std::sin(xs[ i ] * 1.7) * 1.4 + std::cos(xs[ i ] * 0.7) * 0.5;
        }
        auto* line = plot->addLine(xs, ys, "reference");
        line->setColor(QColor(70, 70, 70));
        line->setLineWidth(1.8f);

        m_annotations = plot->addAnnotations("editable annotations");
        m_annotations->setColor(QColor(35, 115, 190));
        m_annotations->setFillColor(QColor(35, 115, 190, 45));
        m_annotations->setTextColor(QColor(25, 25, 25));
        m_annotations->setLineStyle(Qt::SolidLine);
        populateAnnotations();
        plot->setAxesToFit();
    }

    void populateAnnotations()
    {
        m_annotations->clearAnnotations();
        m_annotations->addCircle(QPointF(0.5, 1.3), QPointF(0.9, 1.3), "circle");
        m_annotations->addSquare(QPointF(2.0, -0.15), QPointF(2.9, 0.65), "square");
        m_annotations->addTriangle(QPointF(4.0, 0.55), QPointF(4.0, 1.15), "triangle");
        m_annotations->addArrow(QPointF(0.2, -1.5), QPointF(1.4, -0.75));
        m_annotations->addTextArrow(QPointF(3.0, -1.2), QPointF(4.2, -0.35), "text arrow");
        m_annotations->addDoubleArrow(QPointF(5.1, 1.15), QPointF(6.4, 1.15));
        m_annotations->addDoubleTextArrow(QPointF(6.7, -0.8), QPointF(8.3, -0.2), "two-way");
    }

    void updateSummary()
    {
        if (!m_summaryLabel || !m_annotations) {
            return;
        }
        m_summaryLabel->setText(QString("Annotations: %1 | selected=%2 | editable=%3 | filled=%4 | lineStyle=%5 | lineWidth=%6")
                                    .arg(m_annotations->annotationCount())
                                    .arg(m_annotations->selectedAnnotationIndex())
                                    .arg(m_annotations->isEditable() ? "true" : "false")
                                    .arg(m_annotations->isFilled() ? "true" : "false")
                                    .arg(m_lineStyleCombo ? m_lineStyleCombo->currentText() : QString())
                                    .arg(m_annotations->lineWidth(), 0, 'f', 1));
    }

    void requestRender()
    {
        if (m_figure) {
            m_figure->requestRender();
        }
    }

    QIM::QImFigureWidget* m_figure { nullptr };
    QIM::QImPlotAnnotationItemNode* m_annotations { nullptr };
    QCheckBox* m_editableCheck { nullptr };
    QCheckBox* m_filledCheck { nullptr };
    QComboBox* m_lineStyleCombo { nullptr };
    QDoubleSpinBox* m_lineWidthSpin { nullptr };
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

    AnnotationWindow window;
    window.show();
    return app.exec();
}
