#include <cstdint>
#include <type_traits>
#include <vector>
#include <iostream>
#include <string>
#include <bitset>
#include <optional>
#include <print>
#include <ranges>
#include <unordered_map>

struct Cos1 {
    uint32_t value;
};
struct Cos2 {
    std::string msg;
};
struct Cos3 {
    float value;
};
struct Cos4{};

template <typename ...>
struct ComponentList;

// CONCEPT
template <typename>
struct IsComponentType : std::false_type {};

template <typename ... Components>
struct IsComponentType<ComponentList<Components ...>> : std::true_type {};

template <typename Type>
concept ComponentListType = IsComponentType<Type>::value;

template<typename... Components>
struct EntityBuilder;

template <typename...>
struct TypeList {};

template<typename...>
struct ValueList;

template <typename... Ts>
ValueList(Ts&&...) -> ValueList<std::decay_t<Ts>...>;

template<>
struct ValueList<> {};

template<typename Type, typename... Types>
struct ValueList<Type, Types...> {
    ValueList() = default;

    explicit ValueList(Type&& headVal, Types&&... tailVals) : head(std::forward<Type>(headVal)), tail(std::forward<Types>(tailVals)...) {}

    template<typename Search>
    auto& get() noexcept {
        if constexpr (std::is_same_v<Search, Type>) {
            return head;
        } else if constexpr (sizeof...(Types) != 0) {
            return tail.template get<Search>();
        } else {
            static_assert(std::false_type::value, "Type not found");
        }
    }

    template<typename Search>
    auto& get() const noexcept {
        if constexpr (std::is_same_v<Search, Type>) {
            return head;
        } else if constexpr (sizeof...(Types) != 0) {
            return tail.template get<Search>();
        } else {
            static_assert(std::false_type::value, "Type not found");
        }
    }

    Type head;
    ValueList<Types ...> tail;
};

template<typename... Components>
struct StorageIterator {
    template<typename Type>
    using RangeType = decltype(std::ranges::subrange(std::declval<std::vector<Type>&>()));

    explicit StorageIterator(std::vector<Components>&... components) : ranges(RangeType<Components>(components)...) {}

    // Iterator type for range\-based for
    struct Iterator {
        template<typename Type>
        using ItType = decltype(std::declval<RangeType<Type>>().begin());

        using ItList = ValueList<ItType<Components>...>;

        ItList begins;
        ItList ends;

        Iterator(ItList&& b, ItList&& e) : begins(std::forward<ItList>(b)), ends(std::forward<ItList>(e)) {}

        // Dereference: return tuple-like ValueList of const refs to components
        auto operator*() const {
            return ValueList<const Components&...>{((*begins.template get<ItType<Components>>()))...};
        }

        // Pre-increment: advance all iterators
        Iterator& operator++() {
            (void)std::initializer_list<int>{(++begins.template get<ItType<Components>>(), 0)...};
            return *this;
        }

        // Compare: continue while all begins != end (stop when any reaches end)
        bool operator!=(const Iterator& other) const {
            return ((begins.template get<ItType<Components>>() != other.ends.template get<ItType<Components>>()) && ...);
        }
    };

    Iterator begin() {
        using ItList = typename Iterator::ItList;
        return Iterator(
            ItList{ ranges.template get<RangeType<Components>>().begin()... },
            ItList{ ranges.template get<RangeType<Components>>().end()... }
        );
    }

    Iterator end() {
        using ItList = typename Iterator::ItList;
        // end iterator uses ends for comparison; begins set to ends as well
        return Iterator(
            ItList{ ranges.template get<RangeType<Components>>().end()... },
            ItList{ ranges.template get<RangeType<Components>>().end()... }
        );
    }

private:
    template<typename Type>
    auto isEmpty() const noexcept {
        const auto& range = ranges.template get<RangeType<Type>>();
        return range.begin() == range.end();
    }

    template<typename Type>
    void next() noexcept {
        auto& range = ranges.template get<RangeType<Type>>();
        range = RangeType<Type>(range.begin() + 1, range.end());
    }

    template<typename Type>
    const auto& getCurrentRef() const noexcept {
        const auto& range = ranges.template get<RangeType<Type>>();
        return *range.begin();
    }

    ValueList<RangeType<Components> ...> ranges;
};

template <typename... Ts>
StorageIterator(std::vector<Ts>&...) -> StorageIterator<std::decay_t<Ts>...>;

template<typename... Components>
struct EntityBuilder {
    EntityBuilder() =  default;

    template<typename Component>
    auto& withComponent(Component&& component) noexcept {
        components.template get<std::optional<Component>>().emplace(std::forward<Component>(component));
        return *this;
    }

    auto getArchetype() const noexcept {
        return (getComponentBit<Components>() | ...);
    }

    template<typename Component>
    auto getComponentBit() const noexcept {
        auto index = ComponentList<Components...>{}.template getComponentIndex<Component>();
        return  components.template get<std::optional<Component>>().has_value() ? (static_cast<uint64_t>(1) << index) : 0;
    }

    ValueList<std::optional<Components>...> components;
};

