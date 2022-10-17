#include <iostream>
#include <thread>
#include <conio.h>
#include <queue>
#include <mutex>
#include <random>
#include <chrono>
#include <optional>
#include <future>
#include <sstream>
#include <string>
#include <mutex>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif // _WIN32

#pragma region Matrix

class IMatrix
{
public:
	virtual int GetMatrixElement(int i, int j) const = 0;
	virtual void SetMatrixElement(int i, int j, int value) = 0;
	virtual int GetMatrixSize() const = 0;
	virtual ~IMatrix() {};
};

class MatrixReducer :
	public IMatrix
{
	const IMatrix& m_OriginalMatrix;
	int m_excludeColumn;
	int m_excludeRow;

public:

	MatrixReducer(const IMatrix& matrix, int excludeRow, int excludeColumn)
		: m_OriginalMatrix(matrix)
		, m_excludeColumn(excludeColumn)
		, m_excludeRow(excludeRow)
	{
		//TODO validate boundary. 
	}

	int GetMatrixElement(int i, int j) const
	{
		if (i >= m_excludeRow)
		{
			i++;
		}
		if (j >= m_excludeColumn)
		{
			j++;
		}
		return m_OriginalMatrix.GetMatrixElement(i, j);
	}

	void SetMatrixElement(int i, int j, int value)
	{
		//todo - not needed here
		//throw exception
	}

	int GetMatrixSize() const
	{
		return m_OriginalMatrix.GetMatrixSize() - 1;
	}
};

class Matrix : public IMatrix
{
private:
	int** matrixPointer;
	int matrixSize;

	int RecursiveDeterminantCalculation(const IMatrix& inputMatrix) const
	{
		int det = 0;
		int degree = 1;
		//exit conditions
		int currentSize = inputMatrix.GetMatrixSize();
		if (currentSize == 1) {
			return inputMatrix.GetMatrixElement(0, 0);
		}
		else if (currentSize == 2) {
			return inputMatrix.GetMatrixElement(0, 0) * inputMatrix.GetMatrixElement(1, 1) - inputMatrix.GetMatrixElement(0, 1) * inputMatrix.GetMatrixElement(1, 0);
		}
		else
		{
			for (int i = 0; i < currentSize; i++) {
				const IMatrix& newMatrix = MatrixReducer(inputMatrix, i, 0);
				det = det + (degree * inputMatrix.GetMatrixElement(i, 0) * RecursiveDeterminantCalculation(newMatrix));
				degree = -degree;
			}
		}
		return det;
	}


public:
	//Constructors
	Matrix() : matrixSize(0),
		matrixPointer(nullptr)
	{}

	Matrix(int _matrixSize)
		: matrixSize(_matrixSize),
		matrixPointer(nullptr)
	{
		matrixPointer = (int**) new int* [matrixSize];

		for (int i = 0; i < matrixSize; i++)
			matrixPointer[i] = (int*)new int[matrixSize];
		for (int i = 0; i < matrixSize; i++)
			for (int j = 0; j < matrixSize; j++)
				matrixPointer[i][j] = 0;
	}

	Matrix(int _matrixSize, int _maxGenerationValue)
		: matrixSize(_matrixSize),
		matrixPointer(nullptr)
	{

		matrixPointer = (int**) new int* [matrixSize];

		for (int i = 0; i < matrixSize; i++)
			matrixPointer[i] = new int[matrixSize];

		for (int i = 0; i < matrixSize; i++)
			for (int j = 0; j < matrixSize; j++)
				matrixPointer[i][j] = rand() % _maxGenerationValue;
	}

	//Copy constructor
	Matrix(const Matrix& _matrixPointer)
	{
		//*this <= _matrix
		matrixSize = _matrixPointer.matrixSize;
		matrixPointer = new int* [matrixSize];

		for (int i = 0; i < matrixSize; i++)
			matrixPointer[i] = new int[matrixSize];

		for (int i = 0; i < matrixSize; i++)
			for (int j = 0; j < matrixSize; j++)
				matrixPointer[i][j] = _matrixPointer.matrixPointer[i][j];
	}

