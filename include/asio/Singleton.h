#pragma once
#include <memory>
#include <iostream>

template <typename T>
class Singleton {
protected:
	Singleton() = default;
	Singleton(const Singleton<T>&) = delete;
	Singleton& operator=(const Singleton<T>& st) = delete;
	~Singleton() {
		std::cout << "this is singleton destruct" << std::endl;
	}
public:
	static std::shared_ptr<T> GetInstance() {
		static std::shared_ptr<T> _instance(new T(),[](T* p){delete p;});
		return _instance;
	}
};