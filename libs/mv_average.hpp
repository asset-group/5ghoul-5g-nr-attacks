#pragma once

#ifndef __MV_AVERAGE__
#define __MV_AVERAGE__

// https://github.com/arvidn/moving_average

template <int inverted_gain>
struct mv_average
{
	mv_average() : m_mean(0), m_average_deviation(0), m_num_samples(0) {}

	void add_sample(int s)
	{
		// fixed point
		s *= 64;
		long int deviation;

		if (m_num_samples > 0)
			deviation = abs(m_mean - s);

		if (m_num_samples < inverted_gain)
			++m_num_samples;

		m_mean += (s - m_mean) / m_num_samples;

		if (m_num_samples > 1)
		{
			// the the exact same thing for deviation off the mean except -1 on
			// the samples, because the number of deviation samples always lags
			// behind by 1 (you need to actual samples to have a single deviation
			// sample).
			m_average_deviation += (deviation - m_average_deviation) / (m_num_samples - 1);
		}
	}

	long int mean() const { return m_num_samples > 0 ? (m_mean + 32) / 64 : 0; }
	long int avg_deviation() const { return m_num_samples > 1 ? (m_average_deviation + 32) / 64 : 0; }

private:
	// both of these are fixed point values (* 64)
	long int m_mean;
	long int m_average_deviation;
	// the number of samples we have received, but no more than inverted_gain
	// this is the effective inverted_gain
	long int m_num_samples;
};

#endif
