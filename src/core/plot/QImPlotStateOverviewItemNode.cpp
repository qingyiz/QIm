#include "QImPlotStateOverviewItemNode.h"

#include <algorithm>
#include <cmath>
#include <set>
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

float segmentLength(const ImVec2& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
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

void drawStyledSegment(ImDrawList* drawList, const ImVec2& p1, const ImVec2& p2, Qt::PenStyle style, float width, ImU32 color)
{
    if (style == Qt::NoPen) {
        return;
    }

    const ImVec2 delta = p2 - p1;
    const float length = segmentLength(delta);
    if (length <= 0.0001f) {
        return;
    }

    const std::vector< float > pattern = patternForPenStyle(style, width);
    if (pattern.empty()) {
        drawList->AddLine(p1, p2, color, width);
        return;
    }

    const ImVec2 direction(delta.x / length, delta.y / length);
    float offset = 0.0f;
    int patternIndex = 0;
    bool draw = true;
    while (offset < length) {
        const float next = std::min(offset + pattern[ patternIndex % pattern.size() ], length);
        if (draw) {
            drawList->AddLine(p1 + direction * offset, p1 + direction * next, color, width);
        }
        draw = !draw;
        offset = next;
        ++patternIndex;
    }
}

void drawCenteredText(ImDrawList* drawList, const QString& text, const ImVec2& center, ImU32 color)
{
    if (text.isEmpty()) {
        return;
    }
    const QByteArray utf8 = text.toUtf8();
    const ImVec2 size = ImGui::CalcTextSize(utf8.constData());
    drawList->AddText(center - size * 0.5f, color, utf8.constData());
}

}  // namespace

class QImPlotStateOverviewItemNode::PrivateData
{
    QIM_DECLARE_PUBLIC(QImPlotStateOverviewItemNode)
public:
    explicit PrivateData(QImPlotStateOverviewItemNode* q) : q_ptr(q)
    {
    }

    QVector< Segment > segments;
    QVector< CaseLabel > caseLabels;
    double stableValue { 1.0 };
    double unstableValue { 0.0 };
    QColor stableColor { 0, 119, 190 };
    QColor unstableColor { 0, 119, 190 };
    QColor boundaryColor { 190, 190, 190 };
    QColor labelColor { 55, 65, 85 };
    QColor caseLabelFillColor { 255, 255, 255, 235 };
    QColor caseLabelBorderColor { 190, 190, 190 };
    float lineWidth { 2.0f };
    Qt::PenStyle stableLineStyle { Qt::SolidLine };
    Qt::PenStyle unstableLineStyle { Qt::DashLine };
    Qt::PenStyle boundaryLineStyle { Qt::DashLine };
    bool boundaryLinesVisible { true };
    bool stateLabelsVisible { true };
    bool caseLabelsVisible { true };
};

QImPlotStateOverviewItemNode::QImPlotStateOverviewItemNode(QObject* parent)
    : QImPlotItemNode(parent), QIM_PIMPL_CONSTRUCT
{
    setLabel(QStringLiteral("state overview"));
}

QImPlotStateOverviewItemNode::~QImPlotStateOverviewItemNode()
{
}

void QImPlotStateOverviewItemNode::setSegments(const QVector< Segment >& segments)
{
    d_ptr->segments = segments;
    Q_EMIT segmentsChanged();
}

QVector< QImPlotStateOverviewItemNode::Segment > QImPlotStateOverviewItemNode::segments() const
{
    return d_ptr->segments;
}

int QImPlotStateOverviewItemNode::addSegment(const Segment& segment)
{
    d_ptr->segments.push_back(segment);
    Q_EMIT segmentsChanged();
    return d_ptr->segments.size() - 1;
}

int QImPlotStateOverviewItemNode::addSegment(double x0, double x1, State state)
{
    Segment segment;
    segment.x0 = x0;
    segment.x1 = x1;
    segment.state = state;
    return addSegment(segment);
}

int QImPlotStateOverviewItemNode::addStableSegment(double x0, double x1)
{
    return addSegment(x0, x1, State::Stable);
}

