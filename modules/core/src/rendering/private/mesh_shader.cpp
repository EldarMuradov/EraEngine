// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/mesh_shader.h"
#include "rendering/material.h"
#include "rendering/render_utils.h"

#include "dx/dx_pipeline.h"
#include "dx/dx_command_list.h"
#include "dx/dx_profiling.h"

#include <fstream>

#if defined(SDK_SUPPORTS_MESH_SHADERS) && defined(MESH_SHADER_SUPPORTED)

namespace era_engine
{
	static DXPipeline meshlet_pipeline;
	static DXPipeline blob_pipeline;
	static DXPipeline koch_pipeline;

	static ref<DXBuffer> marching_cubes_buffer;

	struct MarchingCubesLookup
	{
		uint32 indices[4];
		uint8 vertices[12];
		uint16 triangle_count;
		uint16 vertex_count;
	};

	extern const MarchingCubesLookup marching_cubes_lookup[256];

	struct Subset
	{
		uint32 offset;
		uint32 count;
	};

	struct MeshShaderSubmeshInfo
	{
		uint32 first_vertex;
		uint32 num_vertices;

		uint32 first_meshlet;
		uint32 num_meshlets;

		uint32 first_unique_vertex_index;
		uint32 num_unique_vertex_indices;

		uint32 first_packed_triangle;
		uint32 num_packed_triangles;

		uint32 first_meshlet_subset;
		uint32 num_meshlet_subsets;
	};

	struct MeshShaderMesh
	{
		::std::vector<MeshShaderSubmeshInfo> submeshes;
		::std::vector<Subset> subsets;

		ref<DXBuffer> vertices;
		ref<DXBuffer> meshlets;
		ref<DXBuffer> unique_vertex_indices;
		ref<DXBuffer> primitive_indices;
	};

	static ref<MeshShaderMesh> load_mesh_shader_mesh_from_file(const char* filename);

	struct MetaBall
	{
		vec3 pos;
		vec3 dir;
		float radius;
	};

	static const uint32 DEFAULT_SHIFT = 7;
	static const uint32 DEFAULT_BALL_COUNT = 32;
	static const uint32 MAX_BALL_COUNT = 128;
	static const uint32 BALL_COUNT = DEFAULT_BALL_COUNT;
	static const uint32 SHIFT = DEFAULT_SHIFT;

	struct MeshShaderBlobRenderData
	{
		MetaBall balls[MAX_BALL_COUNT];

		MeshShaderBlobRenderData()
		{
			for (uint32 i = 0; i < BALL_COUNT; ++i)
			{
				// Random positions in [0.25, 0.75]
				balls[i].pos.x = float(rand() % RAND_MAX) / float(RAND_MAX - 1) * 0.5f + 0.25f;
				balls[i].pos.y = float(rand() % RAND_MAX) / float(RAND_MAX - 1) * 0.5f + 0.25f;
				balls[i].pos.z = float(rand() % RAND_MAX) / float(RAND_MAX - 1) * 0.5f + 0.25f;

				// Random directions in [-0.6, 0.6]
				balls[i].dir.x = (float(rand() % RAND_MAX) / float(RAND_MAX - 1) - 0.5f) * 1.2f;
				balls[i].dir.y = (float(rand() % RAND_MAX) / float(RAND_MAX - 1) - 0.5f) * 1.2f;
				balls[i].dir.z = (float(rand() % RAND_MAX) / float(RAND_MAX - 1) - 0.5f) * 1.2f;

				// Random radius in [0.02, 0.06]
				balls[i].radius = float(rand() % RAND_MAX) / float(RAND_MAX - 1) * 0.04f + 0.02f;
			}
		}
	};

	struct MeshShaderBlobPipeline
	{
		using render_data_t = MeshShaderBlobRenderData;

		PIPELINE_SETUP_DECL;
		PIPELINE_RENDER_DECL(MeshShaderBlobRenderData);
	};

	PIPELINE_SETUP_IMPL(MeshShaderBlobPipeline)
	{
		cl->set_pipeline_state(*blob_pipeline.pipeline);
		cl->set_graphics_root_signature(*blob_pipeline.root_signature);

		cl->set_graphics_dynamic_constant_buffer(1, common.camera_cbv);
		cl->set_root_graphics_srv(2, marching_cubes_buffer);

		cl->set_descriptor_heap_srv(3, 0, common.sky);
	}

	PIPELINE_RENDER_IMPL(MeshShaderBlobPipeline, MeshShaderBlobRenderData)
	{
		DX_PROFILE_BLOCK(cl, "Mesh shader blob");

		struct ConstantCB
		{
			vec4 balls[MAX_BALL_COUNT];
		};

		ConstantCB cb;

		for (uint32 i = 0; i < BALL_COUNT; ++i)
		{
			float radius = data.balls[i].radius;
			cb.balls[i] = vec4(data.balls[i].pos, radius * radius);
		}

		auto b = dx_context.upload_dynamic_constant_buffer(cb);

		cl->set_graphics_dynamic_constant_buffer(0, b);


		const uint32 grid_size = (1 << SHIFT);
		cl->dispatch_mesh((grid_size / 4) * (grid_size / 4) * (grid_size / 4), 1, 1);
	};

	struct MeshShaderKochPipeline
	{
		PIPELINE_SETUP_DECL;
		PIPELINE_RENDER_DECL(void*);
	};

	PIPELINE_SETUP_IMPL(MeshShaderKochPipeline)
	{
		cl->set_pipeline_state(*koch_pipeline.pipeline);
		cl->set_graphics_root_signature(*koch_pipeline.root_signature);

		cl->set_graphics_dynamic_constant_buffer(0, common.camera_cbv);
		cl->set_root_graphics_srv(1, marching_cubes_buffer);

		cl->set_descriptor_heap_srv(2, 0, common.sky);
	}

