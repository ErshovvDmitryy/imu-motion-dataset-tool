#pragma once

#include <QWidget>
#include <QVector>

#include "pages/createdbdialog.h"
#include "pages/opendbdialog.h"

class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QTreeView;
class QTableView;
class QPushButton;
class QStandardItemModel;
class DatabaseManager;
class QStandardItem;
class QSplitter;
class QLineEdit;
class QSqlTableModel;
class QCustomPlot;

class DataBasePage : public QWidget
{
    Q_OBJECT
public:
    explicit DataBasePage(DatabaseManager *dbManager, QWidget *parent = nullptr);

private:
    void createWidgets();
    void createLayouts();
    void connectSignals();

    void updateTreeView();

    void refreshDatabaseList();
    void openDBDialog();

    void refreshSamplesModel(const QString &dbName, const QString &selectTable);

    void createDBDialog();

    QWidget *leftWidget;
    QWidget *rightWidget;

    QSplitter *mainSplitter;

    QHBoxLayout *mainLayout;
    QVBoxLayout *leftLayout;
    QVBoxLayout *rigthLayout;

// =================== LEFT SIDE

    QHBoxLayout *graphLayout;

    QPushButton *btnOpenDB;
    QPushButton *btnDeleteDB;
    QPushButton *btnEditDB;
    QPushButton *btnCreateDB;
    QPushButton *btnRefreshDB;


    QTreeView *treeView;
    QStandardItem *parentItem;
    QStandardItemModel *treeModel;

// =================== RIGHT SIDE

    QHBoxLayout *pathLayout;
    QHBoxLayout *sampleDo;
    QVBoxLayout *pathAreaLeft;
    QVBoxLayout *pathAreaRight;
    QHBoxLayout *pathArea;

    QHBoxLayout *areaBtnGraph;

    QHBoxLayout *samplesArea;
    QVBoxLayout *samplesAreaRight;

    QCustomPlot *gyroGraph;
    QCustomPlot *accelGraph;

    QLabel *labelStatsChanged;

    QPushButton *btnRefreshGraph;
    QPushButton *btnCutGraph;
    QPushButton *btnExportToDataSet;
    QPushButton *btnExportAllToDataSet;
    QPushButton *btnDeleteFromDataSet;
    QPushButton *btnSetPath;
    QPushButton *btnUndoSample;
    QPushButton *btnNextSample;

    QTableView *samplesView;
    QSqlTableModel *samplesModel;

    QTableView *motionDataView;
    QSqlTableModel *motionDataModel;

    QLineEdit *m_pathEdit;

    QVector<int> changedList;
    bool fileIsChanged = false;

    DatabaseManager *m_dbManager;
    QString m_currentDatabase;

    void onTreeViewDoubleClicked(const QModelIndex &index);

signals:

    void createBD();
};
