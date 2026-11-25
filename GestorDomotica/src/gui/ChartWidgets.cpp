#include "ChartWidgets.h"

#include <QFontMetrics>
#include <QPainter>
#include <algorithm>

namespace domotica {

BarChartWidget::BarChartWidget(QWidget* parent) : QWidget(parent) {}

void BarChartWidget::setData(const QString& title, const QStringList& labels, const QVector<double>& values) {
    title_ = title;
    labels_ = labels;
    values_ = values;
    update();
}

void BarChartWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().base());
    p.setPen(palette().text().color());
    QFont bold = font();
    bold.setBold(true);
    p.setFont(bold);
    p.drawText(QRect(0, 4, width(), 20), Qt::AlignCenter, title_);
    p.setFont(font());

    if (values_.isEmpty()) {
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("Sin datos"));
        return;
    }
    const QRect area = rect().adjusted(12, 34, -12, -34);
    double maxV = *std::max_element(values_.begin(), values_.end());
    if (maxV <= 0) maxV = 1;
    const int n = values_.size();
    const double slot = static_cast<double>(area.width()) / n;
    const QFontMetrics fm(font());

    p.drawLine(area.left(), area.bottom(), area.right(), area.bottom());
    for (int i = 0; i < n; ++i) {
        const double h = values_[i] / maxV * (area.height() - 16);
        const QRectF bar(area.left() + i * slot + slot * 0.18, area.bottom() - h, slot * 0.64, h);
        p.fillRect(bar, QColor(52, 152, 219));
        p.drawText(QRectF(area.left() + i * slot, bar.top() - 16, slot, 14), Qt::AlignCenter,
                   QString::number(values_[i], 'f', 2));
        const QString text = fm.elidedText(i < labels_.size() ? labels_[i] : QString(), Qt::ElideRight,
                                           static_cast<int>(slot) - 2);
        p.drawText(QRectF(area.left() + i * slot, area.bottom() + 4, slot, 16), Qt::AlignCenter, text);
    }
}

PieChartWidget::PieChartWidget(QWidget* parent) : QWidget(parent) {}

void PieChartWidget::setData(const QString& title, const QVector<Slice>& slices) {
    title_ = title;
    slices_ = slices;
    update();
}

void PieChartWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().base());
    p.setPen(palette().text().color());
    QFont bold = font();
    bold.setBold(true);
    p.setFont(bold);
    p.drawText(QRect(0, 4, width(), 20), Qt::AlignCenter, title_);
    p.setFont(font());

    double total = 0;
    for (const Slice& s : slices_) total += s.value;
    if (total <= 0) {
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("Sin datos"));
        return;
    }
    const int size = std::min(width() / 2, height() - 40);
    const QRectF pie(12, 30, size, size);
    int start = 90 * 16;  // arranca arriba
    for (const Slice& s : slices_) {
        if (s.value <= 0) continue;
        const int span = static_cast<int>(-s.value / total * 360.0 * 16.0);
        p.setBrush(s.color);
        p.setPen(Qt::NoPen);
        p.drawPie(pie, start, span);
        start += span;
    }
    p.setPen(palette().text().color());
    int y = 40;
    const int x = static_cast<int>(pie.right()) + 16;
    for (const Slice& s : slices_) {
        p.fillRect(QRect(x, y, 12, 12), s.color);
        p.drawText(QRect(x + 18, y - 2, width() - x - 20, 16), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1: %2").arg(s.label).arg(static_cast<int>(s.value)));
        y += 22;
    }
}

}  // namespace domotica