	PIPELINE_RENDER_IMPL(MeshShaderKochPipeline, void*)
	{
		DX_PROFILE_BLOCK(cl, "Mesh shader koch");

		const uint32 grid_size = (1 << SHIFT);
		cl->dispatch_mesh((grid_size / 4) * (grid_size / 4) * (grid_size / 4), 1, 1);
	};

	void initialize_mesh_shader()
	{
		D3D12_RT_FORMAT_ARRAY render_target_format = {};
		render_target_format.NumRenderTargets = 1;
		render_target_format.RTFormats[0] = hdr_format;

		{
			struct PipelineStateStream : DXPipelineStreamBase
			{
				// Will be set by reloader
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE root_signature;
				CD3DX12_PIPELINE_STATE_STREAM_MS ms;
				CD3DX12_PIPELINE_STATE_STREAM_PS ps;

				// Initialized here
				CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsv_format;
				CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtv_formats;

				void set_root_signature(DXRootSignature rs) override { root_signature = rs.root_signature.Get(); }
				void set_mesh_shader(DXBlob blob) override { ms = CD3DX12_SHADER_BYTECODE(blob.Get()); }
				void set_pixel_shader(DXBlob blob) override { ps = CD3DX12_SHADER_BYTECODE(blob.Get()); }
			};

			PipelineStateStream stream;
			stream.dsv_format = depth_stencil_format;
			stream.rtv_formats = render_target_format;

			GraphicsPipelineFiles files = {};
			files.ms = "meshlet_ms";
			files.ps = "mesh_shader_ps";
			meshlet_pipeline = create_reloadable_pipeline_from_stream(stream, files, RS_IN_MESH_SHADER);
		}
		{
			struct PipelineStateStream : DXPipelineStreamBase
			{
				// Will be set by reloader
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE root_signature;
				CD3DX12_PIPELINE_STATE_STREAM_AS as;
				CD3DX12_PIPELINE_STATE_STREAM_MS ms;
				CD3DX12_PIPELINE_STATE_STREAM_PS ps;

				// Initialized here
				CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsv_format;
				CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtv_formats;
				CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rasterizer;

				void set_root_signature(DXRootSignature rs) override { root_signature = rs.root_signature.Get(); }
				void set_amplification_shader(DXBlob blob) override { as = CD3DX12_SHADER_BYTECODE(blob.Get()); }
				void set_mesh_shader(DXBlob blob) override { ms = CD3DX12_SHADER_BYTECODE(blob.Get()); }
				void set_pixel_shader(DXBlob blob) override { ps = CD3DX12_SHADER_BYTECODE(blob.Get()); }
			};

			auto rasterizer_desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			//rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
			rasterizer_desc.FrontCounterClockwise = TRUE; // Righthanded coordinate system

			PipelineStateStream stream;
			stream.dsv_format = depth_stencil_format;
			stream.rtv_formats = render_target_format;
			stream.rasterizer = rasterizer_desc;

			GraphicsPipelineFiles files = {};
			files.as = "meta_ball_as";
			files.ms = "meta_ball_ms";
			files.ps = "glass_ps";
			blob_pipeline = create_reloadable_pipeline_from_stream(stream, files, RS_IN_MESH_SHADER);

			files.as = "koch_as";
			files.ms = "koch_ms";
			koch_pipeline = create_reloadable_pipeline_from_stream(stream, files, RS_IN_MESH_SHADER);


			marching_cubes_buffer = create_buffer(sizeof(MarchingCubesLookup), arraysize(marching_cubes_lookup), (void*)marching_cubes_lookup);
		}
	}

	void test_render_mesh_shader(TransparentRenderPass* ldr_render_pass, float dt)
	{
		/*overlayRenderPass->renderObjectWithMeshShader(1, 1, 1,
			cubeMaterial,
			createModelMatrix(vec3(0.f, 30.f, 0.f), quat::identity, 1.f),
			true
		);*/

		/*if (meshMaterial->mesh)
		{
			auto& sm = meshMaterial->mesh->submeshes[0];
			overlayRenderPass->renderObjectWithMeshShader(sm.numMeshlets, 1, 1,
				meshMaterial,
				createModelMatrix(vec3(0.f, 30.f, 0.f), quat::identity, 0.3f),
				true
			);
		}*/

		static MeshShaderBlobRenderData blob_data;

		const float animation_speed = 0.25f;
		const float frame_time = animation_speed * dt;
		for (uint32 i = 0; i < BALL_COUNT; ++i)
		{
			vec3 d = vec3(0.5f, 0.5f, 0.5f) - blob_data.balls[i].pos;
			blob_data.balls[i].dir += d * (5.f * frame_time / (2.f + dot(d, d)));
			blob_data.balls[i].pos += blob_data.balls[i].dir * frame_time;
		}

		ldr_render_pass->render_object<MeshShaderBlobPipeline>(blob_data);
		//ldrRenderPass->renderObject<mesh_shader_koch_pipeline>(0);
	}

	enum AttributeType
	{
		ATTRIBUTE_TYPE_POSITION,
		ATTRIBUTE_TYPE_NORMAL,
		ATTRIBUTE_TYPE_TEX_COORD,
		ATTRIBUTE_TYPE_TANGENT,
		ATTRIBUTE_TYPE_BITANGENT,
		ATTRIBUTE_TYPE_COUNT,
	};

	struct MeshHeader
	{
		uint32 indices;
		uint32 index_subsets;
		uint32 attributes[ATTRIBUTE_TYPE_COUNT];

