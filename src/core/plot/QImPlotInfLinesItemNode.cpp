#include "QImPlotInfLinesItemNode.h"
#include <optional>
#include "QImTrackedValue.hpp"
#include "QtImGuiUtils.h"
#include "implot.h"
#include "implot_internal.h"
#include <QByteArray>
#include <QPen>

namespace QIM
{

class QImPlotInfLinesItemNode::PrivateData
{
    QIM_DECLARE_PUBLIC(QImPlotInfLinesItemNode)
public:
    explicit PrivateData(QImPlotInfLinesItemNode* p) : q_ptr(p)
    {
    }

    std::vector< double > values;
    ImPlotInfLinesFlags flags { ImPlotInfLinesFlags_None };
    Qt::PenStyle lineStyle { Qt::SolidLine };
    std::optional< QImTrackedValue< ImVec4, QIM::ImVecComparator< ImVec4 > > > color;
    QImTrackedValue< float > lineWidth { 1.0f };
    QByteArray textUtf8;
    QImPlotInfLinesItemNode::TextPosition textPosition { QImPlotInfLinesItemNode::TextPosition::Maximum };
    QImPlotInfLinesItemNode::TextOffset textOffset { QImPlotInfLinesItemNode::TextOffset::Center };
};

namespace
{
QImPlotInfLinesItemNode::TextPosition textPositionFromAnnotationPosition(QImPlotInfLinesItemNode::AnnotationPosition position)
{
    switch (static_cast< int >(position) / 3) {
    case 0:
        return QImPlotInfLinesItemNode::TextPosition::Minimum;
    case 1:
        return QImPlotInfLinesItemNode::TextPosition::Center;
    default:
        return QImPlotInfLinesItemNode::TextPosition::Maximum;
    }
}

QImPlotInfLinesItemNode::TextOffset textOffsetFromAnnotationPosition(QImPlotInfLinesItemNode::AnnotationPosition position)
{
    switch (static_cast< int >(position) % 3) {
    case 0:
        return QImPlotInfLinesItemNode::TextOffset::Negative;
    case 1:
        return QImPlotInfLinesItemNode::TextOffset::Center;
    default:
        return QImPlotInfLinesItemNode::TextOffset::Positive;
    }
}

QImPlotInfLinesItemNode::AnnotationPosition annotationPositionFromTextPlacement(
    QImPlotInfLinesItemNode::TextPosition position, QImPlotInfLinesItemNode::TextOffset offset)
{
    const int positionIndex = position == QImPlotInfLinesItemNode::TextPosition::Minimum ?
                                  0 :
                              position == QImPlotInfLinesItemNode::TextPosition::Center ? 1 :
                                                                                        2;
    const int offsetIndex = offset == QImPlotInfLinesItemNode::TextOffset::Negative ?
                                0 :
                            offset == QImPlotInfLinesItemNode::TextOffset::Center ? 1 :
                                                                                  2;
    return static_cast< QImPlotInfLinesItemNode::AnnotationPosition >(positionIndex * 3 + offsetIndex);
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

void drawStyledSegment(ImDrawList* drawList, const ImVec2& p1, const ImVec2& p2, Qt::PenStyle style, float width, ImU32 color)
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

void drawCustomInfLines(
    const std::vector< double >& values,
    bool horizontal,
    Qt::PenStyle style,
    float width,
    ImU32 color,
    QImPlotAxisId xAxisId,
    QImPlotAxisId yAxisId)
{
    if (values.empty() || style == Qt::SolidLine || style == Qt::NoPen) {
        return;
    }

    const ImAxis xAxis = static_cast< ImAxis >(toImAxis(xAxisId));
    const ImAxis yAxis = static_cast< ImAxis >(toImAxis(yAxisId));
    const ImPlotRect limits = ImPlot::GetPlotLimits(xAxis, yAxis);
    ImDrawList* drawList = ImPlot::GetPlotDrawList();
    if (!drawList) {
        return;
    }

    for (double value : values) {
        ImVec2 p1;
        ImVec2 p2;
        if (horizontal) {
            p1 = ImPlot::PlotToPixels(limits.X.Min, value, xAxis, yAxis);
            p2 = ImPlot::PlotToPixels(limits.X.Max, value, xAxis, yAxis);
        } else {
            p1 = ImPlot::PlotToPixels(value, limits.Y.Min, xAxis, yAxis);
            p2 = ImPlot::PlotToPixels(value, limits.Y.Max, xAxis, yAxis);
        }
        drawStyledSegment(drawList, p1, p2, style, width, color);
    }
}

double annotationCoordForVertical(
    QImPlotInfLinesItemNode::TextPosition position,
    double lineX,
    QImPlotAxisId xAxisId,
    QImPlotAxisId yAxisId)
{
    const ImAxis xAxis = static_cast< ImAxis >(toImAxis(xAxisId));
    const ImAxis yAxis = static_cast< ImAxis >(toImAxis(yAxisId));
    const ImPlotRect limits = ImPlot::GetPlotLimits(xAxis, yAxis);
    if (position == QImPlotInfLinesItemNode::TextPosition::Center) {
        return (limits.Y.Min + limits.Y.Max) * 0.5;
    }

    const ImVec2 plotPos = ImPlot::GetPlotPos();
    const ImVec2 plotSize = ImPlot::GetPlotSize();
    const float margin = 18.0f;
    const ImVec2 lineAnchor = ImPlot::PlotToPixels(lineX, limits.Y.Max, xAxis, yAxis);
    const float targetY = position == QImPlotInfLinesItemNode::TextPosition::Minimum ?
                              (plotPos.y + plotSize.y - margin) :
                              (plotPos.y + margin);
    return ImPlot::PixelsToPlot(lineAnchor.x, targetY, xAxis, yAxis).y;
}

double annotationCoordForHorizontal(
    QImPlotInfLinesItemNode::TextPosition position,
    double lineY,
    QImPlotAxisId xAxisId,
    QImPlotAxisId yAxisId)
{
    const ImAxis xAxis = static_cast< ImAxis >(toImAxis(xAxisId));
    const ImAxis yAxis = static_cast< ImAxis >(toImAxis(yAxisId));
    const ImPlotRect limits = ImPlot::GetPlotLimits(xAxis, yAxis);
    if (position == QImPlotInfLinesItemNode::TextPosition::Center) {
        return (limits.X.Min + limits.X.Max) * 0.5;
    }

    const ImVec2 plotPos = ImPlot::GetPlotPos();
    const ImVec2 plotSize = ImPlot::GetPlotSize();
    const float margin = 22.0f;
    const ImVec2 lineAnchor = ImPlot::PlotToPixels(limits.X.Min, lineY, xAxis, yAxis);
    const float targetX = position == QImPlotInfLinesItemNode::TextPosition::Minimum ?
                              (plotPos.x + margin) :
                              (plotPos.x + plotSize.x - margin);
    return ImPlot::PixelsToPlot(targetX, lineAnchor.y, xAxis, yAxis).x;
}

ImVec2 textPixelOffset(QImPlotInfLinesItemNode::TextOffset offset, bool horizontal, float lineWidth)
{
    if (offset == QImPlotInfLinesItemNode::TextOffset::Center) {
        return ImVec2(0.0f, 0.0f);
    }

    const float distance = std::max(10.0f, 6.0f + lineWidth * 2.0f);
    if (horizontal) {
        return offset == QImPlotInfLinesItemNode::TextOffset::Negative ? ImVec2(0.0f, distance) : ImVec2(0.0f, -distance);
    }
    return offset == QImPlotInfLinesItemNode::TextOffset::Negative ? ImVec2(-distance, 0.0f) : ImVec2(distance, 0.0f);
}
}  // namespace

/**
 * \if ENGLISH
 * @brief Constructs an infinite lines plot item node.
 * @param[in] parent Parent QObject.
 * \endif
 *
 * \if CHINESE
 * @brief 构造一个无限线绘图项节点。
 * @param[in] parent 父QObject对象。
 * \endif
 */
QImPlotInfLinesItemNode::QImPlotInfLinesItemNode(QObject* parent)
    : QImPlotItemNode(parent), QIM_PIMPL_CONSTRUCT
{
}

/**
 * \if ENGLISH
 * @brief Destroys the infinite lines plot item node.
 * \endif
 *
 * \if CHINESE
 * @brief 销毁无限线绘图项节点。
 * \endif
 */
QImPlotInfLinesItemNode::~QImPlotInfLinesItemNode()
{
}

/**
 * \if ENGLISH
 * @brief Replaces the full coordinate list of infinite lines.
 * @param[in] values Coordinate list used by PlotInfLines.
 * \endif
 *
 * \if CHINESE
 * @brief 替换无限线的完整坐标列表。
 * @param[in] values 传递给PlotInfLines使用的坐标列表。
 * \endif
 */
void QImPlotInfLinesItemNode::setValues(const std::vector< double >& values)
{
    QIM_D(d);
    d->values = values;
    Q_EMIT valuesChanged();
}

/**
 * \if ENGLISH
 * @brief Replaces the full coordinate list of infinite lines with move semantics.
 * @param[in] values Coordinate list used by PlotInfLines.
 * \endif
 *
 * \if CHINESE
 * @brief 使用移动语义替换无限线的完整坐标列表。
 * @param[in] values 传递给PlotInfLines使用的坐标列表。
 * \endif
 */
void QImPlotInfLinesItemNode::setValues(std::vector< double >&& values)
{
    QIM_D(d);
    d->values = std::move(values);
    Q_EMIT valuesChanged();
}

/**
 * \if ENGLISH
 * @brief Returns the stored infinite line coordinates.
 * @return Internal coordinate list.
 * \endif
 *
 * \if CHINESE
 * @brief 返回当前存储的无限线坐标。
 * @return 内部坐标列表。
 * \endif
 */
const std::vector< double >& QImPlotInfLinesItemNode::values() const
{
    return d_ptr->values;
}

/**
 * \if ENGLISH
 * @brief Checks whether the infinite lines are rendered horizontally.
 * @return True if horizontal mode is enabled.
 * \endif
 *
 * \if CHINESE
 * @brief 检查无限线是否以水平方向渲染。
 * @return 启用水平模式时返回true。
 * \endif
 */
bool QImPlotInfLinesItemNode::isHorizontal() const
{
    QIM_DC(d);
    return (d->flags & ImPlotInfLinesFlags_Horizontal) != 0;
}

/**
 * \if ENGLISH
 * @brief Enables or disables horizontal mode.
 * @param[in] on True to render lines horizontally.
 * \endif
 *
 * \if CHINESE
 * @brief 启用或禁用水平模式。
 * @param[in] on 为true时按水平线渲染。
 * \endif
 */
void QImPlotInfLinesItemNode::setHorizontal(bool on)
{
    QIM_D(d);
    const ImPlotInfLinesFlags oldFlags = d->flags;
    if (on) {
        d->flags |= ImPlotInfLinesFlags_Horizontal;
    } else {
        d->flags &= ~ImPlotInfLinesFlags_Horizontal;
    }
    if (oldFlags != d->flags) {
        Q_EMIT infLinesFlagChanged();
    }
}

/**
 * \if ENGLISH
 * @brief Returns the raw ImPlotInfLinesFlags bitmask.
 * @return Current ImPlotInfLinesFlags value.
 * \endif
 *
 * \if CHINESE
 * @brief 返回原始ImPlotInfLinesFlags位掩码。
 * @return 当前的ImPlotInfLinesFlags值。
 * \endif
 */
int QImPlotInfLinesItemNode::infLinesFlags() const
{
    QIM_DC(d);
    return d->flags;
}

/**
 * \if ENGLISH
 * @brief Sets the raw ImPlotInfLinesFlags bitmask.
 * @param[in] flags New ImPlotInfLinesFlags value.
 * \endif
 *
 * \if CHINESE
 * @brief 设置原始ImPlotInfLinesFlags位掩码。
 * @param[in] flags 新的ImPlotInfLinesFlags值。
 * \endif
 */
void QImPlotInfLinesItemNode::setInfLinesFlags(int flags)
{
    QIM_D(d);
    if (d->flags != flags) {
        d->flags = static_cast< ImPlotInfLinesFlags >(flags);
        Q_EMIT infLinesFlagChanged();
    }
}

/**
 * \if ENGLISH
 * @brief Sets the explicit line color.
 * @param[in] c Qt color used for rendering.
 * \endif
 *
 * \if CHINESE
 * @brief 设置显式线条颜色。
 * @param[in] c 用于渲染的Qt颜色。
 * \endif
 */
void QImPlotInfLinesItemNode::setColor(const QColor& c)
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
        Q_EMIT colorChanged(c);
    }
}

