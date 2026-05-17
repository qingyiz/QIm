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
        if (!plot || m_mixedPlotOrder.contains(plot)) {
            return;
        }
        m_mixedPlotOrder.push_back(plot);
    }

    void insertMixedPlot(int index, QImAbstractNode* plot)
    {
        if (!plot) {
            return;
        }
        m_mixedPlotOrder.removeOne(plot);
        if (index < 0) {
            index = 0;
        }
        if (index > m_mixedPlotOrder.size()) {
            index = m_mixedPlotOrder.size();
        }
        m_mixedPlotOrder.insert(index, plot);
    }

    void removeMixedPlot(QImAbstractNode* plot)
    {
        m_mixedPlotOrder.removeOne(plot);
    }

protected:
    bool beginDraw() override
    {
        const bool has2D = m_subplot2D && m_subplot2D->plotCount() > 0;
        const bool has3D = m_subplot3D && m_subplot3D->plotCount() > 0;
        if (has2D && has3D) {
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

    void renderMixedGrid()
    {
        if (!m_subplot2D || !m_subplot3D) {
            return;
        }

        const int rows = std::max(1, m_subplot2D->rows());
        const int cols = std::max(1, m_subplot2D->columns());
        const int gridCount = rows * cols;

        m_mixedPlotOrder.erase(
            std::remove_if(m_mixedPlotOrder.begin(), m_mixedPlotOrder.end(), [](const QPointer< QImAbstractNode >& plot) {
                return plot.isNull();
            }),
            m_mixedPlotOrder.end());

        std::vector< float > rowRatios = m_subplot2D->rowRatios();
        std::vector< float > columnRatios = m_subplot2D->columnRatios();
        float* rowRatiosData = static_cast< int >(rowRatios.size()) == rows ? rowRatios.data() : nullptr;
        float* columnRatiosData = static_cast< int >(columnRatios.size()) == cols ? columnRatios.data() : nullptr;

        if (!ImPlot::BeginSubplots("##FigureMixedSubplots",
                                   rows,
                                   cols,
                                   ImGui::GetContentRegionAvail(),
                                   subplotFlags(),
                                   rowRatiosData,
                                   columnRatiosData)) {
            return;
        }

        const int renderCount = std::min(gridCount, m_mixedPlotOrder.size());
        for (int index = 0; index < renderCount; ++index) {
            QImAbstractNode* node = m_mixedPlotOrder.at(index);
            if (QImPlotNode* plot = qobject_cast< QImPlotNode* >(node)) {
                plot->render();
                continue;
            }

            QImPlot3DNode* plot3D = qobject_cast< QImPlot3DNode* >(node);
            if (!plot3D) {
                advanceEmptyCell();
                continue;
            }

            renderPlot3DCell(plot3D);
        }

        ImPlot::EndSubplots();
    }

    void advanceEmptyCell()
    {
        static constexpr ImPlotFlags placeholderFlags =
            ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame | ImPlotFlags_NoInputs;
        if (ImPlot::BeginPlot("##FigureMixedEmptyCell", ImVec2(-1.0f, -1.0f), placeholderFlags)) {
            ImPlot::SetupAxes(nullptr,
                              nullptr,
                              ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoMenus,
                              ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoMenus);
            ImPlot::EndPlot();
        }
    }

    void renderPlot3DCell(QImPlot3DNode* plot3D)
    {
        if (!plot3D) {
            advanceEmptyCell();
            return;
        }

        static constexpr ImPlotFlags placeholderFlags =
            ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame | ImPlotFlags_NoInputs;
        if (!ImPlot::BeginPlot("##FigureMixed3DCell", ImVec2(-1.0f, -1.0f), placeholderFlags)) {
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
        ImGui::PushID(plot3D);
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
    QList< QPointer< QImAbstractNode > > m_mixedPlotOrder;
};

class QImFigureWidget::PrivateData
{
    QIM_DECLARE_PUBLIC(QImFigureWidget)
public:
    PrivateData(QImFigureWidget* p);

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
};

QImFigureWidget::PrivateData::PrivateData(QImFigureWidget* p) : q_ptr(p)
{
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
        d->m_subplot3DNode->setGrid(rows, cols);
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
    if (plotCount() + plot3DCount() >= d_ptr->m_subplotNode->gridCount()) {
        return nullptr;
    }
    d_ptr->m_usingMatlabLayout = false;
    d_ptr->m_subplotNode->clearManualPlotLayouts();
    QImPlotNode* plot = d_ptr->m_subplotNode->createPlotNode();
    if (plot && d_ptr->m_contentNode) {
        d_ptr->m_contentNode->addMixedPlot(plot);
    }
    return plot;
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
    if (plot3DCount() > 0) {
        qWarning() << "QImFigureWidget::subplot does not support MATLAB-style 2D manual layout while 3D plots exist. "
                      "Use setSubplotGrid + createPlotNode/createPlot3DNode for mixed 2D/3D figures.";
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
        d->m_usingMatlabLayout = true;
        d->m_matlabLayoutRows = rows;
        d->m_matlabLayoutCols = cols;
    }
    QImPlotNode* plot = d->m_subplotNode->createPlotNode(indices);
    if (plot && d->m_contentNode) {
        d->m_contentNode->addMixedPlot(plot);
    }
    return plot;
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
    d_ptr->m_subplotNode->clearManualPlotLayouts();
    d_ptr->m_subplotNode->addPlotNode(plot);
    if (d_ptr->m_contentNode) {
        d_ptr->m_contentNode->addMixedPlot(plot);
    }
}

void QImFigureWidget::insertPlotNode(int plotIndex, QImPlotNode* plot)
{
    d_ptr->m_usingMatlabLayout = false;
    d_ptr->m_subplotNode->clearManualPlotLayouts();
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
    if (!d_ptr->m_subplot3DNode || !d_ptr->m_subplotNode || plotCount() + plot3DCount() >= d_ptr->m_subplotNode->gridCount()) {
        return nullptr;
    }
    if (d_ptr->m_subplotNode->hasManualPlotLayouts()) {
        qWarning() << "QImFigureWidget::createPlot3DNode does not support existing MATLAB-style 2D manual layouts. "
                      "Use setSubplotGrid + createPlotNode/createPlot3DNode for mixed 2D/3D figures.";
        return nullptr;
    }
    QImPlot3DNode* plot = d_ptr->m_subplot3DNode ? d_ptr->m_subplot3DNode->createPlotNode() : nullptr;
    if (plot) {
        if (d_ptr->m_contentNode) {
            d_ptr->m_contentNode->addMixedPlot(plot);
        }
        Q_EMIT plot3DNodeAttached(plot, true);
    }
    return plot;
}

QList< QImPlot3DNode* > QImFigureWidget::plot3DNodes() const
{
    return d_ptr->m_subplot3DNode ? d_ptr->m_subplot3DNode->plotNodes() : QList< QImPlot3DNode* >();
}

int QImFigureWidget::plot3DCount() const
{
    return d_ptr->m_subplot3DNode ? d_ptr->m_subplot3DNode->plotCount() : 0;
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
        Q_EMIT plotNodeAttached(plot, false);
    }
}

void QImFigureWidget::onSubplotChildNodeAdded(QImAbstractNode* c)
{
    QImPlotNode* plot = qobject_cast< QImPlotNode* >(c);
    if (plot) {
        Q_EMIT plotNodeAttached(plot, true);
    }
}

void QImFigureWidget::onSubplot3DChildNodeRemoved(QImAbstractNode* c)
{
    QImPlot3DNode* plot = qobject_cast< QImPlot3DNode* >(c);
    if (plot) {
        Q_EMIT plot3DNodeAttached(plot, false);
    }
}

void QImFigureWidget::onSubplot3DChildNodeAdded(QImAbstractNode* c)
{
    QImPlot3DNode* plot = qobject_cast< QImPlot3DNode* >(c);
    if (plot) {
        Q_EMIT plot3DNodeAttached(plot, true);
    }
}

}  // end namespace QIM
