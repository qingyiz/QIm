#include "QImPlotLineItemNode.h"
#include <numeric>
#include <optional>
#include "QImPlotDataSeries.h"
#include "QImLTTBDownsampler.h"
#include "QImMinMaxLTTBDownsampler.h"
#include "implot.h"
#include "implot_internal.h"
#include "QImTrackedValue.hpp"
#include "QtImGuiUtils.h"
#include <QDebug>
#include <QPen>
namespace QIM
{
// ImPlotMarker_None   ->   无标记
// ImPlotMarker_Circle   ->   ● 圆形
// ImPlotMarker_Square   ->   ■ 正方形
// ImPlotMarker_Diamond   ->   ◆ 菱形
// ImPlotMarker_Up   ->   ▲ 上三角
// ImPlotMarker_Down   ->   ▼ 下三角
// ImPlotMarker_Left   ->   ◀ 左三角
// ImPlotMarker_Right   ->   ▶ 右三角
// ImPlotMarker_Cross   ->   ✕ 叉形
// ImPlotMarker_Plus   ->   ＋ 加号
// ImPlotMarker_Asterisk   ->   ✻ 星形

class QImPlotLineItemNode::PrivateData
{
    QIM_DECLARE_PUBLIC(QImPlotLineItemNode)
public:
    PrivateData(QImPlotLineItemNode* p);
    void resetDownSamplerData();
    std::unique_ptr< QImAbstractXYDataSeries > data;
    std::unique_ptr< QImAbstractXYDataSeries > dataLTTB;
    bool isAdaptiveSampling { true };
    int downsampleThreshold { 20000 };
    ImPlotLineFlags lineFlags { ImPlotLineFlags_None };
    Qt::PenStyle lineStyle { Qt::SolidLine };                                                ///< 线样式
    std::optional< QImTrackedValue< ImVec4, ImVecComparator< ImVec4 > > > color;             ///< 颜色
    QImTrackedValue< float > lineWidth { 1.0f };                                             ///< 线宽
    ImPlotMarker markerShape { ImPlotMarker_None };                                           ///< 点形状
    QImTrackedValue< float > markerSize { 4.0f };                                            ///< 点大小
    QImTrackedValue< float > markerWeight { 1.0f };                                          ///< 点边框宽度
    std::optional< QImTrackedValue< ImVec4, ImVecComparator< ImVec4 > > > markerFillColor;   ///< 点填充颜色
    std::optional< QImTrackedValue< ImVec4, ImVecComparator< ImVec4 > > > markerOutlineColor;  ///< 点轮廓颜色
    std::vector< double > zData;                                                             ///< 用于指定曲线方向的数据
    bool directionArrowsVisible { true };
    bool isPlotItemVisible;
};

namespace
{
constexpr int kSafeRenderPointLimit = 12000;

int effectiveDownsampleTarget(bool adaptiveSampling, int requestedThreshold)
{
    if (adaptiveSampling) {
        return std::min(requestedThreshold, kSafeRenderPointLimit);
    }
    return kSafeRenderPointLimit;
}

enum class DirectionZOrder
{
    None,
    Monotonic
};

DirectionZOrder directionZOrder(const std::vector< double >& zData)
{
    if (zData.size() < 2) {
        return DirectionZOrder::None;
    }

    bool hasPositiveDiff = false;
    bool hasNegativeDiff = false;
    for (std::size_t i = 1; i < zData.size(); ++i) {
        const double previous = zData[ i - 1 ];
        const double current  = zData[ i ];
        if (!std::isfinite(previous) || !std::isfinite(current)) {
            return DirectionZOrder::None;
        }

        const double diff = current - previous;
        if (std::abs(diff) <= 1e-12) {
            continue;
        }
        if (diff > 0.0) {
            hasPositiveDiff = true;
        } else {
            hasNegativeDiff = true;
        }
        if (hasPositiveDiff && hasNegativeDiff) {
            return DirectionZOrder::None;
        }
    }
    return (hasPositiveDiff || hasNegativeDiff) ? DirectionZOrder::Monotonic : DirectionZOrder::None;
}

std::vector< float > patternForPenStyle(Qt::PenStyle style, float width)
{
    const float unit = std::max(width, 1.0f);
    switch (style) {
    case Qt::DashLine:
        return { 4.0f * unit, 2.0f * unit };
    case Qt::DotLine:
        return { 1.0f * unit, 1.5f * unit };
    case Qt::DashDotLine:
        return { 4.0f * unit, 2.0f * unit, 1.0f * unit, 2.0f * unit };
    case Qt::DashDotDotLine:
        return { 4.0f * unit, 2.0f * unit, 1.0f * unit, 2.0f * unit, 1.0f * unit, 2.0f * unit };
    default:
        return {};
    }
}

bool needsCustomLineRendering(Qt::PenStyle style)
{
    return style != Qt::SolidLine;
}

struct DirectionSegment
{
    ImVec2 from;
    ImVec2 to;
    float length;
};

void drawStyledSegment(
    ImDrawList* drawList,
    const ImVec2& p1,
    const ImVec2& p2,
    Qt::PenStyle style,
    float width,
    ImU32 color
)
{
    if (style == Qt::NoPen) {
        return;
    }
    const float dx = p2.x - p1.x;
    const float dy = p2.y - p1.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length <= 0.0f) {
        return;
    }
    const std::vector< float > pattern = patternForPenStyle(style, width);
    if (pattern.empty()) {
        drawList->AddLine(p1, p2, color, width);
        return;
    }

