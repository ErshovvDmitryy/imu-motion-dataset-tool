#include "modules/widgets/plotmanager.h"

#include <QEvent>
#include <QMouseEvent>

#include "modules/widgets/plotconfigdialog.h"
#include "qcustomplot/qcustomplot.h"

#include <algorithm>

PlotManager::PlotManager(QWidget *parent)
    : QWidget(parent)
{
    setVisible(false);
}

PlotManager::~PlotManager()
{
}

void PlotManager::setSchemas(const QList<DataSchema> &schemas) {
    m_schemas.clear();
    for (const DataSchema &schema : schemas) {
        m_schemas.insert(schema.name, QSharedPointer<const DataSchema>(new DataSchema(schema)));
    }
}

QList<DataSchema> PlotManager::schemas() const {
    QList<DataSchema> result;
    for (const auto &entry : m_schemas) {
        result.append(*entry);
    }
    std::sort(result.begin(), result.end(), [](const DataSchema &a, const DataSchema &b) {
        return a.typeId < b.typeId;
    });
    return result;
}

DataSchema PlotManager::schema(const QString &name) const {
    const auto found = m_schemas.constFind(name);
    return found == m_schemas.constEnd() ? DataSchema() : *found.value();
}

void PlotManager::setConfigStore(PlotConfigStore *store) {
    m_store = store;
}

PlotManager::PlotState *PlotManager::stateFor(QCustomPlot &plot) {
    const auto found = m_states.find(&plot);
    if (found != m_states.end()) {
        return &found.value();
    }
    emit logMessage(LogLevel::Warning,
                    QStringLiteral("PlotManager: plot %1 was not created by PlotManager")
                        .arg(plot.objectName()));
    return nullptr;
}

QString PlotManager::traceName(const DataSchema *schema, const QString &field) {
    if (schema == nullptr) {
        return field;
    }
    return schema->slotLabel(field);
}

QPen PlotManager::penForMarker(MarkerType type, const QColor &color) {
    QColor resolved = color;
    switch (type) {
    case MarkerType::Separator:
        if (resolved.isValid() == false) resolved = QColor(Qt::blue);
        return QPen(resolved, 2, Qt::DashLine);
    case MarkerType::TrimStart:
        if (resolved.isValid() == false) resolved = QColor(Qt::green);
        return QPen(resolved, 2, Qt::SolidLine);
    case MarkerType::TrimEnd:
        if (resolved.isValid() == false) resolved = QColor(Qt::red);
        return QPen(resolved, 2, Qt::SolidLine);
    case MarkerType::FlagStart:
        if (resolved.isValid() == false) resolved = QColor(255, 165, 0);
        return QPen(resolved, 2, Qt::DashLine);
    case MarkerType::FlagEnd:
        if (resolved.isValid() == false) resolved = QColor(0, 191, 255);
        return QPen(resolved, 2, Qt::DashLine);
    }
    return QPen(Qt::red, 2, Qt::SolidLine);
}

QCustomPlot *PlotManager::createPlot(const QString &slotId, const PlotConfig &config,
                                     int minWidth, int minHeight) {
    auto *plot = new QCustomPlot(this);
    plot->setObjectName(slotId);
    plot->setMinimumSize(minWidth, minHeight);
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    plot->setBackground(QBrush(QColor(28, 28, 30)));
    plot->setNoAntialiasingOnDrag(true);

    QCPAxis *axis = plot->xAxis;
    axis->setBasePen(QPen(QColor(120, 120, 120)));
    axis->setTickPen(QPen(QColor(120, 120, 120)));
    axis->setSubTickPen(QPen(QColor(90, 90, 90)));
    axis->setTickLabelColor(QColor(200, 200, 200));
    axis->setLabelColor(QColor(200, 200, 200));
    axis->grid()->setPen(QPen(QColor(50, 50, 50, 120)));
    axis->grid()->setSubGridVisible(false);

    QVector<QCPAxis *> axes{ axis, plot->yAxis, plot->xAxis2, plot->yAxis2 };
    for (QCPAxis *each : axes) {
        each->grid()->setPen(QPen(QColor(50, 50, 50, 120)));
        each->grid()->setSubGridVisible(false);
    }
    plot->yAxis->grid()->setPen(QPen(QColor(50, 50, 50, 120)));

    plot->installEventFilter(this);

    PlotState state;
    state.slotId = slotId;
    m_states.insert(plot, state);
    applyConfig(*plot, config);
    return plot;
}

