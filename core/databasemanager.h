#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>
#include <QMap>
#include <QFileInfo>
#include <QDir>
#include "models/loglevel.h"

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool initialize();  // Start bd
    bool isInitialized() const { return m_isInitialized; }

    bool registerDatabase(const QString &dbName, const QString &filePath,
                          const QString &description = QString());
    bool unregisterDatabase(const QString &dbName);

    QSqlDatabase openDatabase(const QString &dbName);
    void closeDatabase(const QString &dbName);
    bool isDatabaseOpen(const QString &dbName) const;

    QStringList getRegisteredDatabases() const;
    QString getDatabasePath(const QString &dbName) const;
    QString getDatabaseDescription(const QString &dbName) const;
    QSqlDatabase getDatabase(const QString &dbName) const;

    QStringList scanForDatabases(const QString &path = QString()) const;
    bool removeDatabase(const QString &dbName, bool deleteFile = false);
    bool importDatabase(const QString &filePath, const QString &description = QString());

    bool createDatasetDatabase(const QString &dbName, const QString &path,
                               const QString &description = QString());

    QStringList getTableNames(const QString &dbName) const;
    int getTableRowCount(const QString &dbName, const QString &tableName) const;

signals:
    void databaseRegistered(const QString &dbName);
    void databaseUnregistered(const QString &dbName);
    void databaseOpened(const QString &dbName);
    void databaseClosed(const QString &dbName);
    void errorOccurred(const QString &error);
    void logMessage(LogLevel level, const QString &text);

private:
    bool createStartDatabase();
    bool createStartTables();
    bool loadRegisteredDatabases();

    QString getDefaultDatabasePath() const;
    QString getStartDatabasePath() const;
    bool validateDatabase(const QString &filePath) const;

    struct DatabaseInfo {
        QString name;
        QString filePath;
        QString description;
        QString connectionName;
        bool isOpen = false;
    };

    QMap<QString, DatabaseInfo> m_databases;
    QStringList m_startDbRequiredTables = {"databases", "app_settings"};

    bool m_isInitialized = false;
    QString m_startDbName = "StartDB";
    QString m_startDbFilePath;

    static int s_connectionCounter;
    QString generateConnectionName() const;
};
