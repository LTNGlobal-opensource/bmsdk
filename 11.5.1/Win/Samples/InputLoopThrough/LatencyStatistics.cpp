/* -LICENSE-START-
 ** Copyright (c) 2019 Blackmagic Design
 **
 ** Permission is hereby granted, free of charge, to any person or organization
 ** obtaining a copy of the software and accompanying documentation covered by
 ** this license (the "Software") to use, reproduce, display, distribute,
 ** execute, and transmit the Software, and to prepare derivative works of the
 ** Software, and to permit third-parties to whom the Software is furnished to
 ** do so, all subject to the following:
 **
 ** The copyright notices in the Software and this entire statement, including
 ** the above license grant, this restriction and the following disclaimer,
 ** must be included in all copies of the Software, in whole or in part, and
 ** all derivative works of the Software, unless such copies or derivative
 ** works are solely in the form of machine-executable object code generated by
 ** a source language processor.
 **
 ** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 ** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 ** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 ** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 ** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 ** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 ** DEALINGS IN THE SOFTWARE.
 ** -LICENSE-END-
 */

#include <algorithm>
#include <cmath>
#include <limits>
#include "LatencyStatistics.h"


LatencyStatistics::LatencyStatistics(int maxRollingSamples) :
	m_maxRollingSamples(maxRollingSamples)
{
	if (m_maxRollingSamples < 1)
		throw std::invalid_argument("Unexpected value for rolling average size");
	
	reset();
}

void LatencyStatistics::reset()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	
	m_rollingSamples.clear();
	m_rollingAggregateLatency = 0;
	
	m_maxLatency	= (std::numeric_limits<BMDTimeValue>::min)();
	m_minLatency	= (std::numeric_limits<BMDTimeValue>::max)();
	
	m_mean			= 0;
	m_m2			= 0;
	m_sampleCount	= 0;
}

void LatencyStatistics::addSample(const BMDTimeValue latency)
{
	BMDTimeValue delta;
	BMDTimeValue delta2;
	
	std::lock_guard<std::mutex> lock(m_mutex);
	
	// Add to rolling average list
	if (m_rollingSamples.size() >= (size_t)m_maxRollingSamples)
	{
		m_rollingAggregateLatency -= m_rollingSamples.front();
		m_rollingSamples.pop_front();
	}
	
	m_rollingAggregateLatency += latency;
	m_rollingSamples.push_back(latency);
	
	// Check minimum and maximum latency
	m_maxLatency = std::max(m_maxLatency, latency);
	m_minLatency = std::min(m_minLatency, latency);
	
	// Compute new mean/variance (Welford's algorithm)
	delta = latency - m_mean;
	m_mean += delta / ++m_sampleCount;
	delta2 = latency - m_mean;
	m_m2 += delta * delta2;
}

BMDTimeValue LatencyStatistics::getMinimum()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_minLatency;
}

BMDTimeValue LatencyStatistics::getMaximum()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_maxLatency;
}

std::pair<BMDTimeValue,BMDTimeValue> LatencyStatistics::getMeanAndStdDev()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_sampleCount < 2)
		return std::make_pair((BMDTimeValue)0, (BMDTimeValue)0);
	
	return std::make_pair(m_mean, (BMDTimeValue)std::sqrt(m_m2 / (m_sampleCount - 1)));
}

BMDTimeValue LatencyStatistics::getRollingAverage()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_rollingSamples.empty())
		return (BMDTimeValue)0;

	return m_rollingAggregateLatency / m_rollingSamples.size();
}