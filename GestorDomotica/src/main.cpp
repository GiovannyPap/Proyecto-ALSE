/**
 * @file main.cpp
 * @brief Punto de entrada del Gestor de Domótica para Hogares.
 *
 * Uso: GestorDomotica [--db ruta/al/archivo.db]
 */
#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

#include "core/AuthService.h"
#include "core/DeviceService.h"
#include "core/Time.h"
#include "db/SqliteRepository.h"
#include "gui/LoginDialog.h"
#include "gui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("GestorDomotica"));
    app.setQuitOnLastWindowClosed(false);  // MainWindow::closeEvent cierra la aplicación

    QString dbPath;
    const QStringList args = app.arguments();
    for (int i = 1; i + 1 < args.size(); ++i) {
        if (args[i] == QLatin1String("--db")) dbPath = args[i + 1];
    }
    if (dbPath.isEmpty()) {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        dbPath = dir + QStringLiteral("/domotica.db");
    }

    domotica::SqliteRepository repo(dbPath);
    if (!repo.isOpen()) {
        QMessageBox::critical(nullptr, QStringLiteral("Base de datos"),
                              QStringLiteral("No se pudo abrir la base de datos:\n%1\n\n%2")
                                  .arg(dbPath, repo.lastError()));
        return 1;
    }

    domotica::AuthService auth(repo, domotica::systemClock);
    domotica::DeviceService devices(repo, domotica::systemClock);
    domotica::RoutineService routines(repo, devices, domotica::systemClock);

    domotica::LoginDialog login(auth);
    if (login.exec() != QDialog::Accepted) return 0;

    domotica::MainWindow window(auth, devices, routines);
    window.show();
    return app.exec();
}
