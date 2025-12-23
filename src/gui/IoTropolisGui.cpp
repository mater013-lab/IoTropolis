#include "gui/IoTropolisGui.h"
#include <QHeaderView>

IoTropolisGui::IoTropolisGui(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    unitTable = new QTableWidget(this);
    unitTable->setColumnCount(6);  // Checkbox, UnitID, IP, Type/Subtype, Sensors, Actuators
    QStringList headers = {"Select", "UnitID", "IP", "Type / Subtype", "Sensors", "Actuators"};
    unitTable->setHorizontalHeaderLabels(headers);
    unitTable->horizontalHeader()->setStretchLastSection(true);
    unitTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    layout->addWidget(unitTable);
    setLayout(layout);
}

void IoTropolisGui::addUnit(IoTropolisUnitConnection* unit)
{
    int row = unitTable->rowCount();
    unitTable->insertRow(row);

    // Checkbox
    QTableWidgetItem* checkbox = new QTableWidgetItem();
    checkbox->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    checkbox->setCheckState(Qt::Unchecked);
    unitTable->setItem(row, 0, checkbox);

    // UnitID
    unitTable->setItem(row, 1, new QTableWidgetItem(QString::number(unit->unitID())));

    // IP (readable format)
    unitTable->setItem(row, 2, new QTableWidgetItem(unit->ipAddress()));

    // Type/Subtype
    unitTable->setItem(row, 3, new QTableWidgetItem(unit->unitType() + " / " + unit->unitSubtype()));

    
    // Sensors
    unitTable->setItem(row, 4, new QTableWidgetItem(unit->sensorNames().join(", ")));

    // Actuators
    unitTable->setItem(row, 5, new QTableWidgetItem(unit->actuatorNames().join(", ")));

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
