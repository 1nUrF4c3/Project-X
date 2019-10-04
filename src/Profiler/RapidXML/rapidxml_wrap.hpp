#pragma once
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

#include "Utilities/StringUtil.hpp"
#include "Utilities/FileUtil.h"

#include <string>
#include <sstream>
#include <ostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <utility>
#include <memory>
#include <functional>
#include <assert.h>

namespace acut
{
    template <typename T>
    struct default_delete
    {
        void operator()( T* t )
        {
            delete t;
        }
    };

    template <typename T>
    struct noop_delete
    {
        void operator()( T* )
        { }
    };

    template <typename Ch>
    class XmlDoc;

    template <typename Ch>
    class XmlIter;

    template <typename Ch>
    class XmlRange;

    struct xml_error : public std::runtime_error
    {
        explicit xml_error(const char* msg) : std::runtime_error(msg) {}
        explicit xml_error(const std::string& msg) : std::runtime_error(msg) {}
    };

    struct xml_key_error : public xml_error
    {
        explicit xml_key_error(const char* msg) : xml_error(msg) {}
        explicit xml_key_error(const std::string& msg) : xml_error(msg) {}
    };

    struct xml_general_error : public xml_error
    {
        explicit xml_general_error(const char* msg) : xml_error(msg) {}
        explicit xml_general_error(const std::string& msg) : xml_error(msg) {}
    };

    template<typename Ch = char>
    class XmlWrap
    {
    public:
        typedef rapidxml::xml_document<Ch> xml_document;
        typedef rapidxml::xml_node<Ch> xml_node;
        typedef rapidxml::xml_attribute<Ch> xml_attribute;
        typedef std::basic_string<Ch> string_t;
        typedef std::unique_ptr<xml_document, std::function<void(xml_document*)>> xml_ptr;
	
	template<typename T> struct type { };

        template <typename T>
        T get(const string_t& key)
        {
            try
            {
                return get_helper(key, type<T>());
            }
            catch (const std::logic_error& ex)
            {
                if (!*noex_)
                    throw xml_general_error(std::string("Failed to convert value: ") + ex.what());
                return T();
            }
            catch (const xml_key_error&)
            {
                if (!*noex_) throw;
                return T();
            }
        }

        bool has(const string_t& key)
        {
            try
            {
                find_node(key);
                return true;
            }
            catch (const std::exception&)
            {
                return false;
            }
        }

        template <typename T>
        void get_if_present(const string_t& key, T& dest)
        {
            try
            {
                dest = get_helper(key, type<T>());
            }
            catch (const std::logic_error&)
            {
            }
            catch (const xml_key_error&)
            {
            }
        }

        template <size_t N>
        bool get(const string_t& key, Ch (&dest)[N])
        {
            return get(key, dest, N);
        }

        bool get(const string_t& key, Ch* dest, int dest_size)
        {
            try
            {
                assert(dest_size > 0);
                auto pair = find_value(key);

                if (static_cast<size_t>(dest_size) <= pair.second)
                    throw xml_general_error("Destination buffer is too small");

                memcpy(dest, pair.first, pair.second * sizeof(Ch));
                dest[pair.second] = acut::ensure_tchar<Ch>('\0');

                return true;
            }
            catch (const xml_key_error&)
            {
                if (!*noex_) throw;
                return false;
            }
        }

        template <typename T>
        bool set(const string_t& key, const T& value)
        {
            return set_helper(key, value);
        }

        bool set(const string_t& key, const Ch* value, int dest_size = 0)
        {
            create_value(key, value, dest_size);
            return true;
        }

        XmlRange<Ch> all_children_of(const string_t& key);

        XmlRange<Ch> all_nodes_named(const string_t& key);

        XmlWrap<Ch> append(const string_t& key)
        {
            auto node = create_or_find_node(key, true);
            return XmlWrap<Ch>(doc_, noex_, node);
        }

        string_t name()
        {
            return string_t(node_->name(), node_->name_size());
        }

        string_t value()
        {
            return string_t( node_->value() );
        }

        void value(const string_t& val)
        {
            node_->value(val.c_str());
        }

