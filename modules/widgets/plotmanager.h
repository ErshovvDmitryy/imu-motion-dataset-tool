#pragma once

#include <QHash>
#include <QList>
#include <QSharedPointer>
#include <QString>
#include <QVector>
#include <QWidget>

#include "models/datapacket.h"
#include "models/dataschema.h"
#include "models/loglevel.h"
#include "modules/plot/plotconfig.h"

class QCustomPlot;
class QCPItemStraightLine;
class PlotConfigStore;
class PlotConfigDialog;

// Смысловые типы вертикальных маркеров на графике. Цвет и начертание
// заданы по умолчанию, но переопределяются при вызове addMarker.
enum class MarkerType : int {
    Separator = 0,  // граница сегмента
    TrimStart,      // начало обрезки
    TrimEnd,        // конец обрезки
    FlagStart,      // флаг начала жеста
    FlagEnd         // флаг конца жеста
};

// Владеет всеми QCustomPlot в приложении: создаёт их по PlotConfig,
// кормит кадрами DataFrame, рисует маркеры и открывает диалог настройки
// по правому клику. Страницы не трогают QCustomPlot напрямую.
class PlotManager : public QWidget
{
    Q_OBJECT

public:
    explicit PlotManager(QWidget *parent = nullptr);
    ~PlotManager() override;

    // Схемы нужны для легенды и для диалога настройки.
    void setSchemas(const QList<DataSchema> &schemas);
    QList<DataSchema> schemas() const;
    DataSchema schema(const QString &name) const;

    void setConfigStore(PlotConfigStore *store);

    // Создать график. slotId - ключ настройки в PlotConfigStore.
    QCustomPlot *createPlot(const QString &slotId, const PlotConfig &config,
                            int minWidth = 200, int minHeight = 250);

    PlotConfig config(QCustomPlot &plot) const;
    // Перестроить график под новую настройку. Данные сбрасываются:
    // они относятся к прежнему набору переменных.
    bool applyConfig(QCustomPlot &plot, const PlotConfig &config);

    // ================= Живой поток =================

    // Добавить кадр. Возвращает false, если кадр не относится к этому
    // графику (другое сообщение) - тогда перерисовывать нечего.
    bool appendFrame(QCustomPlot &plot, const DataFrame &frame);
    // Перерисовать и сдвинуть окно на последнюю точку.
    void refreshLive(QCustomPlot &plot);

    // ================= Готовый набор кадров =================

    void paintFrames(QCustomPlot &plot, const QVector<DataFrame> &frames);
    void clearPlot(QCustomPlot &plot);
    bool isEmpty(QCustomPlot &plot) const;

    // ================= Маркеры =================

    void addMarker(QCustomPlot &plot, double x, MarkerType type,
                   const QColor &color = QColor());
    // Сдвинуть единственный маркер типа. Возвращает false, если его нет.
    bool setMarkerX(QCustomPlot &plot, MarkerType type, double x);
    void clearMarkers(QCustomPlot &plot, MarkerType type);
    void clearAllMarkers(QCustomPlot &plot);
    bool hasMarker(QCustomPlot &plot, MarkerType type) const;

    // ================= Разное =================

    // Диапазон X по данным графика. false, если данных нет.
    bool dataXRange(QCustomPlot &plot, double *lo, double *hi) const;
    // Перевод между координатой на графике (с учётом relativeTime) и
    // абсолютным значением X кадра. Нужно страницам, которые считают
    // отрезки времени по данным, а рисуют их поверх графика.
    double plotXToAbsolute(QCustomPlot &plot, double plotX) const;
    double absoluteToPlotX(QCustomPlot &plot, double absolute) const;
    // Абсолютная координата нуля, от которой считается relativeTime.
    double timeOrigin(QCustomPlot &plot) const;
    // Заблокировать перетаскивание диапазона (нужно при расстановке маркеров).
    void setRangeDragEnabled(QCustomPlot &plot, bool enabled);
    bool isRangeDragEnabled(QCustomPlot &plot) const;

    // X-координата кадра по настройке, без вычитания начала.
    // Общая точка правды для графика и для расчёта индексов при обрезке.
    static double xValueFor(const PlotConfig &config, const DataFrame &frame, int index);

signals:
    void plotConfigured(const QString &slotId, const PlotConfig &config);
    void logMessage(LogLevel level, const QString &text);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    struct PlotState {
        PlotConfig config;
        QString slotId;
        int pointCount = 0;     // для xMode == Index
        double originX = 0.0;   // значение, вычитаемое при relativeTime
        bool originSet = false;
        bool live = false;
        QHash<int, QList<QCPItemStraightLine *>> markers;
    };

    PlotState *stateFor(QCustomPlot &plot);
    void buildGraphs(QCustomPlot &plot, PlotState &state);
    void syncAxes(QCustomPlot &plot, PlotState &state);
    double nextX(PlotState &state, const DataFrame &frame);
    static QPen penForMarker(MarkerType type, const QColor &color);
    static QString traceName(const DataSchema *schema, const QString &field);
    void openConfigDialog(QCustomPlot &plot);

    QHash<QCustomPlot *, PlotState> m_states;
    QHash<QString, QSharedPointer<const DataSchema>> m_schemas;
    PlotConfigStore *m_store = nullptr;
    PlotConfigDialog *m_dialog = nullptr;
};
