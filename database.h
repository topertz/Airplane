#ifndef DATABASE_H
#define DATABASE_H

#include <QWidget>
#include <QString>
#include <QSqlDatabase>
#include <QGraphicsScene>
#include "widget.h"
#include "passenger.h"
#include "draw.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Database : public QWidget
{
    Q_OBJECT

public:
    explicit Database(Widget *parentWidget, QWidget *parent = nullptr);
    ~Database();
    Ui::Widget *ui;
    struct SimulationStats {
        int onboardPassengers = 0;
        int onboardSteps = 0;
        int onboardWaits = 0;
        double onboardAvgWaitTime = 0.0;
        int disembarkedPassengers = 0;
        double disembarkedAvgYourney = 0.0;
        double disembarkedAvgWaitTime = 0.0;
        int totalTime = 0;
    };

public slots:
    void saveInitialStateToDatabase();
    void loadInitialStateFromDatabase();
    void showDatabaseTables(const QString &tableName1, const QString &tableName2);
    void saveInitialState();
    void loadInitialState();
    void findOptimalSeatingArrangement();
    int simulateEvacuation(const std::vector<std::vector<int>>& arrangement);
    void saveOptimalSeatingArrangementToDatabase();
    void loadOptimalSeatingArrangementFromDatabase();
    SimulationStats extractSimulationStats();
    void showOptimalDatabaseTable();
    void drawSimulationResult(const std::vector<std::vector<int>>& arrangement, int simulationIndex, bool isBestSimulation);

private slots:
    void initializeDatabase();

private:
    Widget *widget;
    Passenger *passenger;
    Draw *draw;
    QSqlDatabase db;
    int onboardPassengers = 0;
    int onboardSteps = 0;
    int onboardWaits = 0;
    double onboardAvgWaitTime = 0.0f;
    int disembarkedPassengers = 0;
    double disembarkedAvgYourney = 0.0f;
    double disembarkedAvgWaitTime = 0.0f;
    int totalTime = 0;
    int rowCount = 0;
    int colCount = 0;
    QList<QPoint>* passengers;
    int optimalTime = INT_MAX;
    std::vector<std::vector<int>> optimalArrangement;
    std::vector<std::vector<int>> selectedArrangement;
};

#endif // DATABASE_H
