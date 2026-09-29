#include "pages/messageeditpage.h"

#include "models/schemastore.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

// Значения, не имеющие отношения к протоколу, отбрасываем при выходе
// из редактора полей.
QString describeType(DataValueType type) {
    return dataValueTypeName(type);
}

void fillCombo(QComboBox *combo, const QList<QPair<QString, int>> &entries) {
    for (const auto &entry : entries) {
        combo->addItem(entry.first, entry.second);
    }
}

QList<QPair<QString, int>> typeEntries() {
    QList<QPair<QString, int>> entries;
    for (DataValueType type : allDataValueTypes()) {
        if (type == DataValueType::Invalid) {
            continue;
        }
        entries.append(qMakePair(describeType(type), static_cast<int>(type)));
    }
    return entries;
}

QList<QPair<QString, int>> roleEntries() {
    QList<QPair<QString, int>> entries;
    entries.append(qMakePair(QStringLiteral("none"), static_cast<int>(FieldRole::None)));
    entries.append(qMakePair(QStringLiteral("time"), static_cast<int>(FieldRole::Timestamp)));
    entries.append(qMakePair(QStringLiteral("crc16"), static_cast<int>(FieldRole::Crc16)));
    entries.append(qMakePair(QStringLiteral("recording"), static_cast<int>(FieldRole::Recording)));
    return entries;
}

QList<QPair<QString, int>> unitEntries() {
    QList<QPair<QString, int>> entries;
    entries.append(qMakePair(QStringLiteral("none"), static_cast<int>(FieldUnit::None)));
    entries.append(qMakePair(QStringLiteral("s"), static_cast<int>(FieldUnit::Seconds)));
    entries.append(qMakePair(QStringLiteral("ms"), static_cast<int>(FieldUnit::Milliseconds)));
    entries.append(qMakePair(QStringLiteral("us"), static_cast<int>(FieldUnit::Microseconds)));
    entries.append(qMakePair(QStringLiteral("ns"), static_cast<int>(FieldUnit::Nanoseconds)));
    return entries;
}

} // namespace

MessageEditPage::MessageEditPage(SchemaStore *store, QWidget *parent)
    : QWidget(parent)
    , m_store(store)
{
    buildUi();
    connectSignals();
    refreshList();
}

