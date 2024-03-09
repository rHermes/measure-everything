//
// Created by rhermes on 3/9/24.
//

#include <mimalloc.h>
// #include <mimalloc/types.h>
#include <vector>
#include <iostream>


namespace lol {
    template<typename T>
    using hvector = std::vector<T, mi_heap_stl_allocator<T>>;

    template<typename T>
    using hdvector = std::vector<T, mi_heap_destroy_stl_allocator<T>>;
}


lol::hdvector<int> allocit() {
    lol::hdvector<int> lev;
    lev.emplace_back(10);
    return lev;
}

lol::hdvector<int> allocit(const lol::hdvector<int>::allocator_type& alloc) {
    lol::hdvector<int> lev(alloc);
    lev.emplace_back(10);
    return lev;
}

int main() {
    {
        mi_heap_stl_allocator<int> alloc_a;
        {
            lol::hvector<int> val(alloc_a);
            val.push_back(2);
            std::cout << val.size() << std::endl;
        }

        {
            lol::hvector<int> val(alloc_a);
            val.push_back(2);
            std::cout << val.size() << std::endl;
        }
    }

    {
        mi_heap_destroy_stl_allocator<int> alloc_b;
        {
            lol::hdvector<int> val(alloc_b);
            val.push_back(2);
            std::cout << val.size() << std::endl;
        }
        {
            lol::hdvector<int> val(alloc_b);
            val.push_back(2);
            std::cout << val.size() << std::endl;
        }
    }

    {
        auto wow = allocit();
        std::cout << wow.size() << std::endl;
        std::cout << wow[0] << std::endl;
    }

    {
        mi_heap_t *hp = mi_heap_new();
        mi_heap_destroy_stl_allocator<int> thisOne(hp);
        auto wow = allocit(thisOne);
        mi_heap_destroy(hp);
        std::cout << wow.size() << std::endl;
        std::cout << wow[0] << std::endl;
    }

}