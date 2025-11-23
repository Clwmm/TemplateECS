#include <type_traits>
#include <vector>

struct Cos1{};
struct Cos2{};
struct Cos3{};
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



template <typename ...>
struct ComponentStorage;

template <>
struct ComponentStorage<> {};

template <typename Type, typename ... Types>
struct ComponentStorage<Type, Types ...> {
    std::vector<Type> components{};
    ComponentStorage<Types ...> nextComponentStorage{};

    template <ComponentListType Components>
    auto getReferenceIterator() {
        return void();
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
};

template <typename Type, typename... Types>
struct ComponentList<Type, Types...> {
    using Storage = ComponentStorage<Type, Types ...>;
    using References = ComponentReferences<Type, Types ...>;

    template <typename... OtherTypes>
    constexpr bool intersects(ComponentList<OtherTypes...> otherComponentList) {
        if constexpr (!(std::is_same_v<Type, OtherTypes> || ...)) {
            return ComponentList<Types ...>().intersects(otherComponentList);
        }
        else
            return true;
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

// SYSTEM
template <ComponentListType ReadList, ComponentListType Components>
struct System {
    using Storage = typename Components::Storage;
    using References = typename Components::References;

    void runSystem(const Storage& components) {

    }

    virtual bool execute(const References& references) = 0;

    virtual ~System() = default;
};

int main() {
    ComponentList<Cos1, Cos2, Cos3> cos1;
    ComponentList<Cos4> cos2;

    Ecs<comp1, comp2, comp3>

    auto list = ListComponentList<>().append<ComponentList<Cos1, Cos2, Cos3>>().append<ComponentList<Cos4>>();

    return -1;
}