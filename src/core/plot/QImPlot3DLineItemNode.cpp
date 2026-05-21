#include "QImPlot3DLineItemNode.h"
#include "QtImGuiUtils.h"
#include "imgui.h"
#include "implot3d.h"
#include <algorithm>
#include <cmath>
#include <QPen>

namespace QIM
{
namespace
{
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

void drawStyledSegment(ImDrawList* drawList, const ImVec2& p1, const ImVec2& p2, Qt::PenStyle style, float width, ImU32 color)
{
    if (!drawList || style == Qt::NoPen) {
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

bool isFinitePoint(double x, double y, double z)
{
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}
}  // namespace

QImPlot3DLineItemNode::QImPlot3DLineItemNode(QObject* parent) : QImPlot3DItemNode(parent)
{
}

QImPlot3DLineItemNode::~QImPlot3DLineItemNode()
{
}

const std::vector< double >& QImPlot3DLineItemNode::xData() const
{
    return m_xData;
}

const std::vector< double >& QImPlot3DLineItemNode::yData() const
{
    return m_yData;
}

const std::vector< double >& QImPlot3DLineItemNode::zData() const
{
    return m_zData;
}

bool QImPlot3DLineItemNode::isSegments() const
{
    return (m_lineFlags & ImPlot3DLineFlags_Segments) != 0;
}

void QImPlot3DLineItemNode::setSegments(bool enabled)
{
    const int oldFlags = m_lineFlags;
    if (enabled) {
        m_lineFlags |= ImPlot3DLineFlags_Segments;
    } else {
        m_lineFlags &= ~ImPlot3DLineFlags_Segments;
    }
    if (m_lineFlags != oldFlags) {
        Q_EMIT lineFlagChanged();
    }
}

bool QImPlot3DLineItemNode::isLoop() const
{
    return (m_lineFlags & ImPlot3DLineFlags_Loop) != 0;
}

void QImPlot3DLineItemNode::setLoop(bool enabled)
{
    const int oldFlags = m_lineFlags;
    if (enabled) {
        m_lineFlags |= ImPlot3DLineFlags_Loop;
    } else {
        m_lineFlags &= ~ImPlot3DLineFlags_Loop;
    }
    if (m_lineFlags != oldFlags) {
        Q_EMIT lineFlagChanged();
    }
}

bool QImPlot3DLineItemNode::isSkipNaN() const
{
    return (m_lineFlags & ImPlot3DLineFlags_SkipNaN) != 0;
}

void QImPlot3DLineItemNode::setSkipNaN(bool enabled)
{
    const int oldFlags = m_lineFlags;
    if (enabled) {
        m_lineFlags |= ImPlot3DLineFlags_SkipNaN;
    } else {
        m_lineFlags &= ~ImPlot3DLineFlags_SkipNaN;
    }
    if (m_lineFlags != oldFlags) {
        Q_EMIT lineFlagChanged();
    }
}

int QImPlot3DLineItemNode::lineFlags() const
{
    return m_lineFlags;
}

void QImPlot3DLineItemNode::setLineFlags(int flags)
{
    if (m_lineFlags != flags) {
        m_lineFlags = flags;
        Q_EMIT lineFlagChanged();
    }
}

void QImPlot3DLineItemNode::setColor(const QColor& color)
{
    if (m_color != color) {
        m_color = color;
        Q_EMIT colorChanged(color);
    }
}

QColor QImPlot3DLineItemNode::color() const
{
    return m_color;
}

void QImPlot3DLineItemNode::setLineStyle(int style)
{
    if (m_lineStyle != style) {
        m_lineStyle = style;
        Q_EMIT lineStyleChanged(style);
    }
}

int QImPlot3DLineItemNode::lineStyle() const
{
    return m_lineStyle;
}

void QImPlot3DLineItemNode::setLineWidth(float width)
{
    if (!qFuzzyCompare(m_lineWidth, width)) {
        m_lineWidth = width;
        Q_EMIT lineWidthChanged(width);
    }
}

float QImPlot3DLineItemNode::lineWidth() const
{
    return m_lineWidth;
}

void QImPlot3DLineItemNode::setMarkerShape(int shape)
{
    if (m_markerShape != shape) {
        m_markerShape = shape;
        Q_EMIT markerShapeChanged(shape);
    }
}

int QImPlot3DLineItemNode::markerShape() const
{
    return m_markerShape;
}

void QImPlot3DLineItemNode::setMarkerSize(float size)
{
    if (!qFuzzyCompare(m_markerSize, size)) {
        m_markerSize = size;
        Q_EMIT markerSizeChanged(size);
    }
}

float QImPlot3DLineItemNode::markerSize() const
{
    return m_markerSize;
}

void QImPlot3DLineItemNode::setMarkerWeight(float weight)
{
    if (!qFuzzyCompare(m_markerWeight, weight)) {
        m_markerWeight = weight;
        Q_EMIT markerWeightChanged(weight);
    }
}

float QImPlot3DLineItemNode::markerWeight() const
{
    return m_markerWeight;
}

void QImPlot3DLineItemNode::setMarkerFillColor(const QColor& color)
{
    if (m_markerFillColor != color) {
        m_markerFillColor = color;
        Q_EMIT markerFillColorChanged(color);
    }
}

QColor QImPlot3DLineItemNode::markerFillColor() const
{
    return m_markerFillColor;
}

void QImPlot3DLineItemNode::setMarkerOutlineColor(const QColor& color)
{
    if (m_markerOutlineColor != color) {
        m_markerOutlineColor = color;
        Q_EMIT markerOutlineColorChanged(color);
    }
}

QColor QImPlot3DLineItemNode::markerOutlineColor() const
{
    return m_markerOutlineColor;
}

bool QImPlot3DLineItemNode::beginDraw()
{
    const int count = static_cast< int >(std::min({ m_xData.size(), m_yData.size(), m_zData.size() }));
    if (count <= 0) {
        return false;
    }

    const Qt::PenStyle style = static_cast< Qt::PenStyle >(m_lineStyle);
    const bool customLineStyle = style != Qt::SolidLine;
    ImVec4 lineColor = m_color.isValid() ? toImVec4(m_color) : IMPLOT3D_AUTO_COL;
    if (customLineStyle && !m_color.isValid()) {
        lineColor = ImPlot3D::NextColormapColor();
    }
    if (customLineStyle) {
        ImPlot3D::SetNextLineStyle(lineColor, 0.0f);
    } else {
        ImPlot3D::SetNextLineStyle(lineColor, m_lineWidth);
    }

    ImPlot3D::SetNextMarkerStyle(
        static_cast< ImPlot3DMarker >(m_markerShape),
        m_markerSize,
        m_markerFillColor.isValid() ? toImVec4(m_markerFillColor) : IMPLOT3D_AUTO_COL,
        m_markerWeight,
        m_markerOutlineColor.isValid() ? toImVec4(m_markerOutlineColor) : IMPLOT3D_AUTO_COL
    );

    ImPlot3D::PlotLine(
        labelConstData(),
        m_xData.data(),
        m_yData.data(),
        m_zData.data(),
        count,
        static_cast< ImPlot3DLineFlags >(m_lineFlags)
    );

    if (customLineStyle && style != Qt::NoPen) {
        ImDrawList* drawList = ImPlot3D::GetPlotDrawList();
        if (!drawList) {
            return false;
        }

        ImU32 drawColor = IM_COL32(255, 255, 255, 255);
        drawColor = ImGui::GetColorU32(lineColor);

        const bool segments = (m_lineFlags & ImPlot3DLineFlags_Segments) != 0;
        const bool loop = (m_lineFlags & ImPlot3DLineFlags_Loop) != 0;
        const bool skipNaN = (m_lineFlags & ImPlot3DLineFlags_SkipNaN) != 0;
        bool hasPrevious = false;
        ImPlot3DPoint previousPoint;
        ImPlot3DPoint firstPoint;
        bool hasFirst = false;
        ImPlot3DPoint lastPoint;
        bool hasLast = false;

        auto flushInvalid = [&]() {
            if (!skipNaN) {
                hasPrevious = false;
            }
        };

        for (int i = 0; i < count; ++i) {
            const double x = m_xData[ i ];
            const double y = m_yData[ i ];
            const double z = m_zData[ i ];
            if (!isFinitePoint(x, y, z)) {
                flushInvalid();
                continue;
            }

            const ImPlot3DPoint point(x, y, z);
            if (!hasFirst) {
                firstPoint = point;
                hasFirst = true;
            }
            if (hasPrevious) {
                drawStyledSegment(drawList,
                                  ImPlot3D::PlotToPixels(previousPoint),
                                  ImPlot3D::PlotToPixels(point),
                                  style,
                                  m_lineWidth,
                                  drawColor);
                if (segments) {
                    hasPrevious = false;
                    hasLast = false;
                    continue;
                }
            }
            previousPoint = point;
            lastPoint = point;
            hasPrevious = true;
            hasLast = true;
        }

        if (loop && hasFirst && hasLast) {
            drawStyledSegment(drawList,
                              ImPlot3D::PlotToPixels(lastPoint),
                              ImPlot3D::PlotToPixels(firstPoint),
                              style,
                              m_lineWidth,
                              drawColor);
        }
    }
    return false;
}

void QImPlot3DLineItemNode::trimDataToCommonSize()
{
    const std::size_t commonSize = std::min({ m_xData.size(), m_yData.size(), m_zData.size() });
    m_xData.resize(commonSize);
    m_yData.resize(commonSize);
    m_zData.resize(commonSize);
}
}  // namespace QIM
