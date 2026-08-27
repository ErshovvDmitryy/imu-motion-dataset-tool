#pragma once

#include <QDialog>
#include <QStringList>

class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QTextEdit;
class QLabel;
class QLineEdit;

class CreateDBDialog: public QDialog
{
    Q_OBJECT

public:
    explicit CreateDBDialog(QWidget *parent = nullptr);

    QStringList returnDBdata();

private slots:
    void onOkClicked();
    void onCancelClicked();
    void onEditPathClicked();

private:
    void setupUI();

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_setPathLayout;
    QHBoxLayout *m_buttonsLayout;

    QLabel *m_info1;
    QLabel *m_info2;

    QTextEdit *textBrowser;

    QLineEdit *m_pathEdit;
    QLineEdit *m_fileName;

    QPushButton *m_btnOk;
    QPushButton *m_btnCancel;
    QPushButton *m_btnSetFilePath;

};
