#include <stdexcept>
#include <cstdlib>

#include "include/event_loop.hpp"

using namespace std;

namespace utils {

std::shared_ptr<IdleWatcher> IdleWatcher::Create(EventLoop *pLoop, const std::function<void ()> &handler)
{
	shared_ptr<IdleWatcher> pSelf(new IdleWatcher(pLoop, handler));
	pSelf->m_pThis = pSelf;

	return pSelf;
}

void IdleWatcher::IdleWatcherCallback(struct ev_loop *pLoop, ev_idle *pWatcher, int events)
{
	IdleWatcher *pSelf = reinterpret_cast<IdleWatcher*>(pWatcher->data);
    pSelf->invoke(events);
}

IdleWatcher::IdleWatcher(EventLoop *pLoop, const std::function<void()> &handler)
: m_pLoop(pLoop)
, m_isRunning(false)
, m_handler(handler)
{
	ev_idle_init(&m_watcher, &IdleWatcher::IdleWatcherCallback);
	m_watcher.data = this;
}

void IdleWatcher::invoke(int events)
{
	m_handler();
}

void IdleWatcher::start()
{
	if (m_pLoop->start(m_pThis.lock()))
	{
		m_isRunning = true;
	}
}

void IdleWatcher::stop()
{
	if (m_pLoop->stop(m_pThis.lock()))
	{
		m_isRunning = false;
	}
}

bool IdleWatcher::isRunning() const
{
	return m_isRunning;
}

std::shared_ptr<TimerWatcher> TimerWatcher::Create(EventLoop *pLoop, const std::function<void ()> &handler, double timeout, bool repeat)
{
	shared_ptr<TimerWatcher> pSelf(new TimerWatcher(pLoop, handler, timeout, repeat));
	pSelf->m_pThis = pSelf;

	return pSelf;
}

void TimerWatcher::TimerWatcherCallback(struct ev_loop *pLoop, ev_timer *pWatcher, int events)
{
	TimerWatcher *pSelf = reinterpret_cast<TimerWatcher*>(pWatcher->data);
    pSelf->invoke(events);
}

TimerWatcher::TimerWatcher(EventLoop *pLoop, const std::function<void()> &handler, double timeout, bool repeat)
: m_pLoop(pLoop)
, m_isRunning(false)
, m_handler(handler)
, m_timeout(timeout)
, m_repeat(repeat)
{
	ev_timer_init(&m_watcher, &TimerWatcher::TimerWatcherCallback, m_timeout, 0);
	m_watcher.data = this;
}

void TimerWatcher::invoke(int events)
{
	// resechdule timer if it is repeating timer
	if (m_repeat)
	{
		auto pSharedSelf = m_pThis.lock();
		if (m_pLoop->stop(pSharedSelf))
		{
			m_isRunning = false;
			ev_timer_set(&m_watcher, m_timeout, 0);
			m_isRunning = m_pLoop->start(pSharedSelf);
		}

		m_handler();
	}
	else
	{
		m_handler();
		stop();
	}
}

void TimerWatcher::start()
{
	if (m_pLoop->start(m_pThis.lock()))
	{
		m_isRunning = true;
	}
}

void TimerWatcher::stop()
{
	if (m_pLoop->stop(m_pThis.lock()))
	{
		m_isRunning = false;
	}
}

bool TimerWatcher::isRunning() const
{
	return m_isRunning;
}

std::shared_ptr<InOutWatcher> InOutWatcher::Create(EventLoop *pLoop, const std::function<void(int,int)> &handler, int fd, Event event)
{
    shared_ptr<InOutWatcher> pSelf(new InOutWatcher(pLoop, handler, fd, event));
    pSelf->m_pThis = pSelf;

    return pSelf;
}

void InOutWatcher::InOutWatcherCallback(struct ev_loop *pLoop, ev_io *pWatcher, int revents)
{
    InOutWatcher *pSelf = reinterpret_cast<InOutWatcher*>(pWatcher->data);
    pSelf->invoke(revents);
}

InOutWatcher::InOutWatcher(EventLoop *pLoop, const std::function<void(int,int)> &handler, int fd, Event event)
: m_pLoop(pLoop)
, m_isRunning(false)
, m_handler(handler)
, m_fd(fd)
, m_event(event)
{
    int events = 0;
    events |= (m_event|Event_Readable) ? EV_READ : 0;
    events |= (m_event|Event_Writeable) ? EV_WRITE : 0;

    ev_io_init(&m_watcher, &InOutWatcher::InOutWatcherCallback, m_fd, events);
    m_watcher.data = this;
}

void InOutWatcher::invoke(int events)
{
	m_handler(m_fd, events);
    stop();
}

void InOutWatcher::start()
{
    if (m_pLoop->start(m_pThis.lock()))
    {
        m_isRunning = true;
    }
}

void InOutWatcher::stop()
{
    if (m_pLoop->stop(m_pThis.lock()))
    {
        m_isRunning = false;
    }
}

bool InOutWatcher::isRunning() const
{
    return m_isRunning;
}

bool InOutWatcher::change(Event event)
{
    if (m_event != event)
    {
        auto pSharedSelf = m_pThis.lock();
        if (m_pLoop->stop(pSharedSelf))
        {
            m_isRunning = false;
            m_event = event;

            int events = 0;
            events |= (m_event|Event_Readable) ? EV_READ : 0;
            events |= (m_event|Event_Writeable) ? EV_WRITE : 0;

            ev_io_set(&m_watcher, m_fd, events);
            m_isRunning = m_pLoop->start(pSharedSelf);

            return m_isRunning;
        }
    }

    return false;
}

} /* end of namespace utils */

