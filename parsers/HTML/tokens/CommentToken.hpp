#ifndef COMMENTTOKEN_HPP
#define COMMENTTOKEN_HPP

#include "HTMLToken.hpp"


class CommentToken : public HTMLToken
{
    public:
        CommentToken();
        bool is_comment_token() const;
        std::string get_data() const;
        void add_char_to_data(char next_char);
        void set_data(std::string data_string);

    protected:
        std::string data;
};

#endif // COMMENTTOKEN_HPP
