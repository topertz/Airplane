#ifndef DRAW_H
#define DRAW_H

#include <QWidget>
#include <QPaintEvent>
#include <QPoint>
#include "passenger.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Draw : public QWidget
{
    Q_OBJECT

public:
    explicit Draw(Widget *parentWidget, QWidget *parent = nullptr);
    Ui::Widget *ui;
    bool shouldDrawGrid = false;

public slots:
    void drawPropeller(QPainter& painter, const QPoint& center, int radius);
    void drawArrow(QPainter& painter, const QPoint& start, const QPoint& end, int rectSize, int offsetX, int offsetY);
    void drawRearWings(QPainter& painter, int offsetX, int offsetY, int rowCount, int colCount, int rectSize);

private:
    Widget *widget;
    Passenger *passenger;
    int rowCount = 0;
    int colCount = 0;
    QList<QPoint> passengers;
};

#endif // DRAW_H
