#include "LoginDialog.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace domotica {

LoginDialog::LoginDialog(AuthService& auth, QWidget* parent) : QDialog(parent), auth_(auth) {
    setWindowTitle(QStringLiteral("Gestor de Domótica - Acceso"));
    setModal(true);

    info_ = new QLabel(this);
    info_->setWordWrap(true);
    user_ = new QLineEdit(this);
    pass_ = new QLineEdit(this);
    pass_->setEchoMode(QLineEdit::Password);
    confirm_ = new QLineEdit(this);
    confirm_->setEchoMode(QLineEdit::Password);
    confirmLabel_ = new QLabel(QStringLiteral("Confirmar contraseña:"), this);
    error_ = new QLabel(this);
    error_->setStyleSheet(QStringLiteral("color: #c0392b;"));
    error_->setWordWrap(true);
    submit_ = new QPushButton(this);
    submit_->setDefault(true);
    toggle_ = new QPushButton(this);
    toggle_->setFlat(true);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Usuario:"), user_);
    form->addRow(QStringLiteral("Contraseña:"), pass_);
    form->addRow(confirmLabel_, confirm_);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(info_);
    layout->addLayout(form);
    layout->addWidget(error_);
    layout->addWidget(submit_);
    layout->addWidget(toggle_);

    connect(submit_, &QPushButton::clicked, this, &LoginDialog::onSubmit);
    connect(toggle_, &QPushButton::clicked, this, &LoginDialog::onToggleMode);

    registerMode_ = !auth_.hasUsers();  // primer uso: crear la cuenta inicial
    updateMode();
    setMinimumWidth(340);
}

void LoginDialog::updateMode() {
    confirm_->setVisible(registerMode_);
    confirmLabel_->setVisible(registerMode_);
    submit_->setText(registerMode_ ? QStringLiteral("Crear cuenta") : QStringLiteral("Iniciar sesión"));
    toggle_->setText(registerMode_ ? QStringLiteral("Ya tengo cuenta") : QStringLiteral("Crear una cuenta nueva"));
    toggle_->setVisible(auth_.hasUsers() || !registerMode_);
    info_->setText(auth_.hasUsers() ? QString()
                                    : QStringLiteral("No hay usuarios registrados. Cree la cuenta inicial."));
    error_->clear();
}

void LoginDialog::onToggleMode() {
    registerMode_ = !registerMode_;
    updateMode();
}

void LoginDialog::onSubmit() {
    const std::string username = user_->text().trimmed().toStdString();
    const std::string password = pass_->text().toStdString();
    if (registerMode_) {
        if (pass_->text() != confirm_->text()) {
            error_->setText(QStringLiteral("Las contraseñas no coinciden."));
            return;
        }
        const Result reg = auth_.registerUser(username, password);
        if (!reg.ok) {
            error_->setText(QString::fromStdString(reg.message));
            return;
        }
    }
    const Result res = auth_.login(username, password);
    if (!res.ok) {
        error_->setText(QString::fromStdString(res.message));
        pass_->clear();
        return;
    }
    accept();
}

}  // namespace domotica
