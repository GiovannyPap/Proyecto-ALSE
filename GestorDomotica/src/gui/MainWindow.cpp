#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QCloseEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <map>

#include "../core/Energy.h"
#include "../core/Report.h"
#include "../core/Time.h"
#include "ChartWidgets.h"
#include "DeviceDialog.h"
#include "LoginDialog.h"
#include "RoutineDialog.h"

namespace domotica {

namespace {

QString qs(const std::string& s) { return QString::fromStdString(s); }

QTableWidgetItem* cell(const QString& text, bool alignRight = false) {
    auto* item = new QTableWidgetItem(text);
    if (alignRight) item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

QTableWidget* makeTable(const QStringList& headers, QWidget* parent) {
    auto* t = new QTableWidget(0, headers.size(), parent);
    t->setHorizontalHeaderLabels(headers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->verticalHeader()->setVisible(false);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    t->setAlternatingRowColors(true);
    return t;
}

QPushButton* makeButton(const QString& text, QWidget* parent) { return new QPushButton(text, parent); }

}  // namespace

MainWindow::MainWindow(AuthService& auth, DeviceService& devices, RoutineService& routines, QWidget* parent)
    : QMainWindow(parent), auth_(auth), devices_(devices), routines_(routines) {
    resize(1050, 680);

    auto* logout = new QAction(QStringLiteral("Cerrar &sesión"), this);
    connect(logout, &QAction::triggered, this, &MainWindow::onLogout);
    auto* quit = new QAction(QStringLiteral("&Salir"), this);
    connect(quit, &QAction::triggered, this, &QWidget::close);
    QMenu* fileMenu = menuBar()->addMenu(QStringLiteral("&Archivo"));
    fileMenu->addAction(logout);
    fileMenu->addAction(quit);

    tabs_ = new QTabWidget(this);
    tabs_->addTab(buildPanelTab(), QStringLiteral("Panel de control"));
    tabs_->addTab(buildDevicesTab(), QStringLiteral("Dispositivos"));
    tabs_->addTab(buildRoutinesTab(), QStringLiteral("Rutinas"));
    tabs_->addTab(buildHistoryTab(), QStringLiteral("Historial"));
    setCentralWidget(tabs_);
    connect(tabs_, &QTabWidget::currentChanged, this, [this](int) { refreshAll(); });

    routineTimer_ = new QTimer(this);
    connect(routineTimer_, &QTimer::timeout, this, &MainWindow::onTick);
    routineTimer_->start(15000);
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &MainWindow::refreshPanel);
    refreshTimer_->start(5000);

    updateTitle();
    refreshAll();
}

void MainWindow::updateTitle() {
    setWindowTitle(QStringLiteral("Gestor de Domótica para Hogares - %1").arg(qs(auth_.currentUser())));
}

// ------------------------------------------------------------------ Construcción de pestañas
QWidget* MainWindow::buildPanelTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    auto* summary = new QHBoxLayout;
    lblTotal_ = new QLabel(page);
    lblOn_ = new QLabel(page);
    lblFault_ = new QLabel(page);
    lblPower_ = new QLabel(page);
    for (QLabel* l : {lblTotal_, lblOn_, lblFault_, lblPower_}) {
        l->setStyleSheet(QStringLiteral("font-weight: bold; padding: 6px; border: 1px solid #bbb; border-radius: 4px;"));
        l->setAlignment(Qt::AlignCenter);
        summary->addWidget(l);
    }
    layout->addLayout(summary);

    auto* charts = new QHBoxLayout;
    barChart_ = new BarChartWidget(page);
    pieChart_ = new PieChartWidget(page);
    charts->addWidget(barChart_, 3);
    charts->addWidget(pieChart_, 2);
    layout->addLayout(charts, 1);

    auto* tariffRow = new QHBoxLayout;
    tariffRow->addWidget(new QLabel(QStringLiteral("Tarifa de energía ($/kWh):"), page));
    tariff_ = new QDoubleSpinBox(page);
    tariff_->setRange(0.0, 100000.0);
    tariff_->setDecimals(2);
    tariff_->setValue(devices_.tariff());
    connect(tariff_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        devices_.setTariff(v);
        refreshPanel();
    });
    tariffRow->addWidget(tariff_);
    lblEnergy_ = new QLabel(page);
    tariffRow->addWidget(lblEnergy_, 1);
    layout->addLayout(tariffRow);