    const ImVec2 dir(dx / length, dy / length);
    float offset = 0.0f;
    bool draw = true;
    std::size_t patternIndex = 0;
    while (offset < length) {
        const float step = pattern[ patternIndex % pattern.size() ];
        const float next = std::min(length, offset + step);
        if (draw && next > offset) {
            const ImVec2 a(p1.x + dir.x * offset, p1.y + dir.y * offset);
            const ImVec2 b(p1.x + dir.x * next, p1.y + dir.y * next);
            drawList->AddLine(a, b, color, width);
        }
        draw = !draw;
        offset = next;
        ++patternIndex;
    }
}

void drawCustomStyledLine(
    QImAbstractXYDataSeries* series,
    ImPlotLineFlags flags,
    Qt::PenStyle style,
    float width,
    ImU32 color,
    QImPlotAxisId xAxisId,
    QImPlotAxisId yAxisId
)
{
    if (!series || series->size() < 2 || style == Qt::SolidLine || style == Qt::NoPen) {
        return;
    }

    ImDrawList* drawList = ImPlot::GetPlotDrawList();
    if (!drawList) {
        return;
    }

    std::vector< ImPlotPoint > currentPoints;
    auto flushPoints = [&](bool closeLoop) {
        if (currentPoints.size() < 2) {
            currentPoints.clear();
            return;
        }
        for (std::size_t i = 1; i < currentPoints.size(); ++i) {
            const ImVec2 p1 =
                ImPlot::PlotToPixels(currentPoints[ i - 1 ], static_cast< ImAxis >(toImAxis(xAxisId)), static_cast< ImAxis >(toImAxis(yAxisId)));
            const ImVec2 p2 =
                ImPlot::PlotToPixels(currentPoints[ i ], static_cast< ImAxis >(toImAxis(xAxisId)), static_cast< ImAxis >(toImAxis(yAxisId)));
            drawStyledSegment(drawList, p1, p2, style, width, color);
        }
        if (closeLoop) {
            const ImVec2 p1 =
                ImPlot::PlotToPixels(currentPoints.back(), static_cast< ImAxis >(toImAxis(xAxisId)), static_cast< ImAxis >(toImAxis(yAxisId)));
            const ImVec2 p2 =
                ImPlot::PlotToPixels(currentPoints.front(), static_cast< ImAxis >(toImAxis(xAxisId)), static_cast< ImAxis >(toImAxis(yAxisId)));
            drawStyledSegment(drawList, p1, p2, style, width, color);
        }
        currentPoints.clear();
    };

    const bool skipNaN = (flags & ImPlotLineFlags_SkipNaN) != 0;
    const bool loop = (flags & ImPlotLineFlags_Loop) != 0;
    for (int i = 0; i < series->size(); ++i) {
        const double x = series->xValue(i);
        const double y = series->yValue(i);
        const bool valid = std::isfinite(x) && std::isfinite(y);
        if (!valid) {
            if (!skipNaN) {
                flushPoints(false);
            }
            continue;
        }
        currentPoints.emplace_back(x, y);
    }
    flushPoints(loop);
}

std::vector< DirectionSegment > buildDirectionSegments(
    QImAbstractXYDataSeries* series,
    ImPlotLineFlags flags,
    QImPlotAxisId xAxisId,
    QImPlotAxisId yAxisId
)
{
    std::vector< DirectionSegment > segments;
    if (!series || series->size() < 2) {
        return segments;
    }

    bool hasPrevious = false;
    ImPlotPoint previousPoint {};
    for (int i = 0; i < series->size(); ++i) {
        const double x = series->xValue(i);
        const double y = series->yValue(i);
        const bool valid = std::isfinite(x) && std::isfinite(y);
        if (!valid) {
            hasPrevious = false;
            continue;
        }

        const ImPlotPoint currentPoint(x, y);
        if (hasPrevious) {
            const ImVec2 fromPixels = ImPlot::PlotToPixels(
                previousPoint, static_cast< ImAxis >(toImAxis(xAxisId)), static_cast< ImAxis >(toImAxis(yAxisId)));
            const ImVec2 toPixels = ImPlot::PlotToPixels(
                currentPoint, static_cast< ImAxis >(toImAxis(xAxisId)), static_cast< ImAxis >(toImAxis(yAxisId)));
            const float dx = toPixels.x - fromPixels.x;
            const float dy = toPixels.y - fromPixels.y;
            const float segmentLength = std::sqrt(dx * dx + dy * dy);
            if (segmentLength > 1.0f) {
                segments.push_back({ fromPixels, toPixels, segmentLength });
            }
            if ((flags & ImPlotLineFlags_Segments) != 0) {
                hasPrevious = false;
                continue;
            }
        }
        previousPoint = currentPoint;
        hasPrevious = true;
    }

    return segments;
}

int automaticDirectionArrowCount(float totalLength)
{
    if (totalLength < 140.0f) {
        return 0;
    }
    if (totalLength < 360.0f) {
        return 2;
    }
    return 3;
}

void drawDirectionArrows(
    QImAbstractXYDataSeries* series,
    ImPlotLineFlags flags,
    const std::vector< double >& zData,
    float lineWidth,
    ImU32 color,
    QImPlotAxisId xAxisId,
    QImPlotAxisId yAxisId
)
{
    if (!series || series->size() < 2 || zData.size() != static_cast< std::size_t >(series->size())) {
        return;
    }
    if (directionZOrder(zData) == DirectionZOrder::None) {
        return;
    }

    const std::vector< DirectionSegment > segments = buildDirectionSegments(series, flags, xAxisId, yAxisId);
    if (segments.empty()) {
        return;
    }

    float totalLength = 0.0f;
    for (const DirectionSegment& segment : segments) {
        totalLength += segment.length;
    }

    const int arrowCount = automaticDirectionArrowCount(totalLength);
    if (arrowCount <= 0) {
        return;
    }

    ImDrawList* drawList = ImPlot::GetPlotDrawList();
    if (!drawList) {
        return;
    }

    const float triangleSide = std::max(10.0f, lineWidth * 3.2f + 6.0f);
    const float triangleHeight = triangleSide * std::sqrt(3.0f) * 0.5f;
    const float centroidToTip = triangleHeight * (2.0f / 3.0f);
    const float centroidToBase = triangleHeight * (1.0f / 3.0f);
    const float halfBase = triangleSide * 0.5f;
    for (int arrowIndex = 0; arrowIndex < arrowCount; ++arrowIndex) {
        const float targetLength = totalLength * static_cast< float >(arrowIndex + 1) / static_cast< float >(arrowCount + 1);
        float traversed = 0.0f;
        for (const DirectionSegment& segment : segments) {
            if ((traversed + segment.length) < targetLength) {
                traversed += segment.length;
                continue;
            }

            const float localT = std::clamp((targetLength - traversed) / segment.length, 0.0f, 1.0f);
            const ImVec2 position(
                segment.from.x + (segment.to.x - segment.from.x) * localT,
                segment.from.y + (segment.to.y - segment.from.y) * localT);
            const ImVec2 direction(
                (segment.to.x - segment.from.x) / segment.length, (segment.to.y - segment.from.y) / segment.length);
            const ImVec2 perpendicular(-direction.y, direction.x);

            // Use an equilateral triangle so the direction marker remains visually balanced.
            const ImVec2 tip(position.x + direction.x * centroidToTip, position.y + direction.y * centroidToTip);
            const ImVec2 baseCenter(
                position.x - direction.x * centroidToBase, position.y - direction.y * centroidToBase);
            const ImVec2 left(baseCenter.x + perpendicular.x * halfBase, baseCenter.y + perpendicular.y * halfBase);
            const ImVec2 right(baseCenter.x - perpendicular.x * halfBase, baseCenter.y - perpendicular.y * halfBase);

            drawList->AddTriangleFilled(tip, left, right, color);
            break;
        }
    }
}
}  // namespace

