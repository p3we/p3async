/*
 * (C) Copyright 2015 Artur Sobierak <asobierak@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef P3WE_GUARD_P3SIGNALS_HPP
#define P3WE_GUARD_P3SIGNALS_HPP

#include <functional>
#include <algorithm>
#include <utility>
#include <vector>
#include <mutex>

#include "delegate.hpp"

namespace utils
{
	/* forward declarations */
	template<typename ...Args> class Slot;
	template<typename ...Args> class Signal;

	/*!
	 * \brief Slot holds and manage connection between signal and its handler.
	 * Lifetime of slot has to be the same as lifetime of handler function.
	 * After slot or signal destruction also connection is automaticly invalidated.
	 */
	template<typename ...Args>
	class Slot
	{
	public:
		friend class Signal<Args...>;

	public:
		Slot(const std::function<void(Args...)>& func)
		: m_func(func)
		, m_pSignal(nullptr)
		, m_id(0)
		{
			// pass
		}

		Slot(Slot&&) = default;

		Slot(const Slot&) = delete;

		Slot& operator =(const Slot&) = delete;

		~Slot()
		{
			if (isConnected())
			{
				m_pSignal->disconnect(*this);
			}
		}

	public:
		/*!
		 * \brief Checks if slot is connected to signal
		 */
		operator bool () const
		{
			return isConnected();
		}

		/*!
		 * \brief Check if slot is connected to signal
		 */
		inline bool isConnected() const
		{
			return (m_pSignal!=nullptr && m_id>0);
		}

		/*!
		 * \brief Explicity executes slot function
		 * \param args
		 */
		void operator ()(Args... args)
		{
			exec(args...);
		}

		/*!
		 * \brief Explicity executes slot function
		 * \param args
		 */
		inline void exec(Args... args)
		{
			if (m_func)
				m_func(args...);
		}

	private:
		bool attach(Signal<Args...> *pSignal, size_t id)
		{
			if (m_pSignal == nullptr && m_id == 0)
			{
				m_pSignal = pSignal;
				m_id = id;
				return true;
			}
			return false;
		}

		bool detach(Signal<Args...> *pSignal, size_t id)
		{
			if (m_pSignal == pSignal && m_id == id)
			{
				m_pSignal = nullptr;
				m_id = 0;
				return true;
			}
			return false;
		}

	private:
		std::function<void(Args...)> m_func; ///> function executed on signal trigger
		Signal<Args...> *m_pSignal; ///> pointer to the connected signal
		size_t m_id; ///> connection identifier
	};

	/*!
	 * \brief Signal
	 */
	template<typename ...Args>
	class Signal
	{
	public:
		typedef std::vector<std::pair<size_t,Slot<Args...>*>> SlotContainer;

	public:
		Signal()
		: m_slotIdSeq(1)
		{
			// pass
		}

		Signal(Signal&&) = default;

		Signal(const Signal&) = delete;

		Signal& operator =(const Signal&) = delete;

		~Signal()
		{
			for (auto itr = m_slots.begin(); itr != m_slots.end(); ++itr)
			{
				itr->second->detach(this, itr->second->m_id);
			}
		}

	public:
		/*!
		 * \brief Trigger signal and executes connected slots
		 * \param args
		 */
		void operator ()(Args... args)
		{
			fire(args...);
		}

		/*!
		 * \brief Trigger signal and executes connected slots
		 * \param args
		 */
		void fire(Args... args)
		{
			std::unique_lock<std::recursive_mutex> lock;

			for (auto itr = m_slots.begin(); itr != m_slots.end(); ++itr)
			{
				itr->second->exec(args...);
			}
		}

		/*!
		 * \brief Connects given slot to signal
		 * \param slot
		 * \return
		 */
		Signal& operator +=(Slot<Args...> &slot)
		{
			return connect(slot);
		}

		/*!
		 * \brief Connects given slot to signal
		 * \param slot
		 * \return
		 */
		Signal& connect(Slot<Args...> &slot)
		{
			std::unique_lock<std::recursive_mutex> lock;

			if (slot.attach(this, m_slotIdSeq++))
			{
				m_slots.push_back(std::make_pair(slot.m_id, &slot));
			}

			return *this;
		}

		/*!
		 * \brief Disconnects given slot from signal
		 * \param slot
		 * \return
		 */
		Signal& operator -=(Slot<Args...> &slot)
		{
			return disconnect(slot);
		}

		/*!
		 * \brief Disconnects given slot from signal
		 * \param slot
		 * \return
		 */
		Signal& disconnect(Slot<Args...> &slot)
		{
			std::unique_lock<std::recursive_mutex> lock;

			if (slot.detach(this, slot.m_id))
			{
				auto pred = [](std::pair<size_t,Slot<Args...>*> &item) -> bool { return (item.second->m_id == 0); };
				auto itr = std::remove_if(m_slots.begin(), m_slots.end(), pred);
				m_slots.erase(itr, m_slots.end());
			}

			return *this;
		}

		/*!
		 * \brief Returns connection quantity
		 * \return
		 */
		inline std::size_t quantity() const
		{
			std::unique_lock<std::recursive_mutex> lock;
			return m_slots.size();
		}

	private:
		mutable std::recursive_mutex m_mutex; ///> mutex to protect signal data
		std::size_t m_slotIdSeq; ///> sequance used to generate connection identifiers
		std::vector<std::pair<size_t,Slot<Args...>*>> m_slots; ///> vector of connected slots
	};

	/*!
	 * \brief Helper function creates slot for given object method
	 */
	template<class T, typename ...Args>
	inline Slot<Args...> make_slot(void (T::*pMethod)(Args...), T *pObj)
	{
		return Slot<Args...>(make_delegate(pMethod, pObj));
	}
}

#endif /* P3WE_GUARD_P3SIGNALS_HPP */
