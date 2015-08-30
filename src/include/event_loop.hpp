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

#ifndef P3WE_EVENT_LOOP_H
#define P3WE_EVENT_LOOP_H

#include <ev.h>
#include <memory>
#include <functional>
#include <queue>
#include <set>
#include <mutex>

namespace utils
{
	/* forward declarations */
	class EventLoop;

	/*!
	 * \brief The Watcher interface
	 */
	class Watcher
	{
	public:
		virtual ~Watcher() {}

	public:
		/*!
		 * \brief Invoke registered handler
		 */
        virtual void invoke(int events) = 0;

		/*!
		 * \brief Starts watcher what makes watcher active source if events
		 */
		virtual void start() = 0;

		/*!
		 * \brief Stops watcher what prevent watcher from generating events
		 */
		virtual void stop() = 0;

		/*!
		 * \brief Checks if watcher is running (was started)
		 * \return
		 */
		virtual bool isRunning() const = 0;
	};

	/*!
	 * \brief The IdleWatcher is a source of background events
	 *
	 * Idle watcher executes its handler when there is no other active watchers
	 * with higher priorities. It is sutable for background periodic tasks.
	 */
	class IdleWatcher : public Watcher
	{
	public:
		friend class EventLoop;

	public:
		/*!
		 * \brief Creates IdleWatcher and setup internal m_pThis pointer
		 * \note This method should not be used directly
		 */
		static std::shared_ptr<IdleWatcher> Create(EventLoop *pLoop, const std::function<void()> &handler);

		/*!
		 * \brief Standard ev_idle callback function
		 */
		static void IdleWatcherCallback(struct ev_loop *pLoop, ev_idle *pWatcher, int revents);

	public:
        void invoke(int events);
		void start();
		void stop();
		bool isRunning() const;

	private:
		IdleWatcher(EventLoop *pLoop, const std::function<void()> &handler);

	private:
		std::weak_ptr<IdleWatcher> m_pThis;
		EventLoop *m_pLoop;
		bool m_isRunning;
		std::function<void()> m_handler;
		struct ev_idle m_watcher;
	};

	/*!
	 * \brief The TimerWatcher is time triggered event source.
	 * TimeWatcher can generate one shot time event or repeted interval event.
	 */
	class TimerWatcher : public Watcher
	{
	public:
		friend class EventLoop;

	public:
		/*!
		 * \brief Creates TimerWatcher and setup internal m_pThis pointer
		 * \note This method should not be used directly
		 */
        static std::shared_ptr<TimerWatcher> Create(EventLoop *pLoop, const std::function<void()> &handler, double timeout, bool repeat);

		/*!
		 * \brief Standard ev_timer callback function
		 */
		static void TimerWatcherCallback(struct ev_loop *pLoop, ev_timer *pWatcher, int revents);

	public:
        void invoke(int events);
		void start();
		void stop();
		bool isRunning() const;

	private:
		TimerWatcher(EventLoop *pLoop, const std::function<void()> &handler, double timeout, bool repeat);

	private:
		std::weak_ptr<TimerWatcher> m_pThis;
		EventLoop *m_pLoop;
		bool m_isRunning;
        std::function<void()> m_handler;
		struct ev_timer m_watcher;
		double m_timeout;
		bool m_repeat;
	};

    /*!
     * \brief The InOutWatcher is file descriptor triggered event source.
     */
    class InOutWatcher : public Watcher
    {
    public:
        friend class EventLoop;

    public:
        enum Event
        {
            Event_None = 0,
            Event_Readable = 1,
            Event_Writeable = 2,
            Event_Both = 3
        };

    public:
        /*!
         * \brief Creates TimerWatcher and setup internal m_pThis pointer
         * \note This method should not be used directly
         */
		static std::shared_ptr<InOutWatcher> Create(EventLoop *pLoop, const std::function<void(int, int)> &handler, int fd, Event event = Event_Both);

