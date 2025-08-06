#include <iostream>
#include <string>
#include <ranges>
#include <algorithm>
#include <vector>
#include <deque>
#include <functional>
#include <format>
#include <functional>
#include <fstream>

enum PatternType {
    WORD_CHAR = 0x0,
    DIGIT,
    CHAR,
    POSITIVE_CHR_GROUP,
    NEGATIVE_CHR_GROUP,
    WILDCARD,
    ALTERNATION,
    OPTIONAL,
    WORD_PTTRN,
    GROUP_PTTRN,
    BACK_REF,
    PLUS,
    STAR,
    PAREN_OPEN,
    PAREN_CLOSE,
};

typedef struct
{
	PatternType type;
    char chr;
    std::vector<std::string_view> alternations;
    int backet_idx;
    char terminate;
} Group;

bool match_pattern(const std::string_view input, const std::string_view pattern)
{
    int input_idx = 0;
    auto found_itr = input.begin();
    bool found_beg = false;

    PatternType curr_pattern = CHAR;
    std::vector<std::vector<char>> brackets;
    std::vector<char> char_groups;
    std::vector<std::vector<Group>> group;
    group.push_back({});
    int paren = 0;
    int back_ref = -1;
    std::vector<std::string_view> captures;

    auto build_group = [&](int &pttrn_idx) -> void
    {
        Group g = {};
        g.type = curr_pattern;
        if (g.type == CHAR)
            g.chr = pattern[pttrn_idx];

        if (g.type == POSITIVE_CHR_GROUP or g.type == NEGATIVE_CHR_GROUP)
        {
            g.backet_idx = brackets.size()-1;
        }
        if (g.type == NEGATIVE_CHR_GROUP)
        { 
            int pi =pttrn_idx +1;
            while(pattern[pi] == '+' or pattern[pi] == ')')
                pi++;
        	g.terminate = pattern[pi];
        }

        if (pattern[pttrn_idx - 1] != '(')
        {
            group.back().push_back(g);
            return;
        }
        int bar_idx = pattern.find('|', pttrn_idx);
        if (bar_idx != -1)
        {
            int open_idx = pattern.find('(', pttrn_idx);
            if (open_idx == -1 or bar_idx < open_idx)
            {
                g.type = ALTERNATION;
                int close_idx = pattern.find(')', pttrn_idx);
                std::string_view thing = pattern.substr(pttrn_idx, close_idx - pttrn_idx);
                auto split_view = thing | std::views::split('|');
                for (auto &&range : split_view)
                    g.alternations.emplace_back(range.begin(), range.end());
                pttrn_idx = close_idx - 1;
            }
        }

        group.back().push_back(g);
    };

    auto check_char = [&](PatternType pttrn, char character, std::string_view word = "") -> bool
    {
        if (found_beg == false)
        {
            found_beg = true;
            switch(pttrn)
            {
                case DIGIT:              found_itr = std::ranges::find_if(input, isdigit); break;
                case CHAR:               found_itr = std::ranges::find(input, character);  break;
                case POSITIVE_CHR_GROUP: found_itr = std::ranges::find_if(input, [&](char c) { return std::ranges::find(char_groups, c) != char_groups.end(); }); break;
                case NEGATIVE_CHR_GROUP: found_itr = std::ranges::find_if(input, [&](char c) { return std::ranges::find(char_groups, c) == char_groups.end(); }); break;
                case WORD_CHAR:
                {
                    found_itr = std::ranges::find_if(input, isalnum);
                    if (found_itr == input.end())
                        found_itr = std::ranges::find(input, '_');
                }; break;
                default: break;                                
            }
            
            if (found_itr == input.end())
                return false;
            input_idx = std::distance(input.begin(), found_itr);
        }
        switch(pttrn)
        {
            case WILDCARD:           return true;
            case DIGIT:              return isdigit(input[input_idx]);
            case WORD_CHAR:          return (isalnum(input[input_idx]) or input[input_idx] == '_');
            case CHAR:               return input[input_idx] == character;
            case POSITIVE_CHR_GROUP: return std::ranges::find(char_groups, input[input_idx]) != char_groups.end();
            case NEGATIVE_CHR_GROUP: return std::ranges::find(char_groups, input[input_idx]) == char_groups.end();
            case WORD_PTTRN:
            {
                int idx = 0;
                while (idx < word.length() and (word[idx] == input[input_idx + idx] or word[idx] == '.'))
                    idx++;

                if (idx < word.length()-1)
                    return false;
                return true;
            };
            case BACK_REF:
            {
                int idx = input_idx;
                auto ref = captures[back_ref-1];
                for (auto c: ref)
                {
                	if (c != input[idx++])
                        return false;
                }
                input_idx = idx-1;
                return true;
            };
            default: return false;
        }

        return true;
    };

    std::deque<std::pair<int,int>> depth;
    std::function<bool()> check_group = [&]() -> bool
    {
        int cap_count = captures.size();
        captures.push_back({});
        int cap_start = input_idx;
        Group prev;
        for (int i = 0; i < group.back().size(); ++i)
        {
            auto g = group.back()[i];
            if (g.type == POSITIVE_CHR_GROUP or g.type == NEGATIVE_CHR_GROUP)
            {
                char_groups = brackets[g.backet_idx];
            }

            if (g.type == PAREN_OPEN)
            {
                depth.push_back({captures.size(), input_idx});
                captures.push_back({});                
                continue;
            }
            if (g.type == PAREN_CLOSE)
            {
                auto [a,b] = depth.front();
                depth.pop_front();
                captures[a] = input.substr(b, input_idx-b);
                continue;
            }
            if (g.type == PLUS)
            {
                if (prev.type == NEGATIVE_CHR_GROUP)                
                    char_groups.push_back(prev.terminate);

                while (check_char(prev.type, prev.chr))
                    input_idx++;
                continue;
            }
            prev = g;
            if (g.type == OPTIONAL)
                continue;
            
            if (g.type == ALTERNATION)
            {
                bool match_found = false;
                for (auto &word : g.alternations)
                {
                    if (check_char(WORD_PTTRN, g.chr, word))
                    {
                        match_found = true;
                        input_idx += word.length();
                        break;
                    }
                }
                if (match_found or (group.back().size()-1 > i and group.back()[i+1].type == OPTIONAL))
                    continue;
                return false;
            }

            if (check_char(g.type, g.chr))
                input_idx++;
            else if (i < group.back().size()-1 and group.back()[i+1].type == OPTIONAL)
                continue;
            else
                return false;
        }
        captures[cap_count] = input.substr(cap_start, input_idx-cap_start);
        return true;
    };

    for (int pttrn_idx = 0; pttrn_idx < pattern.length(); ++pttrn_idx)
    {
        switch (pattern[pttrn_idx])
        {
            case '^':found_beg = true; continue;
            case '$':
                if (input_idx < input.length())
                    return false;
                continue;
            case '+':
            case '*':
                if (paren > 0)
                {
                    curr_pattern = PLUS;
                    build_group(pttrn_idx);
                }                
                else
                {
                    if (curr_pattern == GROUP_PTTRN)
                    {
                        group.pop_back();
                        while (check_group());
                        group.push_back({});
                    }
                    else
                    {
                        bool tail_match = false;
                        int tail_match_i = pttrn_idx + 1;
                        std::string tail = "";
                        int p = pttrn_idx + 1;
                        while (p < pattern.length() and (isalnum(pattern[p]) or pattern[p] == '_'))
                        {
                            tail += pattern[p++];
                        }
                        int stop_len = input.length();
                        if (not tail.empty())
                        {
                            int fidx = input.find(tail, input_idx);
                            if (fidx != -1)
                                stop_len = fidx;
                        }

                        while (check_char(curr_pattern, pattern[pttrn_idx - 1]) && input_idx < stop_len)
                        {
                            input_idx++;

                        }
                    }
                }
                continue;
                
            case '?': curr_pattern = OPTIONAL; break;
            case '.': curr_pattern = WILDCARD; break;
            case '\\':
                pttrn_idx += 1;
                switch (pattern[pttrn_idx])
                {
                    case 'd':
                        curr_pattern = DIGIT;
                        break;
                    case 'w':
                        curr_pattern = WORD_CHAR;
                        break;
                    default:
                        if (isdigit(pattern[pttrn_idx]))
                        {
                            curr_pattern = BACK_REF;
                            back_ref = pattern[pttrn_idx] - '0';
                        }
                        break;
                }
                break;
                
            case '[':
                curr_pattern = POSITIVE_CHR_GROUP;
                if (pattern[pttrn_idx + 1] == '^')
                {
                    curr_pattern = NEGATIVE_CHR_GROUP;
                    pttrn_idx++;
                }

                brackets.push_back({});
                while (pattern[++pttrn_idx] != ']')
                {
                    brackets.back().push_back(pattern[pttrn_idx]);
                }
                char_groups = brackets.back();
                break;
                
            case '(':
                if (paren > 0)
                {
                    curr_pattern = PAREN_OPEN;
                    build_group(pttrn_idx);
                }
                paren++;
                continue;
                
            case ')':
                paren--;
                if (paren > 0)
                {
                    curr_pattern = PAREN_CLOSE;
                    build_group(pttrn_idx);
                }
                if (paren == 0)
                {
                    curr_pattern = GROUP_PTTRN;
                    if (not check_group())
                        return false;
                    group.push_back({});
                }
                continue;
                
            default:
                curr_pattern = CHAR;
                break;
        }
        if (paren > 0)
        {
            build_group(pttrn_idx);
            continue;
        }
        if (curr_pattern == OPTIONAL)
            continue;

        if (check_char(curr_pattern, pattern[pttrn_idx]))
            input_idx++;
        else if (pattern[pttrn_idx + 1] == '?')
            pttrn_idx++;
        else
            return false;
    }

    return true;
}

int main(int argc, char *argv[])
{ 
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here" << std::endl;

    if (argc < 3)
    {
        std::cerr << "Expected two arguments" << std::endl;
        return 1;
    }

    std::string flag = argv[1];
    std::string pattern = argv[2];

    if (flag != "-E")
    {
        std::cerr << "Expected first argument to be '-E'" << std::endl;
        return 1;
    }

    std::string filename = "";
    if (argc>=4)
        filename = argv[3];

    if (not filename.empty())
    {
        std::ifstream file(filename);

        if (not file.is_open())
        {
            std::cerr << " Failed to open file " << '\n';
        }

        std::string input_line;
        int res = 1;

        while(std::getline(file, input_line))
        {
            if (match_pattern(input_line, pattern))
            {
                std::cout << input_line << '\n';
                res = 0;
            }
        }
        return res;

    }

    std::string input_line;
    std::getline(std::cin, input_line);

    try
    {
        if (match_pattern(input_line, pattern))
        {
            std::cerr << "found" << std::endl;
            return 0;
        }
        else
        {
            std::cerr << "not found" << std::endl;
            return 1;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
