#include "QImFigureWidget.h"
#include <QDebug>
#include <algorithm>
#include "QImAbstractNode.h"
#include "QImTrackedValue.hpp"
#include "implot.h"
#include "implot3d.h"
#include "imgui.h"
#include "plot/QImSubplotsNode.h"
#include "plot/QImSubplots3DNode.h"
#include "plot/QImPlotNode.h"
#include "plot/QImPlot3DNode.h"
namespace QIM
{
class QImFigureContentNode : public QImAbstractNode
{
public:
    QImFigureContentNode(QImSubplotsNode* subplot2D, QImSubplots3DNode* subplot3D)
        : QImAbstractNode(), m_subplot2D(subplot2D), m_subplot3D(subplot3D)
    {
    }

    void addMixedPlot(QImAbstractNode* plot)
    {
        addMixedPlot(plot, {});
    }

    bool addMixedPlot(QImAbstractNode* plot, const std::vector< int >& indices)
    {
        if (!plot || findLayout(plot)) {
            return plot ? setMixedPlotIndices(plot, indices) : false;
        }
        if (!indices.empty() && !canUseIndices(indices, plot)) {
            return false;
        }
        m_mixedPlotOrder.push_back({ plot, normalizeIndices(indices) });
        return true;
    }

    void insertMixedPlot(int index, QImAbstractNode* plot)
    {
        insertMixedPlot(index, plot, {});
    }

    bool insertMixedPlot(int index, QImAbstractNode* plot, const std::vector< int >& indices)
    {
        if (!plot) {
            return false;
        }
        if (!indices.empty() && !canUseIndices(indices, plot)) {
            return false;
        }
        removeMixedPlot(plot);
        if (index < 0) {
            index = 0;
        }
        if (index > static_cast< int >(m_mixedPlotOrder.size())) {
            index = static_cast< int >(m_mixedPlotOrder.size());
        }
        m_mixedPlotOrder.insert(m_mixedPlotOrder.begin() + index, { plot, normalizeIndices(indices) });
        return true;
    }

    bool setMixedPlotIndices(QImAbstractNode* plot, const std::vector< int >& indices)
    {
        if (!plot) {
            return false;
        }
        if (!indices.empty() && !canUseIndices(indices, plot)) {
            return false;
        }
        if (MixedPlotLayout* layout = findLayout(plot)) {
            layout->indices = normalizeIndices(indices);
            return true;
        }
        return addMixedPlot(plot, indices);
    }

    void clearMixedPlotLayouts()
    {
        for (MixedPlotLayout& layout : m_mixedPlotOrder) {
            layout.indices.clear();
        }
    }

    bool canAddAutoPlot(QImAbstractNode* except = nullptr) const
    {
        std::vector< bool > occupied(gridCount() + 1, false);
        int autoCount = 1;
        for (const MixedPlotLayout& layout : m_mixedPlotOrder) {
            if (!layout.plot || layout.plot == except) {
                continue;
            }
            if (layout.indices.empty()) {
                ++autoCount;
                continue;
            }
            const std::vector< int > indices = normalizeIndices(layout.indices);
            if (!isValidIndices(indices)) {
                continue;
            }
            occupy(indices, occupied);
        }

        return autoCount <= freeCellCount(occupied);
    }

    bool canUseIndices(const std::vector< int >& indices, QImAbstractNode* except = nullptr) const
    {
        const std::vector< int > normalized = normalizeIndices(indices);
        if (!isValidIndices(normalized)) {
            return false;
        }

        std::vector< bool > occupied(gridCount() + 1, false);
        int autoCount = 0;
        for (const MixedPlotLayout& layout : m_mixedPlotOrder) {
            if (!layout.plot || layout.plot == except) {
                continue;
            }
            if (layout.indices.empty()) {
                ++autoCount;
                continue;
            }
            const std::vector< int > other = normalizeIndices(layout.indices);
            if (!isValidIndices(other)) {
                continue;
            }
            if (layoutIntersects(other, normalized)) {
                return false;
            }
            occupy(other, occupied);
        }

        occupy(normalized, occupied);
        return autoCount <= freeCellCount(occupied);
    }

    QImAbstractNode* mixedPlotAtIndices(const std::vector< int >& indices) const
    {
        const std::vector< int > normalized = normalizeIndices(indices);
        for (const MixedPlotLayout& layout : m_mixedPlotOrder) {
            if (layout.plot && normalizeIndices(layout.indices) == normalized) {
                return layout.plot.data();
            }
        }
        return nullptr;
    }

    void removeMixedPlot(QImAbstractNode* plot)
    {
        m_mixedPlotOrder.erase(std::remove_if(m_mixedPlotOrder.begin(),
                                              m_mixedPlotOrder.end(),
                                              [plot](const MixedPlotLayout& layout) { return layout.plot == plot; }),
                               m_mixedPlotOrder.end());
    }

    bool replaceMixedPlot(QImAbstractNode* oldPlot, QImAbstractNode* newPlot)
    {
        if (!oldPlot || !newPlot) {
            return false;
        }

        if (MixedPlotLayout* oldLayout = findLayout(oldPlot)) {
            oldLayout->plot = newPlot;
            return true;
        }

        return addMixedPlot(newPlot, {});
    }

    int mixedPlotOrderIndex(QImAbstractNode* plot) const
    {
        for (int index = 0; index < static_cast< int >(m_mixedPlotOrder.size()); ++index) {
            if (m_mixedPlotOrder[ index ].plot == plot) {
                return index;
            }
        }
        return -1;
    }

protected:
    bool beginDraw() override
    {
        pruneMissingPlots();
        const bool has2D = m_subplot2D && m_subplot2D->plotCount() > 0;
        const bool has3D = m_subplot3D && m_subplot3D->plotCount() > 0;
        const bool hasExplicitLayouts = hasExplicitMixedLayouts();
        if ((has2D || has3D) && (hasExplicitLayouts || (has2D && has3D))) {
            if (m_subplot2D) {
                m_subplot2D->setVisible(false);
            }
            if (m_subplot3D) {
                m_subplot3D->setVisible(false);
            }
            renderMixedGrid();
            return false;
        }

        if (m_subplot2D) {
            m_subplot2D->setVisible(!has3D || has2D);
            m_subplot2D->setSize(QSizeF(-1.0, -1.0));
        }

        if (m_subplot3D) {
            m_subplot3D->setVisible(has3D);
            m_subplot3D->setSize(QSizeF(-1.0, -1.0));
        }

        return true;
    }

