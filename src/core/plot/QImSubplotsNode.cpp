#include "QImSubplotsNode.h"
#include "implot.h"
#include "QtImGuiUtils.h"
#include <algorithm>
#include <QHash>
#include <QList>
#include <QDebug>
// qim
#include "QImPlotNode.h"
namespace QIM
{

// PIMPL 实现（完全隐藏 ImPlot 依赖）
class QImSubplotsNode::PrivateData
{
    QIM_DECLARE_PUBLIC(QImSubplotsNode)
public:
    struct ManualPlotLayout
    {
        QPointer< QImPlotNode > plot;
        std::vector< int > indices;
    };

    PrivateData(QImSubplotsNode* q) : q_ptr(q)
    {
    }
    std::vector< int > normalizeIndices(const std::vector< int >& indices) const;
    bool isValidManualIndices(const std::vector< int >& indices) const;
    bool isRectangularSelection(const std::vector< int >& indices) const;
    bool layoutIntersects(const std::vector< int >& left, const std::vector< int >& right) const;
    ManualPlotLayout* findManualLayout(const std::vector< int >& indices);
    const ManualPlotLayout* findManualLayout(const std::vector< int >& indices) const;
    void removeManualLayout(QImPlotNode* plot);
    ImVec2 resolveTotalSize() const;
    ImVec2 resolveContentSize(const ImVec2& visibleSize) const;
    bool beginScrollableArea(const ImVec2& visibleSize, const ImVec2& contentSize);
    void endScrollableArea();
    std::vector< float > resolveTrackSizes(int count, float totalPixels, const std::vector< float >& ratios, float spacing) const;
    ImRect manualLayoutRect(const std::vector< int >& indices, const ImVec2& origin, const ImVec2& totalSize, const ImVec2& spacing) const;
    void renderManualLayouts();