    energyTable_ = makeTable({QStringLiteral("Dispositivo"), QStringLiteral("Potencia (W)"),
                              QStringLiteral("Uso (h/día)"), QStringLiteral("kWh/día"), QStringLiteral("kWh/mes"),
                              QStringLiteral("Costo/mes ($)"), QStringLiteral("Medido 30 d (kWh)")},
                             page);
    layout->addWidget(energyTable_, 1);
    return page;
}

QWidget* MainWindow::buildDevicesTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* row = new QHBoxLayout;

    struct Btn { QString text; void (MainWindow::*slot)(); };
    const Btn buttons[] = {
        {QStringLiteral("Agregar"), &MainWindow::onAddDevice},
        {QStringLiteral("Editar"), &MainWindow::onEditDevice},
        {QStringLiteral("Eliminar"), &MainWindow::onRemoveDevice},
        {QStringLiteral("Encender / Abrir"), &MainWindow::onActivate},
        {QStringLiteral("Apagar / Cerrar"), &MainWindow::onDeactivate},
        {QStringLiteral("Nivel..."), &MainWindow::onSetLevel},
        {QStringLiteral("Simular fallo"), &MainWindow::onFault},
        {QStringLiteral("Reparar"), &MainWindow::onRepair}};
    for (const Btn& b : buttons) {
        QPushButton* button = makeButton(b.text, page);
        connect(button, &QPushButton::clicked, this, b.slot);
        row->addWidget(button);
    }
    row->addStretch(1);
    layout->addLayout(row);

    deviceTable_ = makeTable({QStringLiteral("Nombre"), QStringLiteral("Habitación"), QStringLiteral("Tipo"),
                              QStringLiteral("Estado"), QStringLiteral("Potencia (W)"), QStringLiteral("Nivel")},
                             page);
    connect(deviceTable_, &QTableWidget::cellDoubleClicked, this, [this](int, int) { onToggleRow(); });
    layout->addWidget(deviceTable_, 1);
    layout->addWidget(new QLabel(QStringLiteral("Doble clic sobre un dispositivo para alternar su estado."), page));
    return page;
}

QWidget* MainWindow::buildRoutinesTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* row = new QHBoxLayout;
    QPushButton* add = makeButton(QStringLiteral("Agregar"), page);
    QPushButton* edit = makeButton(QStringLiteral("Editar"), page);
    QPushButton* remove = makeButton(QStringLiteral("Eliminar"), page);
    connect(add, &QPushButton::clicked, this, &MainWindow::onAddRoutine);
    connect(edit, &QPushButton::clicked, this, &MainWindow::onEditRoutine);
    connect(remove, &QPushButton::clicked, this, &MainWindow::onRemoveRoutine);
    row->addWidget(add);
    row->addWidget(edit);
    row->addWidget(remove);
    row->addStretch(1);
    layout->addLayout(row);

    routineTable_ = makeTable({QStringLiteral("Nombre"), QStringLiteral("Hora"), QStringLiteral("Acción"),
                               QStringLiteral("Días"), QStringLiteral("Dispositivos"), QStringLiteral("Habilitada")},
                              page);
    layout->addWidget(routineTable_, 1);
    return page;
}

