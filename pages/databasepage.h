#pragma once

#include <QWidget>
#include <QVector>
#include <QMouseEvent>

#include "pages/createdbdialog.h"
#include "pages/opendbdialog.h"
#include "models/MotionType.h"

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
class QCPItemStraightLine;
class QSqlQuery;
class QItemSelectionModel;
class QItemSelection;

class DataBasePage : public QWidget
{
    Q_OBJECT
public:
    explicit DataBasePage(DatabaseManager *dbManager, QWidget *parent = nullptr);

private:

    void createWidgets();
    void createLayouts();
    void connectSignals();

    void setupGraph();
    void setupButtonsOnPage();
    void setupsTableView();

    void updateTreeView();

    void refreshDatabaseList();
    void openDBDialog();

    void refreshSamplesModel(const QString &dbName, const QString &);
    void connectSampleSelectionHandler();
    void onSamplesSelectionChanged(const QItemSelection &selected);

    void createDBDialog();

    void loadSampleToGraphs(int sampleId);
    void navigateSample(int delta);
    void updateNavigationButtons();

    void onTrimButtonClicked();
    void onTrimAccept();
    void onTrimDeny();
    void onGraphMousePress(QMouseEvent *event, QCustomPlot *plot);
    void onGraphMouseMove(QMouseEvent *event, QCustomPlot *plot);
    void onGraphMouseRelease(QMouseEvent *event, QCustomPlot *plot);
    void addTrimLine(double sec, bool isStart);
    void updateTrimLine(double sec, bool isStart);
    void clearTrimSeparators();
    void resetTrim();
    void setTrimInteractionEnabled(bool on);
    double clampToData(double sec, int sampleId) const;
    void applyTrimToDatabase(int sampleId, double loSec, double hiSec);

    void onAddSelectedClicked();
    void onExtractSelectedClicked();
    void onDeleteFromDataSetClicked();
    void updateSelectedHighlight();
    void updateStatsLabel();
    QString motionTypeToString(int motionTypeId) const;
    void onTreeViewDoubleClicked(const QModelIndex &index);
    void updateMotionTypeLegend(int highlightId = -1);

    QWidget *leftWidget;
    QWidget *rightWidget;

    QSplitter *mainSplitter;

    QHBoxLayout *mainLayout;
    QVBoxLayout *leftLayout;
    QVBoxLayout *rightLayout;

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
    QHBoxLayout *exportLayout;
    QVBoxLayout *pathAreaLeft;
    QVBoxLayout *pathAreaRight;
    QHBoxLayout *pathArea;

    QHBoxLayout *areaBtnGraph;

    QCustomPlot *gyroGraph;
    QCustomPlot *accelGraph;

    QLabel *labelStatsChanged;
    QLabel *motionTypeLegend;

    QPushButton *btnRefreshGraph;
    QPushButton *btnCutGraph;
    QPushButton *btnExportToDataSet;
    QPushButton *btnExportAllToDataSet;
    QPushButton *btnDeleteFromDataSet;
    QPushButton *btnSetPath;
    QPushButton *btnUndoSample;
    QPushButton *btnNextSample;
    QPushButton *btnAddSelected;
    QPushButton *btnExtractSelected;
    QPushButton *btnTrimAccept;
    QPushButton *btnTrimDeny;

    QTableView *samplesView;
    QSqlTableModel *samplesModel;

    QTableView *motionDataView;
    QSqlTableModel *motionDataModel;

    QVector<int> changedList;
    bool fileIsChanged = false;

    QVector<int> m_selectedList;
    int m_currentSampleRow = -1;

    enum class TrimState { Off, AwaitStart, AwaitEnd, Adjust };
    TrimState m_trimState = TrimState::Off;
    double m_trimStartSec = -1;
    double m_trimEndSec = -1;
    QVector<QCPItemStraightLine *> m_trimLinesStart;
    QVector<QCPItemStraightLine *> m_trimLinesEnd;
    bool m_trimDragging = false;
    bool m_trimDragIsStart = false;
    bool m_rangeDragAccel = false;
    bool m_rangeDragGyro = false;
    static constexpr int TrimDragThresholdPx = 6;

    QLineEdit *m_pathEdit;

    DatabaseManager *m_dbManager;
    QString m_currentDatabase;

signals:

    void createBD();
};
