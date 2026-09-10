#include "plotmanager.h"
#include "qcustomplot/qcustomplot.h"

PlotManager::PlotManager(QObject *parent)
    : QWidget()
{
    Q_UNUSED(parent)
}

QCustomPlot* PlotManager::setupGraph(int totalGraphs,
                                      const QString &xLabel,
                                      const QString &yLabel,
                                      int minWidth, int minHeight) {
    auto *plot = new QCustomPlot(this);
    plot->setMinimumSize(minWidth, minHeight);
    plot->xAxis->setLabel(xLabel);
    plot->yAxis->setLabel(yLabel);

    for (int i = 0; i < totalGraphs; ++i) {
        plot->addGraph();
        plot->graph(i)->setPen(QColor::fromHsv(i % 360, 255, 255));
    }

    PlotInfo info;
    info.totalGraph = totalGraphs;
    mapPlotInfo.insert(plot, info);

    return plot;
}

void PlotManager::addToGraph(QCustomPlot &plot, const MotionSample &sample) {
    const double t = timeUINT32toFloat(sample.time);
    for (int i = 0; i < sample.accel.size(); ++i) {
        plot.graph(i)->addData(t, sample.accel.at(i));
    }
    plot.replot(QCustomPlot::rpQueuedReplot);
}

void PlotManager::paintGraph(QCustomPlot &plot, const QVector<MotionSample> &motionSaved) {
    if (motionSaved.isEmpty()) return;

    clearGraph(plot);
    for (const MotionSample &sample : motionSaved) {
        addToGraph(plot, sample);
    }
}

void PlotManager::clearGraph(QCustomPlot &plot) {
    auto it = mapPlotInfo.find(&plot);
    if (it == mapPlotInfo.end()) return;

    PlotInfo &info = it.value();

    for (int i = 0; i < info.totalGraph; i++) {
        plot.graph(i)->data()->clear();
    }

    info.m_separatorLines.clear();
    info.m_dynamicLines.clear();
    info.m_flags.clear();

    plot.replot(QCustomPlot::rpQueuedReplot);
}

void PlotManager::clearSeparators(QCustomPlot &plot) {
    auto it = mapPlotInfo.find(&plot);
    if (it == mapPlotInfo.end()) return;

    for (auto *line : it.value().m_separatorLines) {
        if (line->parentPlot()) line->parentPlot()->removeItem(line);
    }
    it.value().m_separatorLines.clear();

    plot.replot(QCustomPlot::rpQueuedReplot);
}

QPen PlotManager::penForType(SeporatorTypes type, const QColor &color) {
    switch (type) {
        case SeporatorTypes::SEPARATOR_LINE:    return QPen(color, 2, Qt::SolidLine);
        case SeporatorTypes::SEPARATOR_DASHED:  return QPen(color, 2, Qt::DashLine);
        case SeporatorTypes::SEPARATOR_POINTS:  return QPen(color, 2, Qt::DotLine);
        case SeporatorTypes::SEPARATOR_FLAG:    return QPen(color, 2, Qt::DashDotLine);
        case SeporatorTypes::SEPARATOR_NONE:
        default:                                return QPen(Qt::NoPen);
    }
}

void PlotManager::addSeparator(QCustomPlot &plot, double x,
                                SeporatorTypes type, const QColor &color) {
    if (type == SeporatorTypes::SEPARATOR_NONE) return;

    const QPen pen = penForType(type, color);
    if (pen.style() == Qt::NoPen) return;

    addLineToPlot(plot, x, pen, &PlotInfo::m_separatorLines);
}

void PlotManager::addLineToPlot(QCustomPlot &plot, double x, const QPen &pen,
                                 QList<QCPItemStraightLine*> PlotInfo::*listMember) {
    auto it = mapPlotInfo.find(&plot);
    if (it == mapPlotInfo.end()) {
        logMessage(LogLevel::Warning, QString("PlotManager: plot not registered!"));
        return;
    }

    auto *line = new QCPItemStraightLine(&plot);
    line->point1->setCoords(x, 0);
    line->point2->setCoords(x, 1);
    line->setPen(pen);

    (it.value().*listMember).append(line);
}
