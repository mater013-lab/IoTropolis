#include "gui/IoTropolisGui.h"
#include <QHeaderView>

IoTropolisGui::IoTropolisGui(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    unitTable = new QTableWidget(this);
    unitTable->setColumnCount(5);  // Checkbox, IP, Type, Sensors, Actuators
    QStringList headers = {"Select", "IP", "Type", "Sensors", "Actuators"};
    unitTable->setHorizontalHeaderLabels(headers);
    unitTable->horizontalHeader()->setStretchLastSection(true);

    layout->addWidget(unitTable);
    setLayout(layout);
}

void IoTropolisGui::addUnit(IoTropolisUnitConnection* unit)
{
    int row = unitTable->rowCount();
    unitTable->insertRow(row);

    // Checkbox (visible and interactive)
    QTableWidgetItem* checkbox = new QTableWidgetItem();
    checkbox->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    checkbox->setCheckState(Qt::Unchecked);
    unitTable->setItem(row, 0, checkbox);

    // IP
    unitTable->setItem(row, 1, new QTableWidgetItem(unit->ipAddress()));

    // Type/Subtype
    unitTable->setItem(row, 2, new QTableWidgetItem(unit->unitType() + " / " + unit->unitSubtype()));

    // Sensors (join QStringList into a readable string)
    unitTable->setItem(row, 3, new QTableWidgetItem(unit->sensors().join(", ")));

    // Actuators (join QStringList into a readable string)
    unitTable->setItem(row, 4, new QTableWidgetItem(unit->actuators().join(", ")));
}

void IoTropolisGui::removeUnit(IoTropolisUnitConnection* unit)
{
    // Find the row with this IP
    for (int row = 0; row < unitTable->rowCount(); ++row) {
        if (unitTable->item(row, 1)->text() == unit->ipAddress()) {
            unitTable->removeRow(row);
            break;
        }
    }
}
