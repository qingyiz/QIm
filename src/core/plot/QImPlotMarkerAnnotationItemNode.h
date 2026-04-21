#ifndef QIMPLOTMARKERANNOTATIONITEMNODE_H
#define QIMPLOTMARKERANNOTATIONITEMNODE_H

#include <QColor>
#include <QPointF>
#include <QStringList>
#include <QVector>
#include "QImPlotDataSeries.h"
#include "QImPlotItemNode.h"

namespace QIM
{
class QImAbstractXYDataSeries;

class QIM_CORE_API QImPlotMarkerAnnotationItemNode : public QImPlotItemNode
{
    Q_OBJECT
    QIM_DECLARE_PRIVATE(QImPlotMarkerAnnotationItemNode)

    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int markerShape READ markerShape WRITE setMarkerShape NOTIFY markerShapeChanged)
    Q_PROPERTY(float markerSize READ markerSize WRITE setMarkerSize NOTIFY markerSizeChanged)
    Q_PROPERTY(float markerWeight READ markerWeight WRITE setMarkerWeight NOTIFY markerWeightChanged)
    Q_PROPERTY(bool markerFilled READ isMarkerFilled WRITE setMarkerFilled NOTIFY markerFilledChanged)
    Q_PROPERTY(QColor markerFillColor READ markerFillColor WRITE setMarkerFillColor NOTIFY markerFillColorChanged)
    Q_PROPERTY(
        QColor markerOutlineColor READ markerOutlineColor WRITE setMarkerOutlineColor NOTIFY markerOutlineColorChanged)
    Q_PROPERTY(QStringList texts READ texts WRITE setTexts NOTIFY textsChanged)
    Q_PROPERTY(bool textVisible READ isTextVisible WRITE setTextVisible NOTIFY textVisibilityChanged)
    Q_PROPERTY(QPointF textPixelOffset READ textPixelOffset WRITE setTextPixelOffset NOTIFY textGeometryChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)

public:
    enum
    {
        Type = InnerType + 9
    };

    explicit QImPlotMarkerAnnotationItemNode(QObject* parent = nullptr);
    ~QImPlotMarkerAnnotationItemNode() override;

    virtual int type() const override
    {
        return Type;
    }

    void setData(QImAbstractXYDataSeries* series);

    template< typename ContainerX, typename ContainerY >
    QImAbstractXYDataSeries* setData(const ContainerX& x, const ContainerY& y);

    template< typename ContainerX, typename ContainerY >
    QImAbstractXYDataSeries* setData(ContainerX&& x, ContainerY&& y);

    void setPoints(const QVector< QPointF >& points);

    template< typename Container >
    void setPoints(const Container& points);

    QImAbstractXYDataSeries* data() const;

    void setColor(const QColor& color);
    QColor color() const;

    void setMarkerShape(int shape);
    int markerShape() const;

    void setMarkerSize(float size);
    float markerSize() const;

    void setMarkerWeight(float weight);
    float markerWeight() const;

    void setMarkerFilled(bool filled);
    bool isMarkerFilled() const;

    void setMarkerFillColor(const QColor& color);
    QColor markerFillColor() const;

    void setMarkerOutlineColor(const QColor& color);
    QColor markerOutlineColor() const;

    void setTexts(const QStringList& texts);
    QStringList texts() const;

    void setTextVisible(bool visible);
    bool isTextVisible() const;

    void setTextPixelOffset(const QPointF& offset);
    QPointF textPixelOffset() const;

    void setTextColor(const QColor& color);
    QColor textColor() const;

Q_SIGNALS:
    void dataChanged();
    void colorChanged(const QColor& color);
    void markerShapeChanged(int shape);
    void markerSizeChanged(float size);
    void markerWeightChanged(float weight);
    void markerFilledChanged(bool filled);
    void markerFillColorChanged(const QColor& color);
    void markerOutlineColorChanged(const QColor& color);
    void textsChanged(const QStringList& texts);
    void textVisibilityChanged(bool visible);
    void textGeometryChanged();
    void textColorChanged(const QColor& color);

protected:
    virtual bool beginDraw() override;
};

template< typename ContainerX, typename ContainerY >
inline QImAbstractXYDataSeries* QImPlotMarkerAnnotationItemNode::setData(const ContainerX& x, const ContainerY& y)
{
    QImAbstractXYDataSeries* d = new QImVectorXYDataSeries(x, y);
    setData(d);
    return d;
}

template< typename ContainerX, typename ContainerY >
inline QImAbstractXYDataSeries* QImPlotMarkerAnnotationItemNode::setData(ContainerX&& x, ContainerY&& y)
{
    QImAbstractXYDataSeries* d = new QImVectorXYDataSeries(x, y);
    setData(d);
    return d;
}

template< typename Container >
inline void QImPlotMarkerAnnotationItemNode::setPoints(const Container& points)
{
    std::vector< double > xs;
    std::vector< double > ys;
    xs.reserve(static_cast< std::size_t >(points.size()));
    ys.reserve(static_cast< std::size_t >(points.size()));
    for (const auto& point : points) {
        xs.push_back(static_cast< double >(point.x()));
        ys.push_back(static_cast< double >(point.y()));
    }
    setData(std::move(xs), std::move(ys));
}

}  // namespace QIM

#endif  // QIMPLOTMARKERANNOTATIONITEMNODE_H