    void endDraw() override
    {
    }

private:
    struct MixedPlotLayout
    {
        QPointer< QImAbstractNode > plot;
        std::vector< int > indices;
    };

    struct ResolvedPlotLayout
    {
        QPointer< QImAbstractNode > plot;
        std::vector< int > indices;
    };

    struct LayoutRect
    {
        ImVec2 pos;
        ImVec2 size;
    };

    ImPlotSubplotFlags subplotFlags() const
    {
        if (!m_subplot2D) {
            return ImPlotSubplotFlags_None;
        }

        ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;
        if (!m_subplot2D->isTitleEnabled())
            flags |= ImPlotSubplotFlags_NoTitle;
        if (!m_subplot2D->isLegendEnabled())
            flags |= ImPlotSubplotFlags_NoLegend;
        if (!m_subplot2D->isDefaultMenusEnabled())
            flags |= ImPlotSubplotFlags_NoMenus;
        if (!m_subplot2D->isResizable())
            flags |= ImPlotSubplotFlags_NoResize;
        if (!m_subplot2D->isAlignedEnabled())
            flags |= ImPlotSubplotFlags_NoAlign;
        if (m_subplot2D->isShareItemsEnabled())
            flags |= ImPlotSubplotFlags_ShareItems;
        if (m_subplot2D->isLinkRows())
            flags |= ImPlotSubplotFlags_LinkRows;
        if (m_subplot2D->isLinkColumns())
            flags |= ImPlotSubplotFlags_LinkCols;
        if (m_subplot2D->isLinkAllX())
            flags |= ImPlotSubplotFlags_LinkAllX;
        if (m_subplot2D->isLinkAllY())
            flags |= ImPlotSubplotFlags_LinkAllY;
        if (m_subplot2D->isColumnMajor())
            flags |= ImPlotSubplotFlags_ColMajor;
        return flags;
    }

    int rows() const
    {
        return m_subplot2D ? std::max(1, m_subplot2D->rows()) : 1;
    }

    int columns() const
    {
        return m_subplot2D ? std::max(1, m_subplot2D->columns()) : 1;
    }

    int gridCount() const
    {
        return rows() * columns();
    }

    std::vector< int > normalizeIndices(const std::vector< int >& indices) const
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

    bool isValidIndices(const std::vector< int >& indices) const
    {
        if (indices.empty()) {
            return false;
        }
        const int maxIndex = gridCount();
        for (int index : indices) {
            if (index < 1 || index > maxIndex) {
                return false;
            }
        }
        return isRectangularSelection(indices);
    }

    bool isRectangularSelection(const std::vector< int >& indices) const
    {
        if (indices.empty()) {
            return false;
        }

        const int colCount = columns();
        int minRow = rows();
        int maxRow = 0;
        int minCol = colCount;
        int maxCol = 0;
        std::vector< bool > selected(gridCount() + 1, false);
        for (int index : indices) {
            const int zeroBased = index - 1;
            const int row = zeroBased / colCount;
            const int col = zeroBased % colCount;
            minRow = std::min(minRow, row);
            maxRow = std::max(maxRow, row);
            minCol = std::min(minCol, col);
            maxCol = std::max(maxCol, col);
            selected[ index ] = true;
        }

        const int expectedCount = (maxRow - minRow + 1) * (maxCol - minCol + 1);
        if (expectedCount != static_cast< int >(indices.size())) {
            return false;
        }

        for (int row = minRow; row <= maxRow; ++row) {
            for (int col = minCol; col <= maxCol; ++col) {
                const int index = row * colCount + col + 1;
                if (!selected[ index ]) {
                    return false;
                }
            }
        }
        return true;
    }

    bool layoutIntersects(const std::vector< int >& left, const std::vector< int >& right) const
    {
        for (int index : left) {
            if (std::binary_search(right.begin(), right.end(), index)) {
                return true;
            }
        }
        return false;
    }

    void occupy(const std::vector< int >& indices, std::vector< bool >& occupied) const
    {
        for (int index : indices) {
            if (index >= 1 && index < static_cast< int >(occupied.size())) {
                occupied[ index ] = true;
            }
        }
    }

    int freeCellCount(const std::vector< bool >& occupied) const
    {
        int count = 0;
        for (int index = 1; index < static_cast< int >(occupied.size()); ++index) {
            if (!occupied[ index ]) {
                ++count;
            }
        }
        return count;
    }

    int firstFreeCell(const std::vector< bool >& occupied) const
    {
        for (int index = 1; index < static_cast< int >(occupied.size()); ++index) {
            if (!occupied[ index ]) {
                return index;
            }
        }
        return -1;
    }

    MixedPlotLayout* findLayout(QImAbstractNode* plot)
    {
        auto it = std::find_if(m_mixedPlotOrder.begin(), m_mixedPlotOrder.end(), [plot](const MixedPlotLayout& layout) {
            return layout.plot == plot;
        });
        return it == m_mixedPlotOrder.end() ? nullptr : &(*it);
    }

    bool hasExplicitMixedLayouts() const
    {
        for (const MixedPlotLayout& layout : m_mixedPlotOrder) {
            if (layout.plot && !layout.indices.empty()) {
                return true;
            }
        }
        return false;
    }

    void pruneMissingPlots()
    {
        m_mixedPlotOrder.erase(
            std::remove_if(m_mixedPlotOrder.begin(), m_mixedPlotOrder.end(), [](const MixedPlotLayout& layout) {
                return layout.plot.isNull();
            }),
            m_mixedPlotOrder.end());
    }

