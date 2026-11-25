/**
 * @file LoginDialog.h
 * @brief Diálogo de inicio de sesión y creación de cuentas.
 */
#pragma once

#include <QDialog>

#include "../core/AuthService.h"

class QLabel;
class QLineEdit;
class QPushButton;

namespace domotica {

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(AuthService& auth, QWidget* parent = nullptr);

private slots:
    void onSubmit();
    void onToggleMode();

private:
    void updateMode();

    AuthService& auth_;
    QLineEdit* user_;
    QLineEdit* pass_;
    QLineEdit* confirm_;
    QLabel* confirmLabel_;
    QLabel* info_;
    QLabel* error_;
    QPushButton* submit_;
    QPushButton* toggle_;
    bool registerMode_ = false;
};

}  // namespace domotica
