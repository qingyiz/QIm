#include <array>
#include <cmath>
#include <vector>

#include <QApplication>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMap>
#include <QOpenGLContext>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QSurfaceFormat>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include "QImFigureWidget.h"
#include "plot/QImPlot3DLineItemNode.h"
#include "plot/QImPlot3DNode.h"
#include "plot/QImPlot3DScatterItemNode.h"
#include "plot/QImPlot3DSurfaceItemNode.h"
#include "plot/QImPlotAxisInfo.h"
#include "plot/QImPlotBarsItemNode.h"
#include "plot/QImPlotNode.h"
#include "implot3d.h"

namespace
{
std::vector< double > buildXValues(int count, double step = 0.05)
{
    std::vector< double > x(count);
    for (int i = 0; i < count; ++i) {
        x[ i ] = static_cast< double >(i) * step;
    }
    return x;
}

std::vector< double > buildSineValues(const std::vector< double >& x, double phase, double amplitude = 1.0)
{
    std::vector< double > y(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        y[ i ] = amplitude * std::sin(x[ i ] + phase);
    }
    return y;
}

QColor paletteColor(int index)
{
    static const std::array< QColor, 8 > palette {
        QColor(220, 38, 38),
        QColor(37, 99, 235),
        QColor(22, 163, 74),
        QColor(245, 158, 11),
        QColor(168, 85, 247),
        QColor(14, 116, 144),
        QColor(190, 24, 93),
        QColor(89, 65, 169),
    };
    return palette[ static_cast< std::size_t >(index % static_cast< int >(palette.size())) ];
}

QString formatIndices(const std::vector< int >& indices)
{
    if (indices.empty()) {
        return "{}";
    }

    QStringList parts;
    parts.reserve(static_cast< int >(indices.size()));
    for (int value : indices) {
        parts.append(QString::number(value));
    }
    if (indices.size() == 1) {
        return parts.front();
    }
    return QString("{%1}").arg(parts.join(","));
}

void ensureLinePlot(QIM::QImPlotNode* plot, const QString& title, const QColor& color, double phase)
{
    if (!plot) {
        return;
    }

    plot->setTitle(title);
    plot->x1Axis()->setLabel("x");
    plot->y1Axis()->setLabel("y");
    plot->setLegendEnabled(true);
    if (!plot->plotItemNodes().isEmpty()) {
        return;
    }

    const std::vector< double > x = buildXValues(240);
    const std::vector< double > y = buildSineValues(x, phase);
    auto* line = plot->addLine(x, y, title);
    line->setColor(color);
    line->setLineWidth(2.0f);
}

void ensureBarsPlot(QIM::QImPlotNode* plot, const QString& title, const QColor& fillColor)
{
    if (!plot) {
        return;
    }

    plot->setTitle(title);
    plot->x1Axis()->setLabel("category");
    plot->y1Axis()->setLabel("value");
    plot->setLegendEnabled(true);
    if (!plot->plotItemNodes().isEmpty()) {
        return;
    }

    auto* bars = new QIM::QImPlotBarsItemNode(plot);
    bars->setLabel(title);
    bars->setData(std::vector< double > { 1.0, 2.0, 3.0, 4.0 }, std::vector< double > { 3.2, 4.5, 2.8, 5.1 });
    bars->setFillColor(fillColor);
    bars->setBorderColor(fillColor.darker(160));
    bars->setBorderWidth(1.5f);
}

void configure3DPlot(QIM::QImPlot3DNode* plot, const QString& title, bool equal = true)
{
    if (!plot) {
        return;
    }

    plot->setTitle(title);
    plot->setXAxisLabel("X");
    plot->setYAxisLabel("Y");
    plot->setZAxisLabel("Z");
    plot->setLegendEnabled(true);
    plot->setEqual(equal);
}

void ensure3DLinePlot(QIM::QImPlot3DNode* plot, const QString& title, const QColor& color, double phase)
{
    if (!plot) {
        return;
    }

    configure3DPlot(plot, title);
    plot->setAxisLimits(QIM::QImPlot3DNode::AxisX, -1.25, 1.25);
    plot->setAxisLimits(QIM::QImPlot3DNode::AxisY, -1.25, 1.25);
    plot->setAxisLimits(QIM::QImPlot3DNode::AxisZ, -0.2, 6.8);
    if (!plot->plotItemNodes().isEmpty()) {
        return;
    }

    std::vector< double > xs;
    std::vector< double > ys;
    std::vector< double > zs;
    xs.reserve(240);
    ys.reserve(240);
    zs.reserve(240);

    for (int i = 0; i < 240; ++i) {
        const double t = static_cast< double >(i) * 0.08 + phase;
        xs.push_back(std::cos(t));
        ys.push_back(std::sin(t));
        zs.push_back((static_cast< double >(i) * 0.08) * 0.3);
    }

    auto* line = new QIM::QImPlot3DLineItemNode(plot);
    line->setLabel(title);
    line->setData(xs, ys, zs);
    line->setColor(color);
    line->setLineWidth(2.0f);
}

void ensure3DScatterPlot(QIM::QImPlot3DNode* plot, const QString& title, const QColor& color, double phase)
{
    if (!plot) {
        return;
    }

    configure3DPlot(plot, title);
    plot->setAxisLimits(QIM::QImPlot3DNode::AxisX, -1.3, 1.3);
    plot->setAxisLimits(QIM::QImPlot3DNode::AxisY, -1.3, 1.3);
    plot->setAxisLimits(QIM::QImPlot3DNode::AxisZ, -1.1, 1.4);
    if (!plot->plotItemNodes().isEmpty()) {
        return;
    }

    std::vector< double > xs;
    std::vector< double > ys;
    std::vector< double > zs;
    xs.reserve(72);
    ys.reserve(72);
    zs.reserve(72);

    for (int i = 0; i < 72; ++i) {
        const double t = static_cast< double >(i) * 0.21 + phase;
        xs.push_back(std::cos(t) * (0.4 + 0.5 * std::sin(t * 0.31) * std::sin(t * 0.31)));
        ys.push_back(std::sin(t) * (0.4 + 0.5 * std::cos(t * 0.27) * std::cos(t * 0.27)));
        zs.push_back(std::sin(t * 0.5));
    }

    auto* scatter = new QIM::QImPlot3DScatterItemNode(plot);
    scatter->setLabel(title);
    scatter->setData(xs, ys, zs);
    scatter->setMarkerSize(5.0f);
    scatter->setMarkerWeight(1.5f);
    scatter->setFillColor(color);
    scatter->setOutlineColor(color.darker(170));
}

void ensure3DSurfacePlot(QIM::QImPlot3DNode* plot, const QString& title, const QColor& color, double phase)
{
    if (!plot) {
        return;
    }

    configure3DPlot(plot, title, false);
    plot->setAxisLimits(QIM::QImPlot3DNode::AxisX, -3.2, 3.2);
    plot->setAxisLimits(QIM::QImPlot3DNode::AxisY, -3.2, 3.2);
    plot->setAxisLimits(QIM::QImPlot3DNode::AxisZ, -1.4, 1.4);
    if (!plot->plotItemNodes().isEmpty()) {
        return;
    }

    constexpr int xCount = 36;
    constexpr int yCount = 36;
    std::vector< double > xs;
    std::vector< double > ys;
    std::vector< double > zs;
    xs.reserve(xCount * yCount);
    ys.reserve(xCount * yCount);
    zs.reserve(xCount * yCount);

    for (int yi = 0; yi < yCount; ++yi) {
        const double y = -3.0 + 6.0 * static_cast< double >(yi) / static_cast< double >(yCount - 1);
        for (int xi = 0; xi < xCount; ++xi) {
            const double x = -3.0 + 6.0 * static_cast< double >(xi) / static_cast< double >(xCount - 1);
            const double r = std::sqrt(x * x + y * y);
            xs.push_back(x);
            ys.push_back(y);
            zs.push_back(std::sin(r * 2.0 + phase) / (1.0 + r * 0.6));
        }
    }

    auto* surface = new QIM::QImPlot3DSurfaceItemNode(plot);
    surface->setLabel(title);
    surface->setData(xs, ys, zs, xCount, yCount);
    surface->setColormapEnabled(true);
    surface->setColormap(ImPlot3DColormap_Viridis);
    surface->setLineColor(color.darker(170));
    surface->setLineWidth(1.0f);
}

class SubplotLayoutTestWindow : public QMainWindow
{
public:
    SubplotLayoutTestWindow()
    {
        setWindowTitle("QIm Figure Subplot Layout Test");
        resize(1520, 920);

        QWidget* central = new QWidget(this);
        QHBoxLayout* root = new QHBoxLayout(central);

        QWidget* controlPanel = new QWidget(central);
        controlPanel->setMinimumWidth(370);
        controlPanel->setMaximumWidth(430);
        QVBoxLayout* controlsLayout = new QVBoxLayout(controlPanel);

        m_tipLabel = new QLabel(controlPanel);
        m_tipLabel->setWordWrap(true);
        controlsLayout->addWidget(m_tipLabel);

        m_dimensionCombo = new QComboBox(controlPanel);
        m_dimensionCombo->addItem("2D Subplot");
        m_dimensionCombo->addItem("3D Axes");

        m_rowsSpin = new QSpinBox(controlPanel);
        m_rowsSpin->setRange(1, 12);
        m_rowsSpin->setValue(2);

        m_colsSpin = new QSpinBox(controlPanel);
        m_colsSpin->setRange(1, 12);
        m_colsSpin->setValue(2);

        m_indicesEdit = new QLineEdit(controlPanel);
        m_indicesEdit->setPlaceholderText("2 or {1,2,5,6}");
        m_indicesEdit->setText("2");

        m_plotTypeCombo = new QComboBox(controlPanel);

        m_titleEdit = new QLineEdit(controlPanel);
        m_titleEdit->setPlaceholderText("Optional custom title");

        QFormLayout* form = new QFormLayout();
        form->addRow("Figure Type", m_dimensionCombo);
        form->addRow("Rows", m_rowsSpin);
        form->addRow("Columns", m_colsSpin);
        form->addRow("Indices", m_indicesEdit);
        form->addRow("Plot Type", m_plotTypeCombo);
        form->addRow("Title", m_titleEdit);
        controlsLayout->addLayout(form);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* applyButton = new QPushButton("Apply Command", controlPanel);
        QPushButton* resetButton = new QPushButton("Reset Current Figure", controlPanel);
        buttonLayout->addWidget(applyButton);
        buttonLayout->addWidget(resetButton);
        controlsLayout->addLayout(buttonLayout);

        m_summaryLabel = new QLabel(controlPanel);
        m_summaryLabel->setWordWrap(true);
        controlsLayout->addWidget(m_summaryLabel);

        QLabel* historyTitle = new QLabel("Command History", controlPanel);
        controlsLayout->addWidget(historyTitle);

        m_historyEdit = new QPlainTextEdit(controlPanel);
        m_historyEdit->setReadOnly(true);
        m_historyEdit->setPlaceholderText("Applied subplot commands will appear here.");
        controlsLayout->addWidget(m_historyEdit, 1);

        controlsLayout->addStretch();
        root->addWidget(controlPanel);

        QWidget* figurePanel = new QWidget(central);
        QVBoxLayout* figureLayout = new QVBoxLayout(figurePanel);
        figureLayout->setContentsMargins(0, 0, 0, 0);

        m_figureStack = new QStackedWidget(figurePanel);

        m_figure2D = new QIM::QImFigureWidget(m_figureStack);
        m_figure2D->setRenderMode(QIM::QImWidget::RenderOnDemand);
        m_figureStack->addWidget(m_figure2D);

        m_threeDPage = new QWidget(m_figureStack);
        m_threeDPageLayout = new QVBoxLayout(m_threeDPage);
        m_threeDPageLayout->setContentsMargins(0, 0, 0, 0);
        m_figureStack->addWidget(m_threeDPage);
        recreate3DFigure();

        figureLayout->addWidget(m_figureStack, 1);
        root->addWidget(figurePanel, 1);
        setCentralWidget(central);

        connect(m_dimensionCombo, qOverload< int >(&QComboBox::currentIndexChanged), this, [this](int) {
            updateModeUi();
            refreshSummary("Switched figure type");
        });
        connect(applyButton, &QPushButton::clicked, this, [this]() { applyCommand(); });
        connect(m_indicesEdit, &QLineEdit::returnPressed, this, [this]() { applyCommand(); });
        connect(resetButton, &QPushButton::clicked, this, [this]() {
            resetCurrentFigure();
            appendHistory("Reset current figure");
            refreshSummary("Current figure reset");
        });

        updateModeUi();
        appendHistory("Ready: 2D supports subplot(rows, cols, index/indices); 3D supports subplot(rows, cols, index).");
        refreshSummary("Ready");
    }

private:
    enum FigureMode
    {
        Figure2D,
        Figure3D
    };

