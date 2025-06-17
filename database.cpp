#include "database.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QString>
#include <QSqlTableModel>
#include <QTableView>
#include <QSqlError>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSqlQuery>
#include <QFile>
#include <QFileDialog>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <thread>
#include <mutex>

Database::Database(Widget *parentWidget, QWidget *parent) : QWidget(parent), widget(parentWidget)
{
    widget->ui->textLoadingData->setReadOnly(true);
    rowCount = 10;
    colCount = 7;
    passenger = parentWidget->passenger;
    passengers = &parentWidget->passengers;
    std::vector<std::vector<int>> optimalArrangement(rowCount, std::vector<int>(colCount, 0));
    db = QSqlDatabase::addDatabase("QSQLITE");
    initializeDatabase();
}

Database::~Database()
{
    delete ui;
}

Database::SimulationStats Database::extractSimulationStats() {
    SimulationStats stats;

    // Onboard Passengers
    QString onboardPassengersText = widget->ui->onBoardPassengersLabel->text();
    static QRegularExpression onboardPassengersRegex("Onboard Passengers:\\s*(\\d+)");
    stats.onboardPassengers = onboardPassengersRegex.match(onboardPassengersText).captured(1).toInt();

    // Onboard Steps
    QString onboardStepsText = widget->ui->onBoardStepsLabel->text();
    static QRegularExpression onboardStepsRegex("Onboard Steps:\\s*(\\d+)");
    stats.onboardSteps = onboardStepsRegex.match(onboardStepsText).captured(1).toInt();

    // Onboard Waits
    QString onboardWaitsText = widget->ui->onBoardWaitsLabel->text();
    static QRegularExpression onboardWaitsRegex("Onboard Waits:\\s*(\\d+)");
    stats.onboardWaits = onboardWaitsRegex.match(onboardWaitsText).captured(1).toInt();

    // Onboard Avg Wait Time
    QString onboardAvgWaitTimeText = widget->ui->onBoardAverageWaitTimeLabel->text();
    static QRegularExpression onboardAvgWaitTimeRegex("Onboard Avg Wait Time:\\s*([\\d\\.]+)");
    stats.onboardAvgWaitTime = onboardAvgWaitTimeRegex.match(onboardAvgWaitTimeText).captured(1).toDouble();

    // Disembarked Passengers
    QString disembarkedPassengersText = widget->ui->disembarkedPassengersLabel->text();
    static QRegularExpression disembarkedPassengersRegex("Disembarked Passengers:\\s*(\\d+)");
    stats.disembarkedPassengers = disembarkedPassengersRegex.match(disembarkedPassengersText).captured(1).toInt();

    // Disembarked Avg Yourney
    QString disembarkedAvgYourneyText = widget->ui->disembarkedAvgYourneyLabel->text();
    static QRegularExpression disembarkedAvgYourneyRegex("Disembarked Avg Yourney:\\s*([\\d\\.]+)");
    stats.disembarkedAvgYourney = disembarkedAvgYourneyRegex.match(disembarkedAvgYourneyText).captured(1).toDouble();

    // Disembarked Avg Wait Time
    QString disembarkedAvgWaitTimeText = widget->ui->disembarkedAvgWaitTimeLabel->text();
    static QRegularExpression disembarkedAvgWaitTimeRegex("Disembarked Avg Wait Time:\\s*([\\d\\.]+)");
    stats.disembarkedAvgWaitTime = disembarkedAvgWaitTimeRegex.match(disembarkedAvgWaitTimeText).captured(1).toDouble();

    // Total Time
    QString totalTimeText = widget->ui->totalLabel->text();
    static QRegularExpression totalTimeRegex("Total Time:\\s*(\\d+)");
    stats.totalTime = totalTimeRegex.match(totalTimeText).captured(1).toInt();

    return stats;
}

void Database::saveInitialState() {
    // Open a file dialog for the user to select the file path and name
    QString dataFilePath = QFileDialog::getSaveFileName(this, "Save Initial State", "", "Text Files (*.txt)");

    if (dataFilePath.isEmpty()) {
        // If no file was selected, show a warning and return
        QMessageBox::warning(this, "Save Error", "No file selected for saving.");
        return;
    }

    // Adatok mentése
    QFile file(dataFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Save Error", "Unable to open file for saving.");
        return;
    }

    SimulationStats stats = extractSimulationStats();

    QDataStream out(&file);
    out << rowCount;
    out << colCount;
    out << passengers->size();
    for (QPoint point : *passengers) {
        out << point; // Mentés: x, y koordináták
    }
    out << stats.onboardPassengers;
    out << stats.onboardSteps;
    out << stats.onboardWaits;
    out << stats.onboardAvgWaitTime;
    out << stats.disembarkedPassengers;
    out << stats.disembarkedAvgYourney;
    out << stats.disembarkedAvgWaitTime;
    out << stats.totalTime;
    file.close();

    QMessageBox::information(this, "Save Successful", "Initial state saved successfully to TXT file.");
}

void Database::loadInitialState() {
    // Open a file dialog for the user to select the file to load
    QString dataFilePath = QFileDialog::getOpenFileName(this, "Load Initial State", "", "Text Files (*.txt)");

    if (dataFilePath.isEmpty()) {
        // If no file was selected, show a warning and return
        QMessageBox::warning(this, "Load Error", "No file selected to load.");
        return;
    }

    // We load the data
    QFile dataFile(dataFilePath);
    if (dataFile.open(QIODevice::ReadOnly)) {
        QDataStream in(&dataFile);
        in >> rowCount;
        in >> colCount;
        qsizetype q;
        in >> q;
        passengers->clear();
        QPoint point;
        for (int i=0;i<q;i++) {
            in >> point; // Save: x, y coordinates
            passengers->append(point);
        }
        in >> passenger->onboardPassengers;
        in >> passenger->onboardSteps;
        in >> passenger->onboardWaits;
        in >> passenger->onboardAvgWaitTime;
        in >> passenger->disembarkedPassengers;
        in >> passenger->disembarkedAvgYourney;
        in >> passenger->disembarkedAvgWaitTime;
        in >> passenger->totalTime;
        passenger->iterationCount = 0;
        //passenger->resetStatistics();
        // Prepare the data for the QTextEdit
        QString textData = "Loaded Initial State:\n\n";
        textData += QString("Row Count: %1\n").arg(rowCount);
        textData += QString("Column Count: %1\n").arg(colCount);
        textData += "\nStatistics:\n";
        textData += QString("Onboard Passengers: %1\n").arg(passenger->onboardPassengers);
        textData += QString("Onboard Steps: %1\n").arg(passenger->onboardSteps);
        textData += QString("Onboard Waits: %1\n").arg(passenger->onboardWaits);
        textData += QString("Onboard Avg Wait Time: %1\n").arg(passenger->onboardAvgWaitTime);
        textData += QString("Disembarked Passengers: %1\n").arg(passenger->disembarkedPassengers);
        textData += QString("Disembarked Avg Journey: %1\n").arg(passenger->disembarkedAvgYourney);
        textData += QString("Disembarked Avg Wait Time: %1\n").arg(passenger->disembarkedAvgWaitTime);
        textData += QString("Total Time: %1\n").arg(passenger->totalTime);
        // Set the loaded data to the QTextEdit
        widget->ui->textLoadingData->setText(textData);
        dataFile.close();

    } else {
        QMessageBox::warning(this, "Load Error", "Failed to load data file: " + dataFilePath);
    }
    widget->update();
    passenger->updateStatisticsLabels();

    QMessageBox::information(this, "Load Successful", "Initial state successfully loaded from TXT file.");
}

