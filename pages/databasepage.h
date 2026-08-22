#pragma once

#include <QWidget>
#include <pages/opendbdialog.h>
#include <pages/createdbdialog.h>

class QTreeView;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QComboBox;
class QStandardItemModel;
class DatabaseManager;

class DataBasePage : public QWidget
{
    Q_OBJECT
public:
    explicit DataBasePage(DatabaseManager *dbManager, QWidget *parent = nullptr);

private:
    void createWidgets();
    void createLayouts();
    void connectSignals();

    void refreshDatabaseList();
    void openDBDialog();

    void createDBDialog();

    QHBoxLayout *mainLayout;
    QVBoxLayout *leftLayout;
    QVBoxLayout *rigthLayout;

    QPushButton *btnOpenDB;
    QPushButton *btnDeleteDB;
    QPushButton *btnEditDB;
    QPushButton *btnCreateDB;

    QPushButton *btnFillRight; //deleteLater

    QTreeView *treeView;
    QStandardItemModel *treeModel;

    DatabaseManager *m_dbManager;

    QString m_currentDatabase;

signals:
    void createBD();
};
