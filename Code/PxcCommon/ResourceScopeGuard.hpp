// SPDX-License-Identifier: Apache-2.0
// Copyright(c) 2026-Present Pen-NineCat(PenNineCat)
// https://github.com/Pen-NineCat

#pragma once
#include <exception>
#include <utility>

namespace Pxc
{
	template <typename F>
	class ResourceScopeGuard
	{
	public:
		explicit ResourceScopeGuard(F&& func) noexcept(std::is_nothrow_move_constructible_v<F>)
			: m_func(std::forward<F>(func)), m_active(true)
		{}

		~ResourceScopeGuard() noexcept
		{
			Run();
		}

		ResourceScopeGuard(const ResourceScopeGuard&) = delete;
		ResourceScopeGuard& operator=(const ResourceScopeGuard&) = delete;

		ResourceScopeGuard(ResourceScopeGuard&& other) = delete;

		ResourceScopeGuard& operator=(ResourceScopeGuard&& other) = delete;

		void Dismiss() noexcept
		{
			m_active = false;
		}

		[[nodiscard]] bool IsActive() const noexcept
		{
			return m_active;
		}
	private:
		void Run() noexcept
		{
			if (!m_active)
				return;

			m_active = false;

			try
			{
				m_func();
			}
			catch (...)
			{
				std::terminate();
			}
		}

		F m_func;
		bool m_active = false;
	};

	template <typename F>
	ResourceScopeGuard(F&&) -> ResourceScopeGuard<std::decay_t<F>>;
}