    struct ThreeDPlotSpec
    {
        int slotIndex { 1 };
        QString plotType;
        QString title;
        int sequence { 0 };
    };

    FigureMode currentMode() const
    {
        return m_dimensionCombo->currentIndex() == 1 ? Figure3D : Figure2D;
    }

    std::vector< int > parseIndices(const QString& text, QString* errorMessage) const
    {
        QString normalized = text.trimmed();
        normalized.remove('{');
        normalized.remove('}');
        normalized.remove('[');
        normalized.remove(']');

        const QStringList parts = normalized.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
        if (parts.isEmpty()) {
            if (errorMessage) {
                *errorMessage = "Indices are empty";
            }
            return {};
        }

        std::vector< int > indices;
        indices.reserve(static_cast< std::size_t >(parts.size()));
        for (const QString& part : parts) {
            bool ok = false;
            const int value = part.toInt(&ok);
            if (!ok) {
                if (errorMessage) {
                    *errorMessage = QString("Invalid index token: %1").arg(part);
                }
                return {};
            }
            indices.push_back(value);
        }

        if (errorMessage) {
            errorMessage->clear();
        }
        return indices;
    }

    QString buildCommandText(int rows, int cols, const std::vector< int >& indices) const
    {
        return QString("subplot(%1,%2,%3)").arg(rows).arg(cols).arg(formatIndices(indices));
    }

