#include <type_traits>

struct Cos1{};
struct Cos2{};
struct Cos3{};
struct Cos4{};

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
        if constexpr (!(std::is_same_v<Type, OtherTypes> || ...)) {
            return ComponentList<Types ...>().intersects(otherComponentList);
        }
        else
            return true;
    }
};



int main() {
    ComponentList<Cos1, Cos2, Cos3> cos1;
    ComponentList<Cos4> cos2;
    return cos1.intersects(cos2);
}