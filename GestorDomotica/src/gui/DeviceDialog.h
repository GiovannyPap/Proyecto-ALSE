/**
 * @file DeviceDialog.h
 * @brief Formulario para registrar o editar un dispositivo.
 */
#pragma once

#include <QDialog>

#include "../core/Types.h"

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;

namespace domotica {

class DeviceDialog : public QDialog {
    Q_OBJECT
public:
    explicit DeviceDialog(const DeviceRecord& rec, QWidget* parent = nullptr);
    /// Devuelve el registro con los valores del formulario (conserva id, estado y nivel).
    DeviceRecord record() const;

private:
    DeviceRecord rec_;
    QLineEdit* name_;
    QLineEdit* room_;
    QComboBox* type_;
    QDoubleSpinBox* power_;
    QDoubleSpinBox* hours_;
};

}  // namespace domotica