void Database::showDatabaseTables(const QString &tableName1, const QString &tableName2) {
    // Open the database file
    QString dbFilePath = QFileDialog::getOpenFileName(this, "Open Database", "", "SQLite Files (*.sqlite3)");
    if (dbFilePath.isEmpty()) {
        QMessageBox::warning(this, "Load Error", "No database file selected.");
        return;
    }

    db.setDatabaseName(dbFilePath);
    if (!db.isOpen()) {
        QMessageBox::warning(this, "Database Error", "Database connection is not open.");
        return;
    }

    // Models for both tables
    QSqlTableModel *model1 = new QSqlTableModel(this, db);
    model1->setTable(tableName1);
    model1->select();
    if (model1->lastError().isValid()) {
        QMessageBox::warning(this, "Database Error", "Failed to fetch data from " + tableName1 + ": " + model1->lastError().text());
        return;
    }

    QSqlTableModel *model2 = new QSqlTableModel(this, db);
    model2->setTable(tableName2);
    model2->select();
    if (model2->lastError().isValid()) {
        QMessageBox::warning(this, "Database Error", "Failed to fetch data from " + tableName2 + ": " + model2->lastError().text());
        return;
    }

    // Dialog
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Database Tables: " + tableName1 + " & " + tableName2);
    dialog->resize(1000, 500);

    // Table Views
    QTableView *tableView1 = new QTableView(dialog);
    tableView1->setModel(model1);
    tableView1->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView1->resizeColumnsToContents();

    QTableView *tableView2 = new QTableView(dialog);
    tableView2->setModel(model2);
    tableView2->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView2->resizeColumnsToContents();

    // Layouts
    QVBoxLayout *tableLayout = new QVBoxLayout();
    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

    tableLayout->addWidget(new QLabel("Table: " + tableName1, dialog));
    tableLayout->addWidget(tableView1);
    tableLayout->addWidget(new QLabel("Table: " + tableName2, dialog));
    tableLayout->addWidget(tableView2);

    mainLayout->addLayout(tableLayout);

    // Delete Button (Delete Selected Row)
    QPushButton *deleteButton = new QPushButton("Delete Selected Row(s)", dialog);
    connect(deleteButton, &QPushButton::clicked, this, [=]() {
        QModelIndexList selectedIndexes1 = tableView1->selectionModel()->selectedRows();
        QModelIndexList selectedIndexes2 = tableView2->selectionModel()->selectedRows();

        if (selectedIndexes1.isEmpty() && selectedIndexes2.isEmpty()) {
            QMessageBox::warning(dialog, "No Row Selected", "Please select a row to delete.");
        } else {
            // Confirm deletion
            if (QMessageBox::question(dialog, "Delete Row(s)", "Are you sure you want to delete the selected row(s)?") == QMessageBox::Yes) {
                // Delete selected rows from table1
                for (const QModelIndex &index : selectedIndexes1) {
                    if (!model1->removeRow(index.row())) {
                        QMessageBox::warning(dialog, "Delete Error", "Failed to delete row from " + tableName1 + ": " + model1->lastError().text());
                        return; // Stop if there's an error
                    }
                }
                // Delete selected rows from table2
                for (const QModelIndex &index : selectedIndexes2) {
                    if (!model2->removeRow(index.row())) {
                        QMessageBox::warning(dialog, "Delete Error", "Failed to delete row from " + tableName2 + ": " + model2->lastError().text());
                        return; // Stop if there's an error
                    }
                }
                model1->submitAll(); // Save changes for table1
                model2->submitAll(); // Save changes for table2
            }
        }
    });

    // Delete All Button
    QPushButton *deleteAllButton = new QPushButton("Delete All Rows", dialog);
    connect(deleteAllButton, &QPushButton::clicked, this, [=]() {
        int rowCount1 = model1->rowCount();
        int rowCount2 = model2->rowCount();

        if (rowCount1 > 0 || rowCount2 > 0) {
            if (QMessageBox::question(dialog, "Delete All Rows", "Are you sure you want to delete all rows?") == QMessageBox::Yes) {
                for (int i = rowCount1 - 1; i >= 0; --i) {
                    if (!model1->removeRow(i)) {
                        QMessageBox::warning(dialog, "Delete Error", "Failed to delete row from " + tableName1 + ": " + model1->lastError().text());
                    }
                }
                for (int i = rowCount2 - 1; i >= 0; --i) {
                    if (!model2->removeRow(i)) {
                        QMessageBox::warning(dialog, "Delete Error", "Failed to delete row from " + tableName2 + ": " + model2->lastError().text());
                    }
                }
                model1->submitAll(); // Save changes for table1
                model2->submitAll(); // Save changes for table2
            }
        } else {
            QMessageBox::information(dialog, "No Rows", "There are no rows to delete.");
        }
    });

    // Layouts
    tableLayout->addWidget(deleteButton);
    tableLayout->addWidget(deleteAllButton);

    mainLayout->addLayout(tableLayout);

    dialog->setLayout(mainLayout);
    dialog->exec();
}

