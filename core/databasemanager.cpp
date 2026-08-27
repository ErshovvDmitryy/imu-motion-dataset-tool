#include "core/databasemanager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStringList>
#include <QVector>

#include "models/motionsample.h"
#include "models/MotionType.h"


int DatabaseManager::s_connectionCounter = 0;

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
    if (!QSqlDatabase::drivers().contains("QSQLITE")) {
        logMessage(LogLevel::Warning, QString("SQLite driver not avaliable"));
    }
}

DatabaseManager::~DatabaseManager() {
    for (auto it = m_databases.begin(); it != m_databases.end(); ++it) {
        if (it->isOpen) {
            closeDatabase(it.key());
        }
    }
}

bool DatabaseManager::initialize() {
    if (m_isInitialized) {
        logMessage(LogLevel::Warning, QString("DatabaseManager already initialized"));
        return true;
    }

    m_startDbFilePath = getStartDatabasePath();

    if (!QFileInfo::exists(m_startDbFilePath)) {
        if (!createStartDatabase()) {
            logMessage(LogLevel::Error, QString("Failed to create start database"));
            emit errorOccurred("Failed to create start database");
            return false;
        }
    }

    if (!m_databases.contains(m_startDbName)) {
        DatabaseInfo info;
        info.name = m_startDbName;
        info.filePath = m_startDbFilePath;
        info.description = "Start database";
        info.connectionName = generateConnectionName();
        info.isOpen = false;
        m_databases[m_startDbName] = info;
    }

    QSqlDatabase startDb = openDatabase(m_startDbName);

    if (!startDb.isOpen()) {
        logMessage(LogLevel::Error, QString("Failed to open start database"));
        emit errorOccurred("Failed to open start database");
        return false;
    }

    if (!createStartTables()) {
        logMessage(LogLevel::Error, QString("Failed to create start tables"));
        emit errorOccurred("Failed to create start tables");
        return false;
    }

    if (!loadRegisteredDatabases()) {
        logMessage(LogLevel::Error, QString("Failed to load registered databases"));
        emit errorOccurred("Failed to load registered databases");
        return false;
    }

    m_isInitialized = true;
    emit databaseOpened(m_startDbName);
    logMessage(LogLevel::Info, QString("Start data base is opened"));
    return true;
}

bool DatabaseManager::createStartDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
                                                 generateConnectionName());
    db.setDatabaseName(m_startDbFilePath);

    if (!db.open()) {
        qDebug() << "DB not opened";
        return false;
    }

    QSqlQuery query(db);

    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS databases ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL, "
        "file_path TEXT NOT NULL, "
        "description TEXT, "
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
        "last_opened TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")"
    )) {
        return false;
        qDebug() << "TABLE not exec";
    }

    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS app_settings ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "key TEXT UNIQUE NOT NULL, "
        "value TEXT, "
        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")"
    )) {
        qDebug() << "TABLE app_set not exec";
        return false;
    }

    query.exec("INSERT OR IGNORE INTO app_settings (key, value) VALUES "
               "('app_version', '1.0.0'), "
               "('data_path', ''), "
               "('last_database', '')");

    db.close();
    return true;
}

bool DatabaseManager::createStartTables() {
    QSqlDatabase db = getDatabase(m_startDbName);
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);

    QStringList tables = db.tables();
    bool hasDatabases = tables.contains("databases");
    bool hasSettings = tables.contains("app_settings");

    if (!hasDatabases || !hasSettings) {
        db.close();
        QFile::remove(m_startDbFilePath);
        return createStartDatabase();
    }

    return true;
}

bool DatabaseManager::loadRegisteredDatabases() {
    QSqlDatabase db = getDatabase(m_startDbName);
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    if (!query.exec("SELECT name, file_path, description FROM databases")) {
        return false;
    }

    while (query.next()) {
        QString name = query.value(0).toString();
        QString filePath = query.value(1).toString();
        QString description = query.value(2).toString();

        if (QFileInfo::exists(filePath)) {
            DatabaseInfo info;
            info.name = name;
            info.filePath = filePath;
            info.description = description;
            info.connectionName = generateConnectionName();
            info.isOpen = false;

            m_databases[name] = info;
        } else {
            //logMessage();
        }
    }

    return true;
}

