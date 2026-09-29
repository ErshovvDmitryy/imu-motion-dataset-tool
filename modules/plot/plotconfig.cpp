#include "modules/plot/plotconfig.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <cmath>

QString xAxisModeName(XAxisMode mode) {
    switch (mode) {
    case XAxisMode::Index:     return QStringLiteral("index");
    case XAxisMode::Timestamp: return QStringLiteral("timestamp");
    case XAxisMode::HostTime:  return QStringLiteral("host");
    }
    return QStringLiteral("index");
}

XAxisMode xAxisModeFromName(const QString &name) {
    const QString key = name.trimmed().toLower();
    if (key == QLatin1String("timestamp")) return XAxisMode::Timestamp;
    if (key == QLatin1String("host"))      return XAxisMode::HostTime;
    return XAxisMode::Index;
}

bool PlotConfig::isValid(QString *error) const {
    if (schemaName.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("Pick a message type");
        return false;
    }
    if (traces.isEmpty()) {
        if (error) *error = QStringLiteral("Select at least one variable to draw");
        return false;
    }
    // xField необязателен: пустое значение означает "взять поле с ролью
    // Timestamp", поэтому дефолтные конфиги остаются валидными.
    if (liveWindowSec < 0.0) {
        if (error) *error = QStringLiteral("Window size cannot be negative");
        return false;
    }
    return true;
}

bool PlotConfig::hasField(const QString &fieldName) const {
    for (const PlotTrace &trace : traces) {
        if (trace.field == fieldName) {
            return true;
        }
    }
    return false;
}

int PlotConfig::traceCount() const {
    return traces.size();
}

QColor PlotConfig::defaultColorFor(int index) {
    // Разнесённый по hue круг: соседние сюжеты сразу различимы глазом.
    const int hue = static_cast<int>(std::fmod(index * 47.0, 360.0));
    return QColor::fromHsv(hue, 220, 255);
}

void PlotConfig::applyDefaultColors() {
    for (int i = 0; i < traces.size(); ++i) {
        if (traces.at(i).color.isValid() == false) {
            traces[i].color = defaultColorFor(i);
        }
    }
}

QJsonObject PlotConfig::toJson() const {
    QJsonObject object;
    object.insert(QStringLiteral("title"), title);
    object.insert(QStringLiteral("schema"), schemaName);
    object.insert(QStringLiteral("xMode"), xAxisModeName(xMode));
    object.insert(QStringLiteral("xField"), xField);
    object.insert(QStringLiteral("xLabel"), xLabel);
    object.insert(QStringLiteral("yLabel"), yLabel);
    object.insert(QStringLiteral("liveWindowSec"), liveWindowSec);
    object.insert(QStringLiteral("relativeTime"), relativeTime);
    object.insert(QStringLiteral("showLegend"), showLegend);
    object.insert(QStringLiteral("autoColors"), autoColors);

    QJsonArray array;
    for (const PlotTrace &trace : traces) {
        QJsonObject item;
        item.insert(QStringLiteral("field"), trace.field);
        if (trace.color.isValid()) {
            item.insert(QStringLiteral("color"), trace.color.name());
        }
        array.append(item);
    }
    object.insert(QStringLiteral("traces"), array);
    return object;
}

PlotConfig PlotConfig::fromJson(const QJsonObject &object) {
    PlotConfig config;
    config.title = object.value(QStringLiteral("title")).toString();
    config.schemaName = object.value(QStringLiteral("schema")).toString();
    config.xMode = xAxisModeFromName(object.value(QStringLiteral("xMode")).toString());
    config.xField = object.value(QStringLiteral("xField")).toString();
    config.xLabel = object.value(QStringLiteral("xLabel")).toString(config.xLabel);
    config.yLabel = object.value(QStringLiteral("yLabel")).toString(config.yLabel);
    config.liveWindowSec = object.value(QStringLiteral("liveWindowSec")).toDouble(config.liveWindowSec);
    config.relativeTime = object.value(QStringLiteral("relativeTime")).toBool(config.relativeTime);
    config.showLegend = object.value(QStringLiteral("showLegend")).toBool(config.showLegend);
    config.autoColors = object.value(QStringLiteral("autoColors")).toBool(config.autoColors);

    const QJsonArray array = object.value(QStringLiteral("traces")).toArray();
    for (const QJsonValue &entry : array) {
        const QJsonObject item = entry.toObject();
        PlotTrace trace;
        trace.field = item.value(QStringLiteral("field")).toString();
        const QString color = item.value(QStringLiteral("color")).toString();
        if (color.isEmpty() == false) {
            trace.color = QColor(color);
        }
        config.traces.append(trace);
    }
    return config;
}

// ============================== PlotConfigStore ==============================

PlotConfigStore::PlotConfigStore(QObject *parent)
    : QObject(parent)
{
    m_filePath = QCoreApplication::applicationDirPath() + QStringLiteral("/plots.json");
}

void PlotConfigStore::setFilePath(const QString &path) {
    m_filePath = path;
}

QString PlotConfigStore::filePath() const {
    return m_filePath;
}

void PlotConfigStore::load() {
    m_configs.clear();

    QFile file(m_filePath);
    if (file.exists() == false || file.open(QIODevice::ReadOnly) == false) {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (document.isObject() == false) {
        emit logMessage(QStringLiteral("plots.json is broken: %1").arg(parseError.errorString()));
        return;
    }

    const QJsonObject plots = document.object().value(QStringLiteral("plots")).toObject();
    for (auto it = plots.constBegin(); it != plots.constEnd(); ++it) {
        m_configs.insert(it.key(), PlotConfig::fromJson(it.value().toObject()));
    }
    emit configsChanged();
}

bool PlotConfigStore::save(QString *error) {
    QJsonObject plots;
    for (auto it = m_configs.constBegin(); it != m_configs.constEnd(); ++it) {
        plots.insert(it.key(), it.value().toJson());
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("plots"), plots);

    const QFileInfo info(m_filePath);
    if (QDir().mkpath(info.absolutePath()) == false) {
        if (error) *error = QStringLiteral("Cannot create %1").arg(info.absolutePath());
        return false;
    }

    QSaveFile file(m_filePath);
    if (file.open(QIODevice::WriteOnly) == false) {
        if (error) *error = file.errorString();
        return false;
    }
    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) {
        if (error) *error = file.errorString();
        return false;
    }
    if (file.commit() == false) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

bool PlotConfigStore::has(const QString &slotId) const {
    return m_configs.contains(slotId);
}

PlotConfig PlotConfigStore::config(const QString &slotId) const {
    return m_configs.value(slotId);
}

void PlotConfigStore::setConfig(const QString &slotId, const PlotConfig &config) {
    m_configs.insert(slotId, config);
}

void PlotConfigStore::remove(const QString &slotId) {
    m_configs.remove(slotId);
}

QStringList PlotConfigStore::slotIds() const {
    return m_configs.keys();
}