/**
 * \if ENGLISH
 * @brief Returns the effective line color.
 * @return Explicit color if set; otherwise an invalid QColor.
 * \endif
 *
 * \if CHINESE
 * @brief 返回当前线条颜色。
 * @return 如果已设置显式颜色则返回该颜色，否则返回无效QColor。
 * \endif
 */
QColor QImPlotInfLinesItemNode::color() const
{
    return (d_ptr->color.has_value()) ? toQColor(d_ptr->color->value()) : QColor();
}

void QImPlotInfLinesItemNode::setLineStyle(int style)
{
    QIM_D(d);
    const Qt::PenStyle penStyle = static_cast< Qt::PenStyle >(style);
    if (d->lineStyle != penStyle) {
        d->lineStyle = penStyle;
        Q_EMIT lineStyleChanged(style);
    }
}

int QImPlotInfLinesItemNode::lineStyle() const
{
    return d_ptr->lineStyle;
}

void QImPlotInfLinesItemNode::setLineWidth(float width)
{
    QIM_D(d);
    d->lineWidth = width;
    if (d->lineWidth.is_dirty()) {
        Q_EMIT lineWidthChanged(width);
    }
}

float QImPlotInfLinesItemNode::lineWidth() const
{
    return d_ptr->lineWidth.value();
}

