#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QTimer>
#include <QList>
#include <QPoint>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QMap>
#include <QPainter>
#include <QTime>
#include <QTableView>

class Passenger;
class Database;
class Draw;

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    Ui::Widget *ui;
    QList<QPoint> passengers;
    int totalTime = 0;
    bool shouldDrawGrid = false;
    Passenger *passenger;

public slots:
    void paintEvent(QPaintEvent *event);

private slots:
    void clearTextbox();
    void spinboxChanged(int value);

private:
    Database  *database;
    Draw *draw;
    struct Circle {
        int row;
        int col;
        bool waiting;  // Ha true, akkor várakozik, ha false, akkor mozgásban van
    };
    QSqlDatabase db;
    QTimer* timer;
    QPoint closestExit;
    QList<Circle> circles;
    QList<Circle> removedCircles;
    int rowCount = 0;
    int colCount = 0;
    int passengerCount = 0;
    int optimalSteps = 0;
    QPoint exitPosition = QPoint(3, 0);
    QMap<int, int> passengerStartTime;
    std::vector<std::vector<int>> optimalArrangement;
    std::vector<std::vector<int>> seatingArrangement;
    int evacuationTime = 0;
    int totalWaitTime = 0;
    int totalPassengers = 0;
    QTime evacuationTimeStart;
    QMap<int, int> passengerWaits;
    int optimalTime = INT_MAX;
};
#endif // WIDGET_H