QSqlDatabase DatabaseManager::openDatabase(const QString &dbName) {
    if (!m_databases.contains(dbName)) {
        return QSqlDatabase();
    }

    DatabaseInfo &info = m_databases[dbName];

    if (info.isOpen) {
        return QSqlDatabase::database(info.connectionName);
    }

    if (!QFileInfo::exists(info.filePath)) {
        return QSqlDatabase();
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", info.connectionName);
    db.setDatabaseName(info.filePath);

    if (!db.open()) {
        emit errorOccurred(QString("Failed to open database %1: %2")
                              .arg(dbName, db.lastError().text()));
        return QSqlDatabase();
    }

    db.exec("PRAGMA foreign_keys = ON");

    info.isOpen = true;
    emit databaseOpened(dbName);

    return db;
}

void DatabaseManager::closeDatabase(const QString &dbName) {
    if (!m_databases.contains(dbName)) {
        return;
    }

    DatabaseInfo &info = m_databases[dbName];
    if (!info.isOpen) {
        return;
    }

    QSqlDatabase db = QSqlDatabase::database(info.connectionName);
    if (db.isOpen()) {
        db.close();
    }

    QSqlDatabase::removeDatabase(info.connectionName);

    info.isOpen = false;
    emit databaseClosed(dbName);
}

bool DatabaseManager::isDatabaseOpen(const QString &dbName) const {
    return m_databases.contains(dbName) && m_databases[dbName].isOpen;
}

QSqlDatabase DatabaseManager::getDatabase(const QString &dbName) const {
    if (!m_databases.contains(dbName)) {
        return QSqlDatabase();
    }

    const DatabaseInfo &info = m_databases[dbName];
    if (!info.isOpen) {
        return QSqlDatabase();
    }

    return QSqlDatabase::database(info.connectionName);
}

bool DatabaseManager::registerDatabase(const QString &dbName, const QString &filePath, const QString &description) {
    if (m_databases.contains(dbName)) {
        return false;
    }

    if (!QFileInfo::exists(filePath)) {
        return false;
    }

    DatabaseInfo info;
    info.name = dbName;
    info.filePath = filePath;
    info.description = description;
    info.connectionName = generateConnectionName();
    info.isOpen = false;

    m_databases[dbName] = info;

    QSqlDatabase startDb = getDatabase(m_startDbName);
    if (startDb.isOpen()) {
        QSqlQuery query(startDb);
        query.prepare("INSERT OR REPLACE INTO databases (name, file_path, description) "
                      "VALUES (?, ?, ?)");
        query.addBindValue(dbName);
        query.addBindValue(filePath);
        query.addBindValue(description);

        if (!query.exec()) {
            // toDo debug massage
        }
    }

    emit databaseRegistered(dbName);
    return true;
}

bool DatabaseManager::unregisterDatabase(const QString &dbName) {
    if (!m_databases.contains(dbName)) {
        return false;
    }

    if (m_databases[dbName].isOpen) {
        closeDatabase(dbName);
    }

    m_databases.remove(dbName);

    QSqlDatabase startDb = getDatabase(m_startDbName);
    if (startDb.isOpen()) {
        QSqlQuery query(startDb);
        query.prepare("DELETE FROM databases WHERE name = ?");
        query.addBindValue(dbName);
        query.exec();
    }

    emit databaseUnregistered(dbName);
    return true;
}

QStringList DatabaseManager::scanForDatabases(const QString &path) const {
    QStringList foundDatabases;
    QString searchPath = path.isEmpty() ? getDefaultDatabasePath() : path;

    QDir dir(searchPath);
    if (!dir.exists()) {
        return foundDatabases;
    }

    QStringList filters;
    filters << "*.db" << "*.sqlite" << "*.sqlite3";
    dir.setNameFilters(filters);

    for (const QString &fileName : dir.entryList(QDir::Files)) {
        QString fullPath = dir.absoluteFilePath(fileName);
        if (validateDatabase(fullPath)) {
            foundDatabases << fullPath;
        }
    }

    return foundDatabases;
}

bool DatabaseManager::validateDatabase(const QString &filePath) const {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "temp_validation");
    db.setDatabaseName(filePath);

    if (!db.open()) {
        return false;
    }

    QStringList tables = db.tables();
    bool isValid = !tables.isEmpty();

    db.close();
    QSqlDatabase::removeDatabase("temp_validation");

    return isValid;
}

bool DatabaseManager::removeDatabase(const QString &dbName, bool deleteFile) {
    if (!m_databases.contains(dbName)) {
        return false;
    }

    if (dbName == m_startDbName) {
        return false;
    }

    QString filePath = m_databases[dbName].filePath;

    if (!unregisterDatabase(dbName)) {
        return false;
    }

    if (deleteFile && QFileInfo::exists(filePath)) {
        if (!QFile::remove(filePath)) {
            return false;
        }
        qDebug() << "Database file deleted:" << filePath;
    }

    return true;
}

QStringList DatabaseManager::getTableNames(const QString &dbName) {
    QSqlDatabase db = openDatabase(dbName);
    if (!db.isOpen()) {
        qDebug() << "Нет доступа " << dbName;
        return QStringList();
    }

    return db.tables();
}