    QString buildPlotTitle(int rows, int cols, const std::vector< int >& indices) const
    {
        const QString customTitle = m_titleEdit->text().trimmed();
        if (!customTitle.isEmpty()) {
            return customTitle;
        }
        return buildCommandText(rows, cols, indices);
    }

    void populate2DPlot(QIM::QImPlotNode* plot, const QString& title, int plotIndex)
    {
        if (!plot) {
            return;
        }

        const QColor color = paletteColor(plotIndex);
        const QString plotType = m_plotTypeCombo->currentText();
        if (plotType == "Bars") {
            ensureBarsPlot(plot, title, color);
            return;
        }
        if (plotType == "Alternate" && (plotIndex % 2 == 1)) {
            ensureBarsPlot(plot, title, color);
            return;
        }
        ensureLinePlot(plot, title, color, 0.45 * static_cast< double >(plotIndex));
    }

    void populate3DPlot(QIM::QImPlot3DNode* plot, const ThreeDPlotSpec& spec)
    {
        if (!plot) {
            return;
        }

        const QColor color = paletteColor(spec.sequence);
        const double phase = 0.4 * static_cast< double >(spec.sequence);
        if (spec.plotType == "3D Scatter") {
            ensure3DScatterPlot(plot, spec.title, color, phase);
            return;
        }
        if (spec.plotType == "3D Surface") {
            ensure3DSurfacePlot(plot, spec.title, color, phase);
            return;
        }
        ensure3DLinePlot(plot, spec.title, color, phase);
    }

