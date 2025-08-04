#include <iostream>
#include <string>
#include <ranges>
#include <algorithm>
#include <vector>

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
};

typedef struct
{
	PatternType type;
    char chr;
    std::vector<std::string_view> alternations;

} Group;

bool match_pattern(const std::string_view input, const std::string_view pattern)
{
    int input_idx = 0;
    auto found_itr = input.begin();
    bool found_beg = false;

    PatternType curr_pattern = CHAR;
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

    auto check_group = [&]() -> bool
    {
        int cap_start = input_idx;
        Group prev;
        for (int i = 0; i < group.back().size(); ++i)
        {
            auto g = group.back()[i];
            if (g.type == PLUS)
            {
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
        captures.push_back(input.substr(cap_start, input_idx-cap_start));
        return true;
    };

    for (int pttrn_idx = 0; pttrn_idx < pattern.length(); ++pttrn_idx)
    {

        if (pattern[pttrn_idx] == '^')
        {
            found_beg = true;
            continue;
        }
        if (pattern[pttrn_idx] == '$')
        {
            if (input_idx < input.length())
                return false;
            continue;
        }

        if (pattern[pttrn_idx] == '+')
        {
            if (paren > 0 )
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
                    while (check_char(curr_pattern, pattern[pttrn_idx - 1]) and input_idx < input.length() and input[input_idx + 1] != pattern[pttrn_idx + 2])
                        input_idx++;
            }
            continue;
        }

        curr_pattern = CHAR;
        if (pattern[pttrn_idx] == '?')
            curr_pattern = OPTIONAL;

        if (pattern[pttrn_idx] == '.')
            curr_pattern = WILDCARD;

        if (pattern[pttrn_idx] == '\\')
        {
            pttrn_idx += 1;
            if (pattern[pttrn_idx] == 'd')
                curr_pattern = DIGIT;

            else if (pattern[pttrn_idx] == 'w')
                curr_pattern = WORD_CHAR;
            
            else if (isdigit(pattern[pttrn_idx]))
            {
                curr_pattern = BACK_REF;
                back_ref = pattern[pttrn_idx] - '0';
            }
        }
        if (pattern[pttrn_idx] == '[')
        {
            curr_pattern = POSITIVE_CHR_GROUP;
            if (pattern[pttrn_idx + 1] == '^')
            {
                curr_pattern = NEGATIVE_CHR_GROUP;
                pttrn_idx++;
            }

            char_groups.clear();
            while (pattern[++pttrn_idx] != ']')
            {
                char_groups.push_back(pattern[pttrn_idx]);
            }
        }

        if (pattern[pttrn_idx] == '(')
        {
            paren++;
            continue;
        }
        if (pattern[pttrn_idx] == ')')
        {
            paren--;
            if (paren == 0)
            {
                curr_pattern = GROUP_PTTRN;
                if (not check_group())
                    return false;
                group.push_back({});
            }
            
            continue;
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

typedef struct
{
	std::string_view pattern;
	std::string_view input;
    size_t pttrn_idx;
    size_t input_idx;
} thing;

int main(int argc, char *argv[])
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here" << std::endl;

    if (argc != 3)
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