void MessageEditPage::buildUi() {
    // ---------------- левая часть ----------------

    m_list = new QListWidget();
    m_list->setToolTip(QStringLiteral("Message types loaded from protocols/*.qproto"));

    m_newButton = new QPushButton(QStringLiteral("New"));
    m_duplicateButton = new QPushButton(QStringLiteral("Duplicate"));
    m_renameButton = new QPushButton(QStringLiteral("Rename"));
    m_deleteButton = new QPushButton(QStringLiteral("Delete"));
    m_reloadButton = new QPushButton(QStringLiteral("Reload"));

    auto *listButtons = new QVBoxLayout();
    listButtons->addWidget(m_newButton);
    listButtons->addWidget(m_duplicateButton);
    listButtons->addWidget(m_renameButton);
    listButtons->addWidget(m_deleteButton);
    listButtons->addWidget(m_reloadButton);
    listButtons->addStretch();

    auto *leftLayout = new QVBoxLayout();
    leftLayout->addWidget(m_list, 1);
    leftLayout->addLayout(listButtons);

    auto *listGroup = new QGroupBox(QStringLiteral("Messages"));
    listGroup->setLayout(leftLayout);

    // ---------------- правая часть: шапка ----------------

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText(QStringLiteral("LiveMotion"));
    m_descriptionEdit = new QLineEdit();
    m_typeIdSpin = new QSpinBox();
    m_typeIdSpin->setRange(1, 255);
    m_framingCombo = new QComboBox();
    m_framingCombo->addItem(QStringLiteral("Fixed length"), static_cast<int>(FramingMode::FixedLength));
    m_framingCombo->addItem(QStringLiteral("Header length"), static_cast<int>(FramingMode::HeaderLength));

    auto *headerForm = new QFormLayout();
    headerForm->addRow(QStringLiteral("Name"), m_nameEdit);
    headerForm->addRow(QStringLiteral("Comment"), m_descriptionEdit);
    headerForm->addRow(QStringLiteral("Type id"), m_typeIdSpin);
    headerForm->addRow(QStringLiteral("Framing"), m_framingCombo);

    auto *headerGroup = new QGroupBox(QStringLiteral("Message"));
    headerGroup->setLayout(headerForm);

    // ---------------- правая часть: поля ----------------

    m_fieldTable = new QTableWidget(0, ColumnTotal);
    m_fieldTable->setHorizontalHeaderLabels({ QStringLiteral("Name"),
                                              QStringLiteral("Type"),
                                              QStringLiteral("Count"),
                                              QStringLiteral("Length"),
                                              QStringLiteral("Role"),
                                              QStringLiteral("Unit") });
    m_fieldTable->horizontalHeader()->setStretchLastSection(true);
    m_fieldTable->verticalHeader()->setVisible(false);
    m_fieldTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_addFieldButton = new QPushButton(QStringLiteral("+ field"));
    m_removeFieldButton = new QPushButton(QStringLiteral("- field"));
    m_upFieldButton = new QPushButton(QStringLiteral("Up"));
    m_downFieldButton = new QPushButton(QStringLiteral("Down"));

    auto *fieldButtons = new QHBoxLayout();
    fieldButtons->addWidget(m_addFieldButton);
    fieldButtons->addWidget(m_removeFieldButton);
    fieldButtons->addStretch();
    fieldButtons->addWidget(m_upFieldButton);
    fieldButtons->addWidget(m_downFieldButton);

    auto *fieldLayout = new QVBoxLayout();
    fieldLayout->addWidget(m_fieldTable, 1);
    fieldLayout->addLayout(fieldButtons);

    auto *fieldGroup = new QGroupBox(QStringLiteral("Fields"));
    fieldGroup->setLayout(fieldLayout);

    // ---------------- правая часть: DSL и JSON ----------------

    m_dslEdit = new QPlainTextEdit();
    m_dslEdit->setPlaceholderText(QStringLiteral("ax:float32; ay:float32; t:uint32@time@us; crc:uint16@crc"));
    m_dslEdit->setMaximumHeight(90);
    m_applyDslButton = new QPushButton(QStringLiteral("Apply"));
    m_applyDslButton->setToolTip(QStringLiteral("Replace the field table with the parsed line"));

    m_jsonEdit = new QPlainTextEdit();
    m_jsonEdit->setReadOnly(true);
    m_jsonEdit->setMaximumHeight(140);
    m_jsonButton = new QPushButton(QStringLiteral("Show JSON"));
    m_jsonButton->setCheckable(true);

    auto *dslRow = new QHBoxLayout();
    dslRow->addWidget(m_dslEdit, 1);
    dslRow->addWidget(m_applyDslButton);

    auto *jsonRow = new QHBoxLayout();
    jsonRow->addWidget(m_jsonButton);
    jsonRow->addStretch();

    auto *textGroup = new QGroupBox(QStringLiteral("Quick edit"));
    auto *textLayout = new QVBoxLayout(textGroup);
    textLayout->addLayout(dslRow);
    textLayout->addWidget(new QLabel(QStringLiteral("Saved to *.qproto (JSON):")));
    textLayout->addLayout(jsonRow);
    textLayout->addWidget(m_jsonEdit);

    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    m_sizeLabel = new QLabel();
    m_saveButton = new QPushButton(QStringLiteral("Save"));
    m_revertButton = new QPushButton(QStringLiteral("Revert"));

    auto *footer = new QHBoxLayout();
    footer->addWidget(m_sizeLabel, 1);
    footer->addWidget(m_revertButton);
    footer->addWidget(m_saveButton);

    auto *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(headerGroup);
    rightLayout->addWidget(fieldGroup, 1);
    rightLayout->addWidget(textGroup);
    rightLayout->addWidget(m_statusLabel);
    rightLayout->addLayout(footer);

    auto *root = new QHBoxLayout(this);
    root->addWidget(listGroup, 1);
    root->addLayout(rightLayout, 3);
}