    private:
        class Path
        {
        public:
            Path(const string_t& key)
            {
                acut::split(key, &path_, acut::ensure_tchar<Ch>("."));
                if (path_.size() == 0)
                    last_is_attribute_ = false;
                else
                    last_is_attribute_ = (path_.back().front() == acut::ensure_tchar<Ch>('<') &&
                                          path_.back().back() == acut::ensure_tchar<Ch>('>') );
            }

            const string_t& operator[](size_t index) const
            {
                return path_[index];
            }

            const string_t& back() const
            {
                return path_.back();
            }

            bool is_attribute() const
            {
                return last_is_attribute_;
            }

            size_t size() const
            {
                return path_.size();
            }

        private:
            std::vector<string_t> path_;
            bool last_is_attribute_;
        };

        XmlWrap(xml_ptr* doc, bool* noex, xml_node* current)
            : doc_(doc)
            , noex_(noex)
            , node_(current)
        { }

        void set_current(xml_node* current)
        {
            node_ = current;
        }
        
        template <typename T>
        T get_helper(const string_t& key, type<T>)
        {
            T res;
            std::basic_istringstream<Ch> iss((get_helper(key, type<string_t>())));
            iss >> res;
            return res;
        }

        string_t get_helper(const string_t& key, type<string_t>)
        {
            auto pair = find_value(key);
            return string_t(pair.first, pair.second);
        }

        int get_helper(const string_t& key, type<int>)
        { return std::stoi(get_helper(key, type<string_t>())); }

        long long get_helper(const string_t& key, type<long long>)
        { return std::stoll(get_helper(key, type<string_t>())); }

        unsigned long long get_helper(const string_t& key, type<unsigned long long>)
        { return std::stoull(get_helper(key, type<string_t>())); }

        double get_helper(const string_t& key, type<double>)
        { return std::stod(get_helper(key, type<string_t>())); }

        template <typename T>
        bool set_helper(const string_t& key, const T& value)
        {
            std::basic_ostringstream<Ch> oss;
            oss << value;
            auto data = oss.str();
            create_value(key, data.c_str(), data.size());
            return true;
        }

        bool set_helper(const string_t& key, const string_t& value)
        {
            create_value(key, value.c_str(), value.size());
            return true;
        }

        std::pair<Ch*, size_t> find_value(const Path& key)
        {
            xml_node* node = find_node(key);

            if (key.is_attribute())
            {
                auto attr_name = key.back().substr(1, key.back().size() - 2);
                auto attr = node->first_attribute(attr_name.c_str(), attr_name.size());

                if (!attr)
                    throw xml_key_error("No such attribute '" + acut::ensure_tchar<char>(attr_name.c_str()) + "'");

                return std::make_pair(attr->value(), attr->value_size());
            }
            else
            {
                return std::make_pair(node->value(), node->value_size());
            }
        }

        xml_node* find_node(const Path& key)
        {
            if (!get_doc())
                throw xml_general_error("XML document is not initialized");
            assert(node_);
            xml_node* node = node_;

            for (size_t i = 0; i < key.size() - (key.is_attribute() ? 1 : 0); ++i)
            {
                node = node->first_node(key[i].c_str(), key[i].size());

                if (!node)
                    throw xml_key_error("No such node '" + acut::ensure_tchar<char>(key[i].c_str()) + "'");
            }

            return node;
        }

        xml_node* create_or_find_node(const Path& key, bool always_create = false)
        {
            if (!get_doc())
                throw xml_general_error("XML document is not initialized");
            assert(node_);

            xml_node* node = node_;
            size_t total = key.size() - (key.is_attribute() ? 1 : 0);

            for (size_t i = 0; i < total; ++i)
            {
                auto found_node = node->first_node(key[i].c_str(), key[i].size());

                if (!found_node || (i == total - 1 && always_create))
                {
                    auto name = get_doc()->allocate_string(key[i].c_str(), key[i].size());
                    auto new_node = get_doc()->allocate_node(rapidxml::node_element, name, nullptr, key[i].size());
                    node->append_node(new_node);
                    node = new_node;
                }
                else
                {
                    node = found_node;
                }
            }

            return node;
        }

