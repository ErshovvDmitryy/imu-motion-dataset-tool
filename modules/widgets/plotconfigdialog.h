#pragma once

#include <QDialog>
#include <QList>
#include <QString>

#include "models/dataschema.h"
#include "modules/plot/plotconfig.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;

// Редактор одного графика. Открывается правым кликом по графику.
// Пользователь выбирает сообщение, отмечает нужные переменные, задаёт
// цвета, подписи осей и окно живого отображения.
class PlotConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlotConfigDialog(QWidget *parent = nullptr);

    void setSchemas(const QList<DataSchema> &schemas);
    void setConfig(const PlotConfig &config);
    PlotConfig config() const;

private slots:
    void onSchemaChanged(int index);
    void onFieldStateChanged(QListWidgetItem *item);
    void onTraceUp();
    void onTraceDown();
    void onTraceColor();
    void onSelectAll();
    void onSelectNone();
    void onInvertSelection();
    void onAxesChanged();
    void onAccept();

private:
    void buildUi();
    void reloadFields();
    DataSchema schemaByName(const QString &name) const;
    // Переносит выбранные переменные из списка полей в список сюжетов
    // с учётом порядка и текущего выделения.
    void syncTracesFromFields();
    // Обновляет цвета/имена сюжетов из текущего config().
    void refreshTraceList();

    QList<DataSchema> m_schemas;
    PlotConfig m_config;
    bool m_updating = false;

    QComboBox *m_schemaCombo = nullptr;
    QListWidget *m_fieldList = nullptr;
    QListWidget *m_traceList = nullptr;
    QPushButton *m_upButton = nullptr;
    QPushButton *m_downButton = nullptr;
    QPushButton *m_allButton = nullptr;
    QPushButton *m_noneButton = nullptr;
    QPushButton *m_invertButton = nullptr;

    QLineEdit *m_titleEdit = nullptr;
    QComboBox *m_xModeCombo = nullptr;
    QComboBox *m_xFieldCombo = nullptr;
    QLineEdit *m_xLabelEdit = nullptr;
    QLineEdit *m_yLabelEdit = nullptr;
    QDoubleSpinBox *m_windowSpin = nullptr;
    QCheckBox *m_relativeCheck = nullptr;
    QCheckBox *m_legendCheck = nullptr;
    QPushButton *m_okButton = nullptr;
};