		uint32 meshlets;
		uint32 meshlet_subsets;
		uint32 unique_vertex_indices;
		uint32 primitive_indices;
		uint32 cull_data;
	};

	struct BufferView
	{
		uint32 offset;
		uint32 size;
	};

	struct BufferAccessor
	{
		uint32 buffer_view;
		uint32 offset;
		uint32 size;
		uint32 stride;
		uint32 count;
	};

	struct FileHeader
	{
		uint32 prolog;
		uint32 version;

		uint32 mesh_count;
		uint32 accessor_count;
		uint32 buffer_view_count;
		uint32 buffer_size;
	};

	enum FileVersion
	{
		FILE_VERSION_INITIAL = 0,
		CURRENT_FILE_VERSION = FILE_VERSION_INITIAL
	};

	// Meshlet stuff

	struct MeshletInfo
	{
		uint32 num_vertices;
		uint32 first_vertex;
		uint32 num_primitives;
		uint32 first_primitive;
	};

	struct PackedTriangle
	{
		uint32 i0 : 10;
		uint32 i1 : 10;
		uint32 i2 : 10;
	};

	struct MeshVertex
	{
		vec3 position;
		vec3 normal;
	};

	static ref<MeshShaderMesh> load_mesh_shader_mesh_from_file(const char* filename)
	{
		std::ifstream stream(filename, std::ios::binary);
		if (!stream.is_open())
		{
			std::cerr << "Could not find file '" << filename << "'.\n";
			return 0;
		}

		std::vector<MeshHeader> mesh_headers;
		std::vector<BufferView> buffer_views;
		std::vector<BufferAccessor> accessors;

		FileHeader header;
		stream.read((char*)&header, sizeof(header));

		const uint32 prolog = 'MSHL';
		if (header.prolog != prolog)
		{
			return 0; // Incorrect file format
		}

		if (header.version != CURRENT_FILE_VERSION)
		{
			return 0; // Version mismatch between export and import serialization code
		}

		// Read mesh metdata
		mesh_headers.resize(header.mesh_count);
		stream.read((char*)mesh_headers.data(), mesh_headers.size() * sizeof(mesh_headers[0]));

		accessors.resize(header.accessor_count);
		stream.read((char*)accessors.data(), accessors.size() * sizeof(accessors[0]));

		buffer_views.resize(header.buffer_view_count);
		stream.read((char*)buffer_views.data(), buffer_views.size() * sizeof(buffer_views[0]));

		std::vector<uint8> m_buffer;
		m_buffer.resize(header.buffer_size);
		stream.read((char*)m_buffer.data(), header.buffer_size);

		char eofbyte;
		stream.read(&eofbyte, 1); // Read last byte to hit the eof bit.
		ASSERT(stream.eof()); // There's a problem if we didn't completely consume the file contents..

		stream.close();

		std::vector<MeshShaderSubmeshInfo> submeshes(mesh_headers.size());

		//std::vector<uint32> indices;
		//std::vector<subset> indexSubsets;
		std::vector<MeshVertex> vertices;
		std::vector<MeshletInfo> meshlets;
		std::vector<uint32> unique_vertex_indices;
		std::vector<PackedTriangle> primitive_indices;
		std::vector<Subset> meshlet_subsets;

		for (uint32_t i = 0; i < (uint32)mesh_headers.size(); ++i)
		{
			MeshHeader& mesh_view = mesh_headers[i];
			MeshShaderSubmeshInfo& sm = submeshes[i];

#if 0
			// Index data
			{
				BufferAccessor& accessor = accessors[mesh_view.indices];
				BufferView& buffer_view = buffer_views[accessor.buffer_view];

				ASSERT(accessor.size == sizeof(uint32));
				ASSERT(accessor.count * accessor.size == buffer_view.size);

				uint32* start = (uint32*)(m_buffer.data() + buffer_view.offset);

				sm.first_index = (uint32)indices.size();
				sm.num_indices = accessor.count;

				indices.insert(indices.end(), start, start + sm.num_indices);
			}
#endif

#if 0
			// Index Subset data
			{
				BufferAccessor& accessor = accessors[mesh_view.index_subsets];
				BufferView& buffer_view = buffer_views[accessor.buffer_view];

				ASSERT(accessor.count * accessor.size == buffer_view.size);

				subset* start = (subset*)(m_buffer.data() + buffer_view.offset);

				sm.first_index_subset = (uint32)index_subsets.size();
				sm.numIndexSubsets = accessor.count;

				index_subsets.insert(index_subsets.end(), start, start + sm.numIndexSubsets);
			}
#endif

			// Vertex data & layout metadata

			bool first = true;

			for (uint32 j = 0; j < ATTRIBUTE_TYPE_COUNT; ++j)
			{
				if (mesh_view.attributes[j] == -1)
					continue;

				BufferAccessor& accessor = accessors[mesh_view.attributes[j]];

				BufferView& buffer_view = buffer_views[accessor.buffer_view];

				if (first)
				{
					sm.first_vertex = (uint32)vertices.size();
					sm.num_vertices = accessor.count;
					vertices.resize(vertices.size() + sm.num_vertices);
					first = false;
				}
				else
				{
					ASSERT(sm.num_vertices == accessor.count);
				}

				uint8* data = m_buffer.data() + buffer_view.offset + accessor.offset;

				for (uint32 vertex_id = 0; vertex_id < sm.num_vertices; ++vertex_id)
				{
					MeshVertex& v = vertices[sm.first_vertex + vertex_id];

					uint8* attribute_data = data + accessor.stride * vertex_id;

					if (j == ATTRIBUTE_TYPE_POSITION)
					{
						v.position = *(vec3*)attribute_data;
					}
					else if (j == ATTRIBUTE_TYPE_NORMAL)
					{
						v.normal = *(vec3*)attribute_data;
					}
				}
			}

			// Meshlet data
			{
				BufferAccessor& accessor = accessors[mesh_view.meshlets];
				BufferView& buffer_view = buffer_views[accessor.buffer_view];

				MeshletInfo* start = (MeshletInfo*)(m_buffer.data() + buffer_view.offset);

				ASSERT(accessor.count * accessor.size == buffer_view.size);

				sm.first_meshlet = (uint32)meshlets.size();
				sm.num_meshlets = accessor.count;

				meshlets.insert(meshlets.end(), start, start + sm.num_meshlets);
			}

			// Meshlet Subset data
			{
				BufferAccessor& accessor = accessors[mesh_view.meshlet_subsets];
				BufferView& buffer_view = buffer_views[accessor.buffer_view];

				Subset* start = (Subset*)(m_buffer.data() + buffer_view.offset);

				ASSERT(accessor.count * accessor.size == buffer_view.size);

				sm.first_meshlet_subset = (uint32)meshlet_subsets.size();
				sm.num_meshlet_subsets = accessor.count;

				meshlet_subsets.insert(meshlet_subsets.end(), start, start + sm.num_meshlet_subsets);
			}

			// Unique Vertex Index data
			{
				BufferAccessor& accessor = accessors[mesh_view.unique_vertex_indices];
				BufferView& buffer_view = buffer_views[accessor.buffer_view];

				ASSERT(accessor.count * accessor.size == buffer_view.size);

				sm.first_unique_vertex_index = (uint32)unique_vertex_indices.size();
				sm.num_unique_vertex_indices = accessor.count;

				if (accessor.size == sizeof(uint32))
				{
					uint32* start = (uint32*)(m_buffer.data() + buffer_view.offset);
					unique_vertex_indices.insert(unique_vertex_indices.end(), start, start + sm.num_unique_vertex_indices);
				}
				else
				{
					ASSERT(accessor.size == sizeof(uint16));

					uint16* start = (uint16*)(m_buffer.data() + buffer_view.offset);

					std::vector<uint16> temp;
					temp.insert(temp.end(), start, start + sm.num_unique_vertex_indices);

					for (uint16 t : temp)
					{
						unique_vertex_indices.push_back((uint32)t);
					}
				}
			}

			// Primitive Index data
			{
				BufferAccessor& accessor = accessors[mesh_view.primitive_indices];
				BufferView& buffer_view = buffer_views[accessor.buffer_view];

				PackedTriangle* start = (PackedTriangle*)(m_buffer.data() + buffer_view.offset);

				ASSERT(accessor.count * accessor.size == buffer_view.size);

				sm.first_packed_triangle = (uint32)primitive_indices.size();
				sm.num_packed_triangles = accessor.count;

				primitive_indices.insert(primitive_indices.end(), start, start + sm.num_packed_triangles);
			}

#if 0
			// Cull data
			{
				BufferAccessor& accessor = accessors[mesh_view.CullData];
				BufferView& buffer_view = buffer_views[accessor.BufferView];

				mesh.CullingData = MakeSpan(reinterpret_cast<CullData*>(m_buffer.data() + buffer_view.Offset), accessor.Count);
			}
#endif
		}

#if 0
		// Build bounding spheres for each mesh
		for (uint32_t i = 0; i < static_cast<uint32_t>(m_meshes.size()); ++i)
		{
			auto& m = m_meshes[i];

			uint32_t vbIndexPos = 0;

			// Find the index of the vertex buffer of the position attribute
			for (uint32_t j = 1; j < m.LayoutDesc.NumElements; ++j)
			{
				auto& desc = m.LayoutElems[j];
				if (strcmp(desc.SemanticName, "POSITION") == 0)
				{
					vbIndexPos = j;
					break;
				}
			}

			// Find the byte offset of the position attribute with its vertex buffer
			uint32_t position_offset = 0;

			for (uint32_t j = 0; j < m.LayoutDesc.NumElements; ++j)
			{
				auto& desc = m.LayoutElems[j];
				if (strcmp(desc.SemanticName, "POSITION") == 0)
				{
					break;
				}

				if (desc.InputSlot == vbIndexPos)
				{
					position_offset += GetFormatSize(m.LayoutElems[j].Format);
				}
			}

			XMFLOAT3* v0 = reinterpret_cast<XMFLOAT3*>(m.Vertices[vbIndexPos].data() + position_offset);
			uint32_t stride = m.VertexStrides[vbIndexPos];

			BoundingSphere::CreateFromPoints(m.BoundingSphere, m.VertexCount, v0, stride);

			if (i == 0)
			{
				m_boundingSphere = m.BoundingSphere;
			}
			else
			{
				BoundingSphere::CreateMerged(m_boundingSphere, m_boundingSphere, m.BoundingSphere);
			}
		}
#endif

		ref<MeshShaderMesh> result = make_ref<MeshShaderMesh>();
		result->submeshes = std::move(submeshes);
		result->subsets = std::move(meshlet_subsets);
		result->vertices = create_buffer(sizeof(MeshVertex), (uint32)vertices.size(), vertices.data());
		result->meshlets = create_buffer(sizeof(MeshletInfo), (uint32)meshlets.size(), meshlets.data());
		result->unique_vertex_indices = create_buffer(sizeof(uint32), (uint32)unique_vertex_indices.size(), unique_vertex_indices.data());
		result->primitive_indices = create_buffer(sizeof(PackedTriangle), (uint32)primitive_indices.size(), primitive_indices.data());

		return result;
	}

