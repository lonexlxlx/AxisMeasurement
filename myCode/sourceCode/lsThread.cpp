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
	terminate();
	if (lsPtr != NULL)
	{
		delete lsPtr;
	};
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
		for (int i = 0; i < 4; i++)
		{
			float value = 0;
			const bool valid = lsPtr->tryGetLsMeasurementValue(i + 1, value);
			QMutexLocker locker(&m_resultMutex);
			m_resultValid[i] = valid;
			if (valid) {
				currentResult[i] = value;
				m_resultTimestampMs[i] = QDateTime::currentMSecsSinceEpoch();
			}
		};
		
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

