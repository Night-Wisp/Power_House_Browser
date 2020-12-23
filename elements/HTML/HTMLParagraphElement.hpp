#ifndef HTMLPARAGRAPHELEMENT_HPP
#define HTMLPARAGRAPHELEMENT_HPP

#include "HTMLElement.hpp"


class HTMLParagraphElement : public HTMLElement
{
    public:
        HTMLParagraphElement();
        bool is_paragraph_node() const;
};

#endif // HTMLPARAGRAPHELEMENT_HPP
