#pragma once

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <cassert>
#include <tuple>
#include <algorithm>
#include <iostream>
#include <memory>

namespace ecs
{
    // ============================================================================
    // ENTITY
    // ============================================================================
    using EntityId = uint32_t;

    struct Entity
    {
        EntityId id{0};

        static constexpr uint32_t INDEX_MASK = 0x00FFFFFF;
        static constexpr uint32_t GENERATION_MASK = 0xFF000000;
        static constexpr uint32_t GENERATION_SHIFT = 24;

        Entity() : id(0) {}
        explicit Entity(EntityId raw) : id(raw) {}

        uint32_t index() const { return id & INDEX_MASK; }
        uint8_t generation() const { return (id & GENERATION_MASK) >> GENERATION_SHIFT; }
        bool valid() const { return id != 0; }

        bool operator==(const Entity &o) const { return id == o.id; }
        bool operator!=(const Entity &o) const { return id != o.id; }
    };

    // ============================================================================
    // COMPONENT STORAGE (SparseSet)
    // ============================================================================
    struct IComponentStorage
    {
        virtual ~IComponentStorage() = default;
        virtual void remove(Entity e) = 0;
    };

    template <typename T>
    class ComponentStorage : public IComponentStorage
    {
    public:
        void add(Entity e, const T &component = T{})
        {
            uint32_t idx = e.index();
            if (idx >= sparse.size())
                sparse.resize(idx + 1, -1);

            if (sparse[idx] == -1)
            {
                sparse[idx] = static_cast<int>(dense.size());
                dense.push_back(component);
                denseEntities.push_back(e.id);
            }
            else
            {
                dense[sparse[idx]] = component;
            }
        }

        bool has(Entity e) const
        {
            uint32_t idx = e.index();
            return idx < sparse.size() && sparse[idx] != -1;
        }

        T *get(Entity e)
        {
            uint32_t idx = e.index();
            if (idx < sparse.size() && sparse[idx] != -1)
                return &dense[sparse[idx]];
            return nullptr;
        }

        const T *get(Entity e) const
        {
            uint32_t idx = e.index();
            if (idx < sparse.size() && sparse[idx] != -1)
                return &dense[sparse[idx]];
            return nullptr;
        }

        void remove(Entity e) override
        {
            uint32_t idx = e.index();
            if (idx >= sparse.size() || sparse[idx] == -1)
                return;

            int denseIdx = sparse[idx];
            int lastIdx = static_cast<int>(dense.size() - 1);

            if (denseIdx != lastIdx)
            {
                dense[denseIdx] = std::move(dense[lastIdx]);
                denseEntities[denseIdx] = denseEntities[lastIdx];

                Entity lastEntity{denseEntities[denseIdx]};
                sparse[lastEntity.index()] = denseIdx;
            }

            dense.pop_back();
            denseEntities.pop_back();
            sparse[idx] = -1;
        }

        std::vector<T> &data() { return dense; }
        const std::vector<T> &data() const { return dense; }

        std::vector<EntityId> &entities() { return denseEntities; }
        const std::vector<EntityId> &entities() const { return denseEntities; }

    private:
        std::vector<T> dense;
        std::vector<EntityId> denseEntities;
        std::vector<int> sparse;
    };

    // ============================================================================
    // REGISTRY
    // ============================================================================
    class Registry
    {
    public:
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

            for (auto &[type, storage] : components)
            {
                storage->remove(e);
            }
        }

        bool isAlive(Entity e) const
        {
            uint32_t idx = e.index();
            if (idx >= generations.size())
                return false;
            return generations[idx] == e.generation();
        }

        template <typename T>
        T &addComponent(Entity e, const T &c = T{})
        {
            auto &storage = getOrCreateStorage<T>();
            storage.add(e, c);
            return *storage.get(e);
        }

        template <typename T>
        bool hasComponent(Entity e) const
        {
            auto it = components.find(std::type_index(typeid(T)));
            if (it == components.end())
                return false;
            auto *storage = static_cast<ComponentStorage<T> *>(it->second.get());
            return storage->has(e);
        }

        template <typename T>
        T *getComponent(Entity e)
        {
            auto it = components.find(std::type_index(typeid(T)));
            if (it == components.end())
                return nullptr;
            auto *storage = static_cast<ComponentStorage<T> *>(it->second.get());
            return storage->get(e);
        }

        // ========================================================================
        // VIEW
        // ========================================================================
        template <typename... Cs>
        struct View
        {
            Registry *reg;
            std::tuple<ComponentStorage<Cs> *...> storages;

            struct Iterator
            {
                Registry *reg;
                std::tuple<ComponentStorage<Cs> *...> storages;
                size_t index;

                bool operator!=(const Iterator &other) const
                {
                    return index != other.index;
                }

                void operator++()
                {
                    ++index;
                    skipInvalid();
                }

                void skipInvalid()
                {
                    auto &first = *std::get<0>(storages);
                    auto &ents = first.entities();

                    while (index < ents.size())
                    {
                        Entity e{ents[index]};
                        if (!reg->isAlive(e) || !allHas(e))
                        {
                            ++index;
                            continue;
                        }
                        break;
                    }
                }

                template <size_t... I>
                bool allHasImpl(Entity e, std::index_sequence<I...>)
                {
                    return (std::get<I>(storages)->has(e) && ...);
                }

                bool allHas(Entity e)
                {
                    return allHasImpl(e, std::index_sequence_for<Cs...>{});
                }

                auto operator*()
                {
                    Entity e{std::get<0>(storages)->entities()[index]};
                    return std::tuple<Entity, Cs &...>{
                        e,
                        *std::get<ComponentStorage<Cs> *>(storages)->get(e)...};
                }
            };

            Iterator begin()
            {
                Iterator it{reg, storages, 0};
                it.skipInvalid();
                return it;
            }

            Iterator end()
            {
                return Iterator{reg, storages, std::get<0>(storages)->entities().size()};
            }
        };

        template <typename... Cs>
        View<Cs...> view()
        {
            return View<Cs...>{this, std::make_tuple(&getOrCreateStorage<Cs>()...)};
        }

    private:
        template <typename T>
        ComponentStorage<T> &getOrCreateStorage()
        {
            auto type = std::type_index(typeid(T));
            auto it = components.find(type);
            if (it == components.end())
            {
                auto storage = std::make_unique<ComponentStorage<T>>();
                auto *ptr = storage.get();
                components[type] = std::move(storage);
                return *ptr;
            }
            return *static_cast<ComponentStorage<T> *>(it->second.get());
        }

        std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> components;
        std::vector<uint8_t> generations;
        std::vector<uint32_t> freeIndices;
    };

} // namespace ecs