void Database::initializeDatabase() {
    if (!db.open()) {
        QMessageBox::critical(this, "Database Error", "Failed to open database: " + db.lastError().text());
        return;
    }

    QSqlQuery query;
    // Creating tables

    // Create the 'onboard_state_stats' table if it doesn't exist
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS onboard_state_stats (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            onboard_passengers INTEGER,
            onboard_steps INTEGER,
            onboard_waits INTEGER,
            onboard_avg_wait_time REAL
        );
    )")) {
        qDebug() << "Failed to create onboard_state_stats table:" << query.lastError().text();
    }

    // Create the 'disembarked_state_stats' table if it doesn't exist
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS disembarked_state_stats (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            disembarked_passengers INTEGER,
            disembarked_avg_yourney REAL,
            disembarked_avg_wait_time REAL
        );
    )")) {
        qDebug() << "Failed to create disembarked_state_stats table:" << query.lastError().text();
    }

    // Create the 'passengers_positions' table if it doesn't exist
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS passengers_positions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            x INTEGER,
            y INTEGER
        );
    )")) {
        qDebug() << "Failed to create passengers_positions table:" << query.lastError().text();
    }

    qDebug() << "Database initialized successfully.";
}

void Database::saveInitialStateToDatabase() {
    // Let the user choose the database file path via a file dialog (sqlite3 file)
    QString dbFilePath = QFileDialog::getSaveFileName(this, "Save Database File", "", "SQLite Database Files (*.sqlite3)");

    if (dbFilePath.isEmpty()) {
        // If no file was selected, show a warning and return
        QMessageBox::warning(this, "Save Error", "No database file selected for saving.");
        return;
    }

    // Open the database connection (SQLite)
    db.setDatabaseName(dbFilePath);

    if (!db.open()) {
        QMessageBox::warning(this, "Database Error", "Failed to open the database for saving: " + db.lastError().text());
        return;
    }

    // Create tables if they don't exist
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS onboard_state_stats (id INTEGER PRIMARY KEY AUTOINCREMENT, onboard_passengers INTEGER, onboard_steps INTEGER, onboard_waits INTEGER, onboard_avg_wait_time REAL)");
    query.exec("CREATE TABLE IF NOT EXISTS disembarked_state_stats (id INTEGER PRIMARY KEY AUTOINCREMENT, disembarked_passengers INTEGER, disembarked_avg_yourney REAL, disembarked_avg_wait_time REAL)");
    query.exec("CREATE TABLE IF NOT EXISTS passengers_positions (id INTEGER PRIMARY KEY AUTOINCREMENT, x INTEGER, y INTEGER)");

    SimulationStats stats = extractSimulationStats();

    // Save onboard state
    query.prepare("INSERT INTO onboard_state_stats (onboard_passengers, onboard_steps, onboard_waits, onboard_avg_wait_time) "
                  "VALUES (?, ?, ?, ?)");
    query.addBindValue(stats.onboardPassengers);
    query.addBindValue(stats.onboardSteps);
    query.addBindValue(stats.onboardWaits);
    query.addBindValue(stats.onboardAvgWaitTime);

    if (!query.exec()) {
        QMessageBox::warning(this, "Database Error", "Failed to insert onboard state data: " + query.lastError().text());
        return;
    }

    // Save disembarked state
    query.prepare("INSERT INTO disembarked_state_stats (disembarked_passengers, disembarked_avg_yourney, disembarked_avg_wait_time) "
                  "VALUES (?, ?, ?)");
    query.addBindValue(stats.disembarkedPassengers);
    query.addBindValue(stats.disembarkedAvgYourney);
    query.addBindValue(stats.disembarkedAvgWaitTime);

    if (!query.exec()) {
        QMessageBox::warning(this, "Database Error", "Failed to insert disembarked state data: " + query.lastError().text());
        return;
    }

    // Save passengers' positions (x, y)
    query.prepare("INSERT INTO passengers_positions (x, y) VALUES (?, ?)");
    for (const QPoint &point : *passengers) {
        query.addBindValue(point.x());
        query.addBindValue(point.y());
        if (!query.exec()) {
            QMessageBox::warning(this, "Database Error", "Failed to insert passenger position: " + query.lastError().text());
            return;
        }
    }

    QMessageBox::information(this, "Save Successful", "Initial state saved successfully to the SQLite database.");
}