    void configure3DPlaceholder(QIM::QImPlot3DNode* plot, int slotIndex)
    {
        if (!plot) {
            return;
        }

        configure3DPlot(plot, QString("Empty Slot %1").arg(slotIndex));
        plot->setLegendEnabled(false);
    }

    void recreate3DFigure()
    {
        if (m_figure3D) {
            m_threeDPageLayout->removeWidget(m_figure3D);
            delete m_figure3D;
            m_figure3D = nullptr;
        }

        m_figure3D = new QIM::QImFigureWidget(m_threeDPage);
        m_figure3D->setRenderMode(QIM::QImWidget::RenderOnDemand);
        m_figure3D->setSubplotGrid(m_threeDRows, m_threeDCols);
        m_threeDPageLayout->addWidget(m_figure3D, 1);

        const int maxSlot = m_threeDPlotSpecs.isEmpty() ? 0 : m_threeDPlotSpecs.lastKey();
        for (int slotIndex = 1; slotIndex <= maxSlot; ++slotIndex) {
            QIM::QImPlot3DNode* plot = m_figure3D->createPlot3DNode();
            if (!plot) {
                break;
            }

            const auto it = m_threeDPlotSpecs.constFind(slotIndex);
            if (it != m_threeDPlotSpecs.constEnd()) {
                populate3DPlot(plot, it.value());
            } else {
                configure3DPlaceholder(plot, slotIndex);
            }
        }

        m_figure3D->requestRender();
    }

