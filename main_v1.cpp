#include <iostream>
#include <type_traits>

struct Cos1{};
struct Cos2{};
struct Cos3{};
struct Cos4{};

template <typename ...>
struct IsTypeUnique {};

template <>
struct IsTypeUnique<> {

    template <typename>
    constexpr bool checkUnique() {
        return true;
    }
};

template <typename Type, typename ... Types>
struct IsTypeUnique<Type, Types ...> {

    template <typename OtherType>
    constexpr bool checkUnique() const {
        if constexpr (std::is_same_v<Type, OtherType>)
            return false;
        else {
            return IsTypeUnique<Types ...>().template checkUnique<OtherType>();
        }
    }
};

template <typename ...>
struct ComponentList;

template <>
struct ComponentList<> {
    template <typename ... Types>
    constexpr bool intersects(ComponentList<Types ...>) {
        return false;
    }
};

template <typename Type, typename... Types>
struct ComponentList<Type, Types...> {

    template <typename... OtherTypes>
    constexpr bool intersects(ComponentList<OtherTypes...> otherComponentList) {
        if constexpr (IsTypeUnique<OtherTypes ...>().template checkUnique<Type>()) {
            return ComponentList<Types ...>().intersects(otherComponentList);
        }
        else
            return true;
    }
};

int main() {
    ComponentList<Cos1, Cos2, Cos3> cos1;
    ComponentList<Cos4, Cos1> cos2;
    std::cout << cos1.intersects(cos2) << std::endl;
}