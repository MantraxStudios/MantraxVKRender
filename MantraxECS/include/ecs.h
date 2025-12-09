#pragma once

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <algorithm>
#include <string>

namespace ecs
{
    // ============================================================================
    // ENTITY (con generaciones para reutilizar IDs de forma segura)
    // ============================================================================
    using EntityId = uint32_t;

    struct Entity
    {
        EntityId id{0};

        static constexpr uint32_t INDEX_MASK = 0x00FFFFFF;
        static constexpr uint32_t GENERATION_MASK = 0xFF000000;
        static constexpr uint32_t GENERATION_SHIFT = 24;

        Entity() = default;
        explicit Entity(EntityId raw) : id(raw) {}

        uint32_t index() const { return id & INDEX_MASK; }
        uint8_t generation() const { return (id & GENERATION_MASK) >> GENERATION_SHIFT; }
        bool isValid() const { return id != 0; }

        bool operator==(const Entity &o) const { return id == o.id; }
        bool operator!=(const Entity &o) const { return id != o.id; }
    };

    // ============================================================================
    // COMPONENT STORAGE (SparseSet optimizado)
    // ============================================================================
    struct IComponentStorage
    {
        virtual ~IComponentStorage() = default;
        virtual void remove(Entity e) = 0;
        virtual bool has(Entity e) const = 0;
        virtual size_t size() const = 0;
    };

    template <typename T>
    class ComponentStorage : public IComponentStorage
    {
    public:
        ComponentStorage() { sparse.reserve(64); }

        T &add(Entity e, T component = T{})
        {
            uint32_t idx = e.index();

            if (idx >= sparse.size())
                sparse.resize(idx + 1, INVALID_INDEX);

            if (sparse[idx] != INVALID_INDEX)
            {
                dense[sparse[idx]] = std::move(component);
                return dense[sparse[idx]];
            }

            uint32_t denseIdx = static_cast<uint32_t>(dense.size());
            sparse[idx] = denseIdx;
            dense.push_back(std::move(component));
            entities.push_back(e);

            return dense[denseIdx];
        }

        bool has(Entity e) const override
        {
            uint32_t idx = e.index();
            return idx < sparse.size() && sparse[idx] != INVALID_INDEX;
        }

        T *get(Entity e)
        {
            uint32_t idx = e.index();
            if (idx < sparse.size() && sparse[idx] != INVALID_INDEX)
                return &dense[sparse[idx]];
            return nullptr;
        }

        const T *get(Entity e) const
        {
            uint32_t idx = e.index();
            if (idx < sparse.size() && sparse[idx] != INVALID_INDEX)
                return &dense[sparse[idx]];
            return nullptr;
        }

        void remove(Entity e) override
        {
            uint32_t idx = e.index();
            if (idx >= sparse.size() || sparse[idx] == INVALID_INDEX)
                return;

            uint32_t denseIdx = sparse[idx];
            uint32_t lastIdx = static_cast<uint32_t>(dense.size() - 1);

            if (denseIdx != lastIdx)
            {
                dense[denseIdx] = std::move(dense[lastIdx]);
                entities[denseIdx] = entities[lastIdx];
                sparse[entities[denseIdx].index()] = denseIdx;
            }

            dense.pop_back();
            entities.pop_back();
            sparse[idx] = INVALID_INDEX;
        }

        size_t size() const override { return dense.size(); }

        std::vector<T> &data() { return dense; }
        const std::vector<T> &data() const { return dense; }

        std::vector<Entity> &getEntities() { return entities; }
        const std::vector<Entity> &getEntities() const { return entities; }

    private:
        static constexpr uint32_t INVALID_INDEX = static_cast<uint32_t>(-1);

        std::vector<T> dense;
        std::vector<Entity> entities;
        std::vector<uint32_t> sparse;
    };

