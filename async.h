#pragma once
#include <future>
#include <variant>
#include <iostream>

namespace crows 
{
	template<typename T>
	class _promise_base {
	public:
		
		template<typename Function, typename... Args>
		_promise_base(Function&& fun, Args&&... args) {
			this->future = std::async(std::launch::async, std::forward<Function>(fun), std::forward<Args>(args)...);
		}
		_promise_base(std::future<T>&& _future) : future(std::move(_future)) {
		}

		_promise_base(const _promise_base &) = delete;
		
		bool valid() {
			return this->future.valid();
		}

		virtual ~_promise_base() = default;

	protected:
		std::future<T> future;
	};


	template<typename T>
	class Promise : public _promise_base<T> {
	public:
		template<typename Function, typename... Args>
		Promise(Function&& fun, Args&&... args) :
			_promise_base<T>(std::forward<Function>(fun), std::forward<Args>(args)...) {
		}

		template<typename Callback, typename RejectCallback, typename Result = std::invoke_result_t<Callback, T>>
		Promise<Result>
		then(Callback&& callback, RejectCallback&& rejectCallback) {
			return Promise<Result>(
				[future = this->future.share(), &callback, &rejectCallback] {
				std::exception_ptr eptr;
				try {
					T result = future.get();
					try {
						return callback(std::move(result));
					}
					catch (...) {
						eptr = std::current_exception();
					}
				}
				catch (const T& ex) {
					return rejectCallback(ex);
				}
				catch (...) {
					eptr = std::current_exception();
				}

				if (eptr) {
					std::rethrow_exception(eptr);
				}

			});
		}

		template<typename Callback, typename Result = std::invoke_result_t<Callback, T>>
		Promise<Result>
			then(Callback&& callback) {
			return Promise<Result>(
				[future = this->future.share(), &callback] {
				try {
					return callback(future.get());
				}
				catch (...) {
					std::rethrow_exception(std::current_exception());
				}
			});
		}

	};

	template<>
	class Promise<void> : public _promise_base<void> {
	public:
		template<typename Function, typename... Args>
		explicit Promise(Function&& fun, Args&&... args) :
			_promise_base<void>(std::forward<Function>(fun), std::forward<Args>(args)...) {
		}

		Promise(std::future<void>& _future) : _promise_base(std::move(_future)) {
		}

		template<typename Callback, typename RejectCallback, typename Result = std::invoke_result_t<Callback>>
		Promise<Result>
			then(Callback&& callback, RejectCallback&& rejectCallback) {
			return Promise<Result>(
				[future = this->future.share(), &callback, &rejectCallback] {
				std::exception_ptr eptr;
				try {
					future.get();	

					try {
						return callback();
					}
					catch (...) {
						eptr = std::current_exception();
					}
				}
				catch (...) {
					return rejectCallback();
				}

				if (eptr) {
					std::rethrow_exception(eptr);
				}				
			});
		}
		template<typename Callback, typename Result = std::invoke_result_t<Callback>>
		Promise<Result>
			then(Callback&& callback) {
			return Promise<Result>(
				[future = this->future.share(), &callback] {
				try {
					future.get();
					return callback();
				}
				catch (...) {
					std::rethrow_exception(std::current_exception());
				}
			});
		}
		

	};


	
}


