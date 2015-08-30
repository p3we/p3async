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

#ifndef P3WE_GUARD_P3DELEGATE_HPP
#define P3WE_GUARD_P3DELEGATE_HPP

#include <functional>

namespace p3
{
	/*!
	 * \brief Helper structure used to intruduce make_delegate function.
	 *
	 * The only way to implement smart make_delegate function which will guess
	 * number of placeholders parameters is to use partial specialization.
	 * Unfortunately C++ doesn't support function partial specialization, so
	 * it was necessarily to intruduce this intermidiate structure.
	 */
    template<size_t Arity, class T, typename R, typename ...Args>
	struct _DelegateFactory
	{
        static inline std::function<R (Args...)> bind(R (T::*pMethod)(Args...), T *pObj)
		{
			static_assert(Arity>=0 && Arity<=5, "Unsupported number of method parameters");
            return std::function<R (Args...)>();
		}
	};

    template<class T, typename R, typename ...Args>
    struct _DelegateFactory<0, T, R, Args...>
	{
        static inline std::function<R (Args...)> bind(R (T::*pMethod)(Args...), T *pObj)
		{
			return std::bind(pMethod, pObj);
		}
	};

    template<class T, typename R, typename ...Args>
    struct _DelegateFactory<1, T, R, Args...>
	{
        static inline std::function<R (Args...)> bind(R (T::*pMethod)(Args...), T *pObj)
        {
			using namespace std::placeholders;
			return std::bind(pMethod, pObj, _1);
		}
	};

    template<class T, typename R, typename ...Args>
    struct _DelegateFactory<2, T, R, Args...>
    {
        static inline std::function<R (Args...)> bind(R (T::*pMethod)(Args...), T *pObj)
        {
			using namespace std::placeholders;
			return std::bind(pMethod, pObj, _1, _2);
		}
	};

    template<class T, typename R, typename ...Args>
    struct _DelegateFactory<3, T, R, Args...>
    {
        static inline std::function<R (Args...)> bind(R (T::*pMethod)(Args...), T *pObj)
        {
			using namespace std::placeholders;
			return std::bind(pMethod, pObj, _1, _2, _3);
		}
	};

    template<class T, typename R, typename ...Args>
    struct _DelegateFactory<4, T, R, Args...>
    {
        static inline std::function<R (Args...)> bind(R (T::*pMethod)(Args...), T *pObj)
        {
			using namespace std::placeholders;
			return std::bind(pMethod, pObj, _1, _2, _3, _4);
		}
	};

    template<class T, typename R, typename ...Args>
    struct _DelegateFactory<5, T, R, Args...>
    {
        static inline std::function<R (Args...)> bind(R (T::*pMethod)(Args...), T *pObj)
        {
			using namespace std::placeholders;
			return std::bind(pMethod, pObj, _1, _2, _3, _4, _5);
		}
	};

	/*!
	 * \brief Smart helper function to create delegate for object method
	 * \note Maximum number of method parameters depends on implementation, be aware of
	 * compilation errors (currently maximum is 5).
	 * \param pMethod Address of the method.
	 * \param pObj Pointer to the object.
	 */
    template<class T, typename R, typename ...Args>
    inline std::function<R (Args...)> make_delegate(R (T::*pMethod)(Args...), T *pObj)
	{
        return _DelegateFactory<sizeof...(Args), T, R, Args...>::bind(pMethod, pObj);
	}
}

#endif /* P3WE_GUARD_P3DELEGATE_HPP */