void QImPlotInfLinesItemNode::setText(const QString& text)
{
    QIM_D(d);
    const QByteArray utf8 = text.toUtf8();
    if (d->textUtf8 != utf8) {
        d->textUtf8 = utf8;
        Q_EMIT textChanged(text);
        Q_EMIT annotationTextChanged(text);
    }
}

QString QImPlotInfLinesItemNode::text() const
{
    return QString::fromUtf8(d_ptr->textUtf8);
}

void QImPlotInfLinesItemNode::setTextPosition(TextPosition position)
{
    QIM_D(d);
    if (d->textPosition != position) {
        d->textPosition = position;
        Q_EMIT textPositionChanged(position);
        Q_EMIT annotationPositionChanged(annotationPosition());
    }
}

QImPlotInfLinesItemNode::TextPosition QImPlotInfLinesItemNode::textPosition() const
{
    return d_ptr->textPosition;
}

void QImPlotInfLinesItemNode::setTextOffset(TextOffset offset)
{
    QIM_D(d);
    if (d->textOffset != offset) {
        d->textOffset = offset;
        Q_EMIT textOffsetChanged(offset);
        Q_EMIT annotationPositionChanged(annotationPosition());
    }
}

QImPlotInfLinesItemNode::TextOffset QImPlotInfLinesItemNode::textOffset() const
{
    return d_ptr->textOffset;
}

