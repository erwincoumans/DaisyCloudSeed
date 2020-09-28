
//b3ReadWavFile is implemented based on code from the STK toolkit
//See https://github.com/thestk/stk
//Some improvement: the ticking data (b3WavTicker) is separate from wav file,
//This makes it possoble to play a single wav multiple times at the same time

#include "b3ReadWavFile.h"
#include "b3SwapUtils.h"
#include <math.h>

const unsigned long B3_SINT8 = 0x1;
const unsigned long B3_SINT16 = 0x2;
const unsigned long B3_SINT24 = 0x4;
const unsigned long B3_SINT32 = 0x8;
const unsigned long B3_FLOAT32 = 0x10;
const unsigned long B3_FLOAT64 = 0x20;

b3ReadWavFile::b3ReadWavFile()
{
	m_machineIsLittleEndian = 1;// b3MachineIsLittleEndian();
}
b3ReadWavFile::~b3ReadWavFile()
{
}

void b3ReadWavFile::normalize(double peak)
{
	int i;
	double max = 0.0;

	for (i = 0; i < m_frames.size(); i++)
	{
		if (fabs(m_frames[i]) > max)
			max = (double)fabs((double)m_frames[i]);
	}

	if (max > 0.0)
	{
		max = 1.0 / max;
		max *= peak;
		for (i = 0; i < m_frames.size(); i++)
			m_frames[i] *= max;
	}
}

double b3ReadWavFile::interpolate(double frame, unsigned int channel) const
{
	int iIndex = (int)frame;                        // integer part of index
	double output, alpha = frame - (double)iIndex;  // fractional part of index

	iIndex = iIndex * channels_ + channel;
	output = m_frames[iIndex];
	if (alpha > 0.0)
		output += (alpha * (m_frames[iIndex + channels_] - output));

	return output;
}

double b3ReadWavFile::tick(unsigned int channel, b3WavTicker *ticker)
{
	if (ticker->finished_) return 0.0;

	if (ticker->time_ < 0.0 || ticker->time_ > (double)(this->m_numFrames - 1.0))
	{
		for (int i = 0; i < ticker->lastFrame_.size(); i++) ticker->lastFrame_[i] = 0.0;
		ticker->finished_ = true;
		return 0.0;
	}

	double tyme = ticker->time_;

	bool interpolate_ = true;  //for now

	if (interpolate_)
	{
		for (int i = 0; i < ticker->lastFrame_.size(); i++)
			ticker->lastFrame_[i] = interpolate(tyme, i);
	}

	// Increment time, which can be negative.
	ticker->time_ += ticker->rate_;
	return ticker->lastFrame_[channel];
}

void b3ReadWavFile::resize()
{
	m_frames.resize(channels_ * m_numFrames);
}

b3WavTicker b3ReadWavFile::createWavTicker(double sampleRate)
{
	b3WavTicker ticker;
	ticker.lastFrame_.resize(this->channels_);
	ticker.time_ = 0;
	ticker.finished_ = false;
	ticker.rate_ = fileDataRate_ / sampleRate;
	return ticker;
}