	const MarchingCubesLookup marching_cubes_lookup[256] =
	{
		/*  Packed 8bit indices                                 Two 3bit (octal) corner indices defining the edge                 */
		{ { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, { 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 0, 0 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 010, 020, 040, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 010, 051, 031, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x03020100, 0x00000200, 0x00000000, 0x00000000 }, { 031, 020, 040, 051, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 020, 032, 062, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x03020100, 0x00000200, 0x00000000, 0x00000000 }, { 010, 032, 062, 040, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 031, 010, 051, 032, 062, 020, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 031, 032, 062, 051, 040, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 031, 073, 032, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 010, 020, 040, 031, 073, 032, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x03020100, 0x00000200, 0x00000000, 0x00000000 }, { 051, 073, 032, 010, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 032, 020, 040, 073, 051, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x00000200, 0x00000000, 0x00000000 }, { 020, 031, 073, 062, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 010, 031, 073, 040, 062, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 020, 010, 051, 062, 073, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x01020100, 0x00000203, 0x00000000, 0x00000000 }, { 051, 073, 040, 062, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 054, 040, 064, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x03020100, 0x00000200, 0x00000000, 0x00000000 }, { 054, 010, 020, 064, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 010, 051, 031, 040, 064, 054, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 054, 051, 031, 064, 020, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 040, 064, 054, 020, 032, 062, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 062, 064, 054, 032, 010, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x07060504, 0x00000008, 0x00000000 }, { 051, 031, 010, 040, 064, 054, 032, 062, 020, 000, 000, 000 }, 3, 9 },
		{ { 0x03020100, 0x04030001, 0x04050301, 0x00000000 }, { 054, 062, 064, 051, 032, 031, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 031, 073, 032, 040, 064, 054, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 020, 064, 054, 010, 031, 073, 032, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 051, 073, 032, 010, 040, 064, 054, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x00020100, 0x04000103, 0x01050303, 0x00000000 }, { 032, 051, 073, 064, 020, 054, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 020, 031, 073, 062, 064, 054, 040, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x00020100, 0x03000302, 0x02030504, 0x00000000 }, { 031, 073, 062, 054, 010, 064, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x06030504, 0x05070404, 0x00000000 }, { 054, 040, 064, 051, 062, 010, 073, 020, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 054, 062, 064, 051, 073, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 051, 054, 075, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 051, 054, 075, 010, 020, 040, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x03020100, 0x00000200, 0x00000000, 0x00000000 }, { 010, 054, 075, 031, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 040, 054, 075, 020, 031, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 051, 054, 075, 032, 062, 020, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 010, 032, 062, 040, 054, 075, 051, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 010, 054, 075, 031, 032, 062, 020, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x00020100, 0x04000103, 0x03010503, 0x00000000 }, { 032, 075, 031, 040, 062, 054, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 031, 073, 032, 051, 054, 075, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x03020100, 0x07060504, 0x00000008, 0x00000000 }, { 020, 040, 010, 031, 073, 032, 054, 075, 051, 000, 000, 000 }, 3, 9 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 075, 073, 032, 054, 010, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x04030001, 0x04050301, 0x00000000 }, { 032, 075, 073, 020, 054, 040, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 073, 062, 020, 031, 051, 054, 075, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x03020100, 0x04050504, 0x07060506, 0x00000000 }, { 054, 075, 051, 010, 031, 040, 073, 062, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x04000103, 0x01050303, 0x00000000 }, { 075, 010, 054, 062, 073, 020, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 075, 040, 054, 073, 062, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x00000200, 0x00000000, 0x00000000 }, { 051, 040, 064, 075, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 051, 010, 020, 075, 064, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 010, 040, 064, 031, 075, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x01020100, 0x00000203, 0x00000000, 0x00000000 }, { 031, 020, 075, 064, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 064, 075, 051, 040, 020, 032, 062, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x00020100, 0x04000103, 0x01050303, 0x00000000 }, { 051, 064, 075, 032, 010, 062, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x04050504, 0x07060506, 0x00000000 }, { 032, 062, 020, 010, 040, 031, 064, 075, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 062, 031, 032, 064, 075, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 051, 040, 064, 075, 073, 032, 031, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x03020100, 0x04050504, 0x07060506, 0x00000000 }, { 073, 032, 031, 051, 010, 075, 020, 064, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x04000103, 0x03010503, 0x00000000 }, { 040, 032, 010, 075, 064, 073, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 032, 075, 073, 020, 064, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x01020100, 0x05040203, 0x05070406, 0x00000000 }, { 051, 040, 075, 064, 073, 020, 031, 062, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 075, 010, 064, 051, 062, 031, 073, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 062, 010, 073, 020, 075, 040, 064, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x03020100, 0x00000001, 0x00000000, 0x00000000 }, { 062, 075, 073, 064, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 064, 062, 076, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 020, 040, 010, 062, 076, 064, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 010, 051, 031, 062, 076, 064, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 040, 051, 031, 020, 062, 076, 064, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x03020100, 0x00000200, 0x00000000, 0x00000000 }, { 064, 020, 032, 076, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 064, 040, 010, 076, 032, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 032, 076, 064, 020, 010, 051, 031, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x00020100, 0x03000302, 0x05020304, 0x00000000 }, { 031, 032, 076, 040, 051, 064, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 073, 032, 031, 076, 064, 062, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x03020100, 0x07060504, 0x00000008, 0x00000000 }, { 031, 073, 032, 020, 040, 010, 076, 064, 062, 000, 000, 000 }, 3, 9 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 032, 010, 051, 073, 076, 064, 062, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x03020100, 0x04050504, 0x07060506, 0x00000000 }, { 076, 064, 062, 032, 020, 073, 040, 051, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 073, 076, 064, 031, 020, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x02030200, 0x05040304, 0x00000000 }, { 073, 076, 064, 031, 040, 010, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x04000103, 0x03010503, 0x00000000 }, { 010, 064, 020, 073, 051, 076, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 064, 073, 076, 040, 051, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x00000200, 0x00000000, 0x00000000 }, { 076, 054, 040, 062, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 020, 062, 076, 010, 054, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 040, 062, 076, 054, 051, 031, 010, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x00020100, 0x04000103, 0x03010503, 0x00000000 }, { 051, 076, 054, 020, 031, 062, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 040, 020, 032, 054, 076, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x02020100, 0x00000301, 0x00000000, 0x00000000 }, { 010, 032, 054, 076, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x03020100, 0x06030504, 0x05070404, 0x00000000 }, { 031, 010, 051, 032, 054, 020, 076, 040, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 031, 054, 051, 032, 076, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 076, 054, 040, 062, 032, 031, 073, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x03020100, 0x04050504, 0x07060506, 0x00000000 }, { 031, 073, 032, 020, 062, 010, 076, 054, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x05040302, 0x07050606, 0x00000000 }, { 054, 040, 062, 076, 010, 051, 032, 073, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 073, 020, 051, 032, 054, 062, 076, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x03000302, 0x05020304, 0x00000000 }, { 040, 020, 031, 076, 054, 073, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 073, 010, 031, 076, 054, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 054, 020, 076, 040, 073, 010, 051, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x03020100, 0x00000001, 0x00000000, 0x00000000 }, { 073, 054, 051, 076, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 054, 075, 051, 064, 062, 076, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x03020100, 0x07060504, 0x00000008, 0x00000000 }, { 010, 020, 040, 054, 075, 051, 062, 076, 064, 000, 000, 000 }, 3, 9 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 075, 031, 010, 054, 064, 062, 076, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x03020100, 0x04050504, 0x07060506, 0x00000000 }, { 062, 076, 064, 040, 054, 020, 075, 031, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 064, 020, 032, 076, 075, 051, 054, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x03020100, 0x06030504, 0x05070404, 0x00000000 }, { 051, 054, 075, 010, 076, 040, 032, 064, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x05040302, 0x07050606, 0x00000000 }, { 020, 032, 076, 064, 031, 010, 075, 054, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 076, 040, 032, 064, 031, 054, 075, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x03020100, 0x07060504, 0x00000008, 0x00000000 }, { 051, 054, 075, 073, 032, 031, 064, 062, 076, 000, 000, 000 }, 3, 9 },
		{ { 0x03020100, 0x07060504, 0x0B0A0908, 0x00000000 }, { 076, 064, 062, 031, 073, 032, 010, 020, 040, 054, 075, 051 }, 4, 12},
		{ { 0x03020100, 0x04050504, 0x07060506, 0x00000000 }, { 064, 062, 076, 075, 073, 054, 032, 010, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x03000302, 0x03040504, 0x00080706 }, { 020, 040, 054, 075, 032, 073, 062, 076, 064, 000, 000, 000 }, 5, 9 },
		{ { 0x03020100, 0x04050504, 0x07060506, 0x00000000 }, { 051, 054, 075, 073, 076, 031, 064, 020, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x03000302, 0x03040504, 0x00080706 }, { 031, 073, 076, 064, 010, 040, 051, 054, 075, 000, 000, 000 }, 5, 9 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 054, 073, 010, 075, 020, 076, 064, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x01040103, 0x03010505, 0x00000000 }, { 064, 073, 076, 040, 075, 054, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 076, 075, 051, 062, 040, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x02030200, 0x05040304, 0x00000000 }, { 020, 062, 076, 010, 075, 051, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x03000302, 0x05020304, 0x00000000 }, { 010, 040, 062, 075, 031, 076, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 076, 020, 062, 075, 031, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x03000302, 0x02030504, 0x00000000 }, { 075, 051, 040, 032, 076, 020, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 051, 076, 075, 010, 032, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 031, 040, 075, 010, 076, 020, 032, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x03020100, 0x00000001, 0x00000000, 0x00000000 }, { 031, 076, 075, 032, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x03020100, 0x06030504, 0x05070404, 0x00000000 }, { 031, 073, 032, 051, 062, 075, 040, 076, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x03000302, 0x03040504, 0x00080706 }, { 010, 020, 062, 076, 051, 075, 031, 073, 032, 000, 000, 000 }, 5, 9 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 062, 075, 040, 076, 010, 073, 032, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x01040103, 0x03010505, 0x00000000 }, { 076, 020, 062, 075, 032, 073, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 031, 076, 020, 073, 040, 075, 051, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x01040103, 0x03010505, 0x00000000 }, { 073, 010, 031, 076, 051, 075, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 010, 040, 020, 075, 073, 076, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 073, 076, 075, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 073, 075, 076, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 010, 020, 040, 075, 076, 073, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 051, 031, 010, 075, 076, 073, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 031, 020, 040, 051, 075, 076, 073, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 032, 062, 020, 073, 075, 076, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 062, 040, 010, 032, 073, 075, 076, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x03020100, 0x07060504, 0x00000008, 0x00000000 }, { 010, 051, 031, 032, 062, 020, 075, 076, 073, 000, 000, 000 }, 3, 9 },
		{ { 0x03020100, 0x04050504, 0x07060506, 0x00000000 }, { 075, 076, 073, 031, 032, 051, 062, 040, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x03020100, 0x00000200, 0x00000000, 0x00000000 }, { 031, 075, 076, 032, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 031, 075, 076, 032, 020, 040, 010, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 051, 075, 076, 010, 032, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x04000103, 0x03010503, 0x00000000 }, { 075, 040, 051, 032, 076, 020, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 076, 062, 020, 075, 031, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x04000103, 0x01050303, 0x00000000 }, { 010, 062, 040, 075, 031, 076, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x04030001, 0x04050301, 0x00000000 }, { 020, 076, 062, 010, 075, 051, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 076, 051, 075, 062, 040, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 075, 076, 073, 054, 040, 064, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 054, 010, 020, 064, 076, 073, 075, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x03020100, 0x07060504, 0x00000008, 0x00000000 }, { 031, 010, 051, 075, 076, 073, 040, 064, 054, 000, 000, 000 }, 3, 9 },
		{ { 0x03020100, 0x06030504, 0x05070404, 0x00000000 }, { 073, 075, 076, 031, 064, 051, 020, 054, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x03020100, 0x07060504, 0x00000008, 0x00000000 }, { 020, 032, 062, 064, 054, 040, 073, 075, 076, 000, 000, 000 }, 3, 9 },
		{ { 0x03020100, 0x06030504, 0x05070404, 0x00000000 }, { 075, 076, 073, 054, 032, 064, 010, 062, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x03020100, 0x07060504, 0x0B0A0908, 0x00000000 }, { 010, 051, 031, 054, 040, 064, 032, 062, 020, 075, 076, 073 }, 4, 12},
		{ { 0x00020100, 0x03000302, 0x03040504, 0x00080706 }, { 051, 031, 032, 062, 054, 064, 075, 076, 073, 000, 000, 000 }, 5, 9 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 076, 032, 031, 075, 054, 040, 064, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x01020100, 0x05040203, 0x05070406, 0x00000000 }, { 031, 075, 032, 076, 020, 054, 010, 064, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x03020100, 0x04050504, 0x07060506, 0x00000000 }, { 040, 064, 054, 051, 075, 010, 076, 032, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 064, 051, 020, 054, 032, 075, 076, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x03020100, 0x06030504, 0x05070404, 0x00000000 }, { 040, 064, 054, 020, 075, 062, 031, 076, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 075, 062, 031, 076, 010, 064, 054, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x03000302, 0x03040504, 0x00080706 }, { 010, 051, 075, 076, 020, 062, 040, 064, 054, 000, 000, 000 }, 5, 9 },
		{ { 0x00020100, 0x01040103, 0x03010505, 0x00000000 }, { 076, 051, 075, 062, 054, 064, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x00000200, 0x00000000, 0x00000000 }, { 073, 051, 054, 076, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 054, 076, 073, 051, 010, 020, 040, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 073, 031, 010, 076, 054, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x04000103, 0x01050303, 0x00000000 }, { 040, 031, 020, 076, 054, 073, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 073, 051, 054, 076, 062, 020, 032, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x01020100, 0x05040203, 0x05070406, 0x00000000 }, { 010, 032, 040, 062, 054, 073, 051, 076, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x03020100, 0x06030504, 0x05070404, 0x00000000 }, { 020, 032, 062, 010, 076, 031, 054, 073, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 076, 031, 054, 073, 040, 032, 062, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 031, 051, 054, 032, 076, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x04050504, 0x07060506, 0x00000000 }, { 020, 040, 010, 031, 051, 032, 054, 076, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x01020100, 0x00000203, 0x00000000, 0x00000000 }, { 010, 054, 032, 076, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 040, 032, 020, 054, 076, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x03000302, 0x02030504, 0x00000000 }, { 051, 054, 076, 020, 031, 062, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 040, 031, 062, 010, 076, 051, 054, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 020, 076, 062, 010, 054, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x00000001, 0x00000000, 0x00000000 }, { 076, 040, 054, 062, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 064, 076, 073, 040, 051, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x03000302, 0x02030504, 0x00000000 }, { 010, 020, 064, 073, 051, 076, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x04030001, 0x04050301, 0x00000000 }, { 073, 064, 076, 031, 040, 010, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 073, 064, 076, 031, 020, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x06030504, 0x05070404, 0x00000000 }, { 032, 062, 020, 073, 040, 076, 051, 064, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 032, 064, 010, 062, 051, 076, 073, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x03000302, 0x03040504, 0x00080706 }, { 031, 010, 040, 064, 073, 076, 032, 062, 020, 000, 000, 000 }, 5, 9 },
		{ { 0x00020100, 0x01040103, 0x03010505, 0x00000000 }, { 062, 031, 032, 064, 073, 076, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x04000103, 0x01050303, 0x00000000 }, { 031, 076, 032, 040, 051, 064, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 032, 051, 076, 031, 064, 010, 020, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 064, 010, 040, 076, 032, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x00000001, 0x00000000, 0x00000000 }, { 064, 032, 020, 076, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 040, 076, 051, 064, 031, 062, 020, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 010, 031, 051, 062, 064, 076, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x00020100, 0x01040103, 0x03010505, 0x00000000 }, { 064, 010, 040, 076, 020, 062, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 064, 076, 062, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x03020100, 0x00000200, 0x00000000, 0x00000000 }, { 062, 073, 075, 064, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 062, 073, 075, 064, 040, 010, 020, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x00020100, 0x05040302, 0x00000006, 0x00000000 }, { 075, 064, 062, 073, 031, 010, 051, 000, 000, 000, 000, 000 }, 3, 7 },
		{ { 0x00020100, 0x05040302, 0x07050606, 0x00000000 }, { 073, 075, 064, 062, 051, 031, 040, 020, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 032, 073, 075, 020, 064, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x03000302, 0x02030504, 0x00000000 }, { 040, 010, 032, 075, 064, 073, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x06030504, 0x05070404, 0x00000000 }, { 051, 031, 010, 075, 020, 073, 064, 032, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 051, 032, 040, 031, 064, 073, 075, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 062, 032, 031, 064, 075, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x06030504, 0x05070404, 0x00000000 }, { 010, 020, 040, 031, 064, 032, 075, 062, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x03000302, 0x05020304, 0x00000000 }, { 051, 075, 064, 032, 010, 062, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 064, 032, 075, 062, 051, 020, 040, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x02020100, 0x00000301, 0x00000000, 0x00000000 }, { 031, 075, 020, 064, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 010, 064, 040, 031, 075, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 051, 020, 010, 075, 064, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x00000001, 0x00000000, 0x00000000 }, { 051, 064, 040, 075, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 075, 054, 040, 073, 062, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x03000302, 0x05020304, 0x00000000 }, { 075, 054, 010, 062, 073, 020, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x06030504, 0x05070404, 0x00000000 }, { 010, 051, 031, 040, 073, 054, 062, 075, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 073, 054, 062, 075, 020, 051, 031, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x03020100, 0x02030200, 0x05040304, 0x00000000 }, { 032, 073, 075, 020, 054, 040, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 075, 032, 073, 054, 010, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x03000302, 0x03040504, 0x00080706 }, { 020, 032, 073, 075, 040, 054, 010, 051, 031, 000, 000, 000 }, 5, 9 },
		{ { 0x00020100, 0x01040103, 0x03010505, 0x00000000 }, { 075, 032, 073, 054, 031, 051, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x03000302, 0x02030504, 0x00000000 }, { 032, 031, 075, 040, 062, 054, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 010, 062, 054, 020, 075, 032, 031, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 010, 075, 032, 051, 062, 054, 040, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 051, 075, 054, 032, 020, 062, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 040, 075, 054, 020, 031, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x00000001, 0x00000000, 0x00000000 }, { 010, 075, 054, 031, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x01040103, 0x03010505, 0x00000000 }, { 040, 075, 054, 020, 051, 010, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 051, 075, 054, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x00020100, 0x02030302, 0x00000004, 0x00000000 }, { 054, 064, 062, 051, 073, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x04050504, 0x07060506, 0x00000000 }, { 010, 020, 040, 054, 064, 051, 062, 073, 000, 000, 000, 000 }, 4, 8 },
		{ { 0x00020100, 0x04000103, 0x03010503, 0x00000000 }, { 031, 062, 073, 054, 010, 064, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 020, 054, 031, 040, 073, 064, 062, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x03000302, 0x05020304, 0x00000000 }, { 032, 073, 051, 064, 020, 054, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 051, 064, 073, 054, 032, 040, 010, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x00020100, 0x01020103, 0x01060504, 0x00060104 }, { 020, 073, 064, 032, 054, 031, 010, 000, 000, 000, 000, 000 }, 5, 7 },
		{ { 0x03020100, 0x00000504, 0x00000000, 0x00000000 }, { 031, 032, 073, 040, 054, 064, 000, 000, 000, 000, 000, 000 }, 2, 6 },
		{ { 0x03020100, 0x02030200, 0x05040304, 0x00000000 }, { 054, 064, 062, 051, 032, 031, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x03000302, 0x03040504, 0x00080706 }, { 051, 054, 064, 062, 031, 032, 010, 020, 040, 000, 000, 000 }, 5, 9 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 062, 054, 064, 032, 010, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x01040103, 0x03010505, 0x00000000 }, { 062, 054, 064, 032, 040, 020, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 054, 031, 051, 064, 020, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x01040103, 0x03010505, 0x00000000 }, { 054, 031, 051, 064, 010, 040, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x00000001, 0x00000000, 0x00000000 }, { 054, 020, 010, 064, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 054, 064, 040, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x02020100, 0x00000301, 0x00000000, 0x00000000 }, { 051, 040, 073, 062, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 020, 051, 010, 062, 073, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 010, 073, 031, 040, 062, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x00000001, 0x00000000, 0x00000000 }, { 020, 073, 031, 062, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 032, 040, 020, 073, 051, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x03020100, 0x00000001, 0x00000000, 0x00000000 }, { 051, 032, 073, 010, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x01040103, 0x03010505, 0x00000000 }, { 032, 040, 020, 073, 010, 031, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 031, 032, 073, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x00020100, 0x04030103, 0x00000001, 0x00000000 }, { 031, 062, 032, 051, 040, 000, 000, 000, 000, 000, 000, 000 }, 3, 5 },
		{ { 0x00020100, 0x01040103, 0x03010505, 0x00000000 }, { 020, 051, 010, 062, 031, 032, 000, 000, 000, 000, 000, 000 }, 4, 6 },
		{ { 0x03020100, 0x00000001, 0x00000000, 0x00000000 }, { 010, 062, 032, 040, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 020, 062, 032, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x03020100, 0x00000001, 0x00000000, 0x00000000 }, { 031, 040, 020, 051, 000, 000, 000, 000, 000, 000, 000, 000 }, 2, 4 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 010, 031, 051, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x00020100, 0x00000000, 0x00000000, 0x00000000 }, { 010, 040, 020, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 1, 3 },
		{ { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, { 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000, 000 }, 0, 0 },
	};

}

#else

namespace era_engine
{
	void initialize_mesh_shader() {}
	void test_render_mesh_shader(TransparentRenderPass* ldr_render_pass, float dt) {}
}

#endif