QImPlotLineItemNode::PrivateData::PrivateData(QImPlotLineItemNode* p) : q_ptr(p)
{
}

/**
 * @brief 重置降采样数据，在设置数据后或者
 */
void QImPlotLineItemNode::PrivateData::resetDownSamplerData()
{
    dataLTTB.reset(nullptr);
    if (data) {
        const int targetPoints = effectiveDownsampleTarget(isAdaptiveSampling, downsampleThreshold);
        if (data->size() > targetPoints) {
#if 0
            QImLTTBDownsampler* lttb = new QImLTTBDownsampler(data.get(), targetPoints);
            dataLTTB.reset(lttb);
#else
            QImMinMaxLTTBDownsampler* lttb = new QImMinMaxLTTBDownsampler(data.get(), targetPoints);
            dataLTTB.reset(lttb);
#endif
        }
    }
}
//----------------------------------------------------
// QImPlotLineItemNode
//----------------------------------------------------
QImPlotLineItemNode::QImPlotLineItemNode(QObject* par) : QImPlotItemNode(par), QIM_PIMPL_CONSTRUCT
{
}

QImPlotLineItemNode::~QImPlotLineItemNode()
{
}

void QImPlotLineItemNode::setData(QImAbstractXYDataSeries* series)
{
    QIM_D(d);
    d->data.reset(series);
    if (d->isAdaptiveSampling) {
        d->resetDownSamplerData();
    }
}