template <typename ... Types>
struct ComponentStorage {
    template<typename... ArchetypeComponents>
    ComponentStorage(ComponentList<ArchetypeComponents...>) : archetypeMask{ComponentList<Types...>{}.template getComponentsMask<ArchetypeComponents...>()} {}

    ComponentStorage(uint64_t archetypeMask) : archetypeMask{archetypeMask} {}

    auto getReferenceIterator() {
        return StorageIterator<Types...>(getComponents<Types>()...);
    }

    template <typename... ComponentTypes>
    auto getReferenceIterator() {
        return StorageIterator<ComponentTypes...>(getComponents<ComponentTypes>()...);
    }

    template<typename... ArchetypeComponents>
    void push(ArchetypeComponents&&... components) {
        auto entityArchetype = ComponentList<Types...>{}.template getComponentsMask<ArchetypeComponents...>();
        if (entityArchetype != archetypeMask) {
            std::println("Error: Trying to push components that do not match the archetype mask!");
            std::exit(EXIT_FAILURE);
        }
        (pushToStorage<ArchetypeComponents>(std::forward<ArchetypeComponents>(components)), ...);
    }

    void push(EntityBuilder<Types...>&& entity) {
        auto entityArchetype = entity.getArchetype();
        if (entityArchetype != archetypeMask) {
            std::println("Error: Trying to push components that do not match the archetype mask!");
            std::exit(EXIT_FAILURE);
        }
        pushEntity(std::move(entity.components));
    }

    ValueList<std::vector<Types>...> components;
    uint64_t archetypeMask = {0};

private:
    // ComponentStorage(uint64_t archetypeMask): archetypeMask{archetypeMask} {};

    template<typename Type>
    auto& getComponents() noexcept {
        return components.template get<std::vector<Type>>();
    }

    void pushEntity(ValueList<std::optional<Types>...>&& components) {
        (pushComponent(std::move(components.template get<std::optional<Types>>())), ...);
    }

    template<typename Type>
    void pushComponent(std::optional<Type>&& component) {
        if (component.has_value()) {
            pushToStorage<Type>(std::forward<Type>(component.value()));
        }
    }

    template<typename Type>
    void pushToStorage(Type&& component) noexcept {
        auto& componentStorage = getComponents<Type>();
        componentStorage.emplace_back(std::forward<Type>(component));;
    }
};

template <typename ...>
struct ComponentReferences;

template <>
struct ComponentReferences<> {};

template <typename Type, typename ... Types>
struct ComponentReferences<Type, Types ...> {
    const Type& components;
    ComponentReferences<Types ...> nextComponentReferences;
};

// COMPONENT LIST

template <>
struct ComponentList<> {
    template <typename ... Types>
    constexpr bool intersects(ComponentList<Types ...>) {
        return false;
    }

    template<typename... Types>
    constexpr uint64_t getComponentsMask() {
        return 64;
    }

    template<typename Type>
    constexpr uint64_t getComponentIndex() {
        return 64;
    }
};

template <typename Type, typename... Types>
struct ComponentList<Type, Types...> {
    using Storage = ComponentStorage<Type, Types ...>;
    using References = ComponentReferences<Type, Types ...>;
    using Entity = EntityBuilder<Type, Types...>;

    template <typename... OtherTypes>
    constexpr bool intersects(ComponentList<OtherTypes...> otherComponentList) {
        if constexpr (!(std::is_same_v<Type, OtherTypes> || ...)) {
            return ComponentList<Types ...>().intersects(otherComponentList);
        }
        else
            return true;
    }

   template<typename... OtherTypes>
   constexpr uint64_t getComponentsMask() {
       return ((static_cast<uint64_t>(1) << getComponentIndex<OtherTypes>()) | ...);
   }

    template<typename OtherType>
    constexpr uint64_t getComponentIndex() {
        if constexpr (std::is_same_v<Type, OtherType>) {
            return sizeof...(Types);
        } else {
            return ComponentList<Types...>().template getComponentIndex<OtherType>();
        }
    }

    template<typename... ArchetypeComponents>
    static auto makeArchetypeStorage() {
        return typename Storage::ComponentStorage(ComponentList<ArchetypeComponents...>{});
    }
};

// LIST OF COMPONENT LISTS
template <ComponentListType ... ComponentLists>
struct ListComponentList {

    template <ComponentListType NewComponentList>
    constexpr auto append() {
        if constexpr (!(ComponentLists().intersects(NewComponentList{}) || ...)) {
            return ListComponentList<NewComponentList, ComponentLists...>{};
        }
        else
            static_assert(false, "Jakis error");
    }
};

// // SYSTEM
// template <ComponentListType ReadList, ComponentListType Components>
// struct System {
//     using Storage = typename Components::Storage;
//     using References = typename Components::References;
//
//     void runSystem(const Storage& components) {
//
//     }
//
//     virtual bool execute(const References& references) = 0;
//
//     virtual ~System() = default;
// };

using TestComponentList_1 = ComponentList<Cos1, Cos2, Cos3, Cos4>;
using TestComponentList_2 = ComponentList<Cos4>;

