#pragma once
#include "gameobjects/gameobject.hpp"
#include <thread>
#include <mutex>

constexpr unsigned MAX_WORKER_THREADS = 1;
constexpr unsigned MAX_THREADS = MAX_WORKER_THREADS + 1;

unsigned CurrentThreadId();






class ThreadManager {
public:
	static ThreadManager& Get();

	void RunJobs(std::vector<std::function<void()>> jobs);

	// returns -1 if not worker thread
	static int CurrentWorkerThreadId() {
		//Assert(WORKER_THREAD_ID != -1);
		return WORKER_THREAD_ID;
	}

	

private:
	// -1 if this is not a worker thread.
	static thread_local inline int WORKER_THREAD_ID;

	ThreadManager();
	ThreadManager(const ThreadManager&) = delete;
	~ThreadManager();

	
	// true if the corresponding thread is still doing work.
	//std::array<std::atomic<bool>, MAX_WORKER_THREADS> threadsBusy;
	bool killWorkers = false;
	std::vector<std::function<void()>> remainingJobs;
	std::mutex remainingJobsMutex;
	std::condition_variable notifyWorkers;
	unsigned numIdleWorkers = 0;
	std::condition_variable notifyMain;

	// LAST member declared, first destroyed.
	std::vector<std::thread> threads;

	static void ThreadMain(int workerThreadId);
};

class ThreadedComponentIteration {
public:

	template<typename Func, std::derived_from<BaseComponent> ... Components>
	static void ForEachComponent(Func f, std::vector<ComponentBitIndex::ComponentBitIndex> requiredComponents) {
		
		static_assert(sizeof...(Components) != 0);

		std::bitset<N_COMPONENT_TYPES> requestedArchetype = GameobjectCreateParams(requiredComponents).requestedComponents;
		std::vector<ComponentPool*> matchingPools;
		for (auto& [archetype, pool] : GameObject::COMPONENT_POOLS) {
			if ((archetype & requestedArchetype) == requestedArchetype) {
				matchingPools.push_back(pool.get());
			}
		}

		std::vector<std::function<void()>> jobs;

		for (auto pool : matchingPools) {
			for (auto page : pool->pages) {
				auto runPage = [page, pool, f]() {
					std::tuple<Components* ...> currentComponents;
					OBJSIZE = pool->objectSize;
					(WriteTupleElement<Components*, Components...>(currentComponents, pool, page), ...);
					uint8_t** currentLiveChecker = (uint8_t**)(void*)(page);
					auto f2 = [](auto& ... a) {
						(AddIfNotNull(a), ...);
					};
					//unsigned prop = 0;
					for (unsigned i = 0; i < ComponentPool::COMPONENTS_PER_PAGE; i++) {
						//DebugLogInfo("CHECKER ", (void*)currentLiveChecker, " ", (void*)*currentLiveChecker);
						if (*currentLiveChecker == nullptr) {
							f(currentComponents);
							//prop++;
						}
						
						std::apply(f2, currentComponents);
						currentLiveChecker = (uint8_t**)((uint8_t*)currentLiveChecker + OBJSIZE);
					}
					//DebugLogInfo("Fed on ", prop, " OF ", ComponentPool::COMPONENTS_PER_PAGE, " also ", OBJSIZE);
				};
				jobs.push_back(runPage);
				//runPage();
			}
		}
		
		//for (auto& j : jobs) j();
		ThreadManager::Get().RunJobs(jobs);
	}

	template<typename Func, std::derived_from<BaseComponent> ... Components>
	static void ForEachComponent(Func f) {
		std::vector<ComponentBitIndex::ComponentBitIndex> requiredComponents;
		(requiredComponents.push_back(ComponentIdFromType<Components>()), ...);
		ForEachComponent<Func, Components...>(f, requiredComponents);
	}

	template <typename T>
	inline static void AddIfNotNull(T& t) {
		if (t != nullptr) {
			t = (T)((uint8_t*)t + OBJSIZE);
		}
	}

private:
	static thread_local unsigned inline OBJSIZE = 0;

	

	// stolen from stack overflow
	template <class T, class Tuple>
	struct TupleIndex;

	template <class T, class... Types>
	struct TupleIndex<T, std::tuple<T, Types...>> {
		static const std::size_t value = 0;
	};

	template <class T, class U, class... Types>
	struct TupleIndex<T, std::tuple<U, Types...>> {
		static const std::size_t value = 1 + TupleIndex<T, std::tuple<Types...>>::value;
	};

	template <typename ComponentPtr, typename ... Components>
	static void WriteTupleElement(std::tuple<Components*...>& tup, ComponentPool* pool, void* page) {
		static_assert(std::is_pointer<ComponentPtr>::value);

		short offset = -1;
		for (auto& memoryInfo : pool->componentLayout) {

			constexpr int id(ComponentIdFromType < typename std::remove_pointer<ComponentPtr>::type>());
			if (id == memoryInfo.componentId) {
				offset = memoryInfo.offset;
				break;
			}

		}

		//typedef I = TupleIndex<ComponentPtr, value_type>::value;
		//static_assert(I == 1000);

		if (offset == -1) {
			std::get<TupleIndex<ComponentPtr, std::tuple<Components*...>>::value>(tup) = nullptr;
		}
		else {
			std::get<TupleIndex<ComponentPtr, std::tuple<Components*...>>::value>(tup) = (ComponentPtr)((uint8_t*)page + offset);
		}
	}
};
