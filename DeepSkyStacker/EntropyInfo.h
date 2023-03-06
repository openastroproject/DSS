#ifndef __ENTROPYINFO_H__
#define __ENTROPYINFO_H__
#include "ColorRef.h"

namespace DSS { class ProgressBase; }

/* ------------------------------------------------------------------- */

class CEntropySquare
{
public :
	QPointF						m_ptCenter;
	double						m_fRedEntropy;
	double						m_fGreenEntropy;
	double						m_fBlueEntropy;

private :
	void	CopyFrom(const CEntropySquare & es)
	{
		m_ptCenter		= es.m_ptCenter;
		m_fRedEntropy	= es.m_fRedEntropy;
		m_fGreenEntropy = es.m_fGreenEntropy;
		m_fBlueEntropy  = es.m_fBlueEntropy;
	};

public :
	CEntropySquare()
	{
        m_fRedEntropy = 0;
        m_fGreenEntropy = 0;
        m_fBlueEntropy = 0;
	};

	CEntropySquare(const QPointF & pt, double fRedEntropy, double fGreenEntropy, double fBlueEntropy)
	{
		m_ptCenter = pt;
		m_fRedEntropy	= fRedEntropy;
		m_fGreenEntropy = fGreenEntropy;
		m_fBlueEntropy	= fBlueEntropy;
	};

	CEntropySquare(const CEntropySquare & es)
	{
		CopyFrom(es);
	};

	virtual ~CEntropySquare()
	{
	};

	const CEntropySquare & operator = (const CEntropySquare & es)
	{
		CopyFrom(es);
		return (*this);
	};
};

class CMemoryBitmap;
class CEntropyInfo
{
private:
	std::shared_ptr<CMemoryBitmap> m_pBitmap;
	int m_lWindowSize;
	int m_lNrPixels;
	int m_lNrSquaresX;
	int m_lNrSquaresY;
	std::vector<float> m_vRedEntropies;
	std::vector<float> m_vGreenEntropies;
	std::vector<float> m_vBlueEntropies;
	DSS::ProgressBase* m_pProgress;

private:
	void InitSquareEntropies();
	void ComputeEntropies(int lMinX, int lMinY, int lMaxX, int lMaxY, double & fRedEntropy, double & fGreenEntropy, double & fBlueEntropy);
	void GetSquareCenter(int lX, int lY, QPointF & ptCenter)
	{
		ptCenter.rx() = lX * (m_lWindowSize * 2 + 1) + m_lWindowSize;
		ptCenter.ry() = lY * (m_lWindowSize * 2 + 1) + m_lWindowSize;
	}

	void AddSquare(CEntropySquare& Square, int lX, int lY)
	{
		GetSquareCenter(lX, lY, Square.m_ptCenter);
		Square.m_fRedEntropy	= m_vRedEntropies[lX + lY * m_lNrSquaresX];
		Square.m_fGreenEntropy	= m_vGreenEntropies[lX + lY * m_lNrSquaresX];
		Square.m_fBlueEntropy	= m_vBlueEntropies[lX + lY * m_lNrSquaresX];
	}

public:
    CEntropyInfo() :
		m_pProgress{ nullptr },
		m_lWindowSize{ 0 },
		m_lNrPixels{ 0 },
		m_lNrSquaresX{ 0 },
		m_lNrSquaresY{ 0 }
	{}

	virtual ~CEntropyInfo()
	{}

	const float* redEntropyData() const { return m_vRedEntropies.data(); }
	const float* greenEntropyData() const { return m_vGreenEntropies.data(); }
	const float* blueEntropyData() const { return m_vBlueEntropies.data(); }
	const int nrSquaresX() const { return m_lNrSquaresX; }
	const int nrSquaresY() const { return m_lNrSquaresY; }
	const int windowSize() const { return m_lWindowSize; }

	void Init(std::shared_ptr<CMemoryBitmap> pBitmap, int lWindowSize = 10, DSS::ProgressBase* pProgress = nullptr);
	void GetPixel(int x, int y, double& fRedEntropy, double& fGreenEntropy, double& fBlueEntropy, COLORREF16& crResult);
};

#endif