void MessageEditPage::connectSignals() {
    connect(m_list, &QListWidget::currentRowChanged, this, &MessageEditPage::onListSelectionChanged);
    connect(m_newButton, &QPushButton::clicked, this, &MessageEditPage::onNewMessage);
    connect(m_duplicateButton, &QPushButton::clicked, this, &MessageEditPage::onDuplicateMessage);
    connect(m_renameButton, &QPushButton::clicked, this, &MessageEditPage::onRenameMessage);
    connect(m_deleteButton, &QPushButton::clicked, this, &MessageEditPage::onDeleteMessage);
    connect(m_reloadButton, &QPushButton::clicked, this, &MessageEditPage::onReload);
    connect(m_saveButton, &QPushButton::clicked, this, &MessageEditPage::onSave);
    connect(m_revertButton, &QPushButton::clicked, this, &MessageEditPage::onRevert);
    connect(m_applyDslButton, &QPushButton::clicked, this, &MessageEditPage::onApplyDsl);
    connect(m_addFieldButton, &QPushButton::clicked, this, &MessageEditPage::onAddField);
    connect(m_removeFieldButton, &QPushButton::clicked, this, &MessageEditPage::onRemoveField);
    connect(m_upFieldButton, &QPushButton::clicked, this, &MessageEditPage::onMoveFieldUp);
    connect(m_downFieldButton, &QPushButton::clicked, this, &MessageEditPage::onMoveFieldDown);
    connect(m_jsonButton, &QPushButton::toggled, this, &MessageEditPage::onPreviewJson);
    connect(m_fieldTable, &QTableWidget::itemChanged, this, &MessageEditPage::onFieldTableChanged);

    connect(m_nameEdit, &QLineEdit::textChanged, this, &MessageEditPage::onFieldTableChanged);
    connect(m_typeIdSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MessageEditPage::onFieldTableChanged);
    connect(m_framingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MessageEditPage::onFieldTableChanged);
    connect(m_descriptionEdit, &QLineEdit::textChanged, this, &MessageEditPage::onFieldTableChanged);
}

void MessageEditPage::refreshList(int selectRow) {
    m_loading = true;
    const QString previous = m_list->currentItem() != nullptr ? m_list->currentItem()->text() : QString();

    m_list->clear();
    if (m_store != nullptr) {
        for (const DataSchema &schema : m_store->schemas()) {
            auto *item = new QListWidgetItem(
                QStringLiteral("%1  (id %2, %3 B)").arg(schema.name).arg(schema.typeId).arg(schema.totalSize()));
            item->setData(Qt::UserRole, schema.name);
            if (schema.builtIn) {
                QFont font = item->font();
                font.setBold(true);
                item->setFont(font);
                item->setToolTip(QStringLiteral("Built-in message"));
            }
            m_list->addItem(item);
        }
    }
    m_loading = false;

    int row = -1;
    if (selectRow >= 0) {
        row = selectRow;
    } else if (previous.isEmpty() == false) {
        for (int i = 0; i < m_list->count(); ++i) {
            if (m_list->item(i)->data(Qt::UserRole).toString() == previous) {
                row = i;
                break;
            }
        }
    }
    if (row < 0 && m_list->count() > 0) {
        row = 0;
    }
    if (row >= 0) {
        m_list->setCurrentRow(row);
    } else {
        fillForm(DataSchema());
    }
}

void MessageEditPage::fillForm(const DataSchema &schema) {
    m_loading = true;

    m_original = schema;
    m_draft = schema;
    m_created = schema.name.isEmpty();

    m_nameEdit->setText(schema.name);
    m_descriptionEdit->setText(schema.description);
    m_typeIdSpin->setValue(schema.typeId == 0 ? 1 : schema.typeId);
    const int framingIndex = m_framingCombo->findData(static_cast<int>(schema.framing));
    m_framingCombo->setCurrentIndex(framingIndex < 0 ? 0 : framingIndex);

    m_dslEdit->setPlainText(schema.dsl());

    m_fieldTable->setRowCount(0);
    m_fieldTable->setRowCount(schema.fields.size());
    for (int row = 0; row < schema.fields.size(); ++row) {
        const DataField &field = schema.fields.at(row);

        auto *name = new QTableWidgetItem(field.name);
        m_fieldTable->setItem(row, ColumnName, name);

        auto *type = new QComboBox();
        fillCombo(type, typeEntries());
        type->setCurrentIndex(type->findData(static_cast<int>(field.type)));
        m_fieldTable->setCellWidget(row, ColumnType, type);

        auto *count = new QSpinBox();
        count->setRange(1, 4096);
        count->setValue(field.count);
        m_fieldTable->setCellWidget(row, ColumnCount, count);

        auto *length = new QSpinBox();
        length->setRange(0, 65535);
        length->setValue(field.length);
        m_fieldTable->setCellWidget(row, ColumnLength, length);

        auto *role = new QComboBox();
        fillCombo(role, roleEntries());
        role->setCurrentIndex(role->findData(static_cast<int>(field.role)));
        m_fieldTable->setCellWidget(row, ColumnRole, role);

        auto *unit = new QComboBox();
        fillCombo(unit, unitEntries());
        unit->setCurrentIndex(unit->findData(static_cast<int>(field.unit)));
        m_fieldTable->setCellWidget(row, ColumnUnit, unit);
    }

    m_loading = false;
    setDirty(false);
    refreshPreview();
    updateButtons();
}

