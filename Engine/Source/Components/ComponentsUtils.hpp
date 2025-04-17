#ifndef COMPONENTSUTILS_HPP
#define COMPONENTSUTILS_HPP

// put this in the component constructor to define the required components
#define REQUIRES(...) \
public : \
    using required_components = decltype(RequireComponents<__VA_ARGS__>(std::declval<Super::required_components>()));\
private : \
    friend class EntityComponentSystem; \
    static_assert(true, "") // requires semicolon

#endif