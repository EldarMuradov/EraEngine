// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/shadow_map_cache.h"
#include "rendering/render_resources.h"
#include "rendering/light_source.h"

#include "core/camera.h"
#include "core/imgui.h"
#include "core/hash.h"

#include "dx/dx_texture.h"
#include "dx/dx_command_list.h"

#define DEBUG_VISUALIZATION 0

namespace era_engine
{
    bool enable_static_shadow_map_caching = true;

    static constexpr uint16 maximum_size = 2048;
    static constexpr uint16 minimum_size = 128;
    static_assert(SHADOW_MAP_WIDTH % maximum_size == 0, "");
    static_assert(SHADOW_MAP_HEIGHT % maximum_size == 0, "");
    static_assert(is_power_of_two(maximum_size), "");
    static_assert(is_power_of_two(minimum_size), "");

    static constexpr uint32 num_buckets = index_of_least_significant_set_bit(maximum_size) - index_of_least_significant_set_bit(minimum_size) + 1;
    static std::vector<ShadowMapViewport> nodes[num_buckets];

    struct ShadowMapCache
    {
        ShadowMapViewport viewport;
        uint64 light_movement_hash;
    };

    static std::unordered_map<uint64, ShadowMapCache> cached_allocations;

    static ref<DXTexture> vis_texture;

    static void initialize()
    {
        for (uint32 y = 0; y < SHADOW_MAP_HEIGHT / maximum_size; ++y)
        {
            for (uint32 x = 0; x < SHADOW_MAP_WIDTH / maximum_size; ++x)
            {
                nodes[0].push_back({(uint16)(x * maximum_size), (uint16)(y * maximum_size), maximum_size});
            }
        }
    }

    NODISCARD static uint32 get_index(uint32 size)
    {
        uint32 index = index_of_least_significant_set_bit(size) - index_of_least_significant_set_bit(minimum_size);
        index = num_buckets - index - 1;
        return index;
    }

    NODISCARD static ShadowMapViewport allocate_shadow_viewport(uint32 size)
    {
        ASSERT(size <= maximum_size);
        ASSERT(size >= minimum_size);
        ASSERT(is_power_of_two(size));

        uint32 index = get_index(size);

        uint32 insert_index = index;
        for (; insert_index != -1 && nodes[insert_index].size() == 0; --insert_index)
            ;

        ASSERT(insert_index != -1); // TODO: Handle error
        ASSERT(nodes[insert_index].size() > 0);

        for (uint32 i = insert_index; i < index; ++i)
        {
            uint16 node_size = 1u << (index_of_least_significant_set_bit(maximum_size) - i);
            uint16 half_size = node_size / 2;

            ShadowMapViewport n = nodes[i].back();
            nodes[i].pop_back();

            nodes[i + 1].push_back({n.x, n.y, half_size});
            nodes[i + 1].push_back({(uint16)(n.x + half_size), n.y, half_size});
            nodes[i + 1].push_back({n.x, (uint16)(n.y + half_size), half_size});
            nodes[i + 1].push_back({(uint16)(n.x + half_size), (uint16)(n.y + half_size), half_size});
        }

        ShadowMapViewport result = nodes[index].back();
        nodes[index].pop_back();
        return result;
    }

    static void free_shadow_viewport(ShadowMapViewport remove)
    {
        if (remove.size == maximum_size)
        {
            nodes[0].push_back(remove);
            return;
        }

        uint32 index = get_index(remove.size);
        std::vector<ShadowMapViewport>& list = nodes[index];
        if (list.size() == 0)
        {
            list.push_back(remove);
            return;
        }

        uint32 double_size = remove.size * 2;
        uint32 shift = index_of_least_significant_set_bit(double_size);

        uint32 parent_x = remove.x >> shift;
        uint32 parent_y = remove.y >> shift;
        uint32 parent_width = SHADOW_MAP_WIDTH >> shift;
        uint32 parent_index = parent_y * parent_width + parent_x;

        uint32 nodes_with_this_parent = 0;
        auto insert_it = list.end();

        for (auto it = list.begin(); it != list.end(); ++it)
        {
            ShadowMapViewport node = *it;

            uint32 p_x = node.x >> shift;
            uint32 p_y = node.y >> shift;
            uint32 pi = p_y * parent_width + p_x;

            if (pi == parent_index)
            {
                ++nodes_with_this_parent;
            }
            else if (pi > parent_index)
            {
                insert_it = it;
                break;
            }
        }

        ASSERT(nodes_with_this_parent < 4);

        if (nodes_with_this_parent == 3)
        {
            auto erase_it = insert_it - 3;
            list.erase(erase_it, insert_it);
            free_shadow_viewport(ShadowMapViewport{(uint16)(parent_x * double_size), (uint16)(parent_y * double_size), (uint16)double_size});
        }
        else
        {
            list.insert(insert_it, remove);
        }
    }