    // ============================================================================
    // WORLD (Registry estilo Unity)
    // ============================================================================
    class World
    {
    public:
        World()
        {
            generations.reserve(1024);
            freeIndices.reserve(256);
        }

        // Deshabilitar copia (tiene unique_ptr en el map)
        World(const World &) = delete;
        World &operator=(const World &) = delete;

        // Habilitar movimiento
        World(World &&) noexcept = default;
        World &operator=(World &&) noexcept = default;

        // ========================================================================
        // ENTITY MANAGEMENT
        // ========================================================================

        Entity createEntity()
        {
            uint32_t index;
            uint8_t gen = 0;

            if (!freeIndices.empty())
            {
                index = freeIndices.back();
                freeIndices.pop_back();
                gen = generations[index];
            }
            else
            {
                index = static_cast<uint32_t>(generations.size());
                generations.push_back(0);
            }

            EntityId id = (static_cast<EntityId>(gen) << Entity::GENERATION_SHIFT) | index;
            return Entity{id};
        }

        void destroyEntity(Entity e)
        {
            if (!isAlive(e))
                return;

            uint32_t idx = e.index();
            generations[idx]++;
            freeIndices.push_back(idx);

            for (auto &[type, storage] : storages)
                storage->remove(e);
        }

        bool isAlive(Entity e) const
        {
            uint32_t idx = e.index();
            return idx < generations.size() && generations[idx] == e.generation();
        }

        // ========================================================================
        // COMPONENT MANAGEMENT
        // ========================================================================

        template <typename T, typename... Args>
        T &addComponent(Entity e, Args &&...args)
        {
            auto &storage = getOrCreateStorage<T>();
            return storage.add(e, T{std::forward<Args>(args)...});
        }

        template <typename T>
        T *getComponent(Entity e)
        {
            if (!isAlive(e))
                return nullptr;

            auto it = storages.find(std::type_index(typeid(T)));
            if (it == storages.end())
                return nullptr;

            return static_cast<ComponentStorage<T> *>(it->second.get())->get(e);
        }

        template <typename T>
        const T *getComponent(Entity e) const
        {
            if (!isAlive(e))
                return nullptr;

            auto it = storages.find(std::type_index(typeid(T)));
            if (it == storages.end())
                return nullptr;

            return static_cast<ComponentStorage<T> *>(it->second.get())->get(e);
        }

        template <typename T>
        bool hasComponent(Entity e) const
        {
            if (!isAlive(e))
                return false;

            auto it = storages.find(std::type_index(typeid(T)));
            if (it == storages.end())
                return false;

            return it->second->has(e);
        }

        template <typename T>
        void removeComponent(Entity e)
        {
            auto it = storages.find(std::type_index(typeid(T)));
            if (it != storages.end())
                it->second->remove(e);
        }

        // ========================================================================
        // QUERIES
        // ========================================================================

        template <typename T, typename Func>
        void forEach(Func &&func)
        {
            auto it = storages.find(std::type_index(typeid(T)));
            if (it == storages.end())
                return;

            auto *storage = static_cast<ComponentStorage<T> *>(it->second.get());
            auto &components = storage->data();
            auto &entities = storage->getEntities();

            for (size_t i = 0; i < components.size(); ++i)
            {
                if (isAlive(entities[i]))
                    func(entities[i], components[i]);
            }
        }

        template <typename T1, typename T2, typename Func>
        void forEach(Func &&func)
        {
            auto it1 = storages.find(std::type_index(typeid(T1)));
            auto it2 = storages.find(std::type_index(typeid(T2)));

            if (it1 == storages.end() || it2 == storages.end())
                return;

            auto *storage1 = static_cast<ComponentStorage<T1> *>(it1->second.get());
            auto *storage2 = static_cast<ComponentStorage<T2> *>(it2->second.get());

            if (storage1->size() <= storage2->size())
            {
                auto &entities = storage1->getEntities();
                auto &components1 = storage1->data();

                for (size_t i = 0; i < components1.size(); ++i)
                {
                    Entity e = entities[i];
                    if (isAlive(e))
                    {
                        T2 *c2 = storage2->get(e);
                        if (c2)
                            func(e, components1[i], *c2);
                    }
                }
            }
            else
            {
                auto &entities = storage2->getEntities();
                auto &components2 = storage2->data();

                for (size_t i = 0; i < components2.size(); ++i)
                {
                    Entity e = entities[i];
                    if (isAlive(e))
                    {
                        T1 *c1 = storage1->get(e);
                        if (c1)
                            func(e, *c1, components2[i]);
                    }
                }
            }
        }

