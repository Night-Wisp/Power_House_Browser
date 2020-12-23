#ifndef DOCUMENTTYPE_HPP
#define DOCUMENTTYPE_HPP

#include <string>

class DocumentType
{
    public:
        DocumentType(const std::wstring &type_name);
        std::wstring get_name() const;
        void set_name(const std::wstring &type_name);

    protected:
        std::wstring name;
};

#endif // DOCUMENTTYPE_HPP
