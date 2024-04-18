// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include <map>

#include <px/core/px_physics_engine.h>
#include <rendering/main_renderer.h>
#include <px/core/px_extensions.h>
#include <application.h>
#include <scripting/native_scripting_linker.h>
#include <scene/scene.h>

#include <NvBlast.h>

#include <NvBlastTk.h>
#include <NvBlastExtTkSerialization.h>

#include "NvBlastGlobals.h"

#include <NvBlastExtDamageShaders.h>
#include <NvBlastExtStressSolver.h>
#include "NvBlastExtImpactDamageManager.h"
#include <NvBlastExtPxManager.h>
#include <NvBlastExtPxAsset.h>
#include <NvBlastExtPxTask.h>
#include <NvBlastExtSerialization.h>
#include <NvBlastExtPxActor.h>
#include <NvBlastExtPxFamily.h>
#include <NvBlastExtPxStressSolver.h>
#include <NvBlastExtPxListener.h>
#include "NvBlastExtAuthoring.h"
#include <NvBlastExtPxAsset.h>
#include "NvBlastExtLlSerialization.h"
#include <NvBlastExtSync.h>

namespace physics
{
	using namespace Nv::Blast;

	static const float RIGIDBODY_DENSITY = 2000.0f;

	struct px_blast_timers
	{
		double damageMaterial;
		double damageFracture;
		double splitIsland;
		double splitPartition;
		double splitVisibility;
	};

	struct px_blast_ext_listener : ExtPxListener
	{
		px_blast_ext_listener() {}

		virtual void onActorCreated(ExtPxFamily& fam, ExtPxActor& actor)
		{
			int chunkCount = actor.getChunkCount();
			auto pxactor = &actor.getPhysXActor();
			std::cout << "blast family actor's chunkCount = " << chunkCount << "\n";
		}

		virtual void onActorDestroyed(ExtPxFamily& fam, ExtPxActor& actor)
		{
			
		}
	};

	inline NvBlastID generateIDFromString(const char* str)
	{
		uint32_t h[4] = { 5381, 5381, 5381, 5381 };
		int i = 0;
		for (const char* ptr = str; *ptr; i = ((i + 1) & 3), ++ptr)
		{
			h[i] = ((h[i] << 5) + h[i]) ^ static_cast<uint32_t>(*ptr);
		}
		return *reinterpret_cast<NvBlastID*>(h);
	}

	struct px_blast_overlap_callback : PxOverlapCallback
	{
		px_blast_overlap_callback(ExtPxManager& manager, std::set<ExtPxActor*>& buff)
			: pxManager(manager), actorBuffer(buff), PxOverlapCallback(hitBuffer, sizeof(hitBuffer) / sizeof(hitBuffer[0])) {}

		PxAgain processTouches(const PxOverlapHit* buffer, PxU32 nbHits)
		{
			for (PxU32 i = 0; i < nbHits; ++i)
			{
				PxRigidDynamic* rigidDynamic = buffer[i].actor->is<PxRigidDynamic>();
				if (rigidDynamic)
				{
					ExtPxActor* actor = pxManager.getActorFromPhysXActor(*rigidDynamic);
					if (actor != nullptr)
					{
						actorBuffer.insert(actor);
					}
				}
			}
			return true;
		}

	private:
		ExtPxManager& pxManager;
		std::set<ExtPxActor*>& actorBuffer;
		PxOverlapHit hitBuffer[1000];
	};

	struct px_blast_family;

	struct px_blast_asset
	{
		px_blast_asset(main_renderer& rndr);
		virtual ~px_blast_asset();

		struct actor_desc
		{
			NvBlastID id;
			PxTransform transform;
			TkGroup* group = nullptr;
		};

		virtual ref<px_blast_family> createFamily(ExtPxManager& pxManager, const actor_desc& desc) = 0;

		ExtPxAsset* getPxAsset() const
		{
			return pxAsset;
		}

		size_t getBlastAssetSize() const;

		float getBondHealthMax() const
		{
			return bondHealthMax;
		}