void Database::loadInitialStateFromDatabase() {
    // Let the user choose the database file path via a file dialog (sqlite3 file)
    QString dbFilePath = QFileDialog::getOpenFileName(this, "Load Database File", "", "SQLite Database Files (*.sqlite3)");

    if (dbFilePath.isEmpty()) {
        // If no file was selected, show a warning and return
        QMessageBox::warning(this, "Load Error", "No database file selected for loading.");
        return;
    }

    // Open the database connection (SQLite)
    db.setDatabaseName(dbFilePath);

    if (!db.open()) {
        QMessageBox::warning(this, "Database Error", "Failed to open the database for loading: " + db.lastError().text());
        return;
    }

    // Prepare a QString to store the loaded data
    QString textData = "Loaded Initial State:\n\n";

    // Fetch onboard state data
    QSqlQuery onboardQuery;
    onboardQuery.prepare("SELECT onboard_passengers, onboard_steps, onboard_waits, onboard_avg_wait_time FROM onboard_state_stats ORDER BY id DESC LIMIT 1");

    if (!onboardQuery.exec()) {
        QMessageBox::warning(this, "Database Error", "Failed to fetch onboard state: " + onboardQuery.lastError().text());
        return;
    }

    if (onboardQuery.next()) {
        onboardPassengers = onboardQuery.value(0).toInt();
        onboardSteps = onboardQuery.value(1).toInt();
        onboardWaits = onboardQuery.value(2).toInt();
        onboardAvgWaitTime = onboardQuery.value(3).toDouble();

        // Update UI elements with onboard data
        widget->ui->onBoardPassengersLabel->setText(QString("Onboard Passengers: %1").arg(onboardPassengers));
        widget->ui->onBoardStepsLabel->setText(QString("Onboard Steps: %1").arg(onboardSteps));
        widget->ui->onBoardWaitsLabel->setText(QString("Onboard Waits: %1").arg(onboardWaits));
        widget->ui->onBoardAverageWaitTimeLabel->setText(QString("Onboard Avg Wait Time: %1").arg(onboardAvgWaitTime));

        // Append onboard data to the text data
        textData += QString("Onboard Passengers: %1\n").arg(onboardPassengers);
        textData += QString("Onboard Steps: %1\n").arg(onboardSteps);
        textData += QString("Onboard Waits: %1\n").arg(onboardWaits);
        textData += QString("Onboard Avg Wait Time: %1\n\n").arg(onboardAvgWaitTime);
    } else {
        QMessageBox::warning(this, "Load Error", "No onboard state data found in database.");
        return;
    }

    // Fetch disembarked state data
    QSqlQuery disembarkedQuery;
    disembarkedQuery.prepare("SELECT disembarked_passengers, disembarked_avg_yourney, disembarked_avg_wait_time FROM disembarked_state_stats ORDER BY id DESC LIMIT 1");

    if (!disembarkedQuery.exec()) {
        QMessageBox::warning(this, "Database Error", "Failed to fetch disembarked state: " + disembarkedQuery.lastError().text());
        return;
    }

    if (disembarkedQuery.next()) {
        disembarkedPassengers = disembarkedQuery.value(0).toInt();
        disembarkedAvgYourney = disembarkedQuery.value(1).toDouble();
        disembarkedAvgWaitTime = disembarkedQuery.value(2).toDouble();

        // Update UI elements with disembarked data
        widget->ui->disembarkedPassengersLabel->setText(QString("Disembarked Passengers: %1").arg(disembarkedPassengers));
        widget->ui->disembarkedAvgYourneyLabel->setText(QString("Disembarked Avg Yourney: %1").arg(disembarkedAvgYourney));
        widget->ui->disembarkedAvgWaitTimeLabel->setText(QString("Disembarked Avg Wait Time: %1").arg(disembarkedAvgWaitTime));

        // Append disembarked data to the text data
        textData += QString("Disembarked Passengers: %1\n").arg(disembarkedPassengers);
        textData += QString("Disembarked Avg Journey: %1\n").arg(disembarkedAvgYourney);
        textData += QString("Disembarked Avg Wait Time: %1\n\n").arg(disembarkedAvgWaitTime);
    } else {
        QMessageBox::warning(this, "Load Error", "No disembarked state data found in database.");
        return;
    }

    // Fetch passengers' positions
    QSqlQuery passengersQuery;
    passengersQuery.prepare("SELECT x, y FROM passengers_positions");

    if (!passengersQuery.exec()) {
        QMessageBox::warning(this, "Database Error", "Failed to fetch passengers' positions: " + passengersQuery.lastError().text());
        return;
    }

    // Load passenger positions into the list
    passengers->clear();
    while (passengersQuery.next()) {
        int x = passengersQuery.value(0).toInt();
        int y = passengersQuery.value(1).toInt();
        passengers->append(QPoint(x, y));
    }

    // Set the loaded data to the QTextEdit
    widget->ui->textLoadingData->setText(textData);
    passenger->onboardPassengers = onboardPassengers;
    passenger->onboardSteps = onboardSteps;
    passenger->onboardWaits = onboardWaits;
    passenger->onboardAvgWaitTime = onboardAvgWaitTime;
    passenger->disembarkedPassengers = disembarkedPassengers;
    passenger->disembarkedAvgYourney = disembarkedAvgYourney;
    passenger->disembarkedAvgWaitTime = disembarkedAvgWaitTime;
    passenger->totalTime = totalTime;
    passenger->iterationCount = 0;
    //passenger->resetStatistics();
    widget->update();
    passenger->updateStatisticsLabels();

    QMessageBox::information(this, "Load Initial State", "Initial state loaded from database successfully!");
}