QImAbstractXYDataSeries* QImPlotLineItemNode::data() const
{
    return d_ptr->data.get();
}

void QImPlotLineItemNode::setZData(const std::vector< double >& z)
{
    setZData(std::vector< double >(z));
}

void QImPlotLineItemNode::setZData(std::vector< double >&& z)
{
    if (d_ptr->zData != z) {
        d_ptr->zData = std::move(z);
        emit zDataChanged();
    }
}

void QImPlotLineItemNode::clearZData()
{
    if (!d_ptr->zData.empty()) {
        d_ptr->zData.clear();
        emit zDataChanged();
    }
}

bool QImPlotLineItemNode::hasZData() const
{
    return !d_ptr->zData.empty();
}

bool QImPlotLineItemNode::hasOrderedZData() const
{
    return directionZOrder(d_ptr->zData) == DirectionZOrder::Monotonic;
}

// ===== 在 CPP 文件顶部添加辅助宏定义 =====
#ifndef QImPlotLineItemNode_FLAG_ACCESSOR
#define QImPlotLineItemNode_FLAG_ACCESSOR(FlagName, FlagEnum)                                                          \
    bool QImPlotLineItemNode::is##FlagName() const                                                                     \
    {                                                                                                                  \
        QIM_DC(d);                                                                                                     \
        return (d->lineFlags & FlagEnum) != 0;                                                                         \
    }                                                                                                                  \
    void QImPlotLineItemNode::set##FlagName(bool on)                                                                   \
    {                                                                                                                  \
        QIM_D(d);                                                                                                      \
        const ImPlotLineFlags oldFlags = d->lineFlags;                                                                 \
        if (on)                                                                                                        \
            d->lineFlags |= FlagEnum;                                                                                  \
        else                                                                                                           \
            d->lineFlags &= ~FlagEnum;                                                                                 \
        if (d->lineFlags != oldFlags)                                                                                  \
            emit lineFlagChanged();                                                                                    \
    }
#endif
#ifndef QImPlotLineItemNode_ENABLED_ACCESSOR
#define QImPlotLineItemNode_ENABLED_ACCESSOR(PropName, FlagEnum)                                                       \
    bool QImPlotLineItemNode::is##PropName() const                                                                     \
    {                                                                                                                  \
        QIM_DC(d);                                                                                                     \
        return (d->lineFlags & FlagEnum) == 0;                                                                         \
    }                                                                                                                  \
    void QImPlotLineItemNode::set##PropName(bool enabled)                                                              \
    {                                                                                                                  \
        QIM_D(d);                                                                                                      \
        const ImPlotLineFlags oldFlags = d->lineFlags;                                                                 \
        if (enabled)                                                                                                   \
            d->lineFlags &= ~FlagEnum;                                                                                 \
        else                                                                                                           \
            d->lineFlags |= FlagEnum;                                                                                  \
        if (d->lineFlags != oldFlags)                                                                                  \
            emit lineFlagChanged();                                                                                    \
    }
#endif

/**
 * \if ENGLISH
 * @brief Returns the raw ImPlotLineFlags bitmask value for direct ImPlot API usage
 * @return Current combined flags as ImPlotLineFlags bitmask
 * @details Provides direct access to the underlying ImPlotLineFlags bitmask stored internally.
 *          The value can be passed directly to ImPlot::PlotLine() as the flags parameter.
 * @see setLineFlags(), ImPlot::PlotLine()
 * \endif
 *
 * \if CHINESE
 * @brief 返回原始 ImPlotLineFlags 位掩码值，用于直接调用 ImPlot API
 * @return 当前组合标志的 ImPlotLineFlags 位掩码
 * @details 提供对内部存储的 ImPlotLineFlags 位掩码的直接访问。
 *          该值可直接作为 flags 参数传递给 ImPlot::PlotLine()。
 * @see setLineFlags(), ImPlot::PlotLine()
 * \endif
 */
int QImPlotLineItemNode::lineFlags() const
{
    QIM_DC(d);
    return d->lineFlags;
}