    void updateModeUi()
    {
        const bool is3D = currentMode() == Figure3D;
        m_figureStack->setCurrentIndex(is3D ? 1 : 0);

        const QString currentType = m_plotTypeCombo->currentText();
        {
            QSignalBlocker blocker(m_plotTypeCombo);
            m_plotTypeCombo->clear();
            if (is3D) {
                m_plotTypeCombo->addItem("3D Line");
                m_plotTypeCombo->addItem("3D Scatter");
                m_plotTypeCombo->addItem("3D Surface");
                const int keepIndex = m_plotTypeCombo->findText(currentType);
                m_plotTypeCombo->setCurrentIndex(keepIndex >= 0 ? keepIndex : 0);
            } else {
                m_plotTypeCombo->addItem("Line");
                m_plotTypeCombo->addItem("Bars");
                m_plotTypeCombo->addItem("Alternate");
                const int keepIndex = m_plotTypeCombo->findText(currentType);
                m_plotTypeCombo->setCurrentIndex(keepIndex >= 0 ? keepIndex : 0);
            }
        }

        if (is3D) {
            m_tipLabel->setText(
                "3D mode creates QImFigureWidget 3D axes.\n"
                "Use rows, columns and a single slot index to place a 3D coordinate system.\n"
                "Current 3D example intentionally follows the existing API: only single-slot creation is supported here.\n"
                "When you switch to a different rows x cols grid, previous 3D slots are cleared and rebuilt.");
            m_indicesEdit->setPlaceholderText("Single slot index, e.g. 3");
        } else {
            m_tipLabel->setText(
                "2D mode behaves like a subplot command workbench.\n"
                "Enter rows, columns and indices, then press Apply to create or reuse a subplot.\n"
                "Accepted index formats: 2, 1,2, 1 2 5 6, {1,2,5,6}.\n"
                "Merged selections must still form a rectangle, such as {1,2} or {1,2,5,6}.");
            m_indicesEdit->setPlaceholderText("2 or {1,2,5,6}");
        }
    }

    void applyCommand()
    {
        QString errorMessage;
        const std::vector< int > indices = parseIndices(m_indicesEdit->text(), &errorMessage);
        if (!errorMessage.isEmpty()) {
            appendHistory(QString("Rejected command: %1").arg(errorMessage));
            refreshSummary(errorMessage);
            return;
        }

        const int rows = m_rowsSpin->value();
        const int cols = m_colsSpin->value();
        if (currentMode() == Figure3D) {
            apply3DCommand(rows, cols, indices);
        } else {
            apply2DCommand(rows, cols, indices);
        }
    }

    void apply2DCommand(int rows, int cols, const std::vector< int >& indices)
    {
        const QString commandText = buildCommandText(rows, cols, indices);

        QIM::QImPlotNode* plot = nullptr;
        if (indices.size() == 1) {
            plot = m_figure2D->subplot(rows, cols, indices.front());
        } else {
            plot = m_figure2D->subplot(rows, cols, indices);
        }

        if (!plot) {
            const QString message = QString("%1 rejected. Check whether indices are in range and form a rectangle.")
                                        .arg(commandText);
            appendHistory(message);
            refreshSummary(message);
            return;
        }

        const bool plotWasEmpty = plot->plotItemNodes().isEmpty();
        const QString title = buildPlotTitle(rows, cols, indices);
        populate2DPlot(plot, title, m_commandCount);
        ++m_commandCount;

        const QString action = plotWasEmpty
                                   ? QString("Applied 2D %1").arg(commandText)
                                   : QString("Reused existing 2D axes for %1").arg(commandText);
        appendHistory(action);
        refreshSummary(action);
    }

