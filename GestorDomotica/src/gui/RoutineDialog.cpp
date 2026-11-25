#include "RoutineDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>
#include <algorithm>

namespace domotica {

RoutineDialog::RoutineDialog(const Routine& routine, const std::vector<DeviceRecord>& devices, QWidget* parent)
    : QDialog(parent), routine_(routine) {
    setWindowTitle(routine.id == 0 ? QStringLiteral("Nueva rutina") : QStringLiteral("Editar rutina"));

    name_ = new QLineEdit(QString::fromStdString(routine.name), this);
    name_->setMaxLength(50);
    time_ = new QTimeEdit(QTime(routine.minuteOfDay / 60, routine.minuteOfDay % 60), this);
    time_->setDisplayFormat(QStringLiteral("HH:mm"));
    action_ = new QComboBox(this);
    action_->addItem(QStringLiteral("Encender"), static_cast<int>(RoutineAction::TurnOn));
    action_->addItem(QStringLiteral("Apagar"), static_cast<int>(RoutineAction::TurnOff));
    action_->setCurrentIndex(routine.action == RoutineAction::TurnOn ? 0 : 1);

    auto* daysLayout = new QHBoxLayout;
    const QStringList dayNames = {QStringLiteral("Lun"), QStringLiteral("Mar"), QStringLiteral("Mié"),
                                  QStringLiteral("Jue"), QStringLiteral("Vie"), QStringLiteral("Sáb"),
                                  QStringLiteral("Dom")};
    for (int i = 0; i < 7; ++i) {
        auto* box = new QCheckBox(dayNames[i], this);
        box->setChecked((routine.daysMask >> i) & 1);
        days_.push_back(box);
        daysLayout->addWidget(box);
    }
    enabled_ = new QCheckBox(QStringLiteral("Rutina habilitada"), this);
    enabled_->setChecked(routine.enabled);

    devices_ = new QListWidget(this);
    for (const DeviceRecord& d : devices) {
        auto* item = new QListWidgetItem(QString::fromStdString(d.name + " (" + d.room + ")"), devices_);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        const bool selected = std::find(routine.deviceIds.begin(), routine.deviceIds.end(), d.id) != routine.deviceIds.end();
        item->setCheckState(selected ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, d.id);
    }

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Nombre:"), name_);
    form->addRow(QStringLiteral("Hora:"), time_);
    form->addRow(QStringLiteral("Acción:"), action_);
    form->addRow(QStringLiteral("Días:"), daysLayout);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(new QLabel(QStringLiteral("Dispositivos:"), this));
    layout->addWidget(devices_);
    layout->addWidget(enabled_);
    layout->addWidget(buttons);
    resize(420, 520);
}

Routine RoutineDialog::record() const {
    Routine r = routine_;
    r.name = name_->text().toStdString();
    r.minuteOfDay = time_->time().hour() * 60 + time_->time().minute();
    r.action = static_cast<RoutineAction>(action_->currentData().toInt());
    r.daysMask = 0;
    for (int i = 0; i < days_.size(); ++i) {
        if (days_[i]->isChecked()) r.daysMask |= (1 << i);
    }
    r.enabled = enabled_->isChecked();
    r.deviceIds.clear();
    for (int i = 0; i < devices_->count(); ++i) {
        const QListWidgetItem* item = devices_->item(i);
        if (item->checkState() == Qt::Checked) r.deviceIds.push_back(item->data(Qt::UserRole).toInt());
    }
    return r;
}

}  // namespace domotica
