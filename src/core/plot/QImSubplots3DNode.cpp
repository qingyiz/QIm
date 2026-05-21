#include "QImSubplots3DNode.h"
#include "QImPlot3DNode.h"
#include "imgui.h"
#include <algorithm>
#include <numeric>

namespace QIM
{
class QImSubplots3DNode::CellNode : public QImAbstractNode
{
public:
    explicit CellNode(int subplotIndex, QObject* parent = nullptr) : QImAbstractNode(parent), m_subplotIndex(subplotIndex)
    {
        setAutoIdEnabled(false);
    }

    bool beginDraw() override
    {
        QImSubplots3DNode* subplots = qobject_cast< QImSubplots3DNode* >(parentNode());
        if (!subplots) {
            return true;
        }
        const QPoint cellPos = subplots->cellPosition(m_subplotIndex);
        const QSizeF cellSz  = subplots->cellSize(m_subplotIndex);
        ImGui::SetCursorPos(ImVec2(static_cast< float >(cellPos.x()), static_cast< float >(cellPos.y())));

        for (QImAbstractNode* child : childrenNodes()) {
            if (QImPlot3DNode* plot = qobject_cast< QImPlot3DNode* >(child)) {
                plot->setAutoSize(false);
                plot->setSize(cellSz);
            }
        }
        return true;
    }

