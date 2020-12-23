#include <regex>
#include <algorithm>
#include <set>

#include "HTMLTokenizer.hpp"
#include "tokens/StartToken.hpp"
#include "tokens/EndToken.hpp"
#include "tokens/DoctypeToken.hpp"
#include "tokens/CommentToken.hpp"
#include "tokens/CharacterToken.hpp"
#include "tokens/EOFToken.hpp"

int get_wstring_iposition(std::wstring long_str, std::wstring substr);

// A list of commonly used Unicode whitespace characters
// In order: tab, line feed, form feed, space
std::set<wchar_t> space_chars = {'\u0009', '\u000A', '\u000C', '\u0020'};

HTMLTokenizer::HTMLTokenizer()
{
    //ctor
}

// TODO: Check HTML requirements more strictly
bool HTMLTokenizer::is_valid_html_string(const std::wstring &html_string)
{
    return HTMLTokenizer::contains_doctype(html_string) && HTMLTokenizer::contains_root_element(html_string) && HTMLTokenizer::doctype_before_root(html_string);
}

bool HTMLTokenizer::contains_doctype(const std::wstring &html_string)
{
    return (get_wstring_iposition(html_string, L"<DOCTYPE") != -1);
}

bool HTMLTokenizer::contains_root_element(const std::wstring &html_string)
{
    return HTMLTokenizer::contains_root_open(html_string) && HTMLTokenizer::contains_root_close(html_string) && HTMLTokenizer::contains_root_open_before_close(html_string);
}

bool HTMLTokenizer::contains_root_open(const std::wstring &html_string)
{
    std::wregex html_root (L"<html\\s+.*>|<html>");
    std::wsmatch results;
    return std::regex_search(html_string, results, html_root);
}

bool HTMLTokenizer::contains_root_close(const std::wstring &html_string)
{
    return (html_string.find(L"</html>") != std::wstring::npos);
}

bool HTMLTokenizer::contains_root_open_before_close(const std::wstring &html_string)
{
    return (html_string.find(L"<html") < html_string.find(L"</html>"));
}

bool HTMLTokenizer::doctype_before_root(const std::wstring &html_string)
{
    return get_wstring_iposition(html_string, L"<!DOCTYPE") < get_wstring_iposition(html_string, L"<html");
}

std::shared_ptr<HTMLToken> HTMLTokenizer::create_token_from_string(const std::wstring &html_string)
{
    tokenizer_state state = data_state;
    std::wstring::const_iterator it = html_string.cbegin();
    return create_token_from_string(html_string, state, it);
}

