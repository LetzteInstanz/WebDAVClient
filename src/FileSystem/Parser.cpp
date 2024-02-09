#include "Parser.h"

void Parser::FSObjectStruct::drop_curr_dir_name(QString& path) {
    assert(!path.isEmpty());

    if (path == QStringLiteral("/")) {
        path.clear();
        return;
    }
    const qsizetype from = path.back() == '/' ? -2 : -1;
    const qsizetype pos = path.lastIndexOf('/', from);
    assert(pos != -1);
    path.resize(pos + 1);
}

QString Parser::FSObjectStruct::extract_name(const QStringView& data) {
    assert(!data.empty());

    if (data == QStringLiteral("/"))
        return data.toString();

    const qsizetype from = data.back() == '/' ? -2 : -1;
    const qsizetype pos = data.lastIndexOf('/', from);
    assert(pos != -1);
    return QStringView(std::begin(data) + pos + 1, std::end(data) + from + 1).toString();
}

std::tm Parser::FSObjectStruct::to_tm(const QStringView& str) {
    std::tm time;
    time.tm_isdst = 0;

    const auto str_end = std::end(str);
    const bool last_is_T = *(str_end - 1) == 'T';
    using enum FSObjectStruct::Token;
    using Tokens = std::vector<Token>;
    const auto order = last_is_T ? Tokens{DayName, Day, Month, Year, Hours, Minutes, Seconds} : Tokens{DayName, Month, Day, Hours, Minutes, Seconds, Year};
    const auto order_end = std::end(order);
    auto order_it = std::begin(order);
    auto from = std::begin(str);
    bool prev_is_delimiter = false;
    for (auto it = from; order_it != order_end; ++it) {
        const auto str_is_end = it == str_end;
        const bool is_delimiter = str_is_end || _delimiters.contains(*it);
        const bool lexem_is_end = is_delimiter && !prev_is_delimiter;
        bool ok = true;
        if (lexem_is_end) {
            const QStringView lexem(from, it - from);
            parse_lexem(time, lexem, ok, *order_it);
            if (!ok)
                throw std::runtime_error("timestamp parse error");

            ++order_it;
        }
        if (str_is_end)
            break;

        from = !is_delimiter && prev_is_delimiter ? it : from;
        prev_is_delimiter = is_delimiter;
    }
    if (order_it != order_end)
        throw std::runtime_error("timestamp parse error");

    return time;
}

Parser::FSObjectStruct::Status Parser::FSObjectStruct::to_status(const QStringView& str) {
    for (const std::pair<QString, Status>& pair : _str_code_pairs) {
        if (str.indexOf(pair.first) != -1)
            return pair.second;
    }
    return Status::None;
}

void Parser::FSObjectStruct::replace_status(const Status s) {
    type.first = ret_second_if_first_is_unknown(type.first, s);
    last_modified.first = ret_second_if_first_is_unknown(last_modified.first, s);
}

constexpr Parser::FSObjectStruct::Status Parser::FSObjectStruct::ret_second_if_first_is_unknown(Status first, Status second) {
    return first == Parser::FSObjectStruct::Status::Unknown ? second : first;
}

