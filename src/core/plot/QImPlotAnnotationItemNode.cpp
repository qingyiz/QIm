#include "QImPlotAnnotationItemNode.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include "QtImGuiUtils.h"
#include "imgui.h"
#include "implot.h"

namespace QIM
{
namespace
{

ImVec2 operator+(const ImVec2& a, const ImVec2& b)
{
    return ImVec2(a.x + b.x, a.y + b.y);
}

ImVec2 operator-(const ImVec2& a, const ImVec2& b)
{
    return ImVec2(a.x - b.x, a.y - b.y);
}

ImVec2 operator*(const ImVec2& a, float s)
{
    return ImVec2(a.x * s, a.y * s);
}

float dot(const ImVec2& a, const ImVec2& b)
{
    return a.x * b.x + a.y * b.y;
}

float length(const ImVec2& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

ImVec2 normalized(const ImVec2& v)
{
    const float len = length(v);
    if (len <= 0.0001f) {
        return ImVec2(1.0f, 0.0f);
    }
    return ImVec2(v.x / len, v.y / len);
}

ImVec2 perpendicular(const ImVec2& v)
{
    return ImVec2(-v.y, v.x);
}

std::vector< float > patternForPenStyle(Qt::PenStyle style, float width)
{
    const float unit = std::max(1.0f, width);
    switch (style) {
    case Qt::DashLine:
        return { 6.0f * unit, 4.0f * unit };
    case Qt::DotLine:
        return { 1.0f * unit, 3.0f * unit };
    case Qt::DashDotLine:
        return { 6.0f * unit, 3.0f * unit, 1.0f * unit, 3.0f * unit };
    case Qt::DashDotDotLine:
        return { 6.0f * unit, 3.0f * unit, 1.0f * unit, 3.0f * unit, 1.0f * unit, 3.0f * unit };
    default:
        break;
    }
    return {};
}

bool needsCustomLineRendering(Qt::PenStyle style)
{
    return style == Qt::DashLine || style == Qt::DotLine || style == Qt::DashDotLine || style == Qt::DashDotDotLine;
}

void drawStyledSegment(ImDrawList* drawList, const ImVec2& p1, const ImVec2& p2, Qt::PenStyle style, float width, ImU32 color)
{
    if (style == Qt::NoPen) {
        return;
    }
    if (!needsCustomLineRendering(style)) {
        drawList->AddLine(p1, p2, color, width);
        return;
    }

    const ImVec2 delta = p2 - p1;
    const float segmentLength = length(delta);
    if (segmentLength <= 0.0001f) {
        return;
    }

    const std::vector< float > pattern = patternForPenStyle(style, width);
    if (pattern.empty()) {
        drawList->AddLine(p1, p2, color, width);
        return;
    }

    const ImVec2 direction = normalized(delta);
    float offset = 0.0f;
    int patternIndex = 0;
    bool draw = true;
    while (offset < segmentLength) {
        const float next = std::min(offset + pattern[ patternIndex % pattern.size() ], segmentLength);
        if (draw) {
            drawList->AddLine(p1 + direction * offset, p1 + direction * next, color, width);
        }
        draw = !draw;
        offset = next;
        ++patternIndex;
    }
}

void drawStyledPolyline(ImDrawList* drawList, const std::vector< ImVec2 >& points, bool closed, Qt::PenStyle style, float width, ImU32 color)
{
    if (points.size() < 2 || style == Qt::NoPen) {
        return;
    }
    for (std::size_t i = 1; i < points.size(); ++i) {
        drawStyledSegment(drawList, points[ i - 1 ], points[ i ], style, width, color);
    }
    if (closed) {
        drawStyledSegment(drawList, points.back(), points.front(), style, width, color);
    }
}

void drawStyledCircle(ImDrawList* drawList, const ImVec2& center, float radius, Qt::PenStyle style, float width, ImU32 color)
{
    if (style == Qt::NoPen) {
        return;
    }
    if (!needsCustomLineRendering(style)) {
        drawList->AddCircle(center, radius, color, 48, width);
        return;
    }

    std::vector< ImVec2 > points;
    points.reserve(64);
    constexpr int segmentCount = 64;
    for (int i = 0; i < segmentCount; ++i) {
        constexpr float pi = 3.14159265358979323846f;
        const float angle = (static_cast< float >(i) / segmentCount) * 2.0f * pi;
        points.push_back(center + ImVec2(std::cos(angle) * radius, std::sin(angle) * radius));
    }
    drawStyledPolyline(drawList, points, true, style, width, color);
}

void drawArrowHead(ImDrawList* drawList, const ImVec2& tip, const ImVec2& tail, ImU32 color, float lineWidth)
{
    const ImVec2 dir = normalized(tip - tail);
    const ImVec2 side = perpendicular(dir);
    const float headLength = std::max(10.0f, lineWidth * 5.0f);
    const float headWidth = std::max(5.0f, lineWidth * 2.4f);
    const ImVec2 base = tip - dir * headLength;
    drawList->AddTriangleFilled(tip, base + side * headWidth, base - side * headWidth, color);
}

void drawArrow(ImDrawList* drawList,
               const ImVec2& from,
               const ImVec2& to,
               ImU32 color,
               Qt::PenStyle style,
               float lineWidth,
               bool startHead,
               bool endHead)
{
    drawStyledSegment(drawList, from, to, style, lineWidth, color);
    if (endHead) {
        drawArrowHead(drawList, to, from, color, lineWidth);
    }
    if (startHead) {
        drawArrowHead(drawList, from, to, color, lineWidth);
    }
}

void drawCenteredText(ImDrawList* drawList, const QString& text, const ImVec2& center, ImU32 color)
{
    if (text.isEmpty()) {
        return;
    }
    const QByteArray utf8 = text.toUtf8();
    const ImVec2 textSize = ImGui::CalcTextSize(utf8.constData());
    drawList->AddText(center - textSize * 0.5f + ImVec2(0.0f, -12.0f), color, utf8.constData());
}

ImRect centeredTextRect(const QString& text, const ImVec2& center)
{
    if (text.isEmpty()) {
        return ImRect();
    }
    const QByteArray utf8 = text.toUtf8();
    const ImVec2 textSize = ImGui::CalcTextSize(utf8.constData());
    const ImVec2 min = center - textSize * 0.5f + ImVec2(0.0f, -12.0f);
    return ImRect(min, min + textSize);
}

QPointF midpoint(const QPointF& a, const QPointF& b)
{
    return QPointF((a.x() + b.x()) * 0.5, (a.y() + b.y()) * 0.5);
}

float distanceToSegment(const ImVec2& point, const ImVec2& a, const ImVec2& b)
{
    const ImVec2 ab = b - a;
    const float lenSq = dot(ab, ab);
    if (lenSq <= 0.0001f) {
        return length(point - a);
    }
    const float t = std::clamp(dot(point - a, ab) / lenSq, 0.0f, 1.0f);
    return length(point - (a + ab * t));
}

bool pointInRect(const ImVec2& point, const ImVec2& a, const ImVec2& b)
{
    const float minX = std::min(a.x, b.x);
    const float maxX = std::max(a.x, b.x);
    const float minY = std::min(a.y, b.y);
    const float maxY = std::max(a.y, b.y);
    return point.x >= minX && point.x <= maxX && point.y >= minY && point.y <= maxY;
}

bool pointInTriangle(const ImVec2& point, const ImVec2& a, const ImVec2& b, const ImVec2& c)
{
    const float d1 = (point.x - b.x) * (a.y - b.y) - (a.x - b.x) * (point.y - b.y);
    const float d2 = (point.x - c.x) * (b.y - c.y) - (b.x - c.x) * (point.y - c.y);
    const float d3 = (point.x - a.x) * (c.y - a.y) - (c.x - a.x) * (point.y - a.y);
    const bool hasNeg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
    const bool hasPos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);
    return !(hasNeg && hasPos);
}

bool textHit(const QString& text, const ImVec2& center, const ImVec2& mouse)
{
    if (text.isEmpty()) {
        return false;
    }
    ImRect rect = centeredTextRect(text, center);
    rect.Expand(4.0f);
    return rect.Contains(mouse);
}

bool hitAnnotation(
    const QImPlotAnnotationItemNode::Annotation& annotation,
    const ImVec2& p1,
    const ImVec2& p2,
    const ImVec2& mouse,
    float lineWidth,
    bool filled
)
{
    const float tolerance = std::max(7.0f, lineWidth + 5.0f);
    const ImVec2 center = (p1 + p2) * 0.5f;
    switch (annotation.type) {
    case QImPlotAnnotationItemNode::AnnotationType::Circle: {
        const float radius = std::max(1.0f, length(p2 - p1));
        const float distance = length(mouse - p1);
        return std::abs(distance - radius) <= tolerance || (filled && distance <= radius) ||
               textHit(annotation.text, p1, mouse);
    }
    case QImPlotAnnotationItemNode::AnnotationType::Square:
        return distanceToSegment(mouse, p1, ImVec2(p2.x, p1.y)) <= tolerance ||
               distanceToSegment(mouse, ImVec2(p2.x, p1.y), p2) <= tolerance ||
               distanceToSegment(mouse, p2, ImVec2(p1.x, p2.y)) <= tolerance ||
               distanceToSegment(mouse, ImVec2(p1.x, p2.y), p1) <= tolerance ||
               (filled && pointInRect(mouse, p1, p2)) || textHit(annotation.text, center, mouse);
    case QImPlotAnnotationItemNode::AnnotationType::Triangle: {
        const ImVec2 dir = normalized(p2 - p1);
        const ImVec2 side = perpendicular(dir);
        const float radius = std::max(6.0f, length(p2 - p1));
        const ImVec2 top = p1 + dir * radius;
        const ImVec2 left = p1 - dir * (radius * 0.55f) + side * (radius * 0.9f);
        const ImVec2 right = p1 - dir * (radius * 0.55f) - side * (radius * 0.9f);
        return distanceToSegment(mouse, top, left) <= tolerance || distanceToSegment(mouse, left, right) <= tolerance ||
               distanceToSegment(mouse, right, top) <= tolerance || (filled && pointInTriangle(mouse, top, left, right)) ||
               textHit(annotation.text, p1, mouse);
    }
    case QImPlotAnnotationItemNode::AnnotationType::Arrow:
    case QImPlotAnnotationItemNode::AnnotationType::DoubleArrow:
        return distanceToSegment(mouse, p1, p2) <= tolerance;
    case QImPlotAnnotationItemNode::AnnotationType::TextArrow:
    case QImPlotAnnotationItemNode::AnnotationType::DoubleTextArrow:
        return distanceToSegment(mouse, p1, p2) <= tolerance || textHit(annotation.text, center, mouse);
    }
    return false;
}

bool dragPoint(int id, QPointF& point, const ImVec4& color, float size)
{
    double x = point.x();
    double y = point.y();
    const bool changed = ImPlot::DragPoint(id,
                                           &x,
                                           &y,
                                           color,
                                           size,
                                           ImPlotDragToolFlags_NoFit,
                                           nullptr,
                                           nullptr,
                                           nullptr);
    if (changed) {
        point = QPointF(x, y);
    }
    return changed;
}

}  // namespace

class QImPlotAnnotationItemNode::PrivateData
{
    QIM_DECLARE_PUBLIC(QImPlotAnnotationItemNode)
public:
    explicit PrivateData(QImPlotAnnotationItemNode* q) : q_ptr(q)
    {
    }

