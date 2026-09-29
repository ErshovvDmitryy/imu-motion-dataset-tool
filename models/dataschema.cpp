#include "models/dataschema.h"

#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

namespace {

const char *kFormatTag    = "qproto";
const int    kFormatVersion = 1;

QString roleToken(FieldRole role) {
    switch (role) {
    case FieldRole::Timestamp: return QStringLiteral("time");
    case FieldRole::Crc16:     return QStringLiteral("crc");
    case FieldRole::Recording: return QStringLiteral("rec");
    case FieldRole::None:      break;
    }
    return QString();
}

QString unitToken(FieldUnit unit) {
    switch (unit) {
    case FieldUnit::Seconds:      return QStringLiteral("s");
    case FieldUnit::Milliseconds: return QStringLiteral("ms");
    case FieldUnit::Microseconds: return QStringLiteral("us");
    case FieldUnit::Nanoseconds:  return QStringLiteral("ns");
    case FieldUnit::None:         break;
    }
    return QString();
}

} // namespace

QString fieldRoleName(FieldRole role) {
    switch (role) {
    case FieldRole::Timestamp: return QStringLiteral("Timestamp");
    case FieldRole::Crc16:     return QStringLiteral("CRC-16");
    case FieldRole::Recording: return QStringLiteral("Recording flag");
    case FieldRole::None:      break;
    }
    return QStringLiteral("None");
}

FieldRole fieldRoleFromName(const QString &name) {
    const QString key = name.trimmed().toLower();
    if (key == QLatin1String("time") || key == QLatin1String("timestamp")) return FieldRole::Timestamp;
    if (key == QLatin1String("crc") || key == QLatin1String("crc16"))      return FieldRole::Crc16;
    if (key == QLatin1String("rec") || key == QLatin1String("recording")) return FieldRole::Recording;
    return FieldRole::None;
}

QString fieldUnitName(FieldUnit unit) {
    switch (unit) {
    case FieldUnit::Seconds:      return QStringLiteral("Seconds");
    case FieldUnit::Milliseconds: return QStringLiteral("Milliseconds");
    case FieldUnit::Microseconds: return QStringLiteral("Microseconds");
    case FieldUnit::Nanoseconds:  return QStringLiteral("Nanoseconds");
    case FieldUnit::None:         break;
    }
    return QStringLiteral("Auto");
}

FieldUnit fieldUnitFromName(const QString &name) {
    const QString key = name.trimmed().toLower();
    if (key == QLatin1String("s")  || key == QLatin1String("sec"))  return FieldUnit::Seconds;
    if (key == QLatin1String("ms") || key == QLatin1String("msec")) return FieldUnit::Milliseconds;
    if (key == QLatin1String("us") || key == QLatin1String("usec")) return FieldUnit::Microseconds;
    if (key == QLatin1String("ns") || key == QLatin1String("nsec")) return FieldUnit::Nanoseconds;
    return FieldUnit::None;
}

double fieldUnitToSeconds(FieldUnit unit) {
    switch (unit) {
    case FieldUnit::Seconds:      return 1.0;
    case FieldUnit::Milliseconds: return 1e-3;
    case FieldUnit::Microseconds: return 1e-6;
    case FieldUnit::Nanoseconds:  return 1e-9;
    case FieldUnit::None:         break;
    }
    return 0.0; // авто-подбор по разрядности значения
}

// ============================== DataField ==============================

bool DataField::isVariable() const {
    if (type != DataValueType::String && type != DataValueType::Bytes) {
        return false;
    }
    return length <= 0;
}

int DataField::elementSize() const {
    const int base = dataValueTypeSize(type);
    if (base > 0) {
        return base;
    }
    return isVariable() ? 0 : length;
}

int DataField::byteSize() const {
    const int size = elementSize();
    if (size <= 0) {
        return 0;
    }
    return isVariable() ? 0 : size * slotCount();
}

int DataField::slotCount() const {
    if (isVariable()) {
        return 1;
    }
    return count < 1 ? 1 : count;
}