    // 属性存储（使用 QByteArray 缓存 UTF-8，避免渲染时转换）
    QByteArray titleUtf8;
    int rows = 1;
    int cols = 1;
    std::vector< float > rowRatios;
    std::vector< float > columnRatios;
    std::vector< float > lastRowRatios;
    std::vector< float > lastColumnRatios;
    constexpr static const float epsilon = 0.001f;
    bool trackGridRatios {
        false
    };  ///< 监测subplot的grid信息变化，如果为true，每次绘图都会检测行列的比例是否变化，如果变化将会发出gridInfoChanged信号
    ImVec2 size = ImVec2(-1, -1);
    ImVec2 minimumPlotSize = ImVec2(0, 0);
    bool scrollableAreaOpen = false;
    ImPlotSubplotFlags subplotFlags { ImPlotSubplotFlags_None };
    std::vector< ManualPlotLayout > manualLayouts;
};

std::vector< int > QImSubplotsNode::PrivateData::normalizeIndices(const std::vector< int >& indices) const
{
    std::vector< int > normalized;
    normalized.reserve(indices.size());
    for (int index : indices) {
        if (index > 0) {
            normalized.push_back(index);
        }
    }
    std::sort(normalized.begin(), normalized.end());
    normalized.erase(std::unique(normalized.begin(), normalized.end()), normalized.end());
    return normalized;
}

bool QImSubplotsNode::PrivateData::isValidManualIndices(const std::vector< int >& indices) const
{
    if (indices.empty()) {
        return false;
    }
    const int maxIndex = rows * cols;
    if (maxIndex <= 0) {
        return false;
    }
    for (int index : indices) {
        if (index < 1 || index > maxIndex) {
            return false;
        }
    }
    return isRectangularSelection(indices);
}

bool QImSubplotsNode::PrivateData::isRectangularSelection(const std::vector< int >& indices) const
{
    if (indices.empty() || rows <= 0 || cols <= 0) {
        return false;
    }

    int minRow = rows;
    int maxRow = 0;
    int minCol = cols;
    int maxCol = 0;
    QHash< int, bool > indexLookup;
    for (int index : indices) {
        const int zeroBased = index - 1;
        const int row = zeroBased / cols;
        const int col = zeroBased % cols;
        minRow = std::min(minRow, row);
        maxRow = std::max(maxRow, row);
        minCol = std::min(minCol, col);
        maxCol = std::max(maxCol, col);
        indexLookup.insert(index, true);
    }

    const int expectedCount = (maxRow - minRow + 1) * (maxCol - minCol + 1);
    if (expectedCount != static_cast< int >(indices.size())) {
        return false;
    }

    for (int row = minRow; row <= maxRow; ++row) {
        for (int col = minCol; col <= maxCol; ++col) {
            const int index = row * cols + col + 1;
            if (!indexLookup.contains(index)) {
                return false;
            }
        }
    }
    return true;
}

bool QImSubplotsNode::PrivateData::layoutIntersects(const std::vector< int >& left, const std::vector< int >& right) const
{
    for (int leftIndex : left) {
        if (std::binary_search(right.begin(), right.end(), leftIndex)) {
            return true;
        }
    }
    return false;
}

QImSubplotsNode::PrivateData::ManualPlotLayout* QImSubplotsNode::PrivateData::findManualLayout(const std::vector< int >& indices)
{
    for (ManualPlotLayout& layout : manualLayouts) {
        if (layout.indices == indices) {
            return &layout;
        }
    }
    return nullptr;
}

const QImSubplotsNode::PrivateData::ManualPlotLayout* QImSubplotsNode::PrivateData::findManualLayout(const std::vector< int >& indices) const
{
    for (const ManualPlotLayout& layout : manualLayouts) {
        if (layout.indices == indices) {
            return &layout;
        }
    }
    return nullptr;
}

void QImSubplotsNode::PrivateData::removeManualLayout(QImPlotNode* plot)
{
    manualLayouts.erase(
        std::remove_if(manualLayouts.begin(), manualLayouts.end(), [plot](const ManualPlotLayout& layout) { return layout.plot == plot; }),
        manualLayouts.end());
}

ImVec2 QImSubplotsNode::PrivateData::resolveTotalSize() const
{
    ImVec2 totalSize = size;
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    if (totalSize.x <= 0.0f) {
        totalSize.x = avail.x;
    }
    if (totalSize.y <= 0.0f) {
        totalSize.y = avail.y;
    }
    return totalSize;
}

ImVec2 QImSubplotsNode::PrivateData::resolveContentSize(const ImVec2& visibleSize) const
{
    ImVec2 contentSize = visibleSize;
    const ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

    if (minimumPlotSize.x > 0.0f && cols > 0) {
        const float minWidth =
            minimumPlotSize.x * static_cast< float >(cols) + spacing.x * static_cast< float >(std::max(0, cols - 1));
        contentSize.x = std::max(contentSize.x, minWidth);
    }
    if (minimumPlotSize.y > 0.0f && rows > 0) {
        const float minHeight =
            minimumPlotSize.y * static_cast< float >(rows) + spacing.y * static_cast< float >(std::max(0, rows - 1));
        contentSize.y = std::max(contentSize.y, minHeight);
    }
    return contentSize;
}

bool QImSubplotsNode::PrivateData::beginScrollableArea(const ImVec2& visibleSize, const ImVec2& contentSize)
{
    scrollableAreaOpen = false;
    const bool needsHorizontalScroll = contentSize.x > visibleSize.x + epsilon;
    const bool needsVerticalScroll   = contentSize.y > visibleSize.y + epsilon;
    if (!needsHorizontalScroll && !needsVerticalScroll) {
        return false;
    }

    ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("##SubplotsScrollArea", visibleSize, false, flags);
    scrollableAreaOpen = true;
    return true;
}

void QImSubplotsNode::PrivateData::endScrollableArea()
{
    if (scrollableAreaOpen) {
        ImGui::EndChild();
        scrollableAreaOpen = false;
    }
}

std::vector< float > QImSubplotsNode::PrivateData::resolveTrackSizes(
    int count, float totalPixels, const std::vector< float >& ratios, float spacing) const
{
    std::vector< float > sizes;
    if (count <= 0) {
        return sizes;
    }

    sizes.resize(count, 0.0f);
    const float availablePixels = std::max(0.0f, totalPixels - spacing * static_cast< float >(count - 1));
    const bool useRatios = static_cast< int >(ratios.size()) == count;
    float ratioSum = 0.0f;
    if (useRatios) {
        for (float ratio : ratios) {
            ratioSum += std::max(0.0f, ratio);
        }
    }

    if (!useRatios || ratioSum <= 0.0f) {
        const float sizePerTrack = availablePixels / static_cast< float >(count);
        std::fill(sizes.begin(), sizes.end(), sizePerTrack);
        return sizes;
    }

    for (int i = 0; i < count; ++i) {
        sizes[ i ] = availablePixels * std::max(0.0f, ratios[ i ]) / ratioSum;
    }
    return sizes;
}

ImRect QImSubplotsNode::PrivateData::manualLayoutRect(
    const std::vector< int >& indices, const ImVec2& origin, const ImVec2& totalSize, const ImVec2& spacing) const
{
    const std::vector< float > columnSizes = resolveTrackSizes(cols, totalSize.x, columnRatios, spacing.x);
    const std::vector< float > rowSizes = resolveTrackSizes(rows, totalSize.y, rowRatios, spacing.y);

    std::vector< float > columnOffsets(cols, 0.0f);
    std::vector< float > rowOffsets(rows, 0.0f);
    for (int i = 1; i < cols; ++i) {
        columnOffsets[ i ] = columnOffsets[ i - 1 ] + columnSizes[ i - 1 ] + spacing.x;
    }
    for (int i = 1; i < rows; ++i) {
        rowOffsets[ i ] = rowOffsets[ i - 1 ] + rowSizes[ i - 1 ] + spacing.y;
    }

    int minRow = rows;
    int maxRow = 0;
    int minCol = cols;
    int maxCol = 0;
    for (int index : indices) {
        const int zeroBased = index - 1;
        minRow = std::min(minRow, zeroBased / cols);
        maxRow = std::max(maxRow, zeroBased / cols);
        minCol = std::min(minCol, zeroBased % cols);
        maxCol = std::max(maxCol, zeroBased % cols);
    }

    const ImVec2 min(origin.x + columnOffsets[ minCol ], origin.y + rowOffsets[ minRow ]);
    float width = 0.0f;
    float height = 0.0f;
    for (int col = minCol; col <= maxCol; ++col) {
        width += columnSizes[ col ];
    }
    for (int row = minRow; row <= maxRow; ++row) {
        height += rowSizes[ row ];
    }
    width += spacing.x * static_cast< float >(maxCol - minCol);
    height += spacing.y * static_cast< float >(maxRow - minRow);
    return ImRect(min, ImVec2(min.x + width, min.y + height));
}

void QImSubplotsNode::PrivateData::renderManualLayouts()
{
    Q_Q(QImSubplotsNode);
    const ImVec2 visibleSize = resolveTotalSize();
    const ImVec2 contentSize = resolveContentSize(visibleSize);
    beginScrollableArea(visibleSize, contentSize);

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
    ImGui::Dummy(contentSize);

    for (QImAbstractNode* child : q->childrenNodesZOrdered()) {
        QImPlotNode* plotNode = qobject_cast< QImPlotNode* >(child);
        if (!plotNode) {
            continue;
        }

        auto it = std::find_if(manualLayouts.begin(), manualLayouts.end(), [plotNode](const ManualPlotLayout& layout) {
            return layout.plot == plotNode;
        });
        if (it == manualLayouts.end() || !it->plot) {
            continue;
        }

        const ImRect rect = manualLayoutRect(it->indices, origin, contentSize, spacing);
        ImGui::SetCursorScreenPos(rect.Min);
        ImGui::PushID(plotNode);
        ImGui::BeginChild(
            "##ManualSubplotCell",
            rect.GetSize(),
            false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotMinSize, ImVec2(1.0f, 1.0f));
        plotNode->render();
        ImPlot::PopStyleVar();
        ImGui::EndChild();
        ImGui::PopID();
    }
    endScrollableArea();
}

QImSubplotsNode::QImSubplotsNode(QObject* parent) : QImAbstractNode(parent), QIM_PIMPL_CONSTRUCT
{
    setObjectName(QStringLiteral("SubplotsNode"));
    setTitle(QStringLiteral("##SubplotsNode"));
}

QImSubplotsNode::QImSubplotsNode(const QString& title, QObject* parent) : QImAbstractNode(parent), QIM_PIMPL_CONSTRUCT
{
    setObjectName(QStringLiteral("SubplotsNode"));
    setTitle(title);
}

QImSubplotsNode::~QImSubplotsNode()
{
}

// === 属性实现 ===

QString QImSubplotsNode::title() const
{
    QIM_DC(d);
    return QString::fromUtf8(d->titleUtf8);
}

void QImSubplotsNode::setTitle(const QString& title)
{
    QIM_D(d);
    const QByteArray utf8 = title.toUtf8();
    if (d->titleUtf8 != utf8) {
        d->titleUtf8 = utf8;
        Q_EMIT titleChanged(title);
    }
}

int QImSubplotsNode::rows() const
{
    QIM_DC(d);
    return d->rows;
}

void QImSubplotsNode::setRows(int r)
{
    QIM_D(d);
    if (d->rows != r && r > 0) {
        d->rows = r;
        Q_EMIT gridInfoChanged();
    }
}

int QImSubplotsNode::columns() const
{
    QIM_DC(d);
    return d->cols;
}

void QImSubplotsNode::setColumns(int c)
{
    QIM_D(d);
    if (d->cols != c && c > 0) {
        d->cols = c;
        Q_EMIT gridInfoChanged();
    }
}

std::vector< float > QImSubplotsNode::rowRatios() const
{
    return d_ptr->rowRatios;
}

void QImSubplotsNode::setRowRatios(const std::vector< float >& row_ratios)
{
    QIM_D(d);
    if (!fuzzyEqual< float >(d->rowRatios, row_ratios, PrivateData::epsilon)) {
        d_ptr->rowRatios     = row_ratios;
        d_ptr->lastRowRatios = row_ratios;
        Q_EMIT gridInfoChanged();
    }
}

std::vector< float > QImSubplotsNode::columnRatios() const
{
    return d_ptr->columnRatios;
}

void QImSubplotsNode::setColumnRatios(const std::vector< float >& col_ratios)
{
    QIM_D(d);
    if (!fuzzyEqual< float >(d->columnRatios, col_ratios, PrivateData::epsilon)) {
        d_ptr->columnRatios     = col_ratios;
        d_ptr->lastColumnRatios = col_ratios;
        Q_EMIT gridInfoChanged();
    }
}

void QImSubplotsNode::setGrid(int r, int c, const std::vector< float >& row_ratios, const std::vector< float >& col_ratios)
{
    {
        QSignalBlocker b(this);
        setRows(r);
        setColumns(c);
        setRowRatios(row_ratios);
        setColumnRatios(col_ratios);
    }
    // 只发一次信号
    Q_EMIT gridInfoChanged();
}

QSizeF QImSubplotsNode::size() const
{
    QIM_DC(d);
    return toQSizeF(d->size);
}

/**
 * @brief ImPlot::BeginSubplots 的 size 参数用于指定整个 subplot 网格的总尺寸（而非单个子图的尺寸）
 *
 * - ImVec2(w, h):其中 w>0, h>0 网格精确固定为 w × h 像素
 * - ImVec2(-1, h):宽度填满父窗口可用宽度，高度固定为 h
 * - ImVec2(w, -1):宽度固定为 w，高度填满父窗口可用高度
 * - ImVec2(-1, -1):完全填满父窗口客户区（最常用）
 * - ImVec2(0, 0):使用默认大小
 * @param size
 */
void QImSubplotsNode::setSize(const QSizeF& size)
{
    QIM_D(d);
    ImVec2 newSize = toImVec2(size);

    if (fuzzyEqual(d->size, newSize)) {
        d->size = newSize;
    }
}

QSizeF QImSubplotsNode::minimumPlotSize() const
{
    QIM_DC(d);
    return toQSizeF(d->minimumPlotSize);
}

void QImSubplotsNode::setMinimumPlotSize(const QSizeF& size)
{
    QIM_D(d);
    ImVec2 newSize(std::max(0.0, size.width()), std::max(0.0, size.height()));
    if (!fuzzyEqual(d->minimumPlotSize, newSize)) {
        d->minimumPlotSize = newSize;
    }
}

// === 语义化标志属性 ===

bool QImSubplotsNode::isTitleEnabled() const
{
    QIM_DC(d);
    return !(d->subplotFlags & ImPlotSubplotFlags_NoTitle);
}

void QImSubplotsNode::setTitleEnabled(bool on)
{
    QIM_D(d);
    const bool current = isTitleEnabled();
    if (current != on) {
        if (on) {
            d->subplotFlags &= ~ImPlotSubplotFlags_NoTitle;  // 清除 NoTitle 标志
        } else {
            d->subplotFlags |= ImPlotSubplotFlags_NoTitle;  // 设置 NoTitle 标志
        }
        Q_EMIT subplotFlagChanged();
    }
}

bool QImSubplotsNode::isLegendEnabled() const
{
    QIM_DC(d);
    return !(d->subplotFlags & ImPlotSubplotFlags_NoLegend);
}

void QImSubplotsNode::setLegendEnabled(bool on)
{
    QIM_D(d);
    const bool current = isLegendEnabled();
    if (current != on) {
        if (on) {
            d->subplotFlags &= ~ImPlotSubplotFlags_NoLegend;
        } else {
            d->subplotFlags |= ImPlotSubplotFlags_NoLegend;
        }
        Q_EMIT subplotFlagChanged();
    }
}

bool QImSubplotsNode::isDefaultMenusEnabled() const
{
    QIM_DC(d);
    return !(d->subplotFlags & ImPlotSubplotFlags_NoMenus);
}

void QImSubplotsNode::setDefaultMenusEnabled(bool on)
{
    QIM_D(d);
    const bool current = isDefaultMenusEnabled();
    if (current != on) {
        if (on) {
            d->subplotFlags &= ~ImPlotSubplotFlags_NoMenus;
        } else {
            d->subplotFlags |= ImPlotSubplotFlags_NoMenus;
        }
        Q_EMIT subplotFlagChanged();
    }
}

bool QImSubplotsNode::isResizable() const
{
    QIM_DC(d);
    return !(d->subplotFlags & ImPlotSubplotFlags_NoResize);
}

void QImSubplotsNode::setResizable(bool on)
{
    QIM_D(d);
    const bool current = isResizable();
    if (current != on) {
        if (on) {
            d->subplotFlags &= ~ImPlotSubplotFlags_NoResize;
        } else {
            d->subplotFlags |= ImPlotSubplotFlags_NoResize;
        }
        Q_EMIT subplotFlagChanged();
    }
}

bool QImSubplotsNode::isAlignedEnabled() const
{
    QIM_DC(d);
    return !(d->subplotFlags & ImPlotSubplotFlags_NoAlign);
}

void QImSubplotsNode::setAlignedEnabled(bool on)
{
    QIM_D(d);
    const bool current = isAlignedEnabled();
    if (current != on) {
        if (on) {
            d->subplotFlags &= ~ImPlotSubplotFlags_NoAlign;
        } else {
            d->subplotFlags |= ImPlotSubplotFlags_NoAlign;
        }
        Q_EMIT subplotFlagChanged();
    }
}

bool QImSubplotsNode::isShareItemsEnabled() const
{
    QIM_DC(d);
    return d->subplotFlags & ImPlotSubplotFlags_ShareItems;
}

void QImSubplotsNode::setShareItemsEnabled(bool on)
{
    QIM_D(d);
    const bool current = isShareItemsEnabled();
    if (current != on) {
        if (on) {
            d->subplotFlags |= ImPlotSubplotFlags_ShareItems;
        } else {
            d->subplotFlags &= ~ImPlotSubplotFlags_ShareItems;
        }
        Q_EMIT subplotFlagChanged();
    }
}

// 链接行为（智能互斥处理）
bool QImSubplotsNode::isLinkRows() const
{
    QIM_DC(d);
    // LinkAllY 优先级高于 LinkRows，当存在 LinkAllY 时 LinkRows 无效
    return (d->subplotFlags & ImPlotSubplotFlags_LinkRows) && !(d->subplotFlags & ImPlotSubplotFlags_LinkAllY);
}

void QImSubplotsNode::setLinkRows(bool on)
{
    QIM_D(d);
    const bool current = isLinkRows();
    if (current != on) {
        if (on) {
            // 启用 LinkRows，同时禁用互斥的 LinkAllY
            d->subplotFlags |= ImPlotSubplotFlags_LinkRows;
            d->subplotFlags &= ~ImPlotSubplotFlags_LinkAllY;
        } else {
            d->subplotFlags &= ~ImPlotSubplotFlags_LinkRows;
        }
        Q_EMIT subplotFlagChanged();
    }
}

bool QImSubplotsNode::isLinkColumns() const
{
    QIM_DC(d);
    // LinkAllX 优先级高于 LinkCols，当存在 LinkAllX 时 LinkCols 无效
    return (d->subplotFlags & ImPlotSubplotFlags_LinkCols) && !(d->subplotFlags & ImPlotSubplotFlags_LinkAllX);
}

void QImSubplotsNode::setLinkColumns(bool on)
{
    QIM_D(d);
    const bool current = isLinkColumns();
    if (current != on) {
        if (on) {
            // 启用 LinkCols，同时禁用互斥的 LinkAllX
            d->subplotFlags |= ImPlotSubplotFlags_LinkCols;
            d->subplotFlags &= ~ImPlotSubplotFlags_LinkAllX;
        } else {
            d->subplotFlags &= ~ImPlotSubplotFlags_LinkCols;
        }
        Q_EMIT subplotFlagChanged();
    }
}

bool QImSubplotsNode::isLinkAllX() const
{
    QIM_DC(d);
    return d->subplotFlags & ImPlotSubplotFlags_LinkAllX;
}

void QImSubplotsNode::setLinkAllX(bool on)
{
    QIM_D(d);
    const bool current = isLinkAllX();
    if (current != on) {
        if (on) {
            // 启用 LinkAllX，同时禁用互斥的 LinkCols
            d->subplotFlags |= ImPlotSubplotFlags_LinkAllX;
            d->subplotFlags &= ~ImPlotSubplotFlags_LinkCols;
        } else {
            d->subplotFlags &= ~ImPlotSubplotFlags_LinkAllX;
        }
        Q_EMIT subplotFlagChanged();
    }
}

bool QImSubplotsNode::isLinkAllY() const
{
    QIM_DC(d);
    return d->subplotFlags & ImPlotSubplotFlags_LinkAllY;
}

void QImSubplotsNode::setLinkAllY(bool on)
{
    QIM_D(d);
    const bool current = isLinkAllY();
    if (current != on) {
        if (on) {
            // 启用 LinkAllY，同时禁用互斥的 LinkRows
            d->subplotFlags |= ImPlotSubplotFlags_LinkAllY;
            d->subplotFlags &= ~ImPlotSubplotFlags_LinkRows;
        } else {
            d->subplotFlags &= ~ImPlotSubplotFlags_LinkAllY;
        }
        Q_EMIT subplotFlagChanged();
    }
}

int QImSubplotsNode::gridCount() const
{
    return (d_ptr->cols * d_ptr->rows);
}

// 布局方向
bool QImSubplotsNode::isColumnMajor() const
{
    QIM_DC(d);
    return d->subplotFlags & ImPlotSubplotFlags_ColMajor;
}

void QImSubplotsNode::setColumnMajor(bool on)
{
    QIM_D(d);
    const bool current = isColumnMajor();
    if (current != on) {
        if (on) {
            d->subplotFlags |= ImPlotSubplotFlags_ColMajor;
        } else {
            d->subplotFlags &= ~ImPlotSubplotFlags_ColMajor;
        }
        Q_EMIT subplotFlagChanged();
    }
}

QList< QImPlotNode* > QImSubplotsNode::plotNodes() const
{
    const QList< QImAbstractNode* > cns = childrenNodes();
    QList< QImPlotNode* > res;
    for (QImAbstractNode* n : cns) {
        if (QImPlotNode* pn = qobject_cast< QImPlotNode* >(n)) {
            res.push_back(pn);
        }
    }
    return res;
}

void QImSubplotsNode::addPlotNode(QImPlotNode* plot)
{
    insertChildNode(childNodeCount(), plot);
}

void QImSubplotsNode::insertPlotNode(int plotIndex, QImPlotNode* plot)
{
    QList< QImPlotNode* > ps = plotNodes();
    if (plotIndex < ps.size()) {
        if (plotIndex < 0) {
            insertChildNode(0, plot);
        } else {
            int t = indexOfChildNode(ps[ plotIndex ]);
            if (t >= 0) {
                plotIndex = t + 1;
            } else {
                //? 不可达位置
                plotIndex = 0;
            }
        }
    } else {
        plotIndex = childNodeCount();
    }
    insertChildNode(plotIndex, plot);
}

int QImSubplotsNode::plotCount() const
{
    return plotNodes().size();
}

int QImSubplotsNode::plotNodeSubplotIndex(QImPlotNode* plot)
{
    return plotNodes().indexOf(plot);
}

bool QImSubplotsNode::isTrackGridRatiosEnabled() const
{
    return d_ptr->trackGridRatios;
}

void QImSubplotsNode::setTrackGridRatiosEnabled(bool on)
{
    d_ptr->trackGridRatios = on;
}

QImPlotNode* QImSubplotsNode::createPlotNode()
{
    if (hasManualPlotLayouts()) {
        return nullptr;
    }
    const QList< QImPlotNode* > pns = plotNodes();
    if (pns.size() >= gridCount()) {
        // 此时绘图数量和网格数量已经相等，不能再添加绘图
        return nullptr;
    }
    QImPlotNode* p = new QImPlotNode();
    addPlotNode(p);
    return p;
}

QImPlotNode* QImSubplotsNode::createPlotNode(const std::vector< int >& subplotIndices)
{
    QIM_D(d);
    const std::vector< int > normalized = d->normalizeIndices(subplotIndices);
    if (!d->isValidManualIndices(normalized)) {
        return nullptr;
    }

    if (auto* existingLayout = d->findManualLayout(normalized)) {
        return existingLayout->plot.data();
    }

    std::vector< QImPlotNode* > overlappingPlots;
    for (const PrivateData::ManualPlotLayout& layout : std::as_const(d->manualLayouts)) {
        if (layout.plot && d->layoutIntersects(layout.indices, normalized)) {
            overlappingPlots.push_back(layout.plot.data());
        }
    }
    for (QImPlotNode* plot : overlappingPlots) {
        removeChildNode(plot);
    }

    QImPlotNode* plotNode = new QImPlotNode();
    addPlotNode(plotNode);
    d->manualLayouts.push_back({ plotNode, normalized });
    return plotNode;
}

QImPlotNode* QImSubplotsNode::createPlotNode(std::initializer_list< int > subplotIndices)
{
    return createPlotNode(std::vector< int >(subplotIndices.begin(), subplotIndices.end()));
}

void QImSubplotsNode::clearManualPlotLayouts()
{
    d_ptr->manualLayouts.clear();
}

bool QImSubplotsNode::hasManualPlotLayouts() const
{
    return !d_ptr->manualLayouts.empty();
}

// === 渲染实现 ===

bool QImSubplotsNode::beginDraw()
{
    // 调用 ImPlot API（UTF-8 缓存零开销）
    QIM_D(d);
    d->scrollableAreaOpen = false;
    d->manualLayouts.erase(
        std::remove_if(d->manualLayouts.begin(), d->manualLayouts.end(), [](const PrivateData::ManualPlotLayout& layout) {
            return layout.plot.isNull();
        }),
        d->manualLayouts.end());

    if (!d->manualLayouts.empty()) {
        d->renderManualLayouts();
        return false;
    }

    const bool hasMinimumPlotSize = d->minimumPlotSize.x > 0.0f || d->minimumPlotSize.y > 0.0f;
    ImVec2 subplotSize = d->size;
    if (hasMinimumPlotSize) {
        const ImVec2 visibleSize = d->resolveTotalSize();
        subplotSize = d->resolveContentSize(visibleSize);
        d->beginScrollableArea(visibleSize, subplotSize);
    }

    float* row_ratios = nullptr;
    float* col_ratios = nullptr;
    if (!d->rowRatios.empty() && (static_cast< int >(d->rowRatios.size()) == d->rows)) {
        row_ratios = d->rowRatios.data();
    }
    if (!d->columnRatios.empty() && (static_cast< int >(d->columnRatios.size()) == d->cols)) {
        col_ratios = d->columnRatios.data();
    }
    bool on = ImPlot::BeginSubplots(d_ptr->titleUtf8.isEmpty() ? "##Subplots" : d_ptr->titleUtf8.constData(),
                                    d_ptr->rows,
                                    d_ptr->cols,
                                    subplotSize,
                                    d_ptr->subplotFlags,
                                    row_ratios,
                                    col_ratios);
    if (!on) {
        d->endScrollableArea();
    }
    // 检测row_ratios和col_ratios是否发生了变化，变化则发出gridInfoChanged信号
    if (d->trackGridRatios) {
        bool isGridChanged { false };
        if (row_ratios) {
            if (!fuzzyEqual(d->rowRatios, d->lastRowRatios, PrivateData::epsilon)) {
                d->lastRowRatios = d->rowRatios;
                isGridChanged    = true;
            }
        }
        if (col_ratios) {
            if (!fuzzyEqual(d->columnRatios, d->lastColumnRatios, PrivateData::epsilon)) {
                d->lastColumnRatios = d->columnRatios;
                isGridChanged       = true;
            }
        }
        if (isGridChanged) {
            Q_EMIT gridInfoChanged();
        }
    }
    return on;
}

void QImSubplotsNode::endDraw()
{
    if (!d_ptr->manualLayouts.empty()) {
        return;
    }
    ImPlot::EndSubplots();
    d_ptr->endScrollableArea();
}

}  // namespace QIM
