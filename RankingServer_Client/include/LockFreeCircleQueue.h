#pragma once

#include<iostream>
#include<atomic>

namespace Utility
{
	template<typename T>
	class LockFreeCircleQueue
	{
	private:
		int queueMaxSize;// ���������δ� queueMaxSize-1��ŭ �������ִ�. ����ť�� �ϳ��� ������ ��������ν� full�� empty���¸� �����ϱ� ����.
		std::atomic<int> inputIndex;
		std::atomic<int> outputIndex;
		T* buffer;
	public:
		LockFreeCircleQueue<T>()
		{
			inputIndex = 0;
			outputIndex = 0;
		}

		~LockFreeCircleQueue<T>()
		{
			delete[] buffer;
		}

		void Construct(int queueMaxSize)
		{
			this->queueMaxSize = queueMaxSize;
			buffer = new T[queueMaxSize];
		}

		//���⼭ &&�� rvalue reference�� �ǹ��Ѵ�.
		// rvalue reference�� �ӽ� ��ü�� �����ϴµ� ���Ǹ�, std::move()�� �Բ� ����Ͽ� ��ü�� �������� �̵��� �������� �Ҷ� ���ȴ�.
		//����ȭ �̵��� ���� �߿��� �������Ѵ�.
		bool push(T&& data)
		{
			int currentInputIndex = inputIndex.load(std::memory_order_acquire);
			int nextIndex = (currentInputIndex + 1) % queueMaxSize;

			if (nextIndex == outputIndex.load(std::memory_order_acquire))
			{
				std::cout << "Queue is full" << std::endl;
				return false;
			}

			buffer[currentInputIndex] = std::move(data);
			inputIndex.store(nextIndex, std::memory_order_release);// ������Ʈ �� release
			return true;
		}

		T pop()
		{
			int currentOutputIndex = outputIndex.load(std::memory_order_acquire); // �ֽ� outputIndex�� �����ͼ� ���Ѵ�.

			if (currentOutputIndex == inputIndex.load(std::memory_order_acquire))//�����͸� �д� load �۾����� ����Ͽ� ���� ����ȭ.
			{
				std::cout << "Queue is empty" << std::endl;
				return T();
			}

			T data = std::move(buffer[currentOutputIndex]);
			outputIndex.store((currentOutputIndex + 1) % queueMaxSize, std::memory_order_release);// �����͸� ������ ����� �� �ε����� ������Ʈ�� �� ���
			return data;
		}

		bool empty()
		{
			return inputIndex.load(std::memory_order_acquire) == outputIndex.load(std::memory_order_acquire);
		}

		//TODO Atomic	//size()�� capacity()�� atomic�� ������� ����. �ܼ��� �ε��� ���̷� ����ϱ� ����. ???
		int size()
		{
			auto input = inputIndex.load(std::memory_order_acquire);
			auto output = outputIndex.load(std::memory_order_acquire);

			if (input >= output)
				return input - output;
			else
				return queueMaxSize - outputIndex + input;
		}

		int capacity()
		{
			return queueMaxSize;
		}

		void clear()
		{
			inputIndex.store(0, std::memory_order_release);
			outputIndex.store(0, std::memory_order_release);
		}

		void print()
		{
			std::cout << "Input Index: " << inputIndex << ", Output Index: " << outputIndex << std::endl;
			for (int i = 0; i < size(); i++)
			{
				std::cout << buffer[(outputIndex + i) % queueMaxSize] << " ";
			}
			std::cout << std::endl;
		}

		T Front()
		{
			return buffer[outputIndex.load(std::memory_order_acquire)];
		}
	};
}