bool DataField::isPlotable() const {
    return dataValueTypeIsNumeric(type);
}

QString DataField::label() const {
    if (!isArray()) {
        return name;
    }
    return QStringLiteral("%1[0..%2]").arg(name).arg(slotCount() - 1);
}

QString DataField::slotName(int slot) const {
    if (!isArray()) {
        return name;
    }
    return QStringLiteral("%1[%2]").arg(name).arg(slot);
}

bool DataField::operator==(const DataField &other) const {
    return name            == other.name
        && type            == other.type
        && count           == other.count
        && length          == other.length
        && role            == other.role
        && unit            == other.unit;
}

// ============================== DataSchema ==============================

DataSchema::DataSchema()
{
}

bool DataSchema::isValid() const {
    return validationError().isEmpty();
}

QString DataSchema::validationError() const {
    if (name.trimmed().isEmpty()) {
        return QStringLiteral("Message name is empty");
    }
    if (fields.isEmpty()) {
        return QStringLiteral("Message \"%1\" has no fields").arg(name);
    }

    QVector<bool> rolesUsed(4, false);
    QSet<QString> seen;

    for (int i = 0; i < fields.size(); ++i) {
        const DataField &field = fields.at(i);

        if (field.name.trimmed().isEmpty()) {
            return QStringLiteral("Field #%1 has an empty name").arg(i + 1);
        }
        if (seen.contains(field.name)) {
            return QStringLiteral("Duplicate field name \"%1\"").arg(field.name);
        }
        seen.insert(field.name);

        if (field.type == DataValueType::Invalid) {
            return QStringLiteral("Field \"%1\" has an unknown type").arg(field.name);
        }
        if (field.count < 1) {
            return QStringLiteral("Field \"%1\" must repeat at least once").arg(field.name);
        }
        if (field.isVariable() && framing != FramingMode::HeaderLength) {
            return QStringLiteral("Field \"%1\" has no length; use header framing "
                                  "to let it take the rest of the frame").arg(field.name);
        }

        if (field.role != FieldRole::None) {
            const int slot = static_cast<int>(field.role);
            if (rolesUsed.at(slot)) {
                return QStringLiteral("Role %1 is used more than once")
                    .arg(fieldRoleName(field.role));
            }
            rolesUsed[slot] = true;
        }

        if (field.role == FieldRole::Crc16 && i != fields.size() - 1) {
            return QStringLiteral("CRC-16 field must be the last one");
        }
        if (field.role == FieldRole::Crc16 && field.byteSize() != 2) {
            return QStringLiteral("CRC-16 field must occupy exactly 2 bytes");
        }
        if (field.role == FieldRole::Recording
            && (field.type != DataValueType::Bool || field.slotCount() != 1)) {
            return QStringLiteral("Recording flag must be a single bool value");
        }
        if (field.role == FieldRole::Timestamp && field.isPlotable() == false) {
            return QStringLiteral("Timestamp field must be numeric");
        }
    }

    return QString();
}

int DataSchema::payloadSize() const {
    int total = 0;
    for (const DataField &field : fields) {
        total += field.byteSize();
    }
    return total;
}

int DataSchema::minPayloadSize() const {
    int total = 0;
    for (const DataField &field : fields) {
        if (field.isVariable()) {
            continue;
        }
        total += field.byteSize();
    }
    return total;
}

int DataSchema::headerSize() const {
    return framing == FramingMode::HeaderLength ? 3 : 1;
}

int DataSchema::totalSize() const {
    return headerSize() + payloadSize();
}

int DataSchema::slotCount() const {
    int total = 0;
    for (const DataField &field : fields) {
        total += field.slotCount();
    }
    return total;
}

bool DataSchema::hasVariableFields() const {
    for (const DataField &field : fields) {
        if (field.isVariable()) {
            return true;
        }
    }
    return false;
}

int DataSchema::fieldIndex(const QString &fieldName) const {
    for (int i = 0; i < fields.size(); ++i) {
        if (fields.at(i).name == fieldName) {
            return i;
        }
    }
    return -1;
}