QWidget* MainWindow::buildHistoryTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* row = new QHBoxLayout;

    deviceFilter_ = new QComboBox(page);
    kindFilter_ = new QComboBox(page);
    kindFilter_->addItem(QStringLiteral("Todos los eventos"), QString());
    for (const char* k : {"REGISTRO", "EDICION", "ELIMINACION", "ENCENDIDO", "APAGADO", "NIVEL", "FALLO",
                          "REPARACION", "RUTINA"}) {
        kindFilter_->addItem(QString::fromLatin1(k), QString::fromLatin1(k));
    }
    QPushButton* export_ = makeButton(QStringLiteral("Exportar CSV..."), page);
    row->addWidget(new QLabel(QStringLiteral("Dispositivo:"), page));
    row->addWidget(deviceFilter_);
    row->addWidget(new QLabel(QStringLiteral("Tipo:"), page));
    row->addWidget(kindFilter_);
    row->addStretch(1);
    row->addWidget(export_);
    layout->addLayout(row);

    historyTable_ = makeTable({QStringLiteral("Fecha y hora"), QStringLiteral("Dispositivo"),
                               QStringLiteral("Evento"), QStringLiteral("Detalle"), QStringLiteral("Usuario")},
                              page);
    layout->addWidget(historyTable_, 1);

    connect(deviceFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { refreshHistory(); });
    connect(kindFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { refreshHistory(); });
    connect(export_, &QPushButton::clicked, this, &MainWindow::onExportCsv);
    return page;
}

// ------------------------------------------------------------------ Refrescos
void MainWindow::refreshAll() {
    refreshPanel();
    refreshDevices();
    refreshRoutines();
    refreshHistory();
}

void MainWindow::refreshPanel() {
    const std::vector<DeviceRecord> list = devices_.devices();
    const double tariff = tariff_->value();

    int on = 0, off = 0, fault = 0;
    double instantW = 0.0;
    for (const DeviceRecord& r : list) {
        if (r.state == DeviceState::On) {
            ++on;
            instantW += Device::create(r)->activePowerW();
        } else if (r.state == DeviceState::Off) {
            ++off;
        } else {
            ++fault;
        }
    }
    lblTotal_->setText(QStringLiteral("Dispositivos: %1").arg(static_cast<int>(list.size())));
    lblOn_->setText(QStringLiteral("Encendidos: %1").arg(on));
    lblFault_->setText(QStringLiteral("En fallo: %1").arg(fault));
    lblPower_->setText(QStringLiteral("Potencia instantánea: %1 W").arg(instantW, 0, 'f', 1));

    std::map<int, int64_t> seconds;
    for (const UsageRow& u : devices_.usage(30)) seconds[u.deviceId] = u.seconds;

    energyTable_->setRowCount(static_cast<int>(list.size()));
    QStringList labels;
    QVector<double> monthly;
    int row = 0;
    for (const DeviceRecord& r : list) {
        const EnergyEstimate e = estimateEnergy(r, tariff);
        energyTable_->setItem(row, 0, cell(qs(r.name)));
        energyTable_->setItem(row, 1, cell(QString::number(r.powerW, 'f', 1), true));
        energyTable_->setItem(row, 2, cell(QString::number(r.dailyHours, 'f', 1), true));
        energyTable_->setItem(row, 3, cell(QString::number(e.kwhPerDay, 'f', 3), true));
        energyTable_->setItem(row, 4, cell(QString::number(e.kwhPerMonth, 'f', 2), true));
        energyTable_->setItem(row, 5, cell(QString::number(e.costPerMonth, 'f', 0), true));
        energyTable_->setItem(row, 6, cell(QString::number(measuredKwh(seconds[r.id], r.powerW), 'f', 3), true));
        labels << qs(r.name);
        monthly.push_back(e.kwhPerMonth);
        ++row;
    }
    const EnergyEstimate total = estimateTotal(list, tariff);
    lblEnergy_->setText(QStringLiteral("Total estimado: %1 kWh/mes  |  $%2/mes")
                            .arg(total.kwhPerMonth, 0, 'f', 2)
                            .arg(total.costPerMonth, 0, 'f', 0));

    barChart_->setData(QStringLiteral("Consumo estimado mensual por dispositivo (kWh)"), labels, monthly);
    QVector<PieChartWidget::Slice> slices;
    slices.push_back({QStringLiteral("Encendidos"), static_cast<double>(on), QColor(39, 174, 96)});
    slices.push_back({QStringLiteral("Apagados"), static_cast<double>(off), QColor(149, 165, 166)});
    slices.push_back({QStringLiteral("En fallo"), static_cast<double>(fault), QColor(231, 76, 60)});
    pieChart_->setData(QStringLiteral("Estado de los dispositivos"), slices);
}

