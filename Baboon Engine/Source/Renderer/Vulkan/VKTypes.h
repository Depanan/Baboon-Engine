#pragma once
#include <functional>
template <typename T>
class VKHandleWrapper {

public:
	VKHandleWrapper() : VKHandleWrapper([](T, VkAllocationCallbacks*) {}) {}

	VKHandleWrapper(std::function<void(T, VkAllocationCallbacks*)> deletef) {
		m_DeleterFunction = [=](T handle) { deletef(handle, nullptr); };
	}

	VKHandleWrapper(const VKHandleWrapper<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
		m_DeleterFunction = [&instance, deletef](T handle) { deletef(instance, handle, nullptr); };
	}

	VKHandleWrapper(const VKHandleWrapper<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
		m_DeleterFunction = [&device, deletef](T handle) { deletef(device, handle, nullptr); };
	}

	~VKHandleWrapper() {
		cleanup();
	}

	const T* operator &() const {
		return &m_VKHandle;
	}

	T* replace() {
		cleanup();
		return &m_VKHandle;
	}

	operator T() const {
		return m_VKHandle;
	}

	void operator=(T handle) {
		if (handle != m_VKHandle) {
			cleanup();
			m_VKHandle = handle;
		}
	}

	template<typename V>
	bool operator==(V handle) {
		return m_VKHandle == T(handle);
	}

private:
	T m_VKHandle{ VK_NULL_HANDLE };
	std::function<void(T)> m_DeleterFunction;

	void cleanup() {
		if (m_VKHandle != VK_NULL_HANDLE) {
			m_DeleterFunction(m_VKHandle);
		}
		m_VKHandle = VK_NULL_HANDLE;
	}
};