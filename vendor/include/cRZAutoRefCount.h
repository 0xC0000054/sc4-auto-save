#pragma once
#include "cIGZUnknown.h"
#include <type_traits>

template<class T>
class cRZAutoRefCount
{
	static_assert(std::is_base_of<cIGZUnknown, T>::value, "T must extend cIGZUnknown");
	public:
		cRZAutoRefCount() {
			pObject = nullptr;
		}

		cRZAutoRefCount(T* pObject) {
			this->pObject = pObject;
		}

		cRZAutoRefCount<T>& operator=(T* pOther) {
			if (pObject != pOther) {
				if (pObject) {
					pObject->Release();
				}

				pObject = pOther;
				if (pOther) {
					pOther->AddRef();
				}
			}

			return *this;
		}

		virtual ~cRZAutoRefCount() {
			Reset();
		}

		void Reset() {
			T* temp = pObject;
			pObject = nullptr;

			if (temp) {
				temp->Release();
			}
		}

		void** AsPPVoidParam() {
			Reset();

			return reinterpret_cast<void**>(&pObject);
		}

		T* operator->() const { return pObject; }
		T& operator*() const { return *pObject; }
		operator T*() const { return pObject; }

	protected:
		T* pObject;
};