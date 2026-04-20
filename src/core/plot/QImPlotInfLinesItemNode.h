#ifndef QIMPLOTINFLINESITEMNODE_H
#define QIMPLOTINFLINESITEMNODE_H
#include <QColor>
#include <QString>
#include <vector>
#include "QImPlotItemNode.h"

namespace QIM
{

/**
 * \if ENGLISH
 * @brief Qt-style wrapper for ImPlot infinite lines visualization
 *
 * @class QImPlotInfLinesItemNode
 * @ingroup plot_items
 *
 * @details Provides Qt-style retained mode encapsulation for ImPlot infinite lines.
 *          Infinite lines are axis-aligned guide lines extending across the whole plot
 *          area. The node manages a list of coordinate values and supports vertical
 *          and horizontal orientations through Qt properties and signal-slot updates.
 *
 * @note In vertical mode the values are interpreted on the current x-axis.
 *       In horizontal mode the values are interpreted on the current y-axis.
 *
 * @see QImPlotItemNode
 * @see QImPlotNode
 * \endif
 *
 * \if CHINESE
 * @brief ImPlot无限线的Qt风格封装
 *
 * @class QImPlotInfLinesItemNode
 * @ingroup plot_items
 *
 * @details 为ImPlot无限线提供Qt风格的保留模式封装。
 *          无限线是横贯整个绘图区的轴对齐参考线。
 *          该节点内部管理一组坐标值，并通过Qt属性和信号槽支持垂直/水平两种方向。
 *
 * @note 垂直模式下，数值作用于当前x轴。
 *       水平模式下，数值作用于当前y轴。
 *
 * @see QImPlotItemNode
 * @see QImPlotNode
 * \endif
 */
class QIM_CORE_API QImPlotInfLinesItemNode : public QImPlotItemNode
{
    Q_OBJECT
    QIM_DECLARE_PRIVATE(QImPlotInfLinesItemNode)

