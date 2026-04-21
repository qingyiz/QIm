#include "QImPlotMarkerAnnotationItemNode.h"

#include <algorithm>
#include <optional>
#include "QImTrackedValue.hpp"
#include "QtImGuiUtils.h"
#include "implot.h"
#include "implot_internal.h"

namespace QIM
{

class QImPlotMarkerAnnotationItemNode::PrivateData
{
    QIM_DECLARE_PUBLIC(QImPlotMarkerAnnotationItemNode)
public:
    explicit PrivateData(QImPlotMarkerAnnotationItemNode* q) : q_ptr(q)
    {
    }

    std::unique_ptr< QImAbstractXYDataSeries > data;
    ImPlotMarker markerShape { ImPlotMarker_Cross };
    QImTrackedValue< float > markerSize { 8.0f };
    QImTrackedValue< float > markerWeight { 2.0f };
    bool markerFilled { true };
    std::optional< QImTrackedValue< ImVec4, ImVecComparator< ImVec4 > > > color;
    std::optional< QImTrackedValue< ImVec4, ImVecComparator< ImVec4 > > > markerFillColor;
    std::optional< QImTrackedValue< ImVec4, ImVecComparator< ImVec4 > > > markerOutlineColor;
    QStringList texts;
    bool textVisible { false };
    QPointF textPixelOffset { 8.0, -10.0 };
    std::optional< QImTrackedValue< ImVec4, ImVecComparator< ImVec4 > > > textColor;
};

QImPlotMarkerAnnotationItemNode::QImPlotMarkerAnnotationItemNode(QObject* parent)
    : QImPlotItemNode(parent), QIM_PIMPL_CONSTRUCT
{
}

QImPlotMarkerAnnotationItemNode::~QImPlotMarkerAnnotationItemNode()
{
}

void QImPlotMarkerAnnotationItemNode::setData(QImAbstractXYDataSeries* series)
{
    QIM_D(d);
    d->data.reset(series);
    Q_EMIT dataChanged();
}

void QImPlotMarkerAnnotationItemNode::setPoints(const QVector< QPointF >& points)
{
    setPoints< QVector< QPointF > >(points);
}

QImAbstractXYDataSeries* QImPlotMarkerAnnotationItemNode::data() const
{
    return d_ptr->data.get();
}

void QImPlotMarkerAnnotationItemNode::setColor(const QColor& color)
{
    const ImVec4 imColor = toImVec4(color);
    const bool changed   = !d_ptr->color || !ImVecComparator< ImVec4 > {}(d_ptr->color->value(), imColor);
    if (d_ptr->color) {
        d_ptr->color->value() = imColor;
        if (changed) {
            d_ptr->color->mark_dirty();
        }
    } else {
        d_ptr->color.emplace(imColor);
        d_ptr->color->mark_dirty();
    }
    if (changed) {
        Q_EMIT colorChanged(color);
    }
}

QColor QImPlotMarkerAnnotationItemNode::color() const
{
    return d_ptr->color ? toQColor(d_ptr->color->value()) : QColor();
}

void QImPlotMarkerAnnotationItemNode::setMarkerShape(int shape)
{
    QIM_D(d);
    const ImPlotMarker marker = static_cast< ImPlotMarker >(shape);
    if (d->markerShape != marker) {
        d->markerShape = marker;
        Q_EMIT markerShapeChanged(shape);
    }
}

int QImPlotMarkerAnnotationItemNode::markerShape() const
{
    QIM_DC(d);
    return d->markerShape;
}

void QImPlotMarkerAnnotationItemNode::setMarkerSize(float size)
{
    QIM_D(d);
    d->markerSize = size;
    if (d->markerSize.is_dirty()) {
        Q_EMIT markerSizeChanged(size);
    }
}

float QImPlotMarkerAnnotationItemNode::markerSize() const
{
    QIM_DC(d);
    return d->markerSize.value();
}

void QImPlotMarkerAnnotationItemNode::setMarkerWeight(float weight)
{
    QIM_D(d);
    d->markerWeight = weight;
    if (d->markerWeight.is_dirty()) {
        Q_EMIT markerWeightChanged(weight);
    }
}

float QImPlotMarkerAnnotationItemNode::markerWeight() const
{
    QIM_DC(d);
    return d->markerWeight.value();
}

void QImPlotMarkerAnnotationItemNode::setMarkerFilled(bool filled)
{
    QIM_D(d);
    if (d->markerFilled != filled) {
        d->markerFilled = filled;
        Q_EMIT markerFilledChanged(filled);
    }
}

bool QImPlotMarkerAnnotationItemNode::isMarkerFilled() const
{
    QIM_DC(d);
    return d->markerFilled;
}

void QImPlotMarkerAnnotationItemNode::setMarkerFillColor(const QColor& color)
{
    const ImVec4 imColor = toImVec4(color);
    const bool changed =
        !d_ptr->markerFillColor || !ImVecComparator< ImVec4 > {}(d_ptr->markerFillColor->value(), imColor);
    if (d_ptr->markerFillColor) {
        d_ptr->markerFillColor->value() = imColor;
        if (changed) {
            d_ptr->markerFillColor->mark_dirty();
        }
    } else {
        d_ptr->markerFillColor.emplace(imColor);
        d_ptr->markerFillColor->mark_dirty();
    }
    if (changed) {
        Q_EMIT markerFillColorChanged(color);
    }
}

QColor QImPlotMarkerAnnotationItemNode::markerFillColor() const
{
    if (d_ptr->markerFillColor) {
        return toQColor(d_ptr->markerFillColor->value());
    }
    return color();
}

void QImPlotMarkerAnnotationItemNode::setMarkerOutlineColor(const QColor& color)
{
    const ImVec4 imColor = toImVec4(color);
    const bool changed =
        !d_ptr->markerOutlineColor || !ImVecComparator< ImVec4 > {}(d_ptr->markerOutlineColor->value(), imColor);
    if (d_ptr->markerOutlineColor) {
        d_ptr->markerOutlineColor->value() = imColor;
        if (changed) {
            d_ptr->markerOutlineColor->mark_dirty();
        }
    } else {
        d_ptr->markerOutlineColor.emplace(imColor);
        d_ptr->markerOutlineColor->mark_dirty();
    }
    if (changed) {
        Q_EMIT markerOutlineColorChanged(color);
    }
}

QColor QImPlotMarkerAnnotationItemNode::markerOutlineColor() const
{
    if (d_ptr->markerOutlineColor) {
        return toQColor(d_ptr->markerOutlineColor->value());
    }
    return color();
}

void QImPlotMarkerAnnotationItemNode::setTexts(const QStringList& texts)
{
    QIM_D(d);
    if (d->texts != texts) {
        d->texts = texts;
        Q_EMIT textsChanged(texts);
    }
}

QStringList QImPlotMarkerAnnotationItemNode::texts() const
{
    return d_ptr->texts;
}

void QImPlotMarkerAnnotationItemNode::setTextVisible(bool visible)
{
    QIM_D(d);
    if (d->textVisible != visible) {
        d->textVisible = visible;
        Q_EMIT textVisibilityChanged(visible);
    }
}

bool QImPlotMarkerAnnotationItemNode::isTextVisible() const
{
    QIM_DC(d);
    return d->textVisible;
}

void QImPlotMarkerAnnotationItemNode::setTextPixelOffset(const QPointF& offset)
{
    QIM_D(d);
    if (d->textPixelOffset != offset) {
        d->textPixelOffset = offset;
        Q_EMIT textGeometryChanged();
    }
}

QPointF QImPlotMarkerAnnotationItemNode::textPixelOffset() const
{
    return d_ptr->textPixelOffset;
}

void QImPlotMarkerAnnotationItemNode::setTextColor(const QColor& color)
{
    const ImVec4 imColor = toImVec4(color);
    const bool changed = !d_ptr->textColor || !ImVecComparator< ImVec4 > {}(d_ptr->textColor->value(), imColor);
    if (d_ptr->textColor) {
        d_ptr->textColor->value() = imColor;
    } else {
        d_ptr->textColor.emplace(imColor);
    }
    if (changed) {
        Q_EMIT textColorChanged(color);
    }
}

QColor QImPlotMarkerAnnotationItemNode::textColor() const
{
    if (d_ptr->textColor) {
        return toQColor(d_ptr->textColor->value());
    }
    return color();
}

bool QImPlotMarkerAnnotationItemNode::beginDraw()
{
    QIM_D(d);
    if (!d->data) {
        return false;
    }

    QImAbstractXYDataSeries* series = d->data.get();
    if (!series || series->size() <= 0) {
        return false;
    }

    const ImVec4 primaryColor = d->color ? d->color->value() : IMPLOT_AUTO_COL;
    const ImVec4 fillColor =
        d->markerFillColor ? d->markerFillColor->value() : (d->markerFilled ? primaryColor : ImVec4(0, 0, 0, 0));
    const ImVec4 outlineColor = d->markerOutlineColor ? d->markerOutlineColor->value() : primaryColor;

    if (d->markerShape != ImPlotMarker_Circle || d->markerSize.is_dirty() || d->markerSize.value() != 4.0f ||
        d->markerWeight.is_dirty() || d->markerWeight.value() != 1.0f || d->color || d->markerFillColor ||
        d->markerOutlineColor || !d->markerFilled) {
        ImPlot::SetNextMarkerStyle(
            d->markerShape, d->markerSize.value(), fillColor, d->markerWeight.value(), outlineColor);
        d->markerSize.mark_clean();
        d->markerWeight.mark_clean();
        if (d->color && d->color->is_dirty()) {
            d->color->mark_clean();
        }
        if (d->markerFillColor && d->markerFillColor->is_dirty()) {
            d->markerFillColor->mark_clean();
        }
        if (d->markerOutlineColor && d->markerOutlineColor->is_dirty()) {
            d->markerOutlineColor->mark_clean();
        }
    }

    if (series->isContiguous()) {
        if (series->xRawData()) {
            ImPlot::PlotScatter(labelConstData(),
                                series->xRawData(),
                                series->yRawData(),
                                series->size(),
                                0,
                                series->offset(),
                                series->stride());
        } else {
            ImPlot::PlotScatter(labelConstData(),
                                series->yRawData(),
                                series->size(),
                                series->xScale(),
                                series->xStart(),
                                0,
                                series->offset(),
                                series->stride());
        }
    }

    ImPlotContext* ct    = ImPlot::GetCurrentContext();
    ImPlotItem* plotItem = ct->PreviousItem;
    setImPlotItem(plotItem);
    if (plotItem && plotItem->Show != QImAbstractNode::isVisible()) {
        QImAbstractNode::setVisible(plotItem->Show);
    }
    if (!d->color) {
        d->color = ImPlot::GetLastItemColor();
        d->color->mark_clean();
    }

    if (d->textVisible && !d->texts.isEmpty()) {
        const int count = std::min(series->size(), d->texts.size());
        const bool hasTextColor = d->textColor.has_value();
        if (hasTextColor) {
            ImPlot::PushStyleColor(ImPlotCol_InlayText, d->textColor->value());
        }
        for (int i = 0; i < count; ++i) {
            const QString& text = d->texts.at(i);
            if (text.isEmpty()) {
                continue;
            }
            const QByteArray utf8 = text.toUtf8();
            ImPlot::PlotText(utf8.constData(),
                             series->xValue(i),
                             series->yValue(i),
                             toImVec2(d->textPixelOffset));
        }
        if (hasTextColor) {
            ImPlot::PopStyleColor();
        }
    }
    return false;
}

}  // namespace QIM