void Parser::FSObjectStruct::parse_lexem(std::tm& time, const QStringView& lexem, bool& ok, Token token) {
    // note:
    // HTTP-date    = rfc1123-date | rfc850-date | asctime-date
    // rfc1123-date = wkday "," SP date1 SP time SP "GMT"
    // rfc850-date  = weekday "," SP date2 SP time SP "GMT"
    // asctime-date = wkday SP date3 SP time SP 4DIGIT
    // date1        = 2DIGIT SP month SP 4DIGIT
    //                ; day month year (e.g., 02 Jun 1982)
    // date2        = 2DIGIT "-" month "-" 2DIGIT
    //                ; day-month-year (e.g., 02-Jun-82)
    // date3        = month SP ( 2DIGIT | ( SP 1DIGIT ))
    //                ; month day (e.g., Jun  2)
    // time         = 2DIGIT ":" 2DIGIT ":" 2DIGIT
    //                ; 00:00:00 - 23:59:59
    // wkday        = "Mon" | "Tue" | "Wed"
    //              | "Thu" | "Fri" | "Sat" | "Sun"
    // weekday      = "Monday" | "Tuesday" | "Wednesday"
    //              | "Thursday" | "Friday" | "Saturday" | "Sunday"
    // month        = "Jan" | "Feb" | "Mar" | "Apr"
    //              | "May" | "Jun" | "Jul" | "Aug"
    //              | "Sep" | "Oct" | "Nov" | "Dec"

    // Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
    // Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
    // Sun Nov 6 08:49:37 1994        ; ANSI C's asctime() format

    switch (token) {
        case Token::Day: {
            time.tm_mday = lexem.toLong(&ok);
            break;
        }

        case Token::Month: {
            const auto it = _month_map.find(lexem.toString());
            if (it == std::end(_month_map))
                throw std::runtime_error("month parse error");

            time.tm_mon = it->second;
            break;
        }

        case Token::Year: {
            int year = lexem.toLong(&ok);
            time.tm_year = year > 99 ? year - 1900 : year;
            break;
        }

        case Token::Hours: {
            time.tm_hour = lexem.toLong(&ok);
            break;
        }

        case Token::Minutes: {
            time.tm_min = lexem.toLong(&ok);
            break;
        }

        case Token::Seconds: {
            time.tm_sec = lexem.toLong(&ok);
            break;
        }

        case Token::DayName:
            break;
    }
}

const std::unordered_set<QChar> Parser::FSObjectStruct::_delimiters{' ', ',', '-', ':'};

const std::unordered_map<QString, int> Parser::FSObjectStruct::_month_map{{"Jan", 0}, {"Feb", 1}, {"Mar", 2}, {"Apr", 3}, {"May", 4}, {"Jun", 5},
                                                                          {"Jul", 6}, {"Aug", 7}, {"Sep", 8}, {"Oct", 9}, {"Nov", 10}, {"Dec", 11}};

const std::vector<std::pair<QString, Parser::FSObjectStruct::Status>> Parser::FSObjectStruct::_str_code_pairs{{"200", Status::Ok},
                                                                                                              {"401", Status::Unauthorized},
                                                                                                              {"403", Status::Forbidden},
                                                                                                              {"404", Status::NotFound}};

//-------------------------------------------------------------------------------------------------------------------------------------------------------

Parser::CurrentState::CurrentState(TagOrderMap::const_iterator first, Result& result) : _result(result) { stack.push(first); }

void Parser::CurrentState::update_if_start_tag(const Tag t) {
    switch (t) {
        case Tag::Response:
            _obj = {};
            break;

        case Tag::PropStat:
            _status = FSObjectStruct::Status::Unknown;
            break;

        case Tag::ResourceType:
            _obj.type.first = _status;
            break;

        case Tag::GetLastModified:
            _obj.last_modified.first = _status;
            break;

        case Tag::Collection:
            _obj.type = std::make_pair(_status, FileSystemObject::Type::Directory);
            break;

        default:
            break;
    }
}

void Parser::CurrentState::update_if_end_tag(const Tag t) {
    switch (t) {
        case Tag::PropStat: {
            _obj.replace_status(_status);
            _status = FSObjectStruct::Status::None;
            break;
        }

        case Tag::Response: {
            if (_obj.type.first != FileSystemObject::Status::Ok) {
                set_error(QObject::tr("a type property in the reply hasn't ok status"));
                break;
            }
            Objects& objects = std::get<2>(_result);
            objects.emplace_back(FileSystemObject(std::move(_obj.name), _obj.type.second, std::move(_obj.last_modified)));
            break;
        }

        case Tag::Multistatus: {
            QString& parent_path = std::get<0>(_result);
            Objects& objects = std::get<2>(_result);
            for (auto it = std::begin(objects); it != std::end(objects);) {
                const QString name = it->get_name();
                if (!parent_path.isEmpty() && parent_path.lastIndexOf(name) == -1) {
                    ++it;
                } else {
                    CurrDirObj& obj = std::get<1>(_result);
                    obj = std::make_unique<FileSystemObject>(std::move(*it));
                    it = objects.erase(it);
                }
            }
            FSObjectStruct::drop_curr_dir_name(parent_path);
            break;
        }

        default:
            break;
    }
}

