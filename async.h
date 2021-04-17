#pragma once
#include <future>
#include <iostream>

namespace crows 
{
	template<typename T, typename... Args>
	class Promise {
	public:
		Promise(const std::function<T(Args...)> &executor, Args&&... args) {
			this->status = Status::pPending;
			this->future = this->async(executor, std::forward<Args>(args)...);	
			//std::cout << "std Constructor is called - \n";
		}
		Promise(std::future<T>&& _future) : future(std::move(_future)) {
			this->status = Status::pPending;
			//std::cout << "fut Constructor is called - \n";
		}

		template<typename Callback, typename RejectCallback>
		Promise<std::invoke_result_t<Callback, T>>
		then(Callback&& callback, RejectCallback&& rejectCallback) {
			return Promise<std::invoke_result_t<Callback, T>>(
				this->async([this, &callback, &rejectCallback]() {
				try {
					this->status = Status::pResolved;
					return callback(future.get());
				}
				catch (T& ex) {
					this->status = Status::pRejected;
					return rejectCallback(ex);
				}
				catch (...) {
					this->status = Status::pRejected;
				}
			}));
		}
		template<typename Callback>
		Promise<std::invoke_result_t<Callback, T>>
		then(Callback&& callback) {
			return Promise<std::invoke_result_t<Callback, T>>(
				this->async([this, &callback]() {
				try {
					this->status = Status::pResolved;
					return callback(future.get());
				}
				catch (...) {
					this->status = Status::pRejected;
				}
			}));
		}
		
	private:
		enum class Status {
			pPending = 0,
			pResolved,
			pRejected
		};
		Status status;
		std::future<T> future;

		template<typename Function, typename... FArgs>
		std::future<std::invoke_result_t<Function, FArgs...>>
			async(Function&& fun, FArgs&&... args) const {
			return std::async(std::launch::async, std::forward<Function>(fun), std::forward<FArgs>(args)...);
		}
	};

	/*void*/

	template<typename... Args>
	class Promise<void, Args...> {
	public:
		Promise(const std::function<void(Args...)> &executor, Args&&... args) {
			this->status = Status::pPending;
			this->future = this->async(executor, std::forward<Args>(args)...);
			//std::cout << "void std Constructor is called - \n";
		}
		Promise(std::future<void>&& _future) : future(std::move(_future)) {
			this->status = Status::pPending;
			//std::cout << "void fut Constructor is called - \n";
		}
	
		template<typename Callback, typename RejectCallback>
		Promise< std::invoke_result_t<Callback> >
		then(Callback&& callback, RejectCallback&& rejectCallback) {
			return Promise<std::invoke_result_t<Callback>>(
				this->async([this, &callback, &rejectCallback]() {
				try {
					this->status = Status::pResolved;
					future.wait();
					return callback();
				}
				catch (...) {
					this->status = Status::pRejected;
					return rejectCallback();
				}
			}));
		}
		template<typename Callback>
		Promise<std::invoke_result_t<Callback>>
		then(Callback&& callback) {
			return Promise<std::invoke_result_t<Callback>>(
				this->async([this, &callback]() {
				try {
					this->status = Status::pResolved;
					future.wait();
					return callback();
				}
				catch (...) {
					this->status = Status::pRejected;
				}
			}));
		}

	private:
		enum class Status {
			pPending = 0,
			pResolved,
			pRejected
		};
		Status status;
		std::future<void> future;

		template<typename Function, typename... FArgs>
		std::future<std::invoke_result_t<Function, FArgs...>>
		async(Function&& fun, FArgs&&... args) const {
			return std::async(std::launch::async, std::forward<Function>(fun), std::forward<FArgs>(args)...);
		}
	};
}


