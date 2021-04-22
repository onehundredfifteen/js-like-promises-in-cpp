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
		
		bool valid() {
			return this->future.valid();
		}

		virtual ~_promise_base() = default;

	protected:
		std::future<T> future;

		void propagate_exception(std::exception_ptr eptr) {
			try {
				if (eptr) {
					std::rethrow_exception(eptr);
				}
			}
			catch (const std::exception& e) {
			}
		}
	};


	template<typename T>
	class Promise : public _promise_base<T> {
	public:
		template<typename Function, typename... Args>
		Promise(Function&& fun, Args&&... args) :
			_promise_base<T>(fun, std::forward<Args>(args)...) {
		}

		template<typename Callback, typename RejectCallback, typename Result = std::invoke_result_t<Callback, T>>
		Promise<Result>
		then(Callback&& callback, RejectCallback&& rejectCallback) {
			return Promise<Result>(
				[this, &callback, &rejectCallback] {
				std::exception_ptr eptr;
				try {
					T result = this->future.get();
					try {
						return callback(std::forward<T>(result));
					}
					catch (...) {
						this->propagate_exception(eptr);
					}
				}
				catch (const T& ex) {
					return rejectCallback(ex);
				}
				catch (...) {
					this->propagate_exception(eptr);
				}
			});
		}

		template<typename Callback, typename Result = std::invoke_result_t<Callback, T>>
		Promise<Result>
			then(Callback&& callback) {
			return Promise<Result>(
				[this, &callback] {
				std::exception_ptr eptr;
				try {
					return callback(this->future.get());
				}
				catch (...) {
					this->propagate_exception(eptr);
				}
			});
		}

	};

	template<>
	class Promise<void> : public _promise_base<void> {
	public:
		template<typename Function, typename... Args>
		explicit Promise(Function&& fun, Args&&... args) :
			_promise_base<void>(fun, std::forward<Args>(args)...) {
		}

		Promise(std::future<void>& _future) : _promise_base(std::move(_future)) {
		}

		template<typename Callback, typename RejectCallback, typename Result = std::invoke_result_t<Callback>>
		Promise<Result>
			then(Callback&& callback, RejectCallback&& rejectCallback) {
			return Promise<Result>(
				[this, &callback, &rejectCallback] {
				std::exception_ptr eptr;
				try {
					this->future.get();
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
				this->propagate_exception(eptr);
			});
		}
		template<typename Callback, typename Result = std::invoke_result_t<Callback>>
		Promise<Result>
			then(Callback&& callback) {
			return Promise<Result>(
				[this, &callback] {
				std::exception_ptr eptr;
				try {
					this->future.get();
					return callback();
				}
				catch (...) {
					eptr = std::current_exception();
				}
				this->propagate_exception(eptr);
			});
		}
		

	};


	
}


