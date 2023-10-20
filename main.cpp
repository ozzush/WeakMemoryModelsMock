#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

struct A {
    virtual ~A() = default;
};

struct B : A {
    int field;

    explicit B(int f) : A(), field(f) {}
};

int main() {
    std::vector<std::shared_ptr<A>> container;
    container.push_back(std::make_unique<B>(5));
    std::dynamic_pointer_cast<B>(container[0])->field;
    std::stringstream input(" 123");
    char arg1;
    int16_t arg2 = 0;
    std::cout << bool(input >> arg1) << ' ' << arg1 << '\n';
    std::cout << bool(input >> arg2) << ' ' << arg2 << '\n';
    std::cout << bool(input >> arg1) << ' ' << arg1 << '\n';
    std::cout << bool(input >> arg1) << ' ' << arg1 << '\n';
    return 0;
}
