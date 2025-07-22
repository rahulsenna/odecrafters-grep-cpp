#include <iostream>
#include <string>
#include <ranges>
#include <algorithm>

bool match_pattern(const std::string &input_line, const std::string &pattern)
{
    if (pattern.length() == 1)
    {
        return input_line.find(pattern) != std::string::npos;
    }
    else if (pattern == "\\d")
    {
        return std::ranges::find_if(input_line, isdigit) != input_line.end();
    }
    else if (pattern == "\\w")
    {
        return std::ranges::find_if(input_line, [](char c) { return std::isalnum(c) || c == '_'; }) != input_line.end();
    }
    else if (pattern.front() == '[' and pattern.back() == ']')
    {
        if (pattern[1] == '^')
            return not std::ranges::all_of(pattern.begin()+2, pattern.end()-1,
                                           [&](char c) { return input_line.contains(c); });
        
        return std::ranges::any_of(pattern.begin()+1, pattern.end()-1,
                                           [&](char c) { return input_line.contains(c); });
    }
    else if (pattern.starts_with("\\d"))
    {
        auto loc = std::ranges::find_if(input_line, isdigit);
        if (loc == input_line.end())
            return false;

        int pos = distance(input_line.begin(), loc);
        for (int i = 0; i < pattern.length(); ++i)
        {
            char chr = input_line[pos++];
            char p = pattern[i];

            if (p == chr)
                continue;

            if (p != '\\')
                return false;

            p = pattern[++i];
            if (p == 'd' and isdigit(chr))
                continue;
            
            if (p == 'w' and (isalnum(chr) or chr == '_'))
                continue;
            
            return false;
        }
        return true;
    }
    else
    {
        throw std::runtime_error("Unhandled pattern " + pattern);
    }
}

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