void QImPlotInfLinesItemNode::setAnnotationText(const QString& text)
{
    setText(text);
}

QString QImPlotInfLinesItemNode::annotationText() const
{
    return text();
}

void QImPlotInfLinesItemNode::setAnnotationPosition(AnnotationPosition position)
{
    QIM_D(d);
    const TextPosition newPosition = textPositionFromAnnotationPosition(position);
    const TextOffset newOffset = textOffsetFromAnnotationPosition(position);
    if (d->textPosition == newPosition && d->textOffset == newOffset) {
        return;
    }
    d->textPosition = newPosition;
    d->textOffset = newOffset;
    Q_EMIT textPositionChanged(d->textPosition);
    Q_EMIT textOffsetChanged(d->textOffset);
    Q_EMIT annotationPositionChanged(position);
}

QImPlotInfLinesItemNode::AnnotationPosition QImPlotInfLinesItemNode::annotationPosition() const
{
    return annotationPositionFromTextPlacement(d_ptr->textPosition, d_ptr->textOffset);
}

/**
 * \if ENGLISH
 * @brief Draws the infinite lines through ImPlot::PlotInfLines.
 * @return Always returns false because the item has no child scope to recurse into.
 * \endif
 *
 * \if CHINESE
 * @brief 通过ImPlot::PlotInfLines绘制无限线。
 * @return 始终返回false，因为该绘图项没有子作用域需要递归渲染。
 * \endif
 */
