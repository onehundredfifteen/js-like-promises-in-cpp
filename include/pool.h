#pragma once
#ifndef PROMISE_POOL_INCLUDED
#define PROMISE_POOL_INCLUDED

#include <list>
#include "./base.h"

namespace pro
{
	namespace detail
	{
		template<typename T>
		class _promise_base;
		
		class pool_container
		{
		private:
			template<class T>
			static std::unordered_map<const pool_container*, std::list<T>> items;

		public:
			static pool_container& instance()
			{
				static pool_container instance_;
				return instance_;
			}

			template<typename T, typename PTy_ = _promise_base<T>>
			void submit(PTy_& promise) {
				items<PTy_>[this].emplace_back(std::move(promise));
			}
				
		private:
			pool_container() {};
			pool_container(const pool_container&) {};
			~pool_container() {}
		};

		// storage for static members
		template<class T>
		std::unordered_map<const pool_container*, std::list<T>> pool_container::items;
	}
}
#endif //PROMISE_POOL_INCLUDED
