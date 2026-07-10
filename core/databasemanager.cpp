#include "databasemanager.h"

DatabaseManager::DatabaseManager(const QString &databaseName)
    : m_databaseName(databaseName)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::open()
{
    // Если уже открыта — ничего не делаем
    if (m_database.isOpen())
        return true;

    // Создаем подключение SQLite
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(m_databaseName);

    if (!m_database.open())
    {
        qDebug() << "Database open error:"
                 << m_database.lastError().text();
        return false;
    }

    qDebug() << "Database opened:" << m_databaseName;

    return true;
}

void DatabaseManager::close()
{
    if (!m_database.isValid())
        return;

    if (m_database.isOpen())
        m_database.close();

    // Сохраняем имя соединения перед удалением
    QString connectionName = m_database.connectionName();

    // Освобождаем объект соединения
    m_database = QSqlDatabase();

    QSqlDatabase::removeDatabase(connectionName);

    qDebug() << "Database closed";
}

bool DatabaseManager::isOpen() const
{
    return m_database.isOpen();
}

QSqlDatabase DatabaseManager::database() const
{
    return m_database;
}
