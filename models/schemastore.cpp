#include "models/schemastore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>

#include "models/builtinschemas.h"

#include <algorithm>

namespace {

const char *kSuffix = ".qproto";

} // namespace

SchemaStore::SchemaStore(QObject *parent)
    : QObject(parent)
{
    m_directory = QCoreApplication::applicationDirPath() + QStringLiteral("/protocols");
}

SchemaStore::~SchemaStore()
{
}

void SchemaStore::setDirectory(const QString &path) {
    if (m_directory == path) {
        return;
    }
    m_directory = path;
    // Содержимое не перечитываем: вызывающий код сам решает, когда
    // вызвать loadAll(). Иначе setDirectory перед save() затирал бы
    // уже загруженный набор схем.
}

QString SchemaStore::directory() const {
    return m_directory;
}

bool SchemaStore::isSafeName(const QString &name) const {
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z_][A-Za-z0-9_\\-]*$"));
    return pattern.match(name).hasMatch();
}

QString SchemaStore::fileNameFor(const DataSchema &schema) const {
    return schema.name + QLatin1String(kSuffix);
}

void SchemaStore::loadAll() {
    m_byName.clear();
    m_nameByTypeId.clear();

    QDir dir(m_directory);
    if (dir.exists() == false) {
        return;
    }

    const QStringList entries = dir.entryList(QStringList{QStringLiteral("*%1").arg(QLatin1String(kSuffix))},
                                              QDir::Files, QDir::Name);
    for (const QString &entry : entries) {
        QFile file(dir.filePath(entry));
        if (file.open(QIODevice::ReadOnly) == false) {
            emit logMessage(LogLevel::Warning,
                            QStringLiteral("Schema %1: cannot open (%2)").arg(entry, file.errorString()));
            continue;
        }

        QString error;
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (document.isObject() == false) {
            emit logMessage(LogLevel::Error,
                            QStringLiteral("Schema %1: broken JSON (%2)").arg(entry, parseError.errorString()));
            continue;
        }

        DataSchema schema = DataSchema::fromJson(document.object(), &error);
        if (schema.isValid() == false) {
            emit logMessage(LogLevel::Error,
                            QStringLiteral("Schema %1: %2").arg(entry, error));
            continue;
        }
        if (m_byName.contains(schema.name)) {
            emit logMessage(LogLevel::Warning,
                            QStringLiteral("Schema %1 duplicates name \"%2\", skipped")
                                .arg(entry, schema.name));
            continue;
        }
        if (m_nameByTypeId.contains(schema.typeId)) {
            emit logMessage(LogLevel::Error,
                            QStringLiteral("Schema %1 reuses type id %2 (already taken by \"%3\"), skipped")
                                .arg(schema.name)
                                .arg(schema.typeId)
                                .arg(m_nameByTypeId.value(schema.typeId)));
            continue;
        }

        indexSchema(schema);
        emit logMessage(LogLevel::Info,
                       QStringLiteral("Loaded message \"%1\" (id %2, %3 bytes, %4 fields)")
                           .arg(schema.name)
                           .arg(schema.typeId)
                           .arg(schema.totalSize())
                           .arg(schema.fields.size()));
    }

    emit schemasReloaded();
}

void SchemaStore::ensureBuiltins() {
    QDir dir(m_directory);
    if (dir.exists() == false && dir.mkpath(QStringLiteral(".")) == false) {
        emit logMessage(LogLevel::Error,
                        QStringLiteral("Cannot create schema directory %1").arg(m_directory));
        return;
    }

    const QList<DataSchema> builtins = BuiltinSchemas::all();
    for (const DataSchema &schema : builtins) {
        if (contains(schema.name)) {
            continue;
        }
        QString error;
        if (save(schema, &error) == false) {
            emit logMessage(LogLevel::Error,
                            QStringLiteral("Cannot create built-in schema: %1").arg(error));
        }
    }
}

QList<DataSchema> SchemaStore::schemas() const {
    QList<DataSchema> result = m_byName.values();
    std::sort(result.begin(), result.end(), [](const DataSchema &a, const DataSchema &b) {
        return a.typeId < b.typeId;
    });
    return result;
}

QStringList SchemaStore::names() const {
    const QList<DataSchema> all = schemas();
    QStringList result;
    result.reserve(all.size());
    for (const DataSchema &schema : all) {
        result << schema.name;
    }
    return result;
}

bool SchemaStore::contains(const QString &name) const {
    return m_byName.contains(name);
}

bool SchemaStore::containsTypeId(quint8 typeId) const {
    return m_nameByTypeId.contains(typeId);
}

DataSchema SchemaStore::schema(const QString &name) const {
    return m_byName.value(name);
}