/**
 * \if ENGLISH
 * @brief Sets the raw ImPlotLineFlags bitmask value with signal emission on change
 * @param flags New combined flags as ImPlotLineFlags bitmask
 * @details Directly replaces the internal flag storage with the provided bitmask value.
 *          All individual property states (segments, loop, skipNaN, etc.) are conceptually
 *          updated to reflect the new flags value. Emits lineFlagChanged() signal ONLY if the
 *          new value differs from current value (checked via != comparison).
 * @note This method bypasses individual property validation logic but maintains signal emission
 *       for reactive UI updates. Prefer individual property setters for type-safe configuration.
 * @warning May set invalid flag combinations (though ImPlotLineFlags has few conflicts).
 * @see lineFlags()
 * \endif
 *
 * \if CHINESE
 * @brief 设置原始 ImPlotLineFlags 位掩码值，值变更时触发信号
 * @param flags 新的组合标志 ImPlotLineFlags 位掩码
 * @details 直接用提供的位掩码值替换内部标志存储。
 *          所有单独的属性状态（segments、loop、skipNaN 等）在概念上都会更新以反映新标志值。
 *          仅当新值与当前值不同时（通过 != 比较），才会触发 lineFlagChanged() 信号。
 * @note 此方法绕过单独的属性验证逻辑，但保留信号发射以支持响应式 UI 更新。
 *       为类型安全配置，建议优先使用单独的属性设置器。
 * @warning 可能设置无效标志组合（尽管 ImPlotLineFlags 冲突较少）。
 * @see lineFlags()
 * \endif
 */
void QImPlotLineItemNode::setLineFlags(int flags)
{
    QIM_D(d);
    if (d->lineFlags != flags) {
        d->lineFlags = flags;
        emit lineFlagChanged();
    }
}


void QImPlotLineItemNode::setColor(const QColor& c)
{
    const ImVec4 color = toImVec4(c);
    const bool changed = !d_ptr->color || !ImVecComparator< ImVec4 > {}(d_ptr->color->value(), color);
    if (d_ptr->color) {
        d_ptr->color->value() = color;
        if (changed) {
            d_ptr->color->mark_dirty();
        }
    } else {
        d_ptr->color.emplace(color);
        d_ptr->color->mark_dirty();
    }
    if (changed) {
        emit colorChanged(c);
    }
}

QColor QImPlotLineItemNode::color() const
{
    return (d_ptr->color.has_value()) ? toQColor(d_ptr->color->value()) : QColor();
}

void QImPlotLineItemNode::setLineStyle(int style)
{
    QIM_D(d);
    const Qt::PenStyle penStyle = static_cast< Qt::PenStyle >(style);
    if (d->lineStyle != penStyle) {
        d->lineStyle = penStyle;
        emit lineStyleChanged(style);
    }
}

int QImPlotLineItemNode::lineStyle() const
{
    QIM_DC(d);
    return d->lineStyle;
}

void QImPlotLineItemNode::setLineWidth(float width)
{
    QIM_D(d);
    d->lineWidth = width;
    if (d->lineWidth.is_dirty()) {
        emit lineWidthChanged(width);
    }
}

float QImPlotLineItemNode::lineWidth() const
{
    QIM_DC(d);
    return d->lineWidth.value();
}

void QImPlotLineItemNode::setMarkerShape(int shape)
{
    QIM_D(d);
    const ImPlotMarker marker = static_cast< ImPlotMarker >(shape);
    if (d->markerShape != marker) {
        d->markerShape = marker;
        emit markerShapeChanged(shape);
    }
}

int QImPlotLineItemNode::markerShape() const
{
    QIM_DC(d);
    return d->markerShape;
}

void QImPlotLineItemNode::setMarkerSize(float size)
{
    QIM_D(d);
    d->markerSize = size;
    if (d->markerSize.is_dirty()) {
        emit markerSizeChanged(size);
    }
}

float QImPlotLineItemNode::markerSize() const
{
    QIM_DC(d);
    return d->markerSize.value();
}

void QImPlotLineItemNode::setMarkerWeight(float weight)
{
    QIM_D(d);
    d->markerWeight = weight;
    if (d->markerWeight.is_dirty()) {
        emit markerWeightChanged(weight);
    }
}

float QImPlotLineItemNode::markerWeight() const
{
    QIM_DC(d);
    return d->markerWeight.value();
}

void QImPlotLineItemNode::setMarkerFillColor(const QColor& color)
{
    const bool changed = !d_ptr->markerFillColor ||
                         !ImVecComparator< ImVec4 > {}(d_ptr->markerFillColor->value(), toImVec4(color));
    if (d_ptr->markerFillColor) {
        d_ptr->markerFillColor->value() = toImVec4(color);
        if (changed) {
            d_ptr->markerFillColor->mark_dirty();
        }
    } else {
        d_ptr->markerFillColor.emplace(toImVec4(color));
        d_ptr->markerFillColor->mark_dirty();
    }
    if (changed) {
        emit markerFillColorChanged(color);
    }
}

QColor QImPlotLineItemNode::markerFillColor() const
{
    return d_ptr->markerFillColor ? toQColor(d_ptr->markerFillColor->value()) : QColor();
}

