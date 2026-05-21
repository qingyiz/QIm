#ifndef QIMSUBPLOTS3DNODE_H
#define QIMSUBPLOTS3DNODE_H

#include "QImAbstractNode.h"
#include <QPoint>
#include <QPointF>
#include <QSizeF>
#include <vector>

namespace QIM
{
class QImPlot3DNode;

class QIM_CORE_API QImSubplots3DNode : public QImAbstractNode
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(int rows READ rows WRITE setRows NOTIFY gridInfoChanged)
    Q_PROPERTY(int cols READ columns WRITE setColumns NOTIFY gridInfoChanged)
    Q_PROPERTY(QSizeF size READ size WRITE setSize NOTIFY sizeChanged)

public:
    explicit QImSubplots3DNode(QObject* parent = nullptr);
    explicit QImSubplots3DNode(const QString& title, QObject* parent = nullptr);
    ~QImSubplots3DNode() override;

    QString title() const;
    void setTitle(const QString& title);

    int rows() const;
    void setRows(int rows);

    int columns() const;
    void setColumns(int columns);

    std::vector< float > rowRatios() const;
    void setRowRatios(const std::vector< float >& rowRatios);

    std::vector< float > columnRatios() const;
    void setColumnRatios(const std::vector< float >& columnRatios);

    void setGrid(int rows,
                 int cols,
                 const std::vector< float >& rowRatios = std::vector< float >(),
                 const std::vector< float >& columnRatios = std::vector< float >());

    QSizeF size() const;
    void setSize(const QSizeF& size);

    int gridCount() const;

    QImPlot3DNode* createPlotNode();
    QList< QImPlot3DNode* > plotNodes() const;
    int plotCount() const;
    bool takePlotNode(QImPlot3DNode* plot);
    void removePlotNode(QImPlot3DNode* plot);

Q_SIGNALS:
    void titleChanged(const QString& title);
    void gridInfoChanged();
    void sizeChanged(const QSizeF& size);

protected:
    bool beginDraw() override;
    void endDraw() override;

private:
    class CellNode;
    CellNode* createCellNode();
    QPoint cellPosition(int index) const;
    QSizeF cellSize(int index) const;
    std::vector< double > trackSizes(int count, double totalPixels, const std::vector< float >& ratios) const;

private:
    QByteArray m_titleUtf8;
    int m_rows { 1 };
    int m_cols { 1 };
    std::vector< float > m_rowRatios;
    std::vector< float > m_columnRatios;
    QSizeF m_size { -1.0, -1.0 };
    QPointF m_origin { 0.0, 0.0 };
    QSizeF m_availableSize { 0.0, 0.0 };
};
}  // namespace QIM

#endif  // QIMSUBPLOTS3DNODE_H