		float getSupportChunkHealthMax() const
		{
			return bondHealthMax;
		}

		NvBlastExtDamageAccelerator* getAccelerator() const
		{
			return damageAccelerator;
		}

	protected:
		void initialize();

		main_renderer& renderer;

		ExtPxAsset* pxAsset  = nullptr;

		float bondHealthMax;
		float supportChunkHealthMax;

		NvBlastExtDamageAccelerator* damageAccelerator = nullptr;
	};

	struct px_blast_model
	{
		px_blast_model() {}

		struct material
		{
			std::string diffuseTexture;
		};

		struct chunk
		{
			struct mesh
			{
				uint32_t materialIndex;
				px_simple_mesh mesh;
			};

			std::vector<mesh> meshes;
		};

		std::vector<material> materials;
		std::vector<chunk> chunks;

		//static ref<px_blast_model> loadFromFbxFile(const char* path);
		static ref<px_blast_model> loadFromTinyLoader(const char* path, const char* pathMtl);
	};

	struct px_blast_family
	{
		bool overlap(const PxGeometry& geometry, const PxTransform& pose, std::function<void(ExtPxActor*, px_blast_family&)> hitCall);

		void updatePreSplit(float dt);
		void updateAfterSplit(float dt);

		void drawUI();
		void drawStatsUI();

		enum debug_render_mode
		{
			DEBUG_RENDER_DISABLED,
			DEBUG_RENDER_HEALTH_GRAPH,
			DEBUG_RENDER_CENTROIDS,
			DEBUG_RENDER_HEALTH_GRAPH_CENTROIDS,
			DEBUG_RENDER_JOINTS,
			DEBUG_RENDER_AABB_TREE_CENTROIDS,
			DEBUG_RENDER_AABB_TREE_SEGMENTS,
			DEBUG_RENDER_STRESS_GRAPH,
			DEBUG_RENDER_STRESS_GRAPH_NODES_IMPULSES,
			DEBUG_RENDER_STRESS_GRAPH_BONDS_IMPULSES,

			// count
			DEBUG_RENDER_MODES_COUNT
		};

		void fillDebugRender(px_debug_render_buffer& debugRenderBuffer, debug_render_mode mode, float renderScale);

		const ExtPxFamily* getFamily() const
		{
			return pxFamily;
		}

		const NvBlastExtMaterial& getMaterial() const
		{
			return settings.material;
		}

		uint32_t getActorCount() const;

		uint32_t getTotalVisibleChunkCount() const
		{
			return totalVisibleChunkCount;
		}

		size_t getFamilySize() const
		{
			return familySize;
		}

		const px_blast_asset& getBlastAsset()
		{
			return blastAsset;
		}

		void resetStress();

		void refreshStressSolverSettings();
		void refreshDamageAcceleratorSettings();

		void reloadStressSolver();

		struct family_settings
		{
			bool stressSolverEnabled;
			ExtStressSolverSettings stressSolverSettings;
			bool stressDamageEnabled;
			bool damageAcceleratorEnabled;
			NvBlastExtMaterial material;
		};

		void setSettings(const family_settings& setts);

		const family_settings& getSettings() const
		{
			return settings;
		}

		virtual ~px_blast_family();

	protected:

		px_blast_family(ExtPxManager& manager, const px_blast_asset& asset);

		void initialize(const px_blast_asset::actor_desc& desc);

		virtual void onActorCreated(const ExtPxActor& actor) = 0;
		virtual void onActorUpdate(const ExtPxActor& actor) = 0;
		virtual void onActorDestroyed(const ExtPxActor& actor) = 0;
		virtual void onActorHealthUpdate(const ExtPxActor& pxActor) {};

		virtual void onUpdate() {}

		ExtPxManager& pxManager;
		const px_blast_asset& blastAsset;

	private:

		struct px_manager_listener : ExtPxListener
		{
			px_manager_listener(px_blast_family* fam) : family(fam) {}