void MainWindow::refreshDevices() {
    const int keep = selectedDeviceId();
    const std::vector<DeviceRecord> list = devices_.devices();
    deviceTable_->setRowCount(static_cast<int>(list.size()));
    int row = 0;
    for (const DeviceRecord& r : list) {
        const auto dev = Device::create(r);
        auto* name = cell(qs(r.name));
        name->setData(Qt::UserRole, r.id);
        deviceTable_->setItem(row, 0, name);
        deviceTable_->setItem(row, 1, cell(qs(r.room)));
        deviceTable_->setItem(row, 2, cell(qs(label(r.type))));
        auto* state = cell(qs(label(r.state)));
        QColor bg(236, 240, 241);
        if (r.state == DeviceState::On) bg = QColor(171, 235, 198);
        else if (r.state == DeviceState::Fault) bg = QColor(245, 183, 177);
        state->setBackground(QBrush(bg));
        state->setForeground(QBrush(Qt::black));
        deviceTable_->setItem(row, 3, state);
        deviceTable_->setItem(row, 4, cell(QString::number(r.powerW, 'f', 1), true));
        deviceTable_->setItem(row, 5, cell(dev->supportsLevel() ? QStringLiteral("%1 %").arg(r.level) : QStringLiteral("-"), true));
        if (r.id == keep) deviceTable_->selectRow(row);
        ++row;
    }

    // Filtro del historial: lista de dispositivos actual (sin disparar el refresco en cascada).
    const int currentFilter = deviceFilter_->currentData().toInt();
    {
        const QSignalBlocker blocker(deviceFilter_);
        deviceFilter_->clear();
        deviceFilter_->addItem(QStringLiteral("Todos los dispositivos"), 0);
        for (const DeviceRecord& r : list) deviceFilter_->addItem(qs(r.name), r.id);
        const int idx = deviceFilter_->findData(currentFilter);
        deviceFilter_->setCurrentIndex(idx >= 0 ? idx : 0);
    }
}

void MainWindow::refreshRoutines() {
    const int keep = selectedRoutineId();
    const std::vector<Routine> list = routines_.list();
    routineTable_->setRowCount(static_cast<int>(list.size()));
    int row = 0;
    for (const Routine& r : list) {
        auto* name = cell(qs(r.name));
        name->setData(Qt::UserRole, r.id);
        routineTable_->setItem(row, 0, name);
        routineTable_->setItem(row, 1, cell(qs(formatMinute(r.minuteOfDay))));
        routineTable_->setItem(row, 2, cell(r.action == RoutineAction::TurnOn ? QStringLiteral("Encender") : QStringLiteral("Apagar")));
        routineTable_->setItem(row, 3, cell(qs(daysToString(r.daysMask))));
        routineTable_->setItem(row, 4, cell(QString::number(static_cast<int>(r.deviceIds.size()))));
        routineTable_->setItem(row, 5, cell(r.enabled ? QStringLiteral("Sí") : QStringLiteral("No")));
        if (r.id == keep) routineTable_->selectRow(row);
        ++row;
    }
}

void MainWindow::refreshHistory() {
    const int deviceId = deviceFilter_->currentData().toInt();
    const std::string kind = kindFilter_->currentData().toString().toStdString();
    const std::vector<Event> events = devices_.events(1000, deviceId, kind);
    historyTable_->setRowCount(static_cast<int>(events.size()));
    int row = 0;
    for (const Event& e : events) {
        historyTable_->setItem(row, 0, cell(qs(formatTimestamp(e.timestamp))));
        historyTable_->setItem(row, 1, cell(qs(e.deviceName)));
        historyTable_->setItem(row, 2, cell(qs(e.kind)));
        historyTable_->setItem(row, 3, cell(qs(e.message)));
        historyTable_->setItem(row, 4, cell(qs(e.username)));
        ++row;
    }
}

