#pragma once

#include "Core.h"
#include "Window.h"
#include "LayerStack.h"
#include "ImGui/ImGuiLayer.h"
#include "Events/ApplicationEvent.h"


namespace Debut
{
	class DBT_API Application
	{
	public:
		Application();
		
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }
		static inline Application& Get() { return *s_Instance; }
	private:
		bool OnWindowClosed(WindowCloseEvent& e);

		bool m_Running = true;

		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;

		LayerStack m_LayerStack;
		static Application* s_Instance;

		unsigned int m_VertexArray, m_VertexBuffer, m_IndexBuffer;
	};

	Application* CreateApplication();
}