    std::vector< ResolvedPlotLayout > resolveLayouts() const
    {
        std::vector< ResolvedPlotLayout > resolved;
        std::vector< bool > occupied(gridCount() + 1, false);

        for (const MixedPlotLayout& layout : m_mixedPlotOrder) {
            if (!layout.plot || layout.indices.empty()) {
                continue;
            }
            const std::vector< int > indices = normalizeIndices(layout.indices);
            if (!isValidIndices(indices)) {
                continue;
            }
            bool intersects = false;
            for (int index : indices) {
                intersects = intersects || occupied[ index ];
            }
            if (intersects) {
                continue;
            }
            occupy(indices, occupied);
            resolved.push_back({ layout.plot, indices });
        }

        for (const MixedPlotLayout& layout : m_mixedPlotOrder) {
            if (!layout.plot || !layout.indices.empty()) {
                continue;
            }
            const int index = firstFreeCell(occupied);
            if (index < 1) {
                break;
            }
            occupied[ index ] = true;
            resolved.push_back({ layout.plot, std::vector< int > { index } });
        }

        return resolved;
    }

    bool hasSpanLayout(const std::vector< ResolvedPlotLayout >& layouts) const
    {
        for (const ResolvedPlotLayout& layout : layouts) {
            if (layout.indices.size() > 1) {
                return true;
            }
        }
        return false;
    }

    void renderMixedGrid()
    {
        if (!m_subplot2D || !m_subplot3D) {
            return;
        }

        const int rows = std::max(1, m_subplot2D->rows());
        const int cols = std::max(1, m_subplot2D->columns());
        const int gridCount = rows * cols;
        const std::vector< ResolvedPlotLayout > layouts = resolveLayouts();
        if (hasSpanLayout(layouts)) {
            renderManualMixedGrid(layouts);
            return;
        }

        std::vector< float > rowRatios = m_subplot2D->rowRatios();
        std::vector< float > columnRatios = m_subplot2D->columnRatios();
        float* rowRatiosData = static_cast< int >(rowRatios.size()) == rows ? rowRatios.data() : nullptr;
        float* columnRatiosData = static_cast< int >(columnRatios.size()) == cols ? columnRatios.data() : nullptr;
        std::vector< QPointer< QImAbstractNode > > cellPlots(gridCount + 1);
        for (const ResolvedPlotLayout& layout : layouts) {
            if (layout.plot && layout.indices.size() == 1) {
                cellPlots[ layout.indices.front() ] = layout.plot;
            }
        }

        if (!ImPlot::BeginSubplots("##FigureMixedSubplots",
                                   rows,
                                   cols,
                                   ImGui::GetContentRegionAvail(),
                                   subplotFlags(),
                                   rowRatiosData,
                                   columnRatiosData)) {
            return;
        }

        for (int cellIndex = 1; cellIndex <= gridCount; ++cellIndex) {
            QImAbstractNode* node = cellPlots[ cellIndex ];
            if (QImPlotNode* plot = qobject_cast< QImPlotNode* >(node)) {
                plot->render();
                continue;
            }

            QImPlot3DNode* plot3D = qobject_cast< QImPlot3DNode* >(node);
            if (!plot3D) {
                advanceEmptyCell(cellIndex);
                continue;
            }

            renderPlot3DCell(plot3D);
        }

        ImPlot::EndSubplots();
    }

    ImVec2 resolveTotalSize() const
    {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        QSizeF configuredSize = m_subplot2D ? m_subplot2D->size() : QSizeF(-1.0, -1.0);
        ImVec2 totalSize(
            configuredSize.width() > 0.0 ? static_cast< float >(configuredSize.width()) : avail.x,
            configuredSize.height() > 0.0 ? static_cast< float >(configuredSize.height()) : avail.y
        );
        return totalSize;
    }

    ImVec2 resolveContentSize(const ImVec2& visibleSize) const
    {
        ImVec2 contentSize = visibleSize;
        const QSizeF minimumPlotSize = m_subplot2D ? m_subplot2D->minimumPlotSize() : QSizeF();
        const ImVec2 spacing(0.0f, 0.0f);
        if (minimumPlotSize.width() > 0.0) {
            const float minWidth = static_cast< float >(minimumPlotSize.width()) * static_cast< float >(columns()) +
                                   spacing.x * static_cast< float >(std::max(0, columns() - 1));
            contentSize.x = std::max(contentSize.x, minWidth);
        }
        if (minimumPlotSize.height() > 0.0) {
            const float minHeight = static_cast< float >(minimumPlotSize.height()) * static_cast< float >(rows()) +
                                    spacing.y * static_cast< float >(std::max(0, rows() - 1));
            contentSize.y = std::max(contentSize.y, minHeight);
        }
        return contentSize;
    }

    bool beginScrollableArea(const ImVec2& visibleSize, const ImVec2& contentSize)
    {
        static constexpr float epsilon = 0.001f;
        const bool needsHorizontalScroll = contentSize.x > visibleSize.x + epsilon;
        const bool needsVerticalScroll = contentSize.y > visibleSize.y + epsilon;
        if (!needsHorizontalScroll && !needsVerticalScroll) {
            return false;
        }

        ImGui::BeginChild("##FigureMixedScrollArea", visibleSize, false, ImGuiWindowFlags_HorizontalScrollbar);
        return true;
    }

    std::vector< float > resolveTrackSizes(int count, float totalPixels, const std::vector< float >& ratios, float spacing) const
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

    LayoutRect manualLayoutRect(const std::vector< int >& indices, const ImVec2& origin, const ImVec2& totalSize, const ImVec2& spacing) const
    {
        const int rowCount = rows();
        const int colCount = columns();
        const std::vector< float > columnSizes = resolveTrackSizes(colCount, totalSize.x, m_subplot2D->columnRatios(), spacing.x);
        const std::vector< float > rowSizes = resolveTrackSizes(rowCount, totalSize.y, m_subplot2D->rowRatios(), spacing.y);

        std::vector< float > columnOffsets(colCount, 0.0f);
        std::vector< float > rowOffsets(rowCount, 0.0f);
        for (int i = 1; i < colCount; ++i) {
            columnOffsets[ i ] = columnOffsets[ i - 1 ] + columnSizes[ i - 1 ] + spacing.x;
        }
        for (int i = 1; i < rowCount; ++i) {
            rowOffsets[ i ] = rowOffsets[ i - 1 ] + rowSizes[ i - 1 ] + spacing.y;
        }

        int minRow = rowCount;
        int maxRow = 0;
        int minCol = colCount;
        int maxCol = 0;
        for (int index : indices) {
            const int zeroBased = index - 1;
            minRow = std::min(minRow, zeroBased / colCount);
            maxRow = std::max(maxRow, zeroBased / colCount);
            minCol = std::min(minCol, zeroBased % colCount);
            maxCol = std::max(maxCol, zeroBased % colCount);
        }

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

        return {
            ImVec2(origin.x + columnOffsets[ minCol ], origin.y + rowOffsets[ minRow ]),
            ImVec2(width, height)
        };
    }