void PlotManager::buildGraphs(QCustomPlot &plot, PlotState &state) {
    for (int i = plot.graphCount() - 1; i >= 0; --i) {
        plot.removeGraph(i);
    }

    PlotConfig config = state.config;
    if (config.autoColors) {
        config.applyDefaultColors();
    }

    const QSharedPointer<const DataSchema> schema = m_schemas.value(config.schemaName);
    for (const PlotTrace &trace : config.traces) {
        plot.addGraph();
        QCPGraph *graph = plot.graph(plot.graphCount() - 1);
        graph->setPen(QPen(trace.color, 1.5));
        graph->setName(traceName(schema.data(), trace.field));
    }

    plot.xAxis->setLabel(config.xLabel);
    plot.yAxis->setLabel(config.yLabel);
    const bool legendVisible = config.showLegend && plot.graphCount() > 1;
    plot.legend->setVisible(legendVisible);
    if (legendVisible) {
        plot.legend->setBrush(QBrush(QColor(40, 40, 42, 220)));
        plot.legend->setTextColor(Qt::white);
    }
}

bool PlotManager::applyConfig(QCustomPlot &plot, const PlotConfig &config) {
    PlotState *state = stateFor(plot);
    if (state == nullptr) {
        return false;
    }

    QString error;
    if (config.isValid(&error) == false) {
        emit logMessage(LogLevel::Warning,
                        QStringLiteral("PlotManager: invalid config for \"%1\": %2")
                            .arg(state->slotId, error));
        return false;
    }

    clearAllMarkers(plot);
    state->config = config;
    state->pointCount = 0;
    state->originX = 0.0;
    state->originSet = false;
    buildGraphs(plot, *state);
    plot.replot(QCustomPlot::rpQueuedReplot);
    return true;
}

PlotConfig PlotManager::config(QCustomPlot &plot) const {
    const auto found = m_states.constFind(&plot);
    return found == m_states.constEnd() ? PlotConfig() : found.value().config;
}

double PlotManager::xValueFor(const PlotConfig &config, const DataFrame &frame, int index) {
    switch (config.xMode) {
    case XAxisMode::Index:
        return index;
    case XAxisMode::Timestamp: {
        // Явно выбранное поле X важнее роли: иначе диалог позволяет
        // выбрать поле, а график всё равно строился бы по роли.
        if (config.xField.isEmpty() == false) {
            bool ok = false;
            const double value = frame.toDouble(config.xField, &ok);
            if (ok) {
                return value;
            }
        }
        bool ok = false;
        const double seconds = frame.timeSeconds(&ok);
        return ok ? seconds : index;
    }
    case XAxisMode::HostTime:
        return frame.hostTimeMs() / 1000.0;
    }
    return index;
}

double PlotManager::nextX(PlotState &state, const DataFrame &frame) {
    const int index = state.pointCount;
    ++state.pointCount;

    double x = xValueFor(state.config, frame, index);
    if (state.config.relativeTime) {
        if (state.originSet == false) {
            state.originX = x;
            state.originSet = true;
        }
        x -= state.originX;
    }
    return x;
}

