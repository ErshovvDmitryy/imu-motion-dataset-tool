#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QByteArray>
#include <QObject>


class DatabaseManager
{
public:
    explicit DatabaseManager(const QString &databaseName = "MouseSettings.db");
    ~DatabaseManager();

    bool open();
    void close();

    bool isOpen() const;

    QSqlDatabase database() const;

private:
    QString m_databaseName;
    QSqlDatabase m_database;
};

#endif // DATABASEMANAGER_H
