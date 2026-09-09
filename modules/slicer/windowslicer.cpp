#include "windowslicer.h"

WindowSlicer::WindowSlicer()
    : m_windowSize(10)
    , m_bias(5)
    , m_minRemainingRatio(0.1f)
    , m_maxOffset(0)
    , m_minOffset(0)
    , m_startFlag(0)
    , m_endFlag(0)
{
}

void WindowSlicer::setWindowSize(int size) { m_windowSize = size; }

void WindowSlicer::setBias(int bias) { m_bias = bias; }

void WindowSlicer::setMinRemainingRatio(float ratio) { m_minRemainingRatio = ratio; }

int WindowSlicer::windowSize() const { return m_windowSize; }

int WindowSlicer::bias() const { return m_bias; }

float WindowSlicer::minRemainingRatio() const { return m_minRemainingRatio; }

void WindowSlicer::setMaxOffset(int offset) { m_maxOffset = offset; }
void WindowSlicer::setMinOffset(int offset) { m_minOffset = offset; }
void WindowSlicer::setStartFlag(int offset) { m_startFlag = offset; }
void WindowSlicer::setEndFlag(int offset) { m_endFlag = offset; }

int WindowSlicer::maxOffset() const { return m_maxOffset; }
int WindowSlicer::minOffset() const { return m_minOffset; }
int WindowSlicer::startFlag() const { return m_startFlag; }
int WindowSlicer::endFlag() const { return m_endFlag; }


QVector<QVector<QVector<float>>> WindowSlicer::slice(const QVector<QVector<float>> &data) {

    QVector<QVector<QVector<float>>> windows;

    int totalSamples = data.size();
    if (totalSamples < m_windowSize) {
        return windows;
    }

    int effectiveEnd = m_endFlag;
    if (effectiveEnd <= 0 || effectiveEnd > totalSamples) {
        effectiveEnd = totalSamples;
    }

    int effectiveStart = m_startFlag;
    if (effectiveStart < 0) {
        effectiveStart = 0;
    }

    if (effectiveStart + m_windowSize > effectiveEnd) {
        return windows;
    }

    int maxStart = effectiveEnd - m_windowSize;
    int currentStart = effectiveStart;

    if (m_maxOffset + m_minOffset > 0) {
        int jitter = QRandomGenerator::global()->bounded(-m_minOffset, m_maxOffset + 1);
        currentStart = effectiveStart + jitter;
    }

    currentStart = qBound(0, currentStart, maxStart);

    while (true) {
        windows.append(data.mid(currentStart, m_windowSize));

        if (currentStart >= maxStart) {
            break;
        }

        int nextBase = currentStart + m_bias;

        if (m_maxOffset + m_minOffset > 0) {
            int jitter = QRandomGenerator::global()->bounded(-m_minOffset, m_maxOffset + 1);
            nextBase += jitter;
        }

        if (nextBase <= currentStart) {
            nextBase = currentStart + 1;
        }

        currentStart = qBound(0, nextBase, maxStart);
    }

    return windows;
}
