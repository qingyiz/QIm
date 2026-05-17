#ifndef QIMFIGUREWIDGET_H
#define QIMFIGUREWIDGET_H
#include "QImWidget.h"
#include <initializer_list>
#include <QSizeF>
#include <QWidget>
#include "QImPlotTheme.h"
#include <vector>
namespace QIM
{
class QImSubplotsNode;
class QImPlotNode;
class QImSubplots3DNode;
class QImPlot3DNode;
/**
 * @brief Figure Widget for plot
 *
 * QImFigureWidget内部会创建二维QImSubplotsNode和三维QImSubplots3DNode
 *
 * 绘图的节点会作为对应subplot节点的子节点，默认subplot会有1行1列的布局。
 * 当二维和三维图同时存在时，它们会在同一个QImFigureWidget窗口内按统一subplot网格混合显示。
 *
 * 你也可以直接调用@ref addRenderNode 把节点挂在顶层窗口下面，这样你可以创建任意渲染节点在subplot上面
 */
class QIM_WIDGETS_API QImFigureWidget : public QImWidget
{
    Q_OBJECT
    QIM_DECLARE_PRIVATE(QImFigureWidget)
public:
    QImFigureWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~QImFigureWidget();
    // ===========================
    //  theme
    // ===========================
    // 获取绘图的样式
    void setPlotTheme(const QImPlotTheme& t);
    const QImPlotTheme& plotTheme() const;
    // ===========================
    //  subplot
    // ===========================
    void setSubplotGrid(
        int rows,
        int cols,
        const std::vector< float >& rowsRatios = std::vector< float >(),
        const std::vector< float >& colsRatios = std::vector< float >()
    );
    int subplotGridRows() const;
    int subplotGridColumns() const;
    std::vector< float > subplotGridRowRatios() const;
    std::vector< float > subplotGridColumnRatios() const;
    QImSubplotsNode* subplotNode() const;
    // 设置单个 subplot 的最小可视尺寸；网格总尺寸超出控件时自动出现滚动条
    QSizeF minimumPlotSize() const;
    void setMinimumPlotSize(const QSizeF& size);
    // ===========================
    //  plot
    // ===========================
    // 创建一个绘图，这个绘图会作为subplot的子节点，如果当前subplot的绘图已经和subplot的管理数量一致，此函数返回nullptr
    QImPlotNode* createPlotNode();
    // 创建一个占用指定 subplot 网格的 2D 图；索引从1开始，indices必须形成矩形区域
    QImPlotNode* createPlotNode(const std::vector< int >& subplotIndices);
    QImPlotNode* createPlotNode(std::initializer_list< int > subplotIndices);
    // MATLAB风格subplot接口：索引从1开始，支持跨格矩形区域
    QImPlotNode* subplot(int rows, int cols, int index);
    QImPlotNode* subplot(int rows, int cols, const std::vector< int >& indices);
    QImPlotNode* subplot(int rows, int cols, std::initializer_list< int > indices);
    // 获取所有绘图节点
    QList< QImPlotNode* > plotNodes() const;
    // 绘图的数量
    int plotCount() const;
    void addPlotNode(QImPlotNode* plot);
    // 插入绘图，注意plotIndex是subplot节点下面绘图节点的索引，其它节点会跳过,plotIndex可以是-1，则代表在最前面插入，可以大于等于size，代表最后插入
    void insertPlotNode(int plotIndex, QImPlotNode* plot);
    // plotNode在subplot下的索引
    int plotNodeSubplotIndex(QImPlotNode* plot);
    // 提取出QImPlotNode，不在此figure里管理
    bool takePlotNode(QImPlotNode* plot);
    // 移除绘图，plot会被删除
    void removePlotNode(QImPlotNode* plot);
    // ===========================
    //  3D subplot / plot
    // ===========================
    QImSubplots3DNode* subplot3DNode() const;
    QImPlot3DNode* createPlot3DNode();
    // 创建一个占用指定 subplot 网格的 3D 图；索引从1开始，indices必须形成矩形区域
    QImPlot3DNode* createPlot3DNode(const std::vector< int >& subplotIndices);
    QImPlot3DNode* createPlot3DNode(std::initializer_list< int > subplotIndices);
    QList< QImPlot3DNode* > plot3DNodes() const;
    int plot3DCount() const;
Q_SIGNALS:
    /**
     * @brief QImPlotNode的添加或删除的信号，此信号等同绑定subplotNode的childNodeAdded/childNodeRemoved
     * @param plot
     * @param attach
     */
    void plotNodeAttached(QIM::QImPlotNode* plot, bool attach);
    void plot3DNodeAttached(QIM::QImPlot3DNode* plot, bool attach);

protected:
    void initializeGL() override;
    void beforeRenderImNodes() override;
private Q_SLOTS:
    void onSubplotChildNodeRemoved(QIM::QImAbstractNode* c);
    void onSubplotChildNodeAdded(QIM::QImAbstractNode* c);
    void onSubplot3DChildNodeRemoved(QIM::QImAbstractNode* c);
    void onSubplot3DChildNodeAdded(QIM::QImAbstractNode* c);
};
}
#endif  // QIMFIGUREWIDGET_H