void Parser::CurrentState::update_if_data(const Tag t, const QStringView& data) {
    switch (t) {
        case Tag::Href: {
            QString abs_path = QUrl::fromPercentEncoding(data.toLatin1());
            _obj.name = FSObjectStruct::extract_name(abs_path);
            QString& parent_path = std::get<0>(_result);
            if (parent_path.size() > abs_path.size() || parent_path.isEmpty())
                parent_path = std::move(abs_path);

            break;
        }

        case Tag::GetLastModified: {
            try {
                _obj.last_modified = std::make_pair(_status, FSObjectStruct::to_tm(data));
            } catch (const std::runtime_error& e) {
                set_error(QObject::tr(e.what()));
            }
            break;
        }

        case Tag::Status: {
            _status = FSObjectStruct::to_status(data);
            break;
        }

        default:
            break;
    }
}

void Parser::CurrentState::set_error(QString&& msg) {
    errorMsgPair.first = true;
    errorMsgPair.second = std::move(msg);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------

const std::unordered_map<QString, Parser::Tag> Parser::_propfind_tag_by_str_map{{"multistatus", Tag::Multistatus},
                                                                                {"response", Tag::Response},
                                                                                {"propstat", Tag::PropStat},
                                                                                {"prop", Tag::Prop},
                                                                                {"href", Tag::Href},
                                                                                {"resourcetype", Tag::ResourceType},
                                                                                {"getlastmodified", Tag::GetLastModified},
                                                                                {"collection", Tag::Collection},
                                                                                {"status", Tag::Status}};

const Parser::TagOrderMap Parser::_propfind_tag_order{{Tag::None, {Tag::Multistatus}},
                                                      {Tag::Multistatus, {Tag::Response}},
                                                      {Tag::Response, {Tag::Href, Tag::PropStat}},
                                                      {Tag::PropStat, {Tag::Prop, Tag::Status}},
                                                      {Tag::Prop, {Tag::GetLastModified, Tag::ResourceType}},
                                                      {Tag::ResourceType, {Tag::Collection}},
                                                      {Tag::Href, {}},
                                                      {Tag::GetLastModified, {}},
                                                      {Tag::Collection, {}},
                                                      {Tag::Status, {}}};

#ifndef NDEBUG
Parser::Parser() {
    std::tm time;
    const std::vector<QString> test_dates{"Sun, 06 Nov 1999 08:49:37 GMT", "Sunday, 06-Nov-99 08:49:37 GMT", "Sun Nov 6 08:49:37 1999"};
    for (const auto& date : test_dates) {
        time = {};
        time.tm_isdst = -1;
        time = FSObjectStruct::to_tm(date);
        assert(time.tm_sec == 37);
        assert(time.tm_min == 49);
        assert(time.tm_hour == 8);
        assert(time.tm_mday == 6);
        assert(time.tm_mon == 10);
        assert(time.tm_year == 1999 - 1900);
        assert(time.tm_isdst == 0);
    }
    const QString test_date = "Sun, 06 Nov 2024 08:49:37 GMT";
    time = {};
    time.tm_isdst = -1;
    time = FSObjectStruct::to_tm(test_date);
    assert(time.tm_year == 2024 - 1900);

    const QString test_responce =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<D:multistatus xmlns:D=\"DAV:\" xmlns:ns0=\"DAV:\">"
    "<D:response xmlns:lp1=\"DAV:\" xmlns:lp2=\"http://apache.org/dav/props/\" xmlns:g0=\"DAV:\">"
        "<D:href>/dav/</D:href>"
        "<D:propstat>"
            "<D:prop>"
                "<lp1:resourcetype><D:collection/></lp1:resourcetype>"
            "</D:prop>"
            "<D:status>HTTP/1.1 200 OK</D:status>"
        "</D:propstat>"
        "<D:propstat>"
            "<D:status>HTTP/1.1 403 Forbidden</D:status>"
            "<D:prop>"
                "<lp1:getlastmodified />"
            "</D:prop>"
        "</D:propstat>"
    "</D:response>"
    "<D:response xmlns:lp1=\"DAV:\" xmlns:lp2=\"http://apache.org/dav/props/\" xmlns:g0=\"DAV:\">"
        "<D:propstat>"
            "<D:status>HTTP/1.1 200 OK</D:status>"
            "<D:prop>"
                "<lp1:getlastmodified>Mon, 06 Mar 2023 13:49:01 GMT</lp1:getlastmodified>"
                "<lp1:resourcetype><D:collection/></lp1:resourcetype>"
            "</D:prop>"
        "</D:propstat>"
        "<D:href>/dav/%d0%94%d0%b8%d1%81%d0%ba%201/</D:href>"
    "</D:response>"
    "<D:response xmlns:lp1=\"DAV:\" xmlns:lp2=\"http://apache.org/dav/props/\" xmlns:g0=\"DAV:\">"
        "<D:propstat>"
            "<D:prop>"
                "<lp1:getlastmodified>Thu, 09 Mar 2023 06:55:56 GMT</lp1:getlastmodified>"
                "<lp1:resourcetype><D:collection/></lp1:resourcetype>"
            "</D:prop>"
            "<D:status>HTTP/1.1 200 OK</D:status>"
        "</D:propstat>"
        "<D:href>/dav/%d0%94%d0%b8%d1%81%d0%ba%202/</D:href>"
    "</D:response>"
    "<D:response xmlns:lp1=\"DAV:\" xmlns:lp2=\"http://apache.org/dav/props/\" xmlns:g0=\"DAV:\">"
        "<D:propstat>"
            "<D:status>HTTP/1.1 200 OK</D:status>"
            "<D:prop>"
                "<lp1:getlastmodified>Wednesday, 16-Jul-2025 23:59:58 GMT</lp1:getlastmodified>"
            "</D:prop>"
        "</D:propstat>"
        "<D:href>/dav/%d0%94%d0%b8%d1%81%d0%ba%203/</D:href>"
        "<D:propstat>"
            "<D:prop>"
                "<lp1:resourcetype />"
            "</D:prop>"
            "<D:status>HTTP/1.1 401 Unauthorized</D:status>"
        "</D:propstat>"
    "</D:response>"
    "<D:response xmlns:lp1=\"DAV:\" xmlns:lp2=\"http://apache.org/dav/props/\" xmlns:g0=\"DAV:\">"
        "<D:propstat>"
            "<D:prop>"
                "<lp1:getlastmodified>Wednesday, 16-Jul-2025 23:59:58 GMT</lp1:getlastmodified>"
                "<lp1:resourcetype />"
            "</D:prop>"
            "<D:status>HTTP/1.1 200 OK</D:status>"
        "</D:propstat>"
        "<D:href>/dav/%D0%A2%D0%B5%D1%81%D1%82%D0%BE%D0%B2%D1%8B%D0%B9%20%D1%84%D0%B0%D0%B9%D0%BB.txt</D:href>"
    "</D:response>"
"</D:multistatus>";
    const Result result = parse_propfind_reply(test_responce.toLatin1());
    assert(std::get<1>(result) != nullptr);
    const FileSystemObject* const curr_dir_obj = std::get<1>(result).get();
    assert(curr_dir_obj->get_name() == "dav");
    assert(curr_dir_obj->get_type() == FileSystemObject::Type::Directory);
    assert(curr_dir_obj->is_last_modified_valid() == false);

    const Objects& children = std::get<2>(result);
    assert(children.size() == 3);

    auto it = std::begin(children);
    assert(it->get_name() == "Диск 1");
    assert(it->get_type() == FileSystemObject::Type::Directory);
    assert(it->is_last_modified_valid());
    time = it->get_last_modified();
    assert(time.tm_sec == 1);
    assert(time.tm_min == 49);
    assert(time.tm_hour == 13);
    assert(time.tm_mday == 6);
    assert(time.tm_mon == 2);
    assert(time.tm_year == 2023 - 1900);
    assert(time.tm_isdst == 0);

    ++it;
    assert(it->get_name() == "Диск 2");
    assert(it->get_type() == FileSystemObject::Type::Directory);
    assert(it->is_last_modified_valid());
    time = it->get_last_modified();
    assert(time.tm_sec == 56);
    assert(time.tm_min == 55);
    assert(time.tm_hour == 6);
    assert(time.tm_mday == 9);
    assert(time.tm_mon == 2);
    assert(time.tm_year == 2023 - 1900);
    assert(time.tm_isdst == 0);

    ++it;
    assert(it->get_name() == "Тестовый файл.txt");
    assert(it->get_type() == FileSystemObject::Type::File);
    assert(it->is_last_modified_valid());
    time = it->get_last_modified();
    assert(time.tm_sec == 58);
    assert(time.tm_min == 59);
    assert(time.tm_hour == 23);
    assert(time.tm_mday == 16);
    assert(time.tm_mon == 6);
    assert(time.tm_year == 2025 - 1900);
    assert(time.tm_isdst == 0);
}
#endif

Parser::Result Parser::parse_propfind_reply(const QByteArray& data) const  {
    Result result;
    const auto first = _propfind_tag_order.find(Tag::None);
    assert(first != std::end(_propfind_tag_order));
    CurrentState state(first, result);
    QXmlStreamReader reader(data);
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
            case QXmlStreamReader::StartElement: {
                QStringView& not_dav = state.not_dav_namespace;
                if (!not_dav.isNull())
                    continue;

                const QStringView namespace_uri = reader.namespaceUri();
                if (namespace_uri != QStringLiteral("DAV:")) {
                    not_dav = reader.name();
                    continue;
                }
                const auto tag_it = _propfind_tag_by_str_map.find(reader.name().toString());
                const auto not_found = tag_it == std::end(_propfind_tag_by_str_map);
                if (not_found)
                    throw std::runtime_error("unknown tag");

                const Tag tag = tag_it->second;
                const TagSet& possible_tags = state.stack.top()->second;
                if (!possible_tags.contains(tag))
                    throw std::runtime_error("incorrect tag order");

                state.update_if_start_tag(tag);
                const auto possible_tags_it = _propfind_tag_order.find(tag);
                assert(possible_tags_it != std::end(_propfind_tag_order));
                state.stack.push(possible_tags_it);
                break;
            }

            case QXmlStreamReader::EndElement: {
                QStringView& not_dav = state.not_dav_namespace;
                if (!not_dav.isNull()) {
                    if (not_dav == reader.name())
                        not_dav = QStringView();

                    continue;
                }
                const Tag tag = state.stack.top()->first;
                state.update_if_end_tag(tag);
                state.stack.pop();
                break;
            }

            case QXmlStreamReader::Characters: {
                if (!state.not_dav_namespace.isNull() || reader.isCDATA())
                    continue;

                const Tag tag = state.stack.top()->first;
                state.update_if_data(tag, reader.text());
                break;
            }

            default:
                break;
        }
    }
    if (reader.hasError())
        throw std::runtime_error("invalid XML format");

    if (state.errorMsgPair.first)
        qWarning(qUtf8Printable(QObject::tr("An error has occured during reply parse: %s. The reply text: \n%s")), qUtf8Printable(state.errorMsgPair.second), qUtf8Printable(data));

    return result;
}
