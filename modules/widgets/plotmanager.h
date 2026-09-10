#pragma once

#include <QObject>
#include <QWidget>
#include <QHash>
#include <QList>
#include <QPen>
#include <QColor>

#include "models/motionsample.h"
#include "models/loglevel.h"

class QCustomPlot;
class QCPItemStraightLine;

enum class SeporatorTypes : int {
    SEPARATOR_NONE = 0,
    SEPARATOR_LINE,
    SEPARATOR_DASHED,
    SEPARATOR_POINTS,
    SEPARATOR_FLAG
};

struct PlotInfo {
    QList<QCPItemStraightLine*> m_separatorLines;
    QList<QCPItemStraightLine*> m_dynamicLines;
    QList<QCPItemStraightLine*> m_flags;
    int totalGraph = 0;
};

class PlotManager : public QWidget
{
    Q_OBJECT

public:
    explicit PlotManager(QObject *parent = nullptr);

    QCustomPlot* setupGraph(int totalGraphs,
                            const QString &xLabel,
                            const QString &yLabel,
                            int minWidth = 200, int minHeight = 250);

    void addToGraph(QCustomPlot &plot, const MotionSample &sample);
    void paintGraph(QCustomPlot &plot, const QVector<MotionSample> &motionSaved);
    void clearGraph(QCustomPlot &plot);

    void addSeparator(QCustomPlot &plot, double x,
                      SeporatorTypes type, const QColor &color = Qt::red);
    void clearSeparators(QCustomPlot &plot);

signals:
    void logMessage(LogLevel level, const QString &text);

private:
    void addLineToPlot(QCustomPlot &plot, double x, const QPen &pen,
                       QList<QCPItemStraightLine*> PlotInfo::*listMember);

    static QPen penForType(SeporatorTypes type, const QColor &color);
    static float timeUINT32toFloat(uint32_t time) { return time / 1000000.0f; }

    QHash<QCustomPlot*, PlotInfo> mapPlotInfo;
};