int QImPlotStateOverviewItemNode::addUnstableSegment(double x0, double x1)
{
    return addSegment(x0, x1, State::Unstable);
}

void QImPlotStateOverviewItemNode::clearSegments()
{
    if (d_ptr->segments.isEmpty()) {
        return;
    }
    d_ptr->segments.clear();
    Q_EMIT segmentsChanged();
}

void QImPlotStateOverviewItemNode::setCaseLabels(const QVector< CaseLabel >& labels)
{
    d_ptr->caseLabels = labels;
    Q_EMIT caseLabelsChanged();
}

QVector< QImPlotStateOverviewItemNode::CaseLabel > QImPlotStateOverviewItemNode::caseLabels() const
{
    return d_ptr->caseLabels;
}

int QImPlotStateOverviewItemNode::addCaseLabel(double x0, double x1, const QString& text)
{
    d_ptr->caseLabels.push_back({ x0, x1, text });
    Q_EMIT caseLabelsChanged();
    return d_ptr->caseLabels.size() - 1;
}

void QImPlotStateOverviewItemNode::clearCaseLabels()
{
    if (d_ptr->caseLabels.isEmpty()) {
        return;
    }
    d_ptr->caseLabels.clear();
    Q_EMIT caseLabelsChanged();
}

void QImPlotStateOverviewItemNode::setStableValue(double value)
{
    if (!fuzzyEqual(d_ptr->stableValue, value)) {
        d_ptr->stableValue = value;
        Q_EMIT geometryChanged();
    }
}

double QImPlotStateOverviewItemNode::stableValue() const
{
    return d_ptr->stableValue;
}

void QImPlotStateOverviewItemNode::setUnstableValue(double value)
{
    if (!fuzzyEqual(d_ptr->unstableValue, value)) {
        d_ptr->unstableValue = value;
        Q_EMIT geometryChanged();
    }
}

double QImPlotStateOverviewItemNode::unstableValue() const
{
    return d_ptr->unstableValue;
}

void QImPlotStateOverviewItemNode::setStableColor(const QColor& color)
{
    if (d_ptr->stableColor != color) {
        d_ptr->stableColor = color;
        Q_EMIT stableColorChanged(color);
    }
}

QColor QImPlotStateOverviewItemNode::stableColor() const
{
    return d_ptr->stableColor;
}

void QImPlotStateOverviewItemNode::setUnstableColor(const QColor& color)
{
    if (d_ptr->unstableColor != color) {
        d_ptr->unstableColor = color;
        Q_EMIT unstableColorChanged(color);
    }
}

QColor QImPlotStateOverviewItemNode::unstableColor() const
{
    return d_ptr->unstableColor;
}

void QImPlotStateOverviewItemNode::setBoundaryColor(const QColor& color)
{
    if (d_ptr->boundaryColor != color) {
        d_ptr->boundaryColor = color;
        Q_EMIT boundaryColorChanged(color);
    }
}

QColor QImPlotStateOverviewItemNode::boundaryColor() const
{
    return d_ptr->boundaryColor;
}

void QImPlotStateOverviewItemNode::setLabelColor(const QColor& color)
{
    if (d_ptr->labelColor != color) {
        d_ptr->labelColor = color;
        Q_EMIT labelColorChanged(color);
    }
}

QColor QImPlotStateOverviewItemNode::labelColor() const
{
    return d_ptr->labelColor;
}

void QImPlotStateOverviewItemNode::setCaseLabelFillColor(const QColor& color)
{
    if (d_ptr->caseLabelFillColor != color) {
        d_ptr->caseLabelFillColor = color;
        Q_EMIT caseLabelFillColorChanged(color);
    }
}

QColor QImPlotStateOverviewItemNode::caseLabelFillColor() const
{
    return d_ptr->caseLabelFillColor;
}

void QImPlotStateOverviewItemNode::setCaseLabelBorderColor(const QColor& color)
{
    if (d_ptr->caseLabelBorderColor != color) {
        d_ptr->caseLabelBorderColor = color;
        Q_EMIT caseLabelBorderColorChanged(color);
    }
}

QColor QImPlotStateOverviewItemNode::caseLabelBorderColor() const
{
    return d_ptr->caseLabelBorderColor;
}