			virtual void onActorCreated(ExtPxFamily& fam, ExtPxActor& actor)
			{
				family->processActorCreated(fam, actor);
			}

			virtual void onActorDestroyed(ExtPxFamily& fam, ExtPxActor& actor)
			{
				family->processActorDestroyed(fam, actor);
			}

		private:
			px_blast_family* family;
		};

		friend class px_manager_listener;

		void processActorCreated(ExtPxFamily&, ExtPxActor& actor);
		void processActorDestroyed(ExtPxFamily&, ExtPxActor& actor);

		TkFamily* tkFamily = nullptr;
		ExtPxFamily* pxFamily = nullptr;

		ExtPxStressSolver* stressSolver = nullptr;

		px_manager_listener listener;

		family_settings settings;

		PxTransform initialTransform;

		bool spawned;
		size_t familySize;
		uint32_t totalVisibleChunkCount;

		int debugRenderDepth;
		double stressSolveTime;

		::std::set<ExtPxActor*> actors;
		::std::set<ExtPxActor*> actorsToUpdateHealth;
	};

	struct px_asset_list
	{
		struct px_box_asset
		{
			px_box_asset() : staticHeight(-std::numeric_limits<float>().infinity()),
				jointAllBonds(false), extents(20, 20, 20), bondFlags(7) {}

			struct level
			{
				level(int x_, int y_, int z_, bool isSupport_) :x(x_), y(y_), z(z_), isSupport(isSupport_) {};
				level() :x(0), y(0), z(0), isSupport(0) {};

				int x, y, z;
				bool isSupport;
			};

			::std::string id;
			::std::string name;

			PxVec3 extents;

			float staticHeight;
			bool jointAllBonds;

			uint32_t bondFlags;

			::std::vector<level> levels;
		};

		struct px_model_asset
		{
			px_model_asset() : isSkinned(false), transform(PxIdentity) {}

			::std::string id;
			::std::string file;
			::std::string name;
			PxTransform	transform;
			bool isSkinned;
		};

		struct px_composite_asset
		{
			px_composite_asset() : transform(PxIdentity) {}

			struct asset_ref
			{
				::std::string id;
				PxTransform	transform;
			};

			struct joint
			{
				int32_t assetIndices[2];
				uint32_t chunkIndices[2];
				PxVec3 attachPositions[2];
			};

			::std::string id;
			::std::string name;

			PxTransform transform;

			::std::vector<asset_ref> assetRefs;
			::std::vector<joint> joints;
		};

		::std::vector<px_model_asset> models;
		::std::vector<px_composite_asset> composites;
		::std::vector<px_box_asset>	boxes;
	};

	struct px_asset_generator
	{
		struct blast_chunk_cube
		{
			blast_chunk_cube(PxVec3 position_, PxVec3 extents_)
			{
				position = position_;
				extents = extents_;
			}

			PxVec3 position;
			PxVec3 extents;
		};

		std::vector<NvBlastChunkDesc> solverChunks;
		std::vector<NvBlastBondDesc> solverBonds;
		std::vector<blast_chunk_cube> chunks;

		PxVec3 extents;
	};

	struct px_cube_asset_generator
	{
		struct depth_info
		{
			depth_info(PxVec3 slices = PxVec3(1, 1, 1), NvBlastChunkDesc::Flags flag_ = NvBlastChunkDesc::Flags::NoFlags)
				: slicesPerAxis(slices), flag(flag_) {}

			PxVec3 slicesPerAxis;
			NvBlastChunkDesc::Flags flag;
		};

		enum bound_flags
		{
			NO_BONDS = 0,
			X_BONDS = 1 << 0,
			Y_BONDS = 1 << 1,
			Z_BONDS = 1 << 2,
			X_PLUS_WORLD_BONDS = 1 << 3,
			X_MINUS_WORLD_BONDS = 1 << 4,
			Y_PLUS_WORLD_BONDS = 1 << 5,
			Y_MINUS_WORLD_BONDS = 1 << 6,
			Z_PLUS_WORLD_BONDS = 1 << 7,
			Z_MINUS_WORLD_BONDS = 1 << 8,
			ALL_INTERNAL_BONDS = X_BONDS | Y_BONDS | Z_BONDS
		};