bool PlotManager::appendFrame(QCustomPlot &plot, const DataFrame &frame) {
    PlotState *state = stateFor(plot);
    if (state == nullptr || state->config.traces.isEmpty()) {
        return false;
    }
    if (state->config.schemaName != frame.schemaName()) {
        return false;
    }

    state->live = true;
    const double x = nextX(*state, frame);

    for (int i = 0; i < state->config.traces.size(); ++i) {
        if (i >= plot.graphCount()) {
            break;
        }
        bool ok = false;
        const double value = frame.toDouble(state->config.traces.at(i).field, &ok);
        if (ok) {
            plot.graph(i)->addData(x, value);
        }
    }
    return true;
}

void PlotManager::syncAxes(QCustomPlot &plot, PlotState &state) {
    if (state.config.liveWindowSec > 0.0 && state.live) {
        const int last = plot.graphCount() > 0 ? plot.graph(0)->data()->size() - 1 : -1;
        if (last >= 0) {
            const double x = plot.graph(0)->data()->at(last)->key;
            plot.xAxis->setRange(x - state.config.liveWindowSec, x, Qt::AlignRight);
            plot.yAxis->rescale(true);
            return;
        }
    }

    for (int i = 0; i < plot.graphCount(); ++i) {
        plot.graph(i)->rescaleAxes(true);
    }
    if (state.config.relativeTime && plot.graphCount() > 0) {
        const int last = plot.graph(0)->data()->size() - 1;
        if (last >= 0) {
            plot.xAxis->setRange(0.0, plot.graph(0)->data()->at(last)->key, Qt::AlignLeft);
        }
    }
    plot.yAxis->rescale(true);
}

void PlotManager::refreshLive(QCustomPlot &plot) {
    PlotState *state = stateFor(plot);
    if (state == nullptr) {
        return;
    }
    syncAxes(plot, *state);
    plot.replot(QCustomPlot::rpQueuedReplot);
}

void PlotManager::paintFrames(QCustomPlot &plot, const QVector<DataFrame> &frames) {
    PlotState *state = stateFor(plot);
    if (state == nullptr) {
        return;
    }

    state->live = false;
    state->pointCount = 0;
    state->originX = 0.0;
    state->originSet = false;
    for (int i = 0; i < plot.graphCount(); ++i) {
        plot.graph(i)->data()->clear();
    }

    for (int index = 0; index < frames.size(); ++index) {
        appendFrame(plot, frames.at(index));
    }
    state->live = false;
    refreshLive(plot);
}

void PlotManager::clearPlot(QCustomPlot &plot) {
    PlotState *state = stateFor(plot);
    if (state == nullptr) {
        return;
    }
    state->pointCount = 0;
    state->originX = 0.0;
    state->originSet = false;
    state->live = false;
    for (int i = 0; i < plot.graphCount(); ++i) {
        plot.graph(i)->data()->clear();
    }
    clearAllMarkers(plot);
    plot.replot(QCustomPlot::rpQueuedReplot);
}

bool PlotManager::isEmpty(QCustomPlot &plot) const {
    const auto found = m_states.constFind(&plot);
    if (found == m_states.constEnd() || plot.graphCount() == 0) {
        return true;
    }
    return plot.graph(0)->data()->isEmpty();
}

void PlotManager::addMarker(QCustomPlot &plot, double x, MarkerType type, const QColor &color) {
    PlotState *state = stateFor(plot);
    if (state == nullptr) {
        return;
    }

    auto *line = new QCPItemStraightLine(&plot);
    line->point1->setCoords(x, 0);
    line->point2->setCoords(x, 1);
    line->setPen(penForMarker(type, color));

    state->markers[static_cast<int>(type)].append(line);
    plot.replot(QCustomPlot::rpQueuedReplot);
}

bool PlotManager::setMarkerX(QCustomPlot &plot, MarkerType type, double x) {
    PlotState *state = stateFor(plot);
    if (state == nullptr) {
        return false;
    }
    QList<QCPItemStraightLine *> &lines = state->markers[static_cast<int>(type)];
    if (lines.isEmpty()) {
        return false;
    }
    for (QCPItemStraightLine *line : lines) {
        line->point1->setCoords(x, 0);
        line->point2->setCoords(x, 1);
    }
    plot.replot(QCustomPlot::rpQueuedReplot);
    return true;
}