    void endDraw() override
    {
    }

private:
    int m_subplotIndex { 0 };
};

QImSubplots3DNode::QImSubplots3DNode(QObject* parent) : QImAbstractNode(parent)
{
    setTitle(QStringLiteral("##Subplots3D"));
}

QImSubplots3DNode::QImSubplots3DNode(const QString& title, QObject* parent) : QImAbstractNode(parent)
{
    setTitle(title);
}

QImSubplots3DNode::~QImSubplots3DNode()
{
}

QString QImSubplots3DNode::title() const
{
    return QString::fromUtf8(m_titleUtf8);
}

void QImSubplots3DNode::setTitle(const QString& title)
{
    const QByteArray utf8 = title.toUtf8();
    if (m_titleUtf8 != utf8) {
        m_titleUtf8 = utf8;
        Q_EMIT titleChanged(title);
    }
}

int QImSubplots3DNode::rows() const
{
    return m_rows;
}

void QImSubplots3DNode::setRows(int rows)
{
    if (rows > 0 && m_rows != rows) {
        m_rows = rows;
        Q_EMIT gridInfoChanged();
    }
}

int QImSubplots3DNode::columns() const
{
    return m_cols;
}

void QImSubplots3DNode::setColumns(int columns)
{
    if (columns > 0 && m_cols != columns) {
        m_cols = columns;
        Q_EMIT gridInfoChanged();
    }
}

std::vector< float > QImSubplots3DNode::rowRatios() const
{
    return m_rowRatios;
}

void QImSubplots3DNode::setRowRatios(const std::vector< float >& rowRatios)
{
    if (m_rowRatios != rowRatios) {
        m_rowRatios = rowRatios;
        Q_EMIT gridInfoChanged();
    }
}

std::vector< float > QImSubplots3DNode::columnRatios() const
{
    return m_columnRatios;
}

void QImSubplots3DNode::setColumnRatios(const std::vector< float >& columnRatios)
{
    if (m_columnRatios != columnRatios) {
        m_columnRatios = columnRatios;
        Q_EMIT gridInfoChanged();
    }
}

void QImSubplots3DNode::setGrid(int rows, int cols, const std::vector< float >& rowRatios, const std::vector< float >& columnRatios)
{
    {
        QSignalBlocker blocker(static_cast< QObject* >(this));
        setRows(rows);
        setColumns(cols);
        setRowRatios(rowRatios);
        setColumnRatios(columnRatios);
    }
    Q_EMIT gridInfoChanged();
}

QSizeF QImSubplots3DNode::size() const
{
    return m_size;
}

void QImSubplots3DNode::setSize(const QSizeF& size)
{
    if (m_size != size) {
        m_size = size;
        Q_EMIT sizeChanged(size);
    }
}

int QImSubplots3DNode::gridCount() const
{
    return m_rows * m_cols;
}

QImSubplots3DNode::CellNode* QImSubplots3DNode::createCellNode()
{
    CellNode* cell = new CellNode(plotCount(), static_cast< QObject* >(this));
    this->addChildNode(cell);
    return cell;
}

QImPlot3DNode* QImSubplots3DNode::createPlotNode()
{
    if (plotCount() >= gridCount()) {
        return nullptr;
    }
    CellNode* cell      = createCellNode();
    QImPlot3DNode* plot = new QImPlot3DNode(cell);
    return plot;
}

QList< QImPlot3DNode* > QImSubplots3DNode::plotNodes() const
{
    QList< QImPlot3DNode* > plots;
    const QList< QImAbstractNode* >& cells = this->childrenNodes();
    for (QImAbstractNode* child : cells) {
        if (!child) {
            continue;
        }
        const QList< QImAbstractNode* >& cellChildren = child->childrenNodes();
        for (QImAbstractNode* cellChild : cellChildren) {
            if (QImPlot3DNode* plot = qobject_cast< QImPlot3DNode* >(cellChild)) {
                plots.push_back(plot);
            }
        }
    }
    return plots;
}

int QImSubplots3DNode::plotCount() const
{
    return plotNodes().size();
}

bool QImSubplots3DNode::takePlotNode(QImPlot3DNode* plot)
{
    if (!plot) {
        return false;
    }

    QImAbstractNode* cell = plot->parentNode();
    if (!cell || cell->parentNode() != this) {
        return false;
    }

    const bool taken = cell->takeChildNode(plot);
    if (cell->childNodeCount() == 0) {
        takeChildNode(cell);
        cell->deleteLater();
    }
    return taken;
}

void QImSubplots3DNode::removePlotNode(QImPlot3DNode* plot)
{
    if (takePlotNode(plot)) {
        plot->deleteLater();
    }
}

bool QImSubplots3DNode::beginDraw()
{
    const ImVec2 cursorPos = ImGui::GetCursorPos();
    const ImVec2 availSize = ImGui::GetContentRegionAvail();
    m_origin = QPointF(cursorPos.x, cursorPos.y);
    m_availableSize = QSizeF(
        m_size.width() > 0 ? m_size.width() : static_cast< double >(availSize.x),
        m_size.height() > 0 ? m_size.height() : static_cast< double >(availSize.y)
    );
    return true;
}

void QImSubplots3DNode::endDraw()
{
    ImGui::SetCursorPos(ImVec2(static_cast< float >(m_origin.x()), static_cast< float >(m_origin.y())));
    ImGui::Dummy(ImVec2(static_cast< float >(m_availableSize.width()), static_cast< float >(m_availableSize.height())));
}

QPoint QImSubplots3DNode::cellPosition(int index) const
{
    const int row   = (m_cols > 0) ? (index / m_cols) : 0;
    const int col   = (m_cols > 0) ? (index % m_cols) : 0;
    const std::vector< double > columnSizes = trackSizes(m_cols, m_availableSize.width(), m_columnRatios);
    const std::vector< double > rowSizes = trackSizes(m_rows, m_availableSize.height(), m_rowRatios);

    double xOffset = 0.0;
    double yOffset = 0.0;
    for (int c = 0; c < col && c < static_cast< int >(columnSizes.size()); ++c) {
        xOffset += columnSizes[ c ];
    }
    for (int r = 0; r < row && r < static_cast< int >(rowSizes.size()); ++r) {
        yOffset += rowSizes[ r ];
    }

    return QPoint(
        static_cast< int >(m_origin.x() + xOffset),
        static_cast< int >(m_origin.y() + yOffset)
    );
}

QSizeF QImSubplots3DNode::cellSize(int index) const
{
    const int row = (m_cols > 0) ? (index / m_cols) : 0;
    const int col = (m_cols > 0) ? (index % m_cols) : 0;
    const std::vector< double > columnSizes = trackSizes(m_cols, m_availableSize.width(), m_columnRatios);
    const std::vector< double > rowSizes = trackSizes(m_rows, m_availableSize.height(), m_rowRatios);
    const double width = col >= 0 && col < static_cast< int >(columnSizes.size()) ? columnSizes[ col ] : m_availableSize.width();
    const double height = row >= 0 && row < static_cast< int >(rowSizes.size()) ? rowSizes[ row ] : m_availableSize.height();
    return QSizeF(width, height);
}

std::vector< double > QImSubplots3DNode::trackSizes(int count, double totalPixels, const std::vector< float >& ratios) const
{
    std::vector< double > sizes;
    if (count <= 0) {
        return sizes;
    }

    sizes.resize(static_cast< std::size_t >(count), 0.0);
    const bool useRatios = static_cast< int >(ratios.size()) == count;
    double ratioSum = 0.0;
    if (useRatios) {
        for (float ratio : ratios) {
            ratioSum += std::max(0.0f, ratio);
        }
    }

    if (!useRatios || ratioSum <= 0.0) {
        std::fill(sizes.begin(), sizes.end(), totalPixels / static_cast< double >(count));
        return sizes;
    }

    for (int i = 0; i < count; ++i) {
        sizes[ static_cast< std::size_t >(i) ] = totalPixels * std::max(0.0f, ratios[ static_cast< std::size_t >(i) ]) / ratioSum;
    }
    return sizes;
}
}  // namespace QIM