		struct asset_settings
		{
			asset_settings() : bondFlags(bound_flags::ALL_INTERNAL_BONDS) {}

			std::vector<depth_info> depths;
			PxVec3 extents;
			bound_flags bondFlags;
		};

		static void generate(px_asset_generator& asset, const asset_settings& settings);

	private:
		static void fillBondDesc(std::vector<NvBlastBondDesc>& bondDescs, uint32_t id0, uint32_t id1, PxVec3 pos0, PxVec3 pos1, PxVec3 size, float area);
	};

	inline px_cube_asset_generator::bound_flags operator | (px_cube_asset_generator::bound_flags a, px_cube_asset_generator::bound_flags b)
	{
		return static_cast<px_cube_asset_generator::bound_flags>(static_cast<int>(a) | static_cast<int>(b));
	}

	struct px_blast_asset_boxes : px_blast_asset
	{
		struct desc
		{
			px_cube_asset_generator::asset_settings generatorSettings;
			float staticHeight;
			bool jointAllBonds;
		};

		px_blast_asset_boxes(TkFramework& framework, PxPhysics& physics, main_renderer& renderer, const desc& desc);
		virtual ~px_blast_asset_boxes();

		ref<px_blast_family> createFamily(ExtPxManager& pxManager, const actor_desc& desc);

	private:
		PxConvexMesh* boxMesh = nullptr;
		px_asset_generator generatorAsset;
	};

	struct px_blast_asset_model : px_blast_asset
	{
		px_blast_asset_model(TkFramework& framework, PxPhysics& physics, ExtSerialization& serialization, main_renderer& rndr, const char* modelName);
		virtual ~px_blast_asset_model();

		const ref<px_blast_model>& getModel() const
		{
			return model;
		}

		ref<px_blast_family> createFamily(ExtPxManager& pxManager, const actor_desc& desc);

	private:
		ref<px_blast_model> model;
	};

	struct px_blast_family_boxes : px_blast_family
	{
		px_blast_family_boxes(ExtPxManager& pxManager, main_renderer& rend,
			const px_blast_asset_boxes& blastAsset, const px_blast_asset::actor_desc& desc);
		virtual ~px_blast_family_boxes();

	protected:
		virtual void onActorCreated(const ExtPxActor& actor);
		virtual void onActorUpdate(const ExtPxActor& actor);
		virtual void onActorDestroyed(const ExtPxActor& actor);

	private:
		main_renderer& renderer;
		std::vector<entity_handle> chunkRenderables;
	};

	struct px_blast_family_simple_mesh : px_blast_family
	{
		px_blast_family_simple_mesh(ExtPxManager& pxManager, main_renderer& rndr, const px_blast_asset_model& blastAsset, const px_blast_asset::actor_desc& desc);
		virtual ~px_blast_family_simple_mesh();

	protected:
		virtual void onActorCreated(const ExtPxActor& actor);
		virtual void onActorUpdate(const ExtPxActor& actor);
		virtual void onActorDestroyed(const ExtPxActor& actor);
		virtual void onActorHealthUpdate(const ExtPxActor& pxActor);

	private:
		main_renderer& renderer;

		struct chunk
		{
			std::vector<ref<multi_mesh>> renderMeshes;
			std::vector<entity_handle> renderables;
		};

		std::vector<chunk> chunks;
	};

	struct px_blast_replay
	{
		px_blast_replay();
		~px_blast_replay();

		bool isBlastRecording() const
		{
			return isRecording;
		}

		bool isBlastPlaying() const
		{
			return isPlaying;
		}

		bool hasRecord() const
		{
			return buffer.size() > 0;
		}

		size_t getEventCount() const
		{
			return isBlastRecording() ? sync->getSyncBufferSize() : buffer.size();
		}

		uint32_t getCurrentEventIndex() const
		{
			return nextEventIndex;
		}

