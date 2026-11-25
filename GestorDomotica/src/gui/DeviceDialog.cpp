#include "DeviceDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

namespace domotica {

DeviceDialog::DeviceDialog(const DeviceRecord& rec, QWidget* parent) : QDialog(parent), rec_(rec) {
    setWindowTitle(rec.id == 0 ? QStringLiteral("Nuevo dispositivo") : QStringLiteral("Editar dispositivo"));

    name_ = new QLineEdit(QString::fromStdString(rec.name), this);
    name_->setMaxLength(50);
    room_ = new QLineEdit(QString::fromStdString(rec.room), this);
    room_->setMaxLength(50);
    type_ = new QComboBox(this);
    for (DeviceType t : {DeviceType::Light, DeviceType::Blind, DeviceType::Sensor, DeviceType::Actuator}) {
        type_->addItem(QString::fromStdString(label(t)), static_cast<int>(t));
    }
    type_->setCurrentIndex(type_->findData(static_cast<int>(rec.type)));
    power_ = new QDoubleSpinBox(this);
    power_->setRange(0.0, 20000.0);
    power_->setDecimals(1);
    power_->setSuffix(QStringLiteral(" W"));
    power_->setValue(rec.powerW);
    hours_ = new QDoubleSpinBox(this);
    hours_->setRange(0.0, 24.0);
    hours_->setDecimals(1);
    hours_->setSuffix(QStringLiteral(" h/día"));
    hours_->setValue(rec.dailyHours);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Nombre:"), name_);
    form->addRow(QStringLiteral("Habitación:"), room_);
    form->addRow(QStringLiteral("Tipo:"), type_);
    form->addRow(QStringLiteral("Potencia nominal:"), power_);
    form->addRow(QStringLiteral("Uso promedio diario:"), hours_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

DeviceRecord DeviceDialog::record() const {
    DeviceRecord r = rec_;
    r.name = name_->text().toStdString();
    r.room = room_->text().toStdString();
    r.type = static_cast<DeviceType>(type_->currentData().toInt());
    r.powerW = power_->value();
    r.dailyHours = hours_->value();
    return r;
}

}  // namespace domotica
