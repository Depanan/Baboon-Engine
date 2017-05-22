#pragma once


class Image
{
public:

	void Init(const char* i_sPath);
	const unsigned char* GetData() { return m_Pixels; }

	const int GetWidht() { return m_iWidth; }
	const int GetHeight() { return m_iHeight; }

private:
	unsigned char* m_Pixels;
	int m_iWidth;
	int m_iHeight;
};
