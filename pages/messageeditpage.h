#pragma once

#include <QList>
#include <QString>
#include <QWidget>

#include "models/dataschema.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QComboBox;
class QTableWidget;
class SchemaStore;

// Редактор типов входящих сообщений. Слева - список сообщений, справа -
// описание выбранного: имя, код, фрейминг и таблица полей. Правка полей
// доступна и таблицей, и одной строкой DSL. Сохранение идёт в SchemaStore,
// то есть в protocols/<имя>.qproto.
class MessageEditPage : public QWidget
{
    Q_OBJECT

public:
    explicit MessageEditPage(SchemaStore *store, QWidget *parent = nullptr);

private slots:
    void onListSelectionChanged();
    void onNewMessage();
    void onDuplicateMessage();
    void onRenameMessage();
    void onDeleteMessage();
    void onReload();
    void onSave();
    void onRevert();
    void onApplyDsl();
    void onFieldTableChanged();
    void onAddField();
    void onRemoveField();
    void onMoveFieldUp();
    void onMoveFieldDown();
    void onPreviewJson();

private:
    void buildUi();
    void connectSignals();

    // Читает форму в DataSchema. Возвращает false, если схема невалидна;
    // причина попадает в m_statusLabel.
    bool collectSchema(DataSchema *out);
    void fillForm(const DataSchema &schema);
    void refreshList(int selectRow = -1);
    void refreshPreview();
    void setDirty(bool dirty);
    void updateButtons();
    // Возвращает строку списка, соответствующую m_draft, или -1.
    int currentRowForDraft() const;
    DataField fieldFromRow(int row) const;
    int fieldRow(const QString &name) const;
    void applyDslToTable();

    SchemaStore *m_store = nullptr;
    DataSchema m_original;      // состояние на момент выбора/сохранения
    DataSchema m_draft;         // редактируемая копия
    bool m_loading = false;
    bool m_dirty = false;
    bool m_created = false;     // m_draft ещё не существует в хранилище

    // ЛЕВАЯ ЧАСТЬ
    QListWidget *m_list = nullptr;
    QPushButton *m_newButton = nullptr;
    QPushButton *m_duplicateButton = nullptr;
    QPushButton *m_renameButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_reloadButton = nullptr;

    // ПРАВАЯ ЧАСТЬ
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_descriptionEdit = nullptr;
    QSpinBox *m_typeIdSpin = nullptr;
    QComboBox *m_framingCombo = nullptr;
    QTableWidget *m_fieldTable = nullptr;
    QPlainTextEdit *m_dslEdit = nullptr;
    QPlainTextEdit *m_jsonEdit = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_sizeLabel = nullptr;
    QPushButton *m_addFieldButton = nullptr;
    QPushButton *m_removeFieldButton = nullptr;
    QPushButton *m_upFieldButton = nullptr;
    QPushButton *m_downFieldButton = nullptr;
    QPushButton *m_applyDslButton = nullptr;
    QPushButton *m_jsonButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_revertButton = nullptr;

    static constexpr int ColumnName = 0;
    static constexpr int ColumnType = 1;
    static constexpr int ColumnCount = 2;
    static constexpr int ColumnLength = 3;
    static constexpr int ColumnRole = 4;
    static constexpr int ColumnUnit = 5;
    static constexpr int ColumnTotal = 6;
};
