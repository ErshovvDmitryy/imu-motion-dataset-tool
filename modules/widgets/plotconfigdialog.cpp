#include "modules/widgets/plotconfigdialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

// Роли данных списка. Константы Qt::UserRole не конфликтуют с
// FieldRole из models/dataschema.h.
enum ItemRole : int {
    RoleField = Qt::UserRole + 1,
    RoleColor
};

QString describeType(DataValueType type) {
    return dataValueTypeName(type);
}

} // namespace

PlotConfigDialog::PlotConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    buildUi();
}

void PlotConfigDialog::buildUi() {
    setWindowTitle(QStringLiteral("Plot setup"));
    resize(560, 520);

    m_schemaCombo = new QComboBox();
    m_schemaCombo->setToolTip(QStringLiteral("Message feeding this plot"));

    m_fieldList = new QListWidget();
    m_fieldList->setToolTip(QStringLiteral("Numeric variables of the message"));
    m_fieldList->setSelectionMode(QAbstractItemView::NoSelection);
    connect(m_fieldList, &QListWidget::itemChanged, this, &PlotConfigDialog::onFieldStateChanged);

    m_traceList = new QListWidget();
    m_traceList->setToolTip(QStringLiteral("Variables drawn on the graph, in order"));

    m_upButton = new QPushButton(QStringLiteral("Up"));
    m_downButton = new QPushButton(QStringLiteral("Down"));
    connect(m_upButton, &QPushButton::clicked, this, &PlotConfigDialog::onTraceUp);
    connect(m_downButton, &QPushButton::clicked, this, &PlotConfigDialog::onTraceDown);
    connect(m_traceList, &QListWidget::itemDoubleClicked, this, &PlotConfigDialog::onTraceColor);

    m_allButton = new QPushButton(QStringLiteral("All"));
    m_noneButton = new QPushButton(QStringLiteral("None"));
    m_invertButton = new QPushButton(QStringLiteral("Invert"));
    connect(m_allButton, &QPushButton::clicked, this, &PlotConfigDialog::onSelectAll);
    connect(m_noneButton, &QPushButton::clicked, this, &PlotConfigDialog::onSelectNone);
    connect(m_invertButton, &QPushButton::clicked, this, &PlotConfigDialog::onInvertSelection);

    auto *fieldButtons = new QVBoxLayout();
    fieldButtons->addWidget(m_allButton);
    fieldButtons->addWidget(m_noneButton);
    fieldButtons->addWidget(m_invertButton);
    fieldButtons->addStretch();

    auto *listRow = new QHBoxLayout();
    listRow->addWidget(m_fieldList, 3);
    listRow->addLayout(fieldButtons);
    listRow->addWidget(m_traceList, 2);

    auto *orderBox = new QHBoxLayout();
    orderBox->addWidget(m_upButton);
    orderBox->addWidget(m_downButton);
    orderBox->addStretch();

    auto *listsGroup = new QGroupBox(QStringLiteral("Variables"));
    auto *listsLayout = new QVBoxLayout(listsGroup);
    listsLayout->addLayout(listRow);
    listsLayout->addLayout(orderBox);

    m_titleEdit = new QLineEdit();
    m_xModeCombo = new QComboBox();
    m_xModeCombo->addItem(QStringLiteral("Frame index"), static_cast<int>(XAxisMode::Index));
    m_xModeCombo->addItem(QStringLiteral("Message timestamp"), static_cast<int>(XAxisMode::Timestamp));
    m_xModeCombo->addItem(QStringLiteral("Host time"), static_cast<int>(XAxisMode::HostTime));
    connect(m_xModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PlotConfigDialog::onAxesChanged);

    m_xFieldCombo = new QComboBox();
    m_xLabelEdit = new QLineEdit();
    m_yLabelEdit = new QLineEdit();
    m_windowSpin = new QDoubleSpinBox();
    m_windowSpin->setRange(0.0, 3600.0);
    m_windowSpin->setSingleStep(0.5);
    m_windowSpin->setSuffix(QStringLiteral(" s"));
    m_windowSpin->setSpecialValueText(QStringLiteral("whole plot"));
    m_relativeCheck = new QCheckBox(QStringLiteral("Start X at zero"));
    m_legendCheck = new QCheckBox(QStringLiteral("Show legend"));

    auto *axesGroup = new QGroupBox(QStringLiteral("Axes and view"));
    auto *axesForm = new QFormLayout(axesGroup);
    axesForm->addRow(QStringLiteral("Title"), m_titleEdit);
    axesForm->addRow(QStringLiteral("X source"), m_xModeCombo);
    axesForm->addRow(QStringLiteral("X field"), m_xFieldCombo);
    axesForm->addRow(QStringLiteral("X label"), m_xLabelEdit);
    axesForm->addRow(QStringLiteral("Y label"), m_yLabelEdit);
    axesForm->addRow(QStringLiteral("Live window"), m_windowSpin);
    axesForm->addRow(m_relativeCheck);
    axesForm->addRow(m_legendCheck);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &PlotConfigDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_okButton = buttons->button(QDialogButtonBox::Ok);

    auto *root = new QVBoxLayout(this);
    auto *sourceForm = new QFormLayout();
    sourceForm->addRow(QStringLiteral("Message"), m_schemaCombo);
    root->addLayout(sourceForm);
    root->addWidget(listsGroup, 1);
    root->addWidget(axesGroup);
    root->addWidget(buttons);
}