// ------------------------------------------------------------------ Utilidades
int MainWindow::selectedDeviceId() const {
    if (!deviceTable_) return 0;
    const int row = deviceTable_->currentRow();
    if (row < 0 || !deviceTable_->item(row, 0)) return 0;
    return deviceTable_->item(row, 0)->data(Qt::UserRole).toInt();
}

int MainWindow::selectedRoutineId() const {
    if (!routineTable_) return 0;
    const int row = routineTable_->currentRow();
    if (row < 0 || !routineTable_->item(row, 0)) return 0;
    return routineTable_->item(row, 0)->data(Qt::UserRole).toInt();
}

void MainWindow::report(const Result& r) {
    if (r.ok) statusBar()->showMessage(qs(r.message), 6000);
    else QMessageBox::warning(this, QStringLiteral("Operación no realizada"), qs(r.message));
    refreshAll();
}

// ------------------------------------------------------------------ Dispositivos
void MainWindow::onAddDevice() {
    DeviceDialog dlg(DeviceRecord{}, this);
    while (dlg.exec() == QDialog::Accepted) {
        const Result r = devices_.registerDevice(dlg.record(), auth_.currentUser());
        if (r.ok) {
            report(r);
            return;
        }
        QMessageBox::warning(this, QStringLiteral("No se pudo registrar"), qs(r.message));
    }
}

void MainWindow::onEditDevice() {
    const int id = selectedDeviceId();
    const auto rec = devices_.find(id);
    if (!rec) {
        QMessageBox::information(this, QStringLiteral("Editar"), QStringLiteral("Seleccione un dispositivo."));
        return;
    }
    DeviceDialog dlg(*rec, this);
    while (dlg.exec() == QDialog::Accepted) {
        const Result r = devices_.updateDevice(dlg.record(), auth_.currentUser());
        if (r.ok) {
            report(r);
            return;
        }
        QMessageBox::warning(this, QStringLiteral("No se pudo guardar"), qs(r.message));
    }
}

void MainWindow::onRemoveDevice() {
    const int id = selectedDeviceId();
    const auto rec = devices_.find(id);
    if (!rec) {
        QMessageBox::information(this, QStringLiteral("Eliminar"), QStringLiteral("Seleccione un dispositivo."));
        return;
    }
    const auto answer = QMessageBox::question(
        this, QStringLiteral("Eliminar dispositivo"),
        QStringLiteral("¿Eliminar «%1»? Se quitará también de las rutinas; el historial se conserva.").arg(qs(rec->name)));
    if (answer == QMessageBox::Yes) report(devices_.removeDevice(id, auth_.currentUser()));
}

void MainWindow::onActivate() {
    const int id = selectedDeviceId();
    if (id == 0) { statusBar()->showMessage(QStringLiteral("Seleccione un dispositivo."), 4000); return; }
    report(devices_.activate(id, auth_.currentUser()));
}

void MainWindow::onDeactivate() {
    const int id = selectedDeviceId();
    if (id == 0) { statusBar()->showMessage(QStringLiteral("Seleccione un dispositivo."), 4000); return; }
    report(devices_.deactivate(id, auth_.currentUser()));
}

void MainWindow::onToggleRow() {
    const int id = selectedDeviceId();
    const auto rec = devices_.find(id);
    if (!rec) return;
    report(rec->state == DeviceState::On ? devices_.deactivate(id, auth_.currentUser())
                                         : devices_.activate(id, auth_.currentUser()));
}

void MainWindow::onSetLevel() {
    const int id = selectedDeviceId();
    const auto rec = devices_.find(id);
    if (!rec) { statusBar()->showMessage(QStringLiteral("Seleccione un dispositivo."), 4000); return; }
    if (!Device::create(*rec)->supportsLevel()) {
        QMessageBox::information(this, QStringLiteral("Nivel"),
                                 QStringLiteral("Solo las luces y persianas admiten nivel."));
        return;
    }
    bool ok = false;
    const int level = QInputDialog::getInt(this, QStringLiteral("Nivel"),
                                           QStringLiteral("Nivel de «%1» (0-100 %):").arg(qs(rec->name)), rec->level,
                                           0, 100, 5, &ok);
    if (ok) report(devices_.setLevel(id, level, auth_.currentUser()));
}