    void renderManualMixedGrid(const std::vector< ResolvedPlotLayout >& layouts)
    {
        const ImVec2 visibleSize = resolveTotalSize();
        const ImVec2 contentSize = resolveContentSize(visibleSize);
        const bool scrollable = beginScrollableArea(visibleSize, contentSize);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec2 spacing(0.0f, 0.0f);
        ImGui::Dummy(contentSize);

        for (const ResolvedPlotLayout& layout : layouts) {
            QImAbstractNode* node = layout.plot;
            if (!node || layout.indices.empty()) {
                continue;
            }
            const LayoutRect rect = manualLayoutRect(layout.indices, origin, contentSize, spacing);
            renderPlotInRect(node, rect);
        }

        if (scrollable) {
            ImGui::EndChild();
        }
    }

    void renderPlotInRect(QImAbstractNode* node, const LayoutRect& rect)
    {
        ImGui::SetCursorScreenPos(rect.pos);
        ImGui::PushID(node);
        ImGui::BeginChild(
            "##MixedManualSubplotCell",
            rect.size,
            false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
        if (QImPlot3DNode* plot3D = qobject_cast< QImPlot3DNode* >(node)) {
            plot3D->setAutoSize(false);
            plot3D->setSize(QSizeF(rect.size.x, rect.size.y));
            plot3D->render();
        } else {
            node->render();
        }
        ImGui::EndChild();
        ImGui::PopID();
    }

    void advanceEmptyCell(int cellIndex)
    {
        static constexpr ImPlotFlags placeholderFlags =
            ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame | ImPlotFlags_NoInputs;
        ImGui::PushID(cellIndex);
        if (ImPlot::BeginPlot("##FigureMixedEmptyCell", ImVec2(-1.0f, -1.0f), placeholderFlags)) {
            ImPlot::SetupAxes(nullptr,
                              nullptr,
                              ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoMenus,
                              ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoMenus);
            ImPlot::EndPlot();
        }
        ImGui::PopID();
    }

    void renderPlot3DCell(QImPlot3DNode* plot3D)
    {
        if (!plot3D) {
            advanceEmptyCell(0);
            return;
        }

        static constexpr ImPlotFlags placeholderFlags =
            ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame | ImPlotFlags_NoInputs;
        ImGui::PushID(plot3D);
        if (!ImPlot::BeginPlot("##FigureMixed3DCell", ImVec2(-1.0f, -1.0f), placeholderFlags)) {
            ImGui::PopID();
            return;
        }
        ImPlot::SetupAxes(nullptr,
                          nullptr,
                          ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoMenus,
                          ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoMenus);

        const ImVec2 cellPos = ImPlot::GetPlotPos();
        const ImVec2 cellSize = ImPlot::GetPlotSize();
        ImPlot::EndPlot();

        const ImVec2 restoreCursor = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(cellPos);
        ImGui::BeginChild(
            "##Mixed3DSubplotCell",
            cellSize,
            false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);
        plot3D->setAutoSize(false);
        plot3D->setSize(QSizeF(cellSize.x, cellSize.y));
        plot3D->render();
        ImGui::EndChild();
        ImGui::PopID();
        ImGui::SetCursorScreenPos(restoreCursor);
    }

    QPointer< QImSubplotsNode > m_subplot2D;
    QPointer< QImSubplots3DNode > m_subplot3D;
    std::vector< MixedPlotLayout > m_mixedPlotOrder;
};

class QImFigureWidget::PrivateData
{
    QIM_DECLARE_PUBLIC(QImFigureWidget)
public:
    PrivateData(QImFigureWidget* p);
    void attachPlotNode(QImPlotNode* plot);
    void detachPlotNode(QImPlotNode* plot);
    void attachPlot3DNode(QImPlot3DNode* plot);
    void detachPlot3DNode(QImPlot3DNode* plot);
    void selectCoordinateNode(QImAbstractNode* plot);
    void removeCoordinateNode(QImAbstractNode* plot);
    void emitSelectionChangedIfNeeded(const QList< QImAbstractNode* >& oldSelection, QImAbstractNode* oldCurrent);

public:
    QImTrackedValue< QImPlotTheme > m_theme;
    QPointer< QImFigureContentNode > m_contentNode;
    QPointer< QImSubplotsNode > m_subplotNode;
    QPointer< QImSubplots3DNode > m_subplot3DNode;
    ImPlotContext* m_context { nullptr };
    ImPlot3DContext* m_context3D { nullptr };
    bool m_usingMatlabLayout { false };
    int m_matlabLayoutRows { 1 };
    int m_matlabLayoutCols { 1 };
    QList< QPointer< QImAbstractNode > > m_selectedCoordinateNodes;
    QPointer< QImAbstractNode > m_currentCoordinateNode;
};

QImFigureWidget::PrivateData::PrivateData(QImFigureWidget* p) : q_ptr(p)
{
}

void QImFigureWidget::PrivateData::attachPlotNode(QImPlotNode* plot)
{
    QImFigureWidget* q = q_func();
    if (!plot) {
        return;
    }
    QObject::connect(plot, &QImPlotNode::plotClicked, q, &QImFigureWidget::onPlotNodeClicked, Qt::UniqueConnection);
}

void QImFigureWidget::PrivateData::detachPlotNode(QImPlotNode* plot)
{
    QImFigureWidget* q = q_func();
    if (!plot) {
        return;
    }
    QObject::disconnect(plot, &QImPlotNode::plotClicked, q, &QImFigureWidget::onPlotNodeClicked);
    removeCoordinateNode(plot);
}

void QImFigureWidget::PrivateData::attachPlot3DNode(QImPlot3DNode* plot)
{
    QImFigureWidget* q = q_func();
    if (!plot) {
        return;
    }
    QObject::connect(plot, &QImPlot3DNode::plotClicked, q, &QImFigureWidget::onPlot3DNodeClicked, Qt::UniqueConnection);
}

void QImFigureWidget::PrivateData::detachPlot3DNode(QImPlot3DNode* plot)
{
    QImFigureWidget* q = q_func();
    if (!plot) {
        return;
    }
    QObject::disconnect(plot, &QImPlot3DNode::plotClicked, q, &QImFigureWidget::onPlot3DNodeClicked);
    removeCoordinateNode(plot);
}

void QImFigureWidget::PrivateData::selectCoordinateNode(QImAbstractNode* plot)
{
    if (!plot) {
        return;
    }

    const QList< QImAbstractNode* > oldSelection = q_func()->selectedCoordinateNodes();
    QImAbstractNode* oldCurrent = m_currentCoordinateNode.data();

    m_selectedCoordinateNodes.erase(
        std::remove_if(m_selectedCoordinateNodes.begin(),
                       m_selectedCoordinateNodes.end(),
                       [](const QPointer< QImAbstractNode >& selected) {
                           return selected.isNull();
                       }),
        m_selectedCoordinateNodes.end());

    for (QPointer< QImAbstractNode >& selected : m_selectedCoordinateNodes) {
        if (QImPlotNode* plot2D = qobject_cast< QImPlotNode* >(selected.data())) {
            plot2D->setSelected(false);
        } else if (QImPlot3DNode* plot3D = qobject_cast< QImPlot3DNode* >(selected.data())) {
            plot3D->setSelected(false);
        }
    }
    m_selectedCoordinateNodes.clear();

    const auto existing = std::find_if(m_selectedCoordinateNodes.begin(),
                                       m_selectedCoordinateNodes.end(),
                                       [plot](const QPointer< QImAbstractNode >& selected) {
                                           return selected == plot;
                                       });
    if (existing == m_selectedCoordinateNodes.end()) {
        m_selectedCoordinateNodes.push_back(plot);
    }

    if (QImPlotNode* plot2D = qobject_cast< QImPlotNode* >(plot)) {
        plot2D->setSelected(true);
    } else if (QImPlot3DNode* plot3D = qobject_cast< QImPlot3DNode* >(plot)) {
        plot3D->setSelected(true);
    }
    m_currentCoordinateNode = plot;
    emitSelectionChangedIfNeeded(oldSelection, oldCurrent);
}

void QImFigureWidget::PrivateData::removeCoordinateNode(QImAbstractNode* plot)
{
    if (!plot) {
        return;
    }

    const QList< QImAbstractNode* > oldSelection = q_func()->selectedCoordinateNodes();
    QImAbstractNode* oldCurrent = m_currentCoordinateNode.data();

    m_selectedCoordinateNodes.erase(
        std::remove_if(m_selectedCoordinateNodes.begin(),
                       m_selectedCoordinateNodes.end(),
                       [plot](const QPointer< QImAbstractNode >& selected) {
                           return selected.isNull() || selected == plot;
                       }),
        m_selectedCoordinateNodes.end());
    if (QImPlotNode* plot2D = qobject_cast< QImPlotNode* >(plot)) {
        plot2D->setSelected(false);
    } else if (QImPlot3DNode* plot3D = qobject_cast< QImPlot3DNode* >(plot)) {
        plot3D->setSelected(false);
    }
    if (m_currentCoordinateNode == plot) {
        m_currentCoordinateNode = m_selectedCoordinateNodes.isEmpty() ? nullptr : m_selectedCoordinateNodes.back();
    }
    emitSelectionChangedIfNeeded(oldSelection, oldCurrent);
}

void QImFigureWidget::PrivateData::emitSelectionChangedIfNeeded(const QList< QImAbstractNode* >& oldSelection,
                                                                QImAbstractNode* oldCurrent)
{
    QImFigureWidget* q = q_func();
    const QList< QImAbstractNode* > newSelection = q->selectedCoordinateNodes();
    QImAbstractNode* newCurrent = m_currentCoordinateNode.data();
    if (oldCurrent != newCurrent) {
        Q_EMIT q->currentCoordinateNodeChanged(newCurrent);
    }
    if (oldSelection != newSelection || oldCurrent != newCurrent) {
        Q_EMIT q->coordinateSelectionChanged(newSelection, newCurrent);
    }
}

//----------------------------------------------------
// QImFigureWidget
//----------------------------------------------------
QImFigureWidget::QImFigureWidget(QWidget* parent, Qt::WindowFlags f) : QImWidget(parent, f), QIM_PIMPL_CONSTRUCT
{
    d_ptr->m_subplotNode = new QImSubplotsNode();
    d_ptr->m_subplot3DNode = new QImSubplots3DNode();
    d_ptr->m_contentNode = new QImFigureContentNode(d_ptr->m_subplotNode.data(), d_ptr->m_subplot3DNode.data());
    addRenderNode(d_ptr->m_contentNode.data());
    d_ptr->m_contentNode->addChildNode(d_ptr->m_subplotNode.data());
    d_ptr->m_contentNode->addChildNode(d_ptr->m_subplot3DNode.data());
    d_ptr->m_subplotNode->setTitleEnabled(true);
    connect(d_ptr->m_subplotNode, &QImSubplotsNode::childNodeAdded, this, &QImFigureWidget::onSubplotChildNodeAdded);
    connect(d_ptr->m_subplotNode, &QImSubplotsNode::childNodeRemoved, this, &QImFigureWidget::onSubplotChildNodeRemoved);
    connect(d_ptr->m_subplot3DNode, &QImSubplots3DNode::childNodeAdded, this, &QImFigureWidget::onSubplot3DChildNodeAdded);
    connect(d_ptr->m_subplot3DNode, &QImSubplots3DNode::childNodeRemoved, this, &QImFigureWidget::onSubplot3DChildNodeRemoved);
}

QImFigureWidget::~QImFigureWidget()
{
    if (d_ptr->m_context3D) {
        ImPlot3D::DestroyContext(d_ptr->m_context3D);
        d_ptr->m_context3D = nullptr;
    }
    if (d_ptr->m_context) {
        ImPlot::DestroyContext(d_ptr->m_context);
        d_ptr->m_context = nullptr;
    }
}

void QImFigureWidget::setPlotTheme(const QImPlotTheme& t)
{
    d_ptr->m_theme = t;
}

const QImPlotTheme& QImFigureWidget::plotTheme() const
{
    return d_ptr->m_theme.value();
}

void QImFigureWidget::setSubplotGrid(int rows, int cols, const std::vector< float >& rowsRatios, const std::vector< float >& colsRatios)
{
    QIM_D(d);
    if (d->m_subplotNode) {
        d->m_usingMatlabLayout = false;
        d->m_subplotNode->clearManualPlotLayouts();
        d->m_subplotNode->setGrid(rows, cols, rowsRatios, colsRatios);
    }
    if (d->m_subplot3DNode) {
        d->m_subplot3DNode->setGrid(rows, cols, rowsRatios, colsRatios);
    }
    if (d->m_contentNode) {
        d->m_contentNode->clearMixedPlotLayouts();
    }
}

int QImFigureWidget::subplotGridRows() const
{
    QIM_DC(d);
    if (d->m_subplotNode) {
        return d->m_subplotNode->rows();
    }
    return -1;
}

int QImFigureWidget::subplotGridColumns() const
{
    QIM_DC(d);
    if (d->m_subplotNode) {
        return d->m_subplotNode->columns();
    }
    return -1;
}

std::vector< float > QImFigureWidget::subplotGridRowRatios() const
{
    QIM_DC(d);
    if (d->m_subplotNode) {
        return d->m_subplotNode->rowRatios();
    }
    return {};
}

std::vector< float > QImFigureWidget::subplotGridColumnRatios() const
{
    QIM_DC(d);
    if (d->m_subplotNode) {
        return d->m_subplotNode->columnRatios();
    }
    return {};
}

QImSubplotsNode* QImFigureWidget::subplotNode() const
{
    return d_ptr->m_subplotNode.data();
}

QSizeF QImFigureWidget::minimumPlotSize() const
{
    QIM_DC(d);
    if (d->m_subplotNode) {
        return d->m_subplotNode->minimumPlotSize();
    }
    return QSizeF();
}

void QImFigureWidget::setMinimumPlotSize(const QSizeF& size)
{
    QIM_D(d);
    if (d->m_subplotNode) {
        d->m_subplotNode->setMinimumPlotSize(size);
    }
}

/**
 * @brief 创建一个绘图，这个绘图会作为subplot的子节点
 *
 * @note QImFigureWidget会默认创建一个网格，也就是如果@ref QImFigureWidget 没有调用过@ref setSubplotGrid ，
 * 你通过此函数添加一个绘图，但再次调用时，会超过当前网格总数而返回nullptr
 *
 * @return 如果当前添加的绘图超过subplot的网格数量，此函数返回nullptr
 */
QImPlotNode* QImFigureWidget::createPlotNode()
{
    if (!d_ptr->m_subplotNode || !d_ptr->m_contentNode || !d_ptr->m_contentNode->canAddAutoPlot()) {
        return nullptr;
    }
    d_ptr->m_usingMatlabLayout = false;
    QImPlotNode* plot = new QImPlotNode();
    d_ptr->m_subplotNode->addPlotNode(plot);
    if (plot && d_ptr->m_contentNode) {
        d_ptr->m_contentNode->addMixedPlot(plot);
    }
    return plot;
}

QImPlotNode* QImFigureWidget::createPlotNode(const std::vector< int >& subplotIndices)
{
    QIM_D(d);
    if (!d->m_subplotNode || !d->m_contentNode) {
        return nullptr;
    }
    if (QImPlotNode* existingPlot = qobject_cast< QImPlotNode* >(d->m_contentNode->mixedPlotAtIndices(subplotIndices))) {
        return existingPlot;
    }
    if (!d->m_contentNode->canUseIndices(subplotIndices)) {
        qWarning() << "QImFigureWidget::createPlotNode received invalid, non-rectangular, or overlapping subplot indices.";
        return nullptr;
    }

    QImPlotNode* plot = d->m_subplotNode->createPlotNode(subplotIndices);
    if (!plot) {
        qWarning() << "QImFigureWidget::createPlotNode failed to create a 2D plot for the requested subplot indices.";
        return nullptr;
    }
    d->m_usingMatlabLayout = true;
    d->m_matlabLayoutRows = d->m_subplotNode->rows();
    d->m_matlabLayoutCols = d->m_subplotNode->columns();
    d->m_contentNode->setMixedPlotIndices(plot, subplotIndices);
    return plot;
}

QImPlotNode* QImFigureWidget::createPlotNode(std::initializer_list< int > subplotIndices)
{
    return createPlotNode(std::vector< int >(subplotIndices.begin(), subplotIndices.end()));
}

QImPlotNode* QImFigureWidget::subplot(int rows, int cols, int index)
{
    return subplot(rows, cols, std::vector< int > { index });
}

QImPlotNode* QImFigureWidget::subplot(int rows, int cols, const std::vector< int >& indices)
{
    QIM_D(d);
    if (!d->m_subplotNode || rows <= 0 || cols <= 0) {
        return nullptr;
    }

    const bool layoutChanged = !d->m_usingMatlabLayout || d->m_matlabLayoutRows != rows || d->m_matlabLayoutCols != cols;
    if (layoutChanged) {
        const QList< QImPlotNode* > currentPlots = plotNodes();
        for (QImPlotNode* plot : currentPlots) {
            removePlotNode(plot);
        }
        d->m_subplotNode->clearManualPlotLayouts();
        d->m_subplotNode->setGrid(rows, cols);
        if (d->m_subplot3DNode) {
            d->m_subplot3DNode->setGrid(rows, cols);
        }
        d->m_usingMatlabLayout = true;
        d->m_matlabLayoutRows = rows;
        d->m_matlabLayoutCols = cols;
    }
    return createPlotNode(indices);
}

QImPlotNode* QImFigureWidget::subplot(int rows, int cols, std::initializer_list< int > indices)
{
    return subplot(rows, cols, std::vector< int >(indices.begin(), indices.end()));
}

/**
 * @brief 获取所有绘图节点
 * @return
 */
QList< QImPlotNode* > QImFigureWidget::plotNodes() const
{
    return d_ptr->m_subplotNode->plotNodes();
}

int QImFigureWidget::plotCount() const
{
    return d_ptr->m_subplotNode->plotCount();
}

void QImFigureWidget::addPlotNode(QImPlotNode* plot)
{
    d_ptr->m_usingMatlabLayout = false;
    d_ptr->m_subplotNode->addPlotNode(plot);
    if (d_ptr->m_contentNode) {
        d_ptr->m_contentNode->addMixedPlot(plot);
    }
}

void QImFigureWidget::insertPlotNode(int plotIndex, QImPlotNode* plot)
{
    d_ptr->m_usingMatlabLayout = false;
    d_ptr->m_subplotNode->insertPlotNode(plotIndex, plot);
    if (d_ptr->m_contentNode) {
        d_ptr->m_contentNode->insertMixedPlot(plotIndex, plot);
    }
}

int QImFigureWidget::plotNodeSubplotIndex(QImPlotNode* plot)
{
    return d_ptr->m_subplotNode->plotNodeSubplotIndex(plot);
}

bool QImFigureWidget::takePlotNode(QImPlotNode* plot)
{
    if (d_ptr->m_contentNode) {
        d_ptr->m_contentNode->removeMixedPlot(plot);
    }
    return d_ptr->m_subplotNode->takeChildNode(plot);
}

void QImFigureWidget::removePlotNode(QImPlotNode* plot)
{
    if (d_ptr->m_contentNode) {
        d_ptr->m_contentNode->removeMixedPlot(plot);
    }
    d_ptr->m_subplotNode->removeChildNode(plot);
}

QImSubplots3DNode* QImFigureWidget::subplot3DNode() const
{
    return d_ptr->m_subplot3DNode.data();
}

QImPlot3DNode* QImFigureWidget::createPlot3DNode()
{
    if (!d_ptr->m_subplot3DNode || !d_ptr->m_subplotNode || !d_ptr->m_contentNode || !d_ptr->m_contentNode->canAddAutoPlot()) {
        return nullptr;
    }
    QImPlot3DNode* plot = d_ptr->m_subplot3DNode ? d_ptr->m_subplot3DNode->createPlotNode() : nullptr;
    if (plot) {
        if (d_ptr->m_contentNode) {
            d_ptr->m_contentNode->addMixedPlot(plot);
        }
        d_ptr->attachPlot3DNode(plot);
        Q_EMIT plot3DNodeAttached(plot, true);
    }
    return plot;
}

QImPlot3DNode* QImFigureWidget::createPlot3DNode(const std::vector< int >& subplotIndices)
{
    QIM_D(d);
    if (!d->m_subplot3DNode || !d->m_subplotNode || !d->m_contentNode) {
        return nullptr;
    }
    if (QImPlot3DNode* existingPlot = qobject_cast< QImPlot3DNode* >(d->m_contentNode->mixedPlotAtIndices(subplotIndices))) {
        return existingPlot;
    }
    if (!d->m_contentNode->canUseIndices(subplotIndices)) {
        qWarning() << "QImFigureWidget::createPlot3DNode received invalid, non-rectangular, or overlapping subplot indices.";
        return nullptr;
    }

    QImPlot3DNode* plot = d->m_subplot3DNode->createPlotNode();
    if (!plot) {
        qWarning() << "QImFigureWidget::createPlot3DNode failed to create a 3D plot for the requested subplot indices.";
        return nullptr;
    }
    d->m_usingMatlabLayout = true;
    d->m_matlabLayoutRows = d->m_subplotNode->rows();
    d->m_matlabLayoutCols = d->m_subplotNode->columns();
    d->m_contentNode->setMixedPlotIndices(plot, subplotIndices);
    d->attachPlot3DNode(plot);
    Q_EMIT plot3DNodeAttached(plot, true);
    return plot;
}

QImPlot3DNode* QImFigureWidget::createPlot3DNode(std::initializer_list< int > subplotIndices)
{
    return createPlot3DNode(std::vector< int >(subplotIndices.begin(), subplotIndices.end()));
}

QList< QImPlot3DNode* > QImFigureWidget::plot3DNodes() const
{
    return d_ptr->m_subplot3DNode ? d_ptr->m_subplot3DNode->plotNodes() : QList< QImPlot3DNode* >();
}

int QImFigureWidget::plot3DCount() const
{
    return d_ptr->m_subplot3DNode ? d_ptr->m_subplot3DNode->plotCount() : 0;
}

QImAbstractNode* QImFigureWidget::convertCoordinateNode(QImAbstractNode* plot, CoordinateNodeType targetType)
{
    if (!plot) {
        return nullptr;
    }

    if (targetType == Coordinate2D) {
        if (QImPlotNode* plot2D = qobject_cast< QImPlotNode* >(plot)) {
            return plot2D;
        }
        return convertPlot3DNodeTo2D(qobject_cast< QImPlot3DNode* >(plot));
    }

    if (QImPlot3DNode* plot3D = qobject_cast< QImPlot3DNode* >(plot)) {
        return plot3D;
    }
    return convertPlotNodeTo3D(qobject_cast< QImPlotNode* >(plot));
}

QImPlot3DNode* QImFigureWidget::convertPlotNodeTo3D(QImPlotNode* plot)
{
    QIM_D(d);
    if (!plot || !d->m_subplotNode || !d->m_subplot3DNode || !d->m_contentNode) {
        return nullptr;
    }
    if (!plotNodes().contains(plot)) {
        return nullptr;
    }

    const bool wasSelected = selectedCoordinateNodes().contains(plot);
    QImPlot3DNode* newPlot = d->m_subplot3DNode->createPlotNode();
    if (!newPlot) {
        return nullptr;
    }

    d->attachPlot3DNode(newPlot);
    d->m_contentNode->replaceMixedPlot(plot, newPlot);
    d->m_subplotNode->removeChildNode(plot);
    Q_EMIT plot3DNodeAttached(newPlot, true);

    if (wasSelected) {
        d->selectCoordinateNode(newPlot);
    }
    requestRender();
    return newPlot;
}

QImPlotNode* QImFigureWidget::convertPlot3DNodeTo2D(QImPlot3DNode* plot)
{
    QIM_D(d);
    if (!plot || !d->m_subplotNode || !d->m_subplot3DNode || !d->m_contentNode) {
        return nullptr;
    }
    if (!plot3DNodes().contains(plot)) {
        return nullptr;
    }

    const bool wasSelected = selectedCoordinateNodes().contains(plot);
    const int mixedIndex = d->m_contentNode->mixedPlotOrderIndex(plot);
    QImPlotNode* newPlot = new QImPlotNode();
    d->m_subplotNode->insertPlotNode(mixedIndex, newPlot);
    d->m_contentNode->replaceMixedPlot(plot, newPlot);

    d->detachPlot3DNode(plot);
    d->m_subplot3DNode->removePlotNode(plot);
    Q_EMIT plot3DNodeAttached(plot, false);

    if (wasSelected) {
        d->selectCoordinateNode(newPlot);
    }
    requestRender();
    return newPlot;
}

QList< QImAbstractNode* > QImFigureWidget::selectedCoordinateNodes() const
{
    QList< QImAbstractNode* > nodes;
    for (const QPointer< QImAbstractNode >& selected : d_ptr->m_selectedCoordinateNodes) {
        if (selected) {
            nodes.push_back(selected.data());
        }
    }
    return nodes;
}

QList< QImPlotNode* > QImFigureWidget::selectedPlotNodes() const
{
    QList< QImPlotNode* > plots;
    for (QImAbstractNode* selected : selectedCoordinateNodes()) {
        if (QImPlotNode* plot = qobject_cast< QImPlotNode* >(selected)) {
            plots.push_back(plot);
        }
    }
    return plots;
}

QList< QImPlot3DNode* > QImFigureWidget::selectedPlot3DNodes() const
{
    QList< QImPlot3DNode* > plots;
    for (QImAbstractNode* selected : selectedCoordinateNodes()) {
        if (QImPlot3DNode* plot = qobject_cast< QImPlot3DNode* >(selected)) {
            plots.push_back(plot);
        }
    }
    return plots;
}

QImAbstractNode* QImFigureWidget::currentCoordinateNode() const
{
    return d_ptr->m_currentCoordinateNode.data();
}

QImPlotNode* QImFigureWidget::currentPlotNode() const
{
    return qobject_cast< QImPlotNode* >(currentCoordinateNode());
}

QImPlot3DNode* QImFigureWidget::currentPlot3DNode() const
{
    return qobject_cast< QImPlot3DNode* >(currentCoordinateNode());
}

void QImFigureWidget::clearCoordinateSelection()
{
    const QList< QImAbstractNode* > oldSelection = selectedCoordinateNodes();
    QImAbstractNode* oldCurrent = currentCoordinateNode();
    for (QImAbstractNode* selected : oldSelection) {
        if (QImPlotNode* plot = qobject_cast< QImPlotNode* >(selected)) {
            plot->setSelected(false);
        } else if (QImPlot3DNode* plot3D = qobject_cast< QImPlot3DNode* >(selected)) {
            plot3D->setSelected(false);
        }
    }
    d_ptr->m_selectedCoordinateNodes.clear();
    d_ptr->m_currentCoordinateNode = nullptr;
    d_ptr->emitSelectionChangedIfNeeded(oldSelection, oldCurrent);
}

void QImFigureWidget::initializeGL()
{
    QIM_D(d);
    QIM::QImWidget::initializeGL();
    d->m_context = ImPlot::CreateContext();
    d->m_context3D = ImPlot3D::CreateContext();
    // 默认有个subplot
}

void QImFigureWidget::beforeRenderImNodes()
{
    QIM_D(d);
    QIM::QImWidget::beforeRenderImNodes();
    if (!d->m_context) {
        return;
    }
    ImPlot::SetCurrentContext(d->m_context);
    d->m_theme.value().apply(&ImPlot::GetStyle());
    if (d->m_context3D) {
        ImPlot3D::SetCurrentContext(d->m_context3D);
    }
}

void QImFigureWidget::onSubplotChildNodeRemoved(QImAbstractNode* c)
{
    QImPlotNode* plot = qobject_cast< QImPlotNode* >(c);
    if (plot) {
        d_ptr->detachPlotNode(plot);
        Q_EMIT plotNodeAttached(plot, false);
    }
}

void QImFigureWidget::onSubplotChildNodeAdded(QImAbstractNode* c)
{
    QImPlotNode* plot = qobject_cast< QImPlotNode* >(c);
    if (plot) {
        d_ptr->attachPlotNode(plot);
        Q_EMIT plotNodeAttached(plot, true);
    }
}

void QImFigureWidget::onSubplot3DChildNodeRemoved(QImAbstractNode* c)
{
    QImPlot3DNode* plot = qobject_cast< QImPlot3DNode* >(c);
    if (plot) {
        d_ptr->detachPlot3DNode(plot);
        Q_EMIT plot3DNodeAttached(plot, false);
    }
}

void QImFigureWidget::onSubplot3DChildNodeAdded(QImAbstractNode* c)
{
    QImPlot3DNode* plot = qobject_cast< QImPlot3DNode* >(c);
    if (plot) {
        d_ptr->attachPlot3DNode(plot);
        Q_EMIT plot3DNodeAttached(plot, true);
    }
}

void QImFigureWidget::onPlotNodeClicked(QImPlotNode* plot)
{
    d_ptr->selectCoordinateNode(plot);
}

void QImFigureWidget::onPlot3DNodeClicked(QImPlot3DNode* plot)
{
    d_ptr->selectCoordinateNode(plot);
}

}  // end namespace QIM