template<typename... Components>
struct MasterStorage {
    std::unordered_map<uint64_t, ComponentStorage<Components...>> masterMap;

    void push(EntityBuilder<Components...>&& entity) {
        auto archetype = entity.getArchetype();
        auto [it, inserted] = masterMap.try_emplace(archetype, ComponentStorage<Components...>{archetype});
        it->second.push(std::move(entity));
    }

    // TODO: remove the need to use a double loop
    template<typename... ArchetypeComponents>
    auto getMasterIterator() {
        auto archetypeMask = ComponentList<Components...>{}.template getComponentsMask<ArchetypeComponents...>();
        return std::views::filter(masterMap, [archetypeMask](const auto& pair) {
            return (pair.first & archetypeMask) == archetypeMask;
        }) | std::views::transform([](auto& pair) {
            return pair.second.template getReferenceIterator<ArchetypeComponents...>();
        });
    }
};

int main() {
    auto componentStorage = TestComponentList_1::makeArchetypeStorage<Cos1, Cos2, Cos3>();

    componentStorage.push(Cos1{42}, Cos2{"123"}, Cos3{3.14});
    componentStorage.push(Cos1{23}, Cos2{"Cos2"}, Cos3{2.73});
    componentStorage.push(Cos1{28}, Cos2{"jsdajs"}, Cos3{32});

    auto storageIterator = componentStorage.getReferenceIterator<Cos1, Cos2, Cos3>();
    for (auto x : storageIterator) {
        const auto& cos1 = x.template get<const Cos1&>();
        const auto& cos2 = x.template get<const Cos2&>();
        const auto& cos3 = x.template get<const Cos3&>();
        std::cout << cos1.value << " " << cos2.msg << " " << cos3.value << "ite1" << std::endl;
    }

    std::cout << "\n\n";

    auto storageIterator2 = componentStorage.getReferenceIterator<Cos1, Cos2>();
    for (auto x : storageIterator2) {
        const auto& cos1 = x.template get<const Cos1&>();
        const auto& cos2 = x.template get<const Cos2&>();
        std::cout << cos1.value << " " << cos2.msg << "ite2" << std::endl;
    }

    auto mask1 = TestComponentList_1{}.getComponentsMask<Cos1, Cos3>();
    auto mask2 = TestComponentList_1{}.getComponentsMask<Cos3, Cos1>();
    auto mask3 = TestComponentList_1{}.getComponentsMask<Cos2>();

    std::cout << "mask1:\t" << std::bitset<64>(mask1) << std::endl;
    std::cout << "mask2:\t" << std::bitset<64>(mask2) << std::endl;
    std::cout << "mask3:\t" << std::bitset<64>(mask3) << std::endl;

    auto entity = typename TestComponentList_1::Entity{}.withComponent(Cos1{42}).withComponent(Cos4{});
    std::cout << "Entity Archetype:\t" << std::bitset<64>(entity.getArchetype()) << std::endl;

    auto entity2 = typename TestComponentList_1::Entity{}.withComponent(Cos1{42}).withComponent(Cos2{"65"}).withComponent(Cos3{0.123});
    componentStorage.push(std::move(entity2));

    auto storageIterator3 = componentStorage.getReferenceIterator<Cos1, Cos2, Cos3>();
    for (auto x : storageIterator3) {
        const auto& cos1 = x.template get<const Cos1&>();
        const auto& cos2 = x.template get<const Cos2&>();
        const auto& cos3 = x.template get<const Cos3&>();
        std::cout << cos1.value << " " << cos2.msg << " " << cos3.value << "ite1" << std::endl;
    }

    std::cout << "\n\n";

    MasterStorage<Cos1, Cos2, Cos3, Cos4> masterStorage;
    auto entity3 = typename TestComponentList_1::Entity{}.withComponent(Cos1{99}).withComponent(Cos2{"Master 3"}).withComponent(Cos3{9.99});
    auto entity4 = typename TestComponentList_1::Entity{}.withComponent(Cos1{97}).withComponent(Cos2{"Master 4"}).withComponent(Cos3{9.91});
    auto entity5 = typename TestComponentList_1::Entity{}.withComponent(Cos1{97}).withComponent(Cos2{"Master 5"}).withComponent(Cos3{9.91}).withComponent(Cos4{});
    auto entity6 = typename TestComponentList_1::Entity{}.withComponent(Cos1{97}).withComponent(Cos2{"Master 6"}).withComponent(Cos3{9.91}).withComponent(Cos4{});
    masterStorage.push(std::move(entity3));
    masterStorage.push(std::move(entity4));
    masterStorage.push(std::move(entity5));
    masterStorage.push(std::move(entity6));

    auto masterIterator = masterStorage.getMasterIterator<Cos1, Cos2>();
    for (auto z : masterIterator) {
        for (auto x : z) {
            const auto& cos1 = x.template get<const Cos1&>();
            const auto& cos2 = x.template get<const Cos2&>();
            std::cout << cos1.value << " " << cos2.msg << " master ite" << std::endl;
        }
    }

    return 0;
}