        template <typename T1, typename T2, typename T3, typename Func>
        void forEach(Func &&func)
        {
            auto it1 = storages.find(std::type_index(typeid(T1)));
            auto it2 = storages.find(std::type_index(typeid(T2)));
            auto it3 = storages.find(std::type_index(typeid(T3)));

            if (it1 == storages.end() || it2 == storages.end() || it3 == storages.end())
                return;

            auto *storage1 = static_cast<ComponentStorage<T1> *>(it1->second.get());
            auto *storage2 = static_cast<ComponentStorage<T2> *>(it2->second.get());
            auto *storage3 = static_cast<ComponentStorage<T3> *>(it3->second.get());

            size_t minSize = storage1->size();
            int minIdx = 1;

            if (storage2->size() < minSize)
            {
                minSize = storage2->size();
                minIdx = 2;
            }
            if (storage3->size() < minSize)
            {
                minSize = storage3->size();
                minIdx = 3;
            }

            if (minIdx == 1)
            {
                auto &entities = storage1->getEntities();
                auto &components1 = storage1->data();

                for (size_t i = 0; i < components1.size(); ++i)
                {
                    Entity e = entities[i];
                    if (isAlive(e))
                    {
                        T2 *c2 = storage2->get(e);
                        T3 *c3 = storage3->get(e);
                        if (c2 && c3)
                            func(e, components1[i], *c2, *c3);
                    }
                }
            }
            else if (minIdx == 2)
            {
                auto &entities = storage2->getEntities();
                auto &components2 = storage2->data();

                for (size_t i = 0; i < components2.size(); ++i)
                {
                    Entity e = entities[i];
                    if (isAlive(e))
                    {
                        T1 *c1 = storage1->get(e);
                        T3 *c3 = storage3->get(e);
                        if (c1 && c3)
                            func(e, *c1, components2[i], *c3);
                    }
                }
            }
            else
            {
                auto &entities = storage3->getEntities();
                auto &components3 = storage3->data();

                for (size_t i = 0; i < components3.size(); ++i)
                {
                    Entity e = entities[i];
                    if (isAlive(e))
                    {
                        T1 *c1 = storage1->get(e);
                        T2 *c2 = storage2->get(e);
                        if (c1 && c2)
                            func(e, *c1, *c2, components3[i]);
                    }
                }
            }
        }

        // ========================================================================
        // UTILITIES
        // ========================================================================

        size_t entityCount() const
        {
            return generations.size() - freeIndices.size();
        }

        template <typename T>
        size_t componentCount() const
        {
            auto it = storages.find(std::type_index(typeid(T)));
            if (it == storages.end())
                return 0;
            return it->second->size();
        }

    private:
        template <typename T>
        ComponentStorage<T> &getOrCreateStorage()
        {
            auto type = std::type_index(typeid(T));
            auto it = storages.find(type);

            if (it == storages.end())
            {
                auto storage = std::make_unique<ComponentStorage<T>>();
                auto *ptr = storage.get();
                storages[type] = std::move(storage);
                return *ptr;
            }

            return *static_cast<ComponentStorage<T> *>(it->second.get());
        }

        std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> storages;
        std::vector<uint8_t> generations;
        std::vector<uint32_t> freeIndices;
    };

} // namespace ecs