int DataSchema::slotFieldIndex(int slot) const {
    int base = 0;
    for (int i = 0; i < fields.size(); ++i) {
        const int fieldSlots = fields.at(i).slotCount();
        if (slot < base + fieldSlots) {
            return i;
        }
        base += fieldSlots;
    }
    return -1;
}

int DataSchema::slotIndex(const QString &slotName) const {
    int base = 0;
    for (const DataField &field : fields) {
        const int fieldSlots = field.slotCount();
        if (field.isArray() == false) {
            if (field.name == slotName) {
                return base;
            }
        } else {
            if (field.name == slotName) {
                return base;
            }
            for (int slot = 0; slot < fieldSlots; ++slot) {
                if (field.slotName(slot) == slotName) {
                    return base + slot;
                }
            }
        }
        base += fieldSlots;
    }
    return -1;
}

QStringList DataSchema::slotNames() const {
    QStringList names;
    for (const DataField &field : fields) {
        for (int slot = 0; slot < field.slotCount(); ++slot) {
            names << field.slotName(slot);
        }
    }
    return names;
}

QString DataSchema::slotName(int slot) const {
    const int fieldIndex = slotFieldIndex(slot);
    if (fieldIndex < 0) {
        return QString();
    }
    const DataField &field = fields.at(fieldIndex);
    int inner = slot;
    for (int i = 0; i < fieldIndex; ++i) {
        inner -= fields.at(i).slotCount();
    }
    return field.slotName(inner);
}

int DataSchema::roleSlotIndex(FieldRole role) const {
    if (role == FieldRole::None) {
        return -1;
    }
    int base = 0;
    for (const DataField &field : fields) {
        if (field.role == role) {
            return base;
        }
        base += field.slotCount();
    }
    return -1;
}

QVector<PlottedField> DataSchema::plottableFields() const {
    QVector<PlottedField> result;
    for (int i = 0; i < fields.size(); ++i) {
        const DataField &field = fields.at(i);
        if (field.isPlotable() == false) {
            continue;
        }
        for (int slot = 0; slot < field.slotCount(); ++slot) {
            PlottedField plotted;
            plotted.name = field.slotName(slot);
            plotted.fieldIndex = i;
            plotted.slot = slot;
            plotted.type = field.type;
            plotted.numeric = true;
            result.append(plotted);
        }
    }
    return result;
}

QStringList DataSchema::plottableSlotNames() const {
    QStringList names;
    const QVector<PlottedField> fields = plottableFields();
    for (const PlottedField &field : fields) {
        names << field.name;
    }
    return names;
}

QString DataSchema::traceLabel(const PlottedField &field) const {
    if (field.fieldIndex < 0 || field.fieldIndex >= fields.size()) {
        return field.name;
    }
    const DataField &owner = fields.at(field.fieldIndex);
    const double factor = fieldUnitToSeconds(owner.unit);
    if (field.numeric == false || factor == 0.0) {
        return field.name;
    }
    return QStringLiteral("%1 (%2)").arg(field.name).arg(fieldUnitName(owner.unit).toLower());
}

QString DataSchema::slotLabel(const QString &slotName) const {
    const int index = slotIndex(slotName);
    if (index < 0) {
        return slotName;
    }
    PlottedField field;
    field.name = slotName;
    field.fieldIndex = slotFieldIndex(index);
    field.slot = index;
    if (field.fieldIndex >= 0) {
        field.type = fields.at(field.fieldIndex).type;
    }
    field.numeric = dataValueTypeIsNumeric(field.type);
    return traceLabel(field);
}

QString DataSchema::framingName(FramingMode mode) {
    return mode == FramingMode::HeaderLength ? QStringLiteral("header") : QStringLiteral("fixed");
}

FramingMode DataSchema::framingFromName(const QString &name) {
    return name.trimmed().toLower() == QLatin1String("header")
         ? FramingMode::HeaderLength
         : FramingMode::FixedLength;
}