void QImPlotStateOverviewItemNode::setLineWidth(float width)
{
    width = std::max(0.5f, width);
    if (!fuzzyEqual(d_ptr->lineWidth, width)) {
        d_ptr->lineWidth = width;
        Q_EMIT lineWidthChanged(width);
    }
}

float QImPlotStateOverviewItemNode::lineWidth() const
{
    return d_ptr->lineWidth;
}

void QImPlotStateOverviewItemNode::setStableLineStyle(int style)
{
    const Qt::PenStyle penStyle = static_cast< Qt::PenStyle >(style);
    if (d_ptr->stableLineStyle != penStyle) {
        d_ptr->stableLineStyle = penStyle;
        Q_EMIT lineStyleChanged();
    }
}

int QImPlotStateOverviewItemNode::stableLineStyle() const
{
    return d_ptr->stableLineStyle;
}

void QImPlotStateOverviewItemNode::setUnstableLineStyle(int style)
{
    const Qt::PenStyle penStyle = static_cast< Qt::PenStyle >(style);
    if (d_ptr->unstableLineStyle != penStyle) {
        d_ptr->unstableLineStyle = penStyle;
        Q_EMIT lineStyleChanged();
    }
}

int QImPlotStateOverviewItemNode::unstableLineStyle() const
{
    return d_ptr->unstableLineStyle;
}

void QImPlotStateOverviewItemNode::setBoundaryLineStyle(int style)
{
    const Qt::PenStyle penStyle = static_cast< Qt::PenStyle >(style);
    if (d_ptr->boundaryLineStyle != penStyle) {
        d_ptr->boundaryLineStyle = penStyle;
        Q_EMIT lineStyleChanged();
    }
}

int QImPlotStateOverviewItemNode::boundaryLineStyle() const
{
    return d_ptr->boundaryLineStyle;
}

void QImPlotStateOverviewItemNode::setBoundaryLinesVisible(bool visible)
{
    if (d_ptr->boundaryLinesVisible != visible) {
        d_ptr->boundaryLinesVisible = visible;
        Q_EMIT boundaryLinesVisibilityChanged(visible);
    }
}

bool QImPlotStateOverviewItemNode::areBoundaryLinesVisible() const
{
    return d_ptr->boundaryLinesVisible;
}

void QImPlotStateOverviewItemNode::setStateLabelsVisible(bool visible)
{
    if (d_ptr->stateLabelsVisible != visible) {
        d_ptr->stateLabelsVisible = visible;
        Q_EMIT stateLabelsVisibilityChanged(visible);
    }
}

bool QImPlotStateOverviewItemNode::areStateLabelsVisible() const
{
    return d_ptr->stateLabelsVisible;
}

void QImPlotStateOverviewItemNode::setCaseLabelsVisible(bool visible)
{
    if (d_ptr->caseLabelsVisible != visible) {
        d_ptr->caseLabelsVisible = visible;
        Q_EMIT caseLabelsVisibilityChanged(visible);
    }
}

bool QImPlotStateOverviewItemNode::areCaseLabelsVisible() const
{
    return d_ptr->caseLabelsVisible;
}

