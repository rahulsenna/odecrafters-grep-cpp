#include <iostream>
#include <string>
#include <ranges>
#include <algorithm>
#include <vector>

enum Patterns{
    w = 0x0,
    d,
    chr,
    positive_chr_group,
    negative_chr_group,
    wildcard
};


bool match_pattern(const std::string_view input, const std::string_view pattern)
{
    int input_idx = 0;
    auto found_itr = input.begin();
    bool found_beg = false;

    Patterns curr_pattern = chr;
    std::vector<char> char_groups;

    auto check_char = [&](int pttrn_idx) -> bool
    {
        if (curr_pattern == wildcard)
        	return true;
        
        if (curr_pattern == d)
        {
            if (not isdigit(input[input_idx]))
                return false;
        }
        else if (curr_pattern == w)
        {
            if (not (isalnum(input[input_idx]) or input[input_idx] == '_'))
                return false;
        }
        else if (curr_pattern == chr)
        {
            if (input[input_idx] != pattern[pttrn_idx])
                return false;
        }
        else if (curr_pattern == positive_chr_group)
        {
            if (std::ranges::find(char_groups, input[input_idx]) == char_groups.end())
                return false;
        }
        else if (curr_pattern == negative_chr_group)
        {
            if (std::ranges::find(char_groups, input[input_idx]) != char_groups.end())
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
            while (check_char(pttrn_idx - 1) and input_idx < input.length() and input[input_idx + 1] != pattern[pttrn_idx + 2])
                input_idx++;

            continue;
        }
        if (pattern[pttrn_idx] == '?')
            continue;

        curr_pattern = chr;

        if (pattern[pttrn_idx] == '.')
            curr_pattern = wildcard;

        if (pattern[pttrn_idx] == '\\')
        {
            pttrn_idx += 1;
            if (pattern[pttrn_idx] == 'd')
                curr_pattern = d;

            else if (pattern[pttrn_idx] == 'w')
                curr_pattern = w;
        }
        if (pattern[pttrn_idx] == '[')
        {
            curr_pattern = positive_chr_group;
            if (pattern[pttrn_idx + 1] == '^')
            {
                curr_pattern = negative_chr_group;
                pttrn_idx++;
            }

            char_groups.clear();
            while (pattern[++pttrn_idx] != ']')
            {
                char_groups.push_back(pattern[pttrn_idx]);
            }
        }
        if (found_beg == false)
        {
            found_beg = true;
            if (curr_pattern == d)
                found_itr = std::ranges::find_if(input, isdigit);

            else if (curr_pattern == w)
            {
                found_itr = std::ranges::find_if(input, isalnum);
                if (found_itr == input.end())
                    found_itr = std::ranges::find(input, '_');
            }
            else if (curr_pattern == chr)
                found_itr = std::ranges::find(input, pattern[0]);

            else if (curr_pattern == positive_chr_group)
                found_itr = std::ranges::find_if(input, [&](char c) { return std::ranges::find(char_groups, c) != char_groups.end(); });
            
            else if (curr_pattern == negative_chr_group)
                found_itr = std::ranges::find_if(input, [&](char c) { return std::ranges::find(char_groups, c) == char_groups.end(); });
            
            if (found_itr == input.end())
                return false;
            input_idx = std::distance(input.begin(), found_itr) + 1;
            continue;
        }

        if (check_char(pttrn_idx))
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

    std::string input2 = "goøö0Ogol";
    std::string pattern2 = "g.+gol";
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