DataSchema SchemaStore::schemaByTypeId(quint8 typeId) const {
    return m_byName.value(m_nameByTypeId.value(typeId));
}

quint8 SchemaStore::suggestTypeId() const {
    for (int candidate = 1; candidate <= 255; ++candidate) {
        if (m_nameByTypeId.contains(static_cast<quint8>(candidate)) == false) {
            return static_cast<quint8>(candidate);
        }
    }
    return 0;
}

void SchemaStore::indexSchema(const DataSchema &schema) {
    m_byName.insert(schema.name, schema);
    m_nameByTypeId.insert(schema.typeId, schema.name);
}

bool SchemaStore::writeSchema(const DataSchema &schema, QString *error) {
    QDir dir(m_directory);
    if (dir.exists() == false && dir.mkpath(QStringLiteral(".")) == false) {
        if (error) *error = QStringLiteral("Cannot create directory %1").arg(m_directory);
        return false;
    }

    QSaveFile file(dir.filePath(fileNameFor(schema)));
    if (file.open(QIODevice::WriteOnly) == false) {
        if (error) *error = file.errorString();
        return false;
    }

    const QJsonDocument document(schema.toJson());
    if (file.write(document.toJson(QJsonDocument::Indented)) < 0) {
        if (error) *error = file.errorString();
        return false;
    }
    if (file.commit() == false) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

bool SchemaStore::validateForSave(const DataSchema &schema, QString *error,
                                 const QString &previousName) const {
    const QString problem = schema.validationError();
    if (problem.isEmpty() == false) {
        if (error) *error = problem;
        return false;
    }
    if (isSafeName(schema.name) == false) {
        if (error) *error = QStringLiteral("Name may contain only letters, digits, '_' and '-'");
        return false;
    }

    // previousName - имя этой же схемы до переименования: её typeId
    // законно остаётся за собой.
    const QString owner = m_nameByTypeId.value(schema.typeId);
    if (owner.isEmpty() == false && owner != schema.name && owner != previousName) {
        if (error) {
            *error = QStringLiteral("Type id %1 is already used by \"%2\"").arg(schema.typeId).arg(owner);
        }
        return false;
    }
    return true;
}

bool SchemaStore::save(const DataSchema &schema, QString *error) {
    // Флаг builtIn выставляется на копии: встроенную схему нельзя
    // переименовать, но можно свободно править её поля.
    DataSchema normalized = schema;
    if (validateForSave(normalized, error) == false) {
        return false;
    }
    if (m_byName.value(normalized.name).builtIn) {
        normalized.builtIn = true;
    }

    if (writeSchema(normalized, error) == false) {
        return false;
    }

    indexSchema(normalized);
    emit schemaSaved(normalized.name);
    return true;
}

bool SchemaStore::remove(const QString &name, QString *error) {
    const DataSchema schema = m_byName.value(name);
    if (schema.name.isEmpty()) {
        if (error) *error = QStringLiteral("Unknown message \"%1\"").arg(name);
        return false;
    }
    if (schema.builtIn) {
        if (error) *error = QStringLiteral("\"%1\" is a built-in message and cannot be deleted").arg(name);
        return false;
    }

    if (QFile::remove(m_directory + QLatin1Char('/') + fileNameFor(schema)) == false) {
        if (error) *error = QStringLiteral("Cannot delete file for \"%1\"").arg(name);
        return false;
    }

    m_byName.remove(name);
    m_nameByTypeId.remove(schema.typeId);
    emit schemaRemoved(name);
    return true;
}

bool SchemaStore::rename(const QString &from, const QString &to, QString *error) {
    const DataSchema source = m_byName.value(from);
    if (source.name.isEmpty()) {
        if (error) *error = QStringLiteral("Unknown message \"%1\"").arg(from);
        return false;
    }
    if (source.builtIn) {
        if (error) *error = QStringLiteral("\"%1\" is a built-in message and cannot be renamed").arg(from);
        return false;
    }
    if (isSafeName(to) == false) {
        if (error) *error = QStringLiteral("Name may contain only letters, digits, '_' and '-'");
        return false;
    }
    if (contains(to)) {
        if (error) *error = QStringLiteral("\"%1\" already exists").arg(to);
        return false;
    }

    DataSchema target = source;
    target.name = to;
    if (validateForSave(target, error, from) == false) {
        return false;
    }

    // Пишем новое имя до удаления старого: если запись не удалась,
    // исходный файл остаётся на месте.
    if (writeSchema(target, error) == false) {
        return false;
    }

    m_byName.remove(source.name);
    indexSchema(target);
    QFile::remove(m_directory + QLatin1Char('/') + fileNameFor(source));

    emit schemaRemoved(from);
    emit schemaSaved(to);
    return true;
}