void PlotConfigDialog::setSchemas(const QList<DataSchema> &schemas) {
    m_schemas = schemas;

    m_updating = true;
    const QString previous = m_config.schemaName;
    m_schemaCombo->clear();
    for (const DataSchema &schema : m_schemas) {
        m_schemaCombo->addItem(QStringLiteral("%1 (id %2)").arg(schema.name).arg(schema.typeId),
                               schema.name);
    }
    const int index = m_schemaCombo->findData(previous);
    if (index >= 0) {
        m_schemaCombo->setCurrentIndex(index);
    }
    m_updating = false;

    onSchemaChanged(m_schemaCombo->currentIndex());
}

void PlotConfigDialog::setConfig(const PlotConfig &config) {
    m_config = config;
    m_updating = true;

    m_titleEdit->setText(config.title);
    m_xLabelEdit->setText(config.xLabel);
    m_yLabelEdit->setText(config.yLabel);
    m_windowSpin->setValue(config.liveWindowSec);
    m_relativeCheck->setChecked(config.relativeTime);
    m_legendCheck->setChecked(config.showLegend);

    const int modeIndex = m_xModeCombo->findData(static_cast<int>(config.xMode));
    if (modeIndex >= 0) {
        m_xModeCombo->setCurrentIndex(modeIndex);
    }

    const int schemaIndex = m_schemaCombo->findData(config.schemaName);
    if (schemaIndex >= 0) {
        m_schemaCombo->setCurrentIndex(schemaIndex);
    }
    m_updating = false;

    onSchemaChanged(m_schemaCombo->currentIndex());
    refreshTraceList();
}

PlotConfig PlotConfigDialog::config() const {
    PlotConfig result = m_config;
    result.title = m_titleEdit->text().trimmed();
    result.schemaName = m_schemaCombo->currentData().toString();
    result.xMode = static_cast<XAxisMode>(m_xModeCombo->currentData().toInt());
    result.xField = m_xFieldCombo->currentData().toString();
    result.xLabel = m_xLabelEdit->text();
    result.yLabel = m_yLabelEdit->text();
    result.liveWindowSec = m_windowSpin->value();
    result.relativeTime = m_relativeCheck->isChecked();
    result.showLegend = m_legendCheck->isChecked();

    result.traces.clear();
    for (int i = 0; i < m_traceList->count(); ++i) {
        QListWidgetItem *item = m_traceList->item(i);
        PlotTrace trace;
        trace.field = item->data(RoleField).toString();
        trace.color = item->data(RoleColor).value<QColor>();
        if (trace.field.isEmpty() == false) {
            result.traces.append(trace);
        }
    }
    return result;
}

DataSchema PlotConfigDialog::schemaByName(const QString &name) const {
    for (const DataSchema &schema : m_schemas) {
        if (schema.name == name) {
            return schema;
        }
    }
    return DataSchema();
}

void PlotConfigDialog::onSchemaChanged(int index) {
    if (index < 0 || m_updating) {
        return;
    }

    reloadFields();
    syncTracesFromFields();
    refreshTraceList();
    onAxesChanged();
}

void PlotConfigDialog::reloadFields() {
    const QString name = m_schemaCombo->currentData().toString();
    const DataSchema schema = schemaByName(name);

    m_fieldList->clear();
    for (const PlottedField &field : schema.plottableFields()) {
        auto *item = new QListWidgetItem(QStringLiteral("%1  [%2]")
                                             .arg(field.name, describeType(field.type)));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setData(RoleField, field.name);
        item->setCheckState(m_config.hasField(field.name) ? Qt::Checked : Qt::Unchecked);
        m_fieldList->addItem(item);
    }

    m_updating = true;
    m_xFieldCombo->clear();
    for (const DataField &field : schema.fields) {
        if (field.role == FieldRole::Timestamp) {
            m_xFieldCombo->addItem(field.name, field.name);
        }
    }
    const int previous = m_xFieldCombo->findData(m_config.xField);
    if (previous >= 0) {
        m_xFieldCombo->setCurrentIndex(previous);
    } else if (m_xFieldCombo->count() > 0) {
        m_xFieldCombo->setCurrentIndex(0);
    }
    m_updating = false;
}

void PlotConfigDialog::onFieldStateChanged(QListWidgetItem *) {
    if (m_updating) {
        return;
    }
    syncTracesFromFields();
    refreshTraceList();
}