std::shared_ptr<HTMLToken> HTMLTokenizer::create_token_from_string(const std::wstring &html_string, HTMLTokenizer::tokenizer_state &state, std::wstring::const_iterator &it)
{
    std::shared_ptr<HTMLToken> token = std::make_shared<HTMLToken>();

    // Can't use range-based loop, because we need to
    // be able to look forwards/go backwards
    for (; it != html_string.cend(); ++it)
    {
        wchar_t next_char = *it;
        switch (state)
        {
            case data_state:
            {
                if (next_char == '&')
                    state = character_reference_state;
                else if (next_char == '<')
                    state = tag_open_state;
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    token = std::make_shared<CharacterToken>(next_char);
                    it++;
                    return token;
                }

                break;
            }
            case rcdata_state:
            {
                if (next_char == '&')
                    state = character_reference_state;
                else if (next_char == '<')
                    state = rcdata_less_than_sign_state;
                else if (next_char == '\u0000')
                {
                    token = std::make_shared<CharacterToken>('\uFFFD');
                    it++;
                    return token;
                }
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    token = std::make_shared<CharacterToken>(next_char);
                    it++;
                    return token;
                }

                break;
            }
            case rawtext_state:
            {
                if (next_char == '<')
                    state = rawtext_less_than_sign_state;
                else if (next_char == '\u0000')
                {
                    token = std::make_shared<CharacterToken>('\uFFFD');
                    it++;
                    return token;
                }
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    token = std::make_shared<CharacterToken>(next_char);
                    it++;
                    return token;
                }

                break;
            }
            case script_data_state:
            {

            }
            case plaintext_state:
            {
                break;
            }
            case tag_open_state:
            {
                if (next_char == '!')
                    state = markup_declaration_open_state;
                else if (next_char == '/')
                    state = end_tag_open_state;
                else if (isalpha(next_char))
                {
                    token = std::make_shared<StartToken>(next_char);
                    state = tag_name_state;
                }
                else if (next_char == '?')
                {

                    state = bogus_comment_state;
                }
                else if (it > html_string.cend())
                {

                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    token = std::make_shared<CharacterToken>('\u003C');
                    //it++;
                    state = data_state;
                    return token;
                }

                break;
            }
            case end_tag_open_state:
            {
                if(isalpha(next_char))
                {
                    token = std::make_shared<EndToken>(next_char);
                    state = tag_name_state;
                }
                else if (next_char == '>')
                    state = data_state;
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    token = std::make_shared<CommentToken>();
                    it--;
                    state = bogus_comment_state;
                }

                break;
            }
            case tag_name_state:
            {
                if (space_chars.count(next_char) != 0)
                    state = before_attribute_name_state;
                else if (next_char == '/')
                    state = self_closing_start_tag_state;
                else if (next_char == '>')
                {
                    state = data_state;
                    it++;
                    return token;
                }
                else if (isalpha(next_char))
                    token->add_char_to_tag_name(next_char);
                else if (next_char == '\u0000')
                    token->add_char_to_tag_name('\uFFFD');
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                    token->add_char_to_tag_name(next_char);

                break;
            }
            case rcdata_less_than_sign_state:
            {

            }
            case rcdata_end_tag_open_state:
            {

            }
            case rcdata_end_tag_name_state:
            {

            }
            case rawtext_less_than_sign_state:
            {

            }
            case rawtext_end_tag_open_state:
            {

            }
            case rawtext_end_tag_name_state:
            {

            }
            case script_data_less_than_sign_state:
            {

            }
            case script_data_end_tag_open_state:
            {

            }
            case script_data_end_tag_name_state:
            {

            }
            case script_data_escape_start_state:
            {

            }
            case script_data_escape_start_dash_state:
            {

            }
            case script_data_escaped_state:
            {

            }
            case script_data_escaped_dash_state:
            {

            }
            case script_data_escaped_dash_dash_state:
            {

            }
            case script_data_escaped_less_than_sign_state:
            {

            }
            case script_data_escaped_end_tag_open_state:
            {

            }
            case script_data_escaped_end_tag_name_state:
            {

            }
            case script_data_double_escape_start_state:
            {

            }
            case script_data_double_escaped_state:
            {

            }
            case script_data_double_escaped_dash_state:
            {

            }
            case script_data_double_escaped_dash_dash_state:
            {

            }
            case script_data_double_escaped_less_than_sign_state:
            {

            }
            case script_data_double_escape_end_state:
            {
                break;
            }
            case before_attribute_name_state:
            {
                // End tokens should never have attributes
                if (space_chars.count(next_char) != 0)
                    break;
                else if (next_char == '/' || next_char == '>' || it > html_string.cend())
                {
                    it--;
                    state = after_attribute_name_state;
                }
                else
                {
                    state = attribute_name_state;
                    token->add_to_current_attribute_name(next_char);
                }

                break;

            }
            case attribute_name_state:
            {
                if (space_chars.count(next_char) != 0 || next_char == '/' || next_char == '>' || it > html_string.cend())
                {
                    it--;
                    state = after_attribute_name_state;
                }
                else if (next_char == '=')
                    state = before_attribute_value_state;
                else if (isalpha(next_char))
                    token->add_to_current_attribute_name(next_char);
                else
                    token->add_to_current_attribute_name(next_char);

                break;
            }
            case after_attribute_name_state:
            {
                if (space_chars.count(next_char) != 0)
                    break;
                else if (next_char == '/')
                    state = self_closing_start_tag_state;
                else if (next_char == '=')
                    state = before_attribute_value_state;
                else if (next_char == '>')
                {
                    state = data_state;
                    return token;
                }
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    it--;
                    state = attribute_name_state;
                }

                break;
            }
            case before_attribute_value_state:
            {
                if (space_chars.count(next_char) != 0)
                    break;
                else if (next_char == '"')
                    state = attribute_value_double_quoted_state;
                else if (next_char == '\'')
                    state = attribute_value_single_quoted_state;
                else if (next_char == '&')
                    state = attribute_value_unquoted_state;
                else if (next_char == '>')
                {
                    state = data_state;
                    it++;
                    return token;
                }
                else
                {
                    token->add_to_current_attribute_name(next_char);
                    state = attribute_value_unquoted_state;
                }

                break;
            }
            case attribute_value_double_quoted_state:
            {
                if (next_char == '"')
                    state = after_attribute_value_quoted_state;
                else if (next_char == '&')
                    state = character_reference_state;
                else if (next_char == '\u0000')
                    token->add_to_current_attribute_value('\uFFFD');
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                    token->add_to_current_attribute_value(next_char);

                break;
            }
            case attribute_value_single_quoted_state:
            {
                if (next_char == '\'')
                    state = after_attribute_value_quoted_state;
                else if (next_char == '&')
                    state = character_reference_state;
                else if (next_char == '\u0000')
                    token->add_to_current_attribute_value('\uFFFD');
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                    token->add_to_current_attribute_value(next_char);

                break;
            }
            case attribute_value_unquoted_state:
            {
                if (space_chars.count(next_char) != 0)
                    state = before_attribute_name_state;
                else if (next_char == '&')
                    state = character_reference_state;
                else if (next_char == '>')
                {
                    state = data_state;
                    it++;
                    return token;
                }
                else if (next_char == '\u0000')
                    token->add_to_current_attribute_value('\uFFFD');
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                    token->add_to_current_attribute_value(next_char);

                break;
            }
            case after_attribute_value_quoted_state:
            {
                token->process_current_attribute();

                if (space_chars.count(next_char) != 0)
                    state = before_attribute_name_state;
                else if (next_char == '/')
                    state = self_closing_start_tag_state;
                else if (next_char == '>')
                {
                    state = data_state;
                    it++;
                    return token;
                }
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }

                break;
            }
            case self_closing_start_tag_state:
            {
                if (next_char == '>')
                {
                    state = data_state;
                    token->set_self_closing(true);
                    it++;
                    return token;
                }
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    it--;
                    state = before_attribute_name_state;
                }

                break;
            }
            case bogus_comment_state:
            {
                if (next_char == '>')
                {
                    state = data_state;
                    it++;
                    return token;
                }
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else if (next_char == '\u0000')
                    token->add_char_to_data('\uFFFD');
                else
                    token->add_char_to_data(next_char);

                break;
            }
            case markup_declaration_open_state:
            {
                if (std::wstring(it, it + 2) == L"--")
                {
                    it += 1;
                    token = std::make_shared<CommentToken>();
                    state = comment_start_state;
                    break;
                }

                std::wstring next_seven_chars(it, it + 7);

                if (get_wstring_iposition(next_seven_chars, L"doctype") == 0)
                {
                    // 6, not 7, because for loop increments one extra step
                    it += 6;
                    state = doctype_state;
                    break;
                }

                if (get_wstring_iposition(next_seven_chars, L"[CDATA[") == 0)
                {

                    state = bogus_comment_state;
                    break;
                }

                token = std::make_shared<CommentToken>();
                state = bogus_comment_state;
                it--;

                break;
            }
            case comment_start_state:
            {
                if (next_char == '-')
                    state = comment_start_dash_state;
                else if (next_char == '>')
                {
                    state = data_state;
                    it++;
                    return token;
                }
                else
                {
                    //token->add_char_to_data(next_char);
                    it--;
                    state = comment_state;
                }

                break;
            }
            case comment_start_dash_state:
            {
                if (next_char == '-')
                    state = comment_end_state;
                else if (next_char == '>')
                {
                    state = data_state;
                    it++;
                    return token;
                }
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    token->add_char_to_data('-');
                    it--;
                    state = comment_state;
                }

                break;
            }
            case comment_state:
            {
                if (next_char == '<')
                {
                    token->add_char_to_data(next_char);
                    state = comment_less_than_sign_state;
                }
                else if (next_char == '-')
                    state = comment_end_dash_state;
                else if (next_char == '\u0000')
                    token->add_char_to_data('\uFFFD');
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                    token->add_char_to_data(next_char);

                break;
            }
            case comment_less_than_sign_state:
            {
                if (next_char == '!')
                {
                    token->add_char_to_data(next_char);
                    state = comment_less_than_sign_bang_state;
                }
                else if (next_char == '<')
                    token->add_char_to_data(next_char);
                else
                {
                    state = comment_state;
                    it--;
                }

                break;
            }
            case comment_less_than_sign_bang_state:
            {
                if (next_char == '-')
                    state = comment_less_than_sign_bang_dash_state;
                else
                {
                    state = comment_state;
                    it--;
                }

                break;
            }
            case comment_less_than_sign_bang_dash_state:
            {
                if (next_char == '-')
                    state = comment_less_than_sign_bang_dash_dash_state;
                else
                {
                    state = comment_end_dash_state;
                    it--;
                }

                break;
            }
            case comment_less_than_sign_bang_dash_dash_state:
            {
                if (next_char == '>' || it > html_string.cend())
                {
                    state = comment_end_state;
                    it--;
                }
                else
                {
                    state = comment_end_state;
                    it--;
                }

                break;
            }
            case comment_end_dash_state:
            {
                if (next_char == '-')
                    state = comment_end_state;
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    token->add_char_to_data('-');
                    state = comment_state;
                    it--;
                }

                break;
            }
            case comment_end_state:
            {
                if (next_char == '>')
                {
                    state = data_state;
                    it++;
                    return token;
                }
                else if (next_char == '!')
                    state = comment_end_bang_state;
                else if (next_char == '-')
                    token->add_char_to_data('-');
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    token->add_char_to_data('-');
                    token->add_char_to_data('-');
                    state = comment_state;
                    it--;
                }

                break;
            }
            case comment_end_bang_state:
            {
                if (next_char == '-')
                {
                    token->add_char_to_data('-');
                    token->add_char_to_data('-');
                    token->add_char_to_data('!');
                    state = comment_end_dash_state;
                }
                else if (next_char == '>')
                {
                    state = data_state;
                    it++;
                    return token;
                }
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    token->add_char_to_data('-');
                    token->add_char_to_data('-');
                    token->add_char_to_data('!');
                    state = comment_state;
                    it--;
                }

                break;
            }
            case doctype_state:
            {
                if (space_chars.count(next_char) != 0)
                    state = doctype_name_state;
                else if (next_char == '>')
                {
                    state = before_doctype_name_state;
                    it--;
                }
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    state = before_doctype_name_state;
                    it--;
                }

                break;
            }
            case before_doctype_name_state:
            {
                if (space_chars.count(next_char) != 0)
                    break;

                token = std::make_shared<DoctypeToken>();

                if (next_char == '>')
                {
                    state = data_state;
                    token->set_quirks_required(true);
                    it++;
                    return token;
                }
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    wchar_t out_char = towlower(next_char);
                    if (next_char == '\u0000')
                        out_char = '\uFFFD';

                    token->add_char_to_tag_name(out_char);
                    state = doctype_name_state;
                    break;
                }

                break;
            }
            case doctype_name_state:
            {
                if (space_chars.count(next_char) != 0)
                {
                    state = after_doctype_name_state;
                    token->set_is_name_set(true);
                }
                else if (next_char == '>')
                {
                    state = data_state;
                    token->set_is_name_set(true);
                    it++;
                    return token;
                }
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }
                else
                {
                    wchar_t out_char = towlower(next_char);
                    if (next_char == '\u0000')
                        out_char = '\uFFFD';

                    token->add_char_to_tag_name(out_char);
                    break;
                }

                break;
            }
            case after_doctype_name_state:
            {
                if (space_chars.count(next_char) != 0)
                    break;
                else if (next_char == '>')
                {
                    state = data_state;
                    it++;
                    return token;
                }
                else if (it > html_string.cend())
                {
                    token = std::make_shared<EOFToken>();
                    it++;
                    return token;
                }

                break;
            }
            case after_doctype_public_keyword_state:
            {

            }
            case before_doctype_public_identifier_state:
            {

            }
            case doctype_public_identifier_double_quoted_state:
            {

            }
            case doctype_public_identifier_single_quoted_state:
            {

            }
            case after_doctype_public_identifier_state:
            {

            }
            case between_doctype_public_and_system_identifiers_state:
            {

            }
            case after_doctype_system_keyword_state:
            {

            }
            case before_doctype_system_identifier_state:
            {

            }
            case doctype_system_identifier_double_quoted_state:
            {

            }
            case doctype_system_identifier_single_quoted_state:
            {

            }
            case after_doctype_system_identifier_state:
            {

            }
            case bogus_doctype_state:
            {

            }
            case cdata_section_state:
            {

            }
            case cdata_section_bracket_state:
            {

            }
            case cdata_section_end_state:
            {

            }
            case character_reference_state:
            {

            }
            case named_character_reference_state:
            {

            }
            case ambiguous_ampersand_state:
            {

            }
            case numeric_character_reference_state:
            {

            }
            case hexadecimal_character_reference_start_state:
            {

            }
            case decimal_character_reference_start_state:
            {

            }
            case hexadecimal_character_reference_state:
            {

            }
            case decimal_character_reference_state:
            {

            }
            case numeric_character_reference_end_state:
            {

            }
            default:
            {
                break;
            }
        }
    }

    // Shouldn't get here
    return token;
}

