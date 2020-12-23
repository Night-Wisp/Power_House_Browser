#ifndef ENDTOKEN_HPP
#define ENDTOKEN_HPP

#include <string>
#include <map>

#include "HTMLToken.hpp"


class EndToken : public HTMLToken
{
    public:
        EndToken();
        EndToken(wchar_t token_name);
        bool is_self_closing() const;
        void set_self_closing(bool closing);
        std::map<std::string, std::string> get_attributes() const;
        void add_to_current_attribute_name(wchar_t next_char);
        void add_to_current_attribute_value(wchar_t next_char);
        std::wstring get_attribute_value(std::wstring attribute_name) const;
        bool contains_attribute(std::wstring attribute_name) const;
        void process_current_attribute();
        bool is_end_token() const;

    private:
        bool self_closing;
};

#endif // ENDTOKEN_HPP
