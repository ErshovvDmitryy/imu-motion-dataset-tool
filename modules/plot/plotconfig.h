#pragma once

#include <QColor>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include "models/dataschema.h"

// Откуда брать значение оси X.
enum class XAxisMode : int {
    Index = 0,     // порядковый номер кадра
    Timestamp,     // поле с ролью Timestamp в сообщении
    HostTime       // время приёма на стороне ПК
};

QString xAxisModeName(XAxisMode mode);
XAxisMode xAxisModeFromName(const QString &name);

// Один сюжет на графике: какая переменная сообщения на него выведена.
struct PlotTrace {
    QString field;   // развёрнутое имя: "ax" или "values[3]"
    QColor color;
};

// Настройка одного графика: какое сообщение на него подаётся и какие
// его переменные видно. Именно её редактирует диалог по правому клику.
struct PlotConfig {
    QString title;
    QString schemaName;               // имя сообщения-источника
    XAxisMode xMode = XAxisMode::Index;
    QString xField;                   // только для xMode == Timestamp
    QList<PlotTrace> traces;

    QString xLabel = QStringLiteral("Time (s)");
    QString yLabel = QStringLiteral("Value");

    // Окно отображения для живого потока в секундах. 0 - показывать всё.
    double liveWindowSec = 5.0;
    // Вычитать первое значение X, чтобы кривая начиналась с нуля.
    bool relativeTime = true;
    bool showLegend = true;
    // Автоподбор цветов по порядковому номеру сюжета, если цвет не задан.
    bool autoColors = true;

    bool isValid(QString *error = nullptr) const;
    bool hasField(const QString &fieldName) const;
    int traceCount() const;

    QJsonObject toJson() const;
    static PlotConfig fromJson(const QJsonObject &object);

    static QColor defaultColorFor(int index);
    // Подставить цвета по умолчанию, не заданные явно.
    void applyDefaultColors();
};

// Хранилище настроек графиков в файле plots.json рядом с приложением.
class PlotConfigStore : public QObject
{
    Q_OBJECT

public:
    explicit PlotConfigStore(QObject *parent = nullptr);

    void setFilePath(const QString &path);
    QString filePath() const;

    void load();
    bool save(QString *error = nullptr);

    // Настройки по идентификатору графика (обычно имя виджета-владельца).
    bool has(const QString &slotId) const;
    PlotConfig config(const QString &slotId) const;
    void setConfig(const QString &slotId, const PlotConfig &config);
    void remove(const QString &slotId);
    QStringList slotIds() const;

signals:
    void configsChanged();
    void logMessage(const QString &text);

private:
    QString m_filePath;
    QHash<QString, PlotConfig> m_configs;
};