void QImPlotLineItemNode::setMarkerOutlineColor(const QColor& color)
{
    const bool changed = !d_ptr->markerOutlineColor ||
                         !ImVecComparator< ImVec4 > {}(d_ptr->markerOutlineColor->value(), toImVec4(color));
    if (d_ptr->markerOutlineColor) {
        d_ptr->markerOutlineColor->value() = toImVec4(color);
        if (changed) {
            d_ptr->markerOutlineColor->mark_dirty();
        }
    } else {
        d_ptr->markerOutlineColor.emplace(toImVec4(color));
        d_ptr->markerOutlineColor->mark_dirty();
    }
    if (changed) {
        emit markerOutlineColorChanged(color);
    }
}

QColor QImPlotLineItemNode::markerOutlineColor() const
{
    return d_ptr->markerOutlineColor ? toQColor(d_ptr->markerOutlineColor->value()) : QColor();
}

void QImPlotLineItemNode::setDirectionArrowsVisible(bool on)
{
    if (d_ptr->directionArrowsVisible != on) {
        d_ptr->directionArrowsVisible = on;
        emit directionArrowsVisibilityChanged(on);
    }
}

bool QImPlotLineItemNode::isDirectionArrowsVisible() const
{
    return d_ptr->directionArrowsVisible;
}

void QImPlotLineItemNode::setAdaptivesSampling(bool on)
{
    setAdaptiveSampling(on);
}

void QImPlotLineItemNode::setAdaptiveSampling(bool on)
{
    if (d_ptr->isAdaptiveSampling == on) {
        return;
    }
    d_ptr->isAdaptiveSampling = on;
    d_ptr->resetDownSamplerData();
    emit adaptiveSamplingChanged(on);
}

bool QImPlotLineItemNode::isAdaptiveSampling() const
{
    return d_ptr->isAdaptiveSampling;
}

int QImPlotLineItemNode::downsampleThreshold() const
{
    return d_ptr->downsampleThreshold;
}

void QImPlotLineItemNode::setDownsampleThreshold(int threshold)
{
    if (threshold <= 0 || d_ptr->downsampleThreshold == threshold) {
        return;
    }
    d_ptr->downsampleThreshold = threshold;
    d_ptr->resetDownSamplerData();
    emit downsampleThresholdChanged(threshold);
}

// ===== 标志访问器实现（带 Doxygen 注释）=====
// clang-format off

/**
 * \if ENGLISH
 * @brief Checks if line is rendered as discrete segments between consecutive points
 * @return true if segments mode is enabled (ImPlotLineFlags_Segments set)
 * @details When enabled, each pair of consecutive points forms an independent line segment.
 *          Gaps appear where data is missing (e.g., NaN values) even without SkipNaN flag.
 *          Default behavior (disabled) connects all points with a continuous polyline.
 * @note Corresponds to ImPlotLineFlags_Segments flag (direct mapping).
 * @warning Segments mode disables automatic gap detection; use SkipNaN flag for explicit NaN handling.
 * @see setSegments(), isSkipNaN()
 * \endif
 *
 * \if CHINESE
 * @brief 检查线条是否渲染为连续点之间的离散线段
 * @return true 表示启用线段模式（设置了 ImPlotLineFlags_Segments）
 * @details 启用后，每对连续点形成独立的线段。
 *          即使未设置 SkipNaN 标志，缺失数据处（例如 NaN 值）也会出现间隙。
 *          默认行为（禁用）用连续折线连接所有点。
 * @note 对应 ImPlotLineFlags_Segments 标志（直接映射）。
 * @warning 线段模式禁用自动间隙检测；使用 SkipNaN 标志进行显式的 NaN 处理。
 * @see setSegments(), isSkipNaN()
 * \endif
 */
QImPlotLineItemNode_FLAG_ACCESSOR(Segments, ImPlotLineFlags_Segments)

/**
 * \if ENGLISH
 * @brief Checks if line forms a closed loop by connecting last and first points
 * @return true if loop mode is enabled (ImPlotLineFlags_Loop set)
 * @details When enabled, an additional segment connects the last data point to the first point.
 *          Creates closed shapes suitable for polygons, orbits, or cyclic data visualization.
 *          Requires at least 3 points for visible effect (2 points form a line segment).
 * @note Corresponds to ImPlotLineFlags_Loop flag (direct mapping).
 * @warning Loop mode combined with Segments flag creates disconnected segments plus closing segment.
 * @see setLoop(), isSegments()
 * \endif
 *
 * \if CHINESE
 * @brief 检查线条是否通过连接最后和第一个点形成闭合环
 * @return true 表示启用环模式（设置了 ImPlotLineFlags_Loop）
 * @details 启用后，额外线段连接最后一个数据点和第一个点。
 *          创建适合多边形、轨道或循环数据可视化的闭合形状。
 *          需要至少 3 个点才能看到明显效果（2 个点形成线段）。
 * @note 对应 ImPlotLineFlags_Loop 标志（直接映射）。
 * @warning 环模式与线段模式组合时，会创建不相连的线段加上闭合线段。
 * @see setLoop(), isSegments()
 * \endif
 */