void Database::findOptimalSeatingArrangement() {
    passenger->iterationCount = 0;
    passenger->resetStatistics();
    std::atomic<int> optimalSimulationIndex{-1};  // Index of optimal simulation
    std::atomic<int> bestTime{INT_MAX};          // Best time ever
    std::mutex mutex;                            // To synchronize access

    srand(time(nullptr));  // Initialize random number generator seed

    const int simulationCount = 3;  // Number of simulations
    std::vector<std::thread> simulationThreads; // Simulation threads
    std::vector<std::shared_ptr<std::vector<std::vector<int>>>> simulationArrangements(simulationCount);

    int passengerCount = passenger->getPassengerCount();

    for (int simulation = 0; simulation < simulationCount; ++simulation) {
        simulationThreads.emplace_back([&, simulation]() {
            // Create a layout
            auto currentArrangement = std::make_shared<std::vector<std::vector<int>>>(rowCount, std::vector<int>(colCount, 0));
            int currentPassengerCount = 0;

            // Creating fixed layouts
            //**
            switch (simulation) {
            case 0:
                // Simulation 1: All odd row indices 1st, 2nd, 3rd column indices
                for (int i = 1; i < rowCount-1; i += 2) {
                    for (int j = 0; j < 3 && currentPassengerCount < passengerCount; ++j) {
                        (*currentArrangement)[i][j] = 1;
                        ++currentPassengerCount;
                    }
                    if(currentPassengerCount > passengerCount) {
                        break;
                    }
                }

                for (int i = rowCount-3; i > 0; i -= 2) {
                    for (int j = 4; j < 7 && currentPassengerCount < passengerCount; ++j) {
                        (*currentArrangement)[i][j] = 1;
                        ++currentPassengerCount;

                    }
                    if(currentPassengerCount > passengerCount) {
                        break;
                    }
                }
                break;
                //**
                //**
            case 1:
                // Simulation 2: All odd row indices 4th, 5th, 6th column indices
                for (int i = 1; i < rowCount-1; i += 2) {
                    for (int j = 4; j <= 6 && currentPassengerCount < passengerCount; ++j) {
                        (*currentArrangement)[i][j] = 1;
                        ++currentPassengerCount;
                    }
                    if(currentPassengerCount > passengerCount) {
                        break;
                    }
                }
                for (int i = rowCount-3; i > 0; i -= 2) {
                    for (int j = 0; j < 3 && currentPassengerCount < passengerCount; ++j) {
                        (*currentArrangement)[i][j] = 1;
                        ++currentPassengerCount;
                    }
                    if(currentPassengerCount > passengerCount) {
                        break;
                    }
                }
                break;
                //**
                //**
            case 2:
                // Simulation 3: Column indices 0, 1, 2 on the first odd row index, column indices 4, 5, 6 on the second odd row index, alternating
                for (int i = 1; i < rowCount-1; i += 2) {
                    if (currentPassengerCount >= passengerCount) break;
                    if ((i / 2) % 2 == 0) {  // Odd row with even index (columns 0, 1, 2)
                        for (int j = 0; j < 3 && currentPassengerCount < passengerCount; ++j) {
                            (*currentArrangement)[i][j] = 1;
                            ++currentPassengerCount;
                        }
                    } else {  // Odd row with odd index (columns 4, 5, 6)
                        for (int j = 4; j <= 6 && currentPassengerCount < passengerCount; ++j) {
                            (*currentArrangement)[i][j] = 1;
                            ++currentPassengerCount;
                        }
                    }
                }
                for (int i = rowCount-3; i > 0; i -= 2) {
                    if (currentPassengerCount >= passengerCount) break;
                    if ((i / 2) % 2 == 0) {  // Odd row with even index (columns 0, 1, 2)
                        for (int j = 4; j <= 6 && currentPassengerCount < passengerCount; ++j) {
                            (*currentArrangement)[i][j] = 1;
                            ++currentPassengerCount;
                        }
                    } else {  // Odd row with odd index (columns 4, 5, 6)
                        for (int j = 0; j < 3 && currentPassengerCount < passengerCount; ++j) {
                            (*currentArrangement)[i][j] = 1;
                            ++currentPassengerCount;
                        }
                    }
                }
                break;
            }
            //**

            // Executing a simulation
            int currentTotalTime = simulateEvacuation(*currentArrangement);
            simulationArrangements[simulation] = currentArrangement;

            // Update best time if needed
            std::lock_guard<std::mutex> lock(mutex);
            if (currentTotalTime < bestTime) {
                bestTime = currentTotalTime;
                optimalSimulationIndex = simulation;
                optimalArrangement = *currentArrangement;  // Creating an explicit copy
            }
        });
    }

    // Waiting for all threads to finish
    for (auto& thread : simulationThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    for (int i = 0; i < simulationCount; ++i) {
        drawSimulationResult(*simulationArrangements[i], i, i == optimalSimulationIndex);  // We'll let you know which is the best simulation
    }

    // Show results
    if (bestTime < INT_MAX) {
        // We automatically set the optimal layout
        QList<QPoint> optimalPassengerPositions;

        for (int i = 0; i < rowCount; ++i) {
            for (int j = 0; j < colCount; ++j) {
                if (optimalArrangement[i][j] == 1) {
                    optimalPassengerPositions.append(QPoint(i, j));
                }
            }
        }

        // Initialize passengers with the optimal layout
        passenger->initializePassengers(optimalArrangement, true, optimalPassengerPositions);

        QMessageBox::information(this, "Optimal Seating",
                                 QString("Optimal seating arrangement applied automatically with %1 seconds.")
                                     .arg(bestTime));
    }

    // Error handling: if optimalTime is 0, an error message is displayed
    if (bestTime == 0) {
        QMessageBox::critical(this, "Error", "Optimal time is 0, which is invalid.");
        return;
    }
}

int Database::simulateEvacuation(const std::vector<std::vector<int>>& arrangement) {
    int steps = 0;
    bool allEmpty = false;
    std::vector<std::vector<int>> tempArrangement = arrangement;

    auto isValid = [&](int x, int y) {
        return x >= 0 && x < rowCount && y >= 0 && y < colCount;
    };

    auto distanceToExit = [&](int x, int y) {
        return abs(x - 9) + abs(y - 3);
    };

    while (!allEmpty) {
        allEmpty = true;
        bool hasMoved = false;
        std::vector<std::vector<int>> nextArrangement = tempArrangement;

        for (int i = 0; i < rowCount; ++i) {
            for (int j = 0; j < colCount; ++j) {
                if (tempArrangement[i][j] == 1) {  // If the place is taken
                    allEmpty = false;

                    // Movement possibilities (in four directions)
                    std::vector<std::pair<int, int>> directions = {
                        {i - 1, j},  // Top
                        {i + 1, j},  // Down
                        {i, j - 1},  // Left
                        {i, j + 1}   // Right
                    };

                    // Sort by distance
                    std::sort(directions.begin(), directions.end(), [&](const auto& a, const auto& b) {
                        return distanceToExit(a.first, a.second) < distanceToExit(b.first, b.second);
                    });

                    // Try to move in the best direction
                    for (const auto& [newX, newY] : directions) {
                        if (isValid(newX, newY) && tempArrangement[newX][newY] == 0) {
                            nextArrangement[newX][newY] = 1;
                            nextArrangement[i][j] = 0;
                            hasMoved = true;
                            break;
                        }
                    }
                }
            }
        }

        if (!hasMoved) {
            break;
        }

        tempArrangement = nextArrangement;
        ++steps;

        // We check if the first column of the last row is available
        if (tempArrangement[9][3] == 1) {
            break;
        }
    }

    return steps;
}

void Database::drawSimulationResult(const std::vector<std::vector<int>>& arrangement, int simulationIndex, bool isBestSimulation) {
    QGraphicsScene* scene = new QGraphicsScene(this);
    QGraphicsView* view = new QGraphicsView(scene);

    const int cellSize = 20;  // Size of each cell in pixels
    const int exitRow = 9;    // Exit row
    const int exitCol = 3;    // Exit column

    QList<QPoint> passengerPositions;

    // Get passenger positions directly from the arrangement
    for (int i = 0; i < arrangement.size(); ++i) {
        for (int j = 0; j < arrangement[i].size(); ++j) {
            if (arrangement[i][j] == 1) {
                passengerPositions.append(QPoint(i, j));
            }
        }
    }

    // Draw the grid and objects
    for (int i = 0; i < arrangement.size(); ++i) {
        for (int j = 0; j < arrangement[i].size(); ++j) {
            QColor color;
            bool hasPassenger = passengerPositions.contains(QPoint(i, j));

            // Exit position
            if (i == exitRow && j == exitCol) {
                color = Qt::white;
            }
            // Aisle (column 3)
            else if (j == 3) {
                color = Qt::white;
            }
            // Seat positions
            else if (i % 2 == 0 && (j < 3 || j > 3)) {
                color = Qt::gray;
            }
            // Other positions
            else {
                color = Qt::white;
            }

            if (hasPassenger) {
                color = Qt::yellow;
            }

            scene->addRect(j * cellSize, i * cellSize, cellSize, cellSize, QPen(Qt::black), QBrush(color));
        }
    }

    // Draw passengers with blue circles
    for (const QPoint& pos : passengerPositions) {
        int x = pos.y() * cellSize;
        int y = pos.x() * cellSize;
        scene->addEllipse(x + cellSize / 4, y + cellSize / 4, cellSize / 2, cellSize / 2, QPen(Qt::black), QBrush(Qt::blue));
    }

    // Set window title based on the simulation type
    QString windowTitle = QString("Simulation %1 Result").arg(simulationIndex + 1);
    if (isBestSimulation) {
        windowTitle = "Best Simulation";  // Change title for best simulation
    }

    view->setWindowTitle(windowTitle);
    view->setScene(scene);
    int viewWidth = cellSize * arrangement[0].size() + 200;
    int viewHeight = cellSize * arrangement.size() + 50;
    view->setFixedSize(viewWidth, viewHeight);

    // Add "Choose" button to the view
    QPushButton* chooseButton = new QPushButton("Choose", view);
    chooseButton->move(viewWidth - 90, viewHeight - 140);
    chooseButton->show();

    connect(chooseButton, &QPushButton::clicked, this, [this, arrangement, simulationIndex, isBestSimulation, passengerPositions]() {
        // Store the selected simulation arrangement
        selectedArrangement = arrangement;

        // Pass passenger positions from the selected arrangement to initialize passengers
        passenger->initializePassengers(selectedArrangement, isBestSimulation, passengerPositions);

        QString message;
        if (isBestSimulation) {
            message = "Best simulation selected.";
        } else {
            message = QString("Simulation %1 selected.").arg(simulationIndex + 1);
        }

        QMessageBox::information(nullptr, "Simulation Selected", message);
    });

    view->show();
}

void Database::saveOptimalSeatingArrangementToDatabase() {
    // Select database file
    QString dbFilePath = QFileDialog::getSaveFileName(this, "Save Optimal Seating to Database", "",
                                                      "SQLite Files (*.sqlite3)");

    if (dbFilePath.isEmpty()) {
        QMessageBox::warning(this, "Save Error", "No database file selected.");
        return;
    }

    db.setDatabaseName(dbFilePath);

    if (!db.open()) {
        QMessageBox::warning(this, "Save Error", "Unable to open the database: " + db.lastError().text());
        return;
    }

    QSqlQuery query;

    // Create tables if they do not already exist
    query.exec("CREATE TABLE IF NOT EXISTS Passenger ("
               "PassengerID INTEGER PRIMARY KEY, "
               "PositionX INTEGER, "
               "PositionY INTEGER)");

    query.exec("CREATE TABLE IF NOT EXISTS PLANE ("
               "PlaneID INTEGER PRIMARY KEY AUTOINCREMENT, "
               "PassengerID INTEGER, "
               "StatID INTEGER, "
               "Seed INTEGER, "
               "Percent REAL, "
               "Optimal BOOLEAN, "
               "FOREIGN KEY (PassengerID) REFERENCES Passenger(PassengerID), "
               "FOREIGN KEY (StatID) REFERENCES Stats(StatID))");

    query.exec("CREATE TABLE IF NOT EXISTS Stats ("
               "StatID INTEGER PRIMARY KEY AUTOINCREMENT, "
               "Version TEXT, "
               "ItCount INTEGER, "
               "OnboardPassengers INTEGER, "
               "OnboardSteps INTEGER, "
               "OnboardWaits INTEGER, "
               "OnboardAvgWaitTime REAL, "
               "DisembarkedPassengers INTEGER, "
               "DisembarkedAvgYourney REAL, "
               "DisembarkedAvgWaitTime REAL)");

    // Query the last PassengerID
    int lastPassengerID = 0;
    query.exec("SELECT MAX(PassengerID) FROM Passenger");
    if (query.next()) {
        lastPassengerID = query.value(0).toInt();
    }

    int passengerID = lastPassengerID + 1;  // A new PassengerID starts after the last one
    for (const QPoint &p : *passengers) {
        QSqlQuery query;
        query.prepare("INSERT INTO Passenger (PassengerID, PositionX, PositionY) VALUES (:id, :x, :y)");
        query.bindValue(":id", passengerID++);
        query.bindValue(":x", p.x());
        query.bindValue(":y", p.y());

        if (!query.exec()) {
            QMessageBox::warning(this, "Save Error", "Failed to save passenger: " + query.lastError().text());
            db.close();
            return;
        }
    }

    SimulationStats stats = extractSimulationStats();
    int iterationCount = 5;

    query.prepare("INSERT INTO Stats (Version, ItCount, OnboardPassengers, OnboardSteps, OnboardWaits, OnboardAvgWaitTime, DisembarkedPassengers, DisembarkedAvgYourney, DisembarkedAvgWaitTime) "
                  "VALUES (:version, :itCount, :onboardPassengers, :onboardSteps, :onboardWaits, :onboardAvgWaitTime, :disembarkedPassengers, :disembarkedAvgYourney, :disembarkedAvgWaitTime )");
    query.bindValue(":version", "1.0");
    query.bindValue(":itCount", iterationCount);
    query.bindValue(":onboardPassengers", stats.onboardPassengers);
    query.bindValue(":onboardSteps", stats.onboardSteps);
    query.bindValue(":onboardWaits", stats.onboardWaits);
    query.bindValue(":onboardAvgWaitTime", stats.onboardAvgWaitTime);
    query.bindValue(":disembarkedPassengers", stats.disembarkedPassengers);
    query.bindValue(":disembarkedAvgYourney", stats.disembarkedAvgYourney);
    query.bindValue(":disembarkedAvgWaitTime", stats.disembarkedAvgWaitTime);

    if (!query.exec()) {
        QMessageBox::warning(this, "Save Error", "Failed to save stats: " + query.lastError().text());
        db.close();
        return;
    }

    int statID = query.lastInsertId().toInt();

    query.prepare("INSERT INTO PLANE (PassengerID, StatID, Seed, Percent, Optimal) VALUES (:passengerID, :statID, :seed, :percent, :optimal)");

    double percentComplete = 0.0;
    for (int i = 0; i < passengers->size(); ++i) {
        int passengerID = lastPassengerID + i + 1;  // Continues after the last PassengerID
        query.bindValue(":passengerID", passengerID);
        query.bindValue(":statID", statID);

        int randomSeed = QRandomGenerator::global()->generate();
        query.bindValue(":seed", randomSeed);

        percentComplete += (100.0 / passengers->size());
        query.bindValue(":percent", percentComplete);

        QPoint pos = (*passengers)[i];
        bool isOptimal = optimalArrangement[pos.x()][pos.y()] == 1;
        query.bindValue(":optimal", isOptimal ? 1 : 0);

        if (!query.exec()) {
            QMessageBox::warning(this, "Save Error", "Failed to save plane data: " + query.lastError().text());
            db.close();
            return;
        }
    }

    db.close();

    QMessageBox::information(this, "Save Successful", "Optimal seating arrangement saved to database!");
}

void Database::loadOptimalSeatingArrangementFromDatabase() {
    // Select database file
    QString dbFilePath = QFileDialog::getOpenFileName(this, "Load Optimal Seating from Database", "",
                                                      "SQLite Files (*.sqlite3)");

    if (dbFilePath.isEmpty()) {
        QMessageBox::warning(this, "Load Error", "No database file selected.");
        return;
    }
    db.setDatabaseName(dbFilePath);

    if (!db.open()) {
        QMessageBox::warning(this, "Load Error", "Unable to open the database: " + db.lastError().text());
        return;
    }

    QSqlQuery query;

    // Retrieve last saved Stats record
    query.prepare("SELECT ItCount, OnboardPassengers, OnboardSteps, OnboardWaits, OnboardAvgWaitTime, DisembarkedPassengers, DisembarkedAvgYourney, DisembarkedAvgWaitTime "
                  "FROM Stats ORDER BY StatID DESC LIMIT 1");

    if (!query.exec() || !query.next()) {
        QMessageBox::warning(this, "Load Error", "No stats found in the database.");
        db.close();
        return;
    }

    // Load statistical data
    int iterationCount = query.value(0).toInt();
    int onboardPassengers = query.value(1).toInt();
    int onboardSteps = query.value(2).toInt();
    int onboardWaits = query.value(3).toInt();
    double onBoardAvgWaitTime = query.value(4).toDouble();
    int disembarkedPassengers = query.value(5).toInt();
    double disembarkedAvgYourney = query.value(6).toDouble();
    double disembarkedAvgWaitTime = query.value(7).toDouble();

    widget->ui->textLoadingData->clear();
    widget->ui->textLoadingData->append(QString("Iteration Count: %1\n").arg(iterationCount));
    widget->ui->textLoadingData->append(QString("Onboard Passengers: %1\n").arg(onboardPassengers));
    widget->ui->textLoadingData->append(QString("Onboard Steps: %1\n").arg(onboardSteps));
    widget->ui->textLoadingData->append(QString("Onboard Waits: %1\n").arg(onboardWaits));
    widget->ui->textLoadingData->append(QString("Onboard Avg Wait Time: %1\n").arg(onBoardAvgWaitTime));
    widget->ui->textLoadingData->append(QString("Disembarked Passengers: %1\n").arg(disembarkedPassengers));
    widget->ui->textLoadingData->append(QString("Disembarked Avg Yourney: %1\n").arg(disembarkedAvgYourney));
    widget->ui->textLoadingData->append(QString("Disembarked Avg Wait Time: %1\n").arg(disembarkedAvgWaitTime));

    // Load seating arrangement from Passenger table
    query.prepare("SELECT PositionX, PositionY FROM Passenger");

    if (!query.exec()) {
        QMessageBox::warning(this, "Load Error", "Failed to load passengers: " + query.lastError().text());
        db.close();
        return;
    }

    optimalArrangement.clear();  // Clear previous arrangement
    widget->ui->textLoadingData->append("\nSeating Arrangement:\n");

    passengers->clear();
    while (query.next()) {
        int positionX = query.value(0).toInt();
        int positionY = query.value(1).toInt();
        passengers->append(QPoint(positionX, positionY));

        if (static_cast<int>(optimalArrangement.size()) <= positionX) {
            optimalArrangement.resize(positionX + 1);
        }
        if (static_cast<int>(optimalArrangement[positionX].size()) <= positionY) {
            optimalArrangement[positionX].resize(positionY + 1, -1);  // -1 indicates no passenger at that seat
        }

        // Fill seating arrangement with passenger IDs (here we assume the ID could be found and is consistent)
        optimalArrangement[positionX][positionY] = 1;  // Placeholder as the original ID is not fetched correctly in your provided code
    }

    // Display the seating arrangement
    for (const auto& row : optimalArrangement) {
        QStringList rowData;
        for (int seat : row) {
            rowData.append(seat == -1 ? "0" : "1"); // Assuming '1' means passenger is present, '0' means empty
        }
        widget->ui->textLoadingData->append(rowData.join(" ") + "\n");
    }

    // Fetching plane data (from PLANE table)
    query.prepare("SELECT PassengerID, Seed, Percent, Optimal FROM PLANE");

    if (!query.exec()) {
        QMessageBox::warning(this, "Load Error", "Failed to load plane data: " + query.lastError().text());
        db.close();
        return;
    }

    widget->ui->textLoadingData->append("\nPlane Data:\n");

    while (query.next()) {
        int passengerID = query.value(0).toInt();
        int seed = query.value(1).toInt();
        double percent = query.value(2).toDouble();
        bool isOptimal = query.value(3).toBool();  // Assuming the 'Optimal' column stores boolean (1 for true, 0 for false)

        widget->ui->textLoadingData->append(QString("PassengerID: %1\nSeed: %2\nPercent: %3%\nOptimal: %4\n")
                                                .arg(passengerID)
                                                .arg(seed)
                                                .arg(percent)
                                                .arg(isOptimal ? "Yes" : "No"));
    }

    db.close();

    passenger->onboardPassengers = onboardPassengers;
    passenger->onboardSteps = onboardSteps;
    passenger->onboardWaits = onboardWaits;
    passenger->onboardAvgWaitTime = onboardAvgWaitTime;
    passenger->disembarkedPassengers = disembarkedPassengers;
    passenger->disembarkedAvgYourney = disembarkedAvgYourney;
    passenger->disembarkedAvgWaitTime = disembarkedAvgWaitTime;
    passenger->totalTime = totalTime;
    passenger->iterationCount = 0;
    //passenger->resetStatistics();
    widget->update();
    passenger->updateStatisticsLabels();
    // Success message
    QMessageBox::information(this, "Load Successful", "Optimal seating arrangement loaded successfully!");
}

void Database::showOptimalDatabaseTable() {
    // Open the database file
    QString dbFilePath = QFileDialog::getOpenFileName(this, "Open Optimal Seating from Database", "",
                                                      "SQLite Files (*.sqlite3)");

    if (dbFilePath.isEmpty()) {
        QMessageBox::warning(this, "Load Error", "No database file selected.");
        return;
    }
    db.setDatabaseName(dbFilePath);
    if (!db.open()) {
        QMessageBox::warning(this, "Load Error", "Unable to open the database: " + db.lastError().text());
        return;
    }

    // Create a new dialog
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Optimal Seating Data");
    dialog->resize(900, 700);

    // Create a table view and model
    QSqlTableModel *statsModel = new QSqlTableModel(dialog, db);
    statsModel->setTable("Stats");
    statsModel->select();

    if (statsModel->lastError().isValid()) {
        QMessageBox::warning(this, "Load Error", "Failed to load Stats data: " + statsModel->lastError().text());
        db.close();
        return;
    }

    QTableView *statsTableView = new QTableView(dialog);
    statsTableView->setModel(statsModel);
    statsTableView->resizeColumnsToContents();
    statsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    QSqlTableModel *passengerModel = new QSqlTableModel(dialog, db);
    passengerModel->setTable("Passenger");
    passengerModel->select();

    if (passengerModel->lastError().isValid()) {
        QMessageBox::warning(this, "Load Error", "Failed to load Passenger data: " + passengerModel->lastError().text());
        db.close();
        return;
    }

    QTableView *passengerTableView = new QTableView(dialog);
    passengerTableView->setModel(passengerModel);
    passengerTableView->resizeColumnsToContents();
    passengerTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    QSqlTableModel *planeModel = new QSqlTableModel(dialog, db);
    planeModel->setTable("PLANE");
    planeModel->select();

    if (planeModel->lastError().isValid()) {
        QMessageBox::warning(this, "Load Error", "Failed to load PLANE data: " + planeModel->lastError().text());
        db.close();
        return;
    }

    QTableView *planeTableView = new QTableView(dialog);
    planeTableView->setModel(planeModel);
    planeTableView->resizeColumnsToContents();
    planeTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Layout for the dialog
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->addWidget(new QLabel("Stats Data"));
    layout->addWidget(statsTableView);
    layout->addWidget(new QLabel("Passenger Data"));
    layout->addWidget(passengerTableView);
    layout->addWidget(new QLabel("Plane Data"));
    layout->addWidget(planeTableView);

    // Delete button for selected rows
    QPushButton *deleteButton = new QPushButton("Delete Selected Row(s)", dialog);
    connect(deleteButton, &QPushButton::clicked, this, [=]() {
        QModelIndexList selectedIndexes1 = statsTableView->selectionModel()->selectedRows();
        QModelIndexList selectedIndexes2 = passengerTableView->selectionModel()->selectedRows();
        QModelIndexList selectedIndexes3 = planeTableView->selectionModel()->selectedRows();

        if (selectedIndexes1.isEmpty() && selectedIndexes2.isEmpty() && selectedIndexes3.isEmpty()) {
            QMessageBox::warning(dialog, "No Row Selected", "Please select a row to delete.");
            return;
        }

        if (QMessageBox::question(dialog, "Delete Row(s)", "Are you sure you want to delete the selected row(s)?") == QMessageBox::Yes) {
            db.transaction();  // Start transaction

            for (const QModelIndex &index : selectedIndexes1) {
                if (!statsModel->removeRow(index.row())) {
                    QMessageBox::warning(dialog, "Delete Error", "Failed to delete row from Stats: " + statsModel->lastError().text());
                    db.rollback();
                    return;
                }
            }

            for (const QModelIndex &index : selectedIndexes2) {
                if (!passengerModel->removeRow(index.row())) {
                    QMessageBox::warning(dialog, "Delete Error", "Failed to delete row from Passenger: " + passengerModel->lastError().text());
                    db.rollback();
                    return;
                }
            }

            for (const QModelIndex &index : selectedIndexes3) {
                if (!planeModel->removeRow(index.row())) {
                    QMessageBox::warning(dialog, "Delete Error", "Failed to delete row from PLANE: " + planeModel->lastError().text());
                    db.rollback();
                    return;
                }
            }

            db.commit();  // Commit transaction
        }
    });

    // Delete all rows button
    QPushButton *deleteAllButton = new QPushButton("Delete All Rows", dialog);
    connect(deleteAllButton, &QPushButton::clicked, this, [=]() {
        if (QMessageBox::question(dialog, "Delete All Rows", "Are you sure you want to delete all rows?") == QMessageBox::Yes) {
            db.transaction();  // Start transaction

            for (int i = statsModel->rowCount() - 1; i >= 0; --i) {
                if (!statsModel->removeRow(i)) {
                    QMessageBox::warning(dialog, "Delete Error", "Failed to delete row from Stats: " + statsModel->lastError().text());
                    db.rollback();
                    return;
                }
            }

            for (int i = passengerModel->rowCount() - 1; i >= 0; --i) {
                if (!passengerModel->removeRow(i)) {
                    QMessageBox::warning(dialog, "Delete Error", "Failed to delete row from Passenger: " + passengerModel->lastError().text());
                    db.rollback();
                    return;
                }
            }

            for (int i = planeModel->rowCount() - 1; i >= 0; --i) {
                if (!planeModel->removeRow(i)) {
                    QMessageBox::warning(dialog, "Delete Error", "Failed to delete row from PLANE: " + planeModel->lastError().text());
                    db.rollback();
                    return;
                }
            }

            db.commit();  // Commit transaction
        }
    });

    layout->addWidget(deleteButton);
    layout->addWidget(deleteAllButton);

    dialog->setLayout(layout);
    dialog->exec();

    db.close();
}