bool b3ReadWavFile::getWavInfo(b3DataSource& dataSource)
{
	
	char header[12];
	if (dataSource.fread(&header, 4, 3) != 3)
		return false;
	bool res = false;

	if (!strncmp(header, "RIFF", 4) &&
		!strncmp(&header[8], "WAVE", 4))
		res = true;
	//getWavInfo( fileName );

	// Find "format" chunk ... it must come before the "data" chunk.
	char id[4];
	int chunkSize;
	if (dataSource.fread(&id, 4, 1) != 1)
		return false;
	while (strncmp(id, "fmt ", 4))
	{
		if (dataSource.fread(&chunkSize, 4, 1) != 1)
			return false;
		if (!m_machineIsLittleEndian)
		{
			b3Swap32((unsigned char *)&chunkSize);
		}
		if (dataSource.fseek( chunkSize, B3_SEEK_CUR) == -1)
			return false;
		if (dataSource.fread(&id, 4, 1) != 1)
			return false;
	}

	// Check that the data is not compressed.
	unsigned short format_tag;
	if (dataSource.fread(&chunkSize, 4, 1) != 1)
		return false;  // Read fmt chunk size.
	if (dataSource.fread(&format_tag, 2, 1) != 1)
		return false;
	if (!m_machineIsLittleEndian)
	{
		b3Swap16((unsigned char *)&format_tag);
		b3Swap32((unsigned char *)&chunkSize);
	}
	if (format_tag == 0xFFFE)
	{  // WAVE_FORMAT_EXTENSIBLE
		dataOffset_ = dataSource.ftell();
		if (dataSource.fseek( 14, B3_SEEK_CUR) == -1)
			return false;
		unsigned short extSize;
		if (dataSource.fread(&extSize, 2, 1) != 1)
			return false;
		if (!m_machineIsLittleEndian)
		{
			b3Swap16((unsigned char *)&extSize);
		}
		if (extSize == 0)
			return false;
		if (dataSource.fseek( 6, B3_SEEK_CUR) == -1)
			return false;
		if (dataSource.fread(&format_tag, 2, 1) != 1)
			return false;
		if (!m_machineIsLittleEndian)
		{
			b3Swap16((unsigned char *)&format_tag);
		}
		if (dataSource.fseek( dataOffset_, B3_SEEK_SET) == -1)
			return false;
	}
	if (format_tag != 1 && format_tag != 3)
	{  // PCM = 1, FLOAT = 3
		//  oStream_ << "FileRead: "<< fileName << " contains an unsupported data format type (" << format_tag << ").";
		return false;
	}

	// Get number of channels from the header.
	short int temp;
	if (dataSource.fread(&temp, 2, 1) != 1)
		return false;
	if (!m_machineIsLittleEndian)
	{
		b3Swap16((unsigned char *)&temp);
	}
	channels_ = (unsigned int)temp;

	// Get file sample rate from the header.
	int srate;
	if (dataSource.fread(&srate, 4, 1) != 1)
		return false;
	if (!m_machineIsLittleEndian)
	{
		b3Swap32((unsigned char *)&srate);
	}
	fileDataRate_ = (double)srate;

	// Determine the data type.
	dataType_ = 0;
	if (dataSource.fseek( 6, B3_SEEK_CUR) == -1)
		return false;  // Locate bits_per_sample info.
	if (dataSource.fread(&temp, 2, 1) != 1)
		return false;
	if (!m_machineIsLittleEndian)
	{
		b3Swap16((unsigned char *)&temp);
	}
	if (format_tag == 1)
	{
		if (temp == 8)
			dataType_ = B3_SINT8;
		else if (temp == 16)
			dataType_ = B3_SINT16;
		else if (temp == 24)
			dataType_ = B3_SINT24;
		else if (temp == 32)
			dataType_ = B3_SINT32;
	}
	else if (format_tag == 3)
	{
		if (temp == 32)
			dataType_ = B3_FLOAT32;
		else if (temp == 64)
			dataType_ = B3_FLOAT64;
	}
	if (dataType_ == 0)
	{
		//   oStream_ << "FileRead: " << temp << " bits per sample with data format " << format_tag << " are not supported (" << fileName << ").";
		return false;
	}

	// Jump over any remaining part of the "fmt" chunk.
	if (dataSource.fseek( chunkSize - 16, B3_SEEK_CUR) == -1)
		return false;

	// Find "data" chunk ... it must come after the "fmt" chunk.
	if (dataSource.fread(&id, 4, 1) != 1)
		return false;

	while (strncmp(id, "data", 4))
	{
		if (dataSource.fread(&chunkSize, 4, 1) != 1)
			return false;
		if (!m_machineIsLittleEndian)
		{
			b3Swap32((unsigned char *)&chunkSize);
		}
		chunkSize += chunkSize % 2;  // chunk sizes must be even
		if (dataSource.fseek( chunkSize, B3_SEEK_CUR) == -1)
			return false;
		if (dataSource.fread(&id, 4, 1) != 1)
			return false;
	}

	// Get length of data from the header.
	int bytes;
	if (dataSource.fread(&bytes, 4, 1) != 1)
		return false;
	if (!m_machineIsLittleEndian)
	{
		b3Swap32((unsigned char *)&bytes);
	}
	m_numFrames = bytes / temp / channels_;  // sample frames
	m_numFrames *= 8;                        // sample frames

	dataOffset_ = dataSource.ftell();
	byteswap_ = false;
	if (!m_machineIsLittleEndian)
	{
		byteswap_ = true;
	}
	wavFile_ = true;
	return true;
}

