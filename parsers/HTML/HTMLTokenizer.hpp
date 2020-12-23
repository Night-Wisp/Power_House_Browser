#ifndef HTMLTOKENIZER_HPP
#define HTMLTOKENIZER_HPP


#include <string>
#include <memory>
#include <vector>
#include <functional>

#include "tokens/HTMLToken.hpp"

class HTMLTokenizer
{
    public:
        HTMLTokenizer();

        static bool is_valid_html_string(const std::wstring &html_string);

        /* list of possible states
        * full list and state swithing instructions
        * can be found at
        * https://html.spec.whatwg.org/multipage/parsing.html#tokenization
        */
        enum tokenizer_state {
            data_state,
            rcdata_state,
            rawtext_state,
            script_data_state,
            plaintext_state,
            tag_open_state,
            end_tag_open_state,
            tag_name_state,
            rcdata_less_than_sign_state,
            rcdata_end_tag_open_state,
            rcdata_end_tag_name_state,
            rawtext_less_than_sign_state,
            rawtext_end_tag_open_state,
            rawtext_end_tag_name_state,
            script_data_less_than_sign_state,
            script_data_end_tag_open_state,
            script_data_end_tag_name_state,
            script_data_escape_start_state,
            script_data_escape_start_dash_state,
            script_data_escaped_state,
            script_data_escaped_dash_state,
            script_data_escaped_dash_dash_state,
            script_data_escaped_less_than_sign_state,
            script_data_escaped_end_tag_open_state,
            script_data_escaped_end_tag_name_state,
            script_data_double_escape_start_state,
            script_data_double_escaped_state,
            script_data_double_escaped_dash_state,
            script_data_double_escaped_dash_dash_state,
            script_data_double_escaped_less_than_sign_state,
            script_data_double_escape_end_state,
            before_attribute_name_state,
            attribute_name_state,
            after_attribute_name_state,
            before_attribute_value_state,
            attribute_value_double_quoted_state,
            attribute_value_single_quoted_state,
            attribute_value_unquoted_state,
            after_attribute_value_quoted_state,
            self_closing_start_tag_state,
            bogus_comment_state,
            markup_declaration_open_state,
            comment_start_state,
            comment_start_dash_state,
            comment_state,
            comment_less_than_sign_state,
            comment_less_than_sign_bang_state,
            comment_less_than_sign_bang_dash_state,
            comment_less_than_sign_bang_dash_dash_state,
            comment_end_dash_state,
            comment_end_state,
            comment_end_bang_state,
            doctype_state,
            before_doctype_name_state,
            doctype_name_state,
            after_doctype_name_state,
            after_doctype_public_keyword_state,
            before_doctype_public_identifier_state,
            doctype_public_identifier_double_quoted_state,
            doctype_public_identifier_single_quoted_state,
            after_doctype_public_identifier_state,
            between_doctype_public_and_system_identifiers_state,
            after_doctype_system_keyword_state,
            before_doctype_system_identifier_state,
            doctype_system_identifier_double_quoted_state,
            doctype_system_identifier_single_quoted_state,
            after_doctype_system_identifier_state,
            bogus_doctype_state,
            cdata_section_state,
            cdata_section_bracket_state,
            cdata_section_end_state,
            character_reference_state,
            named_character_reference_state,
            ambiguous_ampersand_state,
            numeric_character_reference_state,
            hexadecimal_character_reference_start_state,
            decimal_character_reference_start_state,
            hexadecimal_character_reference_state,
            decimal_character_reference_state,
            numeric_character_reference_end_state
        };

        std::shared_ptr<HTMLToken> create_token_from_string(const std::wstring &html_string, tokenizer_state &state, std::wstring::const_iterator &it);
        std::shared_ptr<HTMLToken> create_token_from_string(const std::wstring &html_string);
        void create_tokens_from_chars(const char next_char, const bool EOF, std::function<void (std::shared_ptr<HTMLToken>)> emitToken);
        std::vector<std::shared_ptr<HTMLToken>> tokenize_string(const std::wstring &html_string);

    private:
        static bool contains_doctype(const std::wstring &html_string);
        static bool contains_root_element(const std::wstring &html_string);
        static bool contains_root_open(const std::wstring &html_string);
        static bool contains_root_close(const std::wstring &html_string);
        static bool contains_root_open_before_close(const std::wstring &html_string);
        static bool doctype_before_root(const std::wstring &html_string);

        tokenizer_state current_state;
};

#endif // HTMLTOKENIZER_HPP
