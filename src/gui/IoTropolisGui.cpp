#include "gui/IoTropolisGui.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QApplication>

IoTropolisGui::IoTropolisGui(QWidget *parent)
    : QMainWindow(parent)
{
    // ===============================
    // Original central widget + table
    // ===============================
    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);

    unitTable = new QTableWidget(this);
    unitTable->setColumnCount(6);  // Checkbox, UnitID, IP, Type/Subtype, Sensors, Actuators

    QStringList headers = {
        "Select",
        "UnitID",
        "IP",
        "Type / Subtype",
        "Sensors",
        "Actuators"
    };
    unitTable->setHorizontalHeaderLabels(headers);
    unitTable->horizontalHeader()->setStretchLastSection(true);
    unitTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    layout->addWidget(unitTable);
    central->setLayout(layout);
    setCentralWidget(central);

    // ===============================
    // Menu bar (NEW)
    // ===============================
    createMenus();

    setWindowTitle("IoTropolis");
    resize(1000, 500);
}

// ===============================
// Original functionality preserved
// ===============================

void IoTropolisGui::addUnit(IoTropolisUnitConnection* unit)
{
    int row = unitTable->rowCount();
    unitTable->insertRow(row);

    // Checkbox
    QTableWidgetItem* checkbox = new QTableWidgetItem();
    checkbox->setFlags(Qt::ItemIsUserCheckable |
                       Qt::ItemIsEnabled |
                       Qt::ItemIsSelectable);
    checkbox->setCheckState(Qt::Unchecked);
    unitTable->setItem(row, 0, checkbox);

    // UnitID
    unitTable->setItem(row, 1,
        new QTableWidgetItem(QString::number(unit->unitID())));

    // IP
    unitTable->setItem(row, 2,
        new QTableWidgetItem(unit->ipAddress()));

    // Type / Subtype
    unitTable->setItem(row, 3,
        new QTableWidgetItem(unit->unitType() + " / " + unit->unitSubtype()));

    // Sensors
    unitTable->setItem(row, 4,
        new QTableWidgetItem(unit->sensorNames().join(", ")));

    // Actuators
    unitTable->setItem(row, 5,
        new QTableWidgetItem(unit->actuatorNames().join(", ")));
}

void IoTropolisGui::removeUnit(IoTropolisUnitConnection* unit)
{
    int row = findRowByUnitID(unit->unitID());
    if (row >= 0)
        unitTable->removeRow(row);
}

int IoTropolisGui::findRowByUnitID(UnitID id) const
{
    for (int row = 0; row < unitTable->rowCount(); ++row) {
        QTableWidgetItem* item = unitTable->item(row, 1);
        if (item && item->text().toULongLong() == id)
            return row;
    }
    return -1;
}

// ===============================
// Menu-related additions
// ===============================

void IoTropolisGui::createMenus()
{
    // ---- File menu ----
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* quitAction = new QAction("&Quit", this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered,
            qApp, &QApplication::quit);

    fileMenu->addAction(quitAction);

    // ---- About menu ----
    QMenu* aboutMenu = menuBar()->addMenu("&About");

    QAction* aboutAction = new QAction("&About IoTropolis", this);
    connect(aboutAction, &QAction::triggered,
            this, &IoTropolisGui::showAboutDialog);

    aboutMenu->addAction(aboutAction);
}

void IoTropolisGui::showAboutDialog()
{
    const QString aboutText =
        "<b>IoTropolis</b><br><br>"
        "<b>Software version:</b> 1.0.0<br>"
        "<b>Protocol version:</b> 1.0<br><br>"
        "<b>Author:</b> Donnie Materassi Name<br>"
        "<b>Affiliation:</b> University of Minnesota<br><br>"
        "© 2025";

    QMessageBox::about(this, "About IoTropolis", aboutText);
}