    QVector< Annotation > annotations;
    QColor color { 36, 110, 185 };
    QColor fillColor { 36, 110, 185, 45 };
    QColor textColor { 30, 30, 30 };
    Qt::PenStyle lineStyle { Qt::SolidLine };
    float lineWidth { 2.0f };
    float handleSize { 6.0f };
    bool editable { true };
    bool filled { false };
    int selectedAnnotationIndex { -1 };
};

QImPlotAnnotationItemNode::QImPlotAnnotationItemNode(QObject* parent)
    : QImPlotItemNode(parent), QIM_PIMPL_CONSTRUCT
{
    setLabel(QStringLiteral("annotations"));
}

QImPlotAnnotationItemNode::~QImPlotAnnotationItemNode()
{
}

void QImPlotAnnotationItemNode::setAnnotations(const QVector< Annotation >& annotations)
{
    QIM_D(d);
    d->annotations = annotations;
    if (d->selectedAnnotationIndex >= d->annotations.size()) {
        d->selectedAnnotationIndex = -1;
        Q_EMIT selectedAnnotationChanged(-1);
    }
    Q_EMIT annotationsChanged();
}

QVector< QImPlotAnnotationItemNode::Annotation > QImPlotAnnotationItemNode::annotations() const
{
    return d_ptr->annotations;
}

int QImPlotAnnotationItemNode::annotationCount() const
{
    return d_ptr->annotations.size();
}

QImPlotAnnotationItemNode::Annotation QImPlotAnnotationItemNode::annotationAt(int index) const
{
    if (index < 0 || index >= d_ptr->annotations.size()) {
        return {};
    }
    return d_ptr->annotations.at(index);
}

void QImPlotAnnotationItemNode::setAnnotationAt(int index, const Annotation& annotation)
{
    if (index < 0 || index >= d_ptr->annotations.size()) {
        return;
    }
    d_ptr->annotations[ index ] = annotation;
    Q_EMIT annotationEdited(index);
    Q_EMIT annotationsChanged();
}

int QImPlotAnnotationItemNode::addAnnotation(const Annotation& annotation)
{
    d_ptr->annotations.push_back(annotation);
    Q_EMIT annotationsChanged();
    return d_ptr->annotations.size() - 1;
}

void QImPlotAnnotationItemNode::clearAnnotations()
{
    if (d_ptr->annotations.isEmpty()) {
        return;
    }
    d_ptr->annotations.clear();
    if (d_ptr->selectedAnnotationIndex != -1) {
        d_ptr->selectedAnnotationIndex = -1;
        Q_EMIT selectedAnnotationChanged(-1);
    }
    Q_EMIT annotationsChanged();
}

int QImPlotAnnotationItemNode::addCircle(const QPointF& center, const QPointF& radiusHandle, const QString& text)
{
    return addAnnotation({ AnnotationType::Circle, center, radiusHandle, text });
}

int QImPlotAnnotationItemNode::addSquare(const QPointF& cornerA, const QPointF& cornerB, const QString& text)
{
    return addAnnotation({ AnnotationType::Square, cornerA, cornerB, text });
}

int QImPlotAnnotationItemNode::addTriangle(const QPointF& center, const QPointF& sizeHandle, const QString& text)
{
    return addAnnotation({ AnnotationType::Triangle, center, sizeHandle, text });
}

int QImPlotAnnotationItemNode::addArrow(const QPointF& from, const QPointF& to, const QString& text)
{
    return addAnnotation({ AnnotationType::Arrow, from, to, text });
}

int QImPlotAnnotationItemNode::addTextArrow(const QPointF& from, const QPointF& to, const QString& text)
{
    return addAnnotation({ AnnotationType::TextArrow, from, to, text });
}

int QImPlotAnnotationItemNode::addDoubleArrow(const QPointF& from, const QPointF& to, const QString& text)
{
    return addAnnotation({ AnnotationType::DoubleArrow, from, to, text });
}

int QImPlotAnnotationItemNode::addDoubleTextArrow(const QPointF& from, const QPointF& to, const QString& text)
{
    return addAnnotation({ AnnotationType::DoubleTextArrow, from, to, text });
}

void QImPlotAnnotationItemNode::setColor(const QColor& color)
{
    if (d_ptr->color != color) {
        d_ptr->color = color;
        Q_EMIT colorChanged(color);
    }
}

QColor QImPlotAnnotationItemNode::color() const
{
    return d_ptr->color;
}

void QImPlotAnnotationItemNode::setFillColor(const QColor& color)
{
    if (d_ptr->fillColor != color) {
        d_ptr->fillColor = color;
        Q_EMIT fillColorChanged(color);
    }
}

QColor QImPlotAnnotationItemNode::fillColor() const
{
    return d_ptr->fillColor;
}

void QImPlotAnnotationItemNode::setTextColor(const QColor& color)
{
    if (d_ptr->textColor != color) {
        d_ptr->textColor = color;
        Q_EMIT textColorChanged(color);
    }
}

QColor QImPlotAnnotationItemNode::textColor() const
{
    return d_ptr->textColor;
}

void QImPlotAnnotationItemNode::setLineStyle(int style)
{
    const Qt::PenStyle penStyle = static_cast< Qt::PenStyle >(style);
    if (d_ptr->lineStyle != penStyle) {
        d_ptr->lineStyle = penStyle;
        Q_EMIT lineStyleChanged(style);
    }
}

int QImPlotAnnotationItemNode::lineStyle() const
{
    return d_ptr->lineStyle;
}

void QImPlotAnnotationItemNode::setLineWidth(float width)
{
    width = std::max(0.5f, width);
    if (!fuzzyEqual(d_ptr->lineWidth, width)) {
        d_ptr->lineWidth = width;
        Q_EMIT lineWidthChanged(width);
    }
}

float QImPlotAnnotationItemNode::lineWidth() const
{
    return d_ptr->lineWidth;
}

void QImPlotAnnotationItemNode::setHandleSize(float size)
{
    size = std::max(2.0f, size);
    if (!fuzzyEqual(d_ptr->handleSize, size)) {
        d_ptr->handleSize = size;
        Q_EMIT handleSizeChanged(size);
    }
}

float QImPlotAnnotationItemNode::handleSize() const
{
    return d_ptr->handleSize;
}

void QImPlotAnnotationItemNode::setEditable(bool editable)
{
    if (d_ptr->editable != editable) {
        d_ptr->editable = editable;
        Q_EMIT editableChanged(editable);
    }
}

bool QImPlotAnnotationItemNode::isEditable() const
{
    return d_ptr->editable;
}

void QImPlotAnnotationItemNode::setFilled(bool filled)
{
    if (d_ptr->filled != filled) {
        d_ptr->filled = filled;
        Q_EMIT filledChanged(filled);
    }
}

bool QImPlotAnnotationItemNode::isFilled() const
{
    return d_ptr->filled;
}

void QImPlotAnnotationItemNode::setSelectedAnnotationIndex(int index)
{
    if (index < -1 || index >= d_ptr->annotations.size()) {
        index = -1;
    }
    if (d_ptr->selectedAnnotationIndex != index) {
        d_ptr->selectedAnnotationIndex = index;
        Q_EMIT selectedAnnotationChanged(index);
    }
}

int QImPlotAnnotationItemNode::selectedAnnotationIndex() const
{
    return d_ptr->selectedAnnotationIndex;
}

void QImPlotAnnotationItemNode::clearSelection()
{
    setSelectedAnnotationIndex(-1);
}

bool QImPlotAnnotationItemNode::beginDraw()
{
    QIM_D(d);
    if (d->annotations.isEmpty()) {
        return false;
    }

    ImDrawList* drawList = ImPlot::GetPlotDrawList();
    if (!drawList) {
        return false;
    }

    const ImAxis xAxis = static_cast< ImAxis >(toImAxis(xAxisId()));
    const ImAxis yAxis = static_cast< ImAxis >(toImAxis(yAxisId()));
    ImPlot::SetAxes(xAxis, yAxis);
    const ImU32 lineColor = toImU32(d->color);
    const ImU32 fillColor = toImU32(d->fillColor);
    const ImU32 textColor = toImU32(d->textColor);
    const ImVec4 handleColor = toImVec4(d->color);
    bool anyChanged = false;
    int clickedAnnotationIndex = -1;
    const bool shouldPick = d->editable && ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const ImVec2 mousePos = ImGui::GetIO().MousePos;

    for (int i = 0; i < d->annotations.size(); ++i) {
        Annotation& annotation = d->annotations[ i ];
        const ImVec2 p1 = ImPlot::PlotToPixels(annotation.p1.x(), annotation.p1.y(), xAxis, yAxis);
        const ImVec2 p2 = ImPlot::PlotToPixels(annotation.p2.x(), annotation.p2.y(), xAxis, yAxis);
        const ImVec2 center = (p1 + p2) * 0.5f;

        switch (annotation.type) {
        case AnnotationType::Circle: {
            const float radius = std::max(1.0f, length(p2 - p1));
            if (d->filled) {
                drawList->AddCircleFilled(p1, radius, fillColor, 48);
            }
            drawStyledCircle(drawList, p1, radius, d->lineStyle, d->lineWidth, lineColor);
            drawCenteredText(drawList, annotation.text, p1, textColor);
            break;
        }
        case AnnotationType::Square:
            if (d->filled) {
                drawList->AddRectFilled(p1, p2, fillColor);
            }
            drawStyledPolyline(drawList, { p1, ImVec2(p2.x, p1.y), p2, ImVec2(p1.x, p2.y) }, true, d->lineStyle, d->lineWidth, lineColor);
            drawCenteredText(drawList, annotation.text, center, textColor);
            break;
        case AnnotationType::Triangle: {
            const ImVec2 dir = normalized(p2 - p1);
            const ImVec2 side = perpendicular(dir);
            const float radius = std::max(6.0f, length(p2 - p1));
            const ImVec2 top = p1 + dir * radius;
            const ImVec2 left = p1 - dir * (radius * 0.55f) + side * (radius * 0.9f);
            const ImVec2 right = p1 - dir * (radius * 0.55f) - side * (radius * 0.9f);
            if (d->filled) {
                drawList->AddTriangleFilled(top, left, right, fillColor);
            }
            drawStyledPolyline(drawList, { top, left, right }, true, d->lineStyle, d->lineWidth, lineColor);
            drawCenteredText(drawList, annotation.text, p1, textColor);
            break;
        }
        case AnnotationType::Arrow:
            drawArrow(drawList, p1, p2, lineColor, d->lineStyle, d->lineWidth, false, true);
            break;
        case AnnotationType::TextArrow:
            drawArrow(drawList, p1, p2, lineColor, d->lineStyle, d->lineWidth, false, true);
            drawCenteredText(drawList, annotation.text, center, textColor);
            break;
        case AnnotationType::DoubleArrow:
            drawArrow(drawList, p1, p2, lineColor, d->lineStyle, d->lineWidth, true, true);
            break;
        case AnnotationType::DoubleTextArrow:
            drawArrow(drawList, p1, p2, lineColor, d->lineStyle, d->lineWidth, true, true);
            drawCenteredText(drawList, annotation.text, center, textColor);
            break;
        }

        if (shouldPick && hitAnnotation(annotation, p1, p2, mousePos, d->lineWidth, d->filled)) {
            clickedAnnotationIndex = i;
        }
    }

    if (shouldPick) {
        setSelectedAnnotationIndex(clickedAnnotationIndex);
    }

    if (d->editable && d->selectedAnnotationIndex >= 0 && d->selectedAnnotationIndex < d->annotations.size()) {
        const int i = d->selectedAnnotationIndex;
        Annotation& annotation = d->annotations[ i ];

        bool changed = false;
        const bool centerBased = annotation.type == AnnotationType::Circle || annotation.type == AnnotationType::Triangle;
        if (centerBased) {
            QPointF dragCenter = annotation.p1;
            if (dragPoint(i * 3 + 1, dragCenter, handleColor, d->handleSize)) {
                const QPointF delta = dragCenter - annotation.p1;
                annotation.p1 += delta;
                annotation.p2 += delta;
                changed = true;
            }
            changed = dragPoint(i * 3 + 2, annotation.p2, handleColor, d->handleSize) || changed;
        } else {
            QPointF oldCenter = midpoint(annotation.p1, annotation.p2);
            QPointF dragCenter = oldCenter;
            changed = dragPoint(i * 3 + 1, annotation.p1, handleColor, d->handleSize);
            changed = dragPoint(i * 3 + 2, annotation.p2, handleColor, d->handleSize) || changed;
            if (dragPoint(i * 3 + 3, dragCenter, handleColor, d->handleSize * 0.8f)) {
                const QPointF delta = dragCenter - oldCenter;
                annotation.p1 += delta;
                annotation.p2 += delta;
                changed = true;
            }
        }
        if (changed) {
            anyChanged = true;
            Q_EMIT annotationEdited(i);
        }
    }

    if (anyChanged) {
        Q_EMIT annotationsChanged();
    }
    return false;
}

}  // namespace QIM