namespace utils {

std::shared_ptr<EventLoop> EventLoop::GetMainLoop()
{
	static shared_ptr<EventLoop> pMainLoop;
	if (!pMainLoop)
	{
		pMainLoop.reset(new EventLoop());
	}
	return pMainLoop;
}

void EventLoop::AsyncWatcherCallback(struct ev_loop *loop, ev_async *pWatcher, int revents)
{
	EventLoop *pSelf = reinterpret_cast<EventLoop*>(pWatcher->data);
	pSelf->processDeferrers();
}

EventLoop::EventLoop()
{
	m_pLoop = ev_default_loop();
	if (!m_pLoop)
	{
		throw runtime_error("Can't create libev event loop");
	}

	m_pAsync = reinterpret_cast<struct ev_async*>(malloc(sizeof(ev_async)));
	if (!m_pAsync)
	{
		throw runtime_error("Can't create libev async watcher");
	}
	ev_async_init(m_pAsync, &EventLoop::AsyncWatcherCallback);
	m_pAsync->data = this;
	ev_async_start(m_pLoop, m_pAsync);
}


EventLoop::~EventLoop()
{
	ev_async_stop(m_pLoop, m_pAsync);
	free(m_pAsync);
	ev_loop_destroy(m_pLoop);
}

std::shared_ptr<TimerWatcher> EventLoop::timer(const std::function<void ()> &handler, double timeout, bool repeat)
{
	auto pWatcher = TimerWatcher::Create(this, handler, timeout, repeat);
	pWatcher->start();

    return pWatcher;
}

std::shared_ptr<InOutWatcher> EventLoop::inout(const std::function<void(int,int)> &handler, int fd, InOutWatcher::Event event)
{
    auto pWatcher = InOutWatcher::Create(this, handler, fd, event);
    pWatcher->start();

    return pWatcher;
}

std::shared_ptr<IdleWatcher> EventLoop::idle(const std::function<void ()> &handler)
{
	auto pWatcher = IdleWatcher::Create(this, handler);
	pWatcher->start();

	return pWatcher;
}

void EventLoop::defer(const std::function<void()> &handler)
{
	{
		unique_lock<recursive_mutex> lock(m_mutex);
		m_deferrers.push(handler);
	}
	ev_async_send(m_pLoop, m_pAsync);
}

int EventLoop::run()
{
	return ev_run(m_pLoop);
}

void EventLoop::brk()
{
	ev_break(m_pLoop, EVBREAK_ALL);
}

bool EventLoop::start(std::shared_ptr<TimerWatcher> pWatcher)
{
	if (pWatcher && !pWatcher->isRunning())
	{
		unique_lock<recursive_mutex> lock(m_mutex);
		// increase reference counter
		m_watchers.insert(pWatcher);
		// start internal libev watcher
		ev_timer_start(m_pLoop, &pWatcher->m_watcher);
		return true;
	}

	return false;
}

bool EventLoop::start(std::shared_ptr<IdleWatcher> pWatcher)
{
	if (pWatcher && !pWatcher->isRunning())
	{
		unique_lock<recursive_mutex> lock(m_mutex);
		// increase reference counter
		m_watchers.insert(pWatcher);
		// start internal libev watcher
		ev_idle_start(m_pLoop, &pWatcher->m_watcher);
		return true;
	}

    return false;
}

bool EventLoop::start(std::shared_ptr<InOutWatcher> pWatcher)
{
    if (pWatcher && !pWatcher->isRunning())
    {
        unique_lock<recursive_mutex> lock(m_mutex);
        // increase reference counter
        m_watchers.insert(pWatcher);
        // start internal libev watcher
        ev_io_start(m_pLoop, &pWatcher->m_watcher);
        return true;
    }

    return false;
}

bool EventLoop::stop(std::shared_ptr<TimerWatcher> pWatcher)
{
	if (pWatcher && pWatcher->isRunning())
	{
		unique_lock<recursive_mutex> lock(m_mutex);
		// decrease reference counter
		m_watchers.erase(pWatcher);
		// stop internal libev watcher
		ev_timer_stop(m_pLoop, &pWatcher->m_watcher);
		return true;
	}

	return false;
}

bool EventLoop::stop(std::shared_ptr<IdleWatcher> pWatcher)
{
	if (pWatcher && pWatcher->isRunning())
	{
		unique_lock<recursive_mutex> lock(m_mutex);
		// decrease reference counter
		m_watchers.erase(pWatcher);
		// stop internal libev watcher
		ev_idle_stop(m_pLoop, &pWatcher->m_watcher);
		return true;
	}

	return false;
}

bool EventLoop::stop(std::shared_ptr<InOutWatcher> pWatcher)
{
    if (pWatcher && pWatcher->isRunning())
    {
        unique_lock<recursive_mutex> lock(m_mutex);
        // decrease reference counter
        m_watchers.erase(pWatcher);
        // stop internal libev watcher
        ev_io_stop(m_pLoop, &pWatcher->m_watcher);
        return true;
    }

    return false;
}

void EventLoop::processDeferrers()
{
	bool empty = true;

	do
	{
		function<void()> func;
		{
			unique_lock<recursive_mutex> lock(m_mutex);
			empty = m_deferrers.empty();
			if (!empty)
			{
				func = m_deferrers.front();
				m_deferrers.pop();
			}
		}

		if (func)
		{
			func();
		}
	}
	while (!empty);
}

} /* end of namespace utils */
