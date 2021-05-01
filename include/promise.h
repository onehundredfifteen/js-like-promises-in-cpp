#pragma once
#include "./base.h"

namespace pro
{
	template<typename T>
	class Promise : public detail::_promise_base<T> {
	public:
		template<typename Function, typename... Args>
		Promise(Function&& fun, Args&&... args) :
			detail::_promise_base<T>(std::forward<Function>(fun), std::forward<Args>(args)...) {
		}

		template<typename Cb, typename RCb, typename Result = std::invoke_result_t<Cb, T>,
		typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<RCb, T>>::value>>
		Promise<Result> then(Cb&& callback, RCb&& rejectCallback) {
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
					return rejectCallback(std::move(ex));
				}
				catch (...) {
					eptr = std::current_exception();
				}

				if (eptr) {
					std::rethrow_exception(eptr);
				}
			});
		}

		template<typename Cb, typename Result = std::invoke_result_t<Cb, T>>
		Promise<Result> then(Cb&& callback) {
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

		template<typename RCb, typename ExCb, typename Result = std::invoke_result_t<RCb, T>,
		typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<ExCb, std::exception_ptr>>::value>>
		Promise<Result> fail(RCb&& rejectCallback, ExCb&& exceptionCallback) {
			return Promise<Result>(
				[future = this->future.share(), &rejectCallback, &exceptionCallback]{
				try {
					future.get();
				}
				catch (const T& ex) {
					return rejectCallback(std::move(ex));
				}
				catch (...) {
					return exceptionCallback(std::current_exception());
				}
			});
		}
	};

	template<>
	class Promise<void> : public detail::_promise_base<void> {
	public:
		template<typename Function, typename... Args>
		explicit Promise(Function&& fun, Args&&... args) :
			detail::_promise_base<void>(std::forward<Function>(fun), std::forward<Args>(args)...) {
		}

		Promise(std::future<void>& _future) : _promise_base(std::move(_future)) {
		}

		template<typename Cb, typename RCb, typename Result = std::invoke_result_t<Cb>,
		typename = std::enable_if_t<std::is_same<Result, std::invoke_result_t<RCb>>::value>>
		Promise<Result> then(Cb&& callback, RCb&& rejectCallback) {
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

		template<typename Cb, typename Result = std::invoke_result_t<Cb>>
		Promise<Result> then(Cb&& callback) {
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

		template<typename ExCb, typename Result = std::invoke_result_t<ExCb, std::exception_ptr>>
		Promise<Result> fail(ExCb&& exceptionCallback) {
			return Promise<Result>(
				[future = this->future.share(), &exceptionCallback]{
				try {
					future.get();
				}
				catch (...) {
					return exceptionCallback(std::current_exception());
				}
			});
		}
	};


	
}


