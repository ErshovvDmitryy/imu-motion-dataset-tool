#pragma once

#include "pages/opendbdialog.h"

#include <QObject>
#include <QWidget>
#include <QString>
#include <QList>
#include <QMap>

class QCheckBox;

class QLabel;
class QGroupBox;
class QLineEdit;
class QTextEdit;
class QSplitter;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class DatabaseManager;

class QTreeView;
class QTableView;
class QStandardItem;
class QSqlTableModel;
class QStandardItemModel;

struct SelectedRecord {
    QString dbName;
    QString tableName;
    int id;
};

class ExportCSV : public QWidget
{
    Q_OBJECT

public:

    explicit ExportCSV(DatabaseManager *dbManager, QWidget *parent);

private:

    void createWidgets();
    void createLayouts();
    void connectSignals();

    void refreshDatabaseList();
    void onTreeViewDoubleClicked(const QModelIndex &index);
    void onItemCheckChanged(QStandardItem *item);
    void selectAllInTable();
    void unselectAllInTable();
    void updateStatLabel();
    void rebuildSelectedRecords();

    void onEditPathClicked();
    void onRefreshInfoClicked();
    void onExportClicked();

    void createFloders(const QString path, const QString nameFloder);

    void createReportMassage(QString filePath, const QString reportText);

    void createReport(const QString path, int windowSize, int windowBias);
    void createCVSFile(const QString path, int typeMotion, const SelectedRecord &record,
                       int windowSize, int windowBias);

    void resetTypeCounter();

    QStandardItem* findSelectedTableItem() const;

    void setupInfo();
    void openDBDialog();

    QSplitter *mainSplitter;

    QHBoxLayout *mainLayout;

    QWidget *leftWidget;
    QWidget *rightWidget;
    QVBoxLayout *leftLayout;
    QVBoxLayout *rightLayout;

// ====================== TREE VIEW +++++++++++++++++++

    QTreeView *treeView;
    QStandardItem *parentItem;
    QStandardItemModel *treeModel;

    QPushButton *updateDB;
    QPushButton *setCustomDB;

    QPushButton *m_btnSelectAllFromParent;
    QPushButton *m_btnUnselectAllFromParent;

    QLabel *statLabel;

    QList<SelectedRecord> m_selectedRecords;

// ====================== EXPORT SETTINGS +++++++++++++++++++++

    QPushButton *m_btnExportPath;
    QPushButton *m_btnExport;
    QPushButton *m_btnRefreshInfo;

    QSplitter *rightSplitter;

    QGroupBox *topWidget;
    QLineEdit *m_windowSize;
    QLineEdit *m_biasWindow;
    QLineEdit *m_exportPath;
    QLineEdit *m_exportFloderName;

    QCheckBox *m_normalize;
    QLineEdit *m_maxAccel;
    QLineEdit *m_maxGyro;

    QTextEdit *m_descriptionBlock;

    QGroupBox *downWidget;

    QTextEdit *m_downInfo;

    DatabaseManager *m_dbManager;
    QString m_currentDatabase;

    QMap<int, int> typeCounters;
};