bool QImPlotStateOverviewItemNode::beginDraw()
{
    QIM_D(d);
    if (d->segments.isEmpty()) {
        return false;
    }

    ImDrawList* drawList = ImPlot::GetPlotDrawList();
    if (!drawList) {
        return false;
    }

    const ImAxis xAxis = static_cast< ImAxis >(toImAxis(xAxisId()));
    const ImAxis yAxis = static_cast< ImAxis >(toImAxis(yAxisId()));
    const ImPlotRect limits = ImPlot::GetPlotLimits(xAxis, yAxis);
    const ImVec2 plotPos = ImPlot::GetPlotPos();
    const ImVec2 plotSize = ImPlot::GetPlotSize();
    const ImU32 stableColor = toImU32(d->stableColor);
    const ImU32 unstableColor = toImU32(d->unstableColor);
    const ImU32 boundaryColor = toImU32(d->boundaryColor);
    const ImU32 labelColor = toImU32(d->labelColor);

    std::set< double > boundaries;
    ImPlot::PushPlotClipRect();
    for (const Segment& segment : std::as_const(d->segments)) {
        const double x0 = std::min(segment.x0, segment.x1);
        const double x1 = std::max(segment.x0, segment.x1);
        const double y = segment.state == State::Stable ? d->stableValue : d->unstableValue;
        const QColor color = segment.color.isValid() ? segment.color :
                             segment.state == State::Stable ? d->stableColor :
                                                              d->unstableColor;
        const Qt::PenStyle effectiveStyle = segment.lineStyle >= 0 ?
                                                static_cast< Qt::PenStyle >(segment.lineStyle) :
                                            segment.state == State::Stable ?
                                                d->stableLineStyle :
                                                d->unstableLineStyle;
        const ImVec2 p0 = ImPlot::PlotToPixels(x0, y, xAxis, yAxis);
        const ImVec2 p1 = ImPlot::PlotToPixels(x1, y, xAxis, yAxis);
        drawStyledSegment(drawList, p0, p1, effectiveStyle, d->lineWidth, toImU32(color));
        boundaries.insert(x0);
        boundaries.insert(x1);
    }

    if (d->boundaryLinesVisible) {
        for (double x : boundaries) {
            const ImVec2 p0 = ImPlot::PlotToPixels(x, limits.Y.Min, xAxis, yAxis);
            const ImVec2 p1 = ImPlot::PlotToPixels(x, limits.Y.Max, xAxis, yAxis);
            drawStyledSegment(drawList, p0, p1, d->boundaryLineStyle, 1.0f, boundaryColor);
        }
    }
    ImPlot::PopPlotClipRect();

    if (d->stateLabelsVisible) {
        const ImVec2 stablePixel = ImPlot::PlotToPixels(limits.X.Min, d->stableValue, xAxis, yAxis);
        const ImVec2 unstablePixel = ImPlot::PlotToPixels(limits.X.Min, d->unstableValue, xAxis, yAxis);
        drawList->AddText(ImVec2(plotPos.x + 8.0f, stablePixel.y - ImGui::GetTextLineHeight() * 0.5f), labelColor, "stable");
        drawList->AddText(ImVec2(plotPos.x + 8.0f, unstablePixel.y - ImGui::GetTextLineHeight() * 0.5f), labelColor, "unstable");
    }

    if (d->caseLabelsVisible) {
        const float labelHeight = 26.0f;
        const float labelY0 = plotPos.y + plotSize.y - labelHeight - 6.0f;
        const float labelY1 = labelY0 + labelHeight;
        const ImU32 fillColor = toImU32(d->caseLabelFillColor);
        const ImU32 borderColor = toImU32(d->caseLabelBorderColor);
        for (const CaseLabel& label : std::as_const(d->caseLabels)) {
            const double x0 = std::min(label.x0, label.x1);
            const double x1 = std::max(label.x0, label.x1);
            ImVec2 p0 = ImPlot::PlotToPixels(x0, d->unstableValue, xAxis, yAxis);
            ImVec2 p1 = ImPlot::PlotToPixels(x1, d->unstableValue, xAxis, yAxis);
            float minX = std::max(plotPos.x + 4.0f, std::min(p0.x, p1.x));
            float maxX = std::min(plotPos.x + plotSize.x - 4.0f, std::max(p0.x, p1.x));
            const QByteArray utf8 = label.text.toUtf8();
            const ImVec2 textSize = ImGui::CalcTextSize(utf8.constData());
            if (maxX - minX < textSize.x + 20.0f) {
                maxX = std::min(plotPos.x + plotSize.x - 4.0f, minX + textSize.x + 20.0f);
            }
            const ImVec2 rectMin(minX, labelY0);
            const ImVec2 rectMax(maxX, labelY1);
            drawList->AddRectFilled(rectMin, rectMax, fillColor);
            drawList->AddRect(rectMin, rectMax, borderColor);
            drawCenteredText(drawList, label.text, (rectMin + rectMax) * 0.5f, labelColor);
        }
    }

    Q_UNUSED(stableColor)
    Q_UNUSED(unstableColor)
    return false;
}

}  // namespace QIM
