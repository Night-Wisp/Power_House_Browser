#ifndef EOFTOKEN_HPP
#define EOFTOKEN_HPP

#include "HTMLToken.hpp"


class EOFToken : public HTMLToken
{
    public:
        EOFToken();
        bool is_eof_token() const;
};

#endif // EOFTOKEN_HPP