bool DataSchema::decode(const QByteArray &payload, QVector<DataValue> *out, QString *error) const {
    if (out == nullptr) {
        return false;
    }
    out->clear();
    out->reserve(slotCount());

    int offset = 0;
    for (const DataField &field : fields) {
        if (field.isVariable()) {
            const int rest = payload.size() - offset;
            if (rest < 0) {
                if (error) {
                    *error = QStringLiteral("Payload is shorter than the fixed part");
                }
                out->clear();
                return false;
            }
            out->append(DataValue::fromRaw(field.type, payload.constData() + offset, rest));
            offset = payload.size();
            continue;
        }

        const int elementSize = field.elementSize();
        for (int slot = 0; slot < field.slotCount(); ++slot) {
            if (offset + elementSize > payload.size()) {
                if (error) {
                    *error = QStringLiteral("Payload ended inside field \"%1\"").arg(field.name);
                }
                out->clear();
                return false;
            }
            out->append(DataValue::fromRaw(field.type, payload.constData() + offset, elementSize));
            offset += elementSize;
        }
    }

    if (error) {
        error->clear();
    }
    return true;
}

// ============================== JSON ==============================

QJsonObject DataSchema::toJson() const {
    QJsonObject object;
    object.insert(QStringLiteral("format"), QLatin1String(kFormatTag));
    object.insert(QStringLiteral("version"), kFormatVersion);
    object.insert(QStringLiteral("name"), name);
    object.insert(QStringLiteral("typeId"), static_cast<int>(typeId));
    object.insert(QStringLiteral("framing"), framingName(framing));
    object.insert(QStringLiteral("builtIn"), builtIn);
    if (description.isEmpty() == false) {
        object.insert(QStringLiteral("description"), description);
    }

    QJsonArray array;
    for (const DataField &field : fields) {
        QJsonObject item;
        item.insert(QStringLiteral("name"), field.name);
        item.insert(QStringLiteral("type"), dataValueTypeName(field.type));
        if (field.isArray()) {
            item.insert(QStringLiteral("count"), field.count);
        }
        if (field.length > 0) {
            item.insert(QStringLiteral("length"), field.length);
        }
        if (field.role != FieldRole::None) {
            item.insert(QStringLiteral("role"), roleToken(field.role));
        }
        if (field.unit != FieldUnit::None) {
            item.insert(QStringLiteral("unit"), unitToken(field.unit));
        }
        array.append(item);
    }
    object.insert(QStringLiteral("fields"), array);

    return object;
}

DataSchema DataSchema::fromJson(const QJsonObject &object, QString *error) {
    DataSchema schema;
    auto fail = [error](const QString &text) {
        if (error) {
            *error = text;
        }
    };

    if (object.value(QStringLiteral("format")).toString() != QLatin1String(kFormatTag)) {
        fail(QStringLiteral("Not a %1 file").arg(QLatin1String(kFormatTag)));
        return schema;
    }

    schema.name = object.value(QStringLiteral("name")).toString();
    schema.typeId = static_cast<quint8>(object.value(QStringLiteral("typeId")).toInt(0));
    schema.framing = framingFromName(object.value(QStringLiteral("framing")).toString());
    schema.builtIn = object.value(QStringLiteral("builtIn")).toBool(false);
    schema.description = object.value(QStringLiteral("description")).toString();

    const QJsonArray array = object.value(QStringLiteral("fields")).toArray();
    for (const QJsonValue &entry : array) {
        const QJsonObject item = entry.toObject();
        DataField field;
        field.name = item.value(QStringLiteral("name")).toString();
        field.type = dataValueTypeFromName(item.value(QStringLiteral("type")).toString());
        field.count = item.value(QStringLiteral("count")).toInt(1);
        field.length = item.value(QStringLiteral("length")).toInt(0);
        field.role = fieldRoleFromName(item.value(QStringLiteral("role")).toString());
        field.unit = fieldUnitFromName(item.value(QStringLiteral("unit")).toString());
        schema.fields.append(field);
    }

    if (error) {
        *error = schema.validationError();
    }
    return schema;
}

