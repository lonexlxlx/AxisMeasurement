#include "lsThread.h"
#include <QDateTime>
#include <QMutexLocker>

lsThread::lsThread(ls_device* lsDevice)
{
	lsPtr = lsDevice;
	sleepTime = 300;
};

lsThread::~lsThread()
{
	requestInterruption();
	wait();
};
void lsThread::run()
{
	if (lsPtr == NULL)
	{
		return;
	};
	{
		QMutexLocker locker(&m_resultMutex);
		for (int i = 0; i < 4; ++i) m_resultValid[i] = false;
	}
	while (!isInterruptionRequested())
	{
		float values[4] = { 0, 0, 0, 0 };
		bool valid[4] = { false, false, false, false };
		for (int i = 0; i < 4; i++)
			valid[i] = lsPtr->tryGetLsMeasurementValue(i + 1, values[i]);
		const qint64 sampledAt = QDateTime::currentMSecsSinceEpoch();
		{
			QMutexLocker locker(&m_resultMutex);
			for (int i = 0; i < 4; ++i) {
				m_resultValid[i] = valid[i];
				if (valid[i]) {
					currentResult[i] = values[i];
					m_resultTimestampMs[i] = sampledAt;
				}
			}
		}
		
		emit updateLsResult();
		msleep(sleepTime);
	};
};

bool lsThread::latestResult(int outputIndex, float& value, qint64& sampledAtMs) const
{
	if (outputIndex < 0 || outputIndex >= 4) return false;
	QMutexLocker locker(&m_resultMutex);
	if (!m_resultValid[outputIndex]) return false;
	value = currentResult[outputIndex];
	sampledAtMs = m_resultTimestampMs[outputIndex];
	return true;
}

bool lsThread::latestRoundoutResult(float& out1, float& out2, float& out3,
	qint64& sampledAtMs) const
{
	QMutexLocker locker(&m_resultMutex);
	if (!m_resultValid[0] || !m_resultValid[1] || !m_resultValid[2]
		|| m_resultTimestampMs[0] != m_resultTimestampMs[1]
		|| m_resultTimestampMs[1] != m_resultTimestampMs[2]) return false;
	out1 = currentResult[0];
	out2 = currentResult[1];
	out3 = currentResult[2];
	sampledAtMs = m_resultTimestampMs[0];
	return true;
}