bool QImPlotInfLinesItemNode::beginDraw()
{
    QIM_D(d);
    if (d->values.empty()) {
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
    } else if (d->color && d->color->is_dirty()) {
        ImPlot::SetNextLineStyle(d->color->value());
        d->color->mark_clean();
    }

    ImPlot::PlotInfLines(labelConstData(),
                         d->values.data(),
                         static_cast< int >(d->values.size()),
                         d->flags,
                         0,
                         sizeof(double));

    ImPlotContext* ct = ImPlot::GetCurrentContext();
    ImPlotItem* plotItem = ct->PreviousItem;
    setImPlotItem(plotItem);
    if (plotItem && plotItem->Show != QImAbstractNode::isVisible()) {
        QImAbstractNode::setVisible(plotItem->Show);
    }
    if (!d->color) {
        d->color = ImPlot::GetLastItemColor();
        d->color->mark_clean();
    }

    if (customStyledLine) {
        ImPlot::PushPlotClipRect();
        drawCustomInfLines(d->values,
                           isHorizontal(),
                           d->lineStyle,
                           d->lineWidth.value(),
                           plotItem ? plotItem->Color : ImGui::ColorConvertFloat4ToU32(d->color->value()),
                           xAxisId(),
                           yAxisId());
        ImPlot::PopPlotClipRect();
    }

    if (!d->textUtf8.isEmpty()) {
        ImPlot::PushStyleColor(ImPlotCol_InlayText, d->color ? d->color->value() : ImPlot::GetLastItemColor());
        const bool horizontal = isHorizontal();
        const QImPlotAxisId currentXAxis = xAxisId();
        const QImPlotAxisId currentYAxis = yAxisId();
        const ImVec2 pixelOffset = textPixelOffset(d->textOffset, horizontal, d->lineWidth.value());
        for (double value : d->values) {
            if (horizontal) {
                const double textX = annotationCoordForHorizontal(d->textPosition, value, currentXAxis, currentYAxis);
                ImPlot::PlotText(d->textUtf8.constData(), textX, value, pixelOffset);
            } else {
                const double textY = annotationCoordForVertical(d->textPosition, value, currentXAxis, currentYAxis);
                ImPlot::PlotText(d->textUtf8.constData(), value, textY, pixelOffset, ImPlotTextFlags_Vertical);
            }
        }
        ImPlot::PopStyleColor();
    }
    return false;
}

}  // namespace QIM