        /*!
         * \brief Standard ev_timer callback function
         */
        static void InOutWatcherCallback(struct ev_loop *pLoop, ev_io *pWatcher, int revents);

    public:
        void invoke(int events);
        void start();
        void stop();
        bool isRunning() const;
        bool change(Event event);

    private:
		InOutWatcher(EventLoop *pLoop, const std::function<void(int,int)> &handler, int fd, Event event);

    private:
        std::weak_ptr<InOutWatcher> m_pThis;
        EventLoop *m_pLoop;
        bool m_isRunning;
		std::function<void(int,int)> m_handler;
        struct ev_io m_watcher;
        int m_fd;
        Event m_event;
    };


	/*!
	 * \brief The EventLoop is tiny abstraction for libev event loop.
	 * Stock libev event loop is not thread safe but all methods of this class are
	 * protected by mutex.
	 */
	class EventLoop
	{
	public:
		/*!
		 * \brief Returns (and creates if needed) handle to main global event loop.
		 * \return
		 */
		static std::shared_ptr<EventLoop> GetMainLoop();

		/*!
		 * \brief Standard ev_async callback
		 */
		static void AsyncWatcherCallback(struct ev_loop *pLoop, ev_async *pWatcher, int revents);

	public:
		EventLoop();
		~EventLoop();

	public:
		/*!
		 * \brief Creates and starts IdleWatcher
		 * \param handler function executed on event
		 * \return
		 */
		std::shared_ptr<IdleWatcher> idle(const std::function<void()> &handler);

		/*!
		 * \brief Creates and starts TimetWatcher with given timeout and repeat values
		 * \param handler function executed on event
		 * \param timeout time value in seconds
		 * \param repeat true if timeout should be repeted
		 * \return
		 */
		std::shared_ptr<TimerWatcher> timer(const std::function<void()> &handler, double timeout, bool repeat = false);

        /*!
         * \brief inout
         * \param handler
         * \param events
         * \return
         */
		std::shared_ptr<InOutWatcher> inout(const std::function<void(int,int)> &handler, int fd, InOutWatcher::Event event);

		/*!
		 * \brief Defer function execution by delegating it to the event loop
		 * \param handler function executed on event
		 */
		void defer(const std::function<void()> &handler);

		/*!
		 * \brief Runs event loop in current thred.
		 * \note Function is blocking
		 */
		int run();

		/*!
		 * \brief Breaks loop and force run() function to return.
		 * \note Loop breaks after processing current events.
		 */
		void brk();

	public:
		/*!
		 * \brief Starts ev_timer watcher under mutex protection
		 */
		bool start(std::shared_ptr<TimerWatcher> pWatcher);
		/*!
		 * \brief Starts ev_idle watcher under mutex protection
		 */
		bool start(std::shared_ptr<IdleWatcher> pWatcher);
        /*!
         * \brief Starts ev_io watcher under mutex protection
         */
        bool start(std::shared_ptr<InOutWatcher> pWatcher);
        /*!
		 * \brief Stop ev_timer watcher under mutex protection
		 */
		bool stop(std::shared_ptr<TimerWatcher> pWatcher);
		/*!
		 * \brief Stops ev_idle watcher under mutex protection
		 */
		bool stop(std::shared_ptr<IdleWatcher> pWatcher);
        /*!
         * \brief Stops ev_io watcher under mutex protection
         */
        bool stop(std::shared_ptr<InOutWatcher> pWatcher);

	private:
		/*!
		 * \brief Executed queued deferred functions
		 */
		void processDeferrers();

	private:
		mutable std::recursive_mutex m_mutex; ///> main mutex protecting all intenal data
		struct ev_loop *m_pLoop; ///> libev loop
		struct ev_async *m_pAsync; ///> libev async used to dispatch deferred functions
		std::set<std::shared_ptr<Watcher>> m_watchers; ///> local copy of active watchers
		std::queue<std::function<void()>> m_deferrers; ///> queue with deferred functions
	};
}

#endif /* P3WE_EVENT_LOOP_H */
