/*
 *   regex.cpp
 *
 *   Copyright (C) 2004-2016 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
 *
 *   This file is part of medit.  medit is free software; you can
 *   redistribute it and/or modify it under the terms of the
 *   GNU Lesser General Public License as published by the
 *   Free Software Foundation; either version 2.1 of the License,
 *   or (at your option) any later version.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with medit.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "moocpp/regex.h"
#include "mooutils/mooutils-messages.h"
#include <utility>

namespace g
{

Regex::Regex(GRegex* p)
    : m_p(p)
{
}

Regex::~Regex()
{
    g_regex_unref(m_p);
}

Regex::Regex(Regex&& other)
	: m_p(other.m_p)
{
	other.m_p = nullptr;
}

Regex& Regex::operator=(Regex&& other)
{
	std::swap(m_p, other.m_p);
	return *this;
}

bool Regex::is_valid() const
{
	return m_p != nullptr;
}

Regex::operator bool() const
{
	return m_p != nullptr;
}

Regex Regex::compile(const char* pattern, CompileFlags compile_options, MatchFlags match_options, GError** error)
{
    GRegex* p = g_regex_new(pattern, GRegexCompileFlags(compile_options), GRegexMatchFlags(match_options), error);
	return Regex{p};
}

const char* Regex::get_pattern() const
{
    return g_regex_get_pattern(m_p);
}

int Regex::get_max_backref() const
{
    return g_regex_get_max_backref(m_p);
}

int Regex::get_capture_count() const
{
    return g_regex_get_capture_count(m_p);
}

bool Regex::get_has_cr_or_lf() const
{
    return g_regex_get_has_cr_or_lf(m_p);
}

int Regex::get_max_lookbehind() const
{
    return g_regex_get_max_lookbehind(m_p);
}

int Regex::get_string_number(const char *name) const
{
    return g_regex_get_string_number(m_p, name);
}

gstr Regex::escape_string(const char* string, int length)
{
    return gstr::take(g_regex_escape_string(string, length));
}

gstr Regex::escape_nul(const char* string, int length)
{
    return gstr::take(g_regex_escape_nul(string, length));
}

Regex::CompileFlags Regex::get_compile_flags() const
{
    return CompileFlags(g_regex_get_compile_flags(m_p));
}

Regex::MatchFlags Regex::get_match_flags() const
{
    return MatchFlags(g_regex_get_match_flags(m_p));
}

bool Regex::match(const char *pattern, const char *string, CompileFlags compile_options, MatchFlags match_options)
{
    return g_regex_match_simple(pattern, string, GRegexCompileFlags(compile_options), GRegexMatchFlags(match_options));
}

MatchInfo Regex::match(const char* string, MatchFlags match_options) const
{
    return match(string, -1, 0, match_options, nullptr);
}

MatchInfo Regex::match(const gstr& string, MatchFlags match_options) const
{
    return match(string.get(), match_options);
}

MatchInfo Regex::match(const char* string, ssize_t string_len, int start_position, MatchFlags match_options, GError** error) const
{
    GMatchInfo* match_info = nullptr;
    if (!g_regex_match_full(m_p, string, string_len, start_position, GRegexMatchFlags(match_options), &match_info, error))
		return MatchInfo(*this);
    return MatchInfo(*this, match_info, true);
}

MatchInfo Regex::match_all(const char* string, MatchFlags match_options) const
{
    return match_all(string, -1, 0, match_options, nullptr);
}

MatchInfo Regex::match_all(const char* string, ssize_t string_len, int start_position, MatchFlags match_options, GError** error) const
{
    GMatchInfo* match_info = nullptr;
    if (!g_regex_match_all_full(m_p, string, string_len, start_position, GRegexMatchFlags(match_options), &match_info, error))
        return MatchInfo(*this);
    return MatchInfo(*this, match_info, true);
}

std::vector<gstr> Regex::split(const char* pattern, const char* string, CompileFlags compile_options, MatchFlags match_options)
{
    return gstr::take(g_regex_split_simple(pattern, string, GRegexCompileFlags(compile_options), GRegexMatchFlags(match_options)));
}

std::vector<gstr> Regex::split(const char* string, MatchFlags match_options) const
{
    return gstr::take(g_regex_split(m_p, string, GRegexMatchFlags(match_options)));
}

std::vector<gstr> Regex::split(const char* string, ssize_t string_len, int start_position, MatchFlags match_options, int max_tokens, GError** error) const
{
    return gstr::take(g_regex_split_full(m_p, string, string_len, start_position, GRegexMatchFlags(match_options), max_tokens, error));
}

gstr Regex::replace(const char* string, const char* replacement, MatchFlags match_options) const
{
    return replace(string, -1, 0, replacement, match_options, nullptr);
}

gstr Regex::replace(const char* string, ssize_t string_len, int start_position, const char* replacement, MatchFlags match_options, GError** error) const
{
    return gstr::take(g_regex_replace(m_p, string, string_len, start_position, replacement, GRegexMatchFlags(match_options), error));
}

gstr Regex::replace_literal(const char* string, const char* replacement, MatchFlags match_options) const
{
    return replace_literal(string, -1, 0, replacement, match_options, nullptr);
}

gstr Regex::replace_literal(const char* string, ssize_t string_len, int start_position, const char* replacement, MatchFlags match_options, GError** error) const
{
    return gstr::take(g_regex_replace_literal(m_p, string, string_len, start_position, replacement, GRegexMatchFlags(match_options), error));
}

namespace
{

using EvalFunc = std::function<bool(const MatchInfo&, gstr&)>;

struct EvalFuncData
{
    const Regex& regex;
    const EvalFunc& func;
    
    EvalFuncData(const Regex& regex, const EvalFunc& func)
        : regex(regex)
        , func(func)
    {
    }

    EvalFuncData(const EvalFuncData&) = delete;
    EvalFuncData& operator=(const EvalFuncData&) = delete;
};

static gboolean eval_func(const GMatchInfo* match_info, GString* result, gpointer user_data)
{
    EvalFuncData* data = reinterpret_cast<EvalFuncData*>(user_data);
    gstr replacement;
    MatchInfo mi(data->regex, const_cast<GMatchInfo*>(match_info), false);
    bool retval = data->func(mi, replacement);
    if (!replacement.empty())
        g_string_append(result, replacement.get());
    return retval;
}

}

gstr Regex::replace_eval(const char* string, ssize_t string_len, int start_position, MatchFlags match_options, const std::function<bool(const MatchInfo&, gstr&)>& eval, GError** error) const
{
    EvalFuncData data(*this, eval);
    return gstr::take(g_regex_replace_eval(m_p, string, string_len, start_position, GRegexMatchFlags(match_options), eval_func, &data, error));
}

bool Regex::check_replacement(const char* replacement, bool& has_references, GError** error)
{
    gboolean c_has_references;
    bool retval = g_regex_check_replacement(replacement, &c_has_references, error);
    has_references = c_has_references;
    return retval;
}


MatchInfo::MatchInfo(const Regex& regex)
	: MatchInfo(regex, nullptr, false)
{
}

MatchInfo::MatchInfo(const Regex& regex, GMatchInfo* p, bool take_ownership)
    : m_regex(regex)
    , m_p(p)
    , m_own(take_ownership)
{
	moo_assert(m_p || !m_own);
}

MatchInfo::~MatchInfo()
{
    if (m_own)
        g_match_info_free(m_p);
}

MatchInfo::MatchInfo(MatchInfo&& other)
	: m_regex(other.m_regex)
	, m_p(other.m_p)
	, m_own(other.m_own)
{
	other.m_own = false;
}

MatchInfo& MatchInfo::operator=(MatchInfo&& other)
{
	std::swap(m_regex, other.m_regex);
	std::swap(m_p, other.m_p);
	std::swap(m_own, other.m_own);
	return *this;
}

bool MatchInfo::is_match() const
{
	return m_p != nullptr;
}

MatchInfo::operator bool() const
{
	return is_match();
}

const Regex& MatchInfo::get_regex() const
{
    return m_regex;
}

const char* MatchInfo::get_string() const
{
	g_return_val_if_fail(m_p != nullptr, nullptr);
    return g_match_info_get_string(m_p);
}

bool MatchInfo::next(GError** error)
{
    g_return_val_if_fail(m_own, false);
    return g_match_info_next(m_p, error);
}

bool MatchInfo::matches() const
{
    return g_match_info_matches(m_p);
}

int MatchInfo::get_match_count() const
{
    return g_match_info_get_match_count(m_p);
}

bool MatchInfo::is_partial_match() const
{
    return g_match_info_is_partial_match(m_p);
}

gstr MatchInfo::expand_references(const char* string_to_expand, GError** error) const
{
    return gstr::take(g_match_info_expand_references(m_p, string_to_expand, error));
}

gstr MatchInfo::fetch(int match_num) const
{
    return gstr::take(g_match_info_fetch(m_p, match_num));
}

bool MatchInfo::fetch_pos(int match_num, int& start_pos, int& end_pos) const
{
    return g_match_info_fetch_pos(m_p, match_num, &start_pos, &end_pos);
}

gstr MatchInfo::fetch_named(const char* name) const
{
    return gstr::take(g_match_info_fetch_named(m_p, name));
}

bool MatchInfo::fetch_named_pos(const char* name, int& start_pos, int& end_pos) const
{
    return g_match_info_fetch_named_pos(m_p, name, &start_pos, &end_pos);
}

gstrvec MatchInfo::fetch_all() const
{
    return gstr::take(g_match_info_fetch_all(m_p));
}

} // namespace g