void MainWindow::onFault() {
    const int id = selectedDeviceId();
    if (id == 0) { statusBar()->showMessage(QStringLiteral("Seleccione un dispositivo."), 4000); return; }
    report(devices_.reportFault(id, auth_.currentUser()));
}

void MainWindow::onRepair() {
    const int id = selectedDeviceId();
    if (id == 0) { statusBar()->showMessage(QStringLiteral("Seleccione un dispositivo."), 4000); return; }
    report(devices_.repair(id, auth_.currentUser()));
}

// ------------------------------------------------------------------ Rutinas
void MainWindow::onAddRoutine() {
    const std::vector<DeviceRecord> list = devices_.devices();
    if (list.empty()) {
        QMessageBox::information(this, QStringLiteral("Rutinas"), QStringLiteral("Registre primero algún dispositivo."));
        return;
    }
    RoutineDialog dlg(Routine{}, list, this);
    while (dlg.exec() == QDialog::Accepted) {
        const Result r = routines_.add(dlg.record(), auth_.currentUser());
        if (r.ok) {
            report(r);
            return;
        }
        QMessageBox::warning(this, QStringLiteral("No se pudo crear"), qs(r.message));
    }
}

void MainWindow::onEditRoutine() {
    const int id = selectedRoutineId();
    Routine current;
    bool found = false;
    for (const Routine& r : routines_.list()) {
        if (r.id == id) { current = r; found = true; }
    }
    if (!found) {
        QMessageBox::information(this, QStringLiteral("Rutinas"), QStringLiteral("Seleccione una rutina."));
        return;
    }
    RoutineDialog dlg(current, devices_.devices(), this);
    while (dlg.exec() == QDialog::Accepted) {
        const Result r = routines_.update(dlg.record(), auth_.currentUser());
        if (r.ok) {
            report(r);
            return;
        }
        QMessageBox::warning(this, QStringLiteral("No se pudo guardar"), qs(r.message));
    }
}

void MainWindow::onRemoveRoutine() {
    const int id = selectedRoutineId();
    if (id == 0) {
        QMessageBox::information(this, QStringLiteral("Rutinas"), QStringLiteral("Seleccione una rutina."));
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Eliminar rutina"), QStringLiteral("¿Eliminar la rutina seleccionada?")) ==
        QMessageBox::Yes) {
        report(routines_.remove(id, auth_.currentUser()));
    }
}

void MainWindow::onTick() {
    const std::vector<std::string> done = routines_.tick(localTimeFrom(systemClock()), std::string("sistema"));
    if (!done.empty()) {
        statusBar()->showMessage(qs(done.back()), 10000);
        refreshAll();
    }
}

// ------------------------------------------------------------------ Historial / sesión
void MainWindow::onExportCsv() {
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Exportar historial"),
                                                      QStringLiteral("historial.csv"),
                                                      QStringLiteral("Archivos CSV (*.csv)"));
    if (path.isEmpty()) return;
    const std::vector<Event> events =
        devices_.events(100000, deviceFilter_->currentData().toInt(), kindFilter_->currentData().toString().toStdString());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, QStringLiteral("Exportar"), QStringLiteral("No se pudo escribir el archivo."));
        return;
    }
    file.write("\xEF\xBB\xBF");  // BOM UTF-8 para que Excel muestre bien las tildes
    file.write(QByteArray::fromStdString(eventsToCsv(events)));
    file.close();
    statusBar()->showMessage(QStringLiteral("Historial exportado: %1 evento(s).").arg(static_cast<int>(events.size())), 6000);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    QApplication::quit();  // el cierre de esta ventana termina la aplicación (ver onLogout)
    QMainWindow::closeEvent(event);
}

void MainWindow::onLogout() {
    auth_.logout();
    hide();
    LoginDialog dlg(auth_);
    if (dlg.exec() == QDialog::Accepted) {
        updateTitle();
        refreshAll();
        show();
    } else {
        QApplication::quit();
    }
}

}  // namespace domotica
