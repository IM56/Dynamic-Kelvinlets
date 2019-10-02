#pragma once

struct SystemsInterface;

template <typename T>
class Manager
{
public:
	Manager() {}
	~Manager() {}

	void init(SystemsInterface&);
	void update(SystemsInterface&);
	void release();

private:

};

template <typename T>
void Manager<T>::init(SystemsInterface& systems)
{
	static_cast<T*>(this)->init(systems);
}

template <typename T>
void Manager<T>::update(SystemsInterface& systems)
{
	static_cast<T*>(this)->update(systems);
}


template <typename T>
void Manager<T>::release()
{
	static_cast<T*>(this)->release();
}