int DatabaseManager::getTableRowCount(const QString &dbName, const QString &tableName) {
    QSqlDatabase db = openDatabase(dbName);
    if (!db.isOpen()) {
        return -1;
    }

    QSqlQuery query(db);
    query.prepare(QString("SELECT COUNT(*) FROM %1").arg(tableName));

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return -1;
}

QStringList DatabaseManager::getTableRowsNames(const QString &dbName, const QString &tableName) {
    QSqlDatabase db = openDatabase(dbName);

    if (!db.isOpen()) {
        return QStringList();
    }

    QSqlQuery query(db);
    query.prepare(QString("SELECT * FROM %1").arg(tableName));

    QStringList columnNames;

    if (query.exec()) {
        QSqlRecord record = query.record();
        for (int i = 0; i < record.count(); i++) {
            columnNames.append(record.fieldName(i));
        }
    }

    return columnNames;
}

QString DatabaseManager::getDefaultDatabasePath() const {
    QString appPath = QCoreApplication::applicationDirPath();
    QDir dir(appPath);

    QString dataPath = appPath + "/data";
    if (!dir.exists("data")) {
        dir.mkdir("data");
    }

    return dataPath;
}

QString DatabaseManager::getStartDatabasePath() const {
    return getDefaultDatabasePath() + "/" + m_startDbName + ".db";
}

QString DatabaseManager::generateConnectionName() const
{
    return QString("db_conn_%1_%2")
           .arg(s_connectionCounter++)
           .arg(QDateTime::currentMSecsSinceEpoch());
}

QStringList DatabaseManager::getRegisteredDatabases() const {
    return m_databases.keys();
}

QString DatabaseManager::getDatabasePath(const QString &dbName) const {
    if (!m_databases.contains(dbName)) {
        return QString();
    }
    return m_databases[dbName].filePath;
}

QString DatabaseManager::getDatabaseDescription(const QString &dbName) const {
    if (!m_databases.contains(dbName)) {
        return QString();
    }
    return m_databases[dbName].description;
}

bool DatabaseManager::importDatabase(const QString &filePath,
                                     const QString &description)
{
    if (!QFileInfo::exists(filePath)) {
        emit errorOccurred(QString("File not found: %1").arg(filePath));
        return false;
    }

    if (!validateDatabase(filePath)) {
        emit errorOccurred(QString("Invalid database file: %1").arg(filePath));
        return false;
    }

    QFileInfo fi(filePath);
    QString dbName = fi.completeBaseName();

    if (m_databases.contains(dbName)) {
        emit errorOccurred(QString("Database '%1' is already registered").arg(dbName));
        return false;
    }

    return registerDatabase(dbName, filePath, description);
}

