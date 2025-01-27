#pragma once

#include <Debut/Events/Event.h>

#include <string>
#include <sstream>

namespace Debut
{
	class WindowResizedEvent : public Event
	{
	public:
		WindowResizedEvent(unsigned int width, unsigned int height) : m_Width(width), m_Height(height) {}

		inline std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		inline int GetWidth() const { return m_Width; }
		inline int GetHeight() const { return m_Height; }

		EVENT_CLASS_CATEGORY(ApplicationEvent)
		EVENT_CLASS_TYPE(WindowResize)

	private:
		unsigned int m_Width;
		unsigned int m_Height;
	};

	class WindowMovedEvent : public Event
	{
	public:
		WindowMovedEvent(unsigned int x, unsigned int y) : m_x(x), m_y(y) {}

		inline std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowMovedEvent: " << m_x << ", " << m_y;
			return ss.str();
		}

		inline int GetX() const { return m_x; }
		inline int GetY() const { return m_y; }

		EVENT_CLASS_CATEGORY(ApplicationEvent)
		EVENT_CLASS_TYPE(WindowMoved)

	private:
		unsigned int m_x;
		unsigned int m_y;
	};

	class WindowFocusEvent : public Event
	{
	public:
		WindowFocusEvent() {}

		inline std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowFocusEvent";
			return ss.str();
		}


		EVENT_CLASS_CATEGORY(ApplicationEvent)
		EVENT_CLASS_TYPE(WindowFocus)
	};

	class WindowLostFocusEvent : public Event
	{
	public:
		WindowLostFocusEvent() {}

		inline std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowLostFocusEvent";
			return ss.str();
		}


		EVENT_CLASS_CATEGORY(ApplicationEvent)
		EVENT_CLASS_TYPE(WindowLostFocus)
	};

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() {}

		inline std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowCloseEvent";
			return ss.str();
		}


		EVENT_CLASS_CATEGORY(ApplicationEvent)
		EVENT_CLASS_TYPE(WindowClose)
	};

	class AppTickEvent : public Event
	{
	public:
		AppTickEvent() {}

		inline std::string ToString() const override
		{
			std::stringstream ss;
			ss << "AppTickEvent";
			return ss.str();
		}


		EVENT_CLASS_CATEGORY(ApplicationEvent)
		EVENT_CLASS_TYPE(AppTick)
	};

	class AppUpdateEvent : public Event
	{
	public:
		AppUpdateEvent() {}

		inline std::string ToString() const override
		{
			std::stringstream ss;
			ss << "AppUpdateEvent";
			return ss.str();
		}


		EVENT_CLASS_CATEGORY(ApplicationEvent)
		EVENT_CLASS_TYPE(AppUpdate)
	};

	class AppRenderEvent : public Event
	{
	public:
		AppRenderEvent() {}

		inline std::string ToString() const override
		{
			std::stringstream ss;
			ss << "AppRenderEvent";
			return ss.str();
		}


		EVENT_CLASS_CATEGORY(ApplicationEvent)
		EVENT_CLASS_TYPE(AppRender)
	};
}