    void apply3DCommand(int rows, int cols, const std::vector< int >& indices)
    {
        if (indices.size() != 1) {
            const QString message = "3D mode currently accepts only one slot index per command.";
            appendHistory(QString("Rejected 3D command: %1").arg(message));
            refreshSummary(message);
            return;
        }

        const int slotIndex = indices.front();
        if (slotIndex < 1 || slotIndex > rows * cols) {
            const QString message = QString("3D slot index %1 is out of range for %2 x %3.")
                                        .arg(slotIndex)
                                        .arg(rows)
                                        .arg(cols);
            appendHistory(QString("Rejected 3D command: %1").arg(message));
            refreshSummary(message);
            return;
        }

        if (m_threeDRows != rows || m_threeDCols != cols) {
            m_threeDRows = rows;
            m_threeDCols = cols;
            m_threeDPlotSpecs.clear();
        }

        ThreeDPlotSpec spec;
        spec.slotIndex = slotIndex;
        spec.plotType = m_plotTypeCombo->currentText();
        spec.title = buildPlotTitle(rows, cols, indices);
        spec.sequence = m_commandCount;
        m_threeDPlotSpecs.insert(slotIndex, spec);
        ++m_commandCount;

        recreate3DFigure();

        const QString action = QString("Applied 3D subplot(%1,%2,%3) as %4")
                                   .arg(rows)
                                   .arg(cols)
                                   .arg(slotIndex)
                                   .arg(spec.plotType);
        appendHistory(action);
        refreshSummary(action);
    }

    void resetCurrentFigure()
    {
        if (currentMode() == Figure3D) {
            m_threeDPlotSpecs.clear();
            m_threeDRows = 1;
            m_threeDCols = 1;
            recreate3DFigure();
            return;
        }

        const QList< QIM::QImPlotNode* > plots = m_figure2D->plotNodes();
        for (QIM::QImPlotNode* plot : plots) {
            m_figure2D->removePlotNode(plot);
        }
        m_figure2D->setSubplotGrid(1, 1);
        m_figure2D->requestRender();
    }

    void appendHistory(const QString& text)
    {
        m_historyEdit->appendPlainText(text);
    }

    void refreshSummary(const QString& action)
    {
        if (currentMode() == Figure3D) {
            const int renderedAxes = m_figure3D ? m_figure3D->plot3DCount() : 0;
            m_summaryLabel->setText(
                QString("%1\nCurrent figure: 3D axes\nAssigned slots: %2\nRendered axes: %3\nCurrent grid: %4 x %5")
                    .arg(action)
                    .arg(m_threeDPlotSpecs.size())
                    .arg(renderedAxes)
                    .arg(m_threeDRows)
                    .arg(m_threeDCols));
            if (m_figure3D) {
                m_figure3D->requestRender();
            }
            return;
        }

        m_summaryLabel->setText(
            QString("%1\nCurrent figure: 2D subplot\nCurrent plots: %2\nCurrent grid: %3 x %4")
                .arg(action)
                .arg(m_figure2D->plotCount())
                .arg(m_figure2D->subplotGridRows())
                .arg(m_figure2D->subplotGridColumns()));
        m_figure2D->requestRender();
    }

private:
    QLabel* m_tipLabel { nullptr };
    QLabel* m_summaryLabel { nullptr };
    QComboBox* m_dimensionCombo { nullptr };
    QSpinBox* m_rowsSpin { nullptr };
    QSpinBox* m_colsSpin { nullptr };
    QLineEdit* m_indicesEdit { nullptr };
    QComboBox* m_plotTypeCombo { nullptr };
    QLineEdit* m_titleEdit { nullptr };
    QPlainTextEdit* m_historyEdit { nullptr };
    QStackedWidget* m_figureStack { nullptr };
    QIM::QImFigureWidget* m_figure2D { nullptr };
    QWidget* m_threeDPage { nullptr };
    QVBoxLayout* m_threeDPageLayout { nullptr };
    QIM::QImFigureWidget* m_figure3D { nullptr };
    QMap< int, ThreeDPlotSpec > m_threeDPlotSpecs;
    int m_threeDRows { 1 };
    int m_threeDCols { 1 };
    int m_commandCount { 0 };
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

    SubplotLayoutTestWindow window;
    window.show();
    return app.exec();
}