        void create_value(const Path& key, const Ch* value, size_t value_size)
        {
            xml_node* node = create_or_find_node(key);

            if (value_size == 0)
                value_size = rapidxml::internal::measure(value);

            auto allocated_data = get_doc()->allocate_string(value, value_size);

            if (key.is_attribute())
            {
                auto attr_name = key.back().substr(1, key.back().size() - 2);
                auto attr = node->first_attribute(attr_name.c_str(), attr_name.size());

                if (!attr)
                {
                    auto name = get_doc()->allocate_string(attr_name.c_str(), attr_name.size());
                    auto new_attr = get_doc()->allocate_attribute(name, allocated_data, attr_name.size(), value_size);
                    node->append_attribute(new_attr);
                }
                else
                {
                    attr->value(allocated_data, value_size);
                }
            }
            else
            {
                node->value(allocated_data, value_size);
            }
        }

        xml_document* get_doc()
        {
            return doc_->get();
        }

        friend class XmlDoc<Ch>;
        friend class XmlIter<Ch>;

        xml_ptr* doc_;
        bool* noex_;
        xml_node* node_;
    };

    template <typename Ch>
    class XmlIter
    {
    public:
        XmlIter& operator++()
        {
            if (!name_.empty())
                node_ = node_->next_sibling(name_.c_str(), name_.size());
            else
                node_ = node_->next_sibling();

            return *this;
        }

        bool operator!=(const XmlIter<Ch>& snd) const
        {
            return node_ != snd.node_;
        }

        XmlWrap<Ch> operator*()
        {
            return XmlWrap<Ch>(doc_, noex_, node_);
        }

    private:
        XmlIter()
            : node_(nullptr)
            , doc_(nullptr)
            , noex_(nullptr)
        { }

        XmlIter(typename XmlWrap<Ch>::string_t name,
                typename XmlWrap<Ch>::xml_node* node,
                typename XmlWrap<Ch>::xml_ptr* doc,
                bool* noex)
            : name_(name)
            , node_(node)
            , doc_(doc)
            , noex_(noex)
        { }

        friend class XmlRange<Ch>;

        typename XmlWrap<Ch>::string_t name_;
        typename XmlWrap<Ch>::xml_node* node_;
        typename XmlWrap<Ch>::xml_ptr* doc_;
        bool* noex_;
    };


    template <typename Ch>
    class XmlRange
    {
    public:
        XmlIter<Ch> begin()
        {
            return XmlIter<Ch>(name_, node_, doc_, noex_);
        }

        XmlIter<Ch> end()
        {
            return XmlIter<Ch>();
        }

    private:
        XmlRange(typename XmlWrap<Ch>::string_t name,
                 typename XmlWrap<Ch>::xml_node* node,
                 typename XmlWrap<Ch>::xml_ptr* doc,
                 bool* noex)
            : name_(name)
            , node_(node)
            , doc_(doc)
            , noex_(noex)
        { }

        friend class XmlWrap<Ch>;

        typename XmlWrap<Ch>::string_t name_;
        typename XmlWrap<Ch>::xml_node* node_;
        typename XmlWrap<Ch>::xml_ptr* doc_;
        bool* noex_;
    };


    template <typename Ch>
    class XmlDoc : public XmlWrap<Ch>
    {
    public:
	    typedef typename XmlWrap<Ch>::xml_document xml_document;
	    typedef typename XmlWrap<Ch>::string_t string_t;
	    typedef typename XmlWrap<Ch>::xml_ptr xml_ptr;

        explicit XmlDoc(bool use_except = true)
            : XmlWrap<Ch>(&doc_, &noex_, nullptr)
            , noex_(!use_except)
            , doc_(nullptr, acut::noop_delete<xml_document>())
            , buffer_()
        {}

        void read_from_file(const std::wstring& filename);

        void read_from_string(const string_t& content)
        {
            doc_ = xml_ptr(new xml_document, acut::default_delete<xml_document>());
            buffer_.assign(std::begin(content), std::end(content));
            parse_buffer(&buffer_[0]);
            XmlWrap<Ch>::set_current(doc_.get());
        }