QImPlotLineItemNode_FLAG_ACCESSOR(Loop, ImPlotLineFlags_Loop)

/**
 * \if ENGLISH
 * @brief Checks if NaN values are skipped during rendering (no gap drawn)
 * @return true if NaN skipping is enabled (ImPlotLineFlags_SkipNaN set)
 * @details When enabled, points with NaN coordinates are silently skipped without breaking the line.
 *          Creates continuous appearance even with missing data points.
 *          When disabled, NaN values break the line into separate segments (gap appears).
 * @note Corresponds to ImPlotLineFlags_SkipNaN flag (direct mapping).
 * @warning Skipping NaNs may hide data quality issues; use with caution for scientific visualization.
 * @see setSkipNaN(), isSegments()
 * \endif
 *
 * \if CHINESE
 * @brief 检查渲染时是否跳过 NaN 值（不绘制间隙）
 * @return true 表示启用 NaN 跳过（设置了 ImPlotLineFlags_SkipNaN）
 * @details 启用后，具有 NaN 坐标的点会被静默跳过，不会中断线条。
 *          即使存在缺失数据点，也能创建连续外观。
 *          禁用时，NaN 值会将线条断开为独立线段（出现间隙）。
 * @note 对应 ImPlotLineFlags_SkipNaN 标志（直接映射）。
 * @warning 跳过 NaN 可能隐藏数据质量问题；科学可视化中请谨慎使用。
 * @see setSkipNaN(), isSegments()
 * \endif
 */
QImPlotLineItemNode_FLAG_ACCESSOR(SkipNaN, ImPlotLineFlags_SkipNaN)

/**
 * \if ENGLISH
 * @brief Checks if markers on plot edges are clipped to plot area boundaries
 * @return true if clipping is enabled (ImPlotLineFlags_NoClip NOT set)
 * @details When enabled, markers (e.g., circles at data points) are clipped at plot boundaries.
 *          Prevents visual artifacts when markers extend beyond visible area.
 *          When disabled (NoClip set), markers render fully even when partially outside plot area.
 * @note Corresponds to ImPlotLineFlags_NoClip flag with inverted logic:
 *       enabled = flag NOT set (clipping active), disabled = flag set (no clipping).
 * @warning Disabling clipping may cause markers to overlap adjacent UI elements or axes.
 * @see setClippingEnabled()
 * \endif
 *
 * \if CHINESE
 * @brief 检查绘图边缘的标记是否被裁剪到绘图区域边界内
 * @return true 表示启用裁剪（未设置 ImPlotLineFlags_NoClip）
 * @details 启用后，标记（例如数据点处的圆圈）在绘图边界处被裁剪。
 *          防止标记超出可见区域时产生视觉瑕疵。
 *          禁用时（设置 NoClip），即使部分超出绘图区域，标记也会完整渲染。
 * @note 对应 ImPlotLineFlags_NoClip 标志，逻辑反转：
 *       启用 = 未设置标志位（裁剪激活），禁用 = 设置标志位（无裁剪）。
 * @warning 禁用裁剪可能导致标记与相邻 UI 元素或坐标轴重叠。
 * @see setClippingEnabled()
 * \endif
 */
QImPlotLineItemNode_ENABLED_ACCESSOR(ClippingEnabled, ImPlotLineFlags_NoClip)

/**
 * \if ENGLISH
 * @brief Checks if area between line and horizontal origin is filled with color
 * @return true if shaded mode is enabled (ImPlotLineFlags_Shaded set)
 * @details When enabled, a filled region is rendered between the line and y=0 (horizontal origin).
 *          Creates area-chart appearance. Color matches line color with reduced opacity.
 *          For custom fill baselines or multi-line fills, use ImPlot::PlotShaded() directly.
 * @note Corresponds to ImPlotLineFlags_Shaded flag (direct mapping).
 * @warning Shaded mode requires non-negative Y values for predictable appearance.
 *          Negative values create fills extending downward from y=0.
 * @see setShaded(), ImPlot::PlotShaded()
 * \endif
 *
 * \if CHINESE
 * @brief 检查线条与水平原点之间的区域是否填充颜色
 * @return true 表示启用填充模式（设置了 ImPlotLineFlags_Shaded）
 * @details 启用后，线条与 y=0（水平原点）之间会渲染填充区域。
 *          创建面积图外观。颜色与线条颜色匹配但透明度降低。
 *          对于自定义填充基线或多线条填充，请直接使用 ImPlot::PlotShaded()。
 * @note 对应 ImPlotLineFlags_Shaded 标志（直接映射）。
 * @warning 填充模式要求 Y 值非负以获得可预测外观。
 *          负值会创建从 y=0 向下延伸的填充。
 * @see setShaded(), ImPlot::PlotShaded()
 * \endif
 */
