#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include "models/dataschema.h"
#include "models/loglevel.h"

// Хранилище типов входящих сообщений. Источник правды - файлы
// protocols/<имя>.qproto (JSON), поэтому набор схем можно коммитить в git
// и переносить вместе с приложением.
class SchemaStore : public QObject
{
    Q_OBJECT

public:
    explicit SchemaStore(QObject *parent = nullptr);
    ~SchemaStore() override;

    // По умолчанию <applicationDirPath>/protocols.
    void setDirectory(const QString &path);
    QString directory() const;

    // Перечитать папку. Не перезаписывает уже существующие встроенные схемы.
    void loadAll();
    // Создать встроенные схемы, если папки ещё нет.
    void ensureBuiltins();

    QList<DataSchema> schemas() const;
    QStringList names() const;
    bool contains(const QString &name) const;
    bool containsTypeId(quint8 typeId) const;

    DataSchema schema(const QString &name) const;
    DataSchema schemaByTypeId(quint8 typeId) const;

    // Создать или обновить схему. Проверяет валидность и уникальность typeId.
    bool save(const DataSchema &schema, QString *error = nullptr);
    bool remove(const QString &name, QString *error = nullptr);
    bool rename(const QString &from, const QString &to, QString *error = nullptr);

    // Свободный typeId для нового сообщения.
    quint8 suggestTypeId() const;

    // Разобранное имя файла для схемы (без пути).
    QString fileNameFor(const DataSchema &schema) const;

signals:
    void schemaSaved(const QString &name);
    void schemaRemoved(const QString &name);
    void schemasReloaded();
    void logMessage(LogLevel level, const QString &text);

private:
    bool writeSchema(const DataSchema &schema, QString *error);
    // previousName - имя схемы до переименования (её же typeId не считаем конфликтом).
    bool validateForSave(const DataSchema &schema, QString *error,
                         const QString &previousName = QString()) const;
    void indexSchema(const DataSchema &schema);
    bool isSafeName(const QString &name) const;

    QString m_directory;
    QHash<QString, DataSchema> m_byName;
    QHash<quint8, QString> m_nameByTypeId;
};
