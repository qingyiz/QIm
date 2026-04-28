#ifndef QIMPLOTLINEITEMNODE_H
#define QIMPLOTLINEITEMNODE_H
#include <type_traits>
#include <vector>
#include "QImPlotItemNode.h"
#include "QImPlotDataSeries.h"
#include "QImPlotHighlightRule.h"

namespace QIM
{
class QImAbstractXYDataSeries;

class QIM_CORE_API QImPlotLineItemNode : public QImPlotItemNode
{
    Q_OBJECT
    QIM_DECLARE_PRIVATE(QImPlotLineItemNode)

    // 线条标志属性（全部采用肯定语义）
    Q_PROPERTY(bool segments READ isSegments WRITE setSegments NOTIFY lineFlagChanged)
    Q_PROPERTY(bool loop READ isLoop WRITE setLoop NOTIFY lineFlagChanged)
    Q_PROPERTY(bool skipNaN READ isSkipNaN WRITE setSkipNaN NOTIFY lineFlagChanged)
    Q_PROPERTY(bool clippingEnabled READ isClippingEnabled WRITE setClippingEnabled NOTIFY lineFlagChanged)
    Q_PROPERTY(bool shaded READ isShaded WRITE setShaded NOTIFY lineFlagChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int lineStyle READ lineStyle WRITE setLineStyle NOTIFY lineStyleChanged)
    Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(int markerShape READ markerShape WRITE setMarkerShape NOTIFY markerShapeChanged)
    Q_PROPERTY(float markerSize READ markerSize WRITE setMarkerSize NOTIFY markerSizeChanged)
    Q_PROPERTY(float markerWeight READ markerWeight WRITE setMarkerWeight NOTIFY markerWeightChanged)
    Q_PROPERTY(QColor markerFillColor READ markerFillColor WRITE setMarkerFillColor NOTIFY markerFillColorChanged)
    Q_PROPERTY(
        QColor markerOutlineColor READ markerOutlineColor WRITE setMarkerOutlineColor NOTIFY markerOutlineColorChanged)
    Q_PROPERTY(
        bool directionArrowsVisible READ isDirectionArrowsVisible WRITE setDirectionArrowsVisible NOTIFY directionArrowsVisibilityChanged)
    Q_PROPERTY(bool adaptiveSampling READ isAdaptiveSampling WRITE setAdaptiveSampling NOTIFY adaptiveSamplingChanged)
    Q_PROPERTY(int downsampleThreshold READ downsampleThreshold WRITE setDownsampleThreshold NOTIFY downsampleThresholdChanged)
public:
    QImPlotLineItemNode(QObject* par = nullptr);
    ~QImPlotLineItemNode();
    enum
    {
        Type = InnerType + 1
    };
    virtual int type() const override
    {
        return Type;
    }
    //----------------------------------------------------
    // 数据设置
    //----------------------------------------------------
    void setData(QImAbstractXYDataSeries* series);
    template< typename ContainerX, typename ContainerY >
    QImAbstractXYDataSeries* setData(const ContainerX& x, const ContainerY& y);
    template< typename ContainerX, typename ContainerY >
    QImAbstractXYDataSeries* setData(ContainerX&& x, ContainerY&& y);
    void setZData(const std::vector< double >& z);
    void setZData(std::vector< double >&& z);
    template< typename ContainerZ >
    void setZData(const ContainerZ& z);
    template< typename ContainerZ >
    void setZData(ContainerZ&& z);
    void clearZData();
    // 获取数据
    QImAbstractXYDataSeries* data() const;
    bool hasZData() const;
    bool hasOrderedZData() const;
    //----------------------------------------------------
    // ImPlotLineFlags
    //----------------------------------------------------
    // 标志访问器（肯定语义）
    bool isSegments() const;
    void setSegments(bool on);

    bool isLoop() const;
    void setLoop(bool on);

    bool isSkipNaN() const;
    void setSkipNaN(bool on);

    bool isClippingEnabled() const;  // 对应 !NoClip
    void setClippingEnabled(bool enabled);

    bool isShaded() const;
    void setShaded(bool on);

    // 原始标志访问
    int lineFlags() const;
    void setLineFlags(int flags);
    //

    //===============================================================
    // style
    //===============================================================
    void setColor(const QColor& c);
    QColor color() const;
    void setLineStyle(int style);
    int lineStyle() const;
    void setLineWidth(float width);
    float lineWidth() const;
    void setMarkerShape(int shape);
    int markerShape() const;
    void setMarkerSize(float size);
    float markerSize() const;
    void setMarkerWeight(float weight);
    float markerWeight() const;
    void setMarkerFillColor(const QColor& color);
    QColor markerFillColor() const;
    void setMarkerOutlineColor(const QColor& color);
    QColor markerOutlineColor() const;
    void setDirectionArrowsVisible(bool on);
    bool isDirectionArrowsVisible() const;
    void setHighlightRule(const QImPlotHighlightRule& rule);
    QImPlotHighlightRule highlightRule() const;
    void clearHighlightRule();
    bool hasHighlightRule() const;
    //===============================================================
    // name
    //===============================================================
    void setAdaptivesSampling(bool on);
    void setAdaptiveSampling(bool on);
    bool isAdaptiveSampling() const;
    int downsampleThreshold() const;
    void setDownsampleThreshold(int threshold);
Q_SIGNALS:
    void lineFlagChanged();
    void colorChanged(const QColor& color);
    void lineStyleChanged(int style);
    void lineWidthChanged(float width);
    void markerShapeChanged(int shape);
    void markerSizeChanged(float size);
    void markerWeightChanged(float weight);
    void markerFillColorChanged(const QColor& color);
    void markerOutlineColorChanged(const QColor& color);
    void zDataChanged();
    void directionArrowsVisibilityChanged(bool visible);
    void highlightRuleChanged();
    void adaptiveSamplingChanged(bool enabled);
    void downsampleThresholdChanged(int threshold);

protected:
    virtual bool beginDraw() override;
};

template< typename ContainerX, typename ContainerY >
inline QImAbstractXYDataSeries* QImPlotLineItemNode::setData(const ContainerX& x, const ContainerY& y)
{
    QImAbstractXYDataSeries* d = new QImVectorXYDataSeries(x, y);
    setData(d);
    return d;
}
template< typename ContainerX, typename ContainerY >
QImAbstractXYDataSeries* QImPlotLineItemNode::setData(ContainerX&& x, ContainerY&& y)
{
    QImAbstractXYDataSeries* d = new QImVectorXYDataSeries(x, y);
    setData(d);
    return d;
}

template< typename ContainerZ >
inline void QImPlotLineItemNode::setZData(const ContainerZ& z)
{
    static_assert(std::is_convertible_v< typename ContainerZ::value_type, double >, "ContainerZ must store numeric values");
    setZData(std::vector< double >(z.begin(), z.end()));
}

template< typename ContainerZ >
inline void QImPlotLineItemNode::setZData(ContainerZ&& z)
{
    static_assert(std::is_convertible_v< typename ContainerZ::value_type, double >, "ContainerZ must store numeric values");
    setZData(std::vector< double >(z.begin(), z.end()));
}

}  // end namespace QIM
#endif  // QIMPLOTLINEITEMNODE_H