void MessageEditPage::updateButtons() {
    const bool has = m_draft.name.isEmpty() == false;
    m_saveButton->setEnabled(has || m_dirty);
    m_revertButton->setEnabled(m_dirty);
    m_deleteButton->setEnabled(has && m_store != nullptr && m_draft.builtIn == false);
    m_renameButton->setEnabled(has && m_draft.builtIn == false);
    m_addFieldButton->setEnabled(has);
    m_removeFieldButton->setEnabled(has && m_fieldTable->currentRow() >= 0);
    m_upFieldButton->setEnabled(has && m_fieldTable->currentRow() > 0);
    m_downFieldButton->setEnabled(has && m_fieldTable->currentRow() >= 0
                                  && m_fieldTable->currentRow() < m_fieldTable->rowCount() - 1);
}

void MessageEditPage::setDirty(bool dirty) {
    m_dirty = dirty;
    if (m_statusLabel != nullptr) {
        m_statusLabel->setText(dirty ? QStringLiteral("Unsaved changes") : QString());
    }
    updateButtons();
}

int MessageEditPage::fieldRow(const QString &name) const {
    for (int row = 0; row < m_draft.fields.size(); ++row) {
        if (m_draft.fields.at(row).name == name) {
            return row;
        }
    }
    return -1;
}

DataField MessageEditPage::fieldFromRow(int row) const {
    DataField field = m_draft.fields.value(row);
    if (m_fieldTable->item(row, ColumnName) != nullptr) {
        field.name = m_fieldTable->item(row, ColumnName)->text().trimmed();
    }
    if (auto *widget = qobject_cast<QComboBox *>(m_fieldTable->cellWidget(row, ColumnType))) {
        field.type = static_cast<DataValueType>(widget->currentData().toInt());
    }
    if (auto *widget = qobject_cast<QSpinBox *>(m_fieldTable->cellWidget(row, ColumnCount))) {
        field.count = widget->value();
    }
    if (auto *widget = qobject_cast<QSpinBox *>(m_fieldTable->cellWidget(row, ColumnLength))) {
        field.length = widget->value();
    }
    if (auto *widget = qobject_cast<QComboBox *>(m_fieldTable->cellWidget(row, ColumnRole))) {
        field.role = static_cast<FieldRole>(widget->currentData().toInt());
    }
    if (auto *widget = qobject_cast<QComboBox *>(m_fieldTable->cellWidget(row, ColumnUnit))) {
        field.unit = static_cast<FieldUnit>(widget->currentData().toInt());
    }
    return field;
}

bool MessageEditPage::collectSchema(DataSchema *out) {
    DataSchema schema = m_draft;
    schema.name = m_nameEdit->text().trimmed();
    schema.description = m_descriptionEdit->text();
    schema.typeId = static_cast<quint8>(m_typeIdSpin->value());
    schema.framing = static_cast<FramingMode>(m_framingCombo->currentData().toInt());
    schema.builtIn = m_draft.builtIn;

    QVector<DataField> fields;
    for (int row = 0; row < m_fieldTable->rowCount(); ++row) {
        DataField field = fieldFromRow(row);
        if (field.name.isEmpty()) {
            continue;
        }
        if (field.isVariable()) {
            // Переменная длина имеет смысл только в режиме с заголовком.
            schema.framing = FramingMode::HeaderLength;
        }
        fields.append(field);
    }
    schema.fields = fields;

    if (schema.name.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("Message name is required"));
        return false;
    }
    const QString problem = schema.validationError();
    if (problem.isEmpty() == false) {
        m_statusLabel->setText(problem);
        return false;
    }

    *out = schema;
    return true;
}

