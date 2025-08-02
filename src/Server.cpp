#include <iostream>
#include <string>
#include <ranges>
#include <algorithm>
#include <vector>

enum PatternType {
    WORD = 0x0,
    DIGIT,
    CHAR,
    POSITIVE_CHR_GROUP,
    NEGATIVE_CHR_GROUP,
    WILDCARD,
    ALTERNATION,
    OPTIONAL,
    WORD_PTTRN,
    GROUP_PTTRN,
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
    std::vector<Group> group;
    int paren = 0;

    auto check_char = [&](PatternType pttrn, char character, std::string_view word = "") -> bool
    {
        switch(pttrn)
        {
            case WILDCARD:           return true;
            case DIGIT:              return isdigit(input[input_idx]);
            case WORD:               return (isalnum(input[input_idx]) or input[input_idx] == '_');
            case CHAR:               return input[input_idx] == character;
            case POSITIVE_CHR_GROUP: return std::ranges::find(char_groups, input[input_idx]) != char_groups.end();
            case NEGATIVE_CHR_GROUP: return std::ranges::find(char_groups, input[input_idx]) == char_groups.end();
            case WORD_PTTRN:
            {
                int idx = 0;
                while (idx < word.length() and word[idx] == input[input_idx + idx])
                    idx++;

                if (idx < word.length()-1)
                    return false;
                return true;
            };
            default: return false;
        }
        return true;
    };

    auto check_group = [&]() -> bool
    {
        for (int i = 0; i < group.size(); ++i)
        {
            auto g = group[i];
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
                if (match_found or (group.size()-1 > i and group[i+1].type == OPTIONAL))
                    continue;
                return false;
            }

            if (check_char(g.type, g.chr))
                input_idx++;
            else if (i < group.size()-1 and group[i+1].type == OPTIONAL)
                continue;
            else
                return false;
        }
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
            if (curr_pattern == GROUP_PTTRN)
                while (check_group());
            else
                while (check_char(curr_pattern, pattern[pttrn_idx-1]) and input_idx < input.length() and input[input_idx + 1] != pattern[pttrn_idx + 2])
                    input_idx++;

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
                curr_pattern = WORD;
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
            }
                
            continue;
        }
        if (paren > 0)
        {
            Group g = {};
            g.type = curr_pattern;
            if (g.type == CHAR)
                g.chr = pattern[pttrn_idx];
            
            if (pattern[pttrn_idx-1] != '(')
            { 
                group.push_back(g);
            	continue;
            }
            int bar_idx = pattern.find('|', pttrn_idx);
            if (bar_idx != -1)
            {
                int open_idx = pattern.find('(', pttrn_idx);
                if (open_idx == -1 or bar_idx < open_idx)
                {
                    g.type = ALTERNATION;
                    int close_idx = pattern.find(')', pttrn_idx);
                    std::string_view thing = pattern.substr(pttrn_idx, close_idx-pttrn_idx);
                    auto split_view = thing | std::views::split('|');
                    for (auto &&range : split_view)
                        g.alternations.emplace_back(range.begin(), range.end());
                    pttrn_idx = close_idx-1;
                }
            }            

            group.push_back(g);
            continue;
        }
        if (curr_pattern == OPTIONAL)
            continue;

        if (found_beg == false)
        {
            found_beg = true;
            if (curr_pattern == DIGIT)
                found_itr = std::ranges::find_if(input, isdigit);

            else if (curr_pattern == WORD)
            {
                found_itr = std::ranges::find_if(input, isalnum);
                if (found_itr == input.end())
                    found_itr = std::ranges::find(input, '_');
            }
            else if (curr_pattern == CHAR)
                found_itr = std::ranges::find(input, pattern[0]);

            else if (curr_pattern == POSITIVE_CHR_GROUP)
                found_itr = std::ranges::find_if(input, [&](char c) { return std::ranges::find(char_groups, c) != char_groups.end(); });
            
            else if (curr_pattern == NEGATIVE_CHR_GROUP)
                found_itr = std::ranges::find_if(input, [&](char c) { return std::ranges::find(char_groups, c) == char_groups.end(); });
            
            if (found_itr == input.end())
                return false;
            input_idx = std::distance(input.begin(), found_itr) + 1;
            continue;
        }


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

    std::string input2 = "I see 1 cat, 2 dogs and 3 cows";
    std::string pattern2 = "^I see (\\d (cat|dog|cow)s?(, | and )?)+$";

    // std::string input2 = "a cat";
    // std::string pattern2 = "a (cat|dog)";
    auto res = match_pattern(input2, pattern2);

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
