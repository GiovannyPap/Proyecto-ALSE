/**
 * @file ChartWidgets.h
 * @brief Gráficas ligeras dibujadas con QPainter (sin depender de QtCharts).
 */
#pragma once

#include <QColor>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

namespace domotica {

/// Gráfica de barras verticales con etiquetas.
class BarChartWidget : public QWidget {
public:
    explicit BarChartWidget(QWidget* parent = nullptr);
    void setData(const QString& title, const QStringList& labels, const QVector<double>& values);
    QSize minimumSizeHint() const override { return QSize(260, 200); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString title_;
    QStringList labels_;
    QVector<double> values_;
};

/// Gráfica circular con leyenda.
class PieChartWidget : public QWidget {
public:
    struct Slice {
        QString label;
        double value;
        QColor color;
    };
    explicit PieChartWidget(QWidget* parent = nullptr);
    void setData(const QString& title, const QVector<Slice>& slices);
    QSize minimumSizeHint() const override { return QSize(260, 200); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString title_;
    QVector<Slice> slices_;
};

}  // namespace domotica
