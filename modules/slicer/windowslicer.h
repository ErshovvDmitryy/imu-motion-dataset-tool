#pragma once

#include <QVector>
#include <QRandomGenerator>

class WindowSlicer {

public:

    WindowSlicer();

    void setWindowSize(int size);
    void setBias(int bias);
    void setMinRemainingRatio(float ratio);

    void setMaxOffset(int offset);
    void setMinOffset(int offset);
    void setStartFlag(int startFlag);
    void setEndFlag(int endFlag);

    int windowSize() const;
    int bias() const;
    float minRemainingRatio() const;

    int maxOffset() const;
    int minOffset() const;
    int startFlag() const;
    int endFlag() const;

    QVector<QVector<QVector<float>>> slice(const QVector<QVector<float>> &data);

private:
    int m_windowSize;
    int m_bias;
    float m_minRemainingRatio;
    int m_maxOffset;
    int m_minOffset;
    int m_startFlag;
    int m_endFlag;

};