    Q_PROPERTY(bool horizontal READ isHorizontal WRITE setHorizontal NOTIFY infLinesFlagChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(int lineStyle READ lineStyle WRITE setLineStyle NOTIFY lineStyleChanged)
    Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(TextPosition textPosition READ textPosition WRITE setTextPosition NOTIFY textPositionChanged)
    Q_PROPERTY(TextOffset textOffset READ textOffset WRITE setTextOffset NOTIFY textOffsetChanged)

public:
    enum class TextPosition
    {
        Minimum,  ///< At the minimum coordinate of the current axis: left for horizontal lines, bottom for vertical lines.
        Center,   ///< At the middle of the line.
        Maximum   ///< At the maximum coordinate of the current axis: right for horizontal lines, top for vertical lines.
    };
    Q_ENUM(TextPosition)

    enum class TextOffset
    {
        Negative,  ///< Horizontal line: below. Vertical line: left.
        Center,    ///< Centered on the line.
        Positive   ///< Horizontal line: above. Vertical line: right.
    };
    Q_ENUM(TextOffset)

    enum class AnnotationPosition
    {
        StartLeft = 0,   ///< Near the line start, offset to the left side of the line.
        Start = 1,       ///< Near the line start, centered on the line.
        StartCenter = Start,
        StartRight = 2,  ///< Near the line start, offset to the right side of the line.
        CenterLeft = 3,  ///< Middle of the line, offset to the left side of the line.
        Center = 4,      ///< Middle of the line, centered on the line.
        CenterCenter = Center,
        CenterRight = 5, ///< Middle of the line, offset to the right side of the line.
        EndLeft = 6,     ///< Near the line end, offset to the left side of the line.
        End = 7,         ///< Near the line end, centered on the line.
        EndCenter = End,
        EndRight = 8     ///< Near the line end, offset to the right side of the line.
    };
    Q_ENUM(AnnotationPosition)

    enum
    {
        Type = InnerType + 8
    };

    virtual int type() const override
    {
        return Type;
    }

    // Constructs an infinite lines plot item node.
    explicit QImPlotInfLinesItemNode(QObject* parent = nullptr);

    // Destroys the infinite lines plot item node.
    ~QImPlotInfLinesItemNode();

    // Sets all line coordinate values.
    void setValues(const std::vector< double >& values);

    // Sets all line coordinate values with move semantics.
    void setValues(std::vector< double >&& values);

    // Sets all line coordinate values from a generic container.
    template< typename Container >
    void setValues(const Container& values);

    // Returns the stored coordinate values.
    const std::vector< double >& values() const;

    // Returns true when lines are horizontal.
    bool isHorizontal() const;

    // Enables or disables horizontal mode.
    void setHorizontal(bool on);

    // Returns the raw ImPlotInfLinesFlags value.
    int infLinesFlags() const;

    // Sets the raw ImPlotInfLinesFlags value.
    void setInfLinesFlags(int flags);

    // Sets the line color.
    void setColor(const QColor& c);

    // Returns the line color.
    QColor color() const;

    // Sets the line style.
    void setLineStyle(int style);

    // Returns the line style.
    int lineStyle() const;

    // Sets the line width.
    void setLineWidth(float width);

    // Returns the line width.
    float lineWidth() const;

    // Sets the text rendered on every infinite line.
    void setText(const QString& text);

    // Returns the text rendered on every infinite line.
    QString text() const;

    // Sets the text position along the line.
    void setTextPosition(TextPosition position);

    // Returns the text position along the line.
    TextPosition textPosition() const;

    // Sets the text offset side relative to the line.
    void setTextOffset(TextOffset offset);

    // Returns the text offset side relative to the line.
    TextOffset textOffset() const;

    // Compatibility wrapper for the old annotation text API.
    void setAnnotationText(const QString& text);

    // Compatibility wrapper for the old annotation text API.
    QString annotationText() const;

    // Compatibility wrapper for the old combined annotation position API.
    void setAnnotationPosition(AnnotationPosition position);

    // Compatibility wrapper for the old combined annotation position API.
    AnnotationPosition annotationPosition() const;

Q_SIGNALS:
    /**
     * \if ENGLISH
     * @brief Emitted when the stored line coordinates change.
     * \endif
     *
     * \if CHINESE
     * @brief 当无限线坐标数据发生变化时触发。
     * \endif
     */
    void valuesChanged();

    /**
     * \if ENGLISH
     * @brief Emitted when infinite line flags change.
     * \endif
     *
     * \if CHINESE
     * @brief 当无限线标志位发生变化时触发。
     * \endif
     */
    void infLinesFlagChanged();

    /**
     * \if ENGLISH
     * @brief Emitted when the line color changes.
     * @param color New line color.
     * \endif
     *
     * \if CHINESE
     * @brief 当线条颜色发生变化时触发。
     * @param color 新的线条颜色。
     * \endif
     */
    void colorChanged(const QColor& color);

    /**
     * \if ENGLISH
     * @brief Emitted when the line style changes.
     * @param style New line style.
     * \endif
     *
     * \if CHINESE
     * @brief 当线条样式发生变化时触发。
     * @param style 新的线条样式。
     * \endif
     */
    void lineStyleChanged(int style);

    /**
     * \if ENGLISH
     * @brief Emitted when the line width changes.
     * @param width New line width.
     * \endif
     *
     * \if CHINESE
     * @brief 当线条宽度发生变化时触发。
     * @param width 新的线条宽度。
     * \endif
     */
    void lineWidthChanged(float width);

    /**
     * \if ENGLISH
     * @brief Emitted when the infinite line text changes.
     * @param text New text.
     * \endif
     *
     * \if CHINESE
     * @brief 当无限线文本发生变化时触发。
     * @param text 新的文本。
     * \endif
     */
    void textChanged(const QString& text);

    /**
     * \if ENGLISH
     * @brief Emitted when the text position along the line changes.
     * @param position New text position.
     * \endif
     *
     * \if CHINESE
     * @brief 当无限线文本沿线位置发生变化时触发。
     * @param position 新的文本沿线位置。
     * \endif
     */
    void textPositionChanged(QIM::QImPlotInfLinesItemNode::TextPosition position);

    /**
     * \if ENGLISH
     * @brief Emitted when the text offset side changes.
     * @param offset New text offset side.
     * \endif
     *
     * \if CHINESE
     * @brief 当无限线文本偏移侧发生变化时触发。
     * @param offset 新的文本偏移侧。
     * \endif
     */
    void textOffsetChanged(QIM::QImPlotInfLinesItemNode::TextOffset offset);

    /**
     * \if ENGLISH
     * @brief Emitted when the annotation text changes.
     * @param text New annotation text.
     * \endif
     *
     * \if CHINESE
     * @brief 当无限线标注文本发生变化时触发。
     * @param text 新的标注文本。
     * \endif
     */
    void annotationTextChanged(const QString& text);

    /**
     * \if ENGLISH
     * @brief Emitted when the annotation position changes.
     * @param position New annotation position.
     * \endif
     *
     * \if CHINESE
     * @brief 当无限线标注位置发生变化时触发。
     * @param position 新的标注位置。
     * \endif
     */
    void annotationPositionChanged(QIM::QImPlotInfLinesItemNode::AnnotationPosition position);

protected:
    // Draws the infinite lines item.
    virtual bool beginDraw() override;
};

template< typename Container >
inline void QImPlotInfLinesItemNode::setValues(const Container& values)
{
    std::vector< double > copied;
    copied.reserve(static_cast< std::size_t >(values.size()));
    for (const auto& value : values) {
        copied.push_back(static_cast< double >(value));
    }
    setValues(std::move(copied));
}

}  // namespace QIM

#endif  // QIMPLOTINFLINESITEMNODE_H
