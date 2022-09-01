#pragma once

#include <vector>
#include "engine/components/componentstorage.hpp"
#include "engine/entity.hpp"
#include "engine/utils/slotmap.hpp"

namespace Quark {

	template <typename EntityType, typename Component0, typename Component1, typename Component2>
	struct EntityManager {
		using TypeProcessFunc = void (*)(EntityType&);

		EntityManager(std::size_t default_size = 100) {
    	    m_entities.reserve(default_size);
		}

		auto& create_entity() { return this->m_entities.emplace_back(); }

		void forall(TypeProcessFunc process) {
			for(auto& entity : m_entities)
				process(entity);
		}

	private:
		std::vector<EntityType> m_entities{};
		ComponentStorage<cpp_function::Typelist<Component0, Component1, Component2>, cpp_function::Typelist<>> m_components{};

	};

}
