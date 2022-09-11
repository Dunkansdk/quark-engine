#pragma once

#include <vector>
#include "engine/components/componentstorage.hpp"
#include "engine/utils/typelist.hpp"

namespace ADE {

    template <typename COMPONENT_LIST, typename TAG_LIST = META_TYPES::Typelist<>, std::size_t CAPACITY = 10>
    struct EntityManager {

        struct Entity;

        template <typename T>
        using to_key_type           = typename Slotmap<T, CAPACITY>::key_type;
        using component_storage_t   = ComponentStorage<COMPONENT_LIST, TAG_LIST, CAPACITY>;
        using supported_components  = COMPONENT_LIST;

        struct Entity {
            using key_type_list = META_TYPES::mp_transform<to_key_type, COMPONENT_LIST>;
            using key_storage_t = META_TYPES::replace_t<std::tuple, key_type_list>;

            template <typename COMPONENT>
            void add_component(to_key_type<COMPONENT> key) {
                m_component_mask |= component_storage_t::component_info::template mask<COMPONENT>();
                std::get<to_key_type<COMPONENT>>(m_component_keys) = key;
            }

            template <typename COMPONENT>
            bool has_component() const noexcept {
                auto mask = component_storage_t::component_info::template mask<COMPONENT>();
                return m_component_mask & mask;
            }

            template <typename COMPONENT>
            to_key_type<COMPONENT> get_component_key() const {
                assert(has_component<COMPONENT>());
                return std::get<to_key_type<COMPONENT>>(m_component_keys);
            }

            template <typename COMPONENT>
            void erase_component(to_key_type<COMPONENT> key) {
                m_component_mask ^= component_storage_t::component_info::template mask<COMPONENT>();
            }

        private:
            std::size_t id{next_id++};
            typename component_storage_t::component_info::mask_type m_component_mask{};
            typename component_storage_t::tag_info::mask_type       m_tag_mask{};
            key_storage_t m_component_keys {};

            inline static std::size_t next_id{1};

        };

        EntityManager(std::size_t default_size = 100) {
            m_entities.reserve(default_size);
        }

        template<typename COMPONENT, typename... INITIAL_TYPES>
        COMPONENT& add_component(Entity& entity, INITIAL_TYPES&&... values) {
            if(entity.template has_component<COMPONENT>()) {
                return get_component<COMPONENT>(entity);
            }
            // TODO: Create function create_new_component private
            auto& storage = m_components.template get_storage<COMPONENT>();
            to_key_type<COMPONENT> key = storage.push_back(COMPONENT{std::forward<INITIAL_TYPES>(values)...});
            entity.template add_component<COMPONENT>(key);
            return storage[key];
        }

        template<typename COMPONENT>
        COMPONENT& get_component(Entity& entity) {
            assert(entity.template has_component<COMPONENT>());
            auto& storage = m_components.template get_storage<COMPONENT>();
            to_key_type<COMPONENT> key = entity.template get_component_key<COMPONENT>();
            return storage[key];
        }

        template<typename COMPONENT>
        bool erase_component(Entity& entity) {
            if(!entity.template has_component<COMPONENT>()) return false;
            auto& storage = m_components.template get_storage<COMPONENT>();
            to_key_type<COMPONENT> key = entity.template get_component_key<COMPONENT>();
            entity.template erase_component<COMPONENT>(key);
            return storage.erase(key);
        }

        bool erase_entity(Entity& entity) {
            return erase_components_impl(entity, COMPONENT_LIST{});
        }

	    auto& create_entity() { return this->m_entities.emplace_back(); }

        template<typename TFunc>
        void forall(TFunc&& process) {
            for(auto& entity : m_entities) {
                process(entity);
            }
        }

        /**
         * typename C -> Typelist<Components...>
         * typename T -> Typelist<Tags...>
         */
        template<typename C, typename T>
        void foreach(auto&& process) {
            foreach_impl(process, C{}, T{});
        }

        std::size_t get_entities_count() const noexcept {
            return m_entities.size();
        }

    private:
        std::vector<Entity> m_entities{};
	    component_storage_t m_components{};

        template <typename... C, typename... T>
        void foreach_impl(auto&& process, META_TYPES::Typelist<C...>, META_TYPES::Typelist<T...>) {
            for(auto& entity : m_entities) {
                auto has_components = (true && ... && entity.template has_component<C>());
                if(has_components)
                    process(entity, get_component<C>(entity)...);
            }
        }

        template <typename... C>
        bool erase_components_impl(Entity& entity, META_TYPES::Typelist<C...>) {
            return (erase_component<C>(entity),...);
        }
    };

}