void MessageEditPage::refreshPreview() {
    DataSchema schema = m_draft;
    schema.name = m_nameEdit->text().trimmed();
    schema.description = m_descriptionEdit->text();
    schema.typeId = static_cast<quint8>(m_typeIdSpin->value());
    schema.framing = static_cast<FramingMode>(m_framingCombo->currentData().toInt());
    for (int row = 0; row < m_fieldTable->rowCount(); ++row) {
        schema.fields.append(fieldFromRow(row));
    }

    m_sizeLabel->setText(QStringLiteral("%1 bytes, %2 values, %3 fields")
                             .arg(schema.totalSize())
                             .arg(schema.slotCount())
                             .arg(schema.fields.size()));

    if (m_jsonButton->isChecked()) {
        m_jsonEdit->setPlainText(QString::fromUtf8(
            QJsonDocument(schema.toJson()).toJson(QJsonDocument::Indented)));
    } else {
        m_jsonEdit->clear();
    }
}

void MessageEditPage::onFieldTableChanged() {
    if (m_loading) {
        return;
    }
    DataField previous;
    const int row = m_fieldTable->currentRow();
    if (row >= 0 && row < m_draft.fields.size()) {
        previous = m_draft.fields.at(row);
    }
    for (int i = 0; i < m_fieldTable->rowCount(); ++i) {
        if (i < m_draft.fields.size()) {
            m_draft.fields[i] = fieldFromRow(i);
        }
    }
    m_draft.name = m_nameEdit->text().trimmed();
    m_draft.typeId = static_cast<quint8>(m_typeIdSpin->value());
    m_draft.description = m_descriptionEdit->text();
    m_draft.framing = static_cast<FramingMode>(m_framingCombo->currentData().toInt());

    if (previous.name.isEmpty() == false && m_dslEdit->toPlainText().isEmpty() == false) {
        m_dslEdit->setPlainText(DataSchema::fieldListToDsl(m_draft.fields));
    }

    setDirty(m_draft != m_original);
    refreshPreview();
}

void MessageEditPage::applyDslToTable() {
    QString error;
    const QVector<DataField> fields = DataSchema::parseFieldList(m_dslEdit->toPlainText(), &error);
    if (error.isEmpty() == false) {
        QMessageBox::warning(this, QStringLiteral("Field list"), error);
        return;
    }

    m_draft.fields = fields;
    const DataSchema current = m_draft;
    fillForm(current);
    setDirty(m_draft != m_original);
}

int MessageEditPage::currentRowForDraft() const {
    const QString name = m_draft.name.isEmpty() ? m_original.name : m_draft.name;
    if (name.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->data(Qt::UserRole).toString() == name) {
            return i;
        }
    }
    return -1;
}

void MessageEditPage::onListSelectionChanged() {
    if (m_loading || m_store == nullptr) {
        return;
    }

    if (m_dirty) {
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Unsaved changes"),
            QStringLiteral("Discard changes to \"%1\"?").arg(m_draft.name),
            QMessageBox::Discard | QMessageBox::Cancel);
        if (answer != QMessageBox::Discard) {
            const int row = currentRowForDraft();
            if (row >= 0) {
                const bool blocked = m_list->blockSignals(true);
                m_list->setCurrentRow(row);
                m_list->blockSignals(blocked);
            }
            return;
        }
    }

    QListWidgetItem *item = m_list->currentItem();
    if (item == nullptr) {
        fillForm(DataSchema());
        return;
    }
    fillForm(m_store->schema(item->data(Qt::UserRole).toString()));
}

void MessageEditPage::onNewMessage() {
    if (m_store == nullptr) {
        return;
    }
    DataSchema schema;
    schema.typeId = m_store->suggestTypeId();
    schema.name = QStringLiteral("Message%1").arg(schema.typeId);
    DataField value;
    value.name = QStringLiteral("value");
    value.type = DataValueType::Float32;
    schema.fields.append(value);
    fillForm(schema);
    m_created = true;
    setDirty(true);
    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

void MessageEditPage::onDuplicateMessage() {
    if (m_store == nullptr || m_draft.name.isEmpty()) {
        return;
    }

    bool accepted = false;
    const QString name = QInputDialog::getText(
        this, QStringLiteral("Duplicate message"),
        QStringLiteral("Name for the copy:"), QLineEdit::Normal,
        m_draft.name + QStringLiteral("_copy"), &accepted);
    if (accepted == false || name.trimmed().isEmpty()) {
        return;
    }

    DataSchema copy = m_draft;
    copy.name = name.trimmed();
    copy.builtIn = false;
    copy.typeId = m_store->suggestTypeId();
    fillForm(copy);
    m_created = true;
    setDirty(true);
}

void MessageEditPage::onRenameMessage() {
    if (m_store == nullptr || m_draft.name.isEmpty() || m_created) {
        return;
    }
    if (m_dirty) {
        QMessageBox::information(this, QStringLiteral("Rename"),
                                 QStringLiteral("Save the changes first, then rename."));
        return;
    }

    bool accepted = false;
    const QString name = QInputDialog::getText(
        this, QStringLiteral("Rename message"), QStringLiteral("New name:"),
        QLineEdit::Normal, m_draft.name, &accepted);
    if (accepted == false || name.trimmed().isEmpty() || name.trimmed() == m_draft.name) {
        return;
    }

    QString error;
    if (m_store->rename(m_draft.name, name.trimmed(), &error) == false) {
        QMessageBox::warning(this, QStringLiteral("Rename"), error);
        return;
    }
    refreshList();
}

void MessageEditPage::onDeleteMessage() {
    if (m_store == nullptr || m_draft.name.isEmpty() || m_created) {
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("Delete message"),
                              QStringLiteral("Delete \"%1\"?").arg(m_draft.name),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (m_store->remove(m_draft.name, &error) == false) {
        QMessageBox::warning(this, QStringLiteral("Delete"), error);
        return;
    }
    m_created = true;
    refreshList();
}

void MessageEditPage::onReload() {
    if (m_store == nullptr) {
        return;
    }
    m_store->ensureBuiltins();
    m_store->loadAll();
    refreshList();
}

void MessageEditPage::onSave() {
    if (m_store == nullptr) {
        return;
    }

    DataSchema schema;
    if (collectSchema(&schema) == false) {
        return;
    }

    // Встроенное сообщение нельзя переименовать: если имя изменилось,
    // возвращаем исходное и объясняем причину.
    if (m_draft.builtIn && schema.name != m_draft.name) {
        QMessageBox::warning(this, QStringLiteral("Save"),
                             QStringLiteral("Built-in messages keep their name."));
        m_nameEdit->setText(m_draft.name);
        schema.name = m_draft.name;
    }

    QString error;
    if (m_store->save(schema, &error) == false) {
        m_statusLabel->setText(error);
        QMessageBox::warning(this, QStringLiteral("Save"), error);
        return;
    }

    m_created = false;
    m_dslEdit->setPlainText(schema.dsl());
    refreshList();
    m_statusLabel->setText(QStringLiteral("Saved to %1").arg(m_store->fileNameFor(schema)));
}

void MessageEditPage::onRevert() {
    if (m_created) {
        fillForm(DataSchema());
        return;
    }
    fillForm(m_original);
}

void MessageEditPage::onApplyDsl() {
    applyDslToTable();
}

void MessageEditPage::onAddField() {
    DataField field;
    field.name = QStringLiteral("field%1").arg(m_draft.fields.size() + 1);
    field.type = DataValueType::Float32;
    m_draft.fields.append(field);
    const DataSchema current = m_draft;
    fillForm(current);
    m_fieldTable->setCurrentCell(m_fieldTable->rowCount() - 1, ColumnName);
    setDirty(true);
}

void MessageEditPage::onRemoveField() {
    const int row = m_fieldTable->currentRow();
    if (row < 0 || row >= m_draft.fields.size()) {
        return;
    }
    m_draft.fields.remove(row);
    const DataSchema current = m_draft;
    fillForm(current);
    setDirty(true);
}

void MessageEditPage::onMoveFieldUp() {
    const int row = m_fieldTable->currentRow();
    if (row <= 0) {
        return;
    }
    const DataField moved = m_draft.fields.at(row);
    m_draft.fields.remove(row);
    m_draft.fields.insert(row - 1, moved);
    const DataSchema current = m_draft;
    fillForm(current);
    m_fieldTable->setCurrentCell(row - 1, ColumnName);
    setDirty(true);
}

void MessageEditPage::onMoveFieldDown() {
    const int row = m_fieldTable->currentRow();
    if (row < 0 || row >= m_draft.fields.size() - 1) {
        return;
    }
    const DataField moved = m_draft.fields.at(row);
    m_draft.fields.remove(row);
    m_draft.fields.insert(row + 1, moved);
    const DataSchema current = m_draft;
    fillForm(current);
    m_fieldTable->setCurrentCell(row + 1, ColumnName);
    setDirty(true);
}

void MessageEditPage::onPreviewJson() {
    refreshPreview();
}