bool DatabaseManager::createDatasetDatabase( const QString &dbName, const QString &path, const QString &description) {
    const QString filePath = path + "/" + dbName + ".db";

    if (QFileInfo::exists(filePath)) {
        emit errorOccurred(
            QString("File already exists: %1").arg(filePath));
        return false;
    }

    QDir dir(path);

    if (!dir.exists() && !dir.mkpath(path)) {
        emit errorOccurred(
            QString("Failed to create directory: %1").arg(path));
        return false;
    }

    const QString connName = generateConnectionName();

    {
        QSqlDatabase db =
            QSqlDatabase::addDatabase("QSQLITE", connName);

        db.setDatabaseName(filePath);

        if (!db.open()) {
            emit errorOccurred(
                QString("Failed to create database: %1")
                    .arg(db.lastError().text()));

            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(connName);
            return false;
        }

        db.exec("PRAGMA foreign_keys = ON");

        if (!db.transaction()) {
            emit errorOccurred(
                QString("Failed to start transaction: %1")
                    .arg(db.lastError().text()));

            db.close();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(connName);
            return false;
        }

        QSqlQuery query(db);

        if (!query.exec(
                "CREATE TABLE IF NOT EXISTS motion_types ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "description TEXT, "
                "is_export INTEGER DEFAULT 0, "
                "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                ")"
            )) {
                emit errorOccurred(QString("Failed to create motion_types: %1").arg(query.lastError().text()));
                db.close();
                QSqlDatabase::removeDatabase(connName);
                return false;
            }

        query.prepare("INSERT OR REPLACE INTO motion_types (id, description, is_export) "
                      "VALUES (1, :description, 0)");
        query.bindValue(":description", description);

       if (!query.exec())
       {
           emit errorOccurred(
               QString("Failed to update db state: %1")
                   .arg(query.lastError().text()));
       }

        if (!query.exec(
            "CREATE TABLE samples ("
            "id INTEGER PRIMARY KEY,"
            "motion_type INTEGER NOT NULL,"
            "sample_count INTEGER NOT NULL,"
            "crc16 INTEGER,"
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ")"))
        {
            db.rollback();

            emit errorOccurred(
                QString("Failed to create samples: %1")
                    .arg(query.lastError().text()));

            db.close();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(connName);
            return false;
        }

        if (!query.exec(
            "CREATE TABLE motion_data ("
            "id INTEGER PRIMARY KEY,"
            "sample_id INTEGER NOT NULL,"
            "sample_index INTEGER NOT NULL,"
            "ax REAL NOT NULL,"
            "ay REAL NOT NULL,"
            "az REAL NOT NULL,"
            "gx REAL NOT NULL,"
            "gy REAL NOT NULL,"
            "gz REAL NOT NULL,"
            "timestamp INTEGER NOT NULL,"
            "FOREIGN KEY (sample_id) REFERENCES samples(id)"
            ")"))
        {
            db.rollback();

            emit errorOccurred(
                QString("Failed to create motion_data: %1")
                    .arg(query.lastError().text()));

            db.close();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(connName);
            return false;
        }

        if (!db.commit()) {
            emit errorOccurred(
                QString("Failed to commit database: %1")
                    .arg(db.lastError().text()));

            db.close();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(connName);
            return false;
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(connName);

    return registerDatabase(
        dbName,
        filePath,
        description);
}

bool DatabaseManager::insertGesture(const QString &dbName, MotionType type, const QVector<MotionSample> &samples, uint16_t crc16) {
    qDebug() << "insertGesture";

    if (samples.isEmpty())
        return false;

    QSqlDatabase db = openDatabase(dbName);
    if (!db.isOpen()) {
        emit errorOccurred(QString("Cannot open database %1 for insert").arg(dbName));
        return false;
    }

    if (!db.transaction()) {
        emit errorOccurred(QString("Failed to start transaction: %1").arg(db.lastError().text()));
        return false;
    }

    QSqlQuery query(db);

    MotionSample s = samples.at(0);
    qDebug() << s.ax << " " << s.ay << " " << s.az;
    qDebug() << s.gx << " " << s.gy << " " << s.gz;
    qDebug() << s.time;

    query.prepare("INSERT INTO samples (motion_type, sample_count, crc16) "
                  "VALUES (:motion_type, :sample_count, :crc16)");
    query.bindValue(":motion_type", static_cast<int>(type));
    query.bindValue(":sample_count", samples.size());
    query.bindValue(":crc16", crc16);

    if (!query.exec()) {
        emit errorOccurred(QString("Failed to insert sample: %1").arg(query.lastError().text()));
        db.rollback();
        return false;
    }

    const int sampleId = query.lastInsertId().toInt();

    query.prepare("INSERT INTO motion_data "
                  "(sample_id, sample_index, ax, ay, az, gx, gy, gz, timestamp) "
                  "VALUES (:sample_id, :sample_index, :ax, :ay, :az, :gx, :gy, :gz, :timestamp)");

    for (int i = 0; i < samples.size(); ++i) {
        const MotionSample &s = samples.at(i);
        query.bindValue(":sample_id", sampleId);
        query.bindValue(":sample_index", i);
        query.bindValue(":ax", s.ax);
        query.bindValue(":ay", s.ay);
        query.bindValue(":az", s.az);
        query.bindValue(":gx", s.gx);
        query.bindValue(":gy", s.gy);
        query.bindValue(":gz", s.gz);
        query.bindValue(":timestamp", static_cast<qint64>(s.time));

        if (!query.exec()) {
            emit errorOccurred(QString("Failed to insert motion_data: %1").arg(query.lastError().text()));
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        emit errorOccurred(QString("Failed to commit: %1").arg(db.lastError().text()));
        db.rollback();
        return false;
    }

    return true;
}

bool DatabaseManager::updateGestureMotionType(const QString &dbName, int sampleId, MotionType type) {
    QSqlDatabase db = openDatabase(dbName);
    if (!db.isOpen())
        return false;

    QSqlQuery query(db);
    query.prepare("UPDATE samples SET motion_type = :motion_type WHERE id = :id");
    query.bindValue(":motion_type", static_cast<int>(type));
    query.bindValue(":id", sampleId);

    if (!query.exec()) {
        emit errorOccurred(QString("Failed to update motion type: %1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool DatabaseManager::isExportDatabase(const QString &datasetDbName) const {
    if (!m_databases.contains(datasetDbName))
        return false;

    const DatabaseInfo &info = m_databases[datasetDbName];
    if (!QFileInfo::exists(info.filePath))
        return false;

    const QString connName = generateConnectionName();
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(info.filePath);

    bool result = false;
    if (db.open()) {
        db.exec("PRAGMA foreign_keys = ON");
        QSqlQuery query(db);
        if (query.exec("SELECT is_export FROM motion_types WHERE id = 1") && query.next())
            result = query.value(0).toInt() != 0;
    }

    db.close();
    QSqlDatabase::removeDatabase(connName);
    return result;
}
