#include <clean-core/string_view.hh>

#include <cstring>

cc::string_view::string_view(const char* data) : _data(data), _size(data ? strlen(data) : 0) {}
