/**
 * @file MainWindow.h
 * @brief Ventana principal: panel de control, dispositivos, rutinas e historial.
 */
#pragma once

#include <QMainWindow>

#include "../core/AuthService.h"
#include "../core/DeviceService.h"

class QCloseEvent;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QTabWidget;
class QTableWidget;
class QTimer;

namespace domotica {

class BarChartWidget;
class PieChartWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(AuthService& auth, DeviceService& devices, RoutineService& routines, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onAddDevice();
    void onEditDevice();
    void onRemoveDevice();
    void onActivate();
    void onDeactivate();
    void onToggleRow();
    void onSetLevel();
    void onFault();
    void onRepair();
    void onAddRoutine();
    void onEditRoutine();
    void onRemoveRoutine();
    void onExportCsv();
    void onLogout();
    void onTick();
    void refreshAll();

private:
    QWidget* buildPanelTab();
    QWidget* buildDevicesTab();
    QWidget* buildRoutinesTab();
    QWidget* buildHistoryTab();
    void refreshPanel();
    void refreshDevices();
    void refreshRoutines();
    void refreshHistory();
    int selectedDeviceId() const;
    int selectedRoutineId() const;
    void report(const Result& r);
    void updateTitle();

    AuthService& auth_;
    DeviceService& devices_;
    RoutineService& routines_;

    QTabWidget* tabs_ = nullptr;
    // Panel
    QLabel* lblTotal_ = nullptr;
    QLabel* lblOn_ = nullptr;
    QLabel* lblFault_ = nullptr;
    QLabel* lblPower_ = nullptr;
    QLabel* lblEnergy_ = nullptr;
    QDoubleSpinBox* tariff_ = nullptr;
    BarChartWidget* barChart_ = nullptr;
    PieChartWidget* pieChart_ = nullptr;
    QTableWidget* energyTable_ = nullptr;
    // Dispositivos / rutinas / historial
    QTableWidget* deviceTable_ = nullptr;
    QTableWidget* routineTable_ = nullptr;
    QTableWidget* historyTable_ = nullptr;
    QComboBox* deviceFilter_ = nullptr;
    QComboBox* kindFilter_ = nullptr;
    QTimer* routineTimer_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
};

}  // namespace domotica