bool b3ReadWavFile::read(b3DataSource& dataSource, unsigned long startFrame, bool doNormalize)
{
	
	// Check the m_frames size.
	unsigned long nFrames = this->m_numFrames;  //m_frames.frames();
	if (nFrames == 0)
	{
		//    oStream_ << "FileRead::read: StkFrames m_frames size is zero ... no data read!";
		//    Stk::handleError( StkError::WARNING );
		return false;
	}

	if (startFrame >= m_numFrames)
	{
		return false;
		//oStream_ << "FileRead::read: startFrame argument is greater than or equal to the file size!";
		//Stk::handleError( StkError::FUNCTION_ARGUMENT );
	}

	// Check for file end.
	if (startFrame + nFrames > m_numFrames)
		nFrames = m_numFrames - startFrame;

	long i, nSamples = (long)(nFrames * channels_);
	unsigned long offset = startFrame * channels_;

	// Read samples into StkFrames data m_frames.
	if (dataType_ == B3_SINT16)
	{
		signed short int *buf = (signed short int *)&m_frames[0];
		if (dataSource.fseek( dataOffset_ + (offset * 2), B3_SEEK_SET) == -1)
			return false;
		if (dataSource.fread(buf, nSamples * 2, 1) != 1)
			return false;
		if (byteswap_)
		{
			signed short int *ptr = buf;
			for (i = nSamples - 1; i >= 0; i--)
				b3Swap16((unsigned char *)ptr++);
		}
		if (doNormalize)
		{
			double gain = 1.0 / 32768.0;
			for (i = nSamples - 1; i >= 0; i--)
				m_frames[i] = buf[i] * gain;
		}
		else
		{
			for (i = nSamples - 1; i >= 0; i--)
				m_frames[i] = buf[i];
		}
	}
	else if (dataType_ == B3_SINT32)
	{
		int *buf = (int *)&m_frames[0];
		if (dataSource.fseek( dataOffset_ + (offset * 4), B3_SEEK_SET) == -1)
			return false;
		if (dataSource.fread(buf, nSamples * 4, 1) != 1)
			return false;
		if (byteswap_)
		{
			int *ptr = buf;
			for (i = nSamples - 1; i >= 0; i--)
				b3Swap32((unsigned char *)ptr++);
		}
		if (doNormalize)
		{
			double gain = 1.0 / 2147483648.0;
			for (i = nSamples - 1; i >= 0; i--)
				m_frames[i] = buf[i] * gain;
		}
		else
		{
			for (i = nSamples - 1; i >= 0; i--)
				m_frames[i] = buf[i];
		}
	}
	else if (dataType_ == B3_FLOAT32)
	{
		float *buf = (float *)&m_frames[0];
		if (dataSource.fseek( dataOffset_ + (offset * 4), B3_SEEK_SET) == -1)
			return false;
		if (dataSource.fread(buf, nSamples * 4, 1) != 1)
			return false;
		if (byteswap_)
		{
			float *ptr = buf;
			for (i = nSamples - 1; i >= 0; i--)
				b3Swap32((unsigned char *)ptr++);
		}
		for (i = nSamples - 1; i >= 0; i--)
			m_frames[i] = buf[i];
	}
	else if (dataType_ == B3_FLOAT64)
	{
		double *buf = (double *)&m_frames[0];
		if (dataSource.fseek( dataOffset_ + (offset * 8), B3_SEEK_SET) == -1)
			return false;
		if (dataSource.fread(buf, nSamples * 8, 1) != 1)
			return false;
		if (byteswap_)
		{
			double *ptr = buf;
			for (i = nSamples - 1; i >= 0; i--)
				b3Swap64((unsigned char *)ptr++);
		}
		for (i = nSamples - 1; i >= 0; i--)
			m_frames[i] = buf[i];
	}
	else if (dataType_ == B3_SINT8 && wavFile_)
	{  // 8-bit WAV data is unsigned!
		unsigned char *buf = (unsigned char *)&m_frames[0];
		if (dataSource.fseek( dataOffset_ + offset, B3_SEEK_SET) == -1)
			return false;
		if (dataSource.fread(buf, nSamples, 1) != 1)
			return false;
		if (doNormalize)
		{
			double gain = 1.0 / 128.0;
			for (i = nSamples - 1; i >= 0; i--)
				m_frames[i] = (buf[i] - 128) * gain;
		}
		else
		{
			for (i = nSamples - 1; i >= 0; i--)
				m_frames[i] = buf[i] - 128.0;
		}
	}
	else if (dataType_ == B3_SINT8)
	{  // signed 8-bit data
		char *buf = (char *)&m_frames[0];
		if (dataSource.fseek( dataOffset_ + offset, B3_SEEK_SET) == -1)
			return false;
		if (dataSource.fread(buf, nSamples, 1) != 1)
			return false;
		if (doNormalize)
		{
			double gain = 1.0 / 128.0;
			for (i = nSamples - 1; i >= 0; i--)
				m_frames[i] = buf[i] * gain;
		}
		else
		{
			for (i = nSamples - 1; i >= 0; i--)
				m_frames[i] = buf[i];
		}
	}
	else if (dataType_ == B3_SINT24)
	{
		// 24-bit values are harder to import efficiently since there is
		// no native 24-bit type.  The following routine works but is much
		// less efficient than that used for the other data types.
		int temp;
		unsigned char *ptr = (unsigned char *)&temp;
		double gain = 1.0 / 2147483648.0;
		if (dataSource.fseek( dataOffset_ + (offset * 3), B3_SEEK_SET) == -1)
			return false;
		for (i = 0; i < nSamples; i++)
		{
			if (m_machineIsLittleEndian)
			{
				if (byteswap_)
				{
					if (dataSource.fread(ptr, 3, 1) != 1)
						return false;
					temp &= 0x00ffffff;
					b3Swap32((unsigned char *)ptr);
				}
				else
				{
					if (dataSource.fread(ptr + 1, 3, 1) != 1)
						return false;
					temp &= 0xffffff00;
				}
			}
			else
			{
				if (byteswap_)
				{
					if (dataSource.fread(ptr + 1, 3, 1) != 1)
						return false;
					temp &= 0xffffff00;
					b3Swap32((unsigned char *)ptr);
				}
				else
				{
					if (dataSource.fread(ptr, 3, 1) != 1)
						return false;
					temp &= 0x00ffffff;
				}
			}

			if (doNormalize)
			{
				m_frames[i] = (double)temp * gain;  // "gain" also  includes 1 / 256 factor.
			}
			else
				m_frames[i] = (double)temp / 256;  // right shift without affecting the sign bit
		}
	}

	// m_frames.setDataRate( fileDataRate_ );

	return true;

	// error:
	//  oStream_ << "FileRead: Error reading file data.";
	//  handleError( StkError::FILE_ERROR);
}