	//Copy operator
	Matrix& operator=(const Matrix& _matrixPointer)
	{
		if (matrixSize > 0)
		{
			for (int i = 0; i < matrixSize; i++)
				delete[] matrixPointer[i];
			delete[] matrixPointer;
		}

		matrixSize = _matrixPointer.matrixSize;
		matrixPointer = new int* [matrixSize];
		for (int i = 0; i < matrixSize; i++)
			matrixPointer[i] = new int[matrixSize];

		for (int i = 0; i < matrixSize; i++)
			for (int j = 0; j < matrixSize; j++)
				matrixPointer[i][j] = _matrixPointer.matrixPointer[i][j];
		return *this;
	}

	//Destructor
	~Matrix()
	{
		if (matrixPointer != NULL)
		{
			if (matrixSize > 0)
			{
				for (int i = 0; i < matrixSize; i++)
					delete[] matrixPointer[i];
				delete[] matrixPointer;
			}
		}
	}


	//Miscellaneous methods
	//TODO boundary check
	int GetMatrixElement(int i, int j) const
	{
		if (matrixSize > 0)
		{
			if (!(i >= matrixSize) && !(j >= matrixSize))
			{
				return matrixPointer[i][j];
			}
			else
			{
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}

	void SetMatrixElement(int i, int j, int value)
	{
		if ((i < 0) || (i >= matrixSize))
			return;
		if ((j < 0) || (j >= matrixSize))
			return;
		matrixPointer[i][j] = value;
	}

	int GetMatrixSize() const
	{
		return matrixSize;
	}

	void PrintMatrix() const
	{
		for (int i = 0; i < matrixSize; i++)
		{
			for (int j = 0; j < matrixSize; j++)
			{
				std::cout << matrixPointer[i][j] << " ";
			}
			std::cout << std::endl;
		}
	}

	int GetDeterminate() const
	{
		return RecursiveDeterminantCalculation(*this);
	}

};

#pragma endregion

#pragma region Data Consumer

class Thread
{
private:
	HANDLE m_Thread;
	static DWORD ThreadProc(void* object)
	{
		Thread* thread = (Thread*)object;

		return thread->ThreadLoop();
	}


protected:
	bool m_Stopped;
	virtual DWORD ThreadLoop() = 0;

public:
	Thread()
		: m_Stopped(false)
	{
		//TODO ERROR HANDLING
		//This event will be set as soon as something is pushed to the queue 
		DWORD thereadId(0);
		m_Thread = CreateThread(NULL, 0, Thread::ThreadProc, this, 0, &thereadId);
	};

	virtual ~Thread()
	{
		CloseHandle(m_Thread);
	};

	virtual void Stop()
	{
		m_Stopped = true;
	}
};

template <typename T>
class DataConsumer :public Thread
{
private:
	std::queue<T> m_Queue;
	HANDLE m_Event;
	CRITICAL_SECTION m_Lock;
protected:

	virtual DWORD ThreadLoop()
	{
		while (!m_Stopped)
		{
			WaitForSingleObject(m_Event, INFINITE);

			if (m_Stopped)
			{
				break;
			}
#pragma warning( push )
#pragma error( disable : 4703)
			T item = {};

			if (TryEnterCriticalSection)
			{
				if (m_Queue.size() > 0)
				{
					item = m_Queue.front();
					m_Queue.pop();
				}
				else
				{
					ResetEvent(m_Event);
					continue;
				}
			}
			if (!TryEnterCriticalSection)
				//finally - win only... TODO - #ifdef
			{
				LeaveCriticalSection(&m_Lock);
			}

			ProcessItem(item);
#pragma warning( pop )
		}
		return 0;

	}

public:
	DataConsumer()
	{
		//TODO ERROR HANDLING
		//This event will be set as soon as something is pushed to the queue 
		m_Event = CreateEvent(NULL, true, false, NULL);
		InitializeCriticalSection(&m_Lock);
	};

	virtual ~DataConsumer()
	{
		CloseHandle(m_Event);
		DeleteCriticalSection(&m_Lock);
	};


	void Enqueue(T item)
	{
		//try+finally not supported for mingw...
		if (TryEnterCriticalSection)
		{
			m_Queue.push(item);
			SetEvent(m_Event);
		}
		//finally - win only... TODO - #ifdef
		if (!TryEnterCriticalSection)
		{
			LeaveCriticalSection(&m_Lock);
		}
	}

	virtual void ProcessItem(T item) = 0;
};

#pragma endregion

#pragma region thread classes

class MatchesDeterminant
{
	int _DETERMINANT;

public:
	MatchesDeterminant(const int det) : _DETERMINANT(det)
	{}

	bool operator()(std::pair<IMatrix*, int> item) const
	{
		return item.second == _DETERMINANT;
	}

	~MatchesDeterminant()
	{
		_DETERMINANT = NULL;
	}
};

bool sorterHelper(std::pair<IMatrix*, int> const& lhs, std::pair<IMatrix*, int> const& rhs)
{
	if (lhs.second < rhs.second)
		return 1;
	else
		return 0;
}

class TopDeterminantSorter : public DataConsumer<std::pair<IMatrix*, int>>
{
private:
	std::vector<std::pair<IMatrix*, int>> m_topDeterminators;
	HANDLE m_Event;
	std::string outputString;
	void sortArray()
	{
		outputString = "";
		std::sort(m_topDeterminators.begin(), m_topDeterminators.end(), &sorterHelper);
		outputString += "Sorted determinators: ";
		for (int i = 0; i < m_topDeterminators.size(); i++)
		{
			outputString += std::to_string(m_topDeterminators[i].second);
			if (i < m_topDeterminators.size() - 1)
			{
				outputString += ",";
			}
		}
		SetEvent(m_Event);
	}

protected:
	virtual void ProcessItem(std::pair<IMatrix*, int> item)
	{
		if (m_topDeterminators.size() < 10)
		{
			m_topDeterminators.push_back(item);
		}
		else
		{
			bool present = false;
			for (int i = 0; i < m_topDeterminators.size(); i++)
			{
				//item is not in the top 10 list
				if (std::find_if(m_topDeterminators.begin(), m_topDeterminators.end(), MatchesDeterminant(item.second)) == m_topDeterminators.end())
				{
					present = false;
					if (item.second > m_topDeterminators[i].second)
					{
						m_topDeterminators[i].first = item.first;
						m_topDeterminators[i].second = item.second;
						sortArray();
					}
				}
				else
				{
					present = true;
				}
			}
			if (present)
			{
				if (item.first)
				{
					delete item.first;
				}
			}
		}
		/*TODO - avoid memory leak
		if (item.first)
		{
			delete item.first;
		}*/
	}
public:
	TopDeterminantSorter(HANDLE onChangeEvent)
		:m_Event(onChangeEvent)
	{}

	std::string GetTopDeterminators() const
	{
		return outputString;
	}

};

class DeterminateCalculator : public DataConsumer<Matrix*>
{
	TopDeterminantSorter& m_Sorter;
protected:

	virtual void ProcessItem(Matrix* item)
	{
		//TODO calculate determinator and post pair ot sorter
		// std::cout << item->GetDeterminate() << "\n";
		m_Sorter.Enqueue(std::pair<IMatrix*, int>(item, item->GetDeterminate()));
	}
public:
	DeterminateCalculator(TopDeterminantSorter& sorter)
		:m_Sorter(sorter)
	{}

};

class MatrixGenerator : public Thread
{
	DeterminateCalculator& m_Calculator;
public:
	MatrixGenerator(DeterminateCalculator& calculator)
		: m_Calculator(calculator)
	{

	}

protected:
	virtual DWORD ThreadLoop()
	{
		srand((unsigned int)time(NULL));
		while (!m_Stopped)
		{
			Matrix* matrix = new Matrix(8, 10);
			//matrix->PrintMatrix();
			m_Calculator.Enqueue(matrix);
			Sleep(10);
		}
		return 0;
	}
};

#pragma endregion

int main()
{


	HANDLE OnChangeEnvent = CreateEvent(NULL, false, false, NULL);

	TopDeterminantSorter Sorter(OnChangeEnvent);
	DeterminateCalculator calculator(Sorter);
	MatrixGenerator generator(calculator);

	while (!_kbhit())
	{
		WaitForSingleObject(OnChangeEnvent, INFINITE);
		std::cout << Sorter.GetTopDeterminators() << std::endl;
	}
	generator.Stop();
	calculator.Stop();
	Sorter.Stop();

	return 0;
}