    static void visualize(DXCommandList* cl, ref<DXTexture> texture)
    {
        for (uint32 i = 0; i < num_buckets; ++i)
        {
            std::vector<ShadowMapViewport>& nodes_in_level = nodes[i];

            for (ShadowMapViewport vp : nodes_in_level)
            {
                ClearRect rect = {vp.x, vp.y, vp.size, vp.size};
                cl->clear_rtv(texture, 0.f, 1.f, 0.f, 1.f, &rect, 1);
            }
        }

        for (auto a : cached_allocations)
        {
            auto vp = a.second.viewport;
            ClearRect rect = {vp.x, vp.y, vp.size, vp.size};
            cl->clear_rtv(texture, 1.f, 0.f, 0.f, 1.f, &rect, 1);
        }
    }

    static bool init = false;

    NODISCARD std::pair<ShadowMapViewport, bool> assign_shadow_map_viewport(uint64 unique_light_id, uint64 light_movement_hash, uint32 size)
    {
        if (!init)
        {
            initialize();
#if DEBUG_VISUALIZATION
            vis_texture = create_texture(0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, false, true, false);
#endif
            init = true;
        }

        bool static_cache_available = false;
        ShadowMapViewport vp;

        auto it = cached_allocations.find(unique_light_id);
        if (it == cached_allocations.end())
        {
            // New light
            vp = allocate_shadow_viewport(size);
            it = cached_allocations.insert({unique_light_id, {vp, light_movement_hash}}).first;
        }
        else
        {
            if (it->second.viewport.size != size)
            {
                // Reallocate
                free_shadow_viewport(it->second.viewport);
                vp = allocate_shadow_viewport(size);
                it->second.viewport = vp;
            }
            else
            {
                if (it->second.light_movement_hash == light_movement_hash)
                {
                    static_cache_available = true;
                }

                vp = it->second.viewport;
            }

            it->second.light_movement_hash = light_movement_hash;
        }

        static_cache_available &= enable_static_shadow_map_caching;

        return {vp, static_cache_available};
    }

    NODISCARD uint64 get_light_movement_hash(const DirectionalLight& dl)
    {
        size_t seed = 0;
        for (uint32 i = 0; i < dl.num_shadow_cascades; ++i)
        {
            hash_combine(seed, dl.view_projs[i]);
        }
        return seed;
    }

    NODISCARD uint64 get_light_movement_hash(const SpotLightCB& sl)
    {
        size_t seed = 0;
        hash_combine(seed, sl.direction);
        hash_combine(seed, sl.position);
        hash_combine(seed, sl.get_outer_cutoff());
        hash_combine(seed, sl.max_distance);
        return seed;
    }

    NODISCARD uint64 get_light_movement_hash(const PointLightCB& pl)
    {
        size_t seed = 0;
        hash_combine(seed, pl.position);
        hash_combine(seed, pl.radius);
        return seed;
    }

    void update_shadow_map_allocation_visualization()
    {
#if DEBUG_VISUALIZATION
        if (updateVisualization)
        {
            DXCommandList* cl = dx_context.get_free_render_command_list();
            visualize(cl, vis_texture);
            dx_context.execute_command_list(cl);

            ImGui::Begin("Settings");
            ImGui::Image(vis_texture, 512, 512);
            ImGui::End();
        }
#endif
    }

}