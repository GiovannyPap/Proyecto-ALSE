/**
 * @file RoutineDialog.h
 * @brief Formulario para crear o editar una rutina programada.
 */
#pragma once

#include <QDialog>
#include <QVector>
#include <vector>

#include "../core/Routine.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QListWidget;
class QTimeEdit;

namespace domotica {

class RoutineDialog : public QDialog {
    Q_OBJECT
public:
    RoutineDialog(const Routine& routine, const std::vector<DeviceRecord>& devices, QWidget* parent = nullptr);
    Routine record() const;

private:
    Routine routine_;
    QLineEdit* name_;
    QTimeEdit* time_;
    QComboBox* action_;
    QVector<QCheckBox*> days_;
    QCheckBox* enabled_;
    QListWidget* devices_;
};

}  // namespace domotica