void HTMLTokenizer::create_tokens_from_chars(const char next_char, const bool EOF, std::function<void (std::shared_ptr<HTMLToken>)> emitToken)
{
    std::shared_ptr<HTMLToken> token = std::make_shared<HTMLToken>();
    HTMLTokenizer::tokenizer_state state = this->current_state;

    switch (state)
    {
        case data_state:
        {
            if (next_char == '&')
                state = character_reference_state;
            else if (next_char == '<')
                state = tag_open_state;
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                token = std::make_shared<CharacterToken>(next_char);
                it++;
                return token;
            }

            break;
        }
        case rcdata_state:
        {
            if (next_char == '&')
                state = character_reference_state;
            else if (next_char == '<')
                state = rcdata_less_than_sign_state;
            else if (next_char == '\u0000')
            {
                token = std::make_shared<CharacterToken>('\uFFFD');
                it++;
                return token;
            }
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                token = std::make_shared<CharacterToken>(next_char);
                it++;
                return token;
            }

            break;
        }
        case rawtext_state:
        {
            if (next_char == '<')
                state = rawtext_less_than_sign_state;
            else if (next_char == '\u0000')
            {
                token = std::make_shared<CharacterToken>('\uFFFD');
                it++;
                return token;
            }
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                token = std::make_shared<CharacterToken>(next_char);
                it++;
                return token;
            }

            break;
        }
        case script_data_state:
        {

        }
        case plaintext_state:
        {
            break;
        }
        case tag_open_state:
        {
            if (next_char == '!')
                state = markup_declaration_open_state;
            else if (next_char == '/')
                state = end_tag_open_state;
            else if (isalpha(next_char))
            {
                token = std::make_shared<StartToken>(next_char);
                state = tag_name_state;
            }
            else if (next_char == '?')
            {

                state = bogus_comment_state;
            }
            else if (EOF)
            {

                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                token = std::make_shared<CharacterToken>('\u003C');
                //it++;
                state = data_state;
                return token;
            }

            break;
        }
        case end_tag_open_state:
        {
            if(isalpha(next_char))
            {
                token = std::make_shared<EndToken>(next_char);
                state = tag_name_state;
            }
            else if (next_char == '>')
                state = data_state;
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                token = std::make_shared<CommentToken>();
                it--;
                state = bogus_comment_state;
            }

            break;
        }
        case tag_name_state:
        {
            if (space_chars.count(next_char) != 0)
                state = before_attribute_name_state;
            else if (next_char == '/')
                state = self_closing_start_tag_state;
            else if (next_char == '>')
            {
                state = data_state;
                it++;
                return token;
            }
            else if (isalpha(next_char))
                token->add_char_to_tag_name(next_char);
            else if (next_char == '\u0000')
                token->add_char_to_tag_name('\uFFFD');
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
                token->add_char_to_tag_name(next_char);

            break;
        }
        case rcdata_less_than_sign_state:
        {

        }
        case rcdata_end_tag_open_state:
        {

        }
        case rcdata_end_tag_name_state:
        {

        }
        case rawtext_less_than_sign_state:
        {

        }
        case rawtext_end_tag_open_state:
        {

        }
        case rawtext_end_tag_name_state:
        {

        }
        case script_data_less_than_sign_state:
        {

        }
        case script_data_end_tag_open_state:
        {

        }
        case script_data_end_tag_name_state:
        {

        }
        case script_data_escape_start_state:
        {

        }
        case script_data_escape_start_dash_state:
        {

        }
        case script_data_escaped_state:
        {

        }
        case script_data_escaped_dash_state:
        {

        }
        case script_data_escaped_dash_dash_state:
        {

        }
        case script_data_escaped_less_than_sign_state:
        {

        }
        case script_data_escaped_end_tag_open_state:
        {

        }
        case script_data_escaped_end_tag_name_state:
        {

        }
        case script_data_double_escape_start_state:
        {

        }
        case script_data_double_escaped_state:
        {

        }
        case script_data_double_escaped_dash_state:
        {

        }
        case script_data_double_escaped_dash_dash_state:
        {

        }
        case script_data_double_escaped_less_than_sign_state:
        {

        }
        case script_data_double_escape_end_state:
        {
            break;
        }
        case before_attribute_name_state:
        {
            // End tokens should never have attributes
            if (space_chars.count(next_char) != 0)
                break;
            else if (next_char == '/' || next_char == '>' || EOF)
            {
                it--;
                state = after_attribute_name_state;
            }
            else
            {
                state = attribute_name_state;
                token->add_to_current_attribute_name(next_char);
            }

            break;

        }
        case attribute_name_state:
        {
            if (space_chars.count(next_char) != 0 || next_char == '/' || next_char == '>' || EOF)
            {
                it--;
                state = after_attribute_name_state;
            }
            else if (next_char == '=')
                state = before_attribute_value_state;
            else if (isalpha(next_char))
                token->add_to_current_attribute_name(next_char);
            else
                token->add_to_current_attribute_name(next_char);

            break;
        }
        case after_attribute_name_state:
        {
            if (space_chars.count(next_char) != 0)
                break;
            else if (next_char == '/')
                state = self_closing_start_tag_state;
            else if (next_char == '=')
                state = before_attribute_value_state;
            else if (next_char == '>')
            {
                state = data_state;
                return token;
            }
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                it--;
                state = attribute_name_state;
            }

            break;
        }
        case before_attribute_value_state:
        {
            if (space_chars.count(next_char) != 0)
                break;
            else if (next_char == '"')
                state = attribute_value_double_quoted_state;
            else if (next_char == '\'')
                state = attribute_value_single_quoted_state;
            else if (next_char == '&')
                state = attribute_value_unquoted_state;
            else if (next_char == '>')
            {
                state = data_state;
                it++;
                return token;
            }
            else
            {
                token->add_to_current_attribute_name(next_char);
                state = attribute_value_unquoted_state;
            }

            break;
        }
        case attribute_value_double_quoted_state:
        {
            if (next_char == '"')
                state = after_attribute_value_quoted_state;
            else if (next_char == '&')
                state = character_reference_state;
            else if (next_char == '\u0000')
                token->add_to_current_attribute_value('\uFFFD');
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
                token->add_to_current_attribute_value(next_char);

            break;
        }
        case attribute_value_single_quoted_state:
        {
            if (next_char == '\'')
                state = after_attribute_value_quoted_state;
            else if (next_char == '&')
                state = character_reference_state;
            else if (next_char == '\u0000')
                token->add_to_current_attribute_value('\uFFFD');
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
                token->add_to_current_attribute_value(next_char);

            break;
        }
        case attribute_value_unquoted_state:
        {
            if (space_chars.count(next_char) != 0)
                state = before_attribute_name_state;
            else if (next_char == '&')
                state = character_reference_state;
            else if (next_char == '>')
            {
                state = data_state;
                it++;
                return token;
            }
            else if (next_char == '\u0000')
                token->add_to_current_attribute_value('\uFFFD');
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
                token->add_to_current_attribute_value(next_char);

            break;
        }
        case after_attribute_value_quoted_state:
        {
            token->process_current_attribute();

            if (space_chars.count(next_char) != 0)
                state = before_attribute_name_state;
            else if (next_char == '/')
                state = self_closing_start_tag_state;
            else if (next_char == '>')
            {
                state = data_state;
                it++;
                return token;
            }
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }

            break;
        }
        case self_closing_start_tag_state:
        {
            if (next_char == '>')
            {
                state = data_state;
                token->set_self_closing(true);
                it++;
                return token;
            }
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                it--;
                state = before_attribute_name_state;
            }

            break;
        }
        case bogus_comment_state:
        {
            if (next_char == '>')
            {
                state = data_state;
                it++;
                return token;
            }
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else if (next_char == '\u0000')
                token->add_char_to_data('\uFFFD');
            else
                token->add_char_to_data(next_char);

            break;
        }
        case markup_declaration_open_state:
        {
            if (std::wstring(it, it + 2) == L"--")
            {
                it += 1;
                token = std::make_shared<CommentToken>();
                state = comment_start_state;
                break;
            }

            std::wstring next_seven_chars(it, it + 7);

            if (get_wstring_iposition(next_seven_chars, L"doctype") == 0)
            {
                // 6, not 7, because for loop increments one extra step
                it += 6;
                state = doctype_state;
                break;
            }

            if (get_wstring_iposition(next_seven_chars, L"[CDATA[") == 0)
            {

                state = bogus_comment_state;
                break;
            }

            token = std::make_shared<CommentToken>();
            state = bogus_comment_state;
            it--;

            break;
        }
        case comment_start_state:
        {
            if (next_char == '-')
                state = comment_start_dash_state;
            else if (next_char == '>')
            {
                state = data_state;
                it++;
                return token;
            }
            else
            {
                //token->add_char_to_data(next_char);
                it--;
                state = comment_state;
            }

            break;
        }
        case comment_start_dash_state:
        {
            if (next_char == '-')
                state = comment_end_state;
            else if (next_char == '>')
            {
                state = data_state;
                it++;
                return token;
            }
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                token->add_char_to_data('-');
                it--;
                state = comment_state;
            }

            break;
        }
        case comment_state:
        {
            if (next_char == '<')
            {
                token->add_char_to_data(next_char);
                state = comment_less_than_sign_state;
            }
            else if (next_char == '-')
                state = comment_end_dash_state;
            else if (next_char == '\u0000')
                token->add_char_to_data('\uFFFD');
            else if (EOF)
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
                token->add_char_to_data(next_char);

            break;
        }
        case comment_less_than_sign_state:
        {
            if (next_char == '!')
            {
                token->add_char_to_data(next_char);
                state = comment_less_than_sign_bang_state;
            }
            else if (next_char == '<')
                token->add_char_to_data(next_char);
            else
            {
                state = comment_state;
                it--;
            }

            break;
        }
        case comment_less_than_sign_bang_state:
        {
            if (next_char == '-')
                state = comment_less_than_sign_bang_dash_state;
            else
            {
                state = comment_state;
                it--;
            }

            break;
        }
        case comment_less_than_sign_bang_dash_state:
        {
            if (next_char == '-')
                state = comment_less_than_sign_bang_dash_dash_state;
            else
            {
                state = comment_end_dash_state;
                it--;
            }

            break;
        }
        case comment_less_than_sign_bang_dash_dash_state:
        {
            if (next_char == '>' || it > html_string.cend())
            {
                state = comment_end_state;
                it--;
            }
            else
            {
                state = comment_end_state;
                it--;
            }

            break;
        }
        case comment_end_dash_state:
        {
            if (next_char == '-')
                state = comment_end_state;
            else if (it > html_string.cend())
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                token->add_char_to_data('-');
                state = comment_state;
                it--;
            }

            break;
        }
        case comment_end_state:
        {
            if (next_char == '>')
            {
                state = data_state;
                it++;
                return token;
            }
            else if (next_char == '!')
                state = comment_end_bang_state;
            else if (next_char == '-')
                token->add_char_to_data('-');
            else if (it > html_string.cend())
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                token->add_char_to_data('-');
                token->add_char_to_data('-');
                state = comment_state;
                it--;
            }

            break;
        }
        case comment_end_bang_state:
        {
            if (next_char == '-')
            {
                token->add_char_to_data('-');
                token->add_char_to_data('-');
                token->add_char_to_data('!');
                state = comment_end_dash_state;
            }
            else if (next_char == '>')
            {
                state = data_state;
                it++;
                return token;
            }
            else if (it > html_string.cend())
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                token->add_char_to_data('-');
                token->add_char_to_data('-');
                token->add_char_to_data('!');
                state = comment_state;
                it--;
            }

            break;
        }
        case doctype_state:
        {
            if (space_chars.count(next_char) != 0)
                state = doctype_name_state;
            else if (next_char == '>')
            {
                state = before_doctype_name_state;
                it--;
            }
            else if (it > html_string.cend())
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                state = before_doctype_name_state;
                it--;
            }

            break;
        }
        case before_doctype_name_state:
        {
            if (space_chars.count(next_char) != 0)
                break;

            token = std::make_shared<DoctypeToken>();

            if (next_char == '>')
            {
                state = data_state;
                token->set_quirks_required(true);
                it++;
                return token;
            }
            else if (it > html_string.cend())
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                wchar_t out_char = towlower(next_char);
                if (next_char == '\u0000')
                    out_char = '\uFFFD';

                token->add_char_to_tag_name(out_char);
                state = doctype_name_state;
                break;
            }

            break;
        }
        case doctype_name_state:
        {
            if (space_chars.count(next_char) != 0)
            {
                state = after_doctype_name_state;
                token->set_is_name_set(true);
            }
            else if (next_char == '>')
            {
                state = data_state;
                token->set_is_name_set(true);
                it++;
                return token;
            }
            else if (it > html_string.cend())
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }
            else
            {
                wchar_t out_char = towlower(next_char);
                if (next_char == '\u0000')
                    out_char = '\uFFFD';

                token->add_char_to_tag_name(out_char);
                break;
            }

            break;
        }
        case after_doctype_name_state:
        {
            if (space_chars.count(next_char) != 0)
                break;
            else if (next_char == '>')
            {
                state = data_state;
                it++;
                return token;
            }
            else if (it > html_string.cend())
            {
                token = std::make_shared<EOFToken>();
                it++;
                return token;
            }

            break;
        }
        case after_doctype_public_keyword_state:
        {

        }
        case before_doctype_public_identifier_state:
        {

        }
        case doctype_public_identifier_double_quoted_state:
        {

        }
        case doctype_public_identifier_single_quoted_state:
        {

        }
        case after_doctype_public_identifier_state:
        {

        }
        case between_doctype_public_and_system_identifiers_state:
        {

        }
        case after_doctype_system_keyword_state:
        {

        }
        case before_doctype_system_identifier_state:
        {

        }
        case doctype_system_identifier_double_quoted_state:
        {

        }
        case doctype_system_identifier_single_quoted_state:
        {

        }
        case after_doctype_system_identifier_state:
        {

        }
        case bogus_doctype_state:
        {

        }
        case cdata_section_state:
        {

        }
        case cdata_section_bracket_state:
        {

        }
        case cdata_section_end_state:
        {

        }
        case character_reference_state:
        {

        }
        case named_character_reference_state:
        {

        }
        case ambiguous_ampersand_state:
        {

        }
        case numeric_character_reference_state:
        {

        }
        case hexadecimal_character_reference_start_state:
        {

        }
        case decimal_character_reference_start_state:
        {

        }
        case hexadecimal_character_reference_state:
        {

        }
        case decimal_character_reference_state:
        {

        }
        case numeric_character_reference_end_state:
        {

        }
        default:
        {
            break;
        }
    }

    // Shouldn't get here
    return token;
}

std::vector<std::shared_ptr<HTMLToken>> HTMLTokenizer::tokenize_string(const std::wstring &html_string)
{
    std::wstring::const_iterator it = html_string.cbegin();
    tokenizer_state state = data_state;

    std::shared_ptr<HTMLToken> token = create_token_from_string(html_string, state, it);
    std::vector<std::shared_ptr<HTMLToken>> tokens;

    while (!(it >= html_string.cend()))
    {
        tokens.push_back(token);
        token = create_token_from_string(html_string, state, it);
    }

    if (it == html_string.cend())
        tokens.push_back(token);

    return tokens;
}