void PlotConfigDialog::syncTracesFromFields() {
    // Сохраняем порядок уже выбранных переменных, новые добавляем в конец.
    QStringList ordered;
    for (const PlotTrace &trace : m_config.traces) {
        if (m_config.hasField(trace.field)) {
            ordered.removeAll(trace.field);
            ordered.append(trace.field);
        }
    }

    for (int i = 0; i < m_fieldList->count(); ++i) {
        QListWidgetItem *item = m_fieldList->item(i);
        if (item->checkState() != Qt::Checked) {
            continue;
        }
        const QString field = item->data(RoleField).toString();
        if (ordered.contains(field) == false) {
            ordered.append(field);
        }
    }

    QHash<QString, QColor> colors;
    for (const PlotTrace &trace : m_config.traces) {
        colors.insert(trace.field, trace.color);
    }

    m_config.traces.clear();
    for (const QString &field : ordered) {
        PlotTrace trace;
        trace.field = field;
        trace.color = colors.value(field);
        m_config.traces.append(trace);
    }
}

void PlotConfigDialog::refreshTraceList() {
    const QList<PlotTrace> traces = config().traces;

    m_updating = true;
    const QList<QVariant> before = [&]() {
        QList<QVariant> saved;
        for (int i = 0; i < m_traceList->count(); ++i) {
            saved.append(m_traceList->item(i)->data(RoleField));
        }
        return saved;
    }();

    m_traceList->clear();
    for (int i = 0; i < traces.size(); ++i) {
        const PlotTrace &trace = traces.at(i);
        const QColor color = trace.color.isValid() ? trace.color : PlotConfig::defaultColorFor(i);
        const QString text = QStringLiteral("%1  %2")
                                 .arg(trace.field)
                                 .arg(color.name());
        auto *item = new QListWidgetItem(text);
        item->setData(RoleField, trace.field);
        item->setData(RoleColor, color);
        item->setForeground(QBrush(color.lightnessF() < 0.45 ? color.lighter(150) : color.darker(140)));
        m_traceList->addItem(item);
    }

    for (int i = 0; i < before.size() && i < m_traceList->count(); ++i) {
        if (m_traceList->item(i)->data(RoleField) == before.at(i)) {
            m_traceList->setCurrentRow(i);
            break;
        }
    }
    m_updating = false;

    m_upButton->setEnabled(m_traceList->currentRow() > 0);
    m_downButton->setEnabled(m_traceList->currentRow() >= 0
                             && m_traceList->currentRow() < m_traceList->count() - 1);
}

void PlotConfigDialog::onTraceUp() {
    const int row = m_traceList->currentRow();
    if (row <= 0) {
        return;
    }
    QListWidgetItem *item = m_traceList->takeItem(row);
    m_traceList->insertItem(row - 1, item);
    m_traceList->setCurrentRow(row - 1);
    refreshTraceList();
}

void PlotConfigDialog::onTraceDown() {
    const int row = m_traceList->currentRow();
    if (row < 0 || row >= m_traceList->count() - 1) {
        return;
    }
    QListWidgetItem *item = m_traceList->takeItem(row);
    m_traceList->insertItem(row + 1, item);
    m_traceList->setCurrentRow(row + 1);
    refreshTraceList();
}

void PlotConfigDialog::onTraceColor() {
    QListWidgetItem *item = m_traceList->currentItem();
    if (item == nullptr) {
        return;
    }
    const QColor current = item->data(RoleColor).value<QColor>();
    const QColor chosen = QColorDialog::getColor(current.isValid() ? current : Qt::white, this);
    if (chosen.isValid() == false) {
        return;
    }

    m_updating = true;
    item->setData(RoleColor, chosen);
    m_updating = false;

    const QString field = item->data(RoleField).toString();
    for (PlotTrace &trace : m_config.traces) {
        if (trace.field == field) {
            trace.color = chosen;
        }
    }
    refreshTraceList();
}

void PlotConfigDialog::onSelectAll() {
    m_updating = true;
    for (int i = 0; i < m_fieldList->count(); ++i) {
        m_fieldList->item(i)->setCheckState(Qt::Checked);
    }
    m_updating = false;
    syncTracesFromFields();
    refreshTraceList();
}

void PlotConfigDialog::onSelectNone() {
    m_updating = true;
    for (int i = 0; i < m_fieldList->count(); ++i) {
        m_fieldList->item(i)->setCheckState(Qt::Unchecked);
    }
    m_updating = false;
    syncTracesFromFields();
    refreshTraceList();
}

void PlotConfigDialog::onInvertSelection() {
    m_updating = true;
    for (int i = 0; i < m_fieldList->count(); ++i) {
        QListWidgetItem *item = m_fieldList->item(i);
        item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    }
    m_updating = false;
    syncTracesFromFields();
    refreshTraceList();
}

void PlotConfigDialog::onAxesChanged() {
    if (m_updating) {
        return;
    }
    const XAxisMode mode = static_cast<XAxisMode>(m_xModeCombo->currentData().toInt());
    m_xFieldCombo->setEnabled(mode == XAxisMode::Timestamp);
}

void PlotConfigDialog::onAccept() {
    const PlotConfig result = config();
    QString error;
    if (result.isValid(&error) == false) {
        QMessageBox::warning(this, QStringLiteral("Plot setup"), error);
        return;
    }
    m_config = result;
    QDialog::accept();
}