		void addFamily(TkFamily* family) const;
		void removeFamily(TkFamily* family) const;

		void startRecording(ExtPxManager& manager, bool syncFamily, bool syncPhysics);
		void stopRecording();
		void startPlayback(ExtPxManager& manager, TkGroup* group);
		void stopPlayback();
		void update();
		void reset();

	private:
		void clearBuffer();

		ExtPxManager* pxManager = nullptr;
		TkGroup* group = nullptr;
		ExtSync* sync = nullptr;

		::std::chrono::steady_clock::time_point startTime;

		uint64_t firstEventTs;
		uint32_t nextEventIndex;

		bool isRecording;
		bool isPlaying;

		::std::vector<ExtSyncEvent*> buffer;
	};

	struct px_blast_rigidbody_component;

	struct px_blast_scene_asset
	{
		virtual ~px_blast_scene_asset() {}

		virtual const char* getID() const = 0;
		virtual const char* getName() const = 0;

		virtual void spawn(PxVec3 shift) = 0;

		uint32_t spawnCount{};
	};

	struct px_blast_single_scene_asset : px_blast_scene_asset
	{
		px_blast_single_scene_asset() : asset(nullptr) {}
		virtual ~px_blast_single_scene_asset() { unload(); }

		virtual void spawn(PxVec3 shift) override;

		void spawnOnObject(PxVec3 shift, eentity& entity);

		void load()
		{
			if (!asset)
			{
				asset = createAsset();
			}
		}

		void unload()
		{
			RELEASE_PTR(asset)
		}

		bool isLoaded() const
		{
			return asset != nullptr;
		}

		px_blast_asset* getAsset() const
		{
			return asset;
		}

		virtual PxTransform getInitialTransform() = 0;

		px_blast_asset* asset = nullptr;

	protected:
		virtual px_blast_asset* createAsset() = 0;
	};

	struct px_blast
	{
		px_blast();

		~px_blast()
		{
			if (!released)
				release();
		}

		void release() noexcept
		{
			PX_RELEASE(framework)
			released = false;
		}

		void reinitialize();

		void onSampleStart(PxPhysics* physics, PxScene* scene);
		void onSampleStop();

		void animate(double dt);
		void drawUI();

		bool overlap(const PxGeometry& geometry, const PxTransform& pose, std::function<void(ExtPxActor*, px_blast_family&)> hitCall);

		bool stressDamage(ExtPxActor* actor, PxVec3 position, PxVec3 force) const;

		void deferDamage(ExtPxActor* actor, px_blast_family& family, const NvBlastDamageProgram& program, const void* damageDesc, uint32_t damageDescSize);
		void immediateDamage(ExtPxActor* actor, px_blast_family& family, const NvBlastDamageProgram& program, const void* damageDesc);
		NvBlastFractureBuffers& getFractureBuffers(ExtPxActor* actor);

		ref<px_blast_family> spawnFamily(px_blast_asset* blastAsset, const px_blast_asset::actor_desc& desc);
		void removeFamily(ref<px_blast_family> actor);
		void removeAllFamilies();

		TkFramework& getTkFramework() const
		{
			return *framework;
		}

		TkGroup* getTkGroup() const
		{
			return tkGroup;
		}

		ExtPxManager& getExtPxManager() const
		{
			return *extPxManager;
		}

		ExtImpactDamageManager* getExtImpactDamageManager() const
		{
			return extImpactDamageManager;
		}

		px_blast_replay* getReplay() const
		{
			return replay;
		}

		uint32_t getActorCount() const;

		uint32_t getTotalVisibleChunkCount() const;

		size_t getFamilySize() const;

		size_t getBlastAssetsSize() const
		{
			return blastAssetsSize;
		}

		const px_blast_timers& getLastBlastTimers() const
		{
			return lastBlastTimers;
		}

		bool getImpactDamageEnabled() const
		{
			return impactDamageEnabled;
		}

		void setImpactDamageEnabled(bool enabled, bool forceUpdate = false);

