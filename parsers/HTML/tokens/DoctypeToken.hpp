#ifndef DOCTYPETOKEN_HPP
#define DOCTYPETOKEN_HPP

#include <string>

#include "HTMLToken.hpp"


class DoctypeToken : public HTMLToken
{
    public:
        public:
        DoctypeToken();
        DoctypeToken(wchar_t token_name);
        bool quirks_required() const;
        bool is_name_set() const;
        void set_is_name_set(bool is_set);
        bool is_public_identifier_set() const;
        bool is_system_identifier_set() const;
        std::string get_public_identifier() const;
        std::string get_system_identifier() const;
        void set_quirks_required(bool required);
        bool is_doctype_token() const;

    private:
        bool require_quirks;
        bool name_set;
        bool public_identifier_set;
        bool system_identifier_set;
        std::string public_identifier;
        std::string system_identifier;
};

#endif // DOCTYPETOKEN_HPP
