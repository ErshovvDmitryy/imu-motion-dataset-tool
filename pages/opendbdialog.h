#pragma once

#include <QDialog>
#include <QStringList>

class QListWidget;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;

class OpenDBDialog: public QDialog
{
    Q_OBJECT

public:
    explicit OpenDBDialog(const QStringList &registeredDbs,
                                QWidget *parent = nullptr);

    QString getSelectedDatabase() const;

    bool isCustomPath() const { return m_isCustomPath; }

private slots:
    void onSelectFromList();
    void onSelectCustom();
    void onItemDoubleClicked();
    void onOkClicked();
    void onCancelClicked();

private:
    void setupUI();
    void loadRegisteredDatabases(const QStringList &dbs);

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_topLayout;
    QHBoxLayout *m_buttonLayout;

    QLabel *m_titleLabel;
    QListWidget *m_dbList;
    QLineEdit *m_pathEdit;

    QPushButton *m_refreshBtn;
    QPushButton *m_customBtn;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;

    QStringList m_registeredDbs;
    QString m_selectedDatabase;
    bool m_isCustomPath = false;

};