void PlotManager::clearMarkers(QCustomPlot &plot, MarkerType type) {
    PlotState *state = stateFor(plot);
    if (state == nullptr) {
        return;
    }
    QList<QCPItemStraightLine *> &lines = state->markers[static_cast<int>(type)];
    for (QCPItemStraightLine *line : lines) {
        if (line != nullptr && line->parentPlot() != nullptr) {
            line->parentPlot()->removeItem(line);
        }
    }
    lines.clear();
    plot.replot(QCustomPlot::rpQueuedReplot);
}

void PlotManager::clearAllMarkers(QCustomPlot &plot) {
    PlotState *state = stateFor(plot);
    if (state == nullptr) {
        return;
    }
    for (auto it = state->markers.begin(); it != state->markers.end(); ++it) {
        for (QCPItemStraightLine *line : it.value()) {
            if (line != nullptr && line->parentPlot() != nullptr) {
                line->parentPlot()->removeItem(line);
            }
        }
    }
    state->markers.clear();
}

bool PlotManager::hasMarker(QCustomPlot &plot, MarkerType type) const {
    const auto found = m_states.constFind(&plot);
    if (found == m_states.constEnd()) {
        return false;
    }
    return found.value().markers.value(static_cast<int>(type)).isEmpty() == false;
}

bool PlotManager::dataXRange(QCustomPlot &plot, double *lo, double *hi) const {
    if (plot.graphCount() == 0) {
        return false;
    }
    const QSharedPointer<QCPDataContainer<QCPGraphData>> data = plot.graph(0)->data();
    if (data->isEmpty()) {
        return false;
    }
    if (lo) *lo = data->at(0)->key;
    if (hi) *hi = data->at(data->size() - 1)->key;
    return true;
}

double PlotManager::timeOrigin(QCustomPlot &plot) const {
    const auto found = m_states.constFind(&plot);
    if (found == m_states.constEnd()) {
        return 0.0;
    }
    return found.value().originSet ? found.value().originX : 0.0;
}

double PlotManager::plotXToAbsolute(QCustomPlot &plot, double plotX) const {
    return plotX + timeOrigin(plot);
}

double PlotManager::absoluteToPlotX(QCustomPlot &plot, double absolute) const {
    return absolute - timeOrigin(plot);
}

void PlotManager::setRangeDragEnabled(QCustomPlot &plot, bool enabled) {
    plot.setInteraction(QCP::iRangeDrag, enabled);
}

bool PlotManager::isRangeDragEnabled(QCustomPlot &plot) const {
    return plot.interactions().testFlag(QCP::iRangeDrag);
}

bool PlotManager::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::ContextMenu) {
        auto *plot = qobject_cast<QCustomPlot *>(watched);
        if (plot != nullptr && m_states.contains(plot)) {
            openConfigDialog(*plot);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void PlotManager::openConfigDialog(QCustomPlot &plot) {
    if (m_dialog == nullptr) {
        m_dialog = new PlotConfigDialog(this);
        m_dialog->setAttribute(Qt::WA_DeleteOnClose, false);
    }
    m_dialog->setSchemas(schemas());
    m_dialog->setConfig(config(plot));
    m_dialog->setWindowTitle(QStringLiteral("Plot setup: %1").arg(plot.objectName()));

    if (m_dialog->exec() != QDialog::Accepted) {
        return;
    }

    const PlotConfig updated = m_dialog->config();
    if (applyConfig(plot, updated) == false) {
        return;
    }
    if (m_store != nullptr) {
        m_store->setConfig(m_states.value(&plot).slotId, updated);
        QString error;
        if (m_store->save(&error) == false) {
            emit logMessage(LogLevel::Warning,
                            QStringLiteral("Cannot save plot setup: %1").arg(error));
        }
    }
    emit plotConfigured(m_states.value(&plot).slotId, updated);
}
