#include <cstdint>
#include <type_traits>
#include <vector>
#include <iostream>
#include <string>

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

    auto isEmpty() {
        return (isEmpty<Components>() || ...);
    }

    auto next() {
        return (next<Components>(), ...);
    }

    auto getCurrentRef() {
        return ValueList<const Components&...>{getCurrentRef<Components>()...};
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

template <typename ... Types>
struct ComponentStorage {
    ComponentStorage() = default;

    auto getReferenceIterator() {
        return StorageIterator<Types...>(getComponents<Types>()...);
    }

    void push(Types&&... components) {
        (pushToStorage<Types>(std::forward<Types>(components)), ...);
    }

    ValueList<std::vector<Types>...> components;

private:
    template<typename Type>
    auto& getComponents() noexcept {
        return components.template get<std::vector<Type>>();
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

    static auto makeStorage() {
        return Storage{};
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

using TestComponentList_1 = ComponentList<Cos1, Cos2, Cos3>;
using TestComponentList_2 = ComponentList<Cos4>;

int main() {
    auto list = ListComponentList<>().append<TestComponentList_1>().append<TestComponentList_2>();
    auto componentStorage = TestComponentList_1::makeStorage();

    componentStorage.push(Cos1{42}, Cos2{"123"}, Cos3{3.14});
    componentStorage.push(Cos1{23}, Cos2{"Cos2"}, Cos3{2.73});
    componentStorage.push(Cos1{28}, Cos2{"jsdajs"}, Cos3{32});

    auto storageIterator = componentStorage.getReferenceIterator();
    while (!storageIterator.isEmpty()) {
        auto it = storageIterator.getCurrentRef();
        const auto& cos1 = it.template get<const Cos1&>();
        const auto& cos2 = it.template get<const Cos2&>();
        const auto& cos3 = it.template get<const Cos3&>();
        std::cout << cos1.value << " " << cos2.msg << " " << cos3.value << std::endl;
        storageIterator.next();
    }
    return 0;
}
