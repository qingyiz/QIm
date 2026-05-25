#ifndef QIMPLOTSELECTIONOVERLAY_H
#define QIMPLOTSELECTIONOVERLAY_H

#include "imgui.h"
#include "imgui_internal.h"

namespace QIM
{
namespace detail
{
inline constexpr float plotSelectionCornerLength()
{
    return 16.0f;
}

inline constexpr float plotSelectionCornerThickness()
{
    return 3.0f;
}

inline ImU32 plotSelectionFillColor()
{
    return IM_COL32(64, 156, 255, 28);
}

inline ImU32 plotSelectionCornerColor()
{
    return IM_COL32(64, 156, 255, 255);
}

inline void drawPlotSelectionFill(ImDrawList* drawList, const ImRect& rect)
{
    if (!drawList) {
        return;
    }

    const ImVec2 min = rect.Min;
    const ImVec2 max = rect.Max;
    drawList->AddRectFilled(min, max, plotSelectionFillColor());
}

inline void drawPlotSelectionCorners(ImDrawList* drawList, const ImRect& rect)
{
    if (!drawList) {
        return;
    }

    const float length = plotSelectionCornerLength();
    const float thickness = plotSelectionCornerThickness();
    const ImU32 color = plotSelectionCornerColor();
    const ImVec2 min = rect.Min;
    const ImVec2 max = rect.Max;

    drawList->AddLine(min, ImVec2(min.x + length, min.y), color, thickness);
    drawList->AddLine(min, ImVec2(min.x, min.y + length), color, thickness);
    drawList->AddLine(ImVec2(max.x - length, min.y), ImVec2(max.x, min.y), color, thickness);
    drawList->AddLine(ImVec2(max.x, min.y), ImVec2(max.x, min.y + length), color, thickness);
    drawList->AddLine(ImVec2(min.x, max.y - length), ImVec2(min.x, max.y), color, thickness);
    drawList->AddLine(ImVec2(min.x, max.y), ImVec2(min.x + length, max.y), color, thickness);
    drawList->AddLine(ImVec2(max.x - length, max.y), max, color, thickness);
    drawList->AddLine(ImVec2(max.x, max.y - length), max, color, thickness);
}

inline void drawPlotSelectionOverlay(ImDrawList* drawList, const ImRect& rect)
{
    drawPlotSelectionFill(drawList, rect);
    drawPlotSelectionCorners(drawList, rect);
}
}  // namespace detail
}  // namespace QIM

#endif  // QIMPLOTSELECTIONOVERLAY_H