QImPlotLineItemNode_FLAG_ACCESSOR(Shaded, ImPlotLineFlags_Shaded)
// clang-format off
    // clang-format on
    // clang-format on

    /**
     * @brief 绘图
     * @return  这里直接返回false，避免调用endDraw
     */
    bool QImPlotLineItemNode::beginDraw()
{
    QIM_D(d);
    if (!d->data) {
        // 没有数据
        return false;
    }
    QImAbstractXYDataSeries* series = d->data.get();
    if (d->dataLTTB) {
        series = d->dataLTTB.get();
    }
    if (!series) {
        return false;
    }
    const bool customStyledLine = needsCustomLineRendering(d->lineStyle);
    if ((d->color && d->color->is_dirty()) || d->lineWidth.is_dirty() || d->lineWidth.value() != 1.0f || customStyledLine) {
        const float implotLineWidth = (customStyledLine || d->lineStyle == Qt::NoPen) ? 0.0f : d->lineWidth.value();
        ImPlot::SetNextLineStyle(d->color ? d->color->value() : IMPLOT_AUTO_COL, implotLineWidth);
        if (d->color && d->color->is_dirty()) {
            d->color->mark_clean();
        }
        d->lineWidth.mark_clean();
    }
    if (d->markerShape != ImPlotMarker_None || d->markerSize.is_dirty() || d->markerSize.value() != 4.0f ||
        d->markerWeight.is_dirty() || d->markerWeight.value() != 1.0f || d->markerFillColor ||
        d->markerOutlineColor) {
        ImPlot::SetNextMarkerStyle(d->markerShape,
                                   d->markerSize.value(),
                                   d->markerFillColor ? d->markerFillColor->value() : IMPLOT_AUTO_COL,
                                   d->markerWeight.value(),
                                   d->markerOutlineColor ? d->markerOutlineColor->value() : IMPLOT_AUTO_COL);
        d->markerSize.mark_clean();
        d->markerWeight.mark_clean();
        if (d->markerFillColor && d->markerFillColor->is_dirty()) {
            d->markerFillColor->mark_clean();
        }
        if (d->markerOutlineColor && d->markerOutlineColor->is_dirty()) {
            d->markerOutlineColor->mark_clean();
        }
    }
    if (series->isContiguous()) {
        if (series->xRawData()) {
            // 有x指针，说明不是yonly
            ImPlot::PlotLine(
                labelConstData(),
                series->xRawData(),
                series->yRawData(),
                series->size(),
                d->lineFlags,
                series->offset(),
                series->stride()
            );
        } else {
            // x指针没有说明是yonly
            ImPlot::PlotLine(
                labelConstData(),
                series->yRawData(),
                series->size(),
                series->xScale(),
                series->xStart(),
                d->lineFlags,
                series->offset(),
                series->stride()
            );
        }
    } else {
        // TODO:非连续内存
    }
    // 更新item的状态
    ImPlotContext* ct    = ImPlot::GetCurrentContext();
    ImPlotItem* plotItem = ct->PreviousItem;  // 通过源码，PlotLine结束后，ImPlotItem就是PreviousItem
    setImPlotItem(plotItem);
    if (plotItem->Show != QImAbstractNode::isVisible()) {
        // 状态发生了变化，这种情况是label点击，设置了show状态和QImAbstractNode记录的状态不一致
        // 这时要同步状态
        QImAbstractNode::setVisible(plotItem->Show);  // 此函数会触发信号
    }
    if (!d->color) {
        // 一般是首次渲染，且没设定颜色，这时是implot给的默认颜色，把这个默认颜色获取到
        d->color = ImPlot::GetLastItemColor();
        d->color->mark_clean();
    }
    const bool shouldDrawDirectionArrows =
        d->directionArrowsVisible && (d->lineStyle != Qt::NoPen) && (directionZOrder(d->zData) == DirectionZOrder::Monotonic);
    const bool shouldDrawOverlay = plotItem && plotItem->Show && (customStyledLine || shouldDrawDirectionArrows);
    if (shouldDrawOverlay) {
        ImPlot::PushPlotClipRect();
        if (customStyledLine) {
            drawCustomStyledLine(series,
                                 d->lineFlags,
                                 d->lineStyle,
                                 d->lineWidth.value(),
                                 plotItem->Color,
                                 xAxisId(),
                                 yAxisId());
        }
        if (shouldDrawDirectionArrows) {
            drawDirectionArrows(
                d->data.get(), d->lineFlags, d->zData, std::max(d->lineWidth.value(), 1.0f), plotItem->Color, xAxisId(), yAxisId());
        }
        ImPlot::PopPlotClipRect();
    }
    // 绘图之后，更新状态

    return false;
}


}  // end namespace QIM
