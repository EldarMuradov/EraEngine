// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "core/memory.h"

#include "rendering/material.h"

namespace era_engine
{
	struct DXCommandList;
	struct CommonRenderData;

	template <typename key_t, typename command_header>
	struct RenderCommandBuffer
	{
	private:
		struct CommandKey
		{
			key_t key;
			void* data;
		};

		struct CommandWrapperBase
		{
			command_header header;

			virtual ~CommandWrapperBase() {}
		};

		template <typename pipeline_t, typename command_t>
		command_t& push_internal(key_t sort_key)
		{
			struct CommandWrapper : CommandWrapperBase
			{
				command_t command;
			};

			CommandWrapper* command_wrapper = arena.allocate<CommandWrapper>();
			new (command_wrapper) CommandWrapper;

			command_wrapper->header.template initialize<pipeline_t, CommandWrapper>();

			CommandKey key{};
			key.key = sort_key;
			key.data = command_wrapper;

			keys.push_back(key);
			return command_wrapper->command;
		}

		std::vector<CommandKey> keys;
		Allocator arena;

	public:
		RenderCommandBuffer()
		{
			arena.initialize(0, gb(4));
			keys.reserve(128);
		}

		NODISCARD uint64 size() const { return keys.size(); }
		void sort() { std::sort(keys.begin(), keys.end(), [](CommandKey a, CommandKey b) { return a.key < b.key; }); }

		template <typename pipeline_t, typename command_t, typename... args_t>
		command_t& emplace_back(key_t sort_key, args_t&&... args)
		{
			command_t& command = push_internal<pipeline_t, command_t>(sort_key);
			new (&command) command_t(std::forward<args_t>(args)...);
			return command;
		}

		template <typename pipeline_t, typename command_t>
		void push_back(key_t sort_key, const command_t& command_to_push)
		{
			command_t& command = push_internal<pipeline_t, command_t>(sort_key);
			new (&command) command_t(command_to_push);
		}

		template <typename pipeline_t, typename command_t>
		void push_back(key_t sort_key, command_t&& command_to_push)
		{
			command_t& command = push_internal<pipeline_t, command_t>(sort_key);
			new (&command) command_t(std::move(command_to_push));
		}

		void clear()
		{
			for (auto& key : keys)
			{
				CommandWrapperBase* wrapper_base = (CommandWrapperBase*)key.data;
				wrapper_base->~CommandWrapperBase();
			}

			arena.reset();
			keys.clear();
		}

		struct IteratorReturn : command_header
		{
			void* data;
		};

		struct iterator
		{
			typename std::vector<CommandKey>::const_iterator key_iterator;

			friend bool operator==(const iterator& a, const iterator& b) { return a.key_iterator == b.key_iterator; }
			friend bool operator!=(const iterator& a, const iterator& b) { return !(a == b); }
			iterator& operator++() { ++key_iterator; return *this; }

			IteratorReturn operator*()
			{
				CommandWrapperBase* wrapper_base = (CommandWrapperBase*)key_iterator->data;
				void* data = key_iterator->data;
				return IteratorReturn{ wrapper_base->header, data };
			}
		};

		NODISCARD iterator begin() { return iterator{ keys.begin() }; }
		NODISCARD iterator end() { return iterator{ keys.end() }; }

		NODISCARD iterator begin() const { return iterator{ keys.begin() }; }
		NODISCARD iterator end() const { return iterator{ keys.end() }; }
	};

	struct DefaultCommandHeader
	{
		typedef void (*GenericPipelineRenderFunc)(DXCommandList*, const mat4&, void*);

		template <typename pipeline_t, typename CommandWrapper>
		void initialize()
		{
			setup = pipeline_t::setup;
			render = [](DXCommandList* cl, const mat4& view_proj, void* data)
				{
					CommandWrapper* wrapper = (CommandWrapper*)data;
					pipeline_t::render(cl, view_proj, wrapper->command);
				};
		}

		PipelineSetupFunc setup;
		GenericPipelineRenderFunc render;
	};

	template <typename key_t>
	struct DefaultRenderCommandBuffer : RenderCommandBuffer<key_t, DefaultCommandHeader> {};

	struct DepthPrepassCommandHeader
	{
		typedef void (*GenericPipelineRenderFunc)(DXCommandList*, const mat4&, const mat4&, void*);

		template <typename pipeline_t, typename CommandWrapper>
		void initialize()
		{
			setup = pipeline_t::setup;
			render = [](DXCommandList* cl, const mat4& view_proj, const mat4& prev_frame_view_proj, void* data)
				{
					CommandWrapper* wrapper = (CommandWrapper*)data;
					pipeline_t::render(cl, view_proj, prev_frame_view_proj, wrapper->command);
				};
		}

		PipelineSetupFunc setup;
		GenericPipelineRenderFunc render;
	};

	template <typename key_t>
	struct DepthPrepassRenderCommandBuffer : RenderCommandBuffer<key_t, DepthPrepassCommandHeader> {};

	struct ComputeCommandHeader
	{
		typedef void (*GenericComputeFunc)(DXCommandList*, void*);

		template <typename pipeline_t, typename CommandWrapper>
		void initialize()
		{
			compute = [](DXCommandList* cl, void* data)
				{
					CommandWrapper* wrapper = (CommandWrapper*)data;
					pipeline_t::compute(cl, wrapper->command);
				};
		}

		GenericComputeFunc compute;
	};

	template <typename key_t>
	struct ComputeCommandBuffer : RenderCommandBuffer<key_t, ComputeCommandHeader> {};
}