		ExtStressSolverSettings& getStressSolverSettings()
		{
			return extStressSolverSettings;
		}

		float getLastStressDelta() const;

		void notifyPhysXControllerRelease();

		ExtSerialization* getExtSerialization() const
		{
			return extSerialization;
		}

		enum px_blast_filter_data_attributes
		{
			SUPPRESS_CONTACT_NOTIFY = 1,
		};

		void addAsset(px_blast_scene_asset* asset)
		{
			assets.push_back(asset);
		}

		void spawnAsset(uint32_t id)
		{
			PxVec3 shift(PxZero);
			px_blast_scene_asset* asset = assets[id];
			asset->spawn(shift);
		}

		px_blast_family::debug_render_mode debugRenderMode;
		float debugRenderScale;

		::std::vector<px_blast_scene_asset*> assets;
		::std::vector<px_blast_rigidbody_component*> sceneActors;
		::std::map<const TkAsset*, px_blast_single_scene_asset*> tkAssetMap;

	private:

		struct px_blast_fixed_buffer
		{
			px_blast_fixed_buffer(const uint32_t size)
			{
				buffer.resize(size);
				index = 0;
			}

			px_blast_fixed_buffer()
			{
				index = 0;
			}

			void* push(const void* data, uint32_t size)
			{
				if (index + size > buffer.size())
					return nullptr;

				void* dst = &buffer[index];
				memcpy(dst, data, size);
				index += size;
				return dst;
			}

			void clear()
			{
				index = 0;
			}

		private:
			::std::vector<char> buffer;
			uint32_t index;
		};

		void updateDraggingStress();

		void updateImpactDamage();

		void refreshImpactDamageSettings();

		void fillDebugRender();

		void recalculateAssetsSize();

		static bool customImpactDamageFunction(void* data, ExtPxActor* actor, PxShape* shape, PxVec3 position, PxVec3 force);

	private:
		TkFramework* framework = nullptr;

		PxTaskManager* taskManager = nullptr;
		TkGroup* tkGroup = nullptr;
		ExtPxManager* extPxManager = nullptr;
		ExtImpactDamageManager* extImpactDamageManager = nullptr;

		ExtGroupTaskManager* extGroupTaskManager = nullptr;
		ExtSerialization* extSerialization = nullptr;

		px_error_reporter errorReporter;

		ExtStressSolverSettings extStressSolverSettings;
		ExtImpactSettings extImpactDamageManagerSettings;

		::std::vector<ref<px_blast_family>> families;
		px_debug_render_buffer debugRenderBuffer;

		px_blast_fixed_buffer damageDescBuffer;
		px_blast_fixed_buffer damageParamsBuffer;

		NvBlastFractureBuffers fractureBuffers;
		::std::vector<char> fractureData;

		bool impactDamageEnabled;
		bool impactDamageUpdatePending;
		bool impactDamageToStressEnabled;

		float impactDamageToStressFactor;
		float draggingToStressFactor;

		bool rigidBodyLimitEnabled;
		uint32_t rigidBodyLimit;

		px_blast_replay* replay = nullptr;

		px_blast_timers lastBlastTimers;

		size_t blastAssetsSize;

		bool released = false;
	};

	class px_blast_boxes_asset_scene : public px_blast_single_scene_asset
	{
	public:
		px_blast_boxes_asset_scene(const px_asset_list::px_box_asset& d) : desc(d)
		{
			for (uint32_t lv = 0; lv < desc.levels.size(); ++lv)
			{
				const px_asset_list::px_box_asset::level& level = desc.levels[lv];
				NvBlastChunkDesc::Flags fl = (level.isSupport) ? NvBlastChunkDesc::Flags::SupportFlag : NvBlastChunkDesc::Flags::NoFlags;
				assetDesc.generatorSettings.depths.push_back({ PxVec3(level.x, level.y, level.z), fl });
			}
			assetDesc.generatorSettings.extents = PxVec3(desc.extents.x, desc.extents.y, desc.extents.z);
			assetDesc.staticHeight = desc.staticHeight;
			assetDesc.jointAllBonds = desc.jointAllBonds;
			assetDesc.generatorSettings.bondFlags = (px_cube_asset_generator::bound_flags)desc.bondFlags;
		}

