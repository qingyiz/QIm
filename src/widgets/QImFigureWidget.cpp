#include "QImFigureWidget.h"
#include <QDebug>
#include "QImTrackedValue.hpp"
#include "implot.h"
#include "plot/QImSubplotsNode.h"
#include "plot/QImPlotNode.h"
namespace QIM
{
class QImFigureWidget::PrivateData
{
    QIM_DECLARE_PUBLIC(QImFigureWidget)
public:
    PrivateData(QImFigureWidget* p);

public:
    QImTrackedValue< QImPlotTheme > m_theme;
    QPointer< QImSubplotsNode > m_subplotNode;
    ImPlotContext* m_context { nullptr };
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
    addRenderNode(d_ptr->m_subplotNode.data());
    d_ptr->m_subplotNode->setTitleEnabled(true);
    connect(d_ptr->m_subplotNode, &QImSubplotsNode::childNodeAdded, this, &QImFigureWidget::onSubplotChildNodeAdded);
    connect(d_ptr->m_subplotNode, &QImSubplotsNode::childNodeRemoved, this, &QImFigureWidget::onSubplotChildNodeRemoved);
}

QImFigureWidget::~QImFigureWidget()
{
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
    d_ptr->m_usingMatlabLayout = false;
    d_ptr->m_subplotNode->clearManualPlotLayouts();
    return d_ptr->m_subplotNode->createPlotNode();
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
        d->m_usingMatlabLayout = true;
        d->m_matlabLayoutRows = rows;
        d->m_matlabLayoutCols = cols;
    }
    return d->m_subplotNode->createPlotNode(indices);
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
    return d_ptr->m_subplotNode->addPlotNode(plot);
}

void QImFigureWidget::insertPlotNode(int plotIndex, QImPlotNode* plot)
{
    d_ptr->m_usingMatlabLayout = false;
    d_ptr->m_subplotNode->clearManualPlotLayouts();
    return d_ptr->m_subplotNode->insertPlotNode(plotIndex, plot);
}

int QImFigureWidget::plotNodeSubplotIndex(QImPlotNode* plot)
{
    return d_ptr->m_subplotNode->plotNodeSubplotIndex(plot);
}

bool QImFigureWidget::takePlotNode(QImPlotNode* plot)
{
    return d_ptr->m_subplotNode->takeChildNode(plot);
}

void QImFigureWidget::removePlotNode(QImPlotNode* plot)
{
    d_ptr->m_subplotNode->removeChildNode(plot);
}

void QImFigureWidget::initializeGL()
{
    QIM_D(d);
    QIM::QImWidget::initializeGL();
    d->m_context = ImPlot::CreateContext();
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

}  // end namespace QIM