        void read_from_buffer(Ch* buffer)
        {
            doc_ = xml_ptr(new xml_document, acut::default_delete<xml_document>());
            parse_buffer(buffer);
            XmlWrap<Ch>::set_current(doc_.get());
        }

        void use_document(xml_document* doc, bool own = false)
        {
            if (own)
                doc_ = xml_ptr(doc, acut::default_delete<xml_document>());
            else
                doc_ = xml_ptr(doc, acut::noop_delete<xml_document>());

            XmlWrap<Ch>::set_current(doc_.get());
        }

        void create_document()
        {
            doc_ = xml_ptr(new xml_document, acut::default_delete<xml_document>());
            XmlWrap<Ch>::set_current(doc_.get());
        }

        template<class CharT, class Traits>
        void write_document(std::basic_ostream<CharT, Traits>& os)
        {
            if (!doc_)
                throw xml_general_error("XML document is not initialized");

            os << *doc_;
        }

        void write_document(const std::wstring& filename)
        {
            if (!doc_)
                throw xml_general_error("XML document is not initialized");

            std::basic_stringstream<Ch> output;
            output << *doc_;

            std::ofstream outfile(filename);
            std::string utf8_string = acut::ensure_tchar<char>(output.str().c_str());
            outfile.write(utf8_string.c_str(), utf8_string.size());
            outfile.close();
        }

        void use_exceptions(bool use_except)
        {
            noex_ = !use_except;
        }

        bool use_exceptions() const
        {
            return !noex_;
        }

    private:
        void parse_buffer(Ch* buffer)
        {
            doc_->template parse<rapidxml::parse_no_data_nodes>(buffer);
        }

        bool noex_;
        xml_ptr doc_;
        std::basic_string<Ch> buffer_;
    };

    template <typename Ch>
    XmlRange<Ch> XmlWrap<Ch>::all_children_of(const string_t& key)
    {
        Path path(key);
        if (path.is_attribute())
            throw xml_key_error("Cannot iterate attributes");

        auto first_child = find_node(path)->first_node();

        return XmlRange<Ch>(string_t(), first_child, doc_, noex_);
    }

    template <typename Ch>
    XmlRange<Ch> XmlWrap<Ch>::all_nodes_named(const string_t& key)
    {
        Path path(key);
        if (path.is_attribute())
            throw xml_key_error("Cannot iterate attributes");

        return XmlRange<Ch>(path.back(), find_node(path), doc_, noex_);
    }

    template <>
    inline void XmlDoc<char>::read_from_file(const std::wstring& filename)
    {
        try
        {
            doc_ = xml_ptr(new xml_document, acut::default_delete<xml_document>());

            if (!acut::read_file(filename, buffer_))
                throw std::runtime_error("Failed to read file: " + acut::ensure_tchar<char>(filename.c_str()));

            parse_buffer(&buffer_[0]);
            XmlWrap<char>::set_current(doc_.get());
        }
        catch (const rapidxml::parse_error& ex)
        {
            throw xml_general_error("Error parsing file '" + acut::ensure_tchar<char>(filename.c_str()) + "': " + ex.what());
        }
    }

    template <>
    inline void XmlDoc<wchar_t>::read_from_file(const std::wstring& filename)
    {
        try
        {
            doc_ = xml_ptr(new xml_document, acut::default_delete<xml_document>());

            std::string utf8_content;
            if (!acut::read_file(filename, utf8_content))
                throw std::runtime_error("Failed to read file: " + acut::ensure_tchar<char>(filename.c_str()));

            buffer_ = blackbone::Utils::UTF8ToWstring( utf8_content );
            parse_buffer(&buffer_[0]);
            XmlWrap<wchar_t>::set_current(doc_.get());
        }
        catch (const rapidxml::parse_error& ex)
        {
            throw xml_general_error("Error parsing file '" + acut::ensure_tchar<char>(filename.c_str()) + "': " + ex.what());
        }
    }
}
