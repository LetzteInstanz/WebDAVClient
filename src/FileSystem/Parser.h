#pragma once

#include <cstddef>
#include <ctime>
#include <deque>
#include <memory>
#include <stack>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <QByteArray>
#include <QChar>
#include <QString>
#include <QStringView>

#include "../Util.h"
#include "FileSystemObject.h"

class Parser {
public:
    using ParentPath = QString;
    using CurrDirObj = std::unique_ptr<FileSystemObject>;
    using Objects = std::deque<FileSystemObject>;
    using Result = std::tuple<ParentPath, CurrDirObj, Objects>;

#ifndef NDEBUG
    Parser();
#endif

    Result parse_propfind_reply(const QByteArray& data) const;

private:
    enum class Tag {None, Multistatus, Response, PropStat, Prop, Href, ResourceType, GetLastModified, Collection, Status};

    struct TagHasher {
        size_t operator()(Tag t) const noexcept { return to_type<size_t>(t); }
    };
    using TagSet = std::unordered_set<Tag, TagHasher>;
    using TagOrderMap = std::unordered_map<Tag, TagSet, TagHasher>;

    struct FSObjectStruct {
        using Status = FileSystemObject::Status;
        using Type = FileSystemObject::Type;

        static void drop_curr_dir_name(QString& path);
        static QString extract_name(const QStringView& data);
        static std::tm to_tm(const QStringView& str);
        static Status to_status(const QStringView& str);

        void replace_status(Status s);

        QString name;
        std::pair<Status, Type> type = std::make_pair(Status::None, Type::File);
        std::pair<Status, std::tm> last_modified = std::make_pair(Status::None, std::tm{});

    private:
        enum class Token {DayName, Day, Month, Year, Hours, Minutes, Seconds};

        constexpr static Status ret_second_if_first_is_unknown(Status first, Status second);
        static void parse_lexem(std::tm& time, const QStringView& lexem, bool& ok, Token token);

    private:
        static const std::unordered_set<QChar> _delimiters;
        static const std::unordered_map<QString, int> _month_map;
        static const std::vector<std::pair<QString, Status>> _str_code_pairs;
    };

    struct CurrentState {
        CurrentState(TagOrderMap::const_iterator first, Result& result);

        void update_if_start_tag(Tag t);
        void update_if_end_tag(Tag t);
        void update_if_data(Tag t, const QStringView& data);

        std::pair<bool, QString> errorMsgPair = std::make_pair(false, QString());
        std::stack<TagOrderMap::const_iterator, std::vector<TagOrderMap::const_iterator>> stack;
        QStringView not_dav_namespace;

    private:
        void set_error(QString&& msg);

    private:
        FSObjectStruct _obj;
        FSObjectStruct::Status _status = FSObjectStruct::Status::None;
        Result& _result;
    };

    static const std::unordered_map<QString, Tag> _propfind_tag_by_str_map;
    static const TagOrderMap _propfind_tag_order;
};