// ============================== DSL ==============================

QVector<DataField> DataSchema::parseFieldList(const QString &text, QString *error) {
    QVector<DataField> result;
    QStringList errors;

    static const QRegularExpression tokenPattern(
        QStringLiteral("^\\s*([A-Za-z_][A-Za-z0-9_]*)\\s*"
                       "(?::\\s*([A-Za-z0-9_]+)\\s*)?"
                       "(?:\\*\\s*(\\d+)\\s*)?"
                       "(.*)$"));

    const QStringList tokens = text.split(QRegularExpression(QStringLiteral("[;\\r\\n]")));
    for (const QString &rawToken : tokens) {
        const QString token = rawToken.trimmed();
        if (token.isEmpty() || token.startsWith(QLatin1Char('#'))) {
            continue;
        }

        const QRegularExpressionMatch match = tokenPattern.match(token);
        if (!match.hasMatch()) {
            errors << QStringLiteral("Cannot parse \"%1\"").arg(token);
            continue;
        }

        DataField field;
        field.name = match.captured(1);

        const QString typeName = match.captured(2).trimmed();
        if (typeName.isEmpty()) {
            errors << QStringLiteral("Field \"%1\" has no type").arg(field.name);
            continue;
        }
        field.type = dataValueTypeFromName(typeName);
        if (field.type == DataValueType::Invalid) {
            errors << QStringLiteral("Field \"%1\" has unknown type \"%2\"").arg(field.name, typeName);
            continue;
        }

        if (match.captured(3).isEmpty() == false) {
            field.count = match.captured(3).toInt();
        }

        // Остаток: @time@us@len=8 - модификаторы роли, единицы и длины.
        const QStringList modifiers = match.captured(4).split(QLatin1Char('@'));
        for (const QString &rawModifier : modifiers) {
            const QString key = rawModifier.trimmed();
            if (key.isEmpty()) {
                continue;
            }
            if (key.startsWith(QLatin1String("len="), Qt::CaseInsensitive)) {
                field.length = key.mid(4).trimmed().toInt();
                continue;
            }
            if (field.role == FieldRole::None && fieldRoleFromName(key) != FieldRole::None) {
                field.role = fieldRoleFromName(key);
                continue;
            }
            if (field.unit == FieldUnit::None && fieldUnitFromName(key) != FieldUnit::None) {
                field.unit = fieldUnitFromName(key);
                continue;
            }
            errors << QStringLiteral("Field \"%1\": unknown modifier \"%2\"").arg(field.name, key);
        }

        // Для строк и блобов без явной длины оставляем length == 0:
        // в режиме HeaderLength такое поле заберёт остаток кадра.
        result.append(field);
    }

    if (error) {
        *error = errors.join(QStringLiteral("; "));
    }
    return result;
}

QString DataSchema::fieldListToDsl(const QVector<DataField> &fields) {
    QStringList tokens;
    for (const DataField &field : fields) {
        QString token = QStringLiteral("%1:%2").arg(field.name, dataValueTypeName(field.type));
        if (field.isArray()) {
            token += QStringLiteral("*%1").arg(field.count);
        }
        if (field.length > 0) {
            token += QStringLiteral("@len=%1").arg(field.length);
        }
        if (field.role != FieldRole::None) {
            token += QLatin1Char('@') + roleToken(field.role);
        }
        if (field.unit != FieldUnit::None) {
            token += QLatin1Char('@') + unitToken(field.unit);
        }
        tokens << token;
    }
    return tokens.join(QStringLiteral("; "));
}

QString DataSchema::dsl() const {
    return fieldListToDsl(fields);
}

bool DataSchema::operator==(const DataSchema &other) const {
    return name    == other.name
        && typeId  == other.typeId
        && framing == other.framing
        && builtIn == other.builtIn
        && fields  == other.fields;
}