		virtual const char* getID() const override { return desc.id.c_str(); }
		virtual const char* getName() const override { return desc.name.c_str(); }


		px_asset_list::px_box_asset desc;
		px_blast_asset_boxes::desc assetDesc;

		virtual px_blast_asset* createAsset()
		{
			const auto& physics = physics_holder::physicsRef;

			return new px_blast_asset_boxes(physics->blast->getTkFramework(), *physics->getPhysics(),
				*physics->app.getRenderer(), assetDesc);
		}

		virtual PxTransform getInitialTransform() { return PxTransform(PxVec3(0, assetDesc.generatorSettings.extents.y / 2, 0)); }
	};

	struct px_blast_model_scene_asset : px_blast_single_scene_asset
	{
		px_blast_model_scene_asset(px_asset_list::px_model_asset d) : desc(d) {}

		virtual const char* getID() const override { return desc.id.c_str(); }
		virtual const char* getName() const override { return desc.name.c_str(); }

		px_asset_list::px_model_asset desc;

		virtual PxTransform getInitialTransform() { return desc.transform; }
	};

	struct px_blast_simple_scene_asset : px_blast_model_scene_asset
	{
		px_blast_simple_scene_asset(px_asset_list::px_model_asset d) : px_blast_model_scene_asset(d) {}

		virtual px_blast_asset* createAsset()
		{
			const auto& physics = physics_holder::physicsRef;

			return new px_blast_asset_model(physics->blast->getTkFramework(), *physics->getPhysics(),
				*physics->blast->getExtSerialization(), *physics->app.getRenderer(), desc.file.c_str());
		}

		virtual ImVec4 getUIColor() const
		{
			return ImColor(255, 255, 200, 255);
		}
	};

	struct px_blast_rigidbody_component : px_physics_component_base
	{
		px_blast_rigidbody_component() = default;
		px_blast_rigidbody_component(std::string nm, px_blast_single_scene_asset* ast) noexcept
			: asset(ast), name(nm)
		{
			index = asset->spawnCount++;

			spawn();
		}

		~px_blast_rigidbody_component() {}

		void release(bool release = false) noexcept override 
		{
			remove();
		}

		void drawUI(int) noexcept
		{
			actor->drawUI();
		}

		void drawStatsUI(int) noexcept
		{
			actor->drawStatsUI();
		}

		void reload() noexcept
		{
			auto& settings = actor->getSettings();
			remove();
			spawn();
			actor->setSettings(settings);
		}

	private:
		void remove() noexcept
		{
			physics_holder::physicsRef->blast->removeFamily(actor);
			actor = nullptr;
		}

		void spawn() noexcept
		{
			const auto& blast = physics_holder::physicsRef->blast;

			PxTransform pose = asset->getInitialTransform();

			pose.p += shift;

			px_blast_asset::actor_desc actorDesc = 
			{
				actorDesc.id = generateIDFromString(name.c_str()),
				pose,
				blast->getTkGroup()
			};

			actor = blast->spawnFamily(asset->getAsset(), actorDesc);
		}

		PxVec3 shift;

		::std::string name;

		uint32_t index{};

		ref<px_blast_family> actor;
		px_blast_single_scene_asset* asset = nullptr;
	};

	static physx::PxJoint*
		createPxJointCallback(ExtPxActor* actor0, const physx::PxTransform& localFrame0, ExtPxActor* actor1,
			const physx::PxTransform& localFrame1, physx::PxPhysics& physics, TkJoint& joint)
	{
		PxDistanceJoint* pxJoint = PxDistanceJointCreate(physics, actor0 ? &actor0->getPhysXActor() : nullptr, localFrame0,
			actor1 ? &actor1->getPhysXActor() : nullptr, localFrame1);
		pxJoint->setMaxDistance(1.0f);
		return pxJoint;
	}
}
