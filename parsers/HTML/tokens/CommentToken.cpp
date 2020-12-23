#include "CommentToken.hpp"

CommentToken::CommentToken()
{
    data = "";
}

bool CommentToken::is_comment_token() const
{
    return true;
}

std::string CommentToken::get_data() const
{
    return data;
}

void CommentToken::add_char_to_data(char next_char)
{
    data.push_back(next_char);
}

void CommentToken::set_data(std::string data_string)
{
    data = data